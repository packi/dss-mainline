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
#include "core/event.h"
#include "core/propertysystem.h"
#include "core/foreach.h"

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

  const bool OptimizerDebug = true;

  /** Precondition: _set contains only devices of _zone */
  FittingResultPerModulator BestFit(const Zone& _zone, const Set& _set) {
    Set workingCopy = _set;

    vector<Group*> fittingGroups;
    Set singleDevices;

    if(OptimizerDebug) {
      Logger::GetInstance()->Log("Finding fit for zone " + IntToString(_zone.GetZoneID()));
    }


	if(_zone.GetDevices().Length() == _set.Length()) {
	  fittingGroups.push_back(_zone.GetGroup(GroupIDBroadcast));
	  Logger::GetInstance()->Log(string("Optimization: Set contains all devices of zone ") + IntToString(_zone.GetZoneID()));
	} else {
	    vector<Group*> unsuitableGroups;
	    Set workingCopy = _set;

		while(!workingCopy.IsEmpty()) {
		  DeviceReference& ref = workingCopy.Get(0);
		  workingCopy.RemoveDevice(ref);

		  if(OptimizerDebug) {
		    Logger::GetInstance()->Log("Working with device " + ref.GetDSID().ToString());
		  }

		  bool foundGroup = false;
		  for(int iGroup = 0; iGroup < ref.GetDevice().GetGroupsCount(); iGroup++) {
			Group& g = ref.GetDevice().GetGroupByIndex(iGroup);

  		    if(OptimizerDebug) {
		      Logger::GetInstance()->Log("  Checking Group " + IntToString(g.GetID()));
		    }
			// continue if already found unsuitable
			if(find(unsuitableGroups.begin(), unsuitableGroups.end(), &g) != unsuitableGroups.end()) {
  		      if(OptimizerDebug) {
		        Logger::GetInstance()->Log("  Group discarded before, continuing search");
		      }
			  continue;
			}

			// see if we've got a fit
			bool groupFits = true;
			Set devicesInGroup = _zone.GetDevices().GetByGroup(g);
  		    if(OptimizerDebug) {
		      Logger::GetInstance()->Log("    Group has " + IntToString(devicesInGroup.Length()) + " devices");
		    }
			for(int iDevice = 0; iDevice < devicesInGroup.Length(); iDevice++) {
			  if(!_set.Contains(devicesInGroup.Get(iDevice))) {
				unsuitableGroups.push_back(&g);
				groupFits = false;
   		        if(OptimizerDebug) {
		          Logger::GetInstance()->Log("    Original set does _not_ contain device " + devicesInGroup.Get(iDevice).GetDevice().GetDSID().ToString());
		        }
				break;
			  }
   		      if(OptimizerDebug) {
		        Logger::GetInstance()->Log("    Original set contains device " + devicesInGroup.Get(iDevice).GetDevice().GetDSID().ToString());
		      }
			}
			if(groupFits) {
  		      if(OptimizerDebug) {
		        Logger::GetInstance()->Log("  Found a fit " + IntToString(g.GetID()));
		      }
			  foundGroup = true;
			  fittingGroups.push_back(&g);
   		      if(OptimizerDebug) {
		        Logger::GetInstance()->Log("  Removing devices from working copy");
		      }
			  while(!devicesInGroup.IsEmpty()) {
				workingCopy.RemoveDevice(devicesInGroup.Get(0));
			    devicesInGroup.RemoveDevice(devicesInGroup.Get(0));
			  }
   		      if(OptimizerDebug) {
		        Logger::GetInstance()->Log("  Done. (Removing devices from working copy)");
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
  {
    if(_pDSS != NULL) {
      _pDSS->GetPropertySystem().CreateProperty(GetConfigPropertyBasePath() + "rs485devicename")
            ->LinkToProxy(PropertyProxyMemberFunction<DS485Controller, string>(m_DS485Controller, &DS485Controller::GetRS485DeviceName, &DS485Controller::SetRS485DeviceName));

      _pDSS->GetPropertySystem().CreateProperty(GetPropertyBasePath() + "tokensReceived")
            ->LinkToProxy(PropertyProxyMemberFunction<DS485Controller, int>(m_DS485Controller, &DS485Controller::GetTokenCount));

      const DS485FrameReader& reader = m_DS485Controller.GetFrameReader();
      _pDSS->GetPropertySystem().CreateProperty(GetPropertyBasePath() + "framesReceived")
            ->LinkToProxy(PropertyProxyMemberFunction<DS485FrameReader, int>(reader, &DS485FrameReader::GetNumberOfFramesReceived));

      _pDSS->GetPropertySystem().CreateProperty(GetPropertyBasePath() + "incompleteFramesReceived")
            ->LinkToProxy(PropertyProxyMemberFunction<DS485FrameReader, int>(reader, &DS485FrameReader::GetNumberOfIncompleteFramesReceived));

      _pDSS->GetPropertySystem().CreateProperty(GetPropertyBasePath() + "crcErrors")
            ->LinkToProxy(PropertyProxyMemberFunction<DS485FrameReader, int>(reader, &DS485FrameReader::GetNumberOfCRCErrors));
    }
  } // ctor

  bool DS485Proxy::IsReady() {
	  return IsRunning()
	         && ((m_DS485Controller.GetState() == csSlave) ||
	             (m_DS485Controller.GetState() == csDesignatedMaster) ||
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

  vector<int> DS485Proxy::SendCommand(DS485Command _cmd, const Zone& _zone, uint8_t _groupID, int _param) {
    vector<int> result;

    DS485CommandFrame frame;
    frame.GetHeader().SetDestination(0);
    frame.GetHeader().SetBroadcast(true);
    frame.GetHeader().SetType(1);
    frame.SetCommand(CommandRequest);
    int toZone;
    if(GetDSS().GetApartment().GetZones().size() == 2 /* zone 0 + another zone */) {
      toZone = 0;
      Log("SendCommand(Zone,GroupID): Only one zone present, sending frame to broadcast zone");
    } else {
      toZone = _zone.GetZoneID();
    }
    if(_cmd == cmdTurnOn) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupCallScene);
      frame.GetPayload().Add<uint16_t>(toZone);
      frame.GetPayload().Add<uint16_t>(_groupID);
      frame.GetPayload().Add<uint16_t>(SceneMax);
      SendFrame(frame);
      Log("turn on: zone " + IntToString(_zone.GetZoneID()) + " group: " + IntToString(_groupID));
    } else if(_cmd == cmdTurnOff) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupCallScene);
      frame.GetPayload().Add<uint16_t>(toZone);
      frame.GetPayload().Add<uint16_t>(_groupID);
      frame.GetPayload().Add<uint16_t>(SceneOff);
      SendFrame(frame);
      Log("turn off: zone " + IntToString(_zone.GetZoneID()) + " group: " + IntToString(_groupID));
    } else if(_cmd == cmdCallScene) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupCallScene);
      frame.GetPayload().Add<uint16_t>(toZone);
      frame.GetPayload().Add<uint16_t>(_groupID);
      frame.GetPayload().Add<uint16_t>(_param);
      SendFrame(frame);
      Log("call scene: zone " + IntToString(_zone.GetZoneID()) + " group: " + IntToString(_groupID));
    } else if(_cmd == cmdSaveScene) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupSaveScene);
      frame.GetPayload().Add<uint16_t>(toZone);
      frame.GetPayload().Add<uint16_t>(_groupID);
      frame.GetPayload().Add<uint16_t>(_param);
      SendFrame(frame);
    } else if(_cmd == cmdUndoScene) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupUndoScene);
      frame.GetPayload().Add<uint16_t>(toZone);
      frame.GetPayload().Add<uint16_t>(_groupID);
      frame.GetPayload().Add<uint16_t>(_param);
      SendFrame(frame);
    } else if(_cmd == cmdStartDimUp) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupStartDimInc);
      frame.GetPayload().Add<uint16_t>(toZone);
      frame.GetPayload().Add<uint16_t>(_groupID);
      SendFrame(frame);
    } else if(_cmd == cmdStartDimDown) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupStartDimDec);
      frame.GetPayload().Add<uint16_t>(toZone);
      frame.GetPayload().Add<uint16_t>(_groupID);
      SendFrame(frame);
    } else if(_cmd == cmdStopDim) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupEndDim);
      frame.GetPayload().Add<uint16_t>(toZone);
      frame.GetPayload().Add<uint16_t>(_groupID);
      SendFrame(frame);
    } else if(_cmd == cmdIncreaseValue) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupIncreaseValue);
      frame.GetPayload().Add<uint16_t>(toZone);
      frame.GetPayload().Add<uint16_t>(_groupID);
      SendFrame(frame);
    } else if(_cmd == cmdDecreaseValue) {
      frame.GetPayload().Add<uint8_t>(FunctionGroupDecreaseValue);
      frame.GetPayload().Add<uint16_t>(toZone);
      frame.GetPayload().Add<uint16_t>(_groupID);
      SendFrame(frame);
    }
    return result;
  }

  vector<int> DS485Proxy::SendCommand(DS485Command _cmd, const Zone& _zone, Group& _group, int _param) {
    return SendCommand(_cmd, _zone, _group.GetID(), _param);
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
      uint8_t res = ReceiveSingleResult(frame, FunctionDeviceGetOnOff);
      result.push_back(res);
    } else if(_cmd == cmdGetValue) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceGetParameterValue);
      frame.GetPayload().Add<uint16_t>(_id);
      frame.GetPayload().Add<uint16_t>(_param);
      uint8_t res = ReceiveSingleResult(frame, FunctionDeviceGetParameterValue);
      result.push_back(res);
    } else if(_cmd == cmdSetValue) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceSetParameterValue);
      frame.GetPayload().Add<uint16_t>(_id);
      frame.GetPayload().Add<uint16_t>(_param);
      frame.GetPayload().Add<uint16_t>(_param); // TODO: introduce a second parameter for the value itself
      uint8_t res = ReceiveSingleResult(frame, FunctionDeviceSetParameterValue);
      result.push_back(res);
    } else if(_cmd == cmdGetFunctionID) {
      frame.GetPayload().Add<uint8_t>(FunctionDeviceGetFunctionID);
      frame.GetPayload().Add<devid_t>(_id);

      boost::shared_ptr<ReceivedFrame> resFrame = ReceiveSingleFrame(frame, FunctionDeviceGetFunctionID);
      if(resFrame.get() != NULL) {
        PayloadDissector pd(resFrame->GetFrame()->GetPayload());
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

  void DS485Proxy::SendFrame(DS485CommandFrame& _frame) {
    bool broadcast = _frame.GetHeader().IsBroadcast();
    bool sim = IsSimAddress(_frame.GetHeader().GetDestination());
    if(broadcast || sim) {
      Log("Sending packet to sim");
      GetDSS().GetModulatorSim().Process(_frame);
    }
    if(broadcast || !sim) {
      if((m_DS485Controller.GetState() == csSlave) || (m_DS485Controller.GetState() == csMaster)) {
        Log("Sending packet to hardware");
       	m_DS485Controller.EnqueueFrame(_frame);
      }
    }
    boost::shared_ptr<DS485CommandFrame> pFrame(new DS485CommandFrame);
    *pFrame = _frame;
    // relay the frame to update our state
    CollectFrame(pFrame);
  } // SendFrame

  boost::shared_ptr<FrameBucket> DS485Proxy::SendFrameAndInstallBucket(DS485CommandFrame& _frame, const int _functionID) {
    int sourceID = _frame.GetHeader().IsBroadcast() ? -1 :  _frame.GetHeader().GetDestination();
    boost::shared_ptr<FrameBucket> result(new FrameBucket(this, _functionID, sourceID));
    SendFrame(_frame);
    return result;
  }

  bool DS485Proxy::IsSimAddress(const uint8_t _addr) {
    return GetDSS().GetModulatorSim().GetID() == _addr;
  } // IsSimAddress

  vector<int> DS485Proxy::GetModulators() {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(0);
    cmdFrame.GetHeader().SetBroadcast(true);
    cmdFrame.SetCommand(CommandRequest);
    Log("Proxy: GetModulators");
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorGetDSID);

    boost::shared_ptr<FrameBucket> bucket = SendFrameAndInstallBucket(cmdFrame, FunctionModulatorGetDSID);

    bucket->WaitForFrames(1000);

    map<int, bool> resultFrom;

    vector<int> result;
    while(true) {
      boost::shared_ptr<ReceivedFrame> recFrame = bucket->PopFrame();
      if(recFrame.get() == NULL) {
        break;
      }

      int source = recFrame->GetFrame()->GetHeader().GetSource();
      if(resultFrom[source]) {
        Log("already received result from " + IntToString(source));
        continue;
      }
      resultFrom[source] = true;

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
    int8_t res = static_cast<int8_t>(ReceiveSingleResult(cmdFrame, FunctionModulatorGetGroupsSize));
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

      int8_t res = static_cast<int8_t>(ReceiveSingleResult(cmdFrame, FunctionZoneGetGroupIdForInd));
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

    int16_t res = static_cast<int16_t>(ReceiveSingleResult16(cmdFrame, FunctionGroupGetDeviceCount));
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
      int16_t res = static_cast<int16_t>(ReceiveSingleResult16(cmdFrame, FunctionGroupGetDevKeyForInd));
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
      uint8_t tempResult = ReceiveSingleResult(cmdFrame, FunctionModulatorGetZoneIdForInd);
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

    // TODO: check result-code
    uint8_t result = ReceiveSingleResult(cmdFrame, FunctionModulatorGetZonesSize);
    return result;
  } // GetZoneCount

  int DS485Proxy::GetDevicesCountInZone(const int _modulatorID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorCountDevInZone);
    cmdFrame.GetPayload().Add<uint16_t>(_zoneID);
    Log("GetDevicesCountInZone");

    Log(IntToString(_modulatorID) + " " + IntToString(_zoneID));

    int16_t result = static_cast<int16_t>(ReceiveSingleResult16(cmdFrame, FunctionModulatorCountDevInZone));
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

      result.push_back(ReceiveSingleResult16(cmdFrame, FunctionModulatorDevKeyInZone));
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

    // TODO: check result-code
    ReceiveSingleResult(cmdFrame, FunctionDeviceSetZoneID);
  } // SetZoneID

  void DS485Proxy::CreateZone(const int _modulatorID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorAddZone);
    cmdFrame.GetPayload().Add<uint16_t>(_zoneID);

    // TODO: check result-code
    ReceiveSingleResult(cmdFrame, FunctionModulatorAddZone);
  } // CreateZone

  dsid_t DS485Proxy::GetDSIDOfDevice(const int _modulatorID, const int _deviceID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionDeviceGetDSID);
    cmdFrame.GetPayload().Add<uint16_t>(_deviceID);
    Log("Proxy: GetDSIDOfDevice");

    boost::shared_ptr<ReceivedFrame> recFrame = ReceiveSingleFrame(cmdFrame, FunctionDeviceGetDSID);
    if(recFrame.get() == NULL) {
      return NullDSID;
    }

    PayloadDissector pd(recFrame->GetFrame()->GetPayload());
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
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorGetDSID);

    boost::shared_ptr<ReceivedFrame> recFrame = ReceiveSingleFrame(cmdFrame, FunctionModulatorGetDSID);
    if(recFrame.get() == NULL) {
      Log("GetDSIDOfModulator: received no result from " + IntToString(_modulatorID), lsError);
      return NullDSID;
    }

    PayloadDissector pd(recFrame->GetFrame()->GetPayload());
    pd.Get<uint8_t>(); // discard the function id
    //pd.Get<uint8_t>(); // function result, don't know if that's sent though
    return pd.Get<dsid_t>();
  } // GetDSIDOfModulator

  unsigned long DS485Proxy::GetPowerConsumption(const int _modulatorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorGetPowerConsumption);
    Log(string("Proxy: GetPowerConsumption ") + IntToString(_modulatorID));

    boost::shared_ptr<ReceivedFrame> recFrame = ReceiveSingleFrame(cmdFrame, FunctionModulatorGetPowerConsumption);
    if(recFrame.get() == NULL) {
      Log("DS485Proxy::GetPowerConsumption: received no results", lsError);
      return 0;
    }
    if(recFrame->GetFrame()->GetHeader().GetSource() != _modulatorID) {
      Log("GetPowerConsumption: received result from wrong source");
    }
    PayloadDissector pd(recFrame->GetFrame()->GetPayload());
    pd.Get<uint8_t>(); // discard the function id
    return pd.Get<uint32_t>();
  } // GetPowerConsumption

  unsigned long DS485Proxy::GetEnergyMeterValue(const int _modulatorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.GetHeader().SetDestination(_modulatorID);
    cmdFrame.SetCommand(CommandRequest);
    cmdFrame.GetPayload().Add<uint8_t>(FunctionModulatorGetEnergyMeterValue);
    Log(string("Proxy: GetEnergyMeterValue ") + IntToString(_modulatorID));

    boost::shared_ptr<ReceivedFrame> recFrame = ReceiveSingleFrame(cmdFrame, FunctionModulatorGetEnergyMeterValue);
    if(recFrame.get() == NULL) {
      Log("DS485Proxy::GetEnergyMeterValue: received no results", lsError);
      return 0;
    }
    PayloadDissector pd(recFrame->GetFrame()->GetPayload());
    pd.Get<uint8_t>(); // discard the function id
    return pd.Get<uint16_t>();
  } // GetEnergyMeterValue


  void DS485Proxy::AddToGroup(const int _modulatorID, const int _groupID, const int _deviceID) {

  } // AddToGroup

  void DS485Proxy::RemoveFromGroup(const int _modulatorID, const int _groupID, const int _deviceID) {

  } // RemoveFromGroup

  int DS485Proxy::AddUserGroup(const int _modulatorID) {
    return 0;
  } // AddUserGroup

  void DS485Proxy::RemoveUserGroup(const int _modulatorID, const int _groupID) {

  } // RemoveUserGroup

  boost::shared_ptr<ReceivedFrame> DS485Proxy::ReceiveSingleFrame(DS485CommandFrame& _frame, uint8_t _functionID) {
    boost::shared_ptr<FrameBucket> bucket = SendFrameAndInstallBucket(_frame, _functionID);
    bucket->WaitForFrame(1000);

    if(bucket->IsEmpty()) {
      Log(string("received no results for request (") + FunctionIDToString(_functionID) + ")");
      return boost::shared_ptr<ReceivedFrame>();
    } else if(bucket->GetFrameCount() > 1) {
      Log(string("received multiple results (") + IntToString(bucket->GetFrameCount()) + ") for request (" + FunctionIDToString(_functionID) + ")");
      return boost::shared_ptr<ReceivedFrame>();
    }

    boost::shared_ptr<ReceivedFrame> recFrame = bucket->PopFrame();

    if(recFrame.get() != NULL) {
      return recFrame;
    } else {
      throw runtime_error("Received frame is NULL but bucket->IsEmpty() returns false");
    }
  } // ReceiveSingleFrame

  uint8_t DS485Proxy::ReceiveSingleResult(DS485CommandFrame& _frame, const uint8_t _functionID) {
    boost::shared_ptr<ReceivedFrame> recFrame = ReceiveSingleFrame(_frame, _functionID);

    if(recFrame.get() != NULL) {
      PayloadDissector pd(recFrame->GetFrame()->GetPayload());
      uint8_t functionID = pd.Get<uint8_t>();
      if(functionID != _functionID) {
        Log("function ids are different");
      }
      uint8_t result = pd.Get<uint8_t>();
      return result;
    } else {
      return 0;
    }
  } // ReceiveSingleResult

  uint16_t DS485Proxy::ReceiveSingleResult16(DS485CommandFrame& _frame, const uint8_t _functionID) {
    boost::shared_ptr<ReceivedFrame> recFrame = ReceiveSingleFrame(_frame, _functionID);

    if(recFrame.get() != NULL) {
      PayloadDissector pd(recFrame->GetFrame()->GetPayload());
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
      return result;
    } else {
      return 0;
    }
  } // ReceiveSingleResult16

  void DS485Proxy::Initialize() {
    Subsystem::Initialize();
    m_DS485Controller.AddFrameCollector(this);
    GetDSS().GetModulatorSim().AddFrameCollector(this);
  }

  void DS485Proxy::DoStart() {
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
          m_IncomingFramesGuard.Lock();
          // process packets and put them into a functionID-hash
          boost::shared_ptr<DS485CommandFrame> frame = m_IncomingFrames.front();
          m_IncomingFrames.erase(m_IncomingFrames.begin());
          m_IncomingFramesGuard.Unlock();
          Log("R");

          vector<unsigned char> ch = frame->GetPayload().ToChar();
          if(ch.size() < 1) {
            Log("Received Command Frame w/o function identifier");
            continue;
          }

          uint8_t functionID = ch.front();
          if(frame->GetCommand() == CommandRequest || frame->GetCommand() == CommandEvent) {
            string functionIDStr = FunctionIDToString(functionID);
            if(functionIDStr.empty()) {
              functionIDStr = "Unknown function id: " + IntToString(functionID, true);
            }
            Log("Got request: " + functionIDStr);
            PayloadDissector pd(frame->GetPayload());
            /*
            if(frame->GetHeader().IsBroadcast()) {
              Log("Redistributing frame to simulation");
              GetDSS().GetModulatorSim().Process(*frame.get());
            }
            */
            if(functionID == FunctionZoneAddDevice) {
              Log("New device");
              pd.Get<uint8_t>(); // function id
              int modID = frame->GetHeader().GetSource();
              int zoneID = pd.Get<uint16_t>();
              int devID = pd.Get<uint16_t>();
              pd.Get<uint16_t>(); // version
              int functionID = pd.Get<uint16_t>();

              ModelEvent* pEvent = new ModelEvent(ModelEvent::etNewDevice);
              pEvent->AddParameter(modID);
              pEvent->AddParameter(zoneID);
              pEvent->AddParameter(devID);
              pEvent->AddParameter(functionID);
              GetDSS().GetApartment().AddModelEvent(pEvent);
            } else if(functionID == FunctionGroupCallScene) {
              pd.Get<uint8_t>(); // function id
              uint16_t zoneID = pd.Get<uint16_t>();
              uint16_t groupID = pd.Get<uint16_t>();
              uint16_t sceneID = pd.Get<uint16_t>();
              if(sceneID == SceneBell) {
                boost::shared_ptr<Event> evt(new Event("bell"));
                GetDSS().GetEventQueue().PushEvent(evt);
              } else if(sceneID == SceneAlarm) {
                boost::shared_ptr<Event> evt(new Event("alarm"));
                GetDSS().GetEventQueue().PushEvent(evt);
              } else if(sceneID == ScenePanic) {
                boost::shared_ptr<Event> evt(new Event("panic"));
                GetDSS().GetEventQueue().PushEvent(evt);
              } else {
                ModelEvent* pEvent = new ModelEvent(ModelEvent::etCallSceneGroup);
                pEvent->AddParameter(zoneID);
                pEvent->AddParameter(groupID);
                pEvent->AddParameter(sceneID);
                GetDSS().GetApartment().AddModelEvent(pEvent);
              }
            } else if(functionID == FunctionDeviceCallScene) {
              pd.Get<uint8_t>(); // functionID
              uint16_t devID = pd.Get<uint16_t>();
              uint16_t sceneID = pd.Get<uint16_t>();
              int modID = frame->GetHeader().GetDestination();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etCallSceneDevice);
              pEvent->AddParameter(modID);
              pEvent->AddParameter(devID);
              pEvent->AddParameter(sceneID);
              GetDSS().GetApartment().AddModelEvent(pEvent);
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

            Log(string("Response for: ") + FunctionIDToString(functionID));
            boost::shared_ptr<ReceivedFrame> rf(new ReceivedFrame(m_DS485Controller.GetTokenCount(), frame));

            bool bucketFound = false;
            // search for a bucket to put the frame in
            foreach(FrameBucket* bucket, m_FrameBuckets) {
              if(bucket->GetFunctionID() == functionID) {
                if((bucket->GetSourceID() == -1) || (bucket->GetSourceID() == frame->GetHeader().GetSource())) {
                  bucket->AddFrame(rf);
                  bucketFound = true;
                }
              }
            }
            if(!bucketFound) {
              Log("No bucket found for " + IntToString(frame->GetHeader().GetSource()));
            }

          }
        }
      }
    }
  } // Execute

  void DS485Proxy::CollectFrame(boost::shared_ptr<DS485CommandFrame>& _frame) {
    uint8_t commandID = _frame->GetCommand();
    if(commandID != CommandResponse && commandID != CommandRequest && commandID != CommandEvent) {
      Log("discarded non response/request/command frame", lsInfo);
      Log(string("frame type ") + CommandToString(commandID));
    } else {
      m_IncomingFramesGuard.Lock();
      m_IncomingFrames.push_back(_frame);
      m_IncomingFramesGuard.Unlock();
      m_PacketHere.Signal();
    }
  } // CollectFrame

  void DS485Proxy::AddFrameBucket(FrameBucket* _bucket) {
    m_FrameBuckets.push_back(_bucket);
  }

  void DS485Proxy::RemoveFrameBucket(FrameBucket* _bucket) {
    vector<FrameBucket*>::iterator pos = find(m_FrameBuckets.begin(), m_FrameBuckets.end(), _bucket);
    if(pos != m_FrameBuckets.end()) {
      m_FrameBuckets.erase(pos);
    }
  }

  //================================================== ReceivedFrame

  ReceivedFrame::ReceivedFrame(const int _receivedAt, boost::shared_ptr<DS485CommandFrame> _frame)
  : m_ReceivedAtToken(_receivedAt),
    m_Frame(_frame)
  {
  } // ctor


  //================================================== FrameBucket

  FrameBucket::FrameBucket(DS485Proxy* _proxy, int _functionID, int _sourceID)
  : m_pProxy(_proxy),
    m_FunctionID(_functionID),
    m_SourceID(_sourceID)
  {
    Logger::GetInstance()->Log("Bucket: Registering for fid: " + IntToString(_functionID) + " sid: " + IntToString(_sourceID));
    m_pProxy->AddFrameBucket(this);
  } // ctor

  FrameBucket::~FrameBucket() {
    Logger::GetInstance()->Log("Bucket: Removing for fid: " + IntToString(m_FunctionID) + " sid: " + IntToString(m_SourceID));
    m_pProxy->RemoveFrameBucket(this);
  } // dtor

  void FrameBucket::AddFrame(boost::shared_ptr<ReceivedFrame> _frame) {
    m_FramesMutex.Lock();
    m_Frames.push_back(_frame);
    m_FramesMutex.Unlock();

    m_PacketHere.Signal();
  } // AddFrame

  boost::shared_ptr<ReceivedFrame> FrameBucket::PopFrame() {
    boost::shared_ptr<ReceivedFrame> result;

    m_FramesMutex.Lock();
    if(!m_Frames.empty()) {
      result = m_Frames.front();
      m_Frames.pop_front();
    }
    m_FramesMutex.Unlock();
    return result;
  } // PopFrame

  void FrameBucket::WaitForFrames(int _timeoutMS) {
    SleepMS(_timeoutMS);
  } // WaitForFrames

  void FrameBucket::WaitForFrame(int _timeoutMS) {
    Logger::GetInstance()->Log("*** Waiting");
    if(m_PacketHere.WaitFor(_timeoutMS)) {
      Logger::GetInstance()->Log("*** Got Frame");
    } else {
      Logger::GetInstance()->Log("*** No Frame");
    }
  } // WaitForFrame

  int FrameBucket::GetFrameCount() const {
    return m_Frames.size();
  }

  bool FrameBucket::IsEmpty() const {
    return m_Frames.empty();
  }

}
