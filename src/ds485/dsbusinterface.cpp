/*
    Copyright (c) 2009, 2012 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
            Michael Tross, aizo GmbH <michael.tross@aizo.com>

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
#include <digitalSTROM/ds485-client-interface.h>

#include "src/dss.h"
#include "src/logger.h"
#include "src/event.h"
#include "src/propertysystem.h"
#include "src/foreach.h"
#include "src/dsidhelper.h"

#include "src/model/modelevent.h"
#include "src/model/modelconst.h"
#include "src/model/device.h"
#include "src/model/modelmaintenance.h"
#include "src/security/security.h"

#include "dsactionrequest.h"
#include "dsdevicebusinterface.h"
#include "dsmeteringbusinterface.h"
#include "dsstructurequerybusinterface.h"
#include "dsstructuremodifyingbusinterface.h"

#include <sstream>

namespace dss {

  //================================================== DSBusInterface

  DSBusInterface::DSBusInterface(DSS* _pDSS, ModelMaintenance* _pModelMaintenance)
  : ThreadedSubsystem(_pDSS, "DSBusInterface"),
    m_pModelMaintenance(_pModelMaintenance),
    m_dsmApiHandle(NULL),
    m_dsmApiReady(false),
    m_connectionURI("tcp://localhost:8442"),
    m_pBusEventSink(NULL)
  {
    assert(_pModelMaintenance != NULL);

    if(_pDSS != NULL) {

      _pDSS->getPropertySystem().createProperty(getConfigPropertyBasePath() + "connectionURI")
            ->linkToProxy(PropertyProxyReference<std::string>(m_connectionURI, true));
    }
    m_pActionRequestInterface.reset(new DSActionRequest());
    m_pDeviceBusInterface.reset(new DSDeviceBusInterface(m_connectionURI));
    m_pMeteringBusInterface.reset(new DSMeteringBusInterface());
    m_pStructureQueryBusInterface.reset(new DSStructureQueryBusInterface());
    m_pStructureModifyingBusInterface.reset(new DSStructureModifyingBusInterface());
  } // ctor

  void DSBusInterface::checkResultCode(const int _resultCode) {
    if(_resultCode != ERROR_OK) {
      std::string message;
      std::string msgprefix;

      if (_resultCode >= 0) {
        msgprefix = "DSMeter Error [" + intToString(_resultCode) + "] ";
      } else {
        msgprefix = "DS485d/Socket Error [" + intToString(_resultCode) + "] ";
      }

      switch(_resultCode) {

        default:
          message = "unknown error";
          break;

        // libdsm errors
        case ERROR_WRONG_PARAMETER:
          message = "wrong parameter";
          break;
        case ERROR_ZONE_NOT_FOUND:
          message = "zone not found";
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
        case ERROR_OUT_OF_RESOURCES:
          message = "out of resources";
          break;
#if DSM_API_VERSION > 0x106
        case ERROR_PROGRAMMING_MODE_IS_DISABLED:
          message = "programming mode disabled";
          break;
#endif

        // ds485d errors
        case ERROR_RESPONSE_TIMEOUT:
          message = "response timeout";
          break;
        case ERROR_INVALID_HANDLE:
          message = "invalid handle";
          break;
        case ERROR_INVALID_CONNSPEC:
          message = "invalid connection spec";
          break;
        case ERROR_NO_PORT_FOUND:
          message = "no port found";
          break;
        case ERROR_INVALID_ADDRESS:
          message = "invalid address";
          break;
        case ERROR_CREATE_SOCKET:
          message = "error while creating the socket";
          break;
        case ERROR_SOCKET_OPTIONS:
          message = "error while setting socket options";
          break;
        case ERROR_SOCKET_CONNECT:
          message = "error during socket connect";
          break;
        case ERROR_SOCKET_FULL:
          message = "socket full";
          break;
        case ERROR_SOCKET_FAILED:
          message = "socket failed";
          break;
        case ERROR_SOCKET_VANISHED:
          message = "socket vanished";
          break;
        case ERROR_SOCKET_UNKNOWN:
          message = "request can't be executed";
          break;
        case ERROR_INVALID_FD:
          message = "invalid file descriptor";
          break;
        case ERROR_INVALID_PARAMETER:
          message = "invalid parameter";
          break;
      }
      throw BusApiError(msgprefix + message);
    }
  } // checkResultCode

  void DSBusInterface::checkBroadcastResultCode(const int _resultCode) {
    // to be replaced with include from libdsmapi
    static const int kERROR_OK_ASYNC_CALL = NOTICE_CALL_TO_BROADCAST;

    if(_resultCode == kERROR_OK_ASYNC_CALL) {
        return;
    }
    checkResultCode(_resultCode);
  } // checkBroadcastResultCode

  void DSBusInterface::initialize() {
    Subsystem::initialize();
    log("initializing DSBusInterface");

    m_dsmApiHandle = DsmApiInitialize();
    if(!m_dsmApiHandle) {
      log("Couldn't init dsmapi connection");
      return;
    }

    // the callbacks from libdsm-api are running on different thread(s)
    // so we have to login from each callback handler to ensure that
    // DSBusInterface has sufficient rights to access the property tree...
    getDSS().getSecurity().loginAsSystemUser("DSBusInterface needs system rights");
    m_SystemUser = getDSS().getSecurity().getCurrentlyLoggedInUser();

    connectToDS485D();
  } // initialize

  void DSBusInterface::loginFromCallback() {
    User* u = getDSS().getSecurity().getCurrentlyLoggedInUser();
    if (u == NULL) {
      getDSS().getSecurity().signIn(m_SystemUser);
    }
  } // loginFromCallback

  void DSBusInterface::connectToDS485D() {
    if(!m_dsmApiReady) {
      int result = DsmApiOpen(m_dsmApiHandle, m_connectionURI.c_str(), 0);
      if(result < 0) {
        log("Couldn't open dsmapi connection to '" + m_connectionURI + "' result: " + intToString(result));
        return;
      }

      // read out own dsid
      dsid_t ownDSID;
      int ret = DsmApiGetOwnDSID(m_dsmApiHandle, &ownDSID);
      DSBusInterface::checkResultCode(ret);

      log("Successfully connected to " + m_connectionURI +
          " [ " + dsid_helper::toString(ownDSID) + "]");

      m_pActionRequestInterface->setDSMApiHandle(m_dsmApiHandle);
      m_pDeviceBusInterface->setDSMApiHandle(m_dsmApiHandle);
      m_pMeteringBusInterface->setDSMApiHandle(m_dsmApiHandle);
      m_pStructureQueryBusInterface->setDSMApiHandle(m_dsmApiHandle);
      m_pStructureModifyingBusInterface->setDSMApiHandle(m_dsmApiHandle);

      DsmApiCallback_t callback_struct;

      // register callbacks
      callback_struct.function = (void*)DSBusInterface::busStateCallback;
      callback_struct.arg = this;
      DsmApiSetBusStateCallback(m_dsmApiHandle, &callback_struct);
      callback_struct.function = (void*)DSBusInterface::busChangeCallback;
      callback_struct.arg = this;
      DsmApiSetBusChangeCallback(m_dsmApiHandle, &callback_struct);

      EventDeviceAccessibility_on_event_callback_t evDevAccessOn = DSBusInterface::eventDeviceAccessibilityOnCallback;
      callback_struct.function = (void*)evDevAccessOn;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT, EVENT_DEVICE_ACCESSIBILITY,
                        EVENT_DEVICE_ACCESSIBILITY_ON, &callback_struct);
      EventDeviceAccessibility_off_event_callback_t evDevAccessOff = DSBusInterface::eventDeviceAccessibilityOffCallback;
      callback_struct.function = (void*)evDevAccessOff;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT, EVENT_DEVICE_ACCESSIBILITY,
                        EVENT_DEVICE_ACCESSIBILITY_OFF, &callback_struct);
      EventDeviceModelChanged_event_callback_t evDevModelChanged = DSBusInterface::eventDataModelChangedCallback;
      callback_struct.function = (void*)evDevModelChanged;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT, EVENT_DEVICE_MODEL_CHANGED,
                        0, &callback_struct);
      DeviceConfig_set_request_callback_t evDevConfigSet = DSBusInterface::deviceConfigSetCallback;
      callback_struct.function = (void*)evDevConfigSet;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST, DEVICE_CONFIG,
                        DEVICE_CONFIG_SET, &callback_struct);

      ZoneGroupActionRequest_action_call_scene_request_callback_t handleBusCall = DSBusInterface::handleBusCallSceneCallback;
      callback_struct.function = (void*)handleBusCall;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        ZONE_GROUP_ACTION_REQUEST, ZONE_GROUP_ACTION_REQUEST_ACTION_CALL_SCENE,
                        &callback_struct);
      ZoneGroupActionRequest_action_force_call_scene_request_callback_t handleBusCallForced = DSBusInterface::handleBusCallSceneForcedCallback;
      callback_struct.function = (void*)handleBusCallForced;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        ZONE_GROUP_ACTION_REQUEST, ZONE_GROUP_ACTION_REQUEST_ACTION_FORCE_CALL_SCENE,
                        &callback_struct);

      ZoneGroupActionRequest_action_undo_scene_request_callback_t handleBusUndo = DSBusInterface::handleBusUndoSceneCallback;
      callback_struct.function = (void*)handleBusUndo;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        ZONE_GROUP_ACTION_REQUEST, ZONE_GROUP_ACTION_REQUEST_ACTION_UNDO_SCENE,
                        &callback_struct);
      ZoneGroupActionRequest_action_undo_scene_number_request_callback_t handleBusUndoNumber = DSBusInterface::handleBusUndoSceneNumberCallback;
      callback_struct.function = (void*)handleBusUndoNumber;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        ZONE_GROUP_ACTION_REQUEST, ZONE_GROUP_ACTION_REQUEST_ACTION_UNDO_SCENE_NUMBER,
                        &callback_struct);

      EventDeviceLocalAction_event_callback_t localActionCallback = DSBusInterface::handleDeviceLocalActionCallback;
      callback_struct.function = (void*)localActionCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT,
                        EVENT_DEVICE_LOCAL_ACTION, 0, &callback_struct);

      EventDeviceAction_event_callback_t deviceActionCallback = DSBusInterface::handleDeviceActionCallback;
      callback_struct.function = (void*)deviceActionCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT,
                        EVENT_DEVICE_ACTION, 0, &callback_struct);

      DeviceActionRequest_action_call_scene_request_callback_t deviceCallSceneCallback = DSBusInterface::handleDeviceCallSceneCallback;
      callback_struct.function = (void*)deviceCallSceneCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        DEVICE_ACTION_REQUEST, DEVICE_ACTION_REQUEST_ACTION_CALL_SCENE,
                        &callback_struct);
      DeviceActionRequest_action_force_call_scene_request_callback_t deviceCallSceneForcedCallback = DSBusInterface::handleDeviceCallSceneForcedCallback;
      callback_struct.function = (void*)deviceCallSceneForcedCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        DEVICE_ACTION_REQUEST, DEVICE_ACTION_REQUEST_ACTION_FORCE_CALL_SCENE,
                        &callback_struct);

      DeviceProperties_set_name_request_callback_t deviceSetNameCallback = DSBusInterface::handleDeviceSetNameCallback;
      callback_struct.function = (void*)deviceSetNameCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        DEVICE_PROPERTIES, DEVICE_PROPERTIES_SET_NAME,
                        &callback_struct);

      dSMProperties_set_name_request_callback_t dSMSetNameCallback = DSBusInterface::handleDsmSetNameCallback;
      callback_struct.function = (void*)dSMSetNameCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        DSM_PROPERTIES, DSM_PROPERTIES_SET_NAME,
                        &callback_struct);

      CircuitEnergyMeterValue_get_response_callback_t meteringCallback = DSBusInterface::handleCircuitEnergyDataCallback;
      callback_struct.function = (void*)meteringCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_RESPONSE,
                        CIRCUIT_ENERGY_METER_VALUE, CIRCUIT_ENERGY_METER_VALUE_WS_GET,
                        &callback_struct);

      EventDeviceSensor_event_event_callback_t sensorEventCallback = DSBusInterface::handleSensorEventCallback;
      callback_struct.function = (void*)sensorEventCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT,
                        EVENT_DEVICE_SENSOR, EVENT_DEVICE_SENSOR_EVENT,
                        &callback_struct);

      EventDeviceSensor_value_event_callback_t sensorValueCallback = DSBusInterface::handleSensorValueCallback;
      callback_struct.function = (void*)sensorValueCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT,
                        EVENT_DEVICE_SENSOR, EVENT_DEVICE_SENSOR_VALUE,
                        &callback_struct);

      m_dsmApiReady = true;
    }
  }

  void DSBusInterface::doStart() {
    run();
  } // doStart

  void DSBusInterface::execute() {
    const int kSleepTimeBetweenConnectsMS = 200;
    while(!m_Terminated && !m_dsmApiReady) {
      try {
        connectToDS485D();
      } catch(BusApiError& e) {
        log(std::string("Cought exception while connecting: ") + e.what(),
            lsError);
        sleepMS(kSleepTimeBetweenConnectsMS);
      }
    }
    if(m_dsmApiReady) {
      busReady();
    }
  }

  void DSBusInterface::busReady() {
    ModelEvent* pEvent = new ModelEvent(ModelEvent::etBusReady);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // busReady

  void DSBusInterface::shutdown() {
    ThreadedSubsystem::shutdown();
    if(m_dsmApiReady) {
      m_pActionRequestInterface->setDSMApiHandle(NULL);
      m_pDeviceBusInterface->setDSMApiHandle(NULL);
      m_pMeteringBusInterface->setDSMApiHandle(NULL);
      m_pStructureQueryBusInterface->setDSMApiHandle(NULL);
      m_pStructureModifyingBusInterface->setDSMApiHandle(NULL);

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
    loginFromCallback();
    switch (_state) {
      case DS485_STATE_ISOLATED:
        log("STATE: ISOLATED");
        break;
      case DS485_STATE_CONNECTED:
        log("STATE: CONNECTED");
        break;
      case DS485_STATE_ACTIVE:
        log("STATE: ACTIVE");
        break;
      case DS485_STATE_JOIN:
        log("STATE: JOIN");
        break;
      case DS485_STATE_DISCONNECTED:
        log("STATE: DISCONNECTED");
        break;
      default:
        log("STATE: UNKNOWN: " + intToString(_state));
        break;
    }
  }

  void DSBusInterface::busChangeCallback(void* _userData, dsid_t *_id, int _flag) {
    static_cast<DSBusInterface*>(_userData)->handleBusChange(_id, _flag);
  }

  void DSBusInterface::handleBusChange(dsid_t *_id, int _flag) {
    loginFromCallback();
    ModelEvent::EventType eventType;

    if (!DsmApiIsdSM(*_id)) {
      return;
    }

    dss_dsid_t dSMeterID;
    dsid_helper::toDssDsid(*_id, dSMeterID);

    if(_flag) {
      eventType = ModelEvent::etLostDSMeter;
    } else	{
      eventType = ModelEvent::etDS485DeviceDiscovered;
    }

    m_pModelMaintenance->addModelEvent(new ModelEventWithDSID(eventType,
                                                              dSMeterID));
  }

  void DSBusInterface::eventDeviceAccessibilityOffCallback(uint8_t _errorCode, void* _userData,
                                                           dsid_t _sourceDSMeterID, dsid_t _destinationDSMeterID,
                                                           uint16_t _deviceID, uint16_t _zoneID, uint16_t _vendorID, uint32_t _deviceDSID) {
    static_cast<DSBusInterface*>(_userData)->eventDeviceAccessibilityOff(_errorCode, _sourceDSMeterID, _deviceID,
                                                                         _zoneID, _vendorID, _deviceDSID);
  }

  void DSBusInterface::eventDeviceAccessibilityOff(uint8_t _errorCode, dsid_t _dsMeterID, uint16_t _deviceID,
                                                   uint16_t _zoneID, uint16_t _vendorID, uint32_t _deviceDSID) {
    loginFromCallback();
    dss_dsid_t dsMeterID;
    dsid_helper::toDssDsid(_dsMeterID, dsMeterID);

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etLostDevice,
                                                dsMeterID);
    pEvent->addParameter(_zoneID);
    pEvent->addParameter(_deviceID);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::eventDeviceAccessibilityOnCallback(uint8_t _errorCode, void* _userData,
                                                          dsid_t _sourceDSMeterID, dsid_t _destinationDSMeterID,
                                                          uint16_t _deviceID, uint16_t _zoneID, uint16_t _vendorID, uint32_t _deviceDSID) {
    static_cast<DSBusInterface*>(_userData)->eventDeviceAccessibilityOn(_errorCode, _sourceDSMeterID, _deviceID,
                                                                        _zoneID, _vendorID, _deviceDSID);
  }

  void DSBusInterface::eventDeviceAccessibilityOn(uint8_t _errorCode, dsid_t _dsMeterID,
                                                  uint16_t _deviceID, uint16_t _zoneID, uint16_t _vendorID, uint32_t _deviceDSID) {
    loginFromCallback();
    dss_dsid_t dsMeterID;
    dsid_helper::toDssDsid(_dsMeterID, dsMeterID);

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etNewDevice,
                                                dsMeterID);
    pEvent->addParameter(_zoneID);
    pEvent->addParameter(_deviceID);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::eventDataModelChangedCallback(uint8_t _errorCode,
                                                     void* _userData,
                                                     dsid_t _sourceID,
                                                     dsid_t _destinationID,
                                                     uint16_t _shortAddress) {
    static_cast<DSBusInterface*>(_userData)->eventDataModelChanged(_sourceID, _shortAddress);
  }

  void DSBusInterface::eventDataModelChanged(dsid_t _dsMeterID, uint16_t _shortAddress) {
    loginFromCallback();
    dss_dsid_t dsMeterID;
    dsid_helper::toDssDsid(_dsMeterID, dsMeterID);

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceChanged,
                                                dsMeterID);
    pEvent->addParameter(_shortAddress);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::deviceConfigSetCallback(uint8_t _errorCode, void* _userData,
                                               dsid_t _sourceID, dsid_t _destinationID,
                                               uint16_t _deviceID, uint8_t _configClass,
                                               uint8_t _configIndex, uint8_t _value) {
    static_cast<DSBusInterface*>(_userData)->deviceConfigSet(_destinationID, _deviceID,
                                                             _configClass, _configIndex, _value);
  } // deviceConfigSetCallback

  void DSBusInterface::deviceConfigSet(dsid_t _dsMeterID, uint16_t _deviceID,
                                       uint8_t _configClass, uint8_t _configIndex,
                                       uint8_t _value) {
    loginFromCallback();
    dss_dsid_t dsMeterID;
    dsid_helper::toDssDsid(_dsMeterID, dsMeterID);

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceConfigChanged,
                                                dsMeterID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(_configClass);
    pEvent->addParameter(_configIndex);
    pEvent->addParameter(_value);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // deviceConfigSet


  void DSBusInterface::handleBusCallScene(uint8_t _errorCode, dsid_t _sourceID,
                                          uint16_t _zoneID, uint8_t _groupID,
                                          uint16_t _originDeviceId, uint8_t _sceneID,
                                          bool _forced) {
    loginFromCallback();
    if(m_pBusEventSink != NULL) {
      dss_dsid_t dsMeterID;
      dsid_helper::toDssDsid(_sourceID, dsMeterID);
      m_pBusEventSink->onGroupCallScene(this, dsMeterID, _zoneID, _groupID, _originDeviceId, _sceneID, _forced);
    }
  }

  void DSBusInterface::handleBusCallSceneCallback(uint8_t _errorCode, void *_userData, dsid_t _sourceID,
                                                  dsid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                                  uint16_t _originDeviceId, uint8_t _sceneID) {
    static_cast<DSBusInterface*>(_userData)->handleBusCallScene(_errorCode, _sourceID, _zoneID, _groupID,
                                                                _originDeviceId, _sceneID, false);
  }

  void DSBusInterface::handleBusCallSceneForcedCallback(uint8_t _errorCode, void *_userData, dsid_t _sourceID,
                                                  dsid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                                  uint16_t _originDeviceId, uint8_t _sceneID) {
    static_cast<DSBusInterface*>(_userData)->handleBusCallScene(_errorCode, _sourceID, _zoneID, _groupID,
                                                                _originDeviceId, _sceneID, true);
  }

  void DSBusInterface::handleBusUndoScene(uint8_t _errorCode, dsid_t _sourceID,
                                          uint16_t _zoneID, uint8_t _groupID,
                                          uint16_t _originDeviceId, uint8_t _sceneID,
                                          bool _explicit) {
    loginFromCallback();
    if(m_pBusEventSink != NULL) {
      dss_dsid_t dsMeterID;
      dsid_helper::toDssDsid(_sourceID, dsMeterID);
      m_pBusEventSink->onGroupUndoScene(this, dsMeterID, _zoneID, _groupID, _originDeviceId, _sceneID, _explicit);
    }
  }

  void DSBusInterface::handleBusUndoSceneCallback(uint8_t _errorCode, void *_userData, dsid_t _sourceID,
                                                  dsid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                                  uint16_t _originDeviceId) {
    static_cast<DSBusInterface*>(_userData)->handleBusUndoScene(_errorCode, _sourceID, _zoneID, _groupID,
                                                                _originDeviceId, 255, false);
  }

  void DSBusInterface::handleBusUndoSceneNumberCallback(uint8_t _errorCode, void *_userData, dsid_t _sourceID,
                                                        dsid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                                        uint16_t _originDeviceId, uint8_t _sceneID) {
    static_cast<DSBusInterface*>(_userData)->handleBusUndoScene(_errorCode, _sourceID, _zoneID, _groupID,
                                                                _originDeviceId, _sceneID, true);
  }

  void DSBusInterface::handleDeviceSetName(dsid_t _destinationID,
                                           uint16_t _deviceID,
                                           std::string _name) {
    loginFromCallback();
    dss_dsid_t dsMeterID;
    dsid_helper::toDssDsid(_destinationID, dsMeterID);
    m_pModelMaintenance->onDeviceNameChanged(dsMeterID, _deviceID, _name);
  } // handleDeviceSetName

  void DSBusInterface::handleDeviceSetNameCallback(uint8_t _errorCode,
                                                   void* _userData,
                                                   dsid_t _sourceID,
                                                   dsid_t _destinationID,
                                                   uint16_t _deviceID,
                                                   const uint8_t* _name) {
    const char* name = reinterpret_cast<const char*>(_name);
    std::string nameStr;
    if(name != NULL) {
      nameStr = name;
    }
    static_cast<DSBusInterface*>(_userData)->handleDeviceSetName(_destinationID,
                                                                 _deviceID,
                                                                 nameStr);
  } // handleDeviceSetNameCallback

  void DSBusInterface::handleDsmSetName(dsid_t _destinationID,
                                        std::string _name) {
    loginFromCallback();
    dss_dsid_t dsMeterID;
    dsid_helper::toDssDsid(_destinationID, dsMeterID);
    m_pModelMaintenance->onDsmNameChanged(dsMeterID, _name);
  } //handleDsmSetName

  void DSBusInterface::handleDsmSetNameCallback(uint8_t _errorCode,
                                                void *_userData,
                                                dsid_t _sourceID,
                                                dsid_t _destinationID,
                                                const uint8_t *_name) {
    const char* name = reinterpret_cast<const char*>(_name);
    std::string nameStr;
    if(name != NULL) {
      nameStr = name;
    }
    static_cast<DSBusInterface*>(_userData)->handleDsmSetName(_destinationID,
                                                              nameStr);
  } // handleDsmSetNameCallback

  void DSBusInterface::handleDeviceLocalAction(dsid_t _dsMeterID, uint16_t _deviceID, uint8_t _state) {
    loginFromCallback();
    dss_dsid_t dsmDSID;
    if (_state > 2) {
      // invalid device local action value
      return;
    }
    dsid_helper::toDssDsid(_dsMeterID, dsmDSID);
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etCallSceneDeviceLocal, dsmDSID);
    pEvent->addParameter(_deviceID);
    if (_state == 0) {
      pEvent->addParameter(SceneLocalOff);
    } else if (_state == 1) {
      pEvent->addParameter(SceneLocalOn);
    } else if (_state == 2) {
      pEvent->addParameter(SceneStop);
    }
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::handleDeviceLocalActionCallback(uint8_t _errorCode, void* _userData,
                                                       dsid_t _sourceID, dsid_t _destinationID,
                                                       uint16_t _deviceID, uint16_t _zoneID,
                                                       uint8_t _state) {
    static_cast<DSBusInterface*>(_userData)->handleDeviceLocalAction(_sourceID, _deviceID, _state);
  }

  void DSBusInterface::handleDeviceAction(dsid_t _dsMeterID, uint16_t _deviceID,
                                          uint8_t _buttonNr, uint8_t _clickType) {
    loginFromCallback();
    dss_dsid_t dsmDSID;
    dsid_helper::toDssDsid(_dsMeterID, dsmDSID);
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etButtonClickDevice, dsmDSID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(_buttonNr);
    pEvent->addParameter(_clickType);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::handleDeviceActionCallback(uint8_t _errorCode, void* _userData,
                                                       dsid_t _sourceID, dsid_t _destinationID,
                                                       uint16_t _deviceID, uint16_t _zoneID, uint8_t _groupID,
                                                       uint8_t _buttonNr, uint8_t _clickType) {
    static_cast<DSBusInterface*>(_userData)->handleDeviceAction(_sourceID, _deviceID, _buttonNr, _clickType);
  }

  void DSBusInterface::handleDeviceCallScene(dsid_t _dsMeterID, uint16_t _deviceID,
                                             uint8_t _sceneID, bool _forced) {
    loginFromCallback();
    dss_dsid_t dsmDSID;
    dsid_helper::toDssDsid(_dsMeterID, dsmDSID);
    m_pBusEventSink->onDeviceCallScene(this, dsmDSID, _deviceID, 0, _sceneID, _forced);
  }

  void DSBusInterface::handleDeviceCallSceneCallback(uint8_t _errorCode, void* _userData,
                                                     dsid_t _sourceID, dsid_t _destinationID,
                                                     uint16_t _deviceID, uint8_t _sceneID) {
    static_cast<DSBusInterface*>(_userData)->handleDeviceCallScene(_destinationID, _deviceID, _sceneID, false);
  }

  void DSBusInterface::handleDeviceCallSceneForcedCallback(uint8_t _errorCode, void* _userData,
                                                     dsid_t _sourceID, dsid_t _destinationID,
                                                     uint16_t _deviceID, uint8_t _sceneID) {
    static_cast<DSBusInterface*>(_userData)->handleDeviceCallScene(_destinationID, _deviceID, _sceneID, true);
  }


  void DSBusInterface::handleCircuitEnergyData(uint8_t _errorCode,
                                               dsid_t _sourceID, dsid_t _destinationID,
                                               uint32_t _powerW, uint32_t _energyWs) {
    loginFromCallback();
    dss_dsid_t dsMeterID;
    dsid_helper::toDssDsid(_sourceID, dsMeterID);
    m_pBusEventSink->onMeteringEvent(this, dsMeterID, _powerW, _energyWs);
  } // handleCircuitEnergyData

  void DSBusInterface::handleCircuitEnergyDataCallback(uint8_t _errorCode,
                                                void* _userData,
                                                dsid_t _sourceID,
                                                dsid_t _destinationID,
                                                uint32_t _powerW,
                                                uint32_t _energyWs) {
    if (_errorCode == 0) {
      static_cast<DSBusInterface*>(_userData)->
        handleCircuitEnergyData(_errorCode,
                                _sourceID, _destinationID,
                                _powerW, _energyWs);
    }
  } // handleCircuitEnergyDataCallback

  void DSBusInterface::handleSensorEvent(uint8_t _errorCode,
                                         dsid_t _sourceID,
                                         dsid_t _destinationID,
                                         uint16_t _deviceID,
                                         uint8_t _eventIndex) {
    loginFromCallback();
    dss_dsid_t dsMeterID;
    dsid_helper::toDssDsid(_sourceID, dsMeterID);
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceSensorEvent, dsMeterID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(_eventIndex);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // handleSensorEvent

  void DSBusInterface::handleSensorEventCallback(uint8_t _errorCode,
                                                 void* _userData,
                                                 dsid_t _sourceID,
                                                 dsid_t _destinationID,
                                                 uint16_t _deviceID,
                                                 uint8_t _eventIndex) {
    if (_errorCode == 0) {
      static_cast<DSBusInterface*>(_userData)->
        handleSensorEvent(_errorCode, _sourceID, _destinationID,
                          _deviceID, _eventIndex);
    }
  }


  void DSBusInterface::handleSensorValueEvent(uint8_t _errorCode,
                                              dsid_t _sourceID,
                                              dsid_t _destinationID,
                                              uint16_t _deviceID,
                                              uint8_t _sensorIndex,
                                              uint16_t _sensorValue) {
    loginFromCallback();
    dss_dsid_t dsMeterID;
    dsid_helper::toDssDsid(_sourceID, dsMeterID);
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceSensorValue, dsMeterID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(_sensorIndex);
    pEvent->addParameter(_sensorValue);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // handleSensorEvent

  void DSBusInterface::handleSensorValueCallback(uint8_t _errorCode,
                                                 void* _userData,
                                                 dsid_t _sourceID,
                                                 dsid_t _destinationID,
                                                 uint16_t _deviceID,
                                                 uint8_t _sensorIndex,
                                                 uint16_t _sensorValue) {
    if (_errorCode == 0) {
      static_cast<DSBusInterface*>(_userData)->
        handleSensorValueEvent(_errorCode, _sourceID, _destinationID,
                               _deviceID, _sensorIndex, _sensorValue);
    }
  }


  DeviceBusInterface* DSBusInterface::getDeviceBusInterface() {
    return m_pDeviceBusInterface.get();
  } // getDeviceBusInterface

  StructureQueryBusInterface* DSBusInterface::getStructureQueryBusInterface() {
    return m_pStructureQueryBusInterface.get();
  } // getStructureModifyingBusInterface

  MeteringBusInterface* DSBusInterface::getMeteringBusInterface() {
    return m_pMeteringBusInterface.get();
  } // getMeteringBusInterface

  StructureModifyingBusInterface* DSBusInterface::getStructureModifyingBusInterface() {
    return m_pStructureModifyingBusInterface.get();
  } // getStructureModifyingBusInterface

  ActionRequestInterface* DSBusInterface::getActionRequestInterface() {
    return m_pActionRequestInterface.get();
  } // getActionRequestInterface


} // namespace dss
