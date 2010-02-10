/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "core/dss.h"
#include "core/logger.h"
#include "core/ds485const.h"
#include "core/event.h"
#include "core/propertysystem.h"
#include "core/foreach.h"

#include "core/model/modelevent.h"
#include "core/model/device.h"
#include "core/model/modelmaintenance.h"

#include "core/ds485/framebucketcollector.h"
#include "core/ds485/businterfacehandler.h"

#ifdef WITH_SIM
#include "core/sim/dssim.h"
#endif

#include <sstream>

namespace dss {

  const char* FunctionIDToString(const int _functionID); // internal forward declaration

  DS485Proxy::DS485Proxy(DSS* _pDSS, ModelMaintenance* _pModelMaintenance)
  : Subsystem(_pDSS, "DS485Proxy"),
    m_pBusInterfaceHandler(NULL),
    m_pModelMaintenance(_pModelMaintenance),
    m_InitializeDS485Controller(true)
  {
    assert(_pModelMaintenance != NULL);
    if(_pDSS != NULL) {
      _pDSS->getPropertySystem().createProperty(getConfigPropertyBasePath() + "rs485devicename")
            ->linkToProxy(PropertyProxyMemberFunction<DS485Controller, std::string>(m_DS485Controller, &DS485Controller::getRS485DeviceName, &DS485Controller::setRS485DeviceName));
      _pDSS->getPropertySystem().setBoolValue(getConfigPropertyBasePath() + "denyJoiningAsShortDevice", false, true, false);

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

      _pDSS->getPropertySystem().setStringValue("/system/dsid", "3504175FE0000000DEADBEEF", true, false);
    }
  } // ctor

  bool DS485Proxy::isReady() {
    bool simReady =
#ifdef WITH_SIM
             DSS::getInstance()->getSimulation().isReady(); // allow the simulation to run on it's own
#else
             true;
#endif
    bool selfReady = (m_pBusInterfaceHandler != NULL) ? m_pBusInterfaceHandler->isRunning() : true;
    bool controllerReady =
        ((m_DS485Controller.getState() == csSlave) ||
        (m_DS485Controller.getState() == csDesignatedMaster) ||
        (m_DS485Controller.getState() == csError));

	  return simReady && selfReady && controllerReady;
  } // isReady

  uint16_t DS485Proxy::deviceGetParameterValue(devid_t _id, uint8_t _dsMeterID, int _paramID) {
    DS485CommandFrame frame;
    frame.getHeader().setDestination(_dsMeterID);
    frame.getHeader().setBroadcast(false);
    frame.getHeader().setType(1);
    frame.setCommand(CommandRequest);
    frame.getPayload().add<uint8_t>(FunctionDeviceGetParameterValue);
    frame.getPayload().add<uint16_t>(_id);
    frame.getPayload().add<uint16_t>(_paramID);
    uint8_t res = receiveSingleResult(frame, FunctionDeviceGetParameterValue);
    return res;
  } // deviceGetParameterValue

  uint16_t DS485Proxy::deviceGetFunctionID(devid_t _id, uint8_t _dsMeterID) {
    DS485CommandFrame frame;
    frame.getHeader().setDestination(_dsMeterID);
    frame.getHeader().setBroadcast(false);
    frame.getHeader().setType(1);
    frame.setCommand(CommandRequest);
    frame.getPayload().add<uint8_t>(FunctionDeviceGetFunctionID);
    frame.getPayload().add<devid_t>(_id);

    uint16_t result;
    boost::shared_ptr<DS485CommandFrame> resFrame = receiveSingleFrame(frame, FunctionDeviceGetFunctionID);
    if(resFrame.get() != NULL) {
      PayloadDissector pd(resFrame->getPayload());
      pd.get<uint8_t>(); // skip the function id
      if(pd.get<uint16_t>() == 0x0001) {
        result = pd.get<uint16_t>();
        checkResultCode(result);
      }
    }
    return result;
  } // deviceGetFunctionID

  void DS485Proxy::sendFrame(DS485CommandFrame& _frame) {
    _frame.setFrameSource(fsDSS);
    bool broadcast = _frame.getHeader().isBroadcast();
#ifdef WITH_SIM
    bool sim = isSimAddress(_frame.getHeader().getDestination());
    if(broadcast || sim) {
      log("Sending packet to sim");
      if(DSS::hasInstance()) {
        getDSS().getSimulation().process(_frame);
      }
    }
#else
    bool sim = false;
#endif
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
    if(m_pBusInterfaceHandler != NULL) {
      m_pBusInterfaceHandler->collectFrame(pFrame);
    }
  } // sendFrame

