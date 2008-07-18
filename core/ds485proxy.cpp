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

#include <boost/scoped_ptr.hpp>

namespace dss {
  
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
  const uint8 FunctionGroupAddDeviceToGroup = 0x10;
  const uint8 FunctionGroupRemoveDeviceFromGroup = 0x11;
  const uint8 FunctionGroupGetDeviceCount = 0x12;
  const uint8 FunctionGroupGetDevKeyForInd = 0x13;
  
  const uint8 FunctionRoomGetGroupIdForInd = 0x17;
  
  const uint8 FunctionDeviceCallScene = 0x42;
  const uint8 FunctionDeviceIncValue  = 0x40;
  const uint8 FunctionDeviceDecValue  = 0x41;
  
  const uint8 FunctionDeviceGetOnOff = 0x61;
  const uint8 FunctionDeviceGetDSID = 0x65;
  
  const uint8 FunctionModulatorGetDSID = 0xFA;
  
  const uint8 FunctionGetTypeRequest = 0xFD;
  
  const uint8 SceneOff = 0x00;
  const uint8 Scene1 = 0x01;
  const uint8 Scene2 = 0x02;
  const uint8 Scene3 = 0x03;
  const uint8 Scene4 = 0x04;
  const uint8 SceneStandBy = 0x05;
  const uint8 SceneDeepOff = 0x06;
  
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
  
  template<>
  dsid_t PayloadDissector::Get() {
    dsid_t result;
    result = (Get<uint8>() << 24) |
             (Get<uint8>() << 16) |
             (Get<uint8>() << 8)  |
             (Get<uint8>());
    return result;
  }
  
  
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
  
  //================================================== DSModulatorSim
  
  DSModulatorSim::DSModulatorSim() 
  {
    m_ModulatorDSID = SimulationPrefix | 0x0000FFFF;
  } // DSModulatorSim

  
  void DSModulatorSim::Initialize() {
    m_ID = 70;
    m_SimulatedDevices.push_back(new DSIDSim(SimulationPrefix | 0x01));
    m_SimulatedDevices.push_back(new DSIDSim(SimulationPrefix | 0x02));
    m_SimulatedDevices[0]->SetShortAddress(1);
    m_SimulatedDevices[1]->SetShortAddress(2);
    m_Rooms[1] = vector<DSIDSim*>();
    m_Rooms[1].push_back(m_SimulatedDevices[0]);
    m_Rooms[1].push_back(m_SimulatedDevices[1]);
  }
  
  const uint8 HeaderTypeToken = 0;
  const uint8 HeaderTypeCommand = 1;
  
