/*
 *  ds485proxy.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/18/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "ds485proxy.h"

#include "../core/model.h"
#include "../core/dss.h"
#include "../core/logger.h"
#include "../core/ds485const.h"

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
  : Thread("DS485Proxy")
  { } // ctor

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
      frame.GetPayload().Add<devid_t>(_id);
      frame.GetPayload().Add<uint8>(Scene1);
      SendFrame(frame);
    } else if(_cmd == cmdTurnOff) {
      frame.GetPayload().Add<uint8>(FunctionDeviceCallScene);
      frame.GetPayload().Add<devid_t>(_id);
      frame.GetPayload().Add<uint8>(SceneOff);
      SendFrame(frame);
    } else if(_cmd == cmdGetOnOff) {
      frame.GetPayload().Add<uint8>(FunctionDeviceGetOnOff);
      frame.GetPayload().Add<uint8>(_id);
      SendFrame(frame);
      uint8 res = ReceiveSingleResult(FunctionDeviceGetOnOff);
      result.push_back(res);
    } else if(_cmd == cmdGetValue) {
      frame.GetPayload().Add<uint8>(FunctionDeviceGetParameterValue);
      frame.GetPayload().Add<uint8>(_id);
      SendFrame(frame);
      uint8 res = ReceiveSingleResult(FunctionDeviceGetParameterValue);
      result.push_back(res);
    } else if(_cmd == cmdGetFunctionID) {
      frame.GetPayload().Add<uint8>(FunctionDeviceGetFunctionID);
      frame.GetPayload().Add<devid_t>(_id);
      SendFrame(frame);
      uint8 res = ReceiveSingleResult(FunctionDeviceGetFunctionID);
      result.push_back(res);
    }
    return result;
  } // SendCommand

  void DS485Proxy::SendFrame(DS485CommandFrame& _frame, bool _force) {
    bool broadcast = _frame.GetHeader().IsBroadcast();
    bool sim = IsSimAddress(_frame.GetHeader().GetDestination());
    if(broadcast || sim) {
      cout << "sim" << endl;
      DSS::GetInstance()->GetModulatorSim().Process(_frame);
    }
    if(broadcast || !sim) {
      if(m_DS485Controller.GetState() == csSlave || m_DS485Controller.GetState() == csMaster) {
        cout << "hw" << endl;
        // Note: At the moment we're discarding all but the "forced" frames. This is because the
        // firmware in the dSM returns gibberish in it's current state.
        if(_force) {
        	m_DS485Controller.EnqueueFrame(_frame);
        }
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
    Logger::GetInstance()->Log("Proxy: GetModulators");
    SendFrame(*cmdFrame);

    vector<int> result;

    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionGetTypeRequest);
    for(vector<boost::shared_ptr<DS485CommandFrame> >::iterator iFrame = results.begin(), e = results.end();
        iFrame != e; ++iFrame)
    {
      PayloadDissector pd((*iFrame)->GetPayload());
      pd.Get<uint8>();
      uint16_t devID = pd.Get<uint16_t>();
      devID &= 0x00FF;
      if(devID == 0) {
        cout << "Found dSS" << endl;
      } else if(devID == 1) {
        cout << "Found dSM" << endl;
      } else {
        cout << "Found unknown device" << endl;
      }
      /*uint16_t hwVersion =*/ pd.Get<uint16_t>();
      /*uint16_t swVersion =*/ pd.Get<uint16_t>();

      cout << "Name: \"";
      for(int i = 0; i < 6; i++) {
        char c = static_cast<char>(pd.Get<uint8>());
        if(c != '\0') {
          cout << c;
        }
      }
      cout << "\"" << endl;

      result.push_back((*iFrame)->GetHeader().GetSource());
    }

    return result;
  } // GetModulators

  int DS485Proxy::GetGroupCount(const int _modulatorID, const int _roomID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8>(FunctionModulatorGetGroupsSize);
    cmdFrame.GetPayload().Add<uint8>(_roomID);
    SendFrame(cmdFrame);
    uint8 res = ReceiveSingleResult(FunctionModulatorGetGroupsSize);
    return res;
  } // GetGroupCount

  vector<int> DS485Proxy::GetGroups(const int _modulatorID, const int _roomID) {
    vector<int> result;

    int numGroups = GetGroupCount(_modulatorID, _roomID);
    Logger::GetInstance()->Log(string("Modulator has ") + IntToString(numGroups) + " groups");
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

    int numRooms = GetRoomCount(_modulatorID);
    Logger::GetInstance()->Log(string("Proxy: Modulator has ") + IntToString(numRooms) + " rooms");
    for(int iGroup = 0; iGroup < numRooms; iGroup++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.GetHeader().SetDestination(_modulatorID);
      cmdFrame.SetCommand(CommandRequest);
      cmdFrame.GetPayload().Add<uint8>(FunctionModulatorGetRoomIdForInd);
      cmdFrame.GetPayload().Add<uint8>(iGroup);
      Logger::GetInstance()->Log("Proxy: GetRoomID");
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
    Logger::GetInstance()->Log("Proxy: GetRoomCount");
    SendFrame(cmdFrame);

    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionModulatorGetRoomsSize);
    cout << results.size();
    DS485CommandFrame* frame = results.at(0).get();

    PayloadDissector pd(frame->GetPayload());
    uint8 functionID = pd.Get<uint8>();
    if(functionID != FunctionModulatorGetRoomsSize) {
      Logger::GetInstance()->Log("function ids are different");
    }
    uint8 result = pd.Get<uint8>();

    if(!pd.IsEmpty()) {
      cout << "haven't used all of the packet" << endl;
      while(!pd.IsEmpty()) {
        printf("%x\n", result);
        result = pd.Get<uint8>();
      }
      printf("%x\n", result);
      result = 1;
    }

    return result;
  } // GetRoomCount

  int DS485Proxy::GetDevicesCountInRoom(const int _modulatorID, const int _roomID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8>(FunctionModulatorCountDevInRoom);
    cmdFrame.GetPayload().Add<uint8>(_roomID);
    Logger::GetInstance()->Log("Proxy: GetDevicesCountInRoom");
    SendFrame(cmdFrame);

    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionModulatorCountDevInRoom);
    cout << results.size();
    DS485CommandFrame* frame = results.at(0).get();

    PayloadDissector pd(frame->GetPayload());
    uint8_t functionID = pd.Get<uint8_t>();
    if(functionID != FunctionModulatorCountDevInRoom) {
      Logger::GetInstance()->Log("function ids are different");
    }
    uint16_t result = pd.Get<uint16_t>();
