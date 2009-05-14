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

#include "core/thread.h"
#include "serialcom.h"
#include "core/ds485types.h"
#include "core/syncevent.h"

#include <vector>
#include <string>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>

namespace dss {

  class DS485Payload {
  private:
    std::vector<unsigned char> m_Data;
  public:
    template<class t>
    void Add(t _data);

    int Size() const;

    std::vector<unsigned char> ToChar() const;
  }; // DS485Payload

  class DS485Header {
  private:
    uint8_t m_Type;
    uint8_t m_Counter;
    uint8_t m_Source;
    bool m_Broadcast;
    uint8_t m_Destination;
  public:
    DS485Header()
    : m_Type(0x00),
      m_Counter(0x00),
      m_Source(0xff),
      m_Broadcast(false),
      m_Destination(0xff)
    {};

    uint8_t GetDestination() const { return m_Destination; };
    bool IsBroadcast() const { return m_Broadcast; };
    uint8_t GetSource() const { return m_Source; };
    uint8_t GetCounter() const { return m_Counter; };
    uint8_t GetType() const { return m_Type; };

    void SetDestination(const uint8_t _value) { m_Destination = _value; };
    void SetBroadcast(const bool _value) { m_Broadcast = _value; };
    void SetSource(const uint8_t _value) { m_Source = _value; };
    void SetCounter(const uint8_t _value) { m_Counter = _value; };
    void SetType(const uint8_t _value) { m_Type = _value; };

    std::vector<unsigned char> ToChar() const;
    void FromChar(const unsigned char* _data, const int _len);
  };

  typedef enum {
    fsWire,
    fsDSS
  } aFrameSource; 
  
  class DS485Frame {
  private:
    DS485Header m_Header;
    DS485Payload m_Payload;
    aFrameSource m_FrameSource;
  public:
    DS485Frame() {};
    virtual ~DS485Frame() {};
    DS485Header& GetHeader() { return m_Header; };

    virtual std::vector<unsigned char> ToChar() const;

    DS485Payload& GetPayload();
    const DS485Payload& GetPayload() const;
    
    aFrameSource GetFrameSource() const { return m_FrameSource; }
    void SetFrameSource(aFrameSource _value) { m_FrameSource; }
  }; // DS485Frame

  class DS485CommandFrame : public DS485Frame {
  private:
    uint8_t m_Command;
    uint8_t m_Length;
  public:
    DS485CommandFrame();
    virtual ~DS485CommandFrame() {};

    uint8_t GetCommand() const { return m_Command; };
    uint8_t GetLength() const { return m_Length; };

    void SetCommand(const uint8_t _value) { m_Command = _value; };
    void SetLength(const uint8_t _value) { m_Length = _value; };

    virtual std::vector<unsigned char> ToChar() const;
  };

  class IDS485FrameCollector;

  /** A frame provider receives frames from somewhere */
  class DS485FrameProvider {
  private:
    std::vector<IDS485FrameCollector*> m_FrameCollectors;
  protected:
    /** Distributes the frame to the collectors */
    void DistributeFrame(boost::shared_ptr<DS485CommandFrame> _frame);
    /** Distributes the frame to the collectors.
     * NOTE: the ownership of the frame is transfered to the frame provider
     */
    void DistributeFrame(DS485CommandFrame* _pFrame);
  public:
    void AddFrameCollector(IDS485FrameCollector* _collector);
    void RemoveFrameCollector(IDS485FrameCollector* _collector);
  };

  typedef enum {
    csInitial,
    csSensing,
    csDesignatedMaster,
    csBroadcastingDSID,
    csMaster,
    csSlaveWaitingToJoin,
    csSlaveJoining,
    csSlave,
    csSlaveWaitingForFirstToken,
    csError
  } aControllerState;

  typedef enum {
    rsSynchronizing,
    rsReadingHeader,
    rsReadingPacket,
    rsReadingCRC
  } aReaderState;

  const int TheReceiveBufferSizeBytes = 50;

  class DS485FrameReader {
  private:
    // receive functions's state
    int m_ValidBytes;
    unsigned char m_ReceiveBuffer[TheReceiveBufferSizeBytes];
    aReaderState m_State;
    int m_MessageLength;

    bool m_EscapeNext;
    bool m_IsEscaped;
    int m_NumberOfFramesReceived;
    int m_NumberOfIncompleteFramesReceived;
    int m_NumberOfCRCErrors;
  private:
    int m_Handle;
    boost::shared_ptr<SerialComBase> m_SerialCom;
  private:
    bool GetCharTimeout(char& _charOut, const int _timeoutMS);
  public:
    DS485FrameReader();
    virtual ~DS485FrameReader();

    void SetSerialCom(boost::shared_ptr<SerialComBase> _serialCom);

    DS485Frame* GetFrame(const int _timeoutMS);
    bool SenseTraffic(const int _timeoutMS);

    int GetNumberOfFramesReceived() const { return m_NumberOfFramesReceived; }
    int GetNumberOfIncompleteFramesReceived() const { return m_NumberOfIncompleteFramesReceived; }
    int GetNumberOfCRCErrors() const { return m_NumberOfCRCErrors; }
  }; // FrameReader

  class DS485Controller : public Thread,
                          public DS485FrameProvider {
  private:
    aControllerState m_State;
    DS485FrameReader m_FrameReader;
    std::string m_RS485DeviceName;
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

    inline void PutCharOnWire(const unsigned char _ch);

    void DoChangeState(aControllerState _newState);
    void AddToReceivedQueue(DS485CommandFrame* _frame);
  public:
    DS485Controller();
    virtual ~DS485Controller();

    void SetRS485DeviceName(const std::string& _value);
    const std::string& GetRS485DeviceName() const { return m_RS485DeviceName; }

    void EnqueueFrame(DS485CommandFrame& _frame);
    bool WaitForEvent(const int _timeoutMS);
    void WaitForCommandFrame();
    void WaitForToken();

    aControllerState GetState() const;
    int GetTokenCount() const { return m_TokenCounter; };

    const DS485FrameReader& GetFrameReader() const { return m_FrameReader; }

    virtual void Execute();

    int GetStationID() const { return m_StationID; }
  }; // DS485Controller

  class IDS485FrameCollector {
  public:
    virtual void CollectFrame(boost::shared_ptr<DS485CommandFrame>& _frame) = 0;
    virtual ~IDS485FrameCollector() {};
  }; // DS485FrameCollector

  class DS485FrameSniffer {
  private:
    DS485FrameReader m_FrameReader;
    boost::shared_ptr<SerialCom> m_SerialCom;
  public:
    DS485FrameSniffer(const std::string& _deviceName);

    void Run();
  }; // IDS485FrameSniffer

  class PayloadDissector {
  private:
    std::vector<unsigned char> m_Payload;
  public:
    PayloadDissector(DS485Payload& _payload) {
      std::vector<unsigned char> payload =_payload.ToChar();
      m_Payload.insert(m_Payload.begin(), payload.rbegin(), payload.rend());
    }

    bool IsEmpty() { return m_Payload.empty(); };

    template<class t>
    t Get();
  }; // PayloadDissector

  const char* CommandToString(const uint8_t _command);

  const uint32_t SimulationPrefix = 0xFFC00000;
  inline bool IsSimulationDSID(const dsid_t _dsid) {
    return _dsid.lower & SimulationPrefix == SimulationPrefix;
  }

}

#endif
