/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

*/

#include "ds485.h"

#include "core/base.h"
#include "core/logger.h"
#include "core/ds485const.h"
#include "core/model/modelconst.h"
#include "core/dss.h"
#include "core/propertysystem.h"

#include <stdexcept>
#include <sstream>
#include <iostream>

#include <ctime>
#include <sys/time.h>

#include <boost/scoped_ptr.hpp>

namespace dss {

  const unsigned char FrameStart = 0xFD;
  const unsigned char EscapeCharacter = 0xFC;

  const char* controllerStateToString(const aControllerState _state);

  //================================================== DS485Payload

  template<>
  void DS485Payload::add(uint8_t _data) {
    m_Data.push_back(_data);
  } // add<uint8_t>

  template<>
  void DS485Payload::add(devid_t _data) {
    m_Data.push_back((_data >> 0) & 0x00FF);
    m_Data.push_back((_data >> 8) & 0x00FF);
  } // add<devid_t>

  template<>
  void DS485Payload::add(uint32_t _data) {
    add<uint16_t>((_data >>  0) & 0x0000FFFF);
    add<uint16_t>((_data >> 16) & 0x0000FFFF);
  }

  template<>
  void DS485Payload::add(dsid_t _data) {
    // 0x11223344 55667788
    for(int iByte = 0; iByte < 8; iByte++) {
      add<uint8_t>((_data.upper >> ((8 - iByte - 1) * 8)) & 0x00000000000000FF);
    }
    add<uint8_t>((_data.lower >> 24) & 0x000000FF);
    add<uint8_t>((_data.lower >> 16) & 0x000000FF);
    add<uint8_t>((_data.lower >>  8) & 0x000000FF);
    add<uint8_t>((_data.lower >>  0) & 0x000000FF);
  } // add<dsid_t>

  int DS485Payload::size() const {
    return m_Data.size();
  } // size

  const std::vector<unsigned char>& DS485Payload::toChar() const {
    return m_Data;
  } // getData


  //================================================== DS485Header

  std::vector<unsigned char> DS485Header::toChar() const {
    std::vector<unsigned char> result;
    result.push_back(FrameStart);
    unsigned char tmp = (m_Destination << 2) | ((m_Broadcast & 0x01) << 1) | (m_Type & 0x01);
    result.push_back(tmp);
    tmp = ((m_Source & 0x3F) << 2) | (m_Counter & 0x03);
    result.push_back(tmp);

    return result;
  } // toChar

  void DS485Header::fromChar(const unsigned char* _data, const int _len) {
    if(_len < 3) {
      throw std::invalid_argument("_len must be > 3");
    }
    setDestination((_data[1] >> 2) & 0x3F);
    setBroadcast((_data[1] & 0x02) == 0x02);
    setSource((_data[2] >> 2) & 0x3F);
    setCounter(_data[2] & 0x03);
  } // fromChar


  //================================================== DS485Frame

  std::vector<unsigned char> DS485Frame::toChar() const {
    std::vector<unsigned char> result = m_Header.toChar();
    return result;
  } // toChar

  DS485Payload& DS485Frame::getPayload() {
    return m_Payload;
  } // getPayload

  const DS485Payload& DS485Frame::getPayload() const {
    return m_Payload;
  } // getPayload


  //================================================== DS485CommandFrame

  DS485CommandFrame::DS485CommandFrame()
  : m_Length(0xFF)
  {
    getHeader().setType(1);
  } // ctor

  std::vector<unsigned char> DS485CommandFrame::toChar() const {
    std::vector<unsigned char> result = DS485Frame::toChar();
    unsigned char tmp;
    if(m_Length == 0xFF) {
      tmp = (m_Command << 4) | (getPayload().size() & 0x0F);
    } else {
      tmp = (m_Command << 4) | (m_Length & 0x0F);
    }
    result.push_back(tmp);

    const std::vector<unsigned char>& payload = getPayload().toChar();
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
  } // toChar


  //================================================== DS485Controller

  DS485Controller::DS485Controller()
  : Thread("DS485Controller"),
    m_State(csInitial),
    m_RS485DeviceName("/dev/ttyUSB0"),
    m_DenyJoiningAsShortDevice(false)
  {
    m_DSID.upper = DSIDHeader;
    m_DSID.lower = 0xDEADBEEF;
  } // ctor