/*
    if(!pd.IsEmpty()) {
      cout << "haven't used all of the packet" << endl;
      while(!pd.IsEmpty()) {
        printf("%x\n", result);
        result = pd.Get<uint16_t>();
      }
      printf("%x\n", result);
      result = 1;
    }
*/
    return result;
  } // GetDevicesCountInRoom

  vector<int> DS485Proxy::GetDevicesInRoom(const int _modulatorID, const int _roomID) {
    vector<int> result;

    int numDevices = GetDevicesCountInRoom(_modulatorID, _roomID);
    Logger::GetInstance()->Log(string("Proxy: Found ") + IntToString(numDevices) + " in room.");
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.GetHeader().SetDestination(_modulatorID);
      cmdFrame.SetCommand(CommandRequest);
      cmdFrame.GetPayload().Add<uint8>(FunctionModulatorDevKeyInRoom);
      cmdFrame.GetPayload().Add<uint8>(_roomID);
      cmdFrame.GetPayload().Add<devid_t>(iDevice);
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
    Logger::GetInstance()->Log("Proxy: GetDSIDOfDevice");

    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionDeviceGetDSID);
    if(results.size() != 1) {
      Logger::GetInstance()->Log(string("DS485Proxy::GetDSIDOfDevice: received multiple results") + IntToString(results.size()), lsError);
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
    Logger::GetInstance()->Log(string("Proxy: GetDSIDOfModulator ") + IntToString(_modulatorID));
    SendFrame(cmdFrame);

    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionModulatorGetDSID);
    if(results.size() != 1) {
      Logger::GetInstance()->Log(string("DS485Proxy::GetDSIDOfModulator: received multiple results ") + IntToString(results.size()), lsError);
      return 0;
    }
    PayloadDissector pd(results.at(0)->GetPayload());
    pd.Get<uint8>(); // discard the function id
    return pd.Get<dsid_t>();
  } // GetDSIDOfModulator

  void DS485Proxy::Subscribe(const int _modulatorID, const int _groupID, const int _deviceID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8>(FunctionDeviceSubscribe);
    cmdFrame.GetPayload().Add<uint8>(_groupID);
    cmdFrame.GetPayload().Add<devid_t>(_deviceID);
    Logger::GetInstance()->Log(string("Proxy: Subscribe ") + IntToString(_modulatorID));
    SendFrame(cmdFrame);
  } // Subscribe

  void DS485Proxy::AddToGroup(const int _modulatorID, const int _groupID, const int _deviceID) {

  } // AddToGroup

  void DS485Proxy::RemoveFromGroup(const int _modulatorID, const int _groupID, const int _deviceID) {

  } // RemoveFromGroup

  int DS485Proxy::AddUserGroup(const int _modulatorID) {
    return 0;
  } // AddUserGroup

  void DS485Proxy::RemoveUserGroup(const int _modulatorID, const int _groupID) {

  } // RemoveUserGroup


  vector<boost::shared_ptr<DS485CommandFrame> > DS485Proxy::Receive(uint8 _functionID) {
    vector<boost::shared_ptr<DS485CommandFrame> > result;

    if(m_DS485Controller.GetState() == csSlave || m_DS485Controller.GetState() == csMaster) {
      // Wait for two tokens
      m_DS485Controller.WaitForToken();
      m_DS485Controller.WaitForToken();
      m_DS485Controller.WaitForToken();
      m_DS485Controller.WaitForToken();
    } else {
      SleepMS(100);
    }

    FramesByID::iterator iRecvFrame = m_ReceivedFramesByFunctionID.find(_functionID);
    if(iRecvFrame != m_ReceivedFramesByFunctionID.end()) {
      vector<ReceivedFrame*>& frames = iRecvFrame->second;
      while(!frames.empty()) {
        ReceivedFrame* frame = *frames.begin();
        frames.erase(frames.begin());

        result.push_back(frame->GetFrame());
        delete frame;
      }
    }

    return result;
  } // Receive

  uint8 DS485Proxy::ReceiveSingleResult(uint8 _functionID) {
    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(_functionID);

    if(results.size() > 1 || results.size() < 1) {
      cout << results.size() << endl;
      Logger::GetInstance()->Log("received multiple or none results for request");
      return 0;
      //throw runtime_error("received multiple or none results for request");
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
    try {
      m_DS485Controller.Run();
    } catch (const runtime_error& _ex) {
    	Logger::GetInstance()->Log(string("Caught exception while starting DS485Controlle: ") + _ex.what());
    }
    Run();
  } // Start

  void DS485Proxy::WaitForProxyEvent() {
    m_ProxyEvent.WaitFor();
  } // WaitForProxyEvent

  void DS485Proxy::SignalEvent() {
    m_ProxyEvent.Signal();
  } // SignalEvent

  const char* FunctionIDToString(const int _functionID) {
    switch(_functionID) {
    case  FunctionModulatorAddRoom:
      return "Modulator Add Room";
    case  FunctionModulatorRemoveRoom:
      return "Modulator Remove Room";
    case  FunctionModulatorRemoveAllRooms:
      return "Modulator Remove All Rooms";
    case  FunctionModulatorCountDevInRoom:
      return "Modulator Count Dev In Room";
    case  FunctionModulatorDevKeyInRoom:
      return "Modulator Dev Key In Room";
    case  FunctionModulatorGetGroupsSize:
      return "Modulator Get Groups Size";
    case  FunctionModulatorGetRoomsSize:
      return "Modulator Get Rooms Size";
    case  FunctionModulatorGetRoomIdForInd:
      return "Modulator Get Room Id For Index";
    case  FunctionModulatorAddToGroup:
      return "Modulator Add To Group";
    case  FunctionModulatorRemoveFromGroup:
      return "Modulator Remove From Group";
    case  FunctionGroupAddDeviceToGroup:
      return "Group Add Device";
    case  FunctionGroupRemoveDeviceFromGroup:
      return "Group Remove Device";
    case  FunctionGroupGetDeviceCount:
      return "Group Get Device Count";
    case  FunctionGroupGetDevKeyForInd:
      return "Group Get Dev Key For Index";

    case  FunctionRoomGetGroupIdForInd:
      return "Room Get Group ID For Index";

    case  FunctionDeviceCallScene:
      return "Device Call Scene";
    case  FunctionDeviceIncValue:
      return "Device Inc Value";
    case  FunctionDeviceDecValue:
      return "Device Dec Value";

    case  FunctionDeviceGetOnOff:
      return "Function Device Get On Off";
    case  FunctionDeviceGetParameterValue:
      return "Function Device Get Parameter Value";
    case  FunctionDeviceGetDSID:
      return "Function Device Get DSID";

    case FunctionModulatorGetDSID:
      return "Function Modulator Get DSID";

    case FunctionGetTypeRequest:
      return "Function Get Type";

    case FunctionKeyPressed:
      return "Function Key Pressed";

    case FunctionDeviceSubscribe:
      return "Function Device Subscribe";
    }
    return "";
  } // FunctionIDToString

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

            vector<unsigned char> ch = frame->GetPayload().ToChar();
            if(ch.size() < 1) {
              Logger::GetInstance()->Log("Received Command Frame w/o function identifier");
              continue;
            }

            uint8 functionID = ch.front();
            if(frame->GetCommand() == CommandRequest) {
              if(functionID == FunctionKeyPressed) {
                Logger::GetInstance()->Log("Got keypress");
                PayloadDissector pd(frame->GetPayload());
                pd.Get<uint8>();
                uint16_t param = pd.Get<uint16_t>();
                Logger::GetInstance()->Log("Room: " + IntToString((param >> 8) & 0x00FF));
                if(param & 0x0001 == 0x0001) {
                  param = pd.Get<uint16_t>();
                  Logger::GetInstance()->Log("Short version");
                  if((param & 0x8000) == 0x8000) {
                    Logger::GetInstance()->Log("From switch");
                    devid_t shortAddress = (param & 0x7F00) >> 8;
                    Logger::GetInstance()->Log(string("Device: ") + IntToString(shortAddress));
                    int buttonNumber = (param & 0x00F0) >> 4;
                    Logger::GetInstance()->Log(string("Button: ") + IntToString(buttonNumber));
                    ButtonPressKind kind = static_cast<ButtonPressKind>(param & 0x000F);

                    Modulator& mod = DSS::GetInstance()->GetApartment().GetModulatorByBusID(frame->GetHeader().GetSource());
                    DeviceReference devRef = mod.GetDevices().GetByBusID(shortAddress);

                    DSS::GetInstance()->GetApartment().OnKeypress(devRef.GetDSID(), kind, buttonNumber);
                  } else {
                    Logger::GetInstance()->Log("From sensor");
                  }
                } else {
                  Logger::GetInstance()->Log("Long version");
                  uint16_t param2 = pd.Get<uint16_t>();
                  uint16_t param3 = pd.Get<uint16_t>();
                  uint16_t param4 = pd.Get<uint16_t>();

                  cout << hex << "p2: " << param2 << "\np3: " << param3 << "\np4: " << param4 << endl;

                  uint16_t subqualifier = ((param2 & 0x000F) << 4) | ((param3 & 0xF000) >> 12);
                  uint16_t databits = ((param3 & 0x0FF0) >> 4);
                  uint16_t addr = ((param3 & 0x000F) << 6) | ((param4 & 0xFC00) >> 10);
                  uint16_t subqualifier2 = ((param4 & 0x03F0) >> 4);
                  uint16_t mainqualifier = (param4 & 0x000F);
                  cout << "subqualifier: " << subqualifier << "\n"
                       << "databits    : " << databits << "\n"
                       << "addr        : " << addr << "\n"
                       << "subqual2    : " << subqualifier2 << "\n"
                       << "mainqualif  : " << mainqualifier << endl;
                }
              }
            } else {
              // create an explicit copy since frame is a shared ptr and would free the contained
              // frame if going out of scope
              DS485CommandFrame* pFrame = new DS485CommandFrame();
              *pFrame = *frame.get();

              Logger::GetInstance()->Log(string("Response for: ") + FunctionIDToString(functionID));
              ReceivedFrame* rf = new ReceivedFrame(m_DS485Controller.GetTokenCount(), pFrame);
              m_ReceivedFramesByFunctionID[functionID].push_back(rf);
            }
          }
        }
      }
    }
  } // Execute

  void DS485Proxy::CollectFrame(boost::shared_ptr<DS485CommandFrame>& _frame) {
    uint8 commandID = _frame->GetCommand();
    if(commandID != CommandResponse && commandID != CommandRequest) {
      Logger::GetInstance()->Log("discarded non response/request frame", lsInfo);
      Logger::GetInstance()->Log(string("frame type ") + CommandToString(commandID));
    } else {
      m_IncomingFrames.push_back(_frame);
      m_PacketHere.Signal();
    }
  } // CollectFrame

  //================================================== ReceivedPacket

  ReceivedFrame::ReceivedFrame(const int _receivedAt, DS485CommandFrame* _frame)
  : m_ReceivedAtToken(_receivedAt),
    m_Frame(_frame)
  {
  } // ctor

}
