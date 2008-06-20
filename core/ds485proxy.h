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

#include "model.h"

#include "ds485types.h"
#include "ds485.h"
#include "syncevent.h"

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
  
  typedef enum {
    cmdTurnOn,
    cmdTurnOff,
    cmdStartDimUp,
    cmdStartDimDown,
    cmdStopDim,
    cmdCallScene,
    cmdSaveScene,
    cmdUndoScene,
    cmdIncreaseValue,
    cmdDecreaseValue,
    cmdEnable,
    cmdDisable,
    cmdIncreaseParam,
    cmdDecreaseParam,
    cmdGetOnOff
  } DS485Command;
  
  //================================================== Simulation stuff ahead
  
  class DSIDSim;
  
  class DSModulatorSim : public DS485FrameProvider {
  private:
    int m_ID;
    vector<DSIDSim*> m_SimulatedDevices;
    map< const int, vector<DSIDSim*> > m_Rooms;
    map< const pair<const int, const int>,  vector<DSIDSim*> > m_DevicesOfGroupInRoom;
    vector<DS485Frame*> m_PendingFrames;
  private:  
    DSIDSim& LookupDevice(int _id);
    DS485CommandFrame* CreateResponse(DS485CommandFrame& _request, uint8 _functionID);
    DS485CommandFrame* CreateAck(DS485CommandFrame& _request, uint8 _functionID);
    DS485CommandFrame* CreateReply(DS485CommandFrame& _request);
  public:
    DSModulatorSim();
    virtual ~DSModulatorSim() {};
    void Initialize();
    
    int GetID() const;

    void Send(DS485Frame& _frame);
  };
  
  class DSIDSim {
  private:
    const int m_Id;
    bool m_On;
    bool m_Enabled;
    bool m_Dimming;
    time_t m_DimmStartTime;
    bool m_DimmingUp;
    vector<int> m_Parameters;
    DSModulatorSim* m_Modulator;
    vector<uint8> m_ValuesForScene;
    uint8 m_CurrentValue;
    int m_DimTimeMS;
  public:
    DSIDSim(const int _id);
    
    int GetID();
    
    void CallScene(const int _sceneNr);
    void SaveScene(const int _sceneNr);
    void UndoScene(const int _sceneNr);
    
    void IncreaseValue(const int _parameterNr = -1);
    void DecreaseValue(const int _parameterNr = -1);
    
    bool IsTurnedOn();
    
    void Enable();
    void Disable();
    
    void StartDim(bool _directionUp, const int _parameterNr = -1);
    void EndDim(const int _parameterNr = -1);
    void SetValue(const double _value, int _parameterNr = -1);
  };
  
  class ReceivedFrame {
  private:
    int m_ReceivedAtToken;
    boost::shared_ptr<DS485CommandFrame> m_Frame;
  public:
    ReceivedFrame(const int _receivedAt, DS485CommandFrame* _frame);
    DS485CommandFrame& GetFrame() { return *m_Frame.get(); };
    int GetReceivedAt() const { return m_ReceivedAtToken; };
  };
  
  typedef map<int, vector<ReceivedFrame*> > FramesByID;
  typedef vector<boost::shared_ptr<DS485CommandFrame> > CommandFrameSharedPtrVector;
  
  class DS485Proxy : protected Thread,
                     public    IDS485FrameCollector {
  private:
    FittingResult BestFit(Set& _set);
    bool IsSimAddress(const uint8 _addr);

    void SendFrame(DS485Frame& _frame);
    vector<DS485Frame*> Receive(uint8 _functionID);
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
    
    //------------------------------------------------ Handling
    void Start();
    void WaitForProxyEvent();
    
    virtual void CollectFrame(boost::shared_ptr<DS485CommandFrame> _frame);
    
    //------------------------------------------------ Specialized Commands (system)
    vector<int> GetModulators();
    
    vector<int> GetRooms(const int _modulatorID);
    int GetRoomCount(const int _modulatorID);
    vector<int> GetDevicesInRoom(const int _modulatorID, const int _roomID);
    int GetDevicesCountInRoom(const int _modulatorID, const int _roomID);

    int GetGroupCount(const int _modulatorID, const int _roomID);
    vector<int> GetGroups(const int _modulatorID, const int _roomID);
    int GetDevicesInGroupCount(const int _modulatorID, const int _roomID, const int _groupID);
    vector<int> GetDevicesInGroup(const int _modulatorID, const int _roomID, const int _groupID);

    void AddToGroup(const int _modulatorID, const int _groupID, const int _deviceID);
    void RemoveFromGroup(const int _modulatorID, const int _groupID, const int _deviceID);
    
    int AddUserGroup(const int _modulatorID);
    void RemoveUserGroup(const int _modulatorID, const int _groupID);
    
    //------------------------------------------------ Device manipulation    
    vector<int> SendCommand(DS485Command _cmd, Set& _set);
    vector<int> SendCommand(DS485Command _cmd, Device& _device);
    vector<int> SendCommand(DS485Command _cmd, devid_t _id, uint8 _modulatorID);
    vector<int> SendCommand(DS485Command _cmd, const Modulator& _modulator, Group& _group);    
  };
}

#endif
