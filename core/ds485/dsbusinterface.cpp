/*
    Copyright (c) 2009, 2010 digitalSTROM.org, Zurich, Switzerland

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
#include "core/model/modelmaintenance.h"

#include "dsactionrequest.h"
#include "dsdevicebusinterface.h"
#include "dsmeteringbusinterface.h"
#include "dsstructurequerybusinterface.h"

// TODO: libdsm
// #include "core/sim/dssim.h"

#include <sstream>


namespace dss {

  //================================================== DSBusInterface

  DSBusInterface::DSBusInterface(DSS* _pDSS, ModelMaintenance* _pModelMaintenance, DSSim* _pDSSim)
  : Subsystem(_pDSS, "DSBusInterface"),
    m_pModelMaintenance(_pModelMaintenance),
    m_pDSSim(_pDSSim),
    m_dsmApiHandle(NULL),
    m_dsmApiReady(false),
    m_connectionURI("tcp://localhost:8442")
  {
    assert(_pModelMaintenance != NULL);

    SetBroadcastId(m_broadcastDSID);
    if(_pDSS != NULL) {

      _pDSS->getPropertySystem().createProperty(getConfigPropertyBasePath() + "connectionURI")
            ->linkToProxy(PropertyProxyReference<std::string>(m_connectionURI, true));
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

  void DSBusInterface::addToGroup(const dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
    int ret = DeviceGroupMembershipModify_add(m_dsmApiHandle, _dsMeterID, _deviceID, _groupID);
    checkResultCode(ret);
  } // addToGroup

  void DSBusInterface::removeFromGroup(const dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
    int ret = DeviceGroupMembershipModify_remove(m_dsmApiHandle, _dsMeterID, _deviceID, _groupID);
    checkResultCode(ret);
  } // removeFromGroup

  void DSBusInterface::removeInactiveDevices(const dsid_t& _dsMeterID) {

    int ret = CircuitRemoveInactiveDevices(m_dsmApiHandle, _dsMeterID);
    checkResultCode(ret);
  } // removeInactiveDevices

  void DSBusInterface::initialize() {
    Subsystem::initialize();

    m_dsmApiHandle = DsmApiInitialize();
    if(!m_dsmApiHandle) {
      log("Couldn't init dsmapi connection");
      return;
    }

    int result = DsmApiOpen(m_dsmApiHandle, m_connectionURI.c_str(), 0);
    if(result < 0) {
      log("Couldn't open dsmapi connection");
      return;
    }
    log("Successfully connected to " + m_connectionURI);

    m_pActionRequestInterface.reset(new DSActionRequest(m_dsmApiHandle));
    m_pDeviceBusInterface.reset(new DSDeviceBusInterface(m_dsmApiHandle));
    m_pMeteringBusInterface.reset(new DSMeteringBusInterface(m_dsmApiHandle));
    m_pStructureQueryBusInterface.reset(new DSStructureQueryBusInterface(m_dsmApiHandle));


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
    if(m_dsmApiReady) {
      DsmApiClose(m_dsmApiHandle);
      DsmApiCleanup(m_dsmApiHandle);

      m_dsmApiHandle = NULL;
    }
  }

  void DSBusInterface::busStateCallback(void* _userData, bus_state_t _state) {
    static_cast<DSBusInterface*>(_userData)->handleBusState(_state);
  }

  // TODO: expose the state on the property-tree
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
    ModelEvent::EventType eventType = ModelEvent::etNewDSMeter;
    if(_flag) {
      eventType = ModelEvent::etLostDSMeter;
    }	else	{
      eventType = ModelEvent::etNewDSMeter;
    }

    m_pModelMaintenance->addModelEvent(new ModelEvent(eventType));
  }

  void DSBusInterface::eventDeviceAccessibilityOffCallback(uint8_t _errorCode, void* _userData,
                                                           dsid_t _sourceDSMeterID, dsid_t _destinationDSMeterID,
                                                           uint16_t _deviceID, uint16_t _zoneID,uint32_t _deviceDSID) {
    static_cast<DSBusInterface*>(_userData)->eventDeviceAccessibilityOff(_errorCode, _sourceDSMeterID, _deviceID,
                                                                         _zoneID, _deviceDSID);
  }

  void DSBusInterface::eventDeviceAccessibilityOff(uint8_t _errorCode, dsid_t _dsMeterID, uint16_t _deviceID, 
                                                   uint16_t _zoneID, uint32_t _deviceDSID) {
    ModelEvent* pEvent = new ModelEvent(ModelEvent::etLostDevice);
    pEvent->addParameter(_zoneID);
    pEvent->addParameter(_deviceID);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::eventDeviceAccessibilityOnCallback(uint8_t _errorCode, void* _userData,
                                                          dsid_t _sourceDSMeterID, dsid_t _destinationDSMeterID,
                                                          uint16_t _deviceID, uint16_t _zoneID, uint32_t _deviceDSID) {
    static_cast<DSBusInterface*>(_userData)->eventDeviceAccessibilityOn(_errorCode, _sourceDSMeterID, _deviceID,
                                                                        _zoneID, _deviceDSID);
  }

  void DSBusInterface::eventDeviceAccessibilityOn(uint8_t _errorCode, dsid_t _dsMeterID,
                                                  uint16_t _deviceID, uint16_t _zoneID, uint32_t _deviceDSID) {
    dss_dsid_t dsMeterID;
    dsid_helper::toDssDsid(_dsMeterID, dsMeterID);

    ModelEvent* pEvent = new ModelEvent(ModelEvent::etNewDevice);
    pEvent->addParameter(_zoneID);
    pEvent->addParameter(_deviceDSID);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::handleBusCallScene(uint8_t _errorCode, dsid_t _sourceID, 
                                          uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneID) {
    ModelEvent* pEvent = new ModelEvent(ModelEvent::etCallSceneGroup);
    pEvent->addParameter(_zoneID);
    pEvent->addParameter(_groupID);
    pEvent->addParameter(_sceneID);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::handleBusCallSceneCallback(uint8_t _errorCode, void *_userData, dsid_t _sourceID,
                                                  dsid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                                  uint8_t _sceneID) {
    static_cast<DSBusInterface*>(_userData)->handleBusCallScene(_errorCode, _sourceID, _zoneID, _groupID, _sceneID);
  }

} // namespace dss
