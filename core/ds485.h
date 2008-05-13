/*
 *  ds485.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/29/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _DS485_H_INCLUDED
#define _DS485_H_INCLUDED

#include "ds485types.h"

#include <vector>

using namespace std;

namespace dss {
  
  class DS485Payload {
  private:
    vector<unsigned char> m_Data;
  public:
    template<class t>
    void Add(t _data);
    
    vector<unsigned char> ToChar();
  }; // DS485Payload
  
  class DS485Header {
  private:
    uint8 m_Destination;
    bool m_Broadcast;
    uint8 m_Source;
    uint8 m_Counter;
    uint8 m_Type;
  public:
    uint8 GetDestination() const { return m_Destination; };
    bool IsBroadcast() const { return m_Broadcast; };
    uint8 GetSource() const { return m_Source; };
    uint8 GetCounter() const { return m_Counter; };
    uint8 GetType() const { return m_Type; };
    
    void SetDestination(const uint8 _value) { m_Destination = _value; };
    void SetBroadcast(const bool _value) { m_Broadcast = _value; };
    void SetSource(const uint8 _value) { m_Source = _value; };
    void SetCounter(const uint8 _value) { m_Counter = _value; };
    void SetType(const uint8 _value) { m_Type = _value; };
    
    vector<unsigned char> ToChar();
  };
  
  class DS485Frame {
  private:
    DS485Header m_Header;
    DS485Payload m_Payload;
  public:
    DS485Frame() {};
    virtual ~DS485Frame() {};
    DS485Header& GetHeader() { return m_Header; };
    
    virtual vector<unsigned char> ToChar();
    
    DS485Payload& GetPayload();
  }; // DS485Frame
  
  class DS485CommandFrame : public DS485Frame {
  private:
    uint8 m_Command;
    uint8 m_Length;
  public:
    DS485CommandFrame();
    virtual ~DS485CommandFrame() {};
    
    uint8 GetCommand() const { return m_Command; };
    uint8 GetLength() const { return m_Length; };
    
    void SetCommand(const uint8 _value) { m_Command = _value; };
    void SetLength(const uint8 _value) { m_Length = _value; };
    
    virtual vector<unsigned char> ToChar();
  };
  
  class DS485Transport {
  private:
    int m_LogicalID;
    int m_LogicalSuccessorID;
    devid_t m_DSID;
  public:
    void Transmit(const DS485Frame& _frame);
    DS485Frame Receive();
  };
  
  
  typedef enum {
    
  } DS485ComState;
  
  class DS485Endpoint {
  private:
    DS485Transport m_Transport; 
  };
  
}

#endif