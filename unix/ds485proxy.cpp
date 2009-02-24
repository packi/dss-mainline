/*
 *  ds485proxy.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/18/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "ds485proxy.h"

#include "core/model.h"
#include "core/dss.h"
#include "core/logger.h"
#include "core/ds485const.h"
#include "core/sim/dssim.h"

#include <sstream>

#include <boost/scoped_ptr.hpp>

namespace dss {

  const char* FunctionIDToString(const int _functionID); // internal forward declaration

  typedef hash_map<const Modulator*, Set> HashMapModulatorSet;

  HashMapModulatorSet SplitByModulator(const Set& _set) {
    HashMapModulatorSet result;
    for(int iDevice = 0; iDevice < _set.Length(); iDevice++) {
      const DeviceReference& dev = _set.Get(iDevice);
      Modulator& mod = dev.GetDevice().GetApartment().GetModulatorByBusID(dev.GetDevice().GetModulatorID());
      result[&mod].AddDevice(dev);
    }
    return result;
  } // SplitByModulator


  typedef map<const Zone*, Set> HashMapZoneSet;

  HashMapZoneSet SplitByZone(const Set& _set) {
    HashMapZoneSet result;
    for(int iDevice = 0; iDevice < _set.Length(); iDevice++) {
      const DeviceReference& devRef = _set.Get(iDevice);
      const Device& dev = devRef.GetDevice();
      Zone& zone = dev.GetApartment().GetZone(dev.GetZoneID());
      result[&zone].AddDevice(dev);
    }
    return result;
  } // SplitByZone

  typedef pair<vector<Group*>, Set> FittingResultPerModulator;

  /** Precondition: _set contains only devices of _zone */
  FittingResultPerModulator BestFit(const Zone& _zone, const Set& _set) {
    Set workingCopy = _set;

    vector<Group*> fittingGroups;
    Set singleDevices;

	if(_zone.GetDevices().Length() == _set.Length()) {
	  // TODO: use the group of the zone...
	  fittingGroups.push_back(&DSS::GetInstance()->GetApartment().GetGroup(GroupIDBroadcast));
	  Logger::GetInstance()->Log(string("Optimization: Set contains all devices of zone ") + IntToString(_zone.GetZoneID()));
	} else {
	    vector<Group*> unsuitableGroups;
	    Set workingCopy = _set;

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
			Set devicesInGroup = _zone.GetDevices().GetByGroup(g);
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
	}
    return FittingResultPerModulator(fittingGroups, singleDevices);
  }

  FittingResultPerModulator BestFit(const Modulator& _modulator, const Set& _set) {
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

  DS485Proxy::DS485Proxy(DSS* _pDSS)
  : Thread("DS485Proxy"),
    Subsystem(_pDSS, "DS485Proxy")
  { } // ctor

  bool DS485Proxy::IsReady() {
	  return IsRunning()
	         && ((m_DS485Controller.GetState() == csSlave) ||
	             (m_DS485Controller.GetState() == csMaster) ||
	             (m_DS485Controller.GetState() == csError)) // allow the simulation to run on it's own
	         &&  DSS::GetInstance()->GetModulatorSim().Ready();
  } // IsReady

  FittingResult DS485Proxy::BestFit(const Set& _set) {
    FittingResult result;
    HashMapZoneSet zoneToSet = SplitByZone(_set);

    for(HashMapZoneSet::iterator it = zoneToSet.begin(); it != zoneToSet.end(); ++it) {
      result[it->first] = dss::BestFit(*(it->first), it->second);
    }

    return result;
  }

  vector<int> DS485Proxy::SendCommand(DS485Command _cmd, const Set& _set, int _param) {
    if(_set.Length() == 1) {
      Log("Optimization: Set contains only one device");
      return SendCommand(_cmd, _set.Get(0).GetDevice(), _param);
    } else if(_set.Length() > 0) {
      Apartment& apt = _set.Get(0).GetDevice().GetApartment();
      if(_set.Length() == apt.GetDevices().Length()) {
        Log("Optimization: Set contains all devices of apartment");
        // TODO: Get the group from the zone
        return SendCommand(_cmd, apt.GetZone(0), apt.GetGroup(GroupIDBroadcast), _param);
      }
    }

    vector<int> result;
    FittingResult fittedResult = BestFit(_set);
    for(FittingResult::iterator iResult = fittedResult.begin(); iResult != fittedResult.end(); ++iResult) {
      const Zone* zone = iResult->first;
      FittingResultPerModulator res = iResult->second;
      vector<Group*> groups = res.first;
      for(vector<Group*>::iterator ipGroup = groups.begin(); ipGroup != groups.end(); ++ipGroup) {
        SendCommand(_cmd, *zone, **ipGroup, _param);
      }
      Set& set = res.second;
      for(int iDevice = 0; iDevice < set.Length(); iDevice++) {
        SendCommand(_cmd, set.Get(iDevice).GetDevice(), _param);
      }
    }
    return result;
  } // SendCommand

  vector<int> DS485Proxy::SendCommand(DS485Command _cmd, const Zone& _zone, Group& _group, int _param) {
    vector<int> result;

    DS485CommandFrame frame;
    frame.GetHeader().SetDestination(0);
    frame.GetHeader().SetBroadcast(true);
    frame.GetHeader().SetType(1);
    frame.SetCommand(CommandRequest);
    if(_cmd == cmdTurnOn) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupCallScene);
      frame.GetPayload().Add<uint16_t>(_zone.GetZoneID());
      frame.GetPayload().Add<uint16_t>(_group.GetID());
      frame.GetPayload().Add<uint16_t>(SceneMax);
      SendFrame(frame);
    } else if(_cmd == cmdTurnOff) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupCallScene);
      frame.GetPayload().Add<uint16_t>(_zone.GetZoneID());
      frame.GetPayload().Add<uint16_t>(_group.GetID());
      frame.GetPayload().Add<uint16_t>(SceneOff);
      SendFrame(frame);
    } else if(_cmd == cmdCallScene) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupCallScene);
      frame.GetPayload().Add<uint16_t>(_zone.GetZoneID());
      frame.GetPayload().Add<uint16_t>(_group.GetID());
      frame.GetPayload().Add<uint16_t>(_param);
      SendFrame(frame);
    } else if(_cmd == cmdSaveScene) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupSaveScene);
      frame.GetPayload().Add<uint16_t>(_zone.GetZoneID());
      frame.GetPayload().Add<uint16_t>(_group.GetID());
      frame.GetPayload().Add<uint16_t>(_param);
      SendFrame(frame);
    } else if(_cmd == cmdUndoScene) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupUndoScene);
      frame.GetPayload().Add<uint16_t>(_zone.GetZoneID());
      frame.GetPayload().Add<uint16_t>(_group.GetID());
      frame.GetPayload().Add<uint16_t>(_param);
      SendFrame(frame);
    } else if(_cmd == cmdStartDimUp) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupStartDimInc);
      frame.GetPayload().Add<uint16_t>(_zone.GetZoneID());
      frame.GetPayload().Add<uint16_t>(_group.GetID());
      SendFrame(frame);
    } else if(_cmd == cmdStartDimDown) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupStartDimDec);
      frame.GetPayload().Add<uint16_t>(_zone.GetZoneID());
      frame.GetPayload().Add<uint16_t>(_group.GetID());
      SendFrame(frame);
    } else if(_cmd == cmdStopDim) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupEndDim);
      frame.GetPayload().Add<uint16_t>(_zone.GetZoneID());
      frame.GetPayload().Add<uint16_t>(_group.GetID());
      SendFrame(frame);
    } else if(_cmd == cmdIncreaseValue) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupIncreaseValue);
      frame.GetPayload().Add<uint16_t>(_zone.GetZoneID());
      frame.GetPayload().Add<uint16_t>(_group.GetID());
      SendFrame(frame);
    } else if(_cmd == cmdDecreaseValue) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupDecreaseValue);
      frame.GetPayload().Add<uint16_t>(_zone.GetZoneID());
      frame.GetPayload().Add<uint16_t>(_group.GetID());
      SendFrame(frame);
    }
    return result;
  } // SendCommand

  vector<int> DS485Proxy::SendCommand(DS485Command _cmd, const Device& _device, int _param) {
    return SendCommand(_cmd, _device.GetShortAddress(), _device.GetModulatorID(), _param);
  } // SendCommand

  vector<int> DS485Proxy::SendCommand(DS485Command _cmd, devid_t _id, uint8_t _modulatorID, int _param) {
    vector<int> result;
    DS485CommandFrame frame;
    frame.GetHeader().SetDestination(_modulatorID);
    frame.GetHeader().SetBroadcast(false);
    frame.GetHeader().SetType(1);
    frame.SetCommand(CommandRequest);
    if(_cmd == cmdTurnOn) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceCallScene);
      frame.GetPayload().Add<devid_t>(_id);
      frame.GetPayload().Add<uint16_t>(SceneMax);
      SendFrame(frame);
    } else if(_cmd == cmdTurnOff) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceCallScene);
      frame.GetPayload().Add<devid_t>(_id);
      frame.GetPayload().Add<uint16_t>(SceneOff);
      SendFrame(frame);
    } else if(_cmd == cmdGetOnOff) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceGetOnOff);
      frame.GetPayload().Add<uint16_t>(_id);
      SendFrame(frame);
      uint8_t res = ReceiveSingleResult(FunctionDeviceGetOnOff);
      result.push_back(res);
    } else if(_cmd == cmdGetValue) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceGetParameterValue);
      frame.GetPayload().Add<uint16_t>(_id);
      frame.GetPayload().Add<uint16_t>(_param);
      SendFrame(frame);
      uint8_t res = ReceiveSingleResult(FunctionDeviceGetParameterValue);
      result.push_back(res);
    } else if(_cmd == cmdSetValue) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceSetParameterValue);
      frame.GetPayload().Add<uint16_t>(_id);
      frame.GetPayload().Add<uint16_t>(_param);
      frame.GetPayload().Add<uint16_t>(_param); // TODO: introduce a second parameter for the value itself
      SendFrame(frame);
      uint8_t res = ReceiveSingleResult(FunctionDeviceSetParameterValue);
      result.push_back(res);
    } else if(_cmd == cmdGetFunctionID) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceGetFunctionID);
      frame.GetPayload().Add<devid_t>(_id);
      SendFrame(frame);
      vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionDeviceGetFunctionID);
      if(results.size() > 0) {
        PayloadDissector pd((results.front())->GetPayload());
        pd.Get<uint8_t>(); // skip the function id
        if(pd.Get<uint16_t>() == 0x0001) {
          result.push_back(pd.Get<uint16_t>());
        }
      }
    } else if(_cmd == cmdCallScene) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceCallScene);
      frame.GetPayload().Add<devid_t>(_id);
      frame.GetPayload().Add<uint16_t>(_param);
      SendFrame(frame);
    } else if(_cmd == cmdSaveScene) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceSaveScene);
      frame.GetPayload().Add<devid_t>(_id);
      frame.GetPayload().Add<uint16_t>(_param);
      SendFrame(frame);
    } else if(_cmd == cmdUndoScene) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceUndoScene);
      frame.GetPayload().Add<devid_t>(_id);
      frame.GetPayload().Add<uint16_t>(_param);
      SendFrame(frame);
    } else if(_cmd == cmdIncreaseValue) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceIncreaseValue);
      frame.GetPayload().Add<devid_t>(_id);
      SendFrame(frame);
    } else if(_cmd == cmdDecreaseValue) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceDecreaseValue);
      frame.GetPayload().Add<devid_t>(_id);
      SendFrame(frame);
    }
    return result;
  } // SendCommand

  void DS485Proxy::SendFrame(DS485CommandFrame& _frame, bool _force) {
    bool broadcast = _frame.GetHeader().IsBroadcast();
    bool sim = IsSimAddress(_frame.GetHeader().GetDestination());
    if(broadcast || sim) {
      Log("Sending packet to sim");
      DSS::GetInstance()->GetModulatorSim().Process(_frame);
    }
    if(broadcast || !sim) {
      if((m_DS485Controller.GetState() == csSlave) || (m_DS485Controller.GetState() == csMaster)) {
        Log("Sending packet to hardware");
        //if(_force) {
        	m_DS485Controller.EnqueueFrame(_frame);
        //}
      }
    }
  }

  bool DS485Proxy::IsSimAddress(const uint8_t _addr) {
    return DSS::GetInstance()->GetModulatorSim().GetID() == _addr;
  } // IsSimAddress

  vector<int> DS485Proxy::GetModulators() {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(0);
    cmdFrame.GetHeader().SetBroadcast(true);
    cmdFrame.SetCommand(CommandRequest);
    Log("Proxy: GetModulators");
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorGetDSID);
    SendFrame(cmdFrame);
    vector<int> result;

    map<int, bool> resultFrom;

    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionModulatorGetDSID);
    for(vector<boost::shared_ptr<DS485CommandFrame> >::iterator iFrame = results.begin(), e = results.end();
        iFrame != e; ++iFrame)
    {
      int source = (*iFrame)->GetHeader().GetSource();
      if(resultFrom[source]) {
        Log(string("already received result from ") + IntToString(source));
        continue;
      }
      resultFrom[source] = true;

      PayloadDissector pd((*iFrame)->GetPayload());
      result.push_back(source);
    }



    /*
    cmdFrame.GetPayload().Add<uint8_t>(FunctionGetTypeRequest);
    Log("Proxy: GetModulators");
    SendFrame(cmdFrame);


    map<int, bool> resultFrom;

    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionGetTypeRequest);
    for(vector<boost::shared_ptr<DS485CommandFrame> >::iterator iFrame = results.begin(), e = results.end();
        iFrame != e; ++iFrame)
    {
      int source = (*iFrame)->GetHeader().GetSource();
      if(resultFrom[source]) {
        Log(string("already received result from ") + IntToString(source));
        continue;
      }
      resultFrom[source] = true;

      PayloadDissector pd((*iFrame)->GetPayload());
      pd.Get<uint8_t>();
      uint16_t devID = pd.Get<uint16_t>();
      devID &= 0x00FF;
      if(devID == 0) {
        Log("Found dSS");
      } else if(devID == 1) {
        Log("Found dSM");
      } else {
        Log(string("Found unknown device (") + IntToString(devID) + ")");
      }
      uint16_t hwVersion = pd.Get<uint16_t>();
      uint16_t swVersion = pd.Get<uint16_t>();

      Log(string("  HW-Version: ") + IntToString(hwVersion >> 8) + "." + IntToString(hwVersion && 0xFF00));
      Log(string("  SW-Version: ") + IntToString(swVersion >> 8) + "." + IntToString(swVersion && 0xFF00));

      string name;
      for(int i = 0; i < 6; i++) {
        char c = static_cast<char>(pd.Get<uint8_t>());
        if(c != '\0') {
          name += c;
        }
      }
      Log(string("  Name:      \"") + name + "\"");

      result.push_back(source);
    }
*/
    return result;
  } // GetModulators

  int DS485Proxy::GetGroupCount(const int _modulatorID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorGetGroupsSize);
    cmdFrame.GetPayload().Add<uint16_t>(_zoneID);
    SendFrame(cmdFrame);
    int8_t res = static_cast<int8_t>(ReceiveSingleResult(FunctionModulatorGetGroupsSize));
    if(res < 0) {
      Log("GetGroupCount: Negative group count received '" + IntToString(res) +
          " on modulator " + IntToString(_modulatorID) +
          " with zone " + IntToString(_zoneID));
      res = 0;
    }
    return res;
  } // GetGroupCount

  vector<int> DS485Proxy::GetGroups(const int _modulatorID, const int _zoneID) {
    vector<int> result;

    int numGroups = GetGroupCount(_modulatorID, _zoneID);
    Log(string("Modulator has ") + IntToString(numGroups) + " groups");
    for(int iGroup = 0; iGroup < numGroups; iGroup++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.GetHeader().SetDestination(_modulatorID);
      cmdFrame.SetCommand(CommandRequest);
      cmdFrame.GetPayload().Add<uint8_t>(FunctionZoneGetGroupIdForInd);
      cmdFrame.GetPayload().Add<uint16_t>(_zoneID);
      cmdFrame.GetPayload().Add<uint16_t>(iGroup);
      SendFrame(cmdFrame);
      int8_t res = static_cast<int8_t>(ReceiveSingleResult(FunctionZoneGetGroupIdForInd));
      if(res < 0) {
        Log("GetGroups: Negative index received '" + IntToString(res) + "' for index " + IntToString(iGroup));
      } else {
        result.push_back(res);
      }
    }

    return result;
  } // GetGroups

  int DS485Proxy::GetDevicesInGroupCount(const int _modulatorID, const int _zoneID, const int _groupID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionGroupGetDeviceCount);
    cmdFrame.GetPayload().Add<uint16_t>(_zoneID);
    cmdFrame.GetPayload().Add<uint16_t>(_groupID);
    Log("before");
    SendFrame(cmdFrame);
    Log("after");
    int16_t res = static_cast<int16_t>(ReceiveSingleResult16(FunctionGroupGetDeviceCount));
    if(res < 0) {
      Log("GetDevicesInGroupCount: Negative count received '" + IntToString(res) +
          "' on modulator " + IntToString(_modulatorID) +
          " with zoneID " + IntToString(_zoneID) + " in group " + IntToString(_groupID));
      res = 0;
    }
    return res;
  } // GetDevicesInGroupCount

  vector<int> DS485Proxy::GetDevicesInGroup(const int _modulatorID, const int _zoneID, const int _groupID) {
    vector<int> result;

    int numDevices = GetDevicesInGroupCount(_modulatorID, _zoneID, _groupID);
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.GetHeader().SetDestination(_modulatorID);
      cmdFrame.SetCommand(CommandRequest);
      cmdFrame.GetPayload().Add<uint8_t>(FunctionGroupGetDevKeyForInd);
      cmdFrame.GetPayload().Add<uint16_t>(_zoneID);
      cmdFrame.GetPayload().Add<uint16_t>(_groupID);
      cmdFrame.GetPayload().Add<uint16_t>(iDevice);
      Log("b1: zoneID: " + IntToString(_zoneID) + " groupID: " + IntToString(_groupID) + " index: " + IntToString(iDevice));
      SendFrame(cmdFrame);
      Log("b2");
      int16_t res = static_cast<int16_t>(ReceiveSingleResult16(FunctionGroupGetDevKeyForInd));
      if(res < 0) {
        Log("GetDevicesInGroup: Negative device id received '" + IntToString(res) + "' for index " + IntToString(iDevice));
      } else {
        result.push_back(res);
      }
    }

    return result;
  } // GetDevicesInGroup

  vector<int> DS485Proxy::GetZones(const int _modulatorID) {
    vector<int> result;

    int numZones = GetZoneCount(_modulatorID);
    Log(string("Modulator has ") + IntToString(numZones) + " zones");
    for(int iGroup = 0; iGroup < numZones; iGroup++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.GetHeader().SetDestination(_modulatorID);
      cmdFrame.SetCommand(CommandRequest);
      cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorGetZoneIdForInd);
      cmdFrame.GetPayload().Add<uint16_t>(iGroup);
      Log("GetZoneID");
      SendFrame(cmdFrame);
      uint8_t tempResult = ReceiveSingleResult(FunctionModulatorGetZoneIdForInd);
      result.push_back(tempResult);
      Log("Receive ZoneID: " + UIntToString((unsigned int)tempResult));
    }
    return result;
  } // GetZones

  int DS485Proxy::GetZoneCount(const int _modulatorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorGetZonesSize);
    Log("GetZoneCount");
    SendFrame(cmdFrame);

    uint8_t result = ReceiveSingleResult(FunctionModulatorGetZonesSize);
    return result;
  } // GetZoneCount

  int DS485Proxy::GetDevicesCountInZone(const int _modulatorID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorCountDevInZone);
    cmdFrame.GetPayload().Add<uint16_t>(_zoneID);
    Log("GetDevicesCountInZone");
    SendFrame(cmdFrame);

    int16_t result = static_cast<int16_t>(ReceiveSingleResult16(FunctionModulatorCountDevInZone));
    if(result < 0) {
      Log("GetDevicesCountInZone: negative count '" + IntToString(result) + "'");
      result = 0;
    }

    return result;
  } // GetDevicesCountInZone

  vector<int> DS485Proxy::GetDevicesInZone(const int _modulatorID, const int _zoneID) {
    vector<int> result;

    int numDevices = GetDevicesCountInZone(_modulatorID, _zoneID);
    Log(string("Found ") + IntToString(numDevices) + " in zone.");
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.GetHeader().SetDestination(_modulatorID);
      cmdFrame.SetCommand(CommandRequest);
      cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorDevKeyInZone);
      cmdFrame.GetPayload().Add<uint16_t>(_zoneID);
      cmdFrame.GetPayload().Add<uint16_t>(iDevice);
      SendFrame(cmdFrame);

      result.push_back(ReceiveSingleResult16(FunctionModulatorDevKeyInZone));
    }
    return result;
  } // GetDevicesInZone

  void DS485Proxy::SetZoneID(const int _modulatorID, const devid_t _deviceID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionDeviceSetZoneID);
    cmdFrame.GetPayload().Add<devid_t>(_deviceID);
    cmdFrame.GetPayload().Add<uint16_t>(_zoneID);
    SendFrame(cmdFrame);
    ReceiveSingleResult(FunctionDeviceSetZoneID);
  } // SetZoneID

  void DS485Proxy::CreateZone(const int _modulatorID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorAddZone);
    cmdFrame.GetPayload().Add<uint16_t>(_zoneID);
    SendFrame(cmdFrame);
    ReceiveSingleResult(FunctionModulatorAddZone);
  } // CreateZone

  dsid_t DS485Proxy::GetDSIDOfDevice(const int _modulatorID, const int _deviceID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionDeviceGetDSID);
    cmdFrame.GetPayload().Add<uint16_t>(_deviceID);
    SendFrame(cmdFrame);
    Log("Proxy: GetDSIDOfDevice");

    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionDeviceGetDSID);
    if(results.size() != 1) {
      Log(string("DS485Proxy::GetDSIDOfDevice: received multiple or 0 results: ") + IntToString(results.size()), lsError);
      return NullDSID;
    }
    PayloadDissector pd(results.at(0)->GetPayload());
    pd.Get<uint8_t>(); // discard the function id
    pd.Get<uint16_t>(); // function result
    return pd.Get<dsid_t>();
  }

  dsid_t DS485Proxy::GetDSIDOfModulator(const int _modulatorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorGetDSID);
    Log(string("Proxy: GetDSIDOfModulator ") + IntToString(_modulatorID));
    SendFrame(cmdFrame);

    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionModulatorGetDSID);
    if(results.size() != 1) {
      Log(string("DS485Proxy::GetDSIDOfModulator: received multiple results ") + IntToString(results.size()), lsError);
      return NullDSID;
    }
    PayloadDissector pd(results.at(0)->GetPayload());
    pd.Get<uint8_t>(); // discard the function id
    return pd.Get<dsid_t>();
  } // GetDSIDOfModulator

  unsigned long DS485Proxy::GetPowerConsumption(const int _modulatorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorGetPowerConsumption);
    Log(string("Proxy: GetPowerConsumption ") + IntToString(_modulatorID));
    SendFrame(cmdFrame);

    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionModulatorGetPowerConsumption);
    if(results.size() != 1) {
      Log(string("DS485Proxy::GetPowerConsumption: received multiple results ") + IntToString(results.size()), lsError);
      return 0;
    }
    PayloadDissector pd(results.at(0)->GetPayload());
    pd.Get<uint8_t>(); // discard the function id
    return pd.Get<devid_t>();
  } // GetPowerConsumption

  unsigned long DS485Proxy::GetEnergyMeterValue(const int _modulatorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorGetEnergyMeterValue);
    Log(string("Proxy: GetEnergyMeterValue ") + IntToString(_modulatorID));
    SendFrame(cmdFrame);

    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(FunctionModulatorGetEnergyMeterValue);
    if(results.size() != 1) {
      Log(string("DS485Proxy::GetEnergyMeterValue: received multiple results ") + IntToString(results.size()), lsError);
      return 0;
    }
    PayloadDissector pd(results.at(0)->GetPayload());
    pd.Get<uint8_t>(); // discard the function id
    return pd.Get<devid_t>();
  } // GetEnergyMeterValue

  void DS485Proxy::Subscribe(const int _modulatorID, const int _groupID, const int _deviceID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionDeviceSubscribe);
    cmdFrame.GetPayload().Add<uint16_t>(_groupID);
    cmdFrame.GetPayload().Add<devid_t>(_deviceID);
    Log(string("Proxy: Subscribe ") + IntToString(_modulatorID));
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


  vector<boost::shared_ptr<DS485CommandFrame> > DS485Proxy::Receive(uint8_t _functionID) {
    vector<boost::shared_ptr<DS485CommandFrame> > result;

    if(m_DS485Controller.GetState() == csSlave || m_DS485Controller.GetState() == csMaster) {
      // Wait for four tokens
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

  uint8_t DS485Proxy::ReceiveSingleResult(const uint8_t _functionID) {
    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(_functionID);

    if(results.empty()) {
      Log(string("received no results for request (") + FunctionIDToString(_functionID) + ")");
      return 0; // TODO: rewrite
    } else if(results.size() > 1) {
      Log(string("received multiple results (") + IntToString(results.size()) + ") for request (" + FunctionIDToString(_functionID) + ")");
      return 0; // TODO: rewrite
    }

    DS485CommandFrame* frame = results.at(0).get();

    PayloadDissector pd(frame->GetPayload());
    uint8_t functionID = pd.Get<uint8_t>();
    if(functionID != _functionID) {
      Log("function ids are different");
    }
    uint8_t result = pd.Get<uint8_t>();

    results.clear();

    return result;
  } // ReceiveSingleResult

  uint16_t DS485Proxy::ReceiveSingleResult16(const uint8_t _functionID) {
    vector<boost::shared_ptr<DS485CommandFrame> > results = Receive(_functionID);

    if(results.empty()) {
      Log(string("received no results for request (") + FunctionIDToString(_functionID) + ")");
      return 0; // TODO: rewrite
    } else if(results.size() > 1) {
      Log(string("received multiple results (") + IntToString(results.size()) + ") for request (" + FunctionIDToString(_functionID) + ")");
      return 0; // TODO: rewrite
    }

    DS485CommandFrame* frame = results.at(0).get();

    PayloadDissector pd(frame->GetPayload());
    uint8_t functionID = pd.Get<uint8_t>();
    if(functionID != _functionID) {
      Log("function ids are different");
    }
    uint16_t result = pd.Get<uint8_t>();
    if(!pd.IsEmpty()) {
      result |= (pd.Get<uint8_t>() << 8);
    } else {
      Log("ReceiveSingleResult16: only received half of the data (8bit)");
    }

    results.clear();

    return result;
  } // ReceiveSingleResult16

  void DS485Proxy::Initialize() {
    Subsystem::Initialize();
    m_DS485Controller.AddFrameCollector(this);
    DSS::GetInstance()->GetModulatorSim().AddFrameCollector(this);
  }

  void DS485Proxy::Start() {
    Subsystem::Start();
    try {
      m_DS485Controller.Run();
    } catch (const runtime_error& _ex) {
    	Log(string("Caught exception while starting DS485Controlle: ") + _ex.what());
    }
    // call Thread::Run()
    Run();
  } // Run

  void DS485Proxy::WaitForProxyEvent() {
    m_ProxyEvent.WaitFor();
  } // WaitForProxyEvent

  void DS485Proxy::SignalEvent() {
    m_ProxyEvent.Signal();
  } // SignalEvent

  const char* FunctionIDToString(const int _functionID) {
    switch(_functionID) {
    case  FunctionModulatorAddZone:
      return "Modulator Add Zone";
    case  FunctionModulatorRemoveZone:
      return "Modulator Remove Zone";
    case  FunctionModulatorRemoveAllZones:
      return "Modulator Remove All Zones";
    case  FunctionModulatorCountDevInZone:
      return "Modulator Count Dev In Zone";
    case  FunctionModulatorDevKeyInZone:
      return "Modulator Dev Key In Zone";
    case  FunctionModulatorGetGroupsSize:
      return "Modulator Get Groups Size";
    case  FunctionModulatorGetZonesSize:
      return "Modulator Get Zones Size";
    case  FunctionModulatorGetZoneIdForInd:
      return "Modulator Get Zone Id For Index";
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

    case  FunctionZoneGetGroupIdForInd:
      return "Zone Get Group ID For Index";

    case  FunctionDeviceCallScene:
      return "Device Call Scene";
    case  FunctionDeviceSaveScene:
      return "Device Save Scene";
    case  FunctionDeviceUndoScene:
      return "Device Undo Scene";

    case FunctionDeviceIncreaseValue:
    	return "Function Device Increase Value";
    case FunctionDeviceDecreaseValue:
    	return "Function Device Decrease Value";
    case FunctionDeviceStartDimInc:
    	return "Function Device Start Dim Inc";
    case FunctionDeviceStartDimDec:
    	return "Function Device Start Dim Dec";
    case FunctionDeviceEndDim:
    	return "Function Device End Dim";

    case  FunctionGroupCallScene:
      return "Group Call Scene";
    case  FunctionGroupSaveScene:
      return "Group Save Scene";
    case  FunctionGroupUndoScene:
      return "Group Undo Scene";

    case FunctionGroupIncreaseValue:
    	return "Function Group Increase Value";
    case FunctionGroupDecreaseValue:
    	return "Function Group Decrease Value";
    case FunctionGroupStartDimInc:
    	return "Function Group Start Dim Inc";
    case FunctionGroupStartDimDec:
    	return "Function Group Start Dim Dec";
    case FunctionGroupEndDim:
    	return "Function Group End Dim";


    case FunctionDeviceSetZoneID:
    	return "Device Set ZoneID";

    case  FunctionDeviceGetOnOff:
      return "Function Device Get On Off";
    case  FunctionDeviceGetParameterValue:
      return "Function Device Get Parameter Value";
    case  FunctionDeviceGetDSID:
      return "Function Device Get DSID";

    case FunctionModulatorGetDSID:
      return "Function Modulator Get DSID";

    case FunctionModulatorGetPowerConsumption:
    	return "Function Modulator Get PowerConsumption";

    case FunctionModulatorGetEnergyMeterValue:
      return "Function Modulator Get Energy-Meter Value";

    case FunctionGetTypeRequest:
      return "Function Get Type";

    case FunctionKeyPressed:
      return "Function Key Pressed";

    case FunctionDeviceSubscribe:
      return "Function Device Subscribe";

    case FunctionDeviceGetFunctionID:
      return "Function Device Get Function ID";
    }
    return "";
  } // FunctionIDToString

  void DS485Proxy::Execute() {
    SignalEvent();

    while(!m_Terminated) {
		if(!m_IncomingFrames.empty() || m_PacketHere.WaitFor(50)) {
		  while(!m_IncomingFrames.empty()) {
			// process packets and put them into a functionID-hash
			boost::shared_ptr<DS485CommandFrame> frame = m_IncomingFrames.front();
			m_IncomingFrames.erase(m_IncomingFrames.begin());

			vector<unsigned char> ch = frame->GetPayload().ToChar();
			if(ch.size() < 1) {
			  Log("Received Command Frame w/o function identifier");
			  continue;
			}

			uint8_t functionID = ch.front();
			if(frame->GetCommand() == CommandRequest) {
			  if(functionID == FunctionZoneAddDevice) {

			  }
			} else {
			  std::ostringstream sstream;
			  sstream << "Response: ";
			  PayloadDissector pd(frame->GetPayload());
			  while(!pd.IsEmpty()) {
				  uint8_t data = pd.Get<uint8_t>();
				  sstream << "(0x" << std::hex << (unsigned int)data << ", " << std::dec << (int)data << "d)";
			  }
			  sstream << std::dec;
			  Log(sstream.str());

			  // create an explicit copy since frame is a shared ptr and would free the contained
			  // frame if going out of scope
			  DS485CommandFrame* pFrame = new DS485CommandFrame();
			  *pFrame = *frame.get();

			  Log(string("Response for: ") + FunctionIDToString(functionID));
			  ReceivedFrame* rf = new ReceivedFrame(m_DS485Controller.GetTokenCount(), pFrame);
			  m_ReceivedFramesByFunctionID[functionID].push_back(rf);
			}
		  }
		}
    }
  } // Execute

  void DS485Proxy::CollectFrame(boost::shared_ptr<DS485CommandFrame>& _frame) {
    uint8_t commandID = _frame->GetCommand();
    if(commandID != CommandResponse && commandID != CommandRequest) {
      Log("discarded non response/request frame", lsInfo);
      Log(string("frame type ") + CommandToString(commandID));
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
