/*
 *  ds485proxy.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/18/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "ds485proxy.h"

#include "model.h"
#include "dss.h"

namespace dss {

  const uint8 CommandRequest = 0x09;
  const uint8 CommandResponse = 0x0a;
  const uint8 CommandACK = 0x0b;
  const uint8 CommandBusy = 0x0c;

  
  const uint8 FunctionModulatorAddRoom = 0x00;
  const uint8 FunctionModulatorRemoveRoom = 0x01;
  const uint8 FunctionModulatorRemoveAllRooms = 0x02;
  const uint8 FunctionModulatorCountDevInRoom = 0x03;
  const uint8 FunctionModulatorDevKeyInRoom = 0x04;
  const uint8 FunctionModulatorGetGroupsSize = 0x05;
  const uint8 FunctionModulatorGetRoomsSize  = 0x06;
  const uint8 FunctionModulatorGetRoomIdForInd = 0x07;
  const uint8 FunctionModulatorAddToGroup = 0x08;
  const uint8 FunctionModulatorRemoveFromGroup = 0x09;


  
  const uint8 FunctionDeviceCallScene = 0x42;
  const uint8 FunctionDeviceIncValue  = 0x40;
  const uint8 FunctionDeviceDecValue  = 0x41;
  
  const uint8 SceneOff = 0x00;
  const uint8 Scene1 = 0x01;
  const uint8 Scene2 = 0x02;
  const uint8 Scene3 = 0x03;
  const uint8 Scene4 = 0x04;
  const uint8 SceneStandBy = 0x05;
  const uint8 SceneDeepOff = 0x06;
  
  typedef hash_map<const Modulator*, Set> HashMapModulatorSet;
  
  HashMapModulatorSet SplitByModulator(Set& _set) {
    HashMapModulatorSet result;
    for(int iDevice = 0; iDevice < _set.Length(); iDevice++) {
      DeviceReference& dev = _set.Get(iDevice);
      Modulator& mod = dev.GetDevice().GetApartment().GetModulator(dev.GetDevice().GetModulatorID());
      result[&mod].AddDevice(dev);
    }
    return result;
  } // SplitByModulator
  
  typedef pair<vector<Group*>, Set> FittingResultPerModulator;

  
  FittingResultPerModulator BestFit(const Modulator& _modulator, Set& _set) {
    Set workingCopy = _set;
    
    vector<Group*> unsuitableGroups;
    vector<Group*> fittingGroups;
    Set singleDevices;
    
    while(!workingCopy.IsEmpty()) {
      DeviceReference& ref = workingCopy.Get(0);
      workingCopy.RemoveDevice(ref);
      
      bool foundGroup = false;
      for(int iGroup = 0; iGroup < ref.GetDevice().GetGroupsCount(); iGroup++) {
        Group& g = ref.GetDevice().GetGroupByIndex(iGroup);
        
        // continue if already found unsuitable
        if(find(unsuitableGroups.begin(), unsuitableGroups.end(), &g) != unsuitableGroups.end()) {
          continue;
        }
        
        // see if we've got a fit
        bool groupFits = true;
        Set devicesInGroup = _modulator.GetDevices().GetByGroup(g);
        for(int iDevice = 0; iDevice < devicesInGroup.Length(); iDevice++) {
          if(!_set.Contains(devicesInGroup.Get(iDevice))) {
            unsuitableGroups.push_back(&g);
            groupFits = false;
            break;
          }
        }
        if(groupFits) {
          foundGroup = true;
          fittingGroups.push_back(&g);
          while(!devicesInGroup.IsEmpty()) {
            workingCopy.RemoveDevice(devicesInGroup.Get(0));
          }
          break;
        }        
      }
      
      // if no fitting group found
      if(!foundGroup) {
        singleDevices.AddDevice(ref);
      }
    }
    return FittingResultPerModulator(fittingGroups, singleDevices);
  }
  
  FittingResult DS485Proxy::BestFit(Set& _set) {
    FittingResult result;
    HashMapModulatorSet modulatorToSet = SplitByModulator(_set);
    
    for(HashMapModulatorSet::iterator it = modulatorToSet.begin(); it != modulatorToSet.end(); ++it) {
      result[it->first] = dss::BestFit(*(it->first), it->second); 
    }
    
    return result;
  }
  
  void DS485Proxy::SendCommand(DS485Command _cmd, Set& _set) {
    FittingResult fittedResult = BestFit(_set);
    for(FittingResult::iterator iResult = fittedResult.begin(); iResult != fittedResult.end(); ++iResult) {
      const Modulator* modulator = iResult->first;
      FittingResultPerModulator res = iResult->second;
      vector<Group*> groups = res.first;
      for(vector<Group*>::iterator ipGroup = groups.begin(); ipGroup != groups.end(); ++ipGroup) {
        SendCommand(_cmd, *modulator, **ipGroup);
      }
      Set& set = res.second; 
      for(int iDevice = 0; iDevice < set.Length(); iDevice++) {
        SendCommand(_cmd, set.Get(iDevice).GetDevice());
      }
    }
  } // SendCommand
  
  void DS485Proxy::SendCommand(DS485Command _cmd, const Modulator& _modulator, Group& _group) {
  } // SendCommand
 
  void DS485Proxy::SendCommand(DS485Command _cmd, Device& _device) {
    SendCommand(_cmd, _device.GetID(), _device.GetModulatorID());
  } // SendCommand
  
  void DS485Proxy::SendCommand(DS485Command _cmd, devid_t _id, uint8 _modulatorID) {
    DS485CommandFrame frame;
    frame.GetHeader().SetDestination(_modulatorID);
    frame.GetHeader().SetBroadcast(false);
    frame.GetHeader().SetType(1);
    frame.SetCommand(CommandRequest);
    if(_cmd == cmdTurnOn) {
      frame.GetPayload().Add<uint8>(FunctionDeviceCallScene);
      frame.GetPayload().Add<uint8>(_id);
      frame.GetPayload().Add<uint8>(Scene1);
      SendFrame(frame);
    } else if(_cmd == cmdTurnOff) {
      frame.GetPayload().Add<uint8>(FunctionDeviceCallScene);
      frame.GetPayload().Add<uint8>(_id);
      frame.GetPayload().Add<uint8>(SceneOff);
      SendFrame(frame);
    }
  } // SendCommand
  
  void DS485Proxy::SendFrame(DS485Frame& _frame) {
    if(IsSimAddress(_frame.GetHeader().GetDestination())) {
      DSS::GetInstance()->GetModulatorSim().Send(_frame);
    }
  }

  bool DS485Proxy::IsSimAddress(const uint8 _addr) {
    return true;
  } // IsSimAddress
  
  vector<int> DS485Proxy::GetModulators() {
    vector<int> result;
    result.push_back(DSS::GetInstance()->GetModulatorSim().GetID());
    return result;
  } // GetModulators
  
  int DS485Proxy::GetGroupCount(const int _modulatorID) {  
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8>(FunctionModulatorGetGroupsSize);
    SendFrame(cmdFrame);
  } // GetGroupCount
  
  vector<int> DS485Proxy::GetDevicesInGroup(const int _modulatorID, const int _groupID) {
  } // GetDevicesInGroup
  
  vector<int> DS485Proxy::GetRooms(const int _modulatorID) {
    
  } // GetRooms
  
  int DS485Proxy::GetRoomCount(const int _modulatorID) {
  } // GetRoomCount
  
  vector<int> DS485Proxy::GetDevicesInRoom(const int _modulatorID, const int _roomID) {
  } // GetDevicesInRoom
  vector<DS485Frame> DS485Proxy::Receive(uint8 _functionID) {
  } // Receive
 
  
  //================================================== DSModulatorSim
  
  DSModulatorSim::DSModulatorSim() 
  {
  } // DSModulatorSim

  
  void DSModulatorSim::Initialize() {
    m_ID = 1;
    m_SimulatedDevices.push_back(new DSIDSim(1));
  }
  
  const uint8 HeaderTypeToken = 0;
  const uint8 HeaderTypeCommand = 1;
  
  
  class PayloadDissector {
  private:
    vector<unsigned char> m_Payload;
  public:
    PayloadDissector(DS485Payload& _payload) {
      vector<unsigned char> payload =_payload.ToChar();
      m_Payload.insert(m_Payload.begin(), payload.rbegin(), payload.rend());
    }
    
    template<class t>
    t Get();
  };
  
  template<>
  uint8 PayloadDissector::Get() {
    uint8 result = m_Payload.back();
    m_Payload.pop_back();
    return result;
  }
  
  void DSModulatorSim::Send(DS485Frame& _frame) {
    DS485Header& header = _frame.GetHeader();
    if(header.GetType() == HeaderTypeToken ) {
      // Transmit pending things
    } else if(header.GetType() == HeaderTypeCommand) {
      DS485CommandFrame& cmdFrame = dynamic_cast<DS485CommandFrame&>(_frame);
      PayloadDissector pd(cmdFrame.GetPayload());
      if(cmdFrame.GetCommand() == CommandRequest) {
        int cmdNr = pd.Get<uint8>();
        switch(cmdNr) {
          case FunctionDeviceCallScene:
            LookupDevice(pd.Get<uint8>()).CallScene(pd.Get<uint8>());
            break;
        }
      }
    }
  } // Send
  
  DSIDSim& DSModulatorSim::LookupDevice(int _id) {
    for(vector<DSIDSim*>::iterator ipSimDev = m_SimulatedDevices.begin(); ipSimDev != m_SimulatedDevices.end(); ++ipSimDev) {
      if((*ipSimDev)->GetID() == _id) {
        return **ipSimDev;
      }
    }
    throw new runtime_error("could not find device");
  } // LookupDevice
  
  DS485Frame& DSModulatorSim::Receive() {
    throw new runtime_error("not yet implemented");
  } // Receive
  
  int DSModulatorSim::GetID() const {
    return m_ID;
  } // GetID

  
  //================================================== DSIDSim
  
  DSIDSim::DSIDSim(const int _id) 
  : m_Id(_id),
    m_Enabled(true)
  {
    m_ValuesForScene.push_back(0);   // OFF
    m_ValuesForScene.push_back(255); // Scene1
    m_ValuesForScene.push_back(255); // Scene2
    m_ValuesForScene.push_back(255); // Scene3
    m_ValuesForScene.push_back(255); // Scene4
    m_ValuesForScene.push_back(0);   // SceneStandby
    m_ValuesForScene.push_back(0);   // SceneDeepOff
  } // ctor
  
  int DSIDSim::GetID() {
    return m_Id;
  } // GetID
  
  void DSIDSim::CallScene(const int _sceneNr) {
    if(m_Enabled) {
      m_CurrentValue = m_ValuesForScene.at(_sceneNr);
    }
  } // CallScene
  
  void DSIDSim::SaveScene(const int _sceneNr) {
    if(m_Enabled) {
      m_ValuesForScene[_sceneNr] = m_CurrentValue;
    }
  } // SaveScene
  
  void DSIDSim::UndoScene(const int _sceneNr) {
    if(m_Enabled) {
      m_CurrentValue = m_ValuesForScene.at(_sceneNr);
    }
  } // UndoScene
  
  void DSIDSim::IncreaseValue(const int _parameterNr) {
    if(m_Enabled) {
      m_CurrentValue++;
    }
  } // IncreaseValue
  
  void DSIDSim::DecreaseValue(const int _parameterNr) {
    if(m_Enabled) {
      m_CurrentValue--;
    }
  } // DecreaseValue
  
  void DSIDSim::Enable() {
    m_Enabled = true;
  } // Enable
  
  void DSIDSim::Disable() {
    m_Enabled = false;
  } // Disable
  
  void DSIDSim::StartDim(bool _directionUp, const int _parameterNr) {
    if(m_Enabled) {
      //...
    }
  } // StartDim
  
  void DSIDSim::EndDim(const int _parameterNr) {
    if(m_Enabled) {
      //...
    }
  } // EndDim
  
  void DSIDSim::SetValue(const double _value, int _parameterNr) {
    if(m_Enabled) {
      m_CurrentValue = _value;
    }
  } // SetValue
   
}
