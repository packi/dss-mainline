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
  
  vector<unsigned char> DS485Payload::ToChar() {
    return m_Data;
  } // GetData
  
  
  //================================================== DS485Header
  
  vector<unsigned char> DS485Header::ToChar() {
    vector<unsigned char> result;
    result.push_back(FrameStart);
    unsigned char tmp = (m_Destination << 2) | ((m_Broadcast & 0x01) << 1) | (m_Type & 0x01); 
    result.push_back(tmp);
    tmp = ((m_Source & 0x3F) << 2) | (m_Counter & 0x03);
    result.push_back(tmp);
    
    return result;
  } // ToChar

  
  //================================================== DS485Frame
  
  vector<unsigned char> DS485Frame::ToChar() {
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
  
  vector<unsigned char> DS485CommandFrame::ToChar() {
    vector<unsigned char> result = DS485Frame::ToChar();
    unsigned char tmp = (m_Command << 4) | (m_Length & 0x0F);
    result.push_back(tmp);

    vector<unsigned char> payload = GetPayload().ToChar();
    result.insert(result.end(), payload.begin(), payload.end());
    return result;
  } // ToChar
  
  //================================================== DS485Controller
  
  char DS485Controller::GetChar() {
  } // Getchar
  
  DS485Frame* DS485Controller::GetFrameFromWire() {
  } // GetFrameFromWire
  
  bool DS485Controller::PutFrameOnWire(const DS485Frame* _pFrame) {
  } // PutFrameOnWire
  
  DS485Controller::DS485Controller() {
  } // ctor
  
  DS485Controller::~DS485Controller() {
  } // dtor
  
  void DS485Controller::Execute() {
    while(!m_Terminated) {
      SleepMS(10);
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
  } // GetFrame
  
  bool DS485FrameReader::GetCharTimeout(char& _charOut, const int _timeoutSec) {
    return m_SerialCom->GetCharTimeout(_charOut, _timeoutSec);
  } // GetCharTimeout
  
  typedef enum {
    rsSynchronizing,
    rsReadingHeader,
    rsReadingPacket,
    rsReadingCRC
  } aReaderState;
  
  const int TheReceiveBufferSizeBytes = 50;
  
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
            if(validBytes == 3) {
              if(received[1] & 0x02 == 0x02) {
                Logger::GetInstance()->Log("Packet is a broadcast");
              }
              // check if it's a token or not
              if(received[1] & 0x01 == 0x01) {
                Logger::GetInstance()->Log("Packet is a Frame");
                state = rsReadingPacket;
              } else {
                Logger::GetInstance()->Log("Packet is a Token");
              }
            }
            break;
          }
            
          case rsReadingPacket:
          {
            if(messageLen == -1 && validBytes >= 4) {
              // the length does not include the size of the headers
              messageLen = received[3] & 0x0F;
            }
            if(validBytes == (messageLen + 4 /* header */)) {
              Logger::GetInstance()->Log("frame received");
              state = rsReadingCRC;
            }
            break;
          }
            
          case rsReadingCRC:
          {
            if(validBytes == messageLen + 4 /* header */ + 2 /* CRC */) {
              uint16_t receivedCRC = (uint16_t)received[validBytes - 2] << 8 | received[validBytes - 1];
              uint16_t dataCRC = CRC16(received, validBytes - 2 /* CRC */);
              if(receivedCRC != dataCRC) {
                stringstream s;
                s << "crc missmatch. in packet: " << receivedCRC << " calculated: " << dataCRC << endl;
                Logger::GetInstance()->Log(s.str(), lsError);
              } else {
                Logger::GetInstance()->Log("received packet, crc ok");
              }
            }
          }
        }
      }
    }
  } // Execute
  
  
}