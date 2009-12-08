/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

  /** Stores the payload of a DS485Frame */
  class DS485Payload {
  private:
    std::vector<unsigned char> m_Data;
  public:
    /** Adds \a _data to the payload. */
    template<class t>
    void add(t _data);

    /** Returns the size of the payload added.*/
    int size() const;

    /** Returns a const reference of the data */
    const std::vector<unsigned char>& toChar() const;
  }; // DS485Payload

  /** Wrapper for a DS485 header. */
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

    /** Returns the destination of the frame.
     * @note This field may not contain any useful information if isBroadcast
     * returns \c true */
    uint8_t getDestination() const { return m_Destination; };
    /** Returns whether the the frame is a broadcast or not. */
    bool isBroadcast() const { return m_Broadcast; };
    /** Returns the source of the frame */
    uint8_t getSource() const { return m_Source; };
    /** Returns the counter of the frame.
     * The use of this field is mostly for debugging purposes. Implementors
     * should increase it every packet they send to detect dropped frames. */
    uint8_t getCounter() const { return m_Counter; };
    uint8_t getType() const { return m_Type; };

    void setDestination(const uint8_t _value) { m_Destination = _value; };
    void setBroadcast(const bool _value) { m_Broadcast = _value; };
    void setSource(const uint8_t _value) { m_Source = _value; };
    void setCounter(const uint8_t _value) { m_Counter = _value; };
    void setType(const uint8_t _value) { m_Type = _value; };

    std::vector<unsigned char> toChar() const;
    void fromChar(const unsigned char* _data, const int _len);
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
    DS485Frame()
    : m_FrameSource(fsDSS)
    {}
    virtual ~DS485Frame() {}

    DS485Header& getHeader() { return m_Header; }

    DS485Payload& getPayload();
    const DS485Payload& getPayload() const;

    aFrameSource getFrameSource() const { return m_FrameSource; }
    void setFrameSource(aFrameSource _value) { m_FrameSource = _value; }

    virtual std::vector<unsigned char> toChar() const;
  }; // DS485Frame

  class DS485CommandFrame : public DS485Frame {
  private:
    uint8_t m_Command;
    uint8_t m_Length;
  public:
    DS485CommandFrame();
    virtual ~DS485CommandFrame() {};

    uint8_t getCommand() const { return m_Command; };
    uint8_t getLength() const { return m_Length; };

    void setCommand(const uint8_t _value) { m_Command = _value; };
    void setLength(const uint8_t _value) { m_Length = _value; };

    virtual std::vector<unsigned char> toChar() const;
  };

  class IDS485FrameCollector;

  /** A frame provider receives frames from somewhere */
  class DS485FrameProvider {
  private:
    std::vector<IDS485FrameCollector*> m_FrameCollectors;
  protected:
    /** Distributes the frame to the collectors */
    void distributeFrame(boost::shared_ptr<DS485CommandFrame> _frame);
    /** Distributes the frame to the collectors.
     * NOTE: the ownership of the frame is transfered to the frame provider
     */
    void distributeFrame(DS485CommandFrame* _pFrame);
  public:
    void addFrameCollector(IDS485FrameCollector* _collector);
    void removeFrameCollector(IDS485FrameCollector* _collector);
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
    csError,
    csCommError
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
    bool getCharTimeout(char& _charOut, const int _timeoutMS);
  public:
    DS485FrameReader();
    virtual ~DS485FrameReader();

    void setSerialCom(boost::shared_ptr<SerialComBase> _serialCom);

    DS485Frame* getFrame(const int _timeoutMS);
    bool senseTraffic(const int _timeoutMS);

    int getNumberOfFramesReceived() const { return m_NumberOfFramesReceived; }
    int getNumberOfIncompleteFramesReceived() const { return m_NumberOfIncompleteFramesReceived; }
    int getNumberOfCRCErrors() const { return m_NumberOfCRCErrors; }
  }; // FrameReader

  class DS485Controller : public Thread,
                          public DS485FrameProvider {
  private:
    aControllerState m_State;
    DS485FrameReader m_FrameReader;
    std::string m_RS485DeviceName;
    std::string m_StateString;
    int m_StationID;
    int m_NextStationID;
    int m_TokenCounter;

    boost::ptr_vector<DS485CommandFrame> m_PendingFrames;
    Mutex m_PendingFramesGuard;
    boost::shared_ptr<SerialCom> m_SerialCom;
    SyncEvent m_ControllerEvent;
    SyncEvent m_CommandFrameEvent;
    SyncEvent m_TokenEvent;
    dsid_t m_DSID;
  private:
    DS485Frame* getFrameFromWire();
    bool putFrameOnWire(const DS485Frame* _pFrame, bool _freeFrame = true);

    inline void putCharOnWire(const unsigned char _ch);

    void doChangeState(aControllerState _newState);
    void addToReceivedQueue(DS485CommandFrame* _frame);
    bool resetSerialLine();

    void handleSetSuccessor(DS485CommandFrame* _frame);
  public:
    DS485Controller();
    virtual ~DS485Controller();

    void setRS485DeviceName(const std::string& _value);
    const std::string& getRS485DeviceName() const { return m_RS485DeviceName; }

    void enqueueFrame(DS485CommandFrame& _frame);
    bool waitForEvent(const int _timeoutMS);
    void waitForCommandFrame();
    void waitForToken();

    aControllerState getState() const;
    int getTokenCount() const { return m_TokenCounter; };

    const std::string& getStateAsString() const;

    const DS485FrameReader& getFrameReader() const { return m_FrameReader; }

    virtual void execute();

    int getStationID() const { return m_StationID; }

    void setDSID(const dsid_t& _value) { m_DSID = _value; }
  }; // DS485Controller

  class IDS485FrameCollector {
  public:
    virtual void collectFrame(boost::shared_ptr<DS485CommandFrame> _frame) = 0;
    virtual ~IDS485FrameCollector() {};
  }; // DS485FrameCollector

  class DS485FrameSniffer {
  private:
    DS485FrameReader m_FrameReader;
    boost::shared_ptr<SerialCom> m_SerialCom;
  public:
    DS485FrameSniffer(const std::string& _deviceName);

    void run();
  }; // IDS485FrameSniffer

  class PayloadDissector {
  private:
    std::vector<unsigned char> m_Payload;
  public:
    PayloadDissector(DS485Payload& _payload) {
      const std::vector<unsigned char>& payload =_payload.toChar();
      m_Payload.insert(m_Payload.begin(), payload.rbegin(), payload.rend());
    }

    bool isEmpty() { return m_Payload.empty(); };

    template<class t>
    t get();
  }; // PayloadDissector

  const char* commandToString(const uint8_t _command);

  const uint32_t SimulationPrefix = 0xFFC00000;
  inline bool isSimulationDSID(const dsid_t _dsid) {
    return (_dsid.lower & SimulationPrefix) == SimulationPrefix;
  }

}

#endif