  boost::shared_ptr<FrameBucketCollector> DS485Proxy::sendFrameAndInstallBucket(DS485CommandFrame& _frame, const int _functionID) {
    int sourceID = _frame.getHeader().isBroadcast() ? -1 :  _frame.getHeader().getDestination();
    boost::shared_ptr<FrameBucketCollector> result(new FrameBucketCollector(&getDSS().getBusInterfaceHandler(), _functionID, sourceID), FrameBucketBase::removeFromProxyAndDelete);
    result->addToProxy();
    sendFrame(_frame);
    return result;
  } // sendFrameAndInstallBucket

#ifdef WITH_SIM
  bool DS485Proxy::isSimAddress(const uint8_t _addr) {
    if(DSS::hasInstance()) {
      return getDSS().getSimulation().isSimAddress(_addr);
    } else {
      return true;
    }
  } // isSimAddress
#endif

  void DS485Proxy::checkResultCode(const int _resultCode) {
    if(_resultCode < 0) {
      std::string message = "Unknown Error";
      switch(_resultCode) {
      case kDS485NoIDForIndexFound:
        message = "No ID for index found";
        break;
      case kDS485ZoneNotFound:
        message = "Zone not found";
        break;
      case kDS485IndexOutOfBounds:
        message = "Index out of bounds";
        break;
      case kDS485GroupIDOutOfBounds:
        message = "Group ID out of bounds";
        break;
      case kDS485ZoneCannotBeDeleted:
        message = "Zone can not be deleted";
        break;
      case kDS485OutOfMemory:
        message = "dSM is out of memory";
        break;
      case kDS485RoomAlreadyExists:
        message = "Room already exists";
        break;
      case kDS485InvalidDeviceID:
        message = "Invalid device id";
        break;
      case kDS485CannotRemoveFromStandardGroup:
        message = "Cannot remove device from standard group";
        break;
      case kDS485CannotDeleteStandardGroup:
        message = "Cannot delete standard group";
        break;
      case kDS485DSIDIsNull:
        message = "DSID is null";
        break;
      case kDS485ReservedRoomNumber:
        message = "Room number is reserved";
        break;
      case kDS485DeviceNotFound:
        message = "Device not found";
        break;
      case kDS485GroupNotFound:
        message = "Group not found";
        break;
      }
      throw DS485ApiError(message);
    }
  } // checkResultCode

