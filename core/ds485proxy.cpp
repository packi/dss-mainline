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
#include "logger.h"
#include "ds485const.h"

#include <boost/scoped_ptr.hpp>

namespace dss {
  
  typedef hash_map<const Modulator*, Set> HashMapModulatorSet;
  
  HashMapModulatorSet SplitByModulator(Set& _set) {
    HashMapModulatorSet result;
    for(int iDevice = 0; iDevice < _set.Length(); iDevice++) {
      DeviceReference& dev = _set.Get(iDevice);
      Modulator& mod = dev.GetDevice().GetApartment().GetModulatorByBusID(dev.GetDevice().GetModulatorID());
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
  
  DS485Proxy::DS485Proxy()
  : Thread(true, "DS485Proxy")
  {  
  } // ctor
  
  FittingResult DS485Proxy::BestFit(Set& _set) {
    FittingResult result;
    HashMapModulatorSet modulatorToSet = SplitByModulator(_set);
    
    for(HashMapModulatorSet::iterator it = modulatorToSet.begin(); it != modulatorToSet.end(); ++it) {
      result[it->first] = dss::BestFit(*(it->first), it->second); 
    }
    
    return result;
  }
  
  vector<int> DS485Proxy::SendCommand(DS485Command _cmd, Set& _set) {
    vector<int> result;
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
    return result;
  } // SendCommand
  
  vector<int> DS485Proxy::SendCommand(DS485Command _cmd, const Modulator& _modulator, Group& _group) {
    return vector<int>();
  } // SendCommand
 
  vector<int> DS485Proxy::SendCommand(DS485Command _cmd, Device& _device) {
    return SendCommand(_cmd, _device.GetShortAddress(), _device.GetModulatorID());
  } // SendCommand
  
  vector<int> DS485Proxy::SendCommand(DS485Command _cmd, devid_t _id, uint8 _modulatorID) {
    vector<int> result;
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
    } else if(_cmd == cmdGetOnOff) {
      frame.GetPayload().Add<uint8>(FunctionDeviceGetOnOff);
      frame.GetPayload().Add<uint8>(_id);
      SendFrame(frame);
      uint8 res = ReceiveSingleResult(FunctionDeviceGetOnOff);
      result.push_back(res);
    }
    return result;
  } // SendCommand
  
  void DS485Proxy::SendFrame(DS485CommandFrame& _frame) {
    bool broadcast = _frame.GetHeader().IsBroadcast();
    bool sim = IsSimAddress(_frame.GetHeader().GetDestination());
    if(broadcast || sim) {
      DSS::GetInstance()->GetModulatorSim().Send(_frame);
    }
    if(broadcast || !sim) {
      if(m_DS485Controller.GetState() == csSlave || m_DS485Controller.GetState() == csMaster) {
        m_DS485Controller.EnqueueFrame(&_frame);
      }
    }
  }

  bool DS485Proxy::IsSimAddress(const uint8 _addr) {
    return DSS::GetInstance()->GetModulatorSim().GetID() == _addr;
  } // IsSimAddress
  
  vector<int> DS485Proxy::GetModulators() {
    DS485CommandFrame* cmdFrame = new DS485CommandFrame();
    cmdFrame->GetHeader().SetDestination(0);
    cmdFrame->GetHeader().SetBroadcast(true);
    cmdFrame->SetCommand(CommandRequest);
    cmdFrame->GetPayload().Add<uint8>(FunctionGetTypeRequest);
    SendFrame(*cmdFrame);

    vector<int> result;

    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionGetTypeRequest);
    for(vector<boost::shared_ptr<DS485CommandFrame> >::iterator iFrame = results.begin(), e = results.end();
        iFrame != e; ++iFrame)
    {
      PayloadDissector pd((*iFrame)->GetPayload());
      pd.Get<uint8>();
      result.push_back((*iFrame)->GetHeader().GetSource());
    }
    
    return result;
  } // GetModulators
  
  int DS485Proxy::GetGroupCount(const int _modulatorID, const int _roomID) {  
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8>(FunctionModulatorGetGroupsSize);
    SendFrame(cmdFrame);
    cmdFrame.GetPayload().Add<uint8>(_roomID);
    uint8 res = ReceiveSingleResult(FunctionModulatorGetGroupsSize);
    return res;    
  } // GetGroupCount
  
  vector<int> DS485Proxy::GetGroups(const int _modulatorID, const int _roomID) {
    vector<int> result;

    int numGroups = GetGroupCount(_modulatorID, _roomID);
    for(int iGroup = 0; iGroup < numGroups; iGroup++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.GetHeader().SetDestination(_modulatorID);
      cmdFrame.SetCommand(CommandRequest);
      cmdFrame.GetPayload().Add<uint8>(FunctionRoomGetGroupIdForInd);
      cmdFrame.GetPayload().Add<uint8>(_roomID);
      cmdFrame.GetPayload().Add<uint8>(iGroup);
      SendFrame(cmdFrame);
      uint8 res = ReceiveSingleResult(FunctionRoomGetGroupIdForInd);
      result.push_back(res);
    }
    
    return result;
  } // GetGroups

