/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

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

namespace dss {

  const char* FunctionIDToString(const int _functionID); // internal forward declaration

  typedef hash_map<const Modulator*, Set> HashMapModulatorSet;

  HashMapModulatorSet splitByModulator(const Set& _set) {
    HashMapModulatorSet result;
    for(int iDevice = 0; iDevice < _set.length(); iDevice++) {
      const DeviceReference& dev = _set.get(iDevice);
      Modulator& mod = dev.getDevice().getApartment().getModulatorByBusID(dev.getDevice().getModulatorID());
      result[&mod].addDevice(dev);
    }
    return result;
  } // splitByModulator


  typedef map<const Zone*, Set> HashMapZoneSet;

  HashMapZoneSet splitByZone(const Set& _set) {
    HashMapZoneSet result;
    for(int iDevice = 0; iDevice < _set.length(); iDevice++) {
      const DeviceReference& devRef = _set.get(iDevice);
      const Device& dev = devRef.getDevice();
      Zone& zone = dev.getApartment().getZone(dev.getZoneID());
      result[&zone].addDevice(dev);
    }
    return result;
  } // splitByZone

  typedef std::pair<std::vector<Group*>, Set> FittingResultPerModulator;

  const bool OptimizerDebug = true;

  /** Precondition: _set contains only devices of _zone */
  FittingResultPerModulator bestFit(const Zone& _zone, const Set& _set) {
    Set workingCopy = _set;

    std::vector<Group*> fittingGroups;
    Set singleDevices;

    if(OptimizerDebug) {
      Logger::getInstance()->log("Finding fit for zone " + intToString(_zone.getZoneID()));
    }


	if(_zone.getDevices().length() == _set.length()) {
	  Logger::getInstance()->log(string("Optimization: Set contains all devices of zone ") + intToString(_zone.getZoneID()));
      std::bitset<63> possibleGroups;
      possibleGroups.set();
      for(int iDevice = 0; iDevice < _set.length(); iDevice++) {
        possibleGroups &= _set[iDevice].getDevice().getGroupBitmask();
	  }
	  if(possibleGroups.any()) {
        for(unsigned int iGroup = 0; iGroup < possibleGroups.size(); iGroup++) {
          if(possibleGroups.test(iGroup)) {
            Logger::getInstance()->log("Sending the command to group " + intToString(iGroup + 1));
            fittingGroups.push_back(_zone.getGroup(iGroup + 1));
            break;
          }
        }
      } else {
        Logger::getInstance()->log("Sending the command to broadcast group");
  	    fittingGroups.push_back(_zone.getGroup(GroupIDBroadcast));
  	  }
    } else {
	    std::vector<Group*> unsuitableGroups;
	    Set workingCopy = _set;

		while(!workingCopy.isEmpty()) {
		  DeviceReference& ref = workingCopy.get(0);
		  workingCopy.removeDevice(ref);

		  if(OptimizerDebug) {
		    Logger::getInstance()->log("Working with device " + ref.getDSID().toString());
		  }

		  bool foundGroup = false;
		  for(int iGroup = 0; iGroup < ref.getDevice().getGroupsCount(); iGroup++) {
			Group& g = ref.getDevice().getGroupByIndex(iGroup);

  		    if(OptimizerDebug) {
		      Logger::getInstance()->log("  Checking Group " + intToString(g.getID()));
		    }
			// continue if already found unsuitable
			if(find(unsuitableGroups.begin(), unsuitableGroups.end(), &g) != unsuitableGroups.end()) {
  		      if(OptimizerDebug) {
		        Logger::getInstance()->log("  Group discarded before, continuing search");
		      }
			  continue;
			}

			// see if we've got a fit
			bool groupFits = true;
			Set devicesInGroup = _zone.getDevices().getByGroup(g);
  		    if(OptimizerDebug) {
		      Logger::getInstance()->log("    Group has " + intToString(devicesInGroup.length()) + " devices");
		    }
			for(int iDevice = 0; iDevice < devicesInGroup.length(); iDevice++) {
			  if(!_set.contains(devicesInGroup.get(iDevice))) {
				unsuitableGroups.push_back(&g);
				groupFits = false;
   		        if(OptimizerDebug) {
		          Logger::getInstance()->log("    Original set does _not_ contain device " + devicesInGroup.get(iDevice).getDevice().getDSID().toString());
		        }
				break;
			  }
   		      if(OptimizerDebug) {
		        Logger::getInstance()->log("    Original set contains device " + devicesInGroup.get(iDevice).getDevice().getDSID().toString());
		      }
			}
			if(groupFits) {
  		      if(OptimizerDebug) {
		        Logger::getInstance()->log("  Found a fit " + intToString(g.getID()));
		      }
			  foundGroup = true;
			  fittingGroups.push_back(&g);
   		      if(OptimizerDebug) {
		        Logger::getInstance()->log("  Removing devices from working copy");
		      }
			  while(!devicesInGroup.isEmpty()) {
				workingCopy.removeDevice(devicesInGroup.get(0));
			    devicesInGroup.removeDevice(devicesInGroup.get(0));
			  }
   		      if(OptimizerDebug) {
		        Logger::getInstance()->log("  Done. (Removing devices from working copy)");
		      }
			  break;
			}
		  }

		  // if no fitting group found
		  if(!foundGroup) {
			singleDevices.addDevice(ref);
		  }
		}
	}
    return FittingResultPerModulator(fittingGroups, singleDevices);
  }

  FittingResultPerModulator bestFit(const Modulator& _modulator, const Set& _set) {
    Set workingCopy = _set;

    std::vector<Group*> unsuitableGroups;
    std::vector<Group*> fittingGroups;
    Set singleDevices;

    while(!workingCopy.isEmpty()) {
      DeviceReference& ref = workingCopy.get(0);
      workingCopy.removeDevice(ref);

      bool foundGroup = false;
      for(int iGroup = 0; iGroup < ref.getDevice().getGroupsCount(); iGroup++) {
        Group& g = ref.getDevice().getGroupByIndex(iGroup);

        // continue if already found unsuitable
        if(find(unsuitableGroups.begin(), unsuitableGroups.end(), &g) != unsuitableGroups.end()) {
          continue;
        }

        // see if we've got a fit
        bool groupFits = true;
        Set devicesInGroup = _modulator.getDevices().getByGroup(g);
        for(int iDevice = 0; iDevice < devicesInGroup.length(); iDevice++) {
          if(!_set.contains(devicesInGroup.get(iDevice))) {
            unsuitableGroups.push_back(&g);
            groupFits = false;
            break;
          }
        }
        if(groupFits) {
          foundGroup = true;
          fittingGroups.push_back(&g);
          while(!devicesInGroup.isEmpty()) {
            workingCopy.removeDevice(devicesInGroup.get(0));
          }
          break;
        }
      }

      // if no fitting group found
      if(!foundGroup) {
        singleDevices.addDevice(ref);
      }
    }
    return FittingResultPerModulator(fittingGroups, singleDevices);
  }

