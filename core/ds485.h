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

#include "thread.h"
#include "serialcom.h"
#include "ds485types.h"

#include <vector>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>

using namespace std;

namespace dss {
  
  class DS485Payload {
  private:
    vector<unsigned char> m_Data;
  public:
    template<class t>
    void Add(t _data);
    
    vector<unsigned char> ToChar() const;
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
    
    vector<unsigned char> ToChar() const;
    void FromChar(const unsigned char* _data, const int _len);
  };
  
  class DS485Frame {
  private:
    DS485Header m_Header;
    DS485Payload m_Payload;
  public:
    DS485Frame() {};
    virtual ~DS485Frame() {};
    DS485Header& GetHeader() { return m_Header; };
    
    virtual vector<unsigned char> ToChar() const;
    
    DS485Payload& GetPayload();
    const DS485Payload& GetPayload() const;
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
    
    virtual vector<unsigned char> ToChar() const;
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

  typedef enum {
    csInitial,
    csSensing,
    csBroadcastingDSID,
    csMaster,
    csSlaveWaitingToJoin,
    csSlaveJoining,
    csSlave,
    csWaitingForToken
  } aControllerState;
  
  class DS485FrameReader : public Thread {
  private:
    int m_Handle;
    std::vector<DS485Frame*> m_IncomingFrames;
    bool m_Traffic;
    boost::shared_ptr<SerialComBase> m_SerialCom;
  private:
    bool GetCharTimeout(char& _charOut, const int _timeoutSec);
  public:
    DS485FrameReader();
    virtual ~DS485FrameReader();
    
    void SetSerialCom(boost::shared_ptr<SerialComBase> _serialCom);
    
    bool HasFrame();
    DS485Frame* GetFrame();
    
    virtual void Execute();
    bool GetAndResetTraffic();
  }; // FrameReader
  
  class DS485Controller : public Thread {
  private:
    aControllerState m_State;
    DS485FrameReader m_FrameReader;
    int m_StationID;
    int m_NextStationID;
    
    boost::ptr_vector<DS485Frame*> m_PendingFrames;
    boost::shared_ptr<SerialCom> m_SerialCom;
  private:
    DS485Frame* GetFrameFromWire();
    bool PutFrameOnWire(const DS485Frame* _pFrame, bool _freeFrame = true);
  public:
    DS485Controller();
    virtual ~DS485Controller();
        
    virtual void Execute();
  }; // DS485Controller
  
  class DS485ResultsAccumulator : public Thread {
  }; // DS485ResultsAccumulator
  
  
  const uint8 CommandSolicitSuccessorRequest = 0x01;
  const uint8 CommandSolicitSuccessorResponse = 0x02;
  const uint8 CommandGetAddressRequest = 0x03;
  const uint8 CommandGetAddressResponse = 0x04;
  const uint8 CommandSetDeviceAddressRequest = 0x05;
  const uint8 CommandSetDeviceAddressResponse = 0x06;
  const uint8 CommandSetSuccessorAddressRequest = 0x07;
  const uint8 CommandSetSuccessorAddressResponse = 0x08;
  const uint8 CommandRequest = 0x09;
  const uint8 CommandResponse = 0x0a;
  const uint8 CommandAck = 0x0b;
  const uint8 CommandBusy = 0x0c;
  
  const char* CommandToString(const uint8 _command);
  
}

#endif