  DS485Controller::~DS485Controller() {
  } // dtor

  void DS485Controller::setRS485DeviceName(const std::string& _value) {
    m_RS485DeviceName = _value;
    if(isRunning()) {
      Logger::getInstance()->log("DS485Controller::setRS485DeviceName: Called too late. Value updated but it won't have any effect.", lsError);
    }
  } // setRS485DeviceName

  DS485Frame* DS485Controller::getFrameFromWire() {
    DS485Frame* result = m_FrameReader.getFrame(200);
    if(result != NULL) {
      DS485CommandFrame* cmdFrame = dynamic_cast<DS485CommandFrame*>(result);
      if(cmdFrame != NULL) {
        //Logger::getInstance()->log("Command received");
        //Logger::getInstance()->log(string("command: ") + CommandToString(cmdFrame->getCommand()));
        //Logger::getInstance()->log(string("length:  ") + intToString((cmdFrame->getLength())));
        //Logger::getInstance()->log(string("msg nr:  ") + intToString(cmdFrame->getHeader().getCounter()));
      } else {
        //std::cout << "+";
        //flush(std::cout);
        //Logger::getInstance()->log("token received");
      }
    }
    return result;
  } // getFrameFromWire

  inline void DS485Controller::putCharOnWire(const unsigned char _ch) {
    if((_ch == FrameStart) || (_ch == EscapeCharacter)) {
      m_SerialCom->putChar(0xFC);
      // mask out the msb
      m_SerialCom->putChar(_ch & 0x7F);
    } else {
      m_SerialCom->putChar(_ch);
    }
  } // putCharOnWire

  bool DS485Controller::putFrameOnWire(const DS485Frame* _pFrame, bool _freeFrame) {
    std::vector<unsigned char> chars = _pFrame->toChar();
    uint16_t crc = 0x0000;

    unsigned int numChars = chars.size();
    for(unsigned int iChar = 0; iChar < numChars; iChar++) {
      unsigned char c = chars[iChar];
      crc = update_crc(crc, c);
      // escape if it's a reserved character and not the first (frame start)
      if(iChar != 0) {
        putCharOnWire(c);
      } else {
        m_SerialCom->putChar(c);
      }
    }

    if(dynamic_cast<const DS485CommandFrame*>(_pFrame) != NULL) {
      // send crc
      unsigned char c = static_cast<unsigned char>(crc & 0xFF);
      putCharOnWire(c);
      c = static_cast<unsigned char>((crc >> 8) & 0xFF);
      putCharOnWire(c);
    }
    if(_freeFrame) {
      delete _pFrame;
    }
    return true;
  } // putFrameOnWire

  bool DS485Controller::resetSerialLine() {
    m_SerialCom.reset(new SerialCom());
    try {
      Logger::getInstance()->log("DS485Controller::execute: Opening '" + m_RS485DeviceName + "' as serial device", lsInfo);
      m_SerialCom->open(m_RS485DeviceName.c_str());
      m_FrameReader.setSerialCom(m_SerialCom);
      return true;
    } catch(const std::runtime_error& _ex) {
      Logger::getInstance()->log(std::string("Caught exception while opening serial port: ") + _ex.what(), lsFatal);
      return false;
    }
  } // resetSerialLine

  void DS485Controller::execute() {
    if(!resetSerialLine()) {
      doChangeState(csError);
      return;
    }
    doChangeState(csInitial);
    m_TokenCounter = 0;
    m_NextStationID = 0xFF;
    m_StationID = 0xFF;
    time_t responseSentAt;
    time_t tokenReceivedAt;
    m_DSID = dsid_t::fromString(DSS::getInstance()->getPropertySystem().getStringValue("/system/dsid"));

    // prepare token frame
    boost::scoped_ptr<DS485Frame> token(new DS485Frame());
    // prepare solicit successor response frame
    boost::scoped_ptr<DS485CommandFrame> solicitSuccessorResponseFrame(new DS485CommandFrame());
    solicitSuccessorResponseFrame->getHeader().setDestination(0);
    solicitSuccessorResponseFrame->getHeader().setSource(0x3F);
    solicitSuccessorResponseFrame->setCommand(CommandSolicitSuccessorResponse);
    solicitSuccessorResponseFrame->getPayload().add(m_DSID);

    int senseTimeMS = 0;
    int numberOfJoinPacketsToWait = -1;
    bool lastSentWasToken = false;
    int comErrorSleepTimeScaler = 1;
    int missedFramesCounter = 0;

    while(!m_Terminated) {

      if(m_State == csInitial) {
        senseTimeMS = (rand() % 1000) + 2500;
        numberOfJoinPacketsToWait = -1;
        m_TokenCounter = 0;
        lastSentWasToken = false;
        doChangeState(csSensing);
        continue;
      } else if(m_State == csCommError) {
        sleepMS(comErrorSleepTimeScaler++ * 500);
        comErrorSleepTimeScaler = std::min(comErrorSleepTimeScaler, 60);
        if(resetSerialLine()) {
          doChangeState(csInitial);
        }
        continue;
      } else if(m_State == csSensing) {
        try {
          if(m_FrameReader.senseTraffic(senseTimeMS)) {
            Logger::getInstance()->log("DS485: Sensed traffic on the line");
            // wait some time for the first frame and skip it...
            delete m_FrameReader.getFrame(1000);
            doChangeState(csSlaveWaitingToJoin);
          } else {
            Logger::getInstance()->log("DS485: No traffic on the line");
            doChangeState(csDesignatedMaster);
          }
          comErrorSleepTimeScaler = 1;
        } catch(const std::runtime_error&) {
          doChangeState(csCommError);
        }
        continue;
      }

      boost::scoped_ptr<DS485Frame> frame;
      try {
        frame.reset(getFrameFromWire());
      } catch(const std::runtime_error&) {
        doChangeState(csCommError);
        continue;
      }
      if(frame == NULL) {
        if(m_State != csDesignatedMaster) {
          std::cout << "§";
          missedFramesCounter++;
          if(missedFramesCounter == 50) {
            missedFramesCounter = 0;
            Logger::getInstance()->log("DS485: No traffic on the line, restarting");
            doChangeState(csInitial);
          }
        } else {
          sleepMS(1000);
        }
        // resend token after timeout
        if(lastSentWasToken) {
          putFrameOnWire(token.get(), false);
          lastSentWasToken = false;
          std::cout << "t(#)";
        }
        flush(std::cout);
      } else {
        DS485Header& header = frame->getHeader();

        // discard own echo packets
        if (header.getSource() == m_StationID) {
          continue;
        }

        DS485CommandFrame* cmdFrame = dynamic_cast<DS485CommandFrame*>(frame.get());
        lastSentWasToken = false;
        missedFramesCounter = 0;

        // handle cases in which we're obliged to act on disregarding our current state
        if(cmdFrame != NULL) {
          if(cmdFrame->getCommand() == CommandGetAddressRequest) {
            if(header.getDestination() == m_StationID) {
              DS485CommandFrame* frameToSend = new DS485CommandFrame();
              frameToSend->getHeader().setDestination(header.getSource());
              frameToSend->getHeader().setSource(m_StationID);
              frameToSend->setCommand(CommandGetAddressResponse);
              putFrameOnWire(frameToSend);
              Logger::getInstance()->log("DS485: received get address request");
              continue;
            }
          } else if(cmdFrame->getCommand() == CommandSolicitSuccessorRequest) {
            if (m_State == csSlave || m_State == csMaster) {
              Logger::getInstance()->log("DS485: bus is reorganizing, restart", lsError);
              doChangeState(csInitial);
              continue;
            }
          }
        }

        // discard packets which are not addressed to us
        if(!header.isBroadcast() &&
           (header.getDestination() != m_StationID) &&
              (m_State == csSlave ||
               m_State == csMaster)
          )
        {
          continue;
        }

        switch(m_State) {
        case csInitial:
          break;
        case csSensing:
          break;
        case csSlaveWaitingToJoin:
          {
            if(cmdFrame != NULL) {
              bool isSolicitRequestLong = cmdFrame->getCommand() == CommandSolicitSuccessorRequestLong;
              bool isSolicitRequestShort = cmdFrame->getCommand() == CommandSolicitSuccessorRequest;
              if(isSolicitRequestLong || (isSolicitRequestShort && !m_DenyJoiningAsShortDevice)) {
                // if it's the first of it's kind, determine how many we've got to skip
                if(numberOfJoinPacketsToWait == -1) {
                  if(cmdFrame->getCommand() == CommandSolicitSuccessorRequest) {
                    numberOfJoinPacketsToWait = rand() % 10 + 10;
                  } else {
                    // don’t wait forever if we’re in slow-joining mode
                    numberOfJoinPacketsToWait = rand() % 10;
                  }
                  std::cout << "** Waiting for " << numberOfJoinPacketsToWait << std::endl;
                } else {
                  numberOfJoinPacketsToWait--;
                  if(numberOfJoinPacketsToWait == 0) {
                    m_StationID = 0x3F;
                    putFrameOnWire(solicitSuccessorResponseFrame.get(), false);
                    //std::cout << "******* FRAME AWAY ******" << std::endl;
                    doChangeState(csSlaveJoining);
                    time(&responseSentAt);
                  }
                  //std::cout << numberOfJoinPacketsToWait << std::endl;
                }
              }
            }
          }
          break;
        case csSlaveJoining:
          if(cmdFrame != NULL) {
            if(cmdFrame->getCommand() == CommandSetDeviceAddressRequest) {
              m_StationID = cmdFrame->getPayload().toChar().at(0);
              DS485CommandFrame* frameToSend = new DS485CommandFrame();
              frameToSend->getHeader().setDestination(0);
              frameToSend->getHeader().setSource(m_StationID);
              frameToSend->setCommand(CommandSetDeviceAddressResponse);
              putFrameOnWire(frameToSend);
              Logger::getInstance()->log("DS485: got address " + intToString(m_StationID), lsInfo);
            } else if(cmdFrame->getCommand() == CommandSetSuccessorAddressRequest) {
              if(header.getDestination() == m_StationID) {
                handleSetSuccessor(cmdFrame);
              }
            } else {
              // check if our response has timed-out
              time_t now;
              time(&now);
              if((now - responseSentAt) > 5) {
                Logger::getInstance()->log("DS485: startup timeout", lsError);
                doChangeState(csInitial);
              }
            }
            if((m_StationID != 0x3F) && (m_NextStationID != 0xFF)) {
              Logger::getInstance()->log("DS485: linked into network", lsInfo);
              token->getHeader().setDestination(m_NextStationID);
              token->getHeader().setSource(m_StationID);
              doChangeState(csSlaveWaitingForFirstToken);
              time(&tokenReceivedAt);
            }
          }
          break;
        case csBroadcastingDSID:
          break;
        case csMaster:
          break;
        case csSlave:
          if(cmdFrame == NULL) {
//            std::cout << "+";
            // it's a token
            if(!m_PendingFrames.empty() && (m_TokenCounter > 10)) {

              // send frame
              m_PendingFramesGuard.lock();
              DS485CommandFrame& frameToSend = m_PendingFrames.front();
              putFrameOnWire(&frameToSend, false);
              std::cout << "p%" << (int)frameToSend.getCommand() << "%e" << std::endl;

              // if not a broadcast, wait for ack, etc
              if(frameToSend.getHeader().isBroadcast()) {
                m_PendingFrames.erase(m_PendingFrames.begin());
              } else {
                boost::shared_ptr<DS485Frame> ackFrame(m_FrameReader.getFrame(50));

                DS485CommandFrame* cmdAckFrame = dynamic_cast<DS485CommandFrame*>(ackFrame.get());
                if(cmdAckFrame != NULL) {
                  if(cmdAckFrame->getHeader().getSource() == m_StationID) {
                    ackFrame.reset( m_FrameReader.getFrame(50) );
                  }
                }

                cmdAckFrame = dynamic_cast<DS485CommandFrame*>(ackFrame.get());
                if(cmdAckFrame != NULL) {
                  if(cmdAckFrame->getCommand() == CommandAck) {
                    m_PendingFrames.erase(m_PendingFrames.begin());
                    std::cout << "\ngot ack" << std::endl;
                  } else if(cmdAckFrame->getCommand() == CommandBusy) {
                    std::cout << "\ngot busy" << std::endl;
                  } else if(cmdAckFrame->getCommand() == CommandResponse) {
                    std::cout << "\ngot response" << std::endl;
                    m_PendingFrames.erase(m_PendingFrames.begin());
                    addToReceivedQueue(cmdAckFrame);
                  } else {
                    std::cout << "\ngot invalid response" << std::endl;
                  }
                } else {
                  m_PendingFrames.erase(m_PendingFrames.begin());
                  std::cout << "no ack received" << std::endl;
                }
              }
            }
            m_PendingFramesGuard.unlock();
            putFrameOnWire(token.get(), false);
//            std::cout << ".";
//            flush(std::cout);
            time(&tokenReceivedAt);
            m_TokenCounter++;
            lastSentWasToken = true;
          } else {

            // Handle token timeout
            time_t now;
            time(&now);
            if((now - tokenReceivedAt) > 15) {
              Logger::getInstance()->log("DS485: Token timeout, restarting", lsError);
              doChangeState(csInitial);
              continue;
            }
            std::cout << "f*" << (int)cmdFrame->getCommand() << "*";

            bool keep = false;

            if(cmdFrame->getCommand() == CommandResponse) {
              if(!cmdFrame->getHeader().isBroadcast()) {
                // send ack if it's a response and not a broadcasted one
                DS485CommandFrame* ack = new DS485CommandFrame();
                ack->getHeader().setSource(m_StationID);
                ack->getHeader().setDestination(cmdFrame->getHeader().getSource());
                ack->setCommand(CommandAck);
                putFrameOnWire(ack);
                std::cout << "a(res)";
              } else {
                std::cout << "b";
              }
              keep = true;
            } else if(cmdFrame->getCommand() == CommandRequest || cmdFrame->getCommand() == CommandEvent) {
              if(!cmdFrame->getHeader().isBroadcast()) {
                DS485CommandFrame* ack = new DS485CommandFrame();
                ack->getHeader().setSource(m_StationID);
                ack->getHeader().setDestination(cmdFrame->getHeader().getSource());
                ack->setCommand(CommandAck);
                putFrameOnWire(ack);
                std::cout << "a(req)";
              }
              keep = true;
            } else if(cmdFrame->getCommand() == CommandSolicitSuccessorRequest) {
              Logger::getInstance()->log("DS485: bus is reorganizing, restart", lsError);
              doChangeState(csInitial);
            } else if(cmdFrame->getCommand() == CommandSetSuccessorAddressRequest) {
              if(header.getDestination() == m_StationID) {
                handleSetSuccessor(cmdFrame);
                token->getHeader().setDestination(m_NextStationID);
              }
            } else {
              std::cout << "&&&&&&&&&& unknown frame id: " << (int)cmdFrame->getCommand() << std::endl;
            }
            if(keep) {
              // put in into the received queue
              addToReceivedQueue(cmdFrame);
              std::cout << "k";
            }
            flush(std::cout);
          }
          break;
        case csSlaveWaitingForFirstToken:
          {
            if(cmdFrame == NULL) {
              if(header.getDestination() == m_StationID) {
                putFrameOnWire(token.get(), false);
                m_TokenCounter = 0;
                doChangeState(csSlave);
                time(&tokenReceivedAt);
                std::cout << "DS485: Got first TOKEN" << std::endl;
                flush(std::cout);
              }
            }
            else {
              // any command frame indicates there are still devices joining
              time(&tokenReceivedAt);
            }
            // Handle timeout
            time_t now;
            time(&now);
            if((now - tokenReceivedAt) > 20) {
              std::cerr << "DS485: Wait for token timeout, restarting" << std::endl;
              doChangeState(csInitial);
              continue;
            }
          }
          break;
        case csDesignatedMaster:
          sleepMS(10000);
          doChangeState(csInitial);
          break;
        default:
          throw std::runtime_error("invalid value for m_State");
        }
      }
    }
  } // execute