  void DSModulatorSim::Send(DS485Frame& _frame) {
    DS485Header& header = _frame.GetHeader();
    if(header.GetType() == HeaderTypeToken) {
      // Transmit pending things
    } else if(header.GetType() == HeaderTypeCommand) {
      DS485CommandFrame& cmdFrame = dynamic_cast<DS485CommandFrame&>(_frame);
      PayloadDissector pd(cmdFrame.GetPayload());
      if(cmdFrame.GetCommand() == CommandRequest) {
        int cmdNr = pd.Get<uint8>();
        DS485CommandFrame* response;
        switch(cmdNr) {
          case FunctionDeviceCallScene:
            {
              int devID = pd.Get<uint8>();
              int sceneID = pd.Get<uint8>();
              LookupDevice(devID).CallScene(sceneID);
              DistributeFrame(boost::shared_ptr<DS485CommandFrame>(CreateAck(cmdFrame, cmdNr))); 
            }
            break;
          case FunctionModulatorGetRoomsSize:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              response->GetPayload().Add<uint8>(m_Rooms.size());
              DistributeFrame(response);
            }
            break;
          case FunctionModulatorGetRoomIdForInd:
            {
              uint8 index = pd.Get<uint8>();
              map< const int, vector<DSIDSim*> >::iterator it = m_Rooms.begin();
              advance(it, index);
              response = CreateResponse(cmdFrame, cmdNr);
              response->GetPayload().Add<uint8>(it->first);   
              DistributeFrame(response);
            }
            break;
          case FunctionModulatorCountDevInRoom:
            {
              uint8 index = pd.Get<uint8>();
              response = CreateResponse(cmdFrame, cmdNr);
              response->GetPayload().Add<uint8>(m_Rooms[index].size());
              DistributeFrame(response);
            }
            break;
          case FunctionModulatorDevKeyInRoom:
            {
              uint8 roomID = pd.Get<uint8>();
              uint8 deviceIndex = pd.Get<uint8>();
              response = CreateResponse(cmdFrame, cmdNr);
              response->GetPayload().Add<uint8>(m_Rooms[roomID].at(deviceIndex)->GetShortAddress());
              DistributeFrame(response);
            }
            break;
          case FunctionModulatorGetGroupsSize:
    		    {
    		      response = CreateResponse(cmdFrame, cmdNr);
    		      response->GetPayload().Add<uint8>(m_DevicesOfGroupInRoom.size());
    		      DistributeFrame(response); 
    		    }
            break;
          case FunctionModulatorGetDSID:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              response->GetPayload().Add<dsid_t>(m_ModulatorDSID);
              DistributeFrame(response);
            }
            break;
          case FunctionGroupGetDeviceCount:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              int roomID = pd.Get<uint8>();
              int groupID = pd.Get<uint8>();
              int result = m_DevicesOfGroupInRoom[pair<const int, const int>(roomID, groupID)].size();
              response->GetPayload().Add<uint8>(result);
              DistributeFrame(response);
            }
            break;
          case FunctionGroupGetDevKeyForInd:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              int roomID = pd.Get<uint8>();
              int groupID = pd.Get<uint8>();
              int index = pd.Get<uint8>();
              int result = m_DevicesOfGroupInRoom[pair<const int, const int>(roomID, groupID)].at(index)->GetShortAddress();
              response->GetPayload().Add<uint8>(result);
              DistributeFrame(response);
            }
            break;
          case FunctionRoomGetGroupIdForInd:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              int roomID = pd.Get<uint8>();
              int groupIndex= pd.Get<uint8>();
              
