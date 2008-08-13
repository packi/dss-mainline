/*
 *  ds485.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/29/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "ds485.h"

#include "../core/base.h"
#include "../core/logger.h"
#include "../core/ds485const.h"

#include <stdexcept>
#include <sstream>
#include <iostream>

#include <time.h>

#include <boost/scoped_ptr.hpp>

using namespace std;

namespace dss {

  const unsigned char FrameStart = 0xFD;
  const unsigned char EscapeCharacter = 0xFC;

  //================================================== DS485Payload

  template<>
  void DS485Payload::Add(uint8 _data) {
    m_Data.push_back(_data);
  }

  template<>
  void DS485Payload::Add(devid_t _data) {
    m_Data.push_back(_data);
  }

  template<>
  void DS485Payload::Add(bool _data) {
    m_Data.push_back(_data);
  }

  template<>
  void DS485Payload::Add(dsid_t _data) {
    m_Data.push_back((_data & 0xFF000000) >> 24);
    m_Data.push_back((_data & 0x00FF0000) >> 16);
    m_Data.push_back((_data & 0x0000FF00) >> 8);
    m_Data.push_back((_data & 0x000000FF) >> 0);
  }

  int DS485Payload::Size() const {
    return m_Data.size();
  }

  vector<unsigned char> DS485Payload::ToChar() const {
    return m_Data;
  } // GetData


  //================================================== DS485Header

  vector<unsigned char> DS485Header::ToChar() const {
    vector<unsigned char> result;
    result.push_back(FrameStart);
    unsigned char tmp = (m_Destination << 2) | ((m_Broadcast & 0x01) << 1) | (m_Type & 0x01);
    result.push_back(tmp);
    tmp = ((m_Source & 0x3F) << 2) | (m_Counter & 0x03);
    result.push_back(tmp);

    return result;
  } // ToChar

  void DS485Header::FromChar(const unsigned char* _data, const int _len) {
    if(_len < 3) {
      throw invalid_argument("_len must be > 3");
    }
    SetDestination((_data[1] >> 2) & 0x3F);
    SetBroadcast(_data[1] & 0x02 == 0x02);
    SetSource((_data[2] >> 2) & 0x3F);
    SetCounter(_data[2] & 0x03);
  } // FromChar


  //================================================== DS485Frame

  vector<unsigned char> DS485Frame::ToChar() const {
    vector<unsigned char> result = m_Header.ToChar();
    return result;
  } // ToChar

  DS485Payload& DS485Frame::GetPayload() {
    return m_Payload;
  } // GetPayload

  const DS485Payload& DS485Frame::GetPayload() const {
    return m_Payload;
  } // GetPayload


  //================================================== DS485CommandFrame

  DS485CommandFrame::DS485CommandFrame()
  : m_Length(0xFF)
  {
    GetHeader().SetType(1);
  } // ctor

  vector<unsigned char> DS485CommandFrame::ToChar() const {
    vector<unsigned char> result = DS485Frame::ToChar();
    unsigned char tmp;
    if(m_Length == 0xFF) {
      tmp = (m_Command << 4) | (GetPayload().Size() & 0x0F);
    } else {
      tmp = (m_Command << 4) | (m_Length & 0x0F);
    }
    result.push_back(tmp);

    vector<unsigned char> payload = GetPayload().ToChar();
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
  } // ToChar

  //================================================== DS485Controller

  DS485Frame* DS485Controller::GetFrameFromWire() {
    DS485Frame* result = m_FrameReader.GetFrame();
    DS485CommandFrame* cmdFrame = dynamic_cast<DS485CommandFrame*>(result);
    if(cmdFrame != NULL) {
     // Logger::GetInstance()->Log("Command received");
      //Logger::GetInstance()->Log(string("command: ") + CommandToString(cmdFrame->GetCommand()));
      //Logger::GetInstance()->Log(string("length:  ") + IntToString((cmdFrame->GetLength())));
      //Logger::GetInstance()->Log(string("msg nr:  ") + IntToString(cmdFrame->GetHeader().GetCounter()));
    } else {
      cout << "+";
      flush(cout);
      //Logger::GetInstance()->Log("token received");
    }
    return result;
  } // GetFrameFromWire

  bool DS485Controller::PutFrameOnWire(const DS485Frame* _pFrame, bool _freeFrame) {
    vector<unsigned char> chars = _pFrame->ToChar();
    uint16_t crc = 0x0000;

    unsigned int numChars = chars.size();
    for(unsigned int iChar = 0; iChar < numChars; iChar++) {
      unsigned char c = chars[iChar];
      crc = update_crc(crc, c);
      // escape if it's a reserved character and not the first (frame start)
      if(((c == FrameStart) || (c == EscapeCharacter)) && iChar) {
        m_SerialCom->PutChar(0xFC);
        // mask out the msb
        m_SerialCom->PutChar(c & 0x7F);
      } else {
        m_SerialCom->PutChar(c);
      }
    }

    if(dynamic_cast<const DS485CommandFrame*>(_pFrame) != NULL) {
      // send crc
      unsigned char c = static_cast<unsigned char>(crc & 0xFF);
      if(((c == FrameStart) || (c == EscapeCharacter))) {
        m_SerialCom->PutChar(0xFC);
        // mask out the msb
        m_SerialCom->PutChar(c & 0x7F);
      } else {
        m_SerialCom->PutChar(c);
      }
      c = static_cast<unsigned char>((crc >> 8) & 0xFF);
      if(((c == FrameStart) || (c == EscapeCharacter))) {
        m_SerialCom->PutChar(0xFC);
        // mask out the msb
        m_SerialCom->PutChar(c & 0x7F);
      } else {
        m_SerialCom->PutChar(c);
      }
    }
    if(_freeFrame) {
      delete _pFrame;
    }
    return true;
  } // PutFrameOnWire

  DS485Controller::DS485Controller()
  : Thread(true, "DS485Controller")
  {
  } // ctor

  DS485Controller::~DS485Controller() {
  } // dtor


  struct null_deleter {
        void operator()(const void *) {} // Do-nothing operator()
  };

  void DS485Controller::Execute() {
    m_SerialCom.reset(new SerialCom());
    if(!m_SerialCom->Open("/dev/ttyUSB0")) {
      throw runtime_error("could not open serial port");
    }
    m_FrameReader.SetSerialCom(m_SerialCom);
    m_FrameReader.Run();
    DoChangeState(csInitial);
    m_StationID = 0xFF;
    m_NextStationID = 0xFF;
    m_TokenCounter = 0;
    time_t responseSentAt;
    time_t tokenReceivedAt;

    // TODO: read from somewhere
    uint32_t dsid = 0xdeadbeef;

    // prepare token frame
    boost::scoped_ptr<DS485Frame> token(new DS485Frame());
    // prepare solicit successor response frame
    boost::scoped_ptr<DS485CommandFrame> solicitSuccessorResponseFrame(new DS485CommandFrame());
    solicitSuccessorResponseFrame->GetHeader().SetDestination(0);
    solicitSuccessorResponseFrame->GetHeader().SetSource(0x3F);
    solicitSuccessorResponseFrame->SetCommand(CommandSolicitSuccessorResponse);
    solicitSuccessorResponseFrame->GetPayload().Add<uint8>(static_cast<uint8>((dsid >> 24) & 0x000000FF));
    solicitSuccessorResponseFrame->GetPayload().Add<uint8>(static_cast<uint8>((dsid >> 16) & 0x000000FF));
    solicitSuccessorResponseFrame->GetPayload().Add<uint8>(static_cast<uint8>((dsid >> 18) & 0x000000FF));
    solicitSuccessorResponseFrame->GetPayload().Add<uint8>(static_cast<uint8>((dsid >>  0) & 0x000000FF));

    int senseTimeMS = 0;
    int numberOfJoinPacketsToWait = -1;

    while(!m_Terminated) {

      if(m_State == csInitial) {
        senseTimeMS = (rand() % 1000) + 100;
        DoChangeState(csSensing);
        continue;
      } else if(m_State == csSensing) {
        if(m_FrameReader.GetAndResetTraffic()) {
          Logger::GetInstance()->Log("Sensed traffic on the line, changing to csSlaveWaitingToJoin");
          DoChangeState(csSlaveWaitingToJoin);
        }
        if(senseTimeMS == 0) {
          Logger::GetInstance()->Log("No traffic on line, I'll be your master today");
          DoChangeState(csDesignatedMaster);
        }
        SleepMS(senseTimeMS);
        senseTimeMS = 0;
        continue;
      }

      if(m_FrameReader.WaitForFrame()) {
        boost::scoped_ptr<DS485Frame> frame(GetFrameFromWire());
        DS485Header& header = frame->GetHeader();
        DS485CommandFrame* cmdFrame = dynamic_cast<DS485CommandFrame*>(frame.get());

        // discard packets which are not adressed to us
        if(!header.IsBroadcast() && header.GetDestination() != m_StationID) {
          Logger::GetInstance()->Log("packet not for me, discarding");
          cout << "dest: " << header.GetDestination() << endl;
          cout << "src:  " << header.GetSource() << endl;
          if(cmdFrame != NULL) {
            cout << "cmd:  " << CommandToString(cmdFrame->GetCommand()) << endl;
          }
          continue;
        }

        // handle cases in which we're obliged to act on disregarding our current state
        if(cmdFrame != NULL) {
          if(cmdFrame->GetCommand() == CommandGetAddressRequest) {
            if(header.GetDestination() == m_StationID) {
              DS485CommandFrame* frameToSend = new DS485CommandFrame();
              frameToSend->GetHeader().SetDestination(header.GetSource());
              frameToSend->GetHeader().SetSource(m_StationID);
              frameToSend->SetCommand(CommandGetAddressResponse);
              PutFrameOnWire(frameToSend);
              if(header.IsBroadcast()) {
                cerr << "Get address request with bc-flag set" << endl;
              } else {
                cout << "sent response to get address thingie" << endl;
              }
            } else {
              //cout << "not my address " << header.GetDestination() << endl;
            }
          }
        }

        switch(m_State) {
        case csInitial:
          break;
        case csSensing:
          break;
        case csSlaveWaitingToJoin:
          {
            if(cmdFrame != NULL) {
              if(cmdFrame->GetCommand() == CommandSolicitSuccessorRequest) {
                // if it's the first of it's kind, determine how many we've got to skip
                if(numberOfJoinPacketsToWait == -1) {
                  numberOfJoinPacketsToWait = rand() % 10 + 10;
                  cout << "** Waiting for " << numberOfJoinPacketsToWait << endl;
                } else {
                  numberOfJoinPacketsToWait--;
                  if(numberOfJoinPacketsToWait == 0) {
                    m_StationID = 0x3F;
                    PutFrameOnWire(solicitSuccessorResponseFrame.get(), false);
                    //cout << "******* FRAME AWAY ******" << endl;
                    DoChangeState(csSlaveJoining);
                    time(&responseSentAt);
                  }
                  //cout << numberOfJoinPacketsToWait << endl;
                }
              }
            } else {
              cerr << "not a cmdframe" << endl;
            }
          }
          break;
        case csSlaveJoining:
          if(cmdFrame != NULL) {
            DS485Payload& payload = cmdFrame->GetPayload();
            if(cmdFrame->GetCommand() == CommandSetDeviceAddressRequest) {
              m_StationID = payload.ToChar().at(0);
              DS485CommandFrame* frameToSend = new DS485CommandFrame();
              frameToSend->GetHeader().SetDestination(0);
              frameToSend->GetHeader().SetSource(m_StationID);
              frameToSend->SetCommand(CommandSetDeviceAddressResponse);
              PutFrameOnWire(frameToSend);
              cout << "### new address " << m_StationID << endl;
            } else if(cmdFrame->GetCommand() == CommandSetSuccessorAddressRequest) {
              // TODO: handle these in slave and master mode too
              m_NextStationID = payload.ToChar().at(0);
              DS485CommandFrame* frameToSend = new DS485CommandFrame();
              frameToSend->GetHeader().SetDestination(0);
              frameToSend->GetHeader().SetSource(m_StationID);
              frameToSend->SetCommand(CommandSetSuccessorAddressResponse);
              PutFrameOnWire(frameToSend);
            } else {
              // check if our response has timed-out
              time_t now;
              time(&now);
              if((now - responseSentAt) > 1) {
                DoChangeState(csSlaveWaitingToJoin);
                cerr << "çççççççççç haven't received my adress" << endl;
              }
            }
            if(m_StationID != 0x3F && m_NextStationID != 0xFF) {
              Logger::GetInstance()->Log("######### sucessfully joined the network", lsInfo);
              token->GetHeader().SetDestination(m_NextStationID);
              token->GetHeader().SetSource(m_StationID);
              DoChangeState(csSlaveWaitingForFirstToken);
            }
          }
          break;
        case csBroadcastingDSID:
          break;
        case csMaster:
          break;
        case csSlave:
          if(cmdFrame == NULL) {
            // it's a token
            if(!m_PendingFrames.empty() && (m_TokenCounter > 10)) {

              // send frame
              DS485CommandFrame& frameToSend = m_PendingFrames.front();
              PutFrameOnWire(&frameToSend, false);
              cout << "p%" << frameToSend.GetCommand() << "%";
              //m_PendingFrames.erase(m_PendingFrames.begin());


              // if not a broadcast, wait for ack, etc
              if(frameToSend.GetHeader().IsBroadcast()) {
                m_PendingFrames.erase(m_PendingFrames.begin());
              } else {
               // if(!m_FrameReader.HasFrame()) {
               //   m_FrameReader.WaitForFrame();
               // }
                m_PendingFrames.erase(m_PendingFrames.begin());

                //boost::scoped_ptr<DS485Frame> respFrame(GetFrameFromWire());
                //DS485CommandFrame* respFrameCmd = dynamic_cast<DS485CommandFrame*>(respFrame.get());
                //if()
              }

            }
            PutFrameOnWire(token.get(), false);
            cout << ".";
            flush(cout);
            time(&tokenReceivedAt);
            m_TokenEvent.Broadcast();
            m_TokenCounter++;
          } else {

            // Handle token timeout
            time_t now;
            time(&now);
            if(now - tokenReceivedAt > 1) {
              cerr << "restarting" << endl;
              DoChangeState(csInitial);
              m_NextStationID = 0xFF;
              m_StationID = 0xFF;
            }
            cout << "f*" << cmdFrame->GetCommand() << "*";

            bool keep = false;

            if(cmdFrame->GetCommand() == CommandResponse) {
              if(!cmdFrame->GetHeader().IsBroadcast()) {
                // send ack if it's a response and not a broadcasted one
                DS485CommandFrame* ack = new DS485CommandFrame();
                ack->GetHeader().SetSource(m_StationID);
                ack->GetHeader().SetDestination(cmdFrame->GetHeader().GetSource());
                ack->SetCommand(CommandAck);
                PutFrameOnWire(ack);
                cout << "a(r)";
              }
              keep = true;
            } else if(cmdFrame->GetCommand() == CommandRequest) {
              if(!cmdFrame->GetHeader().IsBroadcast()) {
                DS485CommandFrame* ack = new DS485CommandFrame();
                ack->GetHeader().SetSource(m_StationID);
                ack->GetHeader().SetDestination(cmdFrame->GetHeader().GetSource());
                ack->SetCommand(CommandAck);
                PutFrameOnWire(ack);
                cout << "<sid>";
              }
              keep = true;
            }
            if(keep) {
              // put in into the received queue
              AddToReceivedQueue(cmdFrame);
              cout << "k";
            }
            flush(cout);
          }
          break;
        case csSlaveWaitingForFirstToken:
          if(cmdFrame == NULL) {
            PutFrameOnWire(token.get(), false);
            m_TokenCounter = 0;
            DoChangeState(csSlave);
            time(&tokenReceivedAt);
            cout << ".";
            flush(cout);
          }
          break;
        case csDesignatedMaster:
          SleepMS(1000);
          break;
        default:
          throw runtime_error("invalid value for m_State");
        }
      }
    }
  } // Execute

  // TODO: look into boost::weak_ptr
  void DS485Controller::AddToReceivedQueue(DS485CommandFrame* _frame) {
    // Signal our listeners
    DS485CommandFrame* frame = new DS485CommandFrame();
    *frame = *_frame;
    DistributeFrame(boost::shared_ptr<DS485CommandFrame>(frame));
    m_CommandFrameEvent.Signal();
  } // AddToReceivedQueue

  void DS485Controller::DoChangeState(aControllerState _newState) {
    if(_newState != m_State) {
      m_State = _newState;
      m_ControllerEvent.Signal();
    }
  } // DoChangeState

  void DS485Controller::EnqueueFrame(DS485CommandFrame& _frame) {
    //Logger::GetInstance()->Log("Frame queued");
    DS485CommandFrame* frame = new DS485CommandFrame();
    *frame = _frame;
    frame->GetHeader().SetSource(m_StationID);
    m_PendingFrames.push_back(frame);
  } // EnqueueFrame

  bool DS485Controller::WaitForEvent(const int _timeoutMS) {
    return m_ControllerEvent.WaitFor(_timeoutMS);
  } // WaitForEvent

  aControllerState DS485Controller::GetState() const {
    return m_State;
  } // GetState

  void DS485Controller::WaitForCommandFrame() {
    m_CommandFrameEvent.WaitFor();
  } // WaitForCommandFrame

  void DS485Controller::WaitForToken() {
    m_TokenEvent.WaitFor();
  } // WaitForToken


  //================================================== DS485FrameReader

  DS485FrameReader::DS485FrameReader()
  : Thread(true, "DS485FrameReader"),
    m_Traffic(false)
  {
  } // ctor

  DS485FrameReader::~DS485FrameReader() {
  } // dtor

  void DS485FrameReader::SetSerialCom(boost::shared_ptr<SerialComBase> _serialCom) {
    m_SerialCom = _serialCom;
  } // SetHandle

  bool DS485FrameReader::HasFrame() {
    return m_IncomingFrames.size();
  } // HasFrame

  DS485Frame* DS485FrameReader::GetFrame() {
    DS485Frame* result = NULL;
    if(HasFrame()) {
      result = m_IncomingFrames.at(0);
      m_IncomingFrames.erase(m_IncomingFrames.begin());
    }
    return result;
  } // GetFrame

  bool DS485FrameReader::GetCharTimeout(char& _charOut, const int _timeoutSec) {
    return m_SerialCom->GetCharTimeout(_charOut, _timeoutSec);
  } // GetCharTimeout

  bool DS485FrameReader::GetAndResetTraffic() {
    bool result = m_Traffic;
    m_Traffic = false;
    return result;
  }

  typedef enum {
    rsSynchronizing,
    rsReadingHeader,
    rsReadingPacket,
    rsReadingCRC
  } aReaderState;

  const int TheReceiveBufferSizeBytes = 50;
  const int TheHeaderSize = 3;
  const int TheFrameHeaderSize = TheHeaderSize + 1;
  const int TheCRCSize = 2;

  void DS485FrameReader::Execute() {
    int validBytes = 0;
    unsigned char received[TheReceiveBufferSizeBytes];
    aReaderState state = rsSynchronizing;
    int messageLen = -1;

    bool escapeNext = false;
    bool escaped = false;
    while(!m_Terminated) {
      char currentChar;
      if(GetCharTimeout(currentChar, 1)) {
        m_Traffic = true;

        if((unsigned char)currentChar == EscapeCharacter) {
          escapeNext = true;
          continue;
        }

        if((unsigned char)currentChar == FrameStart) {
          state = rsSynchronizing;
          validBytes = 0;
        }

        if(escapeNext) {
          currentChar |= 0x80;
          escapeNext = false;
          escaped = true;
        } else {
          escaped = false;
        }

        // store in our receive buffer
        received[validBytes++] = currentChar;

        if(validBytes == TheReceiveBufferSizeBytes) {
          Logger::GetInstance()->Log("receive buffer overflowing, resyncing", lsInfo);
          state = rsSynchronizing;
          validBytes = 0;
        }

        switch(state) {
          case rsSynchronizing:
          {
            if(((unsigned char)currentChar == FrameStart) && !escaped) {
              state = rsReadingHeader;
            } else {
              cout << "?";
            }
            break;
          }

          case rsReadingHeader:
          {
            if(validBytes < 1) {
              Logger::GetInstance()->Log("in state rsReadingPacket with validBytes < 1, going back to sync", lsError);
            }

            // attempt to parse the header
            if(validBytes == TheHeaderSize) {
              if(received[1] & 0x02 == 0x02) {
                //Logger::GetInstance()->Log("Packet is a broadcast");
              } else {
                //Logger::GetInstance()->Log("*Packet is adressed");
              }
              // check if it's a token or not
              if(received[1] & 0x01 == 0x01) {
                //Logger::GetInstance()->Log("Packet is a Frame");
                state = rsReadingPacket;
                messageLen = -1;
              } else {
                //Logger::GetInstance()->Log("Packet is a Token");
                DS485Frame* frame = new DS485Frame();
                frame->GetHeader().FromChar(received, validBytes);
                m_IncomingFrames.push_back(frame);
                m_PacketReceivedEvent.Signal();
                /*
                DS485Frame* token = new DS485Frame();
                token->GetHeader().SetDestination(0);
                token->GetHeader().SetSource(1);
                */
                cout << "-";
                //flush(cout);
                state = rsSynchronizing;
              }
            }
            break;
          }

          case rsReadingPacket:
          {
            if(messageLen == -1 && validBytes >= TheFrameHeaderSize) {
              // the length does not include the size of the headers
              messageLen = received[3] & 0x0F;
            }
            if(validBytes == (messageLen + TheFrameHeaderSize)) {
              //Logger::GetInstance()->Log("frame received");
              state = rsReadingCRC;
            }
            break;
          }

          case rsReadingCRC:
          {
            if(validBytes == (messageLen + TheFrameHeaderSize + TheCRCSize)) {
              uint16_t dataCRC = CRC16(received, validBytes);
              if(dataCRC != 0) {
                Logger::GetInstance()->Log("*********** crc mismatch.", lsError);
              } else {
                //Logger::GetInstance()->Log("received packet, crc ok");
                //cout << "#";
              }
              DS485CommandFrame* frame = new DS485CommandFrame();
              frame->GetHeader().FromChar(received, validBytes);
              frame->SetLength(received[3] & 0x0F);
              frame->SetCommand(received[3] >> 4 & 0x0F);
              for(int iByte = 0; iByte < messageLen; iByte++) {
                frame->GetPayload().Add<uint8>(static_cast<uint8>(received[iByte + 4]));
              }
              //cout << "*" << frame->GetCommand() << "*";
              //flush(cout);
              m_IncomingFrames.push_back(frame);
              m_PacketReceivedEvent.Signal();

              // the show must go on...
              messageLen = -1;
              state = rsSynchronizing;
            }
          }
        }
      }
    }
  } // Execute

  bool DS485FrameReader::WaitForFrame() {
    if(!HasFrame()) {
      m_PacketReceivedEvent.WaitFor();
    }
    return true;
  }

  //================================================== DS485FrameProvider

  void DS485FrameProvider::AddFrameCollector(IDS485FrameCollector* _collector) {
    m_FrameCollectors.push_back(_collector);
  } // AddFrameCollector

  void DS485FrameProvider::RemoveFrameCollector(IDS485FrameCollector* _collector) {
    vector<IDS485FrameCollector*>::iterator iCollector = find(m_FrameCollectors.begin(), m_FrameCollectors.end(), _collector);
    if(iCollector != m_FrameCollectors.end()) {
      m_FrameCollectors.erase(iCollector);
    }
  } // RemoveFrameCollector

  void DS485FrameProvider::DistributeFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
    for(vector<IDS485FrameCollector*>::iterator iCollector = m_FrameCollectors.begin(), e = m_FrameCollectors.end();
        iCollector != e; ++iCollector)
    {
      (*iCollector)->CollectFrame(_frame);
    }
  } // DistributeFrame

  void DS485FrameProvider::DistributeFrame(DS485CommandFrame* _pFrame) {
    DistributeFrame(boost::shared_ptr<DS485CommandFrame>(_pFrame));
  } // DistributeFrame

  //================================================== DS485FrameSniffer