  void DS485Controller::handleSetSuccessor(DS485CommandFrame* _frame) {
    m_NextStationID = _frame->getPayload().toChar().at(0);
    DS485CommandFrame* frameToSend = new DS485CommandFrame();
    frameToSend->getHeader().setDestination(0);
    frameToSend->getHeader().setSource(m_StationID);
    frameToSend->setCommand(CommandSetSuccessorAddressResponse);
    putFrameOnWire(frameToSend);
    std::cout << "### successor " << m_NextStationID << "\n";
  } // handleSetSuccessor

  // TODO: look into boost::weak_ptr
  void DS485Controller::addToReceivedQueue(DS485CommandFrame* _frame) {
    if(_frame == NULL) {
      Logger::getInstance()->log("DS485Controller::addToReceivedQueue: _frame is NULL", lsFatal);
      return;
    }
    boost::shared_ptr<DS485CommandFrame> frame(new DS485CommandFrame());
    (*frame) = *_frame;
    frame->setFrameSource(fsWire);
    distributeFrame(frame);
  } // addToReceivedQueue

  void DS485Controller::doChangeState(aControllerState _newState) {
    if(_newState != m_State) {
      Logger::getInstance()->log("DS485Controller::doStateChange: " +
          std::string(controllerStateToString(m_State)) + " -> " + std::string(controllerStateToString(_newState)),
          lsInfo);
      m_State = _newState;
      m_StateString = controllerStateToString(m_State);
      if(m_State == csInitial) {
        m_NextStationID = 0xFF;
        m_StationID = 0xFF;
      } else if((m_State == csSlave) || (m_State == csMaster)) {
        if(m_BusReadyCallback) {
          m_BusReadyCallback();
        }
      }
    }
  } // doChangeState

