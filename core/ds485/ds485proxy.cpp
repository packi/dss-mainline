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

#include <digitalSTROM/dsm-api-v2/dsm-api-const.h>

#include "core/dss.h"
#include "core/logger.h"
#include "core/ds485const.h"
#include "core/event.h"
#include "core/propertysystem.h"
#include "core/foreach.h"
#include "core/dsidhelper.h"

#include "core/model/modelevent.h"
#include "core/model/device.h"
#include "core/model/modelmaintenance.h"

// TODO: libdsm
// #include "core/ds485/framebucketcollector.h"
// #include "core/ds485/businterfacehandler.h"

// TODO: libdsm
// #include "core/sim/dssim.h"

#include <sstream>


namespace dss {

  const char* FunctionIDToString(const int _functionID); // internal forward declaration

  DS485Proxy::DS485Proxy(DSS* _pDSS, ModelMaintenance* _pModelMaintenance, DSSim* _pDSSim)
  : Subsystem(_pDSS, "DS485Proxy"),
    m_pBusInterfaceHandler(NULL),
    m_pModelMaintenance(_pModelMaintenance),
    m_pDSSim(_pDSSim),
    
    m_dsmApiHandle(NULL),
    m_dsmApiReady(false),
    m_connection("tcp://localhost:8442"),
    m_InitializeDS485Controller(true)
  {
    assert(_pModelMaintenance != NULL);
    if(_pDSS != NULL) {
      
#if 0
      // TODO: libdsm
      
      // read m_connection from config

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

#endif

      _pDSS->getPropertySystem().setStringValue("/system/dsid", "3504175FE0000000DEADBEEF", true, false);
    }
  } // ctor

  bool DS485Proxy::isReady() {
#if 0
    // TODO: libdsm
    bool simReady = (m_pDSSim != NULL) ? m_pDSSim->isReady() : true;
    bool selfReady = (m_pBusInterfaceHandler != NULL) ? m_pBusInterfaceHandler->isRunning() : true;
    bool controllerReady =
      ((m_dsmApiController.getState() == csSlave) ||
       (m_dsmApiController.getState() == csDesignatedMaster) ||
       (m_dsmApiController.getState() == csError));

    return simReady && selfReady && controllerReady;
#endif
    return m_dsmApiReady;
  } // isReady

  uint16_t DS485Proxy::deviceGetParameterValue(devid_t _id, const dss_dsid_t& _dsMeterDSID, int _paramID) {
    log("deviceGetParameterValue: not implemented yet");
    return 0;
  } // deviceGetParameterValue