  DS485Proxy::DS485Proxy(DSS* _pDSS)
  : Thread("DS485Proxy"),
    Subsystem(_pDSS, "DS485Proxy")
  {
    if(_pDSS != NULL) {
      _pDSS->getPropertySystem().createProperty(getConfigPropertyBasePath() + "rs485devicename")
            ->linkToProxy(PropertyProxyMemberFunction<DS485Controller, std::string>(m_DS485Controller, &DS485Controller::getRS485DeviceName, &DS485Controller::setRS485DeviceName));

      _pDSS->getPropertySystem().createProperty(getPropertyBasePath() + "tokensReceived")
            ->linkToProxy(PropertyProxyMemberFunction<DS485Controller, int>(m_DS485Controller, &DS485Controller::getTokenCount));

      const DS485FrameReader& reader = m_DS485Controller.getFrameReader();
      _pDSS->getPropertySystem().createProperty(getPropertyBasePath() + "framesReceived")
            ->linkToProxy(PropertyProxyMemberFunction<DS485FrameReader, int>(reader, &DS485FrameReader::getNumberOfFramesReceived));

      _pDSS->getPropertySystem().createProperty(getPropertyBasePath() + "incompleteFramesReceived")
            ->linkToProxy(PropertyProxyMemberFunction<DS485FrameReader, int>(reader, &DS485FrameReader::getNumberOfIncompleteFramesReceived));

      _pDSS->getPropertySystem().createProperty(getPropertyBasePath() + "crcErrors")
            ->linkToProxy(PropertyProxyMemberFunction<DS485FrameReader, int>(reader, &DS485FrameReader::getNumberOfCRCErrors));

      _pDSS->getPropertySystem().createProperty(getPropertyBasePath() + "state")
            ->linkToProxy(PropertyProxyMemberFunction<DS485Controller, std::string>(m_DS485Controller, &DS485Controller::getStateAsString));

      _pDSS->getPropertySystem().setStringValue(getConfigPropertyBasePath() + "dsid", "3504175FE0000000DEADBEEF", true, false);
    }
  } // ctor

  bool DS485Proxy::isReady() {
	  return isRunning()
	         && ((m_DS485Controller.getState() == csSlave) ||
	             (m_DS485Controller.getState() == csDesignatedMaster) ||
	             (m_DS485Controller.getState() == csError)) // allow the simulation to run on it's own
	         &&  DSS::getInstance()->getSimulation().isReady();
  } // isReady

  FittingResult DS485Proxy::bestFit(const Set& _set) {
    FittingResult result;
    HashMapZoneSet zoneToSet = splitByZone(_set);

    for(HashMapZoneSet::iterator it = zoneToSet.begin(); it != zoneToSet.end(); ++it) {
      result[it->first] = dss::bestFit(*(it->first), it->second);
    }

    return result;
  } // bestFit(const Set&)

  std::vector<int> DS485Proxy::sendCommand(DS485Command _cmd, const Set& _set, int _param) {
    if(_set.length() == 1) {
      log("Optimization: Set contains only one device");
      return sendCommand(_cmd, _set.get(0).getDevice(), _param);
    } else if(_set.length() > 0) {
      Apartment& apt = _set.get(0).getDevice().getApartment();
      if(_set.length() == apt.getDevices().length()) {
        log("Optimization: Set contains all devices of apartment");
        return sendCommand(_cmd, apt.getZone(0), apt.getGroup(GroupIDBroadcast), _param);
      }
    }

    std::vector<int> result;
    FittingResult fittedResult = bestFit(_set);
    for(FittingResult::iterator iResult = fittedResult.begin(); iResult != fittedResult.end(); ++iResult) {
      const Zone* zone = iResult->first;
      FittingResultPerModulator res = iResult->second;
      std::vector<Group*> groups = res.first;
      for(vector<Group*>::iterator ipGroup = groups.begin(); ipGroup != groups.end(); ++ipGroup) {
        sendCommand(_cmd, *zone, **ipGroup, _param);
      }
      Set& set = res.second;
      for(int iDevice = 0; iDevice < set.length(); iDevice++) {
        sendCommand(_cmd, set.get(iDevice).getDevice(), _param);
      }
    }
    return result;
  } // sendCommand

  std::vector<int> DS485Proxy::sendCommand(DS485Command _cmd, const Zone& _zone, uint8_t _groupID, int _param) {
    std::vector<int> result;

    DS485CommandFrame frame;
    frame.getHeader().setDestination(0);
    frame.getHeader().setBroadcast(true);
    frame.getHeader().setType(1);
    frame.setCommand(CommandRequest);
    int toZone;
    if(getDSS().getApartment().getZones().size() == 2 /* zone 0 + another zone */) {
      toZone = 0;
      log("sendCommand(Zone,GroupID): Only one zone present, sending frame to broadcast zone");
    } else {
      toZone = _zone.getZoneID();
    }
    if(_cmd == cmdTurnOn) {
      frame.getPayload().add<uint8_t>(FunctionGroupCallScene);
      frame.getPayload().add<uint16_t>(toZone);
      frame.getPayload().add<uint16_t>(_groupID);
      frame.getPayload().add<uint16_t>(SceneMax);
      sendFrame(frame);
      log("turn on: zone " + intToString(_zone.getZoneID()) + " group: " + intToString(_groupID));
    } else if(_cmd == cmdTurnOff) {
      frame.getPayload().add<uint8_t>(FunctionGroupCallScene);
      frame.getPayload().add<uint16_t>(toZone);
      frame.getPayload().add<uint16_t>(_groupID);
      frame.getPayload().add<uint16_t>(SceneMin);
      sendFrame(frame);
      log("turn off: zone " + intToString(_zone.getZoneID()) + " group: " + intToString(_groupID));
    } else if(_cmd == cmdCallScene) {
      frame.getPayload().add<uint8_t>(FunctionGroupCallScene);
      frame.getPayload().add<uint16_t>(toZone);
      frame.getPayload().add<uint16_t>(_groupID);
      frame.getPayload().add<uint16_t>(_param);
      sendFrame(frame);
      log("call scene: zone " + intToString(_zone.getZoneID()) + " group: " + intToString(_groupID));
    } else if(_cmd == cmdSaveScene) {
      frame.getPayload().add<uint8_t>(FunctionGroupSaveScene);
      frame.getPayload().add<uint16_t>(toZone);
      frame.getPayload().add<uint16_t>(_groupID);
      frame.getPayload().add<uint16_t>(_param);
      sendFrame(frame);
    } else if(_cmd == cmdUndoScene) {
      frame.getPayload().add<uint8_t>(FunctionGroupUndoScene);
      frame.getPayload().add<uint16_t>(toZone);
      frame.getPayload().add<uint16_t>(_groupID);
      frame.getPayload().add<uint16_t>(_param);
      sendFrame(frame);
    } else if(_cmd == cmdStartDimUp) {
      frame.getPayload().add<uint8_t>(FunctionGroupStartDimInc);
      frame.getPayload().add<uint16_t>(toZone);
      frame.getPayload().add<uint16_t>(_groupID);
      sendFrame(frame);
    } else if(_cmd == cmdStartDimDown) {
      frame.getPayload().add<uint8_t>(FunctionGroupStartDimDec);
      frame.getPayload().add<uint16_t>(toZone);
      frame.getPayload().add<uint16_t>(_groupID);
      sendFrame(frame);
    } else if(_cmd == cmdStopDim) {
      frame.getPayload().add<uint8_t>(FunctionGroupEndDim);
      frame.getPayload().add<uint16_t>(toZone);
      frame.getPayload().add<uint16_t>(_groupID);
      sendFrame(frame);
    } else if(_cmd == cmdIncreaseValue) {
      frame.getPayload().add<uint8_t>(FunctionGroupIncreaseValue);
      frame.getPayload().add<uint16_t>(toZone);
      frame.getPayload().add<uint16_t>(_groupID);
      sendFrame(frame);
    } else if(_cmd == cmdDecreaseValue) {
      frame.getPayload().add<uint8_t>(FunctionGroupDecreaseValue);
      frame.getPayload().add<uint16_t>(toZone);
      frame.getPayload().add<uint16_t>(_groupID);
      sendFrame(frame);
    } else if(_cmd == cmdSetValue) {
      frame.getPayload().add<uint8_t>(FunctionGroupSetValue);
      frame.getPayload().add<devid_t>(toZone);
      frame.getPayload().add<devid_t>(_groupID);
      frame.getPayload().add<devid_t>(_param);
      sendFrame(frame);
    }
    return result;
  } // sendCommand(zone, group)