  void DS485Controller::enqueueFrame(DS485CommandFrame& _frame) {
    //Logger::getInstance()->log("Frame queued");
    DS485CommandFrame* frame = new DS485CommandFrame();
    *frame = _frame;
    frame->getHeader().setSource(m_StationID);
    m_PendingFramesGuard.lock();
    m_PendingFrames.push_back(frame);
    m_PendingFramesGuard.unlock();
  } // enqueueFrame

  aControllerState DS485Controller::getState() const {
    return m_State;
  } // getState

  const std::string& DS485Controller::getStateAsString() const {
    return m_StateString;
  } // getStateAsString


  //================================================== DS485FrameReader

  DS485FrameReader::DS485FrameReader() {
    m_ValidBytes = 0;
    m_State = rsSynchronizing;
    m_MessageLength = -1;

    m_EscapeNext = false;
    m_IsEscaped = false;

    m_NumberOfFramesReceived = 0;
    m_NumberOfIncompleteFramesReceived = 0;
    m_NumberOfCRCErrors = 0;
  } // ctor

  DS485FrameReader::~DS485FrameReader() {
  } // dtor

  void DS485FrameReader::setSerialCom(boost::shared_ptr<SerialComBase> _serialCom) {
    m_SerialCom = _serialCom;
  } // setHandle

  bool DS485FrameReader::getCharTimeout(char& _charOut, const int _timeoutMS) {
    return m_SerialCom->getCharTimeout(_charOut, _timeoutMS);
  } // getCharTimeout

  bool DS485FrameReader::senseTraffic(const int _timeoutMS) {
    char tmp;
    return getCharTimeout(tmp, _timeoutMS);
  } // senseTraffic

  const int TheHeaderSize = 3;
  const int TheFrameHeaderSize = TheHeaderSize + 1;
  const int TheCRCSize = 2;

  DS485Frame* DS485FrameReader::getFrame(const int _timeoutMS) {
    struct timeval timeStarted;
    gettimeofday(&timeStarted, 0);
    while(true) {
      struct timeval now;
      gettimeofday(&now, 0);
      int diffMS = (now.tv_sec - timeStarted.tv_sec) * 1000 + (now.tv_usec - timeStarted.tv_usec) / 1000;
      if(diffMS > _timeoutMS) {
        flush(std::cout);
        if(m_State == rsSynchronizing || m_ValidBytes == 0) {
          break;
        }
        m_NumberOfIncompleteFramesReceived++;
        return NULL;
      }

      char currentChar;
      if(getCharTimeout(currentChar, 1)) {

        if((unsigned char)currentChar == EscapeCharacter) {
          m_EscapeNext = true;
          continue;
        }

        if((unsigned char)currentChar == FrameStart) {
          m_State = rsSynchronizing;
          m_ValidBytes = 0;
        }

        if(m_EscapeNext) {
          currentChar |= 0x80;
          m_EscapeNext = false;
          m_IsEscaped = true;
        } else {
          m_IsEscaped = false;
        }

        // store in our receive buffer
        m_ReceiveBuffer[m_ValidBytes++] = currentChar;

        if(m_ValidBytes == TheReceiveBufferSizeBytes) {
          Logger::getInstance()->log("dS485: receive buffer overflowing, resyncing", lsInfo);
          m_State = rsSynchronizing;
          m_ValidBytes = 0;
        }

        switch(m_State) {
          case rsSynchronizing:
          {
            if((unsigned char)currentChar == FrameStart) {
              m_State = rsReadingHeader;
            } else {
              std::cout << "?";
            }
            break;
          }

          case rsReadingHeader:
          {
            if(m_ValidBytes < 1) {
              Logger::getInstance()->log("in state rsReadingPacket with validBytes < 1, going back to sync", lsError);
            }

            // attempt to parse the header
            if(m_ValidBytes == TheHeaderSize) {
              if((m_ReceiveBuffer[1] & 0x02) == 0x02) {
                //Logger::getInstance()->log("Packet is a broadcast");
              } else {
                //Logger::getInstance()->log("*Packet is adressed");
              }
              // check if it's a token or not
              if((m_ReceiveBuffer[1] & 0x01) == 0x01) {
                //Logger::getInstance()->log("Packet is a Frame");
                m_State = rsReadingPacket;
                m_MessageLength = -1;
              } else {
                //Logger::getInstance()->log("Packet is a Token");
                DS485Frame* frame = new DS485Frame();
                frame->getHeader().fromChar(m_ReceiveBuffer, m_ValidBytes);
                m_NumberOfFramesReceived++;

                //std::cout << "-";
                //flush(std::cout);
                m_State = rsSynchronizing;
                return frame;
              }
            }
            break;
          }

          case rsReadingPacket:
          {
            if(m_MessageLength == -1 && m_ValidBytes >= TheFrameHeaderSize) {
              // the length does not include the size of the headers
              m_MessageLength = m_ReceiveBuffer[3] & 0x0F;
            }
            if(m_ValidBytes == (m_MessageLength + TheFrameHeaderSize)) {
              //Logger::getInstance()->log("frame received");
              m_State = rsReadingCRC;
            }
            break;
          }

          case rsReadingCRC:
          {
            if(m_ValidBytes == (m_MessageLength + TheFrameHeaderSize + TheCRCSize)) {
              uint16_t dataCRC = crc16(m_ReceiveBuffer, m_ValidBytes);
              if(dataCRC != 0) {
                Logger::getInstance()->log("dS485: CRC Mismatch.", lsError);
                m_NumberOfCRCErrors++;
                return NULL;
              }
              m_NumberOfFramesReceived++;
              DS485CommandFrame* frame = new DS485CommandFrame();
              frame->getHeader().fromChar(m_ReceiveBuffer, m_ValidBytes);
              frame->setLength(m_ReceiveBuffer[3] & 0x0F);
              frame->setCommand(m_ReceiveBuffer[3] >> 4 & 0x0F);
              for(int iByte = 0; iByte < m_MessageLength; iByte++) {
                frame->getPayload().add<uint8_t>(static_cast<uint8_t>(m_ReceiveBuffer[iByte + 4]));
              }
              //std::cout << "*" << frame->getCommand() << "*";
              //flush(std::cout);

              // the show must go on...
              m_MessageLength = -1;
              m_State = rsSynchronizing;
              return frame;
            }
          }
        }
      }
    }
    return NULL;
  } // getFrame

