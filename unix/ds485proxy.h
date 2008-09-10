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

#include "../core/model.h"

#include "../core/ds485types.h"
#include "ds485.h"
#include "../core/syncevent.h"
#include "../core/DS485Interface.h"

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

  typedef hash_map<const Modulator*, pair< vector<Group*>, Set> > FittingResult;


  template<class t>
  class DS485Parameter {
  private:
    t _data;
  };

  class ReceivedFrame {
  private:
    int m_ReceivedAtToken;
    boost::shared_ptr<DS485CommandFrame> m_Frame;
  public:
    ReceivedFrame(const int _receivedAt, DS485CommandFrame* _frame);
    boost::shared_ptr<DS485CommandFrame> GetFrame() { return m_Frame; };
    int GetReceivedAt() const { return m_ReceivedAtToken; };
  };

  typedef map<int, vector<ReceivedFrame*> > FramesByID;
  typedef vector<boost::shared_ptr<DS485CommandFrame> > CommandFrameSharedPtrVector;

  class DS485Proxy : protected Thread,
                     public    DS485Interface,
                     public    IDS485FrameCollector {
  private:
    FittingResult BestFit(Set& _set);
    bool IsSimAddress(const uint8 _addr);

    vector<boost::shared_ptr<DS485CommandFrame> > Receive(uint8 _functionID);
    uint8 ReceiveSingleResult(uint8 _functionID);

    void SignalEvent();

    DS485Controller m_DS485Controller;
    SyncEvent m_ProxyEvent;

    SyncEvent m_PacketHere;
    FramesByID m_ReceivedFramesByFunctionID;
    CommandFrameSharedPtrVector m_IncomingFrames;
  protected:
    virtual void Execute();
  public:
    DS485Proxy();
    virtual ~DS485Proxy() {};

    void SendFrame(DS485CommandFrame& _frame, bool _force = false);

    //------------------------------------------------ Handling
    void Start();
    void WaitForProxyEvent();

    virtual void CollectFrame(boost::shared_ptr<DS485CommandFrame>& _frame);

    //------------------------------------------------ Specialized Commands (system)
    virtual vector<int> GetModulators();

    virtual vector<int> GetRooms(const int _modulatorID);
    virtual int GetRoomCount(const int _modulatorID);
    virtual vector<int> GetDevicesInRoom(const int _modulatorID, const int _roomID);
    virtual int GetDevicesCountInRoom(const int _modulatorID, const int _roomID);

    virtual int GetGroupCount(const int _modulatorID, const int _roomID);
    virtual vector<int> GetGroups(const int _modulatorID, const int _roomID);
    virtual int GetDevicesInGroupCount(const int _modulatorID, const int _roomID, const int _groupID);
    virtual vector<int> GetDevicesInGroup(const int _modulatorID, const int _roomID, const int _groupID);

    virtual void AddToGroup(const int _modulatorID, const int _groupID, const int _deviceID);
    virtual void RemoveFromGroup(const int _modulatorID, const int _groupID, const int _deviceID);

    virtual int AddUserGroup(const int _modulatorID);
    virtual void RemoveUserGroup(const int _modulatorID, const int _groupID);

    virtual dsid_t GetDSIDOfDevice(const int _modulatorID, const int _deviceID);
    virtual dsid_t GetDSIDOfModulator(const int _modulatorID);

    virtual void Subscribe(const int _moduatorID, const int _groupID, const int _deviceID);

    //------------------------------------------------ Device manipulation
    virtual vector<int> SendCommand(DS485Command _cmd, Set& _set);
    virtual vector<int> SendCommand(DS485Command _cmd, Device& _device);
    virtual vector<int> SendCommand(DS485Command _cmd, devid_t _id, uint8 _modulatorID);
    virtual vector<int> SendCommand(DS485Command _cmd, const Modulator& _modulator, Group& _group);

    //------------------------------------------------ Helpers
    DS485Controller& GetController() { return m_DS485Controller; }
  };
}

#endif
