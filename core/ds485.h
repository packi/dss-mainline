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
#include "syncevent.h"

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
  
  class IDS485FrameCollector;
  
  /** A frame provider receives frames from somewhere */
  class DS485FrameProvider {
  private:
    vector<IDS485FrameCollector*> m_FrameCollectors;
  protected:
    /** Distributes the frame to the collectors */
    void DistributeFrame(boost::shared_ptr<DS485CommandFrame> _frame);
    /** Distributes the frame to the collectors.
     * NOTE: the ownership of the frame is tranfered to the frame provider
     */
    void DistributeFrame(DS485CommandFrame* _pFrame);
  public:
    void AddFrameCollector(IDS485FrameCollector* _collector);
    void RemoveFrameCollector(IDS485FrameCollector* _collector);
  };
  
  typedef enum {
    csInitial,
    csSensing,
    csBroadcastingDSID,
    csMaster,
    csSlaveWaitingToJoin,
    csSlaveJoining,
    csSlave,
    csSlaveWaitingForFirstToken
  } aControllerState;
  
  class DS485FrameReader : public Thread {
  private:
    int m_Handle;
    std::vector<DS485Frame*> m_IncomingFrames;
    bool m_Traffic;
    boost::shared_ptr<SerialComBase> m_SerialCom;
    SyncEvent m_PacketReceivedEvent;
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
    bool WaitForFrame();
  }; // FrameReader
  
  class DS485Controller : public Thread,
                          public DS485FrameProvider {
  private:
    aControllerState m_State;
    DS485FrameReader m_FrameReader;
    int m_StationID;
    int m_NextStationID;
    int m_TokenCounter;

    boost::ptr_vector<DS485CommandFrame> m_PendingFrames;
    boost::shared_ptr<SerialCom> m_SerialCom;
    SyncEvent m_ControllerEvent;
    SyncEvent m_CommandFrameEvent;
    SyncEvent m_TokenEvent;
  private:
    DS485Frame* GetFrameFromWire();
    bool PutFrameOnWire(const DS485Frame* _pFrame, bool _freeFrame = true);
    
    void DoChangeState(aControllerState _newState);
    void AddToReceivedQueue(DS485CommandFrame* _frame);
  public:
    DS485Controller();
    virtual ~DS485Controller();
    
    void EnqueueFrame(DS485CommandFrame* _frame);
    void WaitForEvent();
    void WaitForCommandFrame();
    void WaitForToken();
    
    aControllerState GetState() const;
    int GetTokenCount() const { return m_TokenCounter; };
        
    virtual void Execute();
  }; // DS485Controller
  
  class IDS485FrameCollector {
  public:
    virtual void CollectFrame(boost::shared_ptr<DS485CommandFrame> _frame) = 0;
    virtual ~IDS485FrameCollector() {};
  }; // DS485FrameCollector
  
  
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