  //================================================== DS485FrameProvider

  void DS485FrameProvider::addFrameCollector(IDS485FrameCollector* _collector) {
    m_FrameCollectors.push_back(_collector);
  } // addFrameCollector

  void DS485FrameProvider::removeFrameCollector(IDS485FrameCollector* _collector) {
    std::vector<IDS485FrameCollector*>::iterator iCollector = find(m_FrameCollectors.begin(), m_FrameCollectors.end(), _collector);
    if(iCollector != m_FrameCollectors.end()) {
      m_FrameCollectors.erase(iCollector);
    }
  } // removeFrameCollector

  void DS485FrameProvider::distributeFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
    for(std::vector<IDS485FrameCollector*>::iterator iCollector = m_FrameCollectors.begin(), e = m_FrameCollectors.end();
        iCollector != e; ++iCollector)
    {
      (*iCollector)->collectFrame(_frame);
    }
  } // distributeFrame

  void DS485FrameProvider::distributeFrame(DS485CommandFrame* _pFrame) {
    boost::shared_ptr<DS485CommandFrame> tempRef(_pFrame);
    distributeFrame(tempRef);
  } // distributeFrame


  //================================================== PayloadDissector

  template<>
  uint8_t PayloadDissector::get() {
    uint8_t result = m_Payload.back();
    m_Payload.pop_back();
    return result;
  }

  template<>
  uint32_t PayloadDissector::get() {
    uint32_t result;
    result = (get<uint8_t>() <<  0) |
             (get<uint8_t>() <<  8) |
             (get<uint8_t>() << 16) |
             (get<uint8_t>() << 24);
    return result;
  }

  template<>
  dsid_t PayloadDissector::get() {
    dsid_t result;
    result.upper = 0;
    for(int iByte = 0; iByte < 8; iByte++) {
      result.upper |= ((uint64_t)get<uint8_t>() << ((8 - iByte - 1) * 8));
    }
    result.lower  = (get<uint8_t>() << 24);
    result.lower |= (get<uint8_t>() << 16);
    result.lower |= (get<uint8_t>() <<  8);
    result.lower |= (get<uint8_t>() <<  0);

    return result;
  }

  template<>
  uint16_t PayloadDissector::get() {
    uint16_t result;
    result = (get<uint8_t>() << 0)  |
             (get<uint8_t>() << 8);
    return result;
  }

  //================================================== DS485FrameSniffer