#ifndef __APPLE__
  DS485FrameSniffer::DS485FrameSniffer(const string& _deviceName)
  : Thread(true, "DS485FrameSniffer")
  {
     m_SerialCom.reset(new SerialCom());
     m_SerialCom->Open(_deviceName.c_str());
     m_FrameReader.SetSerialCom(m_SerialCom);
  }

  void DS485FrameSniffer::Execute() {
    m_FrameReader.Run();
    struct timespec lastFrame;
    struct timespec thisFrame;
    clock_gettime(CLOCK_REALTIME, &lastFrame);
    while(!m_Terminated) {
      if(m_FrameReader.WaitForFrame()){
        clock_gettime(CLOCK_REALTIME, &thisFrame);
        boost::scoped_ptr<DS485Frame> frame(m_FrameReader.GetFrame());
        if(frame.get() != NULL) {

          double diffMS = ((thisFrame.tv_sec*1000.0 + thisFrame.tv_nsec/1000.0/1000.0) -
                           (lastFrame.tv_sec*1000.0 + lastFrame.tv_nsec/1000.0/1000.0));

          cout << "+" << diffMS << endl;

          DS485CommandFrame* cmdFrame = dynamic_cast<DS485CommandFrame*>(frame.get());
          if(cmdFrame != NULL) {
            cout << "Command Frame: " << CommandToString(cmdFrame->GetCommand()) << endl;
          } else {
            cout << "token " << frame->GetHeader().GetSource() << " -> " << frame->GetHeader().GetDestination()  << endl;
          }
          cout << "seq: " << frame->GetHeader().GetCounter() << endl;

          lastFrame = thisFrame;
        }
      }
    }
  }
