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

#include <map>
#include <vector>

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
    cmdDecreaseParam
  } DS485Command;
  
  class DS485Interface {
  public:
    virtual void Send(DS485Frame& _frame) = 0;
    virtual DS485Frame& Receive() = 0;
  };
  
  //================================================== Simulation stuff ahead
  
  class DSIDSim;
  
  class DSModulatorSim : public DS485Interface {
  private:
    int m_ID;
    vector<DSIDSim*> m_SimulatedDevices;
    map< const int, vector<DSIDSim*> > m_Groups;
    map< const int, vector<DSIDSim*> > m_Rooms;
    vector<DS485Frame> m_PendingFrames;
  private:  
    DSIDSim& LookupDevice(int _id);
    DS485CommandFrame CreateResponse(DS485CommandFrame& _request, uint8 _functionID);
    DS485CommandFrame CreateAck(DS485CommandFrame& _request, uint8 _functionID);
    DS485CommandFrame CreateReply(DS485CommandFrame& _request);
  public:
    DSModulatorSim();
    void Initialize();
    
    int GetID() const;

    virtual void Send(DS485Frame& _frame);
    bool HasPendingFrame();
    virtual DS485Frame& Receive();
  };
  
  class DSIDSim {
  private:
    const int m_Id;
    bool m_On;
    bool m_Enabled;
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
    
    void Enable();
    void Disable();
    
    void StartDim(bool _directionUp, const int _parameterNr = -1);
    void EndDim(const int _parameterNr = -1);
    void SetValue(const double _value, int _parameterNr = -1);
  };
  
  class DS485Proxy {
  private:
    FittingResult BestFit(Set& _set);
    bool IsSimAddress(const uint8 _addr);

    void SendFrame(DS485Frame& _frame);
    vector<DS485Frame> Receive(uint8 _functionID);
    uint8 ReceiveSingleResult(uint8 _functionID);
  public:
    //------------------------------------------------ Specialized Commands (system)
    vector<int> GetModulators();
    int GetGroupCount(const int _modulatorID);
    vector<int> GetDevicesInGroup(const int _modulatorID, const int _groupID);
    
    vector<int> GetRooms(const int _modulatorID);
    int GetRoomCount(const int _modulatorID);
    vector<int> GetDevicesInRoom(const int _modulatorID, const int _roomID);
    int GetDevicesCountInRoom(const int _modulatorID, const int _roomID);

    ///vector<int> GetDevices(const int _modulatorID);
    
    void AddToGroup(const int _modulatorID, const int _groupID, const int _deviceID);
    void RemoveFromGroup(const int _modulatorID, const int _groupID, const int _deviceID);
    
    int AddUserGroup(const int _modulatorID);
    void RemoveUserGroup(const int _modulatorID, const int _groupID);
    
    //------------------------------------------------ Device manipulation    
    void SendCommand(DS485Command _cmd, Set& _set);
    void SendCommand(DS485Command _cmd, Device& _device);
    void SendCommand(DS485Command _cmd, devid_t _id, uint8 _modulatorID);
    void SendCommand(DS485Command _cmd, const Modulator& _modulator, Group& _group);
    
  };
}

#endif