#ifndef __APPLE__
  DS485FrameSniffer::DS485FrameSniffer(const std::string& _deviceName)
  {
     m_SerialCom.reset(new SerialCom());
     m_SerialCom->open(_deviceName.c_str());
     m_FrameReader.setSerialCom(m_SerialCom);
  }

  void DS485FrameSniffer::run() {
    struct timespec lastFrame;
    struct timespec thisFrame;
    clock_gettime(CLOCK_REALTIME, &lastFrame);
    while(true) {
      boost::scoped_ptr<DS485Frame> frame(m_FrameReader.getFrame(500));
      if(frame.get() != NULL){
        clock_gettime(CLOCK_REALTIME, &thisFrame);
        if(frame.get() != NULL) {

          double diffMS = ((thisFrame.tv_sec*1000.0 + thisFrame.tv_nsec/1000.0/1000.0) -
                           (lastFrame.tv_sec*1000.0 + lastFrame.tv_nsec/1000.0/1000.0));

          std::cout << "+" << diffMS << "\n";

          DS485CommandFrame* cmdFrame = dynamic_cast<DS485CommandFrame*>(frame.get());
          if(cmdFrame != NULL) {
            uint8_t cmd = cmdFrame->getCommand();
            std::cout << "Command Frame: " << commandToString(cmd) <<  " " << "(" <<  (int)cmd << ") " << (int)frame->getHeader().getSource() << " -> " << (int)frame->getHeader().getDestination()  << "\n";

            if(cmd == CommandRequest || cmd == CommandResponse) {
              PayloadDissector pd(cmdFrame->getPayload());
              std::cout << (int)pd.get<uint8_t>() << "\n";
            }
          } else {
            std::cout << "token " << (int)frame->getHeader().getSource() << " -> " << (int)frame->getHeader().getDestination()  << "\n";
          }
          std::cout << "seq: " << (int)frame->getHeader().getCounter() << std::endl;

          lastFrame = thisFrame;
        }
      }
    }
  }
#endif

  //================================================== Global helpers

  const char* commandToString(const uint8_t _command) {
    switch(_command) {
    case CommandSolicitSuccessorRequestLong:
      return "solicit successor request long";
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
  } // commandToString

  const char* controllerStateToString(const aControllerState _state) {
    switch(_state) {
    case csInitial:
      return "initial";
    case csSensing:
      return "sensing";
    case csDesignatedMaster:
      return "designated master";
    case csBroadcastingDSID:
      return "broadcasting DSID";
    case csMaster:
      return "master";
    case csSlaveWaitingToJoin:
      return "slave waiting to join";
    case csSlaveJoining:
      return "slave joining";
    case csSlave:
      return "slave";
    case csSlaveWaitingForFirstToken:
      return "slave waiting for first token";
    case csError:
      return "error";
    case csCommError:
      return "comm error";
    default:
      return "(unknown)";
    }
  } // controllerStateToString

  const dsid_t NullDSID(0,0);

}