#endif
  //================================================== PayloadDissector

  template<>
  uint8 PayloadDissector::Get() {
    uint8 result = m_Payload.back();
    m_Payload.pop_back();
    return result;
  }

  template<>
  dsid_t PayloadDissector::Get() {
    dsid_t result;
    result = (Get<uint8>() << 24) |
             (Get<uint8>() << 16) |
             (Get<uint8>() << 8)  |
             (Get<uint8>());
    return result;
  }

  //================================================== Global helpers

  const char* CommandToString(const uint8 _command) {
    switch(_command) {
    case CommandSolicitSuccessorRequest:
      return "solicit successor request";
    case CommandSolicitSuccessorResponse:
      return "solicit successor response";
    case CommandGetAddressRequest:
      return "get address request";
    case CommandGetAddressResponse:
      return "get address response";
    case CommandSetDeviceAddressRequest:
      return "set device address request";
    case CommandSetDeviceAddressResponse:
      return "set device address response";
    case CommandSetSuccessorAddressRequest:
      return "set successor address request";
    case CommandSetSuccessorAddressResponse:
      return "set successor address response";
    case CommandRequest:
      return "request";
    case CommandResponse:
      return "response";
    case CommandAck:
      return "ack";
    case CommandBusy:
      return "busy";
    default:
      return "(unknown)";
    }
  } // CommandToString

}
