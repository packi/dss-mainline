/*
 *  ds485proxy.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/18/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _DS485_PROXY_H_INCLUDED
#define _DS485_PROXY_H_INCLUDED

#include "core/model.h"

#include "core/ds485types.h"
#include "ds485.h"
#include "core/syncevent.h"
#include "core/DS485Interface.h"
#include "core/subsystem.h"
#include "core/mutex.h"

#include <map>
#include <vector>

#ifndef WIN32
  #include <ext/hash_map>
#else
  #include <hash_map>
#endif

#ifndef WIN32
using namespace __gnu_cxx;
#else
using namespace stdext;
#endif

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/shared_ptr.hpp>

using namespace std;

namespace dss {

  class DS485Proxy;
  typedef hash_map<const Zone*, pair< vector<Group*>, Set> > FittingResult;

  class ReceivedFrame {
  private:
    int m_ReceivedAtToken;
    boost::shared_ptr<DS485CommandFrame> m_Frame;
  public:
    ReceivedFrame(const int _receivedAt, boost::shared_ptr<DS485CommandFrame> _frame);
    boost::shared_ptr<DS485CommandFrame> GetFrame() { return m_Frame; };
    int GetReceivedAt() const { return m_ReceivedAtToken; };
  }; // ReceivedFrame

  /** A frame bucket holds response-frames for any given function id */
  class FrameBucket {
  private:
    deque<boost::shared_ptr<ReceivedFrame> > m_Frames;
    DS485Proxy* m_pProxy;
    int m_FunctionID;
    int m_SourceID;
    SyncEvent m_PacketHere;
  public:
    FrameBucket(DS485Proxy* _proxy, int _functionID, int _sourceID);
    ~FrameBucket();

    int GetFunctionID() const { return m_FunctionID; }
    int GetSourceID() const { return m_SourceID; }

    void AddFrame(boost::shared_ptr<ReceivedFrame> _frame);
    boost::shared_ptr<ReceivedFrame> PopFrame();
    void WaitForFrames(int _timeoutMS);
    void WaitForFrame(int _timeoutMS);

    int GetFrameCount() const;
    bool IsEmpty() const;
  }; // FrameBucket

  typedef vector<boost::shared_ptr<DS485CommandFrame> > CommandFrameSharedPtrVector;

  class DS485Proxy : protected Thread,
                     public    Subsystem,
                     public    DS485Interface,
                     public    IDS485FrameCollector {
  private:
    FittingResult BestFit(const Set& _set);
    bool IsSimAddress(const uint8_t _addr);

    boost::shared_ptr<ReceivedFrame> ReceiveSingleFrame(DS485CommandFrame& _frame, uint8_t _functionID);
    uint8_t ReceiveSingleResult(DS485CommandFrame& _frame, const uint8_t _functionID);
    uint16_t ReceiveSingleResult16(DS485CommandFrame& _frame, const uint8_t _functionID);

    vector<FrameBucket*> m_FrameBuckets;

    void SignalEvent();

    DS485Controller m_DS485Controller;
    SyncEvent m_ProxyEvent;

    SyncEvent m_PacketHere;
    Mutex m_IncomingFramesGuard;
    CommandFrameSharedPtrVector m_IncomingFrames;
  protected:
    virtual void Execute();
  public:
    DS485Proxy(DSS* _pDSS);
    virtual ~DS485Proxy() {};

    virtual bool IsReady();

    virtual void SendFrame(DS485CommandFrame& _frame);
    boost::shared_ptr<FrameBucket> SendFrameAndInstallBucket(DS485CommandFrame& _frame, const int _functionID);

    //------------------------------------------------ Handling
    virtual void Initialize();
    virtual void Start();
    void WaitForProxyEvent();

    virtual void CollectFrame(boost::shared_ptr<DS485CommandFrame>& _frame);

    void AddFrameBucket(FrameBucket* _bucket);
    void RemoveFrameBucket(FrameBucket* _bucket);

    //------------------------------------------------ Specialized Commands (system)
    virtual vector<int> GetModulators();

    virtual vector<int> GetZones(const int _modulatorID);
    virtual int GetZoneCount(const int _modulatorID);
    virtual vector<int> GetDevicesInZone(const int _modulatorID, const int _zoneID);
    virtual int GetDevicesCountInZone(const int _modulatorID, const int _zoneID);

    virtual void SetZoneID(const int _modulatorID, const devid_t _deviceID, const int _zoneID);
    virtual void CreateZone(const int _modulatorID, const int _zoneID);

    virtual int GetGroupCount(const int _modulatorID, const int _zoneID);
    virtual vector<int> GetGroups(const int _modulatorID, const int _zoneID);
    virtual int GetDevicesInGroupCount(const int _modulatorID, const int _zoneID, const int _groupID);
    virtual vector<int> GetDevicesInGroup(const int _modulatorID, const int _zoneID, const int _groupID);

    virtual void AddToGroup(const int _modulatorID, const int _groupID, const int _deviceID);
    virtual void RemoveFromGroup(const int _modulatorID, const int _groupID, const int _deviceID);

    virtual int AddUserGroup(const int _modulatorID);
    virtual void RemoveUserGroup(const int _modulatorID, const int _groupID);

    virtual dsid_t GetDSIDOfDevice(const int _modulatorID, const int _deviceID);
    virtual dsid_t GetDSIDOfModulator(const int _modulatorID);

    virtual unsigned long GetPowerConsumption(const int _modulatorID);
    virtual unsigned long GetEnergyMeterValue(const int _modulatorID);

    //------------------------------------------------ Device manipulation
    virtual vector<int> SendCommand(DS485Command _cmd, const Set& _set, int _param);
    virtual vector<int> SendCommand(DS485Command _cmd, const Device& _device, int _param);
    virtual vector<int> SendCommand(DS485Command _cmd, devid_t _id, uint8_t _modulatorID, int _param);
    virtual vector<int> SendCommand(DS485Command _cmd, const Zone& _zone, Group& _group, int _param);

    //------------------------------------------------ Helpers
    DS485Controller& GetController() { return m_DS485Controller; }
  };
}

#endif