  void DS485Proxy::setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size) {
    DS485CommandFrame frame;
    frame.getHeader().setDestination(_device.getDSMeterID());
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

  DSMeterSpec_t DS485Proxy::dsMeterSpecFromFrame(boost::shared_ptr<DS485CommandFrame> _frame) {
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
      log(std::string("Found unknown device (") + intToString(devID, true) + ")");
    }
    uint16_t hwVersion = (pd.get<uint8_t>() << 8) | pd.get<uint8_t>();
    uint16_t swVersion = (pd.get<uint8_t>() << 8) | pd.get<uint8_t>();

    log(std::string("  HW-Version: ") + intToString(hwVersion >> 8) + "." + intToString(hwVersion & 0x00FF));
    log(std::string("  SW-Version: ") + intToString(swVersion >> 8) + "." + intToString(swVersion & 0x00FF));

    std::string name;
    for(int i = 0; i < 6; i++) {
      char c = char(pd.get<uint8_t>());
      if(c != '\0') {
        name += c;
      }
    }
    log(std::string("  Name:      \"") + name + "\"");

    // bus-id, sw-version, hw-version, name, device-id
    DSMeterSpec_t spec(source, swVersion, hwVersion, name, devID);
    return spec;
  } // dsMeterSpecFromFrame

  std::vector<DSMeterSpec_t> DS485Proxy::getDSMeters() {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(0);
    cmdFrame.getHeader().setBroadcast(true);
    cmdFrame.setCommand(CommandRequest);
    log("Proxy: GetDSMeters");

    cmdFrame.getPayload().add<uint8_t>(FunctionGetTypeRequest);
    boost::shared_ptr<FrameBucketCollector> bucket = sendFrameAndInstallBucket(cmdFrame, FunctionGetTypeRequest);
    bucket->waitForFrames(5000);

    std::map<int, bool> resultFrom;

    std::vector<DSMeterSpec_t> result;
    while(true) {
      boost::shared_ptr<DS485CommandFrame> recFrame = bucket->popFrame();
      if(recFrame == NULL) {
        break;
      }
      int source = recFrame->getHeader().getSource();
      if(resultFrom[source]) {
        log(std::string("already received result from ") + intToString(source));
        continue;
      }
      DSMeterSpec_t spec = dsMeterSpecFromFrame(recFrame);
      result.push_back(spec);
    }

    return result;
  } // getDSMeters

  DSMeterSpec_t DS485Proxy::getDSMeterSpec(const int _dsMeterID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.getHeader().setBroadcast(false);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionGetTypeRequest);
    log("Proxy: getDSMeterSpec");

    boost::shared_ptr<DS485CommandFrame> recFrame = receiveSingleFrame(cmdFrame, FunctionGetTypeRequest);

    if(recFrame == NULL) {
      throw DS485ApiError("No frame received");
    }

    DSMeterSpec_t result = dsMeterSpecFromFrame(recFrame);

    return result;
  } // getDSMeterSpec

  int DS485Proxy::getGroupCount(const int _dsMeterID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterGetGroupsSize);
    cmdFrame.getPayload().add<uint16_t>(_zoneID);
    int8_t res = int8_t(receiveSingleResult(cmdFrame, FunctionDSMeterGetGroupsSize));
    if(res < 0) {
      log("GetGroupCount: Negative group count received '" + intToString(res) +
          " on dsMeter " + intToString(_dsMeterID) +
          " with zone " + intToString(_zoneID));
    }
    checkResultCode(res);
    // Every dsMeter should provide all standard-groups
    if(res < GroupIDStandardMax) {
      res = GroupIDStandardMax;
    }
    return res;
  } // getGroupCount

  std::vector<int> DS485Proxy::getGroups(const int _dsMeterID, const int _zoneID) {
    std::vector<int> result;

    int numGroups = getGroupCount(_dsMeterID, _zoneID);
    log(std::string("DSMeter has ") + intToString(numGroups) + " groups");
    for(int iGroup = 0; iGroup < numGroups; iGroup++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.getHeader().setDestination(_dsMeterID);
      cmdFrame.setCommand(CommandRequest);
      cmdFrame.getPayload().add<uint8_t>(FunctionZoneGetGroupIdForInd);
      cmdFrame.getPayload().add<uint16_t>(_zoneID);
      cmdFrame.getPayload().add<uint16_t>(iGroup);

      int8_t res = int8_t(receiveSingleResult(cmdFrame, FunctionZoneGetGroupIdForInd));
      if(res < 0) {
        log("GetGroups: Negative index received '" + intToString(res) + "' for index " + intToString(iGroup), lsFatal);
      } else {
        result.push_back(res);
      }
      //checkResultCode(res);
    }

    return result;
  } // getGroups

  int DS485Proxy::getDevicesInGroupCount(const int _dsMeterID, const int _zoneID, const int _groupID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionGroupGetDeviceCount);
    cmdFrame.getPayload().add<uint16_t>(_zoneID);
    cmdFrame.getPayload().add<uint16_t>(_groupID);

    int16_t res = int16_t(receiveSingleResult16(cmdFrame, FunctionGroupGetDeviceCount));
    if(res < 0) {
      log("GetDevicesInGroupCount: Negative count received '" + intToString(res) +
          "' on dsMeter " + intToString(_dsMeterID) +
          " with zoneID " + intToString(_zoneID) + " in group " + intToString(_groupID));
    }
    checkResultCode(res);

    return res;
  } // getDevicesInGroupCount

  std::vector<int> DS485Proxy::getDevicesInGroup(const int _dsMeterID, const int _zoneID, const int _groupID) {
    std::vector<int> result;

    int numDevices = getDevicesInGroupCount(_dsMeterID, _zoneID, _groupID);
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.getHeader().setDestination(_dsMeterID);
      cmdFrame.setCommand(CommandRequest);
      cmdFrame.getPayload().add<uint8_t>(FunctionGroupGetDevKeyForInd);
      cmdFrame.getPayload().add<uint16_t>(_zoneID);
      cmdFrame.getPayload().add<uint16_t>(_groupID);
      cmdFrame.getPayload().add<uint16_t>(iDevice);
      int16_t res = int16_t(receiveSingleResult16(cmdFrame, FunctionGroupGetDevKeyForInd));
      if(res < 0) {
        log("GetDevicesInGroup: Negative device id received '" + intToString(res) + "' for index " + intToString(iDevice), lsFatal);
      } else {
        result.push_back(res);
      }
      try {
        checkResultCode(res);
      } catch(DS485ApiError& err) {
        log(std::string("Error reported back by dSM: ") + err.what(), lsFatal);
      }
    }

    return result;
  } // getDevicesInGroup

  std::vector<int> DS485Proxy::getGroupsOfDevice(const int _dsMeterID, const int _deviceID) {
    std::vector<int> result;

    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDeviceGetGroups);
    cmdFrame.getPayload().add<uint16_t>(_deviceID);

    boost::shared_ptr<FrameBucketCollector> bucket = sendFrameAndInstallBucket(cmdFrame, FunctionDeviceGetGroups);

    bucket->waitForFrame(5000);

    while(true) {
      boost::shared_ptr<DS485CommandFrame> recFrame = bucket->popFrame();
      if(recFrame == NULL) {
        break;
      }

      PayloadDissector pd(recFrame->getPayload());
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

  std::vector<int> DS485Proxy::getZones(const int _dsMeterID) {
    std::vector<int> result;

    int numZones = getZoneCount(_dsMeterID);
    log(std::string("DSMeter has ") + intToString(numZones) + " zones");
    for(int iZone = 0; iZone < numZones; iZone++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.getHeader().setDestination(_dsMeterID);
      cmdFrame.setCommand(CommandRequest);
      cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterGetZoneIdForInd);
      cmdFrame.getPayload().add<uint16_t>(iZone);
      log("GetZoneID");
      int16_t tempResult = int16_t(receiveSingleResult16(cmdFrame, FunctionDSMeterGetZoneIdForInd));
      // TODO: The following line is a workaround as described in #246
      if((tempResult <= 0) && (tempResult > -20)) {
        log("GetZones: Negative zone id " + intToString(tempResult) + " received. DSMeter: " + intToString(_dsMeterID) + " index: " + intToString(iZone), lsError);
        // TODO: take this line outside the if-clause after the dSM-API has been reworked
        checkResultCode(tempResult);
      } else {
        result.push_back(tempResult);
      }
      log("received ZoneID: " + uintToString((unsigned int)tempResult));
    }
    return result;
  } // getZones

  int DS485Proxy::getZoneCount(const int _dsMeterID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterGetZonesSize);
    log("GetZoneCount");

    int8_t result = int8_t(
        receiveSingleResult(cmdFrame, FunctionDSMeterGetZonesSize));
    checkResultCode(result);
    return result;
  } // getZoneCount

  int DS485Proxy::getDevicesCountInZone(const int _dsMeterID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterCountDevInZone);
    cmdFrame.getPayload().add<uint16_t>(_zoneID);
    log("GetDevicesCountInZone");

    log(intToString(_dsMeterID) + " " + intToString(_zoneID));

    int16_t result = int16_t(receiveSingleResult16(cmdFrame, FunctionDSMeterCountDevInZone));
    if(result < 0) {
      log("GetDevicesCountInZone: negative count '" + intToString(result) + "'", lsError);
    }
    checkResultCode(result);

    return result;
  } // getDevicesCountInZone

  std::vector<int> DS485Proxy::getDevicesInZone(const int _dsMeterID, const int _zoneID) {
    std::vector<int> result;

    int numDevices = getDevicesCountInZone(_dsMeterID, _zoneID);
    log(std::string("Found ") + intToString(numDevices) + " in zone.");
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      DS485CommandFrame cmdFrame;
      cmdFrame.getHeader().setDestination(_dsMeterID);
      cmdFrame.setCommand(CommandRequest);
      cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterDevKeyInZone);
      cmdFrame.getPayload().add<uint16_t>(_zoneID);
      cmdFrame.getPayload().add<uint16_t>(iDevice);

      uint16_t devID = receiveSingleResult16(cmdFrame, FunctionDSMeterDevKeyInZone);
      checkResultCode(int16_t(devID));
      result.push_back(devID);
    }
    return result;
  } // getDevicesInZone

  void DS485Proxy::setZoneID(const int _dsMeterID, const devid_t _deviceID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDeviceSetZoneID);
    cmdFrame.getPayload().add<devid_t>(_deviceID);
    cmdFrame.getPayload().add<uint16_t>(_zoneID);

    int16_t res = int16_t(receiveSingleResult(cmdFrame, FunctionDeviceSetZoneID));
    checkResultCode(res);
  } // setZoneID

  void DS485Proxy::createZone(const int _dsMeterID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterAddZone);
    cmdFrame.getPayload().add<uint16_t>(_zoneID);

    int16_t res = int16_t(receiveSingleResult(cmdFrame, FunctionDSMeterAddZone));
    checkResultCode(res);
  } // createZone

  void DS485Proxy::removeZone(const int _dsMeterID, const int _zoneID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterRemoveZone);
    cmdFrame.getPayload().add<uint16_t>(_zoneID);

    int16_t res = int16_t(receiveSingleResult(cmdFrame, FunctionDSMeterAddZone));
    checkResultCode(res);
  } // removeZone

  dsid_t DS485Proxy::getDSIDOfDevice(const int _dsMeterID, const int _deviceID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDeviceGetDSID);
    cmdFrame.getPayload().add<uint16_t>(_deviceID);
    log("Proxy: GetDSIDOfDevice");

    boost::shared_ptr<DS485CommandFrame> recFrame = receiveSingleFrame(cmdFrame, FunctionDeviceGetDSID);
    if(recFrame == NULL) {
      throw DS485ApiError("No frame received");
    }

    PayloadDissector pd(recFrame->getPayload());
    pd.get<uint8_t>(); // discard the function id
    int16_t res = int16_t(pd.get<uint16_t>());
    checkResultCode(res);
    return pd.get<dsid_t>();
  } // getDSIDOfDevice

  dsid_t DS485Proxy::getDSIDOfDSMeter(const int _dsMeterID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterGetDSID);
    log(std::string("Proxy: GetDSIDOfDSMeter ") + intToString(_dsMeterID));

    boost::shared_ptr<DS485CommandFrame> recFrame = receiveSingleFrame(cmdFrame, FunctionDSMeterGetDSID);
    if(recFrame == NULL) {
      log("GetDSIDOfDSMeter: received no result from " + intToString(_dsMeterID), lsError);
      throw DS485ApiError("No frame received");
    }

    PayloadDissector pd(recFrame->getPayload());
    pd.get<uint8_t>(); // discard the function id
    return pd.get<dsid_t>();
  } // getDSIDOfDSMeter

  int DS485Proxy::getLastCalledScene(const int _dsMeterID, const int _zoneID, const int _groupID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionGroupGetLastCalledScene);
    log(std::string("Proxy: GetLastCalledScene ") + intToString(_dsMeterID));
    cmdFrame.getPayload().add<uint16_t>(_zoneID);
    cmdFrame.getPayload().add<uint16_t>(_groupID);

    int16_t res = int16_t(receiveSingleResult16(cmdFrame, FunctionGroupGetLastCalledScene));
    if(res < 0) {
      log("DS485Proxy::getLastCalledScene: negative result received: " + intToString(res));
    }
    checkResultCode(res);
    return res;
  } // getLastCalledScene

  unsigned long DS485Proxy::getPowerConsumption(const int _dsMeterID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterGetPowerConsumption);
    log(std::string("Proxy: GetPowerConsumption ") + intToString(_dsMeterID));

    boost::shared_ptr<DS485CommandFrame> recFrame = receiveSingleFrame(cmdFrame, FunctionDSMeterGetPowerConsumption);
    if(recFrame == NULL) {
      log("DS485Proxy::getPowerConsumption: received no results", lsError);
      throw DS485ApiError("No frame received");
    }
    if(recFrame->getHeader().getSource() != _dsMeterID) {
      log("GetPowerConsumption: received result from wrong source");
    }
    PayloadDissector pd(recFrame->getPayload());
    pd.get<uint8_t>(); // discard the function id
    return pd.get<uint32_t>();
  } // getPowerConsumption

  void DS485Proxy::requestPowerConsumption() {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(0);
    cmdFrame.getHeader().setBroadcast(true);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterGetPowerConsumption);
    sendFrame(cmdFrame);
  } // requestPowerConsumption

  unsigned long DS485Proxy::getEnergyMeterValue(const int _dsMeterID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterGetEnergyMeterValue);
    log(std::string("Proxy: GetEnergyMeterValue ") + intToString(_dsMeterID));

    boost::shared_ptr<DS485CommandFrame> recFrame = receiveSingleFrame(cmdFrame, FunctionDSMeterGetEnergyMeterValue);
    if(recFrame == NULL) {
      log("DS485Proxy::getEnergyMeterValue: received no results", lsError);
      throw DS485ApiError("No frame received");
    }
    PayloadDissector pd(recFrame->getPayload());
    pd.get<uint8_t>(); // discard the function id
    return pd.get<uint32_t>();
  } // getEnergyMeterValue

  void DS485Proxy::requestEnergyMeterValue() {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(0);
    cmdFrame.getHeader().setBroadcast(true);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterGetEnergyMeterValue);
    sendFrame(cmdFrame);
  } // requestEnergyMeterValue

  bool DS485Proxy::getEnergyBorder(const int _dsMeterID, int& _lower, int& _upper) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterGetEnergyLevel);

    boost::shared_ptr<FrameBucketCollector> bucket = sendFrameAndInstallBucket(cmdFrame, FunctionDSMeterGetEnergyLevel);

    bucket->waitForFrame(5000);

    boost::shared_ptr<DS485CommandFrame> recFrame = bucket->popFrame();
    if(recFrame == NULL) {
      throw DS485ApiError("No frame received");
    }

    PayloadDissector pd(recFrame->getPayload());
    pd.get<uint8_t>(); // discard the function id
    _lower = pd.get<uint16_t>();
    _upper = pd.get<uint16_t>();
    return true;
  } // getEnergyBorder

  int DS485Proxy::getSensorValue(const Device& _device, const int _sensorID) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_device.getDSMeterID());
    cmdFrame.getHeader().setBroadcast(false);
    cmdFrame.getHeader().setType(1);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDeviceGetSensorValue);
    cmdFrame.getPayload().add<uint16_t>(_device.getShortAddress());
    cmdFrame.getPayload().add<uint16_t>(_sensorID);
    log("GetSensorValue");

    boost::shared_ptr<FrameBucketCollector> bucket = sendFrameAndInstallBucket(cmdFrame, FunctionDeviceGetSensorValue);
    bucket->waitForFrame(5000);
    boost::shared_ptr<DS485CommandFrame> recFrame;
    if(bucket->isEmpty()) {
      log(std::string("received no ack for request getSensorValue"));
      throw DS485ApiError("no Ack for sensorValue");
    } else if(bucket->getFrameCount() == 1) {
        // first frame received, wait for the next frame
      recFrame= bucket->popFrame();
      bucket->waitForFrame(5000);
    } else
        recFrame= bucket->popFrame();
    // first frame is only request ack;

    PayloadDissector pd(recFrame->getPayload());
    pd.get<uint8_t>(); // discard functionID
    checkResultCode((int)pd.get<uint16_t>()); // check first ack

    if(bucket->isEmpty()) {
        // no next frame after additional waiting.
        throw DS485ApiError("no Answer for sensorValue");
    }

    recFrame = bucket->popFrame();

    if(recFrame.get() != NULL) {
        PayloadDissector pd(recFrame->getPayload());
        pd.get<uint8_t>(); // discard functionID
        pd.get<uint16_t>();
        pd.get<uint16_t>();
        checkResultCode((int)pd.get<uint16_t>()); // check sensorvalue
        int result = int(pd.get<uint16_t>());
        log(std::string("result ") + intToString(result));
        return result;
    } else {
      throw std::runtime_error("received frame is NULL but bucket->isEmpty() returns false");
    }
  } // getSensorValue

  uint8_t DS485Proxy::dSLinkSend(const int _dsMeterID, devid_t _devAdr, uint8_t _value, uint8_t _flags) {
    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(_dsMeterID);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSLinkSendDevice);
    cmdFrame.getPayload().add<uint16_t>(_devAdr);
    cmdFrame.getPayload().add<uint16_t>(_value);
    cmdFrame.getPayload().add<uint16_t>(_flags);

    if((_flags & DSLinkSendWriteOnly) == 0) {
      boost::shared_ptr<FrameBucketCollector> bucket = sendFrameAndInstallBucket(cmdFrame, FunctionDSLinkReceive);
      bucket->waitForFrame(10000);
      boost::shared_ptr<DS485CommandFrame> recFrame = bucket->popFrame();
      if(recFrame == NULL) {
        log("dsLinkSend: No packet received", lsError);
        throw DS485ApiError("No frame received");
      }
      PayloadDissector pd(recFrame->getPayload());
      pd.get<uint8_t>(); // discard the function id
      pd.get<uint16_t>(); // garbage
      devid_t devAddress = pd.get<uint16_t>(); // device address
      if(devAddress != _devAdr) {
        std::string errStr =
            "dSLinkSend: Received answer for wrong device expected: "+
            intToString(_devAdr, true) +
            " got: " + intToString(devAddress, true);
        log(errStr, lsError);
        throw DS485ApiError(errStr);
      }
      return pd.get<uint16_t>();
    } else {
      sendFrame(cmdFrame);
      log("dsLinkSend: Not waiting for response (writeOnly is set)");
      return 0;
    }
  } // dsLinkSend

  void DS485Proxy::addToGroup(const int _dsMeterID, const int _groupID, const int _deviceID) {

  } // addToGroup

  void DS485Proxy::removeFromGroup(const int _dsMeterID, const int _groupID, const int _deviceID) {

  } // removeFromGroup

  int DS485Proxy::addUserGroup(const int _dsMeterID) {
    return 0;
  } // addUserGroup

  void DS485Proxy::removeUserGroup(const int _dsMeterID, const int _groupID) {

  } // removeUserGroup

  boost::shared_ptr<DS485CommandFrame> DS485Proxy::receiveSingleFrame(DS485CommandFrame& _frame, uint8_t _functionID) {
    boost::shared_ptr<FrameBucketCollector> bucket = sendFrameAndInstallBucket(_frame, _functionID);
    bucket->waitForFrame(5000);

    if(bucket->isEmpty()) {
      log(std::string("received no results for request (") + FunctionIDToString(_functionID) + ")");
      return boost::shared_ptr<DS485CommandFrame>();
    } else if(bucket->getFrameCount() > 1) {
      log(std::string("received multiple results (") + intToString(bucket->getFrameCount()) + ") for request (" + FunctionIDToString(_functionID) + ")");
      // TODO: check
      return bucket->popFrame();
    }

    boost::shared_ptr<DS485CommandFrame> recFrame = bucket->popFrame();

    if(recFrame.get() != NULL) {
      return recFrame;
    } else {
      throw std::runtime_error("received frame is NULL but bucket->isEmpty() returns false");
    }
  } // receiveSingleFrame

  uint8_t DS485Proxy::receiveSingleResult(DS485CommandFrame& _frame, const uint8_t _functionID) {
    boost::shared_ptr<DS485CommandFrame> recFrame = receiveSingleFrame(_frame, _functionID);

    if(recFrame.get() != NULL) {
      PayloadDissector pd(recFrame->getPayload());
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
    boost::shared_ptr<DS485CommandFrame> recFrame = receiveSingleFrame(_frame, _functionID);

    if(recFrame.get() != NULL) {
      PayloadDissector pd(recFrame->getPayload());
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
    if(m_pBusInterfaceHandler != NULL) {
      m_DS485Controller.addFrameCollector(m_pBusInterfaceHandler);
#ifdef WITH_SIM
      if(DSS::hasInstance()) {
        getDSS().getSimulation().addFrameCollector(m_pBusInterfaceHandler);
      }
#endif
    }
  } // initialize

  void DS485Proxy::doStart() {
    if(m_InitializeDS485Controller) {
      try {
        m_DS485Controller.setDenyJoiningAsShortDevice(getDSS().getPropertySystem().getBoolValue(getConfigPropertyBasePath() + "denyJoiningAsShortDevice"));
        m_DS485Controller.setDSID(dsid_t::fromString(getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "dsid")));
        m_DS485Controller.setBusReadyCallback(boost::bind(&DS485Proxy::busReady,this));
        m_DS485Controller.run();
      } catch (const std::runtime_error& _ex) {
        log(std::string("Caught exception while starting DS485Controller: ") + _ex.what(), lsFatal);
      }
    }
  } // doStart

  void DS485Proxy::busReady() {
    ModelEvent* pEvent = new ModelEvent(ModelEvent::etBusReady);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // busReady


} // namespace dss