              IntPairToDSIDSimVector::iterator it = m_DevicesOfGroupInRoom.begin();
              IntPairToDSIDSimVector::iterator end = m_DevicesOfGroupInRoom.end();
              int result = -1;
              while(it != end) {
                if(it->first.first == roomID) { 
                  groupIndex--;
                  if(groupIndex == 0) {
                    result = it->first.second; // yes, that's the group id
                    break;
                  }
                }
                it++;
              }
              response->GetPayload().Add<uint8>(result);
              DistributeFrame(response);
            }
            break;
          case FunctionDeviceGetOnOff:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              int devID = pd.Get<uint8>();
              DSIDSim& dev = LookupDevice(devID);
              response->GetPayload().Add<uint8>(dev.IsTurnedOn());
              DistributeFrame(response);
            }
            break;
          case FunctionDeviceGetDSID:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              int devID = pd.Get<uint8>();
              DSIDSim& dev = LookupDevice(devID);
              response->GetPayload().Add<dsid_t>(dev.GetDSID());
              DistributeFrame(response);
            }
            break;
          case FunctionGetTypeRequest:
            {
              response = CreateResponse(cmdFrame, cmdNr);
              // Type 2 bytes (high, low)
              response->GetPayload().Add<uint8>(0x00);
              response->GetPayload().Add<uint8>(0x01);
              // HW-Version 2 bytes (high, low)
              response->GetPayload().Add<uint8>(0x00);
              response->GetPayload().Add<uint8>(0x01);
              // SW-Version 2 bytes (high, low)
              // HW-Version 2 bytes (high, low)
              response->GetPayload().Add<uint8>(0x00);
              response->GetPayload().Add<uint8>(0x01);
              // free text
              response->GetPayload().Add<uint8>('d');
              response->GetPayload().Add<uint8>('S');
              response->GetPayload().Add<uint8>('M');
              response->GetPayload().Add<uint8>(' ');
              response->GetPayload().Add<uint8>('S');
              response->GetPayload().Add<uint8>('i');
              response->GetPayload().Add<uint8>('m');
              response->GetPayload().Add<uint8>(0x00);
              DistributeFrame(response);
            }
            break;
          default:
            throw new runtime_error(string("DSModulatorSim: Invalid function id: ") + IntToString(cmdNr));
        }
      }
    }
  } // Send

  
  DS485CommandFrame* DSModulatorSim::CreateReply(DS485CommandFrame& _request) {
    DS485CommandFrame* result = new DS485CommandFrame();
    result->GetHeader().SetDestination(_request.GetHeader().GetSource());
    result->GetHeader().SetSource(m_ID);
    result->GetHeader().SetBroadcast(false);
    result->GetHeader().SetCounter(_request.GetHeader().GetCounter());
    return result;
  } // CreateReply

  
  DS485CommandFrame* DSModulatorSim::CreateAck(DS485CommandFrame& _request, uint8 _functionID) {
    DS485CommandFrame* result = CreateReply(_request);
    result->SetCommand(CommandAck);
    result->GetPayload().Add(_functionID);
    return result;
  }

  DS485CommandFrame* DSModulatorSim::CreateResponse(DS485CommandFrame& _request, uint8 _functionID) {
    DS485CommandFrame* result = CreateReply(_request);
    result->SetCommand(CommandResponse);
    result->GetPayload().Add(_functionID);
    return result;
  } // CreateResponse

  
  DSIDSim& DSModulatorSim::LookupDevice(const devid_t _shortAddress) {
    for(vector<DSIDSim*>::iterator ipSimDev = m_SimulatedDevices.begin(); ipSimDev != m_SimulatedDevices.end(); ++ipSimDev) {
      if((*ipSimDev)->GetShortAddress() == _shortAddress)  {
        return **ipSimDev;
      }
    }
    throw new runtime_error("could not find device");

  } // LookupDevice
  
  int DSModulatorSim::GetID() const {
    return m_ID;
  } // GetID

  //================================================== DSIDSim
  
  DSIDSim::DSIDSim(const dsid_t _dsid) 
  : m_DSID(_dsid),
    m_ShortAddress(0xFF),
    m_Enabled(true),
    m_CurrentValue(0)
  {
    m_ValuesForScene.push_back(0);   // OFF
    m_ValuesForScene.push_back(255); // Scene1
    m_ValuesForScene.push_back(255); // Scene2
    m_ValuesForScene.push_back(255); // Scene3
    m_ValuesForScene.push_back(255); // Scene4
    m_ValuesForScene.push_back(0);   // SceneStandby
    m_ValuesForScene.push_back(0);   // SceneDeepOff
  } // ctor
  
  dsid_t DSIDSim::GetDSID() const {
    return m_DSID;
  } // GetDSID
  
  devid_t DSIDSim::GetShortAddress() const {
    return m_ShortAddress;
  } // GetShortAddress
  
  void DSIDSim::SetShortAddress(const devid_t _shortAddress) {
    m_ShortAddress = _shortAddress;
  }
  
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
      m_DimmingUp = _directionUp;
      m_Dimming = true;
      time(&m_DimmStartTime);
    }
  } // StartDim
  
  void DSIDSim::EndDim(const int _parameterNr) {
    if(m_Enabled) {
      time_t now;
      time(&now);
      if(m_DimmingUp) {
        m_CurrentValue = static_cast<int>(max(m_CurrentValue + difftime(m_DimmStartTime, now) * 5, 255.0));
      } else {
        m_CurrentValue = static_cast<int>(min(m_CurrentValue - difftime(m_DimmStartTime, now) * 5, 255.0));
      }
    }
  } // EndDim
  
  void DSIDSim::SetValue(const double _value, int _parameterNr) {
    if(m_Enabled) {
      m_CurrentValue = static_cast<int>(_value);
    }
  } // SetValue
  
  bool DSIDSim::IsTurnedOn() const {
    return m_CurrentValue > 0;
  } // IsTurnedOn
  
  //================================================== ReceivedPacket
  
  ReceivedFrame::ReceivedFrame(const int _receivedAt, DS485CommandFrame* _frame)
  : m_ReceivedAtToken(_receivedAt),
    m_Frame(_frame)
  {  
  } // ctor
   
}