  int DS485Proxy::GetDevicesInGroupCount(const int _modulatorID, const int _roomID, const int _groupID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8>(FunctionGroupGetDeviceCount);
    cmdFrame.GetPayload().Add<uint8>(_roomID);
    cmdFrame.GetPayload().Add<uint8>(_groupID);
    SendFrame(cmdFrame);
    uint8 res = ReceiveSingleResult(FunctionGroupGetDeviceCount);
    return res;
  } // GetDevicesInGroupCount

  vector<int> DS485Proxy::GetDevicesInGroup(const int _modulatorID, const int _roomID, const int _groupID) {
    vector<int> result;

    int numDevices = GetDevicesInGroupCount(_modulatorID, _roomID, _groupID);
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.GetHeader().SetDestination(_modulatorID);
      cmdFrame.SetCommand(CommandRequest);
      cmdFrame.GetPayload().Add<uint8>(FunctionGroupGetDevKeyForInd);
      cmdFrame.GetPayload().Add<uint8>(_roomID);
      cmdFrame.GetPayload().Add<uint8>(_groupID);
      cmdFrame.GetPayload().Add<uint8>(iDevice);
      SendFrame(cmdFrame);
      uint8 res = ReceiveSingleResult(FunctionGroupGetDevKeyForInd);
      result.push_back(res);
    }
    