  DeviceSpec_t DS485Proxy::deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID) {

    uint16_t functionId, productId, version;
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsmDSID);
    int ret = DeviceInfo_by_device_id_sync(m_dsmApiHandle, dsmDSID, _id,
                                           NULL, &functionId, &productId, &version,
                                           NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    checkResultCode(ret);

    DeviceSpec_t spec(functionId, productId, version, _id);
    return spec;
  } // deviceGetSpec

  void DS485Proxy::lockOrUnlockDevice(const Device& _device, const bool _lock) {
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    int ret = DeviceProperties_set_locked_flag_sync(m_dsmApiHandle, dsmDSID,
                                                    _device.getShortAddress(), _lock);
    checkResultCode(ret);
  } // lockOrUnlockDevice

  bool DS485Proxy::isLocked(boost::shared_ptr<const Device> _device) {

    uint8_t locked;
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device->getDSMeterDSID(), dsmDSID);
    int ret = DeviceInfo_by_device_id_sync(m_dsmApiHandle, dsmDSID, _device->getShortAddress(),
                                           NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                           &locked, NULL, NULL, NULL, NULL);
    checkResultCode(ret);
    return locked;
  } // isLocked

  bool DS485Proxy::isSimAddress(const uint8_t _addr) {
#if 0
    // TODO: libdsm
    if(m_pDSSim != NULL) {
      return m_pDSSim->isSimAddress(_addr);
    } else {
      return false;
    }
#endif
    return false;
  } // isSimAddress

  void DS485Proxy::checkResultCode(const int _resultCode) {
    if(_resultCode != ERROR_OK) {
      std::string message = "Unknown Error";
      switch(_resultCode) {
        case ERROR_WRONG_PARAMETER:
          message = "Wrong parameter";
          break;
        case ERROR_ZONE_NOT_FOUND:
          message = "Zone not found";
          break;
        case ERROR_DEVICE_NOT_FOUND:
          message = "device not found";
          break;
        case ERROR_GROUP_NOT_FOUND:
          message = "group not found";
          break;
        case ERROR_ZONE_CAN_NOT_BE_DELETED:
          message = "zone can't be deleted";
          break;
        case ERROR_GROUP_CAN_NOT_BE_DELETED:
          message = "group can't be deleted";
          break;
        case ERROR_DEVICE_CAN_NOT_BE_DELETED:
          message = "device can't be deleted";
          break;
        case ERROR_NO_FURTHER_ZONES:
          message = "no further zones";
          break;
        case ERROR_NO_FURTHER_GROUPS:
          message = "no further groups";
          break;
        case ERROR_ZONE_ALREADY_EXISTS:
          message = "zone already exists";
          break;
        case ERROR_GROUP_ALREADY_EXISTS:
          message = "group already exists";
          break;
        case ERROR_ZONE_NOT_EMPTY:
          message = "zone not empty";
          break;
        case ERROR_TIMEOUT:
          message = "timeout";
          break;
        case ERROR_WRONG_SIZE:
          message = "wrong size";
          break;
        case ERROR_WRONG_MSGID:
          message = "wrong msgid";
          break;
        case ERROR_WRONG_MODIFIER:
          message = "wrong modifier";
          break;
        case ERROR_WRONG_PACKET_NR:
          message = "wrong packet number";
          break;
        case ERROR_WRONG_IMAGE_SIZE:
          message = "wrong image size";
          break;
        case ERROR_NO_IMAGE_TRANSFER_ACTIVE:
          message = "no image tranfer active";
          break;
        case ERROR_IMAGE_INVALID:
          message = "image invalid";
          break;
        case ERROR_NO_CONFIG:
          message = "no config";
          break;
        case ERROR_REQUEST_CAN_NOT_BE_EXECUTED:
          message = "request can't be executed";
          break;
      }
      throw BusApiError(message);
    }
  } // checkResultCode

  void DS485Proxy::setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size) {
    log("Not implemented yet: setValueDevice()");
  } // setValueDevice


  std::vector<DSMeterSpec_t> DS485Proxy::getDSMeters() {
    std::vector<DSMeterSpec_t> result;
    dsid_t device_list[63];
    dsid_t ownDSID;

    // TODO: we could cache our own DSID
    int ret = DsmApiGetOwnDSID(m_dsmApiHandle, &ownDSID);
    checkResultCode(ret);

    ret = DsmApiGetBusMembers(m_dsmApiHandle, device_list, 63);
    if (ret < 0) {
      // DsmApiGetBusMembers returns >= 0 on success
      checkResultCode(ret);
    }

    for (int i = 0; i < ret; ++i) {
      // don't include ourself
      if (IsEqualId(device_list[i], ownDSID)) {
        continue;
      }
      DSMeterSpec_t spec = getDSMeterSpec(device_list[i]);
      result.push_back(spec);
    }
    return result;
  } // getDSMeters

  DSMeterSpec_t DS485Proxy::getDSMeterSpec(dsid_t _dsMeterID) {

    uint32_t hwVersion;
    uint32_t swVersion;
    uint16_t apiVersion;
    uint8_t dsidBuf[DSID_LEN];
    uint8_t nameBuf[NAME_LEN];
    int ret = dSMInfo_sync(m_dsmApiHandle, _dsMeterID,
                           &hwVersion, &swVersion, &apiVersion, dsidBuf, nameBuf);
    checkResultCode(ret);

    // convert to std::string
    char nameStr[NAME_LEN];
    memcpy(nameStr, nameBuf, NAME_LEN);
    DSMeterSpec_t spec(_dsMeterID, swVersion, hwVersion, apiVersion, nameStr);
    return spec;
  } // getDSMeterSpec

  int DS485Proxy::getGroupCount(dsid_t _dsMeterID, const int _zoneID) {

    uint16_t zoneId;
    uint8_t virtualZoneId, numberOfGroups;
    uint8_t name[NAME_LEN];

    int ret = ZoneInfo_by_id_sync(m_dsmApiHandle, _dsMeterID, _zoneID,
                                  &zoneId, &virtualZoneId, &numberOfGroups, name);
    checkResultCode(ret);

    // TODO: libdsm:
    // assert 0 <= numberOfGroups < GroupIDStandardMax

    return numberOfGroups;
  } // getGroupCount

  std::vector<int> DS485Proxy::getGroups(dsid_t _dsMeterID, const int _zoneID) {

    std::vector<int> result;

    int numGroups = getGroupCount(_dsMeterID, _zoneID);
    log(std::string("DSMeter has ") + intToString(numGroups) + " groups");

    uint8_t groupId;
    for(int iGroup = 0; iGroup < numGroups; iGroup++) {
      int ret = ZoneGroupInfo_by_index_sync(m_dsmApiHandle, _dsMeterID, _zoneID, iGroup,
                                            &groupId, NULL, NULL);
      checkResultCode(ret);

      result.push_back(groupId);
    }
    return result;
  } // getGroups

  std::vector<int> DS485Proxy::getGroupsOfDevice(dsid_t _dsMeterID, const int _deviceID) {
    uint8_t groups[GROUPS_LEN];
    int ret = DeviceInfo_by_device_id_sync(m_dsmApiHandle, _dsMeterID, _deviceID,
                                           NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                           groups, NULL, NULL, NULL);
    checkResultCode(ret);
    std::vector<int> result;
    for (int iByte = 0; iByte < GROUPS_LEN; iByte++) {
      uint8_t byte = groups[iByte];
      for(int iBit = 0; iBit < 8; iBit++) {
        if(byte & (1 << iBit)) {
          result.push_back((iByte * 8 + iBit) + 1);
        }
      }
    }
    return result;
  } // getGroupsOfDevice

  int DS485Proxy::getZoneCount(dsid_t _dsMeterID) {
    uint8_t zoneCount;
    int ret = ZoneCount_sync(m_dsmApiHandle, _dsMeterID, &zoneCount);
    checkResultCode(ret);
    return zoneCount;
  } // getZoneCount

  std::vector<int> DS485Proxy::getZones(dsid_t _dsMeterID) {
    std::vector<int> result;

    int numZones = getZoneCount(_dsMeterID);
    log(std::string("DSMeter has ") + intToString(numZones) + " zones");

    uint16_t zoneId;
    for(int iZone = 0; iZone < numZones; iZone++) {
      int ret = ZoneInfo_by_index_sync(m_dsmApiHandle, _dsMeterID, iZone,
                                       &zoneId, NULL, NULL, NULL);
    checkResultCode(ret);

      result.push_back(zoneId);
      log("received ZoneID: " + uintToString(zoneId));
    }
    return result;
  } // getZones

  int DS485Proxy::getDevicesCountInZone(dsid_t _dsMeterID, const int _zoneID) {
    
    uint16_t numberOfDevices;
    int ret = ZoneDeviceCount_all_sync(m_dsmApiHandle, _dsMeterID, _zoneID, &numberOfDevices);
    checkResultCode(ret);
    
    return numberOfDevices;
  } // getDevicesCountInZone

  std::vector<int> DS485Proxy::getDevicesInZone(dsid_t _dsMeterID, const int _zoneID) {

    std::vector<int> result;

    int numDevices = getDevicesCountInZone(_dsMeterID, _zoneID);
    log(std::string("Found ") + intToString(numDevices) + " devices in zone.");
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      uint16_t deviceId;
      int ret = DeviceInfo_by_index_sync(m_dsmApiHandle, _dsMeterID, _zoneID, iDevice, &deviceId,
                                         NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      checkResultCode(ret);
      result.push_back(deviceId);
    }
    return result;
  } // getDevicesInZone

  void DS485Proxy::setZoneID(dsid_t _dsMeterID, const devid_t _deviceID, const int _zoneID) {

    int ret = DeviceProperties_set_zone_sync(m_dsmApiHandle, _dsMeterID, _deviceID, _zoneID);
    checkResultCode(ret);
  } // setZoneID

  void DS485Proxy::createZone(dsid_t _dsMeterID, const int _zoneID) {

    int ret = ZoneModify_add_sync(m_dsmApiHandle, _dsMeterID, _zoneID);
    checkResultCode(ret);
  } // createZone

  void DS485Proxy::removeZone(dsid_t _dsMeterID, const int _zoneID) {
    int ret = ZoneModify_remove_sync(m_dsmApiHandle, _dsMeterID, _zoneID);
    checkResultCode(ret);
  } // removeZone

  dss_dsid_t DS485Proxy::getDSIDOfDevice(dsid_t _dsMeterID, const int _deviceID) {

    dsid_t dsid;
    int ret = DeviceInfo_by_index_sync(m_dsmApiHandle, _dsMeterID, 0 /* all devices */, _deviceID,
                                       NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, dsid.id);
    checkResultCode(ret);

    dss_dsid_t dss_dsid;
    dsid_helper::toDssDsid(dsid, dss_dsid);
    return dss_dsid;
  } // getDSIDOfDevice

  int DS485Proxy::getLastCalledScene(const int _dsMeterID, const int _zoneID, const int _groupID) {
    // TODO: libdsm-api
    return 42;
  } // getLastCalledScene

  unsigned long DS485Proxy::getPowerConsumption(dsid_t _dsMeterID) {
    uint32_t power;
    int ret = CircuitEnergyMeterValue_get_sync(m_dsmApiHandle, _dsMeterID, &power, NULL);
    checkResultCode(ret);

    return power;
  } // getPowerConsumption

  void DS485Proxy::requestPowerConsumption() {
#if 0
    // TODO: libdsm-api

    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(0);
    cmdFrame.getHeader().setBroadcast(true);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterGetPowerConsumption);
    sendFrame(cmdFrame);
