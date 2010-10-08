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

#include "dsbusinterface.h"

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <digitalSTROM/dsm-api-v2/dsm-api-const.h>

#include "core/dss.h"
#include "core/logger.h"
#include "core/event.h"
#include "core/propertysystem.h"
#include "core/foreach.h"
#include "core/dsidhelper.h"

#include "core/model/modelevent.h"
#include "core/model/modelconst.h"
#include "core/model/device.h"
#include "core/model/group.h"
#include "core/model/zone.h"
#include "core/model/modelmaintenance.h"


// TODO: libdsm
// #include "core/sim/dssim.h"

#include <sstream>


namespace dss {

  const char* FunctionIDToString(const int _functionID); // internal forward declaration

  DSBusInterface::DSBusInterface(DSS* _pDSS, ModelMaintenance* _pModelMaintenance, DSSim* _pDSSim)
  : Subsystem(_pDSS, "DSBusInterface"),
    m_pModelMaintenance(_pModelMaintenance),
    m_pDSSim(_pDSSim),
    m_dsmApiHandle(NULL),
    m_dsmApiReady(false),
    m_connection("tcp://localhost:8442")
  {
    assert(_pModelMaintenance != NULL);
    
    SetBroadcastId(m_broadcastDSID);
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

      _pDSS->getPropertySystem().setStringValue("/system/dsid", "3504175FE0000000DEADBEEF", true, false);
#endif

    }
  } // ctor

  bool DSBusInterface::isReady() {
#if 0
    // TODO: libdsm
    bool simReady = (m_pDSSim != NULL) ? m_pDSSim->isReady() : true;
    bool controllerReady =
      ((m_dsmApiController.getState() == csSlave) ||
       (m_dsmApiController.getState() == csDesignatedMaster) ||
       (m_dsmApiController.getState() == csError));

    return simReady && selfReady && controllerReady;
#endif
    return m_dsmApiReady;
  } // isReady

  uint16_t DSBusInterface::deviceGetParameterValue(devid_t _id, const dss_dsid_t& _dsMeterDSID, int _paramID) {
    // TODO: libdsm
    log("deviceGetParameterValue: not implemented yet");
    return 0;
  } // deviceGetParameterValue

  DeviceSpec_t DSBusInterface::deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID) {

    uint16_t functionId, productId, version;
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsmDSID);
    int ret = DeviceInfo_by_device_id(m_dsmApiHandle, dsmDSID, _id,
                                      NULL, &functionId, &productId, &version,
                                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    checkResultCode(ret);

    DeviceSpec_t spec(functionId, productId, version, _id);
    return spec;
  } // deviceGetSpec

  void DSBusInterface::lockOrUnlockDevice(const Device& _device, const bool _lock) {
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    int ret = DeviceProperties_set_locked_flag(m_dsmApiHandle, dsmDSID, _device.getShortAddress(), _lock);
    checkResultCode(ret);
  } // lockOrUnlockDevice

  bool DSBusInterface::isLocked(boost::shared_ptr<const Device> _device) {

    uint8_t locked;
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device->getDSMeterDSID(), dsmDSID);
    int ret = DeviceInfo_by_device_id(m_dsmApiHandle, dsmDSID, _device->getShortAddress(),
                                      NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                      &locked, NULL, NULL, NULL, NULL);
    checkResultCode(ret);
    return locked;
  } // isLocked

  bool DSBusInterface::isSimAddress(const uint8_t _addr) {
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

  void DSBusInterface::checkResultCode(const int _resultCode) {
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

  void DSBusInterface::setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size) {
    // TODO: libdsm
    log("Not implemented yet: setValueDevice()");
  } // setValueDevice


  std::vector<DSMeterSpec_t> DSBusInterface::getDSMeters() {
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

  DSMeterSpec_t DSBusInterface::getDSMeterSpec(const dsid_t& _dsMeterID) {

    uint32_t hwVersion;
    uint32_t swVersion;
    uint16_t apiVersion;
    uint8_t dsidBuf[DSID_LEN];
    uint8_t nameBuf[NAME_LEN];
    int ret = dSMInfo(m_dsmApiHandle, _dsMeterID, &hwVersion, &swVersion, &apiVersion, dsidBuf, nameBuf);
    checkResultCode(ret);

    // convert to std::string
    char nameStr[NAME_LEN];
    memcpy(nameStr, nameBuf, NAME_LEN);
    DSMeterSpec_t spec(_dsMeterID, swVersion, hwVersion, apiVersion, nameStr);
    return spec;
  } // getDSMeterSpec

  int DSBusInterface::getGroupCount(const dsid_t& _dsMeterID, const int _zoneID) {

    uint16_t zoneId;
    uint8_t virtualZoneId, numberOfGroups;
    uint8_t name[NAME_LEN];

    int ret = ZoneInfo_by_id(m_dsmApiHandle, _dsMeterID, _zoneID, &zoneId, &virtualZoneId, &numberOfGroups, name);
    checkResultCode(ret);

    // TODO: libdsm:
    // assert 0 <= numberOfGroups < GroupIDStandardMax

    return numberOfGroups;
  } // getGroupCount

  std::vector<int> DSBusInterface::getGroups(const dsid_t& _dsMeterID, const int _zoneID) {

    std::vector<int> result;

    int numGroups = getGroupCount(_dsMeterID, _zoneID);
    log(std::string("DSMeter has ") + intToString(numGroups) + " groups");

    uint8_t groupId;
    for(int iGroup = 0; iGroup < numGroups; iGroup++) {
      int ret = ZoneGroupInfo_by_index(m_dsmApiHandle, _dsMeterID, _zoneID, iGroup, &groupId, NULL, NULL);
      checkResultCode(ret);

      result.push_back(groupId);
    }
    return result;
  } // getGroups

  std::vector<int> DSBusInterface::getGroupsOfDevice(const dsid_t& _dsMeterID, const int _deviceID) {
    uint8_t groups[GROUPS_LEN];
    int ret = DeviceInfo_by_device_id(m_dsmApiHandle, _dsMeterID, _deviceID, NULL, NULL, NULL, NULL, 
                                      NULL, NULL, NULL, NULL, groups, NULL, NULL, NULL);
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

  int DSBusInterface::getZoneCount(const dsid_t& _dsMeterID) {
    uint8_t zoneCount;
    int ret = ZoneCount(m_dsmApiHandle, _dsMeterID, &zoneCount);
    checkResultCode(ret);
    return zoneCount;
  } // getZoneCount

  std::vector<int> DSBusInterface::getZones(const dsid_t& _dsMeterID) {
    std::vector<int> result;

    int numZones = getZoneCount(_dsMeterID);
    log(std::string("DSMeter has ") + intToString(numZones) + " zones");

    uint16_t zoneId;
    for(int iZone = 0; iZone < numZones; iZone++) {
      int ret = ZoneInfo_by_index(m_dsmApiHandle, _dsMeterID, iZone,
                                       &zoneId, NULL, NULL, NULL);
    checkResultCode(ret);

      result.push_back(zoneId);
      log("received ZoneID: " + uintToString(zoneId));
    }
    return result;
  } // getZones

  int DSBusInterface::getDevicesCountInZone(const dsid_t& _dsMeterID, const int _zoneID) {

    uint16_t numberOfDevices;
    int ret = ZoneDeviceCount_all(m_dsmApiHandle, _dsMeterID, _zoneID, &numberOfDevices);
    checkResultCode(ret);

    return numberOfDevices;
  } // getDevicesCountInZone

  std::vector<int> DSBusInterface::getDevicesInZone(const dsid_t& _dsMeterID, const int _zoneID) {

    std::vector<int> result;

    int numDevices = getDevicesCountInZone(_dsMeterID, _zoneID);
    log(std::string("Found ") + intToString(numDevices) + " devices in zone.");
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      uint16_t deviceId;
      int ret = DeviceInfo_by_index(m_dsmApiHandle, _dsMeterID, _zoneID, iDevice, &deviceId,
                                    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      checkResultCode(ret);
      result.push_back(deviceId);
    }
    return result;
  } // getDevicesInZone

  void DSBusInterface::setZoneID(const dsid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID) {

    int ret = DeviceProperties_set_zone(m_dsmApiHandle, _dsMeterID, _deviceID, _zoneID);
    checkResultCode(ret);
  } // setZoneID

  void DSBusInterface::createZone(const dsid_t& _dsMeterID, const int _zoneID) {

    int ret = ZoneModify_add(m_dsmApiHandle, _dsMeterID, _zoneID);
    checkResultCode(ret);
  } // createZone

  void DSBusInterface::removeZone(const dsid_t& _dsMeterID, const int _zoneID) {
    int ret = ZoneModify_remove(m_dsmApiHandle, _dsMeterID, _zoneID);
    checkResultCode(ret);
  } // removeZone

  dss_dsid_t DSBusInterface::getDSIDOfDevice(const dsid_t& _dsMeterID, const int _deviceID) {
    dsid_t dsid;
    int ret = DeviceInfo_by_device_id(m_dsmApiHandle, _dsMeterID, _deviceID, NULL, NULL, NULL, NULL,
                                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, dsid.id);
    checkResultCode(ret);

    dss_dsid_t dss_dsid;
    dsid_helper::toDssDsid(dsid, dss_dsid);
    return dss_dsid;
  } // getDSIDOfDevice

  int DSBusInterface::getLastCalledScene(const int _dsMeterID, const int _zoneID, const int _groupID) {
    // TODO: libdsm-api
    return SceneOff;
  } // getLastCalledScene

  unsigned long DSBusInterface::getPowerConsumption(const dsid_t& _dsMeterID) {
    uint32_t power;
    int ret = CircuitEnergyMeterValue_get(m_dsmApiHandle, _dsMeterID, &power, NULL);
    checkResultCode(ret);

    return power;
  } // getPowerConsumption

  void DSBusInterface::requestPowerConsumption() {
    uint32_t power;
    int ret = CircuitEnergyMeterValue_get(m_dsmApiHandle, m_broadcastDSID, &power, NULL);
    checkResultCode(ret);
  } // requestPowerConsumption

  unsigned long DSBusInterface::getEnergyMeterValue(const dsid_t& _dsMeterID) {
    uint32_t energy;
    int ret = CircuitEnergyMeterValue_get(m_dsmApiHandle, _dsMeterID, NULL, &energy);
    checkResultCode(ret);

    return energy;
  } // getEnergyMeterValue

  void DSBusInterface::requestEnergyMeterValue() {
    uint32_t energy;
    int ret = CircuitEnergyMeterValue_get(m_dsmApiHandle, m_broadcastDSID, NULL, &energy);
    checkResultCode(ret);
  } // requestEnergyMeterValue

  bool DSBusInterface::getEnergyBorder(const int _dsMeterID, int& _lower, int& _upper) {
    // TODO: libdsm
    log("getEnergyBorder(): not implemented yet");
    return false;
  } // getEnergyBorder

  int DSBusInterface::getSensorValue(const Device& _device, const int _sensorID) {
    // TODO: libdsm
    log("getSensorValue(): not implemented yet");
    return 0;
  } // getSensorValue

  void DSBusInterface::addToGroup(const dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
    int ret = DeviceGroupMembershipModify_add(m_dsmApiHandle, _dsMeterID, _deviceID, _groupID);
    checkResultCode(ret);
  } // addToGroup

  void DSBusInterface::removeFromGroup(const dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
    int ret = DeviceGroupMembershipModify_remove(m_dsmApiHandle, _dsMeterID, _deviceID, _groupID);
    checkResultCode(ret);
  } // removeFromGroup

  int DSBusInterface::addUserGroup(const int _dsMeterID) {
    log("addUserGroup(): not implemented yet");
    return 0;
  } // addUserGroup

  void DSBusInterface::removeUserGroup(const int _dsMeterID, const int _groupID) {
    log("removeUserGroup(): not implemented yet");
  } // removeUserGroup

  void DSBusInterface::removeInactiveDevices(const dsid_t& _dsMeterID) {

    int ret = CircuitRemoveInactiveDevices(m_dsmApiHandle, _dsMeterID);
    checkResultCode(ret);
  }

  void DSBusInterface::initialize() {
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
    DsmApiRegisterBusStateCallback(m_dsmApiHandle, DSBusInterface::busStateCallback, this);
    DsmApiRegisterBusChangeCallback(m_dsmApiHandle, DSBusInterface::busChangeCallback, this);

    EventDeviceAccessibility_off_response_callback_t evDevAccessOn = DSBusInterface::eventDeviceAccessibilityOnCallback;
    DsmApiRegisterCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT, EVENT_DEVICE_ACCESSIBILITY, 
                           EVENT_DEVICE_ACCESSIBILITY_ON, (void*)evDevAccessOn, this);
    EventDeviceAccessibility_off_response_callback_t evDevAccessOff = DSBusInterface::eventDeviceAccessibilityOffCallback;
    DsmApiRegisterCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT, EVENT_DEVICE_ACCESSIBILITY, 
                           EVENT_DEVICE_ACCESSIBILITY_OFF, (void*)evDevAccessOff, this);
    
    
    ZoneGroupActionRequest_action_call_scene_request_callback_t handleBusCall = DSBusInterface::handleBusCallSceneCallback;
    DsmApiRegisterCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST, 
                           ZONE_GROUP_ACTION_REQUEST, ZONE_GROUP_ACTION_REQUEST_ACTION_CALL_SCENE,
                           (void*)handleBusCall, this);
    
    // TODO: libdsm
    // register callbacks for 
    // - CircuitEnergyMeterValue_get
    // - CircuitPowerMeterValue_get

    m_dsmApiReady = true;
  } // initialize

  void DSBusInterface::doStart() {
    busReady();
  } // doStart

  void DSBusInterface::busReady() {
    ModelEvent* pEvent = new ModelEvent(ModelEvent::etBusReady);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // busReady

  void DSBusInterface::shutdown() {
    if (m_dsmApiReady) {
      DsmApiClose(m_dsmApiHandle);
      DsmApiCleanup(m_dsmApiHandle);

      m_dsmApiHandle = NULL;
    }
  }



  void DSBusInterface::callScene(AddressableModelItem *pTarget, const uint16_t scene) {
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);
    Zone *pZone =  dynamic_cast<Zone*>(pTarget);

    if (pGroup) {
      ZoneGroupActionRequest_action_call_scene(m_dsmApiHandle, m_broadcastDSID, pGroup->getZoneID(), pGroup->getID(), scene);
    } else if (pDevice)	{
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      DeviceActionRequest_action_call_scene(m_dsmApiHandle, dsid, pDevice->getShortAddress(), scene);
    } else if (pZone) {
      ZoneGroupActionRequest_action_call_scene(m_dsmApiHandle, m_broadcastDSID, pGroup->getZoneID(), 0, scene);
    }
		}

  void DSBusInterface::saveScene(AddressableModelItem *pTarget, const uint16_t scene) {
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);
    Zone *pZone =  dynamic_cast<Zone*>(pTarget);

    if (pGroup) {
      ZoneGroupActionRequest_action_call_scene(m_dsmApiHandle, m_broadcastDSID, pGroup->getZoneID(), pGroup->getID(), scene);
    } else if (pDevice) {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      DeviceActionRequest_action_save_scene(m_dsmApiHandle, dsid, pDevice->getShortAddress(), scene);
    } else if (pZone) {
      ZoneGroupActionRequest_action_call_scene(m_dsmApiHandle, m_broadcastDSID, pGroup->getZoneID(), 0, scene);
    }
  }
  void DSBusInterface::undoScene(AddressableModelItem *pTarget)	{
    Group *pGroup = dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);
    Zone *pZone =  dynamic_cast<Zone*>(pTarget);

    if (pGroup) {
      ZoneGroupActionRequest_action_undo_scene(m_dsmApiHandle, m_broadcastDSID, pGroup->getZoneID(), pGroup->getID());
    } else if (pDevice)	{
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      DeviceActionRequest_action_undo_scene(m_dsmApiHandle, dsid, pDevice->getShortAddress());
    } else if (pZone) {
      ZoneGroupActionRequest_action_undo_scene(m_dsmApiHandle, m_broadcastDSID, pGroup->getZoneID(), 0);
    }
  }

  void DSBusInterface::blink(AddressableModelItem *pTarget) {
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);
    Zone *pZone =  dynamic_cast<Zone*>(pTarget);
			
    if (pGroup) {
      ZoneGroupActionRequest_action_blink(m_dsmApiHandle, m_broadcastDSID, pGroup->getZoneID(), pGroup->getID());
    } else if (pDevice) {
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(pDevice->getDSMeterDSID(), dsid);
      DeviceActionRequest_action_blink(m_dsmApiHandle, dsid, pDevice->getShortAddress());
    } else if (pZone) {
      ZoneGroupActionRequest_action_blink(m_dsmApiHandle, m_broadcastDSID, pGroup->getZoneID(), 0);
    }
  }

  void DSBusInterface::increaseValue(AddressableModelItem *pTarget) {
    callScene(pTarget, SceneInc);
  }

  void DSBusInterface::decreaseValue(AddressableModelItem *pTarget) {
    callScene(pTarget, SceneDec);
  }

  void DSBusInterface::setValue(AddressableModelItem *pTarget,const double _value) {
    // TODO: libdsm
    log("DSBusInterface::setValue(): not implemented yet");
  }




  void DSBusInterface::busStateCallback(void* _userData, bus_state_t _state) {
    static_cast<DSBusInterface*>(_userData)->handleBusState(_state);
  }

  void DSBusInterface::handleBusState(bus_state_t _state) {
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

  void DSBusInterface::busChangeCallback(void* _userData, dsid_t *_id, int _flag) {
    static_cast<DSBusInterface*>(_userData)->handleBusChange(_id, _flag);
  }
  void DSBusInterface::handleBusChange(dsid_t *_id, int _flag) {
    std::string s = dsid_helper::toString(*_id);
    if (_flag) {
      s += "left";
    }	else	{
      s += "joined";
    }
    s += " bus";

    log(s);
  }

  void DSBusInterface::eventDeviceAccessibilityOffCallback(uint8_t _errorCode, void* _userData, dsid_t _dsMeterID,
                                                           uint16_t _deviceID, uint16_t _zoneID,uint32_t _deviceDSID) {
    static_cast<DSBusInterface*>(_userData)->eventDeviceAccessibilityOff(_errorCode, _dsMeterID, _deviceID, 
                                                                         _zoneID, _deviceDSID);
  }

  void DSBusInterface::eventDeviceAccessibilityOff(uint8_t _errorCode, dsid_t _dsMeterId, uint16_t _deviceID, 
                                                   uint16_t _zoneID, uint32_t _deviceDSID) {
    printf("Device 0x%08x (DeviceId: %d in Zone: %d) became inactive\n", _deviceDSID, _deviceID, _zoneID);
  }

  void DSBusInterface::eventDeviceAccessibilityOnCallback(uint8_t _errorCode, void* _userData, dsid_t _dsMeterID, 
                                                          uint16_t _deviceID, uint16_t _zoneID, uint32_t _deviceDSID) {
    static_cast<DSBusInterface*>(_userData)->eventDeviceAccessibilityOn(_errorCode, _dsMeterID, _deviceID, 
                                                                        _zoneID, _deviceDSID);
  }

  void DSBusInterface::eventDeviceAccessibilityOn(uint8_t _errorCode, dsid_t _dsMeterID,
                                                  uint16_t _deviceID, uint16_t _zoneID, uint32_t _deviceDSID) {
#if 0
    ModelEventWithDSID* pEvent = new ModelEventWithDSID(ModelEvent::etNewDevice, );
    pEvent->addParameter(zoneID);
    pEvent->addParameter(devID);
    pEvent->addParameter(functionID);
    raiseModelEvent(pEvent);
#endif

    // Device 0x0000ffff (DeviceId: 3792 in Zone: 66) became active
    printf("Device 0x%08x (DeviceId: %d in Zone: %d) became active\n", _deviceDSID, _deviceID, _zoneID);
  }
  
  
  void DSBusInterface::handleBusCallScene(uint8_t _errorCode, dsid_t _sourceID, 
                                          uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneID) {
    log("Scene called: sceneNr " + intToString(_sceneID) + " in zone " + intToString(_zoneID) + ", group " + 
        intToString(_groupID) +  ", dsm " + dsid_helper::toString(_sourceID));
  }

  void DSBusInterface::handleBusCallSceneCallback(uint8_t _errorCode, void *_userData, dsid_t _sourceID, 
                                                  uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneID) {
    
    static_cast<DSBusInterface*>(_userData)->handleBusCallScene(_errorCode, _sourceID, _zoneID, _groupID, _sceneID);
  }


} // namespace dss