    return result;
  } // GetDevicesInGroup
  
  vector<int> DS485Proxy::GetRooms(const int _modulatorID) {
    vector<int> result;
    
    int numGroups = GetRoomCount(_modulatorID);
    for(int iGroup = 0; iGroup < numGroups; iGroup++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.GetHeader().SetDestination(_modulatorID);
      cmdFrame.SetCommand(CommandRequest);
      cmdFrame.GetPayload().Add<uint8>(FunctionModulatorGetRoomIdForInd);
      cmdFrame.GetPayload().Add<uint8>(iGroup);
      SendFrame(cmdFrame);
      result.push_back(ReceiveSingleResult(FunctionModulatorGetRoomIdForInd));
    }
    return result;
  } // GetRooms
  
  int DS485Proxy::GetRoomCount(const int _modulatorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8>(FunctionModulatorGetRoomsSize);
    SendFrame(cmdFrame);
    return ReceiveSingleResult(FunctionModulatorGetRoomsSize);
  } // GetRoomCount
  
  int DS485Proxy::GetDevicesCountInRoom(const int _modulatorID, const int _roomID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8>(FunctionModulatorCountDevInRoom);
    cmdFrame.GetPayload().Add<uint8>(_roomID);
    SendFrame(cmdFrame);
    return ReceiveSingleResult(FunctionModulatorCountDevInRoom);
  } // GetDevicesCountInRoom
  
  vector<int> DS485Proxy::GetDevicesInRoom(const int _modulatorID, const int _roomID) {
    vector<int> result;
    
    int numDevices = GetDevicesCountInRoom(_modulatorID, _roomID);
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.GetHeader().SetDestination(_modulatorID);
      cmdFrame.SetCommand(CommandRequest);
      cmdFrame.GetPayload().Add<uint8>(FunctionModulatorDevKeyInRoom);
      cmdFrame.GetPayload().Add<uint8>(_roomID);
      cmdFrame.GetPayload().Add<uint8>(iDevice);
      SendFrame(cmdFrame);
      result.push_back(ReceiveSingleResult(FunctionModulatorDevKeyInRoom));
    }
    return result;    
  } // GetDevicesInRoom
  
  dsid_t DS485Proxy::GetDSIDOfDevice(const int _modulatorID, const int _deviceID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8>(FunctionDeviceGetDSID);
    cmdFrame.GetPayload().Add<uint8>(_deviceID);
    SendFrame(cmdFrame);
    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionDeviceGetDSID);
    if(results.size() != 1) {
      Logger::GetInstance()->Log("DS485Proxy::GetDSIDOfDevice: received multiple results", lsError);
      return 0;
    }
    PayloadDissector pd(results.at(0)->GetPayload());
    pd.Get<uint8>(); // discard the function id
    return pd.Get<dsid_t>();
  }
  
  dsid_t DS485Proxy::GetDSIDOfModulator(const int _modulatorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8>(FunctionModulatorGetDSID);
    SendFrame(cmdFrame);
    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionModulatorGetDSID);
    if(results.size() != 1) {
      Logger::GetInstance()->Log("DS485Proxy::GetDSIDOfModulator: received multiple results", lsError);
      return 0;
    }
    PayloadDissector pd(results.at(0)->GetPayload());
    pd.Get<uint8>(); // discard the function id
    return pd.Get<dsid_t>();
  }
  
  vector<boost::shared_ptr<DS485CommandFrame> > DS485Proxy::Receive(uint8 _functionID) {
    vector<boost::shared_ptr<DS485CommandFrame> > result;

    if(m_DS485Controller.GetState() == csSlave || m_DS485Controller.GetState() == csMaster) {
      // Wait for two tokens
      m_DS485Controller.WaitForToken();
      m_DS485Controller.WaitForToken();
    } else {
      SleepMS(100);
    }
    
    FramesByID::iterator iRecvFrame = m_ReceivedFramesByFunctionID.find(_functionID);
    if(iRecvFrame != m_ReceivedFramesByFunctionID.end()) {
      vector<ReceivedFrame*>& frames = iRecvFrame->second;
      for(vector<ReceivedFrame*>::iterator iFrame = frames.begin(), e = frames.end(); iFrame != e; /* nop */) {
        ReceivedFrame* frame = *iFrame;
        frames.erase(iFrame++);
        
        result.push_back(frame->GetFrame());
        delete frame;
      }
    }
    
    return result;
  } // Receive  
  
  uint8 DS485Proxy::ReceiveSingleResult(uint8 _functionID) {
    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(_functionID);
    
    if(results.size() > 1 || results.size() < 1) {
      throw runtime_error("received multiple or none results for request");
    }
    
    DS485CommandFrame* frame = results.at(0).get();
    
    PayloadDissector pd(frame->GetPayload());
    uint8 functionID = pd.Get<uint8>();
    if(functionID != _functionID) {
      Logger::GetInstance()->Log("function ids are different");
    }
    uint8 result = pd.Get<uint8>();
    
    results.clear();
    
    return result;    
  } // ReceiveSingleResult

  void DS485Proxy::Start() {
    m_DS485Controller.AddFrameCollector(this);
    DSS::GetInstance()->GetModulatorSim().AddFrameCollector(this);
    m_DS485Controller.Run();
    Run();
  } // Start
  
  void DS485Proxy::WaitForProxyEvent() {
    m_ProxyEvent.WaitFor();
  } // WaitForProxyEvent
  
  void DS485Proxy::SignalEvent() {
    m_ProxyEvent.Signal();
  } // SignalEvent
  
  void DS485Proxy::Execute() {
    SleepSeconds(1);
    SignalEvent();
    
    aControllerState oldState = m_DS485Controller.GetState();
    while(!m_Terminated) {
      if(m_DS485Controller.GetState() != csSlave || m_DS485Controller.GetState() != csMaster) {
        if(m_DS485Controller.WaitForEvent(50)) {
          aControllerState newState = m_DS485Controller.GetState();
          if(newState != oldState) {
            if(newState == csSlave) {
              SignalEvent();
            } else if(newState == csMaster) {
              SignalEvent();
            }
          }
        }

        if(!m_IncomingFrames.empty() || m_PacketHere.WaitFor(50)) {
          while(!m_IncomingFrames.empty()) { 
            // process packets and put them into a functionID-hash            
            boost::shared_ptr<DS485CommandFrame> frame = m_IncomingFrames.front();
            m_IncomingFrames.erase(m_IncomingFrames.begin());
            
            // create an explicit copy since frame is a shared ptr and would free the contained
            // frame if going out of scope
            DS485CommandFrame* pFrame = new DS485CommandFrame();
            *pFrame = *frame.get();
            
            vector<unsigned char> ch = pFrame->GetPayload().ToChar();
            if(ch.size() < 1) {
              Logger::GetInstance()->Log("Received Command Frame w/o function identifier");
              delete pFrame;
              continue;
            }
            
            uint8 functionID = ch.front();
            ReceivedFrame* rf = new ReceivedFrame(m_DS485Controller.GetTokenCount(), pFrame);
            m_ReceivedFramesByFunctionID[functionID].push_back(rf);
          }
        }
      }
    }
  } // Execute
  
  void DS485Proxy::CollectFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
    uint8 commandID = _frame->GetCommand();
    if(commandID != CommandResponse) {
      Logger::GetInstance()->Log("discarded non response frame", lsInfo);
    } else {
      m_IncomingFrames.push_back(_frame);
      m_PacketHere.Signal();
    }
  }

  //================================================== ReceivedPacket
  
  ReceivedFrame::ReceivedFrame(const int _receivedAt, DS485CommandFrame* _frame)
  : m_ReceivedAtToken(_receivedAt),
    m_Frame(_frame)
  {  
  } // ctor
   
}