  std::vector<int> DS485Proxy::sendCommand(DS485Command _cmd, const Zone& _zone, Group& _group, int _param) {
    return sendCommand(_cmd, _zone, _group.getID(), _param);
  } // sendCommand

  std::vector<int> DS485Proxy::sendCommand(DS485Command _cmd, const Device& _device, int _param) {
    return sendCommand(_cmd, _device.getShortAddress(), _device.getModulatorID(), _param);
  } // sendCommand

  std::vector<int> DS485Proxy::sendCommand(DS485Command _cmd, devid_t _id, uint8_t _modulatorID, int _param) {
    std::vector<int> result;
    DS485CommandFrame frame;
    frame.getHeader().setDestination(_modulatorID);
    frame.getHeader().setBroadcast(false);
    frame.getHeader().setType(1);
    frame.setCommand(CommandRequest);
    if(_cmd == cmdTurnOn) {
      frame.getPayload().add<uint8_t>(FunctionDeviceCallScene);
      frame.getPayload().add<devid_t>(_id);
      frame.getPayload().add<uint16_t>(SceneMax);
      sendFrame(frame);
    } else if(_cmd == cmdTurnOff) {
      frame.getPayload().add<uint8_t>(FunctionDeviceCallScene);
      frame.getPayload().add<devid_t>(_id);
      frame.getPayload().add<uint16_t>(SceneMin);
      sendFrame(frame);
    } else if(_cmd == cmdGetOnOff) {
      frame.getPayload().add<uint8_t>(FunctionDeviceGetOnOff);
      frame.getPayload().add<uint16_t>(_id);
      uint8_t res = receiveSingleResult(frame, FunctionDeviceGetOnOff);
      result.push_back(res);
    } else if(_cmd == cmdGetValue) {
      frame.getPayload().add<uint8_t>(FunctionDeviceGetParameterValue);
      frame.getPayload().add<uint16_t>(_id);
      frame.getPayload().add<uint16_t>(_param);
      uint8_t res = receiveSingleResult(frame, FunctionDeviceGetParameterValue);
      result.push_back(res);
    } else if(_cmd == cmdGetFunctionID) {
      frame.getPayload().add<uint8_t>(FunctionDeviceGetFunctionID);
      frame.getPayload().add<devid_t>(_id);

      boost::shared_ptr<ReceivedFrame> resFrame = receiveSingleFrame(frame, FunctionDeviceGetFunctionID);
      if(resFrame.get() != NULL) {
        PayloadDissector pd(resFrame->getFrame()->getPayload());
        pd.get<uint8_t>(); // skip the function id
        if(pd.get<uint16_t>() == 0x0001) {
          result.push_back(pd.get<uint16_t>());
        }
      }
    } else if(_cmd == cmdCallScene) {
      frame.getPayload().add<uint8_t>(FunctionDeviceCallScene);
      frame.getPayload().add<devid_t>(_id);
      frame.getPayload().add<uint16_t>(_param);
      sendFrame(frame);
    } else if(_cmd == cmdSaveScene) {
      frame.getPayload().add<uint8_t>(FunctionDeviceSaveScene);
      frame.getPayload().add<devid_t>(_id);
      frame.getPayload().add<uint16_t>(_param);
      sendFrame(frame);
    } else if(_cmd == cmdUndoScene) {
      frame.getPayload().add<uint8_t>(FunctionDeviceUndoScene);
      frame.getPayload().add<devid_t>(_id);
      frame.getPayload().add<uint16_t>(_param);
      sendFrame(frame);
    } else if(_cmd == cmdIncreaseValue) {
      frame.getPayload().add<uint8_t>(FunctionDeviceIncreaseValue);
      frame.getPayload().add<devid_t>(_id);
      sendFrame(frame);
    } else if(_cmd == cmdDecreaseValue) {
      frame.getPayload().add<uint8_t>(FunctionDeviceDecreaseValue);
      frame.getPayload().add<devid_t>(_id);
      sendFrame(frame);
    } else if(_cmd == cmdSetValue) {
      frame.getPayload().add<uint8_t>(FunctionDeviceSetValue);
      frame.getPayload().add<devid_t>(_id);
      frame.getPayload().add<devid_t>(_param);
      sendFrame(frame);
    }
    return result;
  } // sendCommand(device)

  void DS485Proxy::sendFrame(DS485CommandFrame& _frame) {
    bool broadcast = _frame.getHeader().isBroadcast();
    bool sim = isSimAddress(_frame.getHeader().getDestination());
    _frame.setFrameSource(fsDSS);
    if(broadcast || sim) {
      log("Sending packet to sim");
      if(DSS::hasInstance()) {
        getDSS().getSimulation().process(_frame);
      }
    }
    if(broadcast || !sim) {
      if((m_DS485Controller.getState() == csSlave) || (m_DS485Controller.getState() == csMaster)) {
        log("Sending packet to hardware");
       	m_DS485Controller.enqueueFrame(_frame);
      }
    }
    std::ostringstream sstream;
    sstream << "Frame content: ";
    PayloadDissector pd(_frame.getPayload());
    while(!pd.isEmpty()) {
      uint8_t data = pd.get<uint8_t>();
      sstream << "(0x" << std::hex << (unsigned int)data << ", " << std::dec << (int)data << "d)";
    }
    sstream << std::dec;
    sstream << " to " << int(_frame.getHeader().getDestination());
    if(broadcast) {
      sstream << " as broadcast";
    }
    log(sstream.str());

    boost::shared_ptr<DS485CommandFrame> pFrame(new DS485CommandFrame);
    *pFrame = _frame;
    // relay the frame to update our state
    collectFrame(pFrame);
  } // sendFrame

  boost::shared_ptr<FrameBucketCollector> DS485Proxy::sendFrameAndInstallBucket(DS485CommandFrame& _frame, const int _functionID) {
    int sourceID = _frame.getHeader().isBroadcast() ? -1 :  _frame.getHeader().getDestination();
    boost::shared_ptr<FrameBucketCollector> result(new FrameBucketCollector(this, _functionID, sourceID));
    sendFrame(_frame);
    return result;
  }

  bool DS485Proxy::isSimAddress(const uint8_t _addr) {
    if(DSS::hasInstance()) {
      return getDSS().getSimulation().isSimAddress(_addr);
    } else {
      return true;
    }
  } // isSimAddress

  void DS485Proxy::setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size) {
    DS485CommandFrame frame;
    frame.getHeader().setDestination(_device.getModulatorID());
    frame.getHeader().setBroadcast(false);
    frame.getHeader().setType(1);
    frame.setCommand(CommandRequest);
    frame.getPayload().add<uint8_t>(FunctionDeviceSetParameterValue);
    frame.getPayload().add<uint16_t>(_device.getShortAddress());
    frame.getPayload().add<uint16_t>(_parameterID);
    frame.getPayload().add<uint16_t>(_size - 1);
    frame.getPayload().add<uint16_t>(_value);
    sendFrame(frame);
  } // setValueDevice
  
  ModulatorSpec_t DS485Proxy::modulatorSpecFromFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
    int source = _frame->getHeader().getSource();

    PayloadDissector pd(_frame->getPayload());
    pd.get<uint8_t>();
    uint16_t devID = pd.get<uint16_t>();
    devID =  devID >> 8;
    if(devID == 0) {
      log("Found dSS");
    } else if(devID == 1) {
      log("Found dSM");
    } else {
      log(string("Found unknown device (") + intToString(devID, true) + ")");
    }
    uint16_t hwVersion = (pd.get<uint8_t>() << 8) | pd.get<uint8_t>();
    uint16_t swVersion = (pd.get<uint8_t>() << 8) | pd.get<uint8_t>();

    log(string("  HW-Version: ") + intToString(hwVersion >> 8) + "." + intToString(hwVersion & 0x00FF));
    log(string("  SW-Version: ") + intToString(swVersion >> 8) + "." + intToString(swVersion & 0x00FF));

    std::string name;
    for(int i = 0; i < 6; i++) {
      char c = static_cast<char>(pd.get<uint8_t>());
      if(c != '\0') {
        name += c;
      }
    }
    log(string("  Name:      \"") + name + "\"");

    // bus-id, sw-version, hw-version, name, device-id
    ModulatorSpec_t spec(source, swVersion, hwVersion, name, devID);
    return spec;
  } // modulatorSpecFromFrame

  std::vector<ModulatorSpec_t> DS485Proxy::getModulators() {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(0);
    cmdFrame.getHeader().setBroadcast(true);
    cmdFrame.setCommand(CommandRequest);
    log("Proxy: GetModulators");

    cmdFrame.getPayload().add<uint8_t>(FunctionGetTypeRequest);
    boost::shared_ptr<FrameBucketCollector> bucket = sendFrameAndInstallBucket(cmdFrame, FunctionGetTypeRequest);
    bucket->waitForFrames(1000);

    map<int, bool> resultFrom;

    std::vector<ModulatorSpec_t> result;
    while(true) {
      boost::shared_ptr<ReceivedFrame> recFrame = bucket->popFrame();
      if(recFrame.get() == NULL) {
        break;
      }
      int source = recFrame->getFrame()->getHeader().getSource();
      if(resultFrom[source]) {
        log(string("already received result from ") + intToString(source));
        continue;
      }
      ModulatorSpec_t spec = modulatorSpecFromFrame(recFrame->getFrame());
      result.push_back(spec);
    }

    return result;
  } // getModulators

  ModulatorSpec_t DS485Proxy::getModulatorSpec(const int _modulatorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.getHeader().setBroadcast(true);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionGetTypeRequest);
    log("Proxy: getModulatorSpec");

    boost::shared_ptr<ReceivedFrame> recFrame = receiveSingleFrame(cmdFrame, FunctionGetTypeRequest);

    if(recFrame.get() == NULL) {
      throw new runtime_error("No frame received");
    }

    ModulatorSpec_t result = modulatorSpecFromFrame(recFrame->getFrame());

    return result;
  } // getModulatorSpec

  int DS485Proxy::getGroupCount(const int _modulatorID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionModulatorGetGroupsSize);
    cmdFrame.getPayload().add<uint16_t>(_zoneID);
    int8_t res = static_cast<int8_t>(receiveSingleResult(cmdFrame, FunctionModulatorGetGroupsSize));
    if(res < 0) {
      log("GetGroupCount: Negative group count received '" + intToString(res) +
          " on modulator " + intToString(_modulatorID) +
          " with zone " + intToString(_zoneID));
      res = 0;
    }
    return res;
  } // getGroupCount

  std::vector<int> DS485Proxy::getGroups(const int _modulatorID, const int _zoneID) {
    std::vector<int> result;

    int numGroups = getGroupCount(_modulatorID, _zoneID);
    log(string("Modulator has ") + intToString(numGroups) + " groups");
    for(int iGroup = 0; iGroup < numGroups; iGroup++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.getHeader().setDestination(_modulatorID);
      cmdFrame.setCommand(CommandRequest);
      cmdFrame.getPayload().add<uint8_t>(FunctionZoneGetGroupIdForInd);
      cmdFrame.getPayload().add<uint16_t>(_zoneID);
      cmdFrame.getPayload().add<uint16_t>(iGroup);

      int8_t res = static_cast<int8_t>(receiveSingleResult(cmdFrame, FunctionZoneGetGroupIdForInd));
      if(res < 0) {
        log("GetGroups: Negative index received '" + intToString(res) + "' for index " + intToString(iGroup));
      } else {
        result.push_back(res);
      }
    }

    return result;
  } // getGroups

  int DS485Proxy::getDevicesInGroupCount(const int _modulatorID, const int _zoneID, const int _groupID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionGroupGetDeviceCount);
    cmdFrame.getPayload().add<uint16_t>(_zoneID);
    cmdFrame.getPayload().add<uint16_t>(_groupID);

    int16_t res = static_cast<int16_t>(receiveSingleResult16(cmdFrame, FunctionGroupGetDeviceCount));
    if(res < 0) {
      log("GetDevicesInGroupCount: Negative count received '" + intToString(res) +
          "' on modulator " + intToString(_modulatorID) +
          " with zoneID " + intToString(_zoneID) + " in group " + intToString(_groupID));
      res = 0;
    }
    return res;
  } // getDevicesInGroupCount

  std::vector<int> DS485Proxy::getDevicesInGroup(const int _modulatorID, const int _zoneID, const int _groupID) {
    std::vector<int> result;

    int numDevices = getDevicesInGroupCount(_modulatorID, _zoneID, _groupID);
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.getHeader().setDestination(_modulatorID);
      cmdFrame.setCommand(CommandRequest);
      cmdFrame.getPayload().add<uint8_t>(FunctionGroupGetDevKeyForInd);
      cmdFrame.getPayload().add<uint16_t>(_zoneID);
      cmdFrame.getPayload().add<uint16_t>(_groupID);
      cmdFrame.getPayload().add<uint16_t>(iDevice);
      int16_t res = static_cast<int16_t>(receiveSingleResult16(cmdFrame, FunctionGroupGetDevKeyForInd));
      if(res < 0) {
        log("GetDevicesInGroup: Negative device id received '" + intToString(res) + "' for index " + intToString(iDevice));
      } else {
        result.push_back(res);
      }
    }

    return result;
  } // getDevicesInGroup

  std::vector<int> DS485Proxy::getGroupsOfDevice(const int _modulatorID, const int _deviceID) {
    std::vector<int> result;

    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDeviceGetGroups);
    cmdFrame.getPayload().add<uint16_t>(_deviceID);

    boost::shared_ptr<FrameBucketCollector> bucket = sendFrameAndInstallBucket(cmdFrame, FunctionDeviceGetGroups);

    bucket->waitForFrame(1000);

    while(true) {
      boost::shared_ptr<ReceivedFrame> recFrame = bucket->popFrame();
      if(recFrame.get() == NULL) {
        break;
      }

      PayloadDissector pd(recFrame->getFrame()->getPayload());
      pd.get<uint8_t>(); // discard the function id
      pd.get<uint16_t>(); // function result

      for(int iByte = 0; iByte < 8; iByte++) {
        uint8_t byte = pd.get<uint8_t>();
        for(int iBit = 0; iBit < 8; iBit++) {
          if(byte & (1 << iBit)) {
            result.push_back((iByte * 8 + iBit) + 1);
          }
        }
      }
    }
    return result;
  } // getGroupsOfDevice

  std::vector<int> DS485Proxy::getZones(const int _modulatorID) {
    std::vector<int> result;

    int numZones = getZoneCount(_modulatorID);
    log(string("Modulator has ") + intToString(numZones) + " zones");
    for(int iZone = 0; iZone < numZones; iZone++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.getHeader().setDestination(_modulatorID);
      cmdFrame.setCommand(CommandRequest);
      cmdFrame.getPayload().add<uint8_t>(FunctionModulatorGetZoneIdForInd);
      cmdFrame.getPayload().add<uint16_t>(iZone);
      log("GetZoneID");
      int16_t tempResult = static_cast<int16_t>(receiveSingleResult16(cmdFrame, FunctionModulatorGetZoneIdForInd));
      if(tempResult < 0) {
        log("GetZones: Negative zone id " + intToString(tempResult) + " received. Modulator: " + intToString(_modulatorID) + " index: " + intToString(iZone), lsError);
      } else {
        result.push_back(tempResult);
      }
      log("receive ZoneID: " + uintToString((unsigned int)tempResult));
    }
    return result;
  } // getZones

  int DS485Proxy::getZoneCount(const int _modulatorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionModulatorGetZonesSize);
    log("GetZoneCount");

    // TODO: check result-code
    uint8_t result = receiveSingleResult(cmdFrame, FunctionModulatorGetZonesSize);
    return result;
  } // getZoneCount

  int DS485Proxy::getDevicesCountInZone(const int _modulatorID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionModulatorCountDevInZone);
    cmdFrame.getPayload().add<uint16_t>(_zoneID);
    log("GetDevicesCountInZone");

    log(intToString(_modulatorID) + " " + intToString(_zoneID));

    int16_t result = static_cast<int16_t>(receiveSingleResult16(cmdFrame, FunctionModulatorCountDevInZone));
    if(result < 0) {
      log("GetDevicesCountInZone: negative count '" + intToString(result) + "'", lsError);
      result = 0;
    }

    return result;
  } // getDevicesCountInZone

  std::vector<int> DS485Proxy::getDevicesInZone(const int _modulatorID, const int _zoneID) {
    std::vector<int> result;

    int numDevices = getDevicesCountInZone(_modulatorID, _zoneID);
    log(string("Found ") + intToString(numDevices) + " in zone.");
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.getHeader().setDestination(_modulatorID);
      cmdFrame.setCommand(CommandRequest);
      cmdFrame.getPayload().add<uint8_t>(FunctionModulatorDevKeyInZone);
      cmdFrame.getPayload().add<uint16_t>(_zoneID);
      cmdFrame.getPayload().add<uint16_t>(iDevice);

      result.push_back(receiveSingleResult16(cmdFrame, FunctionModulatorDevKeyInZone));
    }
    return result;
  } // getDevicesInZone

  void DS485Proxy::setZoneID(const int _modulatorID, const devid_t _deviceID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDeviceSetZoneID);
    cmdFrame.getPayload().add<devid_t>(_deviceID);
    cmdFrame.getPayload().add<uint16_t>(_zoneID);

    // TODO: check result-code
    receiveSingleResult(cmdFrame, FunctionDeviceSetZoneID);
  } // setZoneID

  void DS485Proxy::createZone(const int _modulatorID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionModulatorAddZone);
    cmdFrame.getPayload().add<uint16_t>(_zoneID);

    // TODO: check result-code
    receiveSingleResult(cmdFrame, FunctionModulatorAddZone);
  } // createZone

  void DS485Proxy::removeZone(const int _modulatorID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionModulatorRemoveZone);
    cmdFrame.getPayload().add<uint16_t>(_zoneID);

    // TODO: check result-code
    receiveSingleResult(cmdFrame, FunctionModulatorAddZone);
  } // removeZone

  dsid_t DS485Proxy::getDSIDOfDevice(const int _modulatorID, const int _deviceID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDeviceGetDSID);
    cmdFrame.getPayload().add<uint16_t>(_deviceID);
    log("Proxy: GetDSIDOfDevice");

    boost::shared_ptr<ReceivedFrame> recFrame = receiveSingleFrame(cmdFrame, FunctionDeviceGetDSID);
    if(recFrame.get() == NULL) {
      return NullDSID;
    }

    PayloadDissector pd(recFrame->getFrame()->getPayload());
    pd.get<uint8_t>(); // discard the function id
    pd.get<uint16_t>(); // function result
    return pd.get<dsid_t>();
  }

  dsid_t DS485Proxy::getDSIDOfModulator(const int _modulatorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionModulatorGetDSID);
    log(string("Proxy: GetDSIDOfModulator ") + intToString(_modulatorID));

    boost::shared_ptr<ReceivedFrame> recFrame = receiveSingleFrame(cmdFrame, FunctionModulatorGetDSID);
    if(recFrame.get() == NULL) {
      log("GetDSIDOfModulator: received no result from " + intToString(_modulatorID), lsError);
      return NullDSID;
    }

    PayloadDissector pd(recFrame->getFrame()->getPayload());
    pd.get<uint8_t>(); // discard the function id
    //pd.get<uint8_t>(); // function result, don't know if that's sent though
    return pd.get<dsid_t>();
  } // getDSIDOfModulator

  int DS485Proxy::getLastCalledScene(const int _modulatorID, const int _zoneID, const int _groupID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionGroupGetLastCalledScene);
    log(string("Proxy: GetLastCalledScene ") + intToString(_modulatorID));
    cmdFrame.getPayload().add<uint16_t>(_zoneID);
    cmdFrame.getPayload().add<uint16_t>(_groupID);

    int16_t res = static_cast<int16_t>(receiveSingleResult16(cmdFrame, FunctionGroupGetLastCalledScene));
    if(res < 0) {
      log("DS485Proxy::getLastCalledScene: negative result received: " + intToString(res));
    }
    return res;
  } // getLastCalledScene

  unsigned long DS485Proxy::getPowerConsumption(const int _modulatorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionModulatorGetPowerConsumption);
    log(string("Proxy: GetPowerConsumption ") + intToString(_modulatorID));

    boost::shared_ptr<ReceivedFrame> recFrame = receiveSingleFrame(cmdFrame, FunctionModulatorGetPowerConsumption);
    if(recFrame.get() == NULL) {
      log("DS485Proxy::getPowerConsumption: received no results", lsError);
      return 0;
    }
    if(recFrame->getFrame()->getHeader().getSource() != _modulatorID) {
      log("GetPowerConsumption: received result from wrong source");
    }
    PayloadDissector pd(recFrame->getFrame()->getPayload());
    pd.get<uint8_t>(); // discard the function id
    return pd.get<uint32_t>();
  } // getPowerConsumption

  unsigned long DS485Proxy::getEnergyMeterValue(const int _modulatorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionModulatorGetEnergyMeterValue);
    log(string("Proxy: GetEnergyMeterValue ") + intToString(_modulatorID));

    boost::shared_ptr<ReceivedFrame> recFrame = receiveSingleFrame(cmdFrame, FunctionModulatorGetEnergyMeterValue);
    if(recFrame.get() == NULL) {
      log("DS485Proxy::getEnergyMeterValue: received no results", lsError);
      return 0;
    }
    PayloadDissector pd(recFrame->getFrame()->getPayload());
    pd.get<uint8_t>(); // discard the function id
    return pd.get<uint16_t>();
  } // getEnergyMeterValue

  bool DS485Proxy::getEnergyBorder(const int _modulatorID, int& _lower, int& _upper) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionModulatorGetEnergyBorder);

    boost::shared_ptr<FrameBucketCollector> bucket = sendFrameAndInstallBucket(cmdFrame, FunctionModulatorGetEnergyBorder);

    bucket->waitForFrame(1000);

    boost::shared_ptr<ReceivedFrame> recFrame = bucket->popFrame();
    if(recFrame.get() == NULL) {
      return false;
    }

    PayloadDissector pd(recFrame->getFrame()->getPayload());
    pd.get<uint8_t>(); // discard the function id
    _lower = pd.get<uint16_t>();
    _upper = pd.get<uint16_t>();
    return true;
  } // getEnergyBorder

  uint8_t DS485Proxy::dSLinkSend(const int _modulatorID, devid_t _devAdr, uint8_t _value, uint8_t _flags) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_modulatorID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSLinkSendDevice);
    cmdFrame.getPayload().add<uint16_t>(_devAdr);
    cmdFrame.getPayload().add<uint16_t>(_value);
    cmdFrame.getPayload().add<uint16_t>(_flags);

    if((_flags & DSLinkSendWriteOnly) == 0) {
      boost::shared_ptr<FrameBucketCollector> bucket = sendFrameAndInstallBucket(cmdFrame, FunctionDSLinkReceive);
      bucket->waitForFrame(10000);
      boost::shared_ptr<ReceivedFrame> recFrame = bucket->popFrame();
      if(recFrame.get() == NULL) {
        log("dsLinkSend: No packet received");
        return 0;
      }
      PayloadDissector pd(recFrame->getFrame()->getPayload());
      pd.get<uint8_t>(); // discard the function id
      pd.get<uint16_t>(); // garbage
      devid_t devAddress = pd.get<uint16_t>(); // device address
      if(devAddress != _devAdr) {
        log("dSLinkSend: Received answer for wrong device expected: " 
            + intToString(_devAdr, true) + 
            " got: " + intToString(devAddress, true));
      }
      return pd.get<uint16_t>();
    }
    log("dsLinkSend: Not waiting for response (waitOnly is set)");
    return 0;
  } // dsLinkSend

  void DS485Proxy::addToGroup(const int _modulatorID, const int _groupID, const int _deviceID) {

  } // addToGroup

  void DS485Proxy::removeFromGroup(const int _modulatorID, const int _groupID, const int _deviceID) {

  } // removeFromGroup

  int DS485Proxy::addUserGroup(const int _modulatorID) {
    return 0;
  } // addUserGroup

  void DS485Proxy::removeUserGroup(const int _modulatorID, const int _groupID) {

  } // removeUserGroup

  boost::shared_ptr<ReceivedFrame> DS485Proxy::receiveSingleFrame(DS485CommandFrame& _frame, uint8_t _functionID) {
    boost::shared_ptr<FrameBucketCollector> bucket = sendFrameAndInstallBucket(_frame, _functionID);
    bucket->waitForFrame(1000);

    if(bucket->isEmpty()) {
      log(string("received no results for request (") + FunctionIDToString(_functionID) + ")");
      return boost::shared_ptr<ReceivedFrame>();
    } else if(bucket->getFrameCount() > 1) {
      log(string("received multiple results (") + intToString(bucket->getFrameCount()) + ") for request (" + FunctionIDToString(_functionID) + ")");
      // TODO: check
      return bucket->popFrame();
    }

    boost::shared_ptr<ReceivedFrame> recFrame = bucket->popFrame();

    if(recFrame.get() != NULL) {
      return recFrame;
    } else {
      throw runtime_error("received frame is NULL but bucket->isEmpty() returns false");
    }
  } // receiveSingleFrame

  uint8_t DS485Proxy::receiveSingleResult(DS485CommandFrame& _frame, const uint8_t _functionID) {
    boost::shared_ptr<ReceivedFrame> recFrame = receiveSingleFrame(_frame, _functionID);

    if(recFrame.get() != NULL) {
      PayloadDissector pd(recFrame->getFrame()->getPayload());
      uint8_t functionID = pd.get<uint8_t>();
      if(functionID != _functionID) {
        log("function ids are different", lsFatal);
      }
      uint8_t result = pd.get<uint8_t>();
      return result;
    } else {
      return 0;
    }
  } // receiveSingleResult

  uint16_t DS485Proxy::receiveSingleResult16(DS485CommandFrame& _frame, const uint8_t _functionID) {
    boost::shared_ptr<ReceivedFrame> recFrame = receiveSingleFrame(_frame, _functionID);

    if(recFrame.get() != NULL) {
      PayloadDissector pd(recFrame->getFrame()->getPayload());
      uint8_t functionID = pd.get<uint8_t>();
      if(functionID != _functionID) {
        log("function ids are different");
      }
      uint16_t result = pd.get<uint8_t>();
      if(!pd.isEmpty()) {
        result |= (pd.get<uint8_t>() << 8);
      } else {
        log("receiveSingleResult16: only received half of the data (8bit)", lsFatal);
      }
      return result;
    } else {
      return 0;
    }
  } // receiveSingleResult16

  void DS485Proxy::initialize() {
    Subsystem::initialize();
    m_DS485Controller.addFrameCollector(this);
    if(DSS::hasInstance()) {
      getDSS().getSimulation().addFrameCollector(this);
    }
  }

  void DS485Proxy::doStart() {
    try {
      m_DS485Controller.setDSID(dsid_t::fromString(getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "dsid")));
      m_DS485Controller.run();
    } catch (const runtime_error& _ex) {
    	log(string("Caught exception while starting DS485Controlle: ") + _ex.what(), lsFatal);
    }
    // call Thread::run()
    run();
  } // doStart

  void DS485Proxy::waitForProxyEvent() {
    m_ProxyEvent.waitFor();
  } // waitForProxyEvent

  void DS485Proxy::signalEvent() {
    m_ProxyEvent.signal();
  } // signalEvent

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

    case FunctionDeviceGetFunctionID:
      return "Function Device Get Function ID";
    case FunctionDSLinkConfigWrite:
      return "Function dSLink Config Write";
    case FunctionDSLinkConfigRead:
      return "Function dSLink Config Read";
    case FunctionDSLinkSendDevice:
      return "Function dSLink Send Device";
    case FunctionDSLinkSendGroup:
      return "Function dSLink Send Group";
    case FunctionDSLinkReceive:
      return "Function dSLink Receive";
    case FunctionDSLinkInterrupt:
      return "Function DSLink Interrupt";
      
    case FunctionZoneAddDevice:
      return "Function Zone Add Device";
    case FunctionZoneRemoveDevice:
      return "Function Zone Remove Device";
    case FunctionDeviceAddToGroup:
      return "Function Device Add To Group";
    case EventNewDS485Device:
      return "Event New DS485 Device";
    case EventLostDS485Device:
      return "Event Lost DS485 Device";
    case EventDeviceReady:
      return "Event Device Ready";    
    }
    return "";
  } // functionIDToString

  void DS485Proxy::execute() {
    signalEvent();
    
    aControllerState lastState = m_DS485Controller.getState();

    while(!m_Terminated) {
      aControllerState currentState = m_DS485Controller.getState();
      if(currentState != lastState) {
        if((currentState == csSlave) || (currentState == csMaster)) {
          ModelEvent* pEvent = new ModelEvent(ModelEvent::etBusReady);
          getDSS().getApartment().addModelEvent(pEvent);
        }
        lastState = currentState;
      }
      if(!m_IncomingFrames.empty() || m_PacketHere.waitFor(50)) {
        while(!m_IncomingFrames.empty()) {
          m_IncomingFramesGuard.lock();
          // process packets and put them into a functionID-hash
          boost::shared_ptr<DS485CommandFrame> frame = m_IncomingFrames.front();
          m_IncomingFrames.erase(m_IncomingFrames.begin());
          m_IncomingFramesGuard.unlock();
          log("R");

          const std::vector<unsigned char>& ch = frame->getPayload().toChar();
          if(ch.size() < 1) {
            log("received Command Frame w/o function identifier", lsFatal);
            continue;
          }

          uint8_t functionID = ch.front();
          if((frame->getCommand() == CommandRequest || frame->getCommand() == CommandEvent) && functionID != FunctionDSLinkReceive) {
            std::string functionIDStr = FunctionIDToString(functionID);
            if(functionIDStr.empty()) {
              functionIDStr = "Unknown function id: " + intToString(functionID, true);
            }
            std::ostringstream sstream;
            sstream << "Got request: " << functionIDStr << " from " << int(frame->getHeader().getSource()) << " ";

            PayloadDissector pdDump(frame->getPayload());
            while(!pdDump.isEmpty()) {
              uint8_t data = pdDump.get<uint8_t>();
              sstream << "(0x" << std::hex << (unsigned int)data << ", " << std::dec << (int)data << "d)";
            }
            sstream << std::dec;
            log(sstream.str());


            PayloadDissector pd(frame->getPayload());

            if(frame->getFrameSource() == fsWire) {
              getDSS().getSimulation().process(*frame.get());
            }
            if(functionID == FunctionZoneAddDevice) {
              log("New device");
              pd.get<uint8_t>(); // function id
              int modID = frame->getHeader().getSource();
              int zoneID = pd.get<uint16_t>();
              int devID = pd.get<uint16_t>();
              pd.get<uint16_t>(); // version
              int functionID = pd.get<uint16_t>();

              ModelEvent* pEvent = new ModelEvent(ModelEvent::etNewDevice);
              pEvent->addParameter(modID);
              pEvent->addParameter(zoneID);
              pEvent->addParameter(devID);
              pEvent->addParameter(functionID);
              getDSS().getApartment().addModelEvent(pEvent);
            } else if(functionID == FunctionGroupCallScene) {
              pd.get<uint8_t>(); // function id
              uint16_t zoneID = pd.get<uint16_t>();
              uint16_t groupID = pd.get<uint16_t>();
              uint16_t sceneID = pd.get<uint16_t>();
              if((sceneID & 0x00ff) == SceneBell) {
                boost::shared_ptr<Event> evt(new Event("bell"));
                getDSS().getEventQueue().pushEvent(evt);
              } else if((sceneID & 0x00ff) == SceneAlarm) {
                boost::shared_ptr<Event> evt(new Event("alarm"));
                getDSS().getEventQueue().pushEvent(evt);
              } else if((sceneID & 0x00ff) == ScenePanic) {
                boost::shared_ptr<Event> evt(new Event("panic"));
                getDSS().getEventQueue().pushEvent(evt);
              }
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etCallSceneGroup);
              pEvent->addParameter(zoneID);
              pEvent->addParameter(groupID);
              pEvent->addParameter(sceneID);
              getDSS().getApartment().addModelEvent(pEvent);
            } else if(functionID == FunctionDeviceCallScene) {
              pd.get<uint8_t>(); // functionID
              uint16_t devID = pd.get<uint16_t>();
              uint16_t sceneID = pd.get<uint16_t>();
              int modID = frame->getHeader().getDestination();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etCallSceneDevice);
              pEvent->addParameter(modID);
              pEvent->addParameter(devID);
              pEvent->addParameter(sceneID);
              getDSS().getApartment().addModelEvent(pEvent);
            } else if(functionID == FunctionDSLinkInterrupt) {
              pd.get<uint8_t>(); // functionID
              uint16_t devID = pd.get<uint16_t>();
              uint16_t priority = pd.get<uint16_t>();
              int modID = frame->getHeader().getSource();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etDSLinkInterrupt);
              pEvent->addParameter(modID);
              pEvent->addParameter(devID);
              pEvent->addParameter(priority);
              getDSS().getApartment().addModelEvent(pEvent);
            } else if(functionID == EventNewDS485Device) {
              pd.get<uint8_t>(); // functionID
              int modID = pd.get<uint16_t>();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etNewModulator);
              pEvent->addParameter(modID);
              getDSS().getApartment().addModelEvent(pEvent);
            } else if(functionID == EventLostDS485Device) {
              pd.get<uint8_t>(); // functionID
              int modID = pd.get<uint16_t>();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etLostModulator);
              pEvent->addParameter(modID);
              getDSS().getApartment().addModelEvent(pEvent);
            } else if(functionID == EventDeviceReady) {
              int modID = frame->getHeader().getDestination();
              ModelEvent* pEvent = new ModelEvent(ModelEvent::etModulatorReady);
              pEvent->addParameter(modID);
              getDSS().getApartment().addModelEvent(pEvent);
            }
          } else {
            std::ostringstream sstream;
            sstream << "Response: ";
            PayloadDissector pd(frame->getPayload());
            while(!pd.isEmpty()) {
              uint8_t data = pd.get<uint8_t>();
              sstream << "(0x" << std::hex << (unsigned int)data << ", " << std::dec << (int)data << "d)";
            }
            sstream << std::dec;
            sstream << " from " << int(frame->getHeader().getSource());
            log(sstream.str());

            log(string("Response for: ") + FunctionIDToString(functionID));
            boost::shared_ptr<ReceivedFrame> rf(new ReceivedFrame(m_DS485Controller.getTokenCount(), frame));

            bool bucketFound = false;
            // search for a bucket to put the frame in
            m_FrameBucketsGuard.lock();
            foreach(FrameBucketBase* bucket, m_FrameBuckets) {
              if(bucket->getFunctionID() == functionID) {
                if((bucket->getSourceID() == -1) || (bucket->getSourceID() == frame->getHeader().getSource())) {
                  if(bucket->addFrame(rf)) {
                    bucketFound = true;
                  }
                }
              }
            }
            m_FrameBucketsGuard.unlock();
            if(!bucketFound) {
              log("No bucket found for " + intToString(frame->getHeader().getSource()));
            }

          }
        }
      }
    }
  } // execute

  void DS485Proxy::collectFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
    uint8_t commandID = _frame->getCommand();
    if(commandID != CommandResponse && commandID != CommandRequest && commandID != CommandEvent) {
      log("discarded non response/request/command frame", lsInfo);
      log(string("frame type ") + commandToString(commandID));
    } else {
      m_IncomingFramesGuard.lock();
      m_IncomingFrames.push_back(_frame);
      m_IncomingFramesGuard.unlock();
      m_PacketHere.signal();
    }
  } // collectFrame

  void DS485Proxy::addFrameBucket(FrameBucketBase* _bucket) {
    m_FrameBucketsGuard.lock();
    m_FrameBuckets.push_back(_bucket);
    m_FrameBucketsGuard.unlock();
  } // addFrameBucket

  void DS485Proxy::removeFrameBucket(FrameBucketBase* _bucket) {
    m_FrameBucketsGuard.lock();
    std::vector<FrameBucketBase*>::iterator pos = find(m_FrameBuckets.begin(), m_FrameBuckets.end(), _bucket);
    if(pos != m_FrameBuckets.end()) {
      m_FrameBuckets.erase(pos);
    }
    m_FrameBucketsGuard.unlock();
  } // removeFrameBucket

  //================================================== receivedFrame

  ReceivedFrame::ReceivedFrame(const int _receivedAt, boost::shared_ptr<DS485CommandFrame> _frame)
  : m_ReceivedAtToken(_receivedAt),
    m_Frame(_frame)
  {
  } // ctor


  //================================================== FrameBucketBase

  FrameBucketBase::FrameBucketBase(DS485Proxy* _proxy, int _functionID, int _sourceID)
  : m_pProxy(_proxy),
    m_FunctionID(_functionID),
    m_SourceID(_sourceID)
  {
    Logger::getInstance()->log("Bucket: Registering for fid: " + intToString(_functionID) + " sid: " + intToString(_sourceID));
    m_pProxy->addFrameBucket(this);
  } // ctor

  FrameBucketBase::~FrameBucketBase() {
    Logger::getInstance()->log("Bucket: Removing for fid: " + intToString(m_FunctionID) + " sid: " + intToString(m_SourceID));
    m_pProxy->removeFrameBucket(this);
  } // dtor


  //================================================== FrameBucket

  FrameBucketCollector::FrameBucketCollector(DS485Proxy* _proxy, int _functionID, int _sourceID)
  : FrameBucketBase(_proxy, _functionID, _sourceID),
    m_SingleFrame(false)
  { } // ctor

  bool FrameBucketCollector::addFrame(boost::shared_ptr<ReceivedFrame> _frame) {
    bool result = false;
    m_FramesMutex.lock();
    if(!m_SingleFrame || m_Frames.empty()) {
      m_Frames.push_back(_frame);
      result = true;
    }
    m_FramesMutex.unlock();

    if(result) {
      m_PacketHere.signal();
    }
    return result;
  } // addFrame

  boost::shared_ptr<ReceivedFrame> FrameBucketCollector::popFrame() {
    boost::shared_ptr<ReceivedFrame> result;

    m_FramesMutex.lock();
    if(!m_Frames.empty()) {
      result = m_Frames.front();
      m_Frames.pop_front();
    }
    m_FramesMutex.unlock();
    return result;
  } // popFrame

  void FrameBucketCollector::waitForFrames(int _timeoutMS) {
    sleepMS(_timeoutMS);
  } // waitForFrames

  bool FrameBucketCollector::waitForFrame(int _timeoutMS) {
    m_SingleFrame = true;
    if(m_Frames.empty()) {
      Logger::getInstance()->log("FrameBucket::waitForFrame: Waiting for frame");
      if(m_PacketHere.waitFor(_timeoutMS)) {
        Logger::getInstance()->log("FrameBucket::waitForFrame: Got frame");
      } else {
        Logger::getInstance()->log("FrameBucket::waitForFrame: No frame received");
        return false;
      }
    }
    return true;
  } // waitForFrame

  int FrameBucketCollector::getFrameCount() const {
    return m_Frames.size();
  } // getFrameCount

  bool FrameBucketCollector::isEmpty() const {
    return m_Frames.empty();
  } // isEmpty

}