#endif
  } // requestPowerConsumption

  unsigned long DS485Proxy::getEnergyMeterValue(dsid_t _dsMeterID) {
    uint32_t energy;
    int ret = CircuitEnergyMeterValue_get_sync(m_dsmApiHandle, _dsMeterID, NULL, &energy);
    checkResultCode(ret);

    return energy;
  } // getEnergyMeterValue

  void DS485Proxy::requestEnergyMeterValue() {
#if 0
    // TODO: libdsm-api

    DS485CommandFrame cmdFrame;
    cmdFrame.getHeader().setDestination(0);
    cmdFrame.getHeader().setBroadcast(true);
    cmdFrame.setCommand(CommandRequest);
    cmdFrame.getPayload().add<uint8_t>(FunctionDSMeterGetEnergyMeterValue);
    sendFrame(cmdFrame);
#endif
  } // requestEnergyMeterValue

  bool DS485Proxy::getEnergyBorder(const int _dsMeterID, int& _lower, int& _upper) {
    log("getEnergyBorder(): not implemented yet");
    return false;
  } // getEnergyBorder

  int DS485Proxy::getSensorValue(const Device& _device, const int _sensorID) {
    log("getSensorValue(): not implemented yet");
    return 0;
  } // getSensorValue

  void DS485Proxy::addToGroup(dsid_t _dsMeterID, const int _groupID, const int _deviceID) {
    int ret = DeviceGroupMembershipModify_add_sync(m_dsmApiHandle, _dsMeterID, _deviceID, _groupID);
    checkResultCode(ret);
  } // addToGroup

  void DS485Proxy::removeFromGroup(dsid_t _dsMeterID, const int _groupID, const int _deviceID) {
    int ret = DeviceGroupMembershipModify_remove_sync(m_dsmApiHandle, _dsMeterID, _deviceID, _groupID);
    checkResultCode(ret);
  } // removeFromGroup

  int DS485Proxy::addUserGroup(const int _dsMeterID) {
    return 0;
  } // addUserGroup

  void DS485Proxy::removeUserGroup(const int _dsMeterID, const int _groupID) {

  } // removeUserGroup

  void DS485Proxy::removeInactiveDevices(dsid_t _dsMeterID) {

    int ret = CircuitRemoveInactiveDevices_sync(m_dsmApiHandle, _dsMeterID);
    checkResultCode(ret);
  }

  void DS485Proxy::initialize() {
    Subsystem::initialize();

    m_dsmApiHandle = DsmApiInitialize();
    if (!m_dsmApiHandle) {
      log("Couldn't init dsmapi connection");
      return;
    }

    // TODO: libdsm
    m_connection = "tcp://192.168.2.124:8442";
      
    int result = DsmApiOpen(m_dsmApiHandle, m_connection.c_str(), 0);
    if (result < 0) {
      log("Couldn't open dsmapi connection");
      return;
    }
    log("Successfully connected to " + m_connection);
    
   
    // register callbacks
    DsmApiRegisterBusStateCallback(m_dsmApiHandle, DS485Proxy::busStateCallback, this);
    DsmApiRegisterBusChangeCallback(m_dsmApiHandle, DS485Proxy::busChangeCallback, this);

    DsmApiRegisterCallback(m_dsmApiHandle, EVENT_DEVICE_ACCESSIBILITY, EVENT_DEVICE_ACCESSIBILITY_ON, 
                           (void*)DS485Proxy::eventDeviceAccessibilityOnCallback, this);
    DsmApiRegisterCallback(m_dsmApiHandle, EVENT_DEVICE_ACCESSIBILITY, EVENT_DEVICE_ACCESSIBILITY_OFF, 
                           (void*)DS485Proxy::eventDeviceAccessibilityOffCallback, this);
    m_dsmApiReady = true;
  } // initialize

  void DS485Proxy::doStart() {    
    busReady();
  } // doStart

  void DS485Proxy::busReady() {
    ModelEvent* pEvent = new ModelEvent(ModelEvent::etBusReady);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // busReady

  void DS485Proxy::shutdown() {
    if (m_dsmApiReady) {
      DsmApiClose(m_dsmApiHandle);
      DsmApiCleanup(m_dsmApiHandle);

      m_dsmApiHandle = NULL;
    }
  }

  void DS485Proxy::busStateCallback(void* _userData, bus_state_t _state) {
    static_cast<DS485Proxy*>(_userData)->handleBusState(_state);
  }

  void DS485Proxy::handleBusState(bus_state_t _state) {
    switch (_state) {
      case DS485_ISOLATED:
        log("STATE: ISOLATED");
        break;
      case DS485_CONNECTED:
        log("STATE: CONNECTED");
        break;
      case DS485_ACTIVE:
        log("STATE: ACTIVE");
        break;
      case DS485_JOIN:
        log("STATE: JOIN");
        break;
      default:
        log("STATE: *UNKNOWN*");
        break;
    }
  }

  void DS485Proxy::busChangeCallback(void* _userData, dsid_t *_id, int _flag) {
    static_cast<DS485Proxy*>(_userData)->handleBusChange(_id, _flag);
  }
  void DS485Proxy::handleBusChange(dsid_t *_id, int _flag) {
    std::string s = dsid_helper::toString(*_id);
    if (_flag) {
      s += "left";
    }	else	{
      s += "joined";
    }
    s += " bus";
    
    log(s);
  }

  void DS485Proxy::eventDeviceAccessibilityOffCallback(uint8_t _errorCode, uint16_t _deviceID, uint16_t _zoneID,
                                                       uint32_t _deviceDSID, void* _userData) {
    
    static_cast<DS485Proxy*>(_userData)->eventDeviceAccessibilityOff(_errorCode, _deviceID, _zoneID, _deviceDSID);
  }

  void DS485Proxy::eventDeviceAccessibilityOff(uint8_t _errorCode, uint16_t _deviceID, uint16_t _zoneID,
                                               uint32_t _deviceDSID) {
    printf("Device 0x%08x (DeviceId: %d in Zone: %d) became inactive\n", _deviceDSID, _deviceID, _zoneID);
  }

  void DS485Proxy::eventDeviceAccessibilityOnCallback(uint8_t _errorCode, uint16_t _deviceID, uint16_t _zoneID,
                                                      uint32_t _deviceDSID, void* _userData) {
    static_cast<DS485Proxy*>(_userData)->eventDeviceAccessibilityOn(_errorCode, _deviceID, _zoneID, _deviceDSID);
  }
  
  void DS485Proxy::eventDeviceAccessibilityOn(uint8_t _errorCode, uint16_t _deviceID, uint16_t _zoneID,
                                              uint32_t _deviceDSID) {
#if 0
    ModelEventWithDSID* pEvent = new ModelEventWithDSID(ModelEvent::etNewDevice, );
    pEvent->addParameter(zoneID);
    pEvent->addParameter(devID);
    pEvent->addParameter(functionID);
    raiseModelEvent(pEvent);
#endif

    printf("Device 0x%08x (DeviceId: %d in Zone: %d) became active\n", _deviceDSID, _deviceID, _zoneID);
  }


} // namespace dss
