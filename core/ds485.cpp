/*
 *  ds485.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/29/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "ds485.h"

namespace dss {
  
  const unsigned char FrameStart = 0xFD;
  
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
  
}