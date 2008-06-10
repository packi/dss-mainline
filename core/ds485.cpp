/*
 *  ds485.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/29/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "ds485.h"

#include "base.h"
#include "logger.h"

#include <stdexcept>
#include <sstream>

#include <boost/scoped_ptr.hpp>

using namespace std;

namespace dss {
  
  const char FrameStart = 0xFD;
  
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
      throw new invalid_argument("_len must be > 3");
    }
    SetBroadcast(_data[1] & 0x02 == 0x02);
    SetCounter(_data[2] & 0x03);
    SetSource((_data[1] >> 2) & 0x3F);
    SetDestination((_data[2] >> 2) & 0x3F);
  } // FromChar

  
  //================================================== DS485Frame
  
  vector<unsigned char> DS485Frame::ToChar() const {
    vector<unsigned char> result = m_Header.ToChar();
    return result;
  } // ToChar
  
  DS485Payload& DS485Frame::GetPayload() {
    return m_Payload;
  } // GetPayload

  
  //================================================== DS485CommandFrame
  
  DS485CommandFrame::DS485CommandFrame() {
    GetHeader().SetType(1);
  } // ctor
  
  vector<unsigned char> DS485CommandFrame::ToChar() const {
    vector<unsigned char> result = DS485Frame::ToChar();
    unsigned char tmp = (m_Command << 4) | (m_Length & 0x0F);
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
      Logger::GetInstance()->Log(string("command: ") + CommandToString(cmdFrame->GetCommand()));
      //Logger::GetInstance()->Log(string("length:  ") + IntToString((cmdFrame->GetLength())));
      //Logger::GetInstance()->Log(string("msg nr:  ") + IntToString(cmdFrame->GetHeader().GetCounter()));
    } else {
      Logger::GetInstance()->Log("token received");
    }
    return result;
  } // GetFrameFromWire
  
  bool DS485Controller::PutFrameOnWire(const DS485Frame* _pFrame) {
    vector<unsigned char> chars = _pFrame->ToChar();
    for(vector<unsigned char>::iterator iChar = chars.begin(), e = chars.end();
        iChar != e; ++iChar)
    {
      m_SerialCom->PutChar(*iChar);
    }
  } // PutFrameOnWire
  
  DS485Controller::DS485Controller()
  : Thread(true, "DS485Controller")
  {
  } // ctor
  
  DS485Controller::~DS485Controller() {
  } // dtor
  
  void DS485Controller::Execute() {
    m_SerialCom.reset(new SerialCom());
    if(!m_SerialCom->Open("/dev/ttyUSB0")) {
      throw new runtime_error("could not open serial port");
    }
    m_FrameReader.SetSerialCom(m_SerialCom);
    m_FrameReader.Run();
    m_State = csInitial;
    m_StationID = 0xFF;
    m_NextStationID = 0xFF;
    
    uint32_t dsid = 0xdeadbeef;
    
    int senseTimeMS = 0;
    int numberOfJoinPacketsToWait = -1;
    
    while(!m_Terminated) {
      
      if(m_State == csInitial) {
        senseTimeMS = rand() % 100 + 1;
        m_State = csSensing;
        continue;
      } else if(m_State == csSensing) {
        if(m_FrameReader.GetAndResetTraffic()) {
          Logger::GetInstance()->Log("Sensed traffic on the line, changing to csSlaveWaitingToJoin");
          m_State = csSlaveWaitingToJoin;
        }
        if(senseTimeMS == 0) {
          Logger::GetInstance()->Log("No traffic on line, I'll be your master today");
        }
        SleepMS(senseTimeMS);
        senseTimeMS = 0;
        continue;
      }
      
      if(m_FrameReader.HasFrame()) {
        boost::scoped_ptr<DS485Frame> frame(GetFrameFromWire());
        DS485Header& header = frame->GetHeader(); 
        if(!header.IsBroadcast() && header.GetDestination() != m_StationID) {
          Logger::GetInstance()->Log("packet not for me, discarding");
        }
        DS485CommandFrame* cmdFrame = dynamic_cast<DS485CommandFrame*>(cmdFrame);
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
                  numberOfJoinPacketsToWait = rand() % 100 + 1;
                } else {
                  numberOfJoinPacketsToWait--;
                  if(numberOfJoinPacketsToWait == 0) {
                    m_StationID = 0x3F;
                    DS485CommandFrame* frameToSend = new DS485CommandFrame();
                    frameToSend->GetHeader().SetDestination(0);
                    frameToSend->GetHeader().SetSource(m_StationID);
                    frameToSend->SetCommand(CommandSolicitSuccessorResponse);
                    frameToSend->SetLength(4);
                    frameToSend->GetPayload().Add<uint8>(static_cast<uint8>((dsid >> 24) & 0x000000FF));
                    frameToSend->GetPayload().Add<uint8>(static_cast<uint8>((dsid >> 16) & 0x000000FF));
                    frameToSend->GetPayload().Add<uint8>(static_cast<uint8>((dsid >> 18) & 0x000000FF));
                    frameToSend->GetPayload().Add<uint8>(static_cast<uint8>((dsid >>  0) & 0x000000FF));
                    PutFrameOnWire(frameToSend);
                    m_State = csSlaveJoining;
                  }
                }
              }
            }
          }
          break;
        case csSlaveJoining:
          if(cmdFrame != NULL) {
            DS485Payload& payload = cmdFrame->GetPayload(); 
            if(cmdFrame->GetCommand() == CommandSetDeviceAddressRequest) {
              m_StationID = payload.ToChar().at(0);
            } else if(cmdFrame->GetCommand() == CommandSetSuccessorAddressRequest) {
              m_NextStationID = payload.ToChar().at(0);
            }
            if(m_StationID != 0x3F && m_NextStationID != 0xFF) {
              m_State = csSlave;
            }
          }
          break;
        case csBroadcastingDSID:
          break;
        case csMaster:
          break;
        case csSlave:
          break;
        case csWaitingForToken:
          break;
        default:
          throw new runtime_error("invalid value for m_State");
        }
      } else {
        SleepMS(10);
      }
    }
  } // Execute
  
  //================================================== DS485FrameReader
  
  DS485FrameReader::DS485FrameReader()
  : Thread(true, "DS485FrameReader")
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
        
        if((unsigned char)currentChar == 0xFC) {
          escapeNext = true;
          continue;
        }
        
        if(currentChar == FrameStart) {
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
            if((currentChar == FrameStart) && !escaped) {
              state = rsReadingHeader;
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
                Logger::GetInstance()->Log("Packet is a broadcast");
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
                //stringstream s;
                //s << "crc missmatch.";
                //Logger::GetInstance()->Log(s.str(), lsError);
              } else {
               // Logger::GetInstance()->Log("received packet, crc ok");
              }
              DS485CommandFrame* frame = new DS485CommandFrame();
              frame->GetHeader().FromChar(received, validBytes);
              frame->SetLength(received[3] & 0x0F);
              frame->SetCommand(received[3] >> 4 & 0x0F);
              for(int iByte = 0; iByte < messageLen; iByte++) {
                frame->GetPayload().Add<uint8>(static_cast<uint8>(received[iByte + 4]));
              }
              m_IncomingFrames.push_back(frame);

              // the show must go on...
              messageLen = -1;
              state = rsSynchronizing;
            }
          }
        }
      }
    }
  } // Execute
  
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
    case CommandSetSuccessorAddressRequest: 
      return "set successor address request";
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
