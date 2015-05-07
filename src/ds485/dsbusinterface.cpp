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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "dsbusinterface.h"

#include <algorithm>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <digitalSTROM/dsm-api-v2/dsm-api-const.h>
#include <digitalSTROM/ds485-client-interface.h>

#include "src/dss.h"
#include "src/logger.h"
#include "src/propertysystem.h"
#include "src/foreach.h"
#include "src/sceneaccess.h"
#include "src/util.h"

#include "src/model/modelevent.h"
#include "src/model/modelconst.h"
#include "src/model/apartment.h"
#include "src/model/device.h"
#include "src/model/modulator.h"
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
    m_SystemUser(NULL),
    m_pModelMaintenance(_pModelMaintenance),
    m_dsmApiHandle(NULL),
    m_dsmApiReady(false),
    m_connectionURI("tcp://localhost:8440"),
    m_pBusEventSink(NULL)
  {
    assert(_pModelMaintenance != NULL);
    if(_pDSS != NULL) {
      _pDSS->getPropertySystem().createProperty(getConfigPropertyBasePath() + "connectionURI")
            ->linkToProxy(PropertyProxyReference<std::string>(m_connectionURI, true));
    }
    m_pActionRequestInterface.reset(new DSActionRequest());
    m_pDeviceBusInterface.reset(new DSDeviceBusInterface());
    m_pMeteringBusInterface.reset(new DSMeteringBusInterface());
    m_pStructureQueryBusInterface.reset(new DSStructureQueryBusInterface());
    m_pStructureModifyingBusInterface.reset(new DSStructureModifyingBusInterface());
    m_pStructureModifyingBusInterface->setModelMaintenace(m_pModelMaintenance);
  } // ctor

  void DSBusInterface::setBusEventSink(BusEventSink* _eventSink) {
      m_pBusEventSink = _eventSink;
      m_pActionRequestInterface->setBusEventSink(_eventSink);
  }

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
        default:
          message = ds485c_strerror(_resultCode);
          break;
      }
      throw BusApiError(msgprefix + message, _resultCode);
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
    if (!m_dsmApiHandle) {
      log("DSBusInterface::initialize: Couldn't init dsmapi handle");
      throw std::runtime_error("DSBusInterface::initialize: Couldn't init dsmapi handle");
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
      dsuid_t ownDSID;
      int ret = DsmApiGetOwnDSUID(m_dsmApiHandle, &ownDSID);
      DSBusInterface::checkResultCode(ret);

      log("Successfully connected to " + m_connectionURI +
          " [ " + dsuid2str(ownDSID) + "]");

      m_pActionRequestInterface->setDSMApiHandle(m_dsmApiHandle);
      m_pDeviceBusInterface->setDSMApiHandle(m_dsmApiHandle);
      m_pMeteringBusInterface->setDSMApiHandle(m_dsmApiHandle);
      m_pStructureQueryBusInterface->setDSMApiHandle(m_dsmApiHandle);
      m_pStructureModifyingBusInterface->setDSMApiHandle(m_dsmApiHandle);

      DsmApiCallback_t callback_struct;

      // register callbacks
      callback_struct.function = (void*)DSBusInterface::busStateCallback;
      callback_struct.arg = this;
      DsmApiSetBusStateCallback(m_dsmApiHandle, &callback_struct, NULL);
      callback_struct.function = (void*)DSBusInterface::busChangeCallback;
      callback_struct.arg = this;
      DsmApiSetBusChangeCallback(m_dsmApiHandle, &callback_struct, NULL);

      EventDeviceAccessibility_on_event_callback_t evDevAccessOn = DSBusInterface::eventDeviceAccessibilityOnCallback;
      callback_struct.function = (void*)evDevAccessOn;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT, EVENT_DEVICE_ACCESSIBILITY,
                        EVENT_DEVICE_ACCESSIBILITY_ON, &callback_struct, NULL);
      EventDeviceAccessibility_off_event_callback_t evDevAccessOff = DSBusInterface::eventDeviceAccessibilityOffCallback;
      callback_struct.function = (void*)evDevAccessOff;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT, EVENT_DEVICE_ACCESSIBILITY,
                        EVENT_DEVICE_ACCESSIBILITY_OFF, &callback_struct, NULL);
      EventTestDevicePresence_event_callback_t evDevPresence = DSBusInterface::eventDevicePresenceCallback;
      callback_struct.function = (void*)evDevPresence;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT, EVENT_TEST_DEVICE_PRESENCE,
                        0, &callback_struct, NULL);
      EventDeviceModelChanged_event_callback_t evDevModelChanged = DSBusInterface::eventDataModelChangedCallback;
      callback_struct.function = (void*)evDevModelChanged;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT, EVENT_DEVICE_MODEL_CHANGED,
                        0, &callback_struct, NULL);
      DeviceConfig_set_request_callback_t evDevConfigSet = DSBusInterface::deviceConfigSetCallback;
      callback_struct.function = (void*)evDevConfigSet;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST, DEVICE_CONFIG,
                        DEVICE_CONFIG_SET, &callback_struct, NULL);

      ZoneGroupActionRequest_action_call_scene_request_callback_t handleBusCall = DSBusInterface::handleBusCallSceneCallback;
      callback_struct.function = (void*)handleBusCall;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        ZONE_GROUP_ACTION_REQUEST, ZONE_GROUP_ACTION_REQUEST_ACTION_CALL_SCENE,
                        &callback_struct, NULL);
      ZoneGroupActionRequest_action_force_call_scene_request_callback_t handleBusCallForced = DSBusInterface::handleBusCallSceneForcedCallback;
      callback_struct.function = (void*)handleBusCallForced;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        ZONE_GROUP_ACTION_REQUEST, ZONE_GROUP_ACTION_REQUEST_ACTION_FORCE_CALL_SCENE,
                        &callback_struct, NULL);

      ZoneGroupActionRequest_action_undo_scene_request_callback_t handleBusUndo = DSBusInterface::handleBusUndoSceneCallback;
      callback_struct.function = (void*)handleBusUndo;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        ZONE_GROUP_ACTION_REQUEST, ZONE_GROUP_ACTION_REQUEST_ACTION_UNDO_SCENE,
                        &callback_struct, NULL);
      ZoneGroupActionRequest_action_undo_scene_number_request_callback_t handleBusUndoNumber = DSBusInterface::handleBusUndoSceneNumberCallback;
      callback_struct.function = (void*)handleBusUndoNumber;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        ZONE_GROUP_ACTION_REQUEST, ZONE_GROUP_ACTION_REQUEST_ACTION_UNDO_SCENE_NUMBER,
                        &callback_struct, NULL);

      ZoneGroupActionRequest_action_blink_request_callback_t handleBusBlink = DSBusInterface::handleBusBlinkCallback;
      callback_struct.function = (void*)handleBusBlink;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        ZONE_GROUP_ACTION_REQUEST, ZONE_GROUP_ACTION_REQUEST_ACTION_BLINK,
                        &callback_struct, NULL);

      EventDeviceLocalAction_event_callback_t localActionCallback = DSBusInterface::handleDeviceLocalActionCallback;
      callback_struct.function = (void*)localActionCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT,
                        EVENT_DEVICE_LOCAL_ACTION, 0, &callback_struct, NULL);

      EventDeviceAction_event_callback_t deviceActionCallback = DSBusInterface::handleDeviceActionCallback;
      callback_struct.function = (void*)deviceActionCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT,
                        EVENT_DEVICE_ACTION, 0, &callback_struct, NULL);

      DeviceActionRequest_action_call_scene_request_callback_t deviceCallSceneCallback = DSBusInterface::handleDeviceCallSceneCallback;
      callback_struct.function = (void*)deviceCallSceneCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        DEVICE_ACTION_REQUEST, DEVICE_ACTION_REQUEST_ACTION_CALL_SCENE,
                        &callback_struct, NULL);
      DeviceActionRequest_action_force_call_scene_request_callback_t deviceCallSceneForcedCallback = DSBusInterface::handleDeviceCallSceneForcedCallback;
      callback_struct.function = (void*)deviceCallSceneForcedCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        DEVICE_ACTION_REQUEST, DEVICE_ACTION_REQUEST_ACTION_FORCE_CALL_SCENE,
                        &callback_struct, NULL);

      DeviceActionRequest_action_blink_request_callback_t deviceBlinkCallback = DSBusInterface::handleDeviceBlinkCallback;
      callback_struct.function = (void*)deviceBlinkCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        DEVICE_ACTION_REQUEST, DEVICE_ACTION_REQUEST_ACTION_BLINK,
                        &callback_struct, NULL);

      DeviceProperties_set_name_request_callback_t deviceSetNameCallback = DSBusInterface::handleDeviceSetNameCallback;
      callback_struct.function = (void*)deviceSetNameCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        DEVICE_PROPERTIES, DEVICE_PROPERTIES_SET_NAME,
                        &callback_struct, NULL);

      dSMProperties_set_name_request_callback_t dSMSetNameCallback = DSBusInterface::handleDsmSetNameCallback;
      callback_struct.function = (void*)dSMSetNameCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        DSM_PROPERTIES, DSM_PROPERTIES_SET_NAME,
                        &callback_struct, NULL);

      CircuitEnergyMeterValue_get_response_callback_t meteringCallback = DSBusInterface::handleCircuitEnergyDataCallback;
      callback_struct.function = (void*)meteringCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_RESPONSE,
                        CIRCUIT_ENERGY_METER_VALUE, CIRCUIT_ENERGY_METER_VALUE_WS_GET,
                        &callback_struct, NULL);

      EventDeviceSensor_event_event_callback_t sensorEventCallback = DSBusInterface::handleSensorEventCallback;
      callback_struct.function = (void*)sensorEventCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT,
                        EVENT_DEVICE_SENSOR, EVENT_DEVICE_SENSOR_EVENT,
                        &callback_struct, NULL);

      EventDeviceSensor_binaryInputEvent_event_callback_t binaryInputCallback = DSBusInterface::handleBinaryInputEventCallback;
      callback_struct.function = (void*)binaryInputCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT,
                        EVENT_DEVICE_SENSOR, EVENT_DEVICE_SENSOR_BINARYINPUTEVENT,
                        &callback_struct, NULL);

      EventDeviceSensor_value_event_callback_t sensorValueCallback = DSBusInterface::handleSensorValueCallback;
      callback_struct.function = (void*)sensorValueCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT,
                        EVENT_DEVICE_SENSOR, EVENT_DEVICE_SENSOR_VALUE,
                        &callback_struct, NULL);

      ZoneGroupSensorPush_request_callback_t sensorValueZoneCallback = DSBusInterface::handleZoneSensorValueCallback;
      callback_struct.function = (void*)sensorValueZoneCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        ZONE_GROUP_SENSOR_PUSH, 0,
                        &callback_struct, NULL);

      ControllerHeating_set_config_request_callback_t heatingConfigCallback = DSBusInterface::handleHeatingControllerConfigCallback;
      callback_struct.function = (void*)heatingConfigCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        CONTROLLER_HEATING, CONTROLLER_HEATING_SET_CONFIG,
                        &callback_struct, NULL);

      ControllerHeating_set_operation_modes_request_callback_t heatingOperationModesCallback = DSBusInterface::handleHeatingControllerOperationModesCallback;
      callback_struct.function = (void*)heatingOperationModesCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        CONTROLLER_HEATING, CONTROLLER_HEATING_SET_OPERATION_MODES,
                        &callback_struct, NULL);

      ControllerHeating_set_state_request_callback_t heatingStateCallback = DSBusInterface::handleHeatingControllerStateCallback;
      callback_struct.function = (void*)heatingStateCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        CONTROLLER_HEATING, CONTROLLER_HEATING_SET_STATE,
                        &callback_struct, NULL);

      EventHeatingControllerState_event_callback_t heatingStateEventCallback = DSBusInterface::handleHeatingControllerStateEventCallback;
      callback_struct.function = (void*)heatingStateEventCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT,
                        EVENT_HEATING_CONTROLLER_STATE, 0,
                        &callback_struct, NULL);

      dSMProperties_set_flags_request_callback_t dSMSetFlagsCallback = DSBusInterface::handleDsmSetFlagsCallback;
      callback_struct.function = (void*)dSMSetFlagsCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        DSM_PROPERTIES, DSM_PROPERTIES_SET_FLAGS,
                        &callback_struct, NULL);

      ClusterProperties_set_configuration_lock_request_callback_t clusterSetConfigrationLockCallback = DSBusInterface::handleClusterSetConfigrationLockCallback;
      callback_struct.function = (void*)clusterSetConfigrationLockCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        CLUSTER_PROPERTIES, CLUSTER_PROPERTIES_SET_CONFIGURATION_LOCK,
                        &callback_struct, NULL);

      ClusterProperties_set_scene_lock_request_callback_t clusterSetSceneLockCallback = DSBusInterface::handleClusterSetSceneLockCallback;
      callback_struct.function = (void*)clusterSetSceneLockCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        CLUSTER_PROPERTIES, CLUSTER_PROPERTIES_SET_SCENE_LOCK,
                        &callback_struct, NULL);

      EventGeneric_request_callback_t genericEventCallback = DSBusInterface::handleGenericEventCallback;
      callback_struct.function = (void*)genericEventCallback;
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        EVENT_GENERIC, 0,
                        &callback_struct, NULL);

      ZoneGroupActionRequest_action_extra_command_request_callback_t actionRequestExtraCmd = DSBusInterface::handleActionRequestExtraCmdCallback;
      callback_struct.function = reinterpret_cast<void *>(actionRequestExtraCmd);
      callback_struct.arg = this;
      DsmApiSetCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                        ZONE_GROUP_ACTION_REQUEST,
                        ZONE_GROUP_ACTION_REQUEST_ACTION_EXTRA_COMMAND,
                        &callback_struct, NULL);

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

  void DSBusInterface::handleBusState(bus_state_t _state) {
    loginFromCallback();
    switch (_state) {
      case DS485_STATE_ISOLATED:
        log("dS485 BusState: ISOLATED", lsError);
        m_pModelMaintenance->addModelEvent(new ModelEvent(ModelEvent::etBusDown));
        break;
      case DS485_STATE_DISCONNECTED:
        log("dS485 BusState: DISCONNECTED", lsError);
        m_pModelMaintenance->addModelEvent(new ModelEvent(ModelEvent::etBusDown));
        break;
      case DS485_STATE_ACTIVE:
        log("dS485 BusState: ACTIVE", lsWarning);
        m_pModelMaintenance->addModelEvent(new ModelEvent(ModelEvent::etBusReady));
        break;
      case DS485_STATE_CONNECTED:
        log("dS485 BusState: CONNECTED");
        break;
      case DS485_STATE_JOIN:
        log("dS485 BusState: JOIN");
        break;
      default:
        log("dS485 BusState: UNKNOWN: " + intToString(_state));
        break;
    }
  }

  void DSBusInterface::busChangeCallback(void* _userData, dsuid_t *_id, int _flag) {
    static_cast<DSBusInterface*>(_userData)->handleBusChange(_id, _flag);
  }

  void DSBusInterface::handleBusChange(dsuid_t *_id, int _flag) {
    loginFromCallback();

    try {
      m_pModelMaintenance->getDSS().getApartment().getDSMeterByDSID(*_id);
    } catch(ItemNotFoundException& e) {
      // only important for dSMs that are joining; a dSM that is not known
      // in the model and that is leaving can be ignored
      if (!_flag) {
        m_pModelMaintenance->addModelEvent(
          new ModelEventWithDSID(ModelEvent::etDS485DeviceDiscovered, *_id));
        return;
      }
    }

    ModelEvent::EventType eventType;
    if (_flag) {
      eventType = ModelEvent::etLostDSMeter;
    } else  {
      eventType = ModelEvent::etDS485DeviceDiscovered;
    }
    m_pModelMaintenance->addModelEvent(new ModelEventWithDSID(eventType, *_id));
  }

  void DSBusInterface::eventDeviceAccessibilityOffCallback(uint8_t _errorCode, void* _userData,
                                                           dsuid_t _sourceDSMeterID, dsuid_t _destinationDSMeterID,
                                                           uint16_t _deviceID, uint16_t _zoneID, uint16_t _vendorID, uint16_t _productID, uint16_t _functionID, uint16_t _version, dsuid_t _deviceDSID) {
    static_cast<DSBusInterface*>(_userData)->eventDeviceAccessibilityOff(_errorCode, _sourceDSMeterID, _deviceID,
                                                                         _zoneID, _vendorID, _deviceDSID);
  }

  void DSBusInterface::eventDeviceAccessibilityOff(uint8_t _errorCode, dsuid_t _dsMeterID, uint16_t _deviceID,
                                                   uint16_t _zoneID, uint16_t _vendorID, dsuid_t _deviceDSID) {
    loginFromCallback();

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etLostDevice,
                                                _dsMeterID);
    pEvent->addParameter(_zoneID);
    pEvent->addParameter(_deviceID);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::eventDevicePresenceCallback(uint8_t _errorCode, void* _userData,
                                                   dsuid_t _sourceDSMeterID, dsuid_t _destinationDSMeterID,
                                                   uint16_t _deviceID, uint8_t _present) {
    static_cast<DSBusInterface*>(_userData)->eventDevicePresence(_errorCode, _sourceDSMeterID, _deviceID,
                                                                 _present);
  }

  void DSBusInterface::eventDevicePresence(uint8_t _errorCode, dsuid_t _dsMeterID, uint16_t _deviceID,
                                           uint8_t _present) {
    loginFromCallback();

    ModelEvent* pEvent;

    if (_present == 0) {
      pEvent = new ModelEventWithDSID(ModelEvent::etLostDevice,  _dsMeterID);
    } else {
      pEvent = new ModelEventWithDSID(ModelEvent::etNewDevice,  _dsMeterID);
    }
    pEvent->addParameter(0);
    pEvent->addParameter(_deviceID);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::eventDeviceAccessibilityOnCallback(uint8_t _errorCode, void* _userData,
                                                          dsuid_t _sourceDSMeterID, dsuid_t _destinationDSMeterID,
                                                          uint16_t _deviceID, uint16_t _zoneID, uint16_t _vendorID, uint16_t _productID, uint16_t _functionID, uint16_t _version, dsuid_t _deviceDSID) {
    static_cast<DSBusInterface*>(_userData)->eventDeviceAccessibilityOn(_errorCode, _sourceDSMeterID, _deviceID,
                                                                        _zoneID, _vendorID, _deviceDSID);
  }

  void DSBusInterface::eventDeviceAccessibilityOn(uint8_t _errorCode, dsuid_t _dsMeterID,
                                                  uint16_t _deviceID, uint16_t _zoneID, uint16_t _vendorID, dsuid_t _deviceDSID) {
    loginFromCallback();

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etNewDevice,
                                                _dsMeterID);
    pEvent->addParameter(_zoneID);
    pEvent->addParameter(_deviceID);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::eventDataModelChangedCallback(uint8_t _errorCode,
                                                     void* _userData,
                                                     dsuid_t _sourceID,
                                                     dsuid_t _destinationID,
                                                     uint16_t _shortAddress) {
    static_cast<DSBusInterface*>(_userData)->eventDataModelChanged(_sourceID, _shortAddress);
  }

  void DSBusInterface::eventDataModelChanged(dsuid_t _dsMeterID, uint16_t _shortAddress) {
    loginFromCallback();

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceChanged,
                                                _dsMeterID);
    pEvent->addParameter(_shortAddress);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::deviceConfigSetCallback(uint8_t _errorCode, void* _userData,
                                               dsuid_t _sourceID, dsuid_t _destinationID,
                                               uint16_t _deviceID, uint8_t _configClass,
                                               uint8_t _configIndex, uint8_t _value) {
    static_cast<DSBusInterface*>(_userData)->deviceConfigSet(_destinationID, _deviceID,
                                                             _configClass, _configIndex, _value);
  } // deviceConfigSetCallback

  void DSBusInterface::deviceConfigSet(dsuid_t _dsMeterID, uint16_t _deviceID,
                                       uint8_t _configClass, uint8_t _configIndex,
                                       uint8_t _value) {
    loginFromCallback();

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceConfigChanged,
                                                _dsMeterID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(_configClass);
    pEvent->addParameter(_configIndex);
    pEvent->addParameter(_value);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // deviceConfigSet


  void DSBusInterface::handleBusCallScene(uint8_t _errorCode, dsuid_t _sourceID,
                                          uint16_t _zoneID, uint8_t _groupID,
                                          uint16_t _originDeviceId, uint8_t _sceneID,
                                          bool _forced) {
    loginFromCallback();
    if(m_pBusEventSink != NULL) {
      m_pBusEventSink->onGroupCallScene(this, _sourceID, _zoneID, _groupID, _originDeviceId, SAC_MANUAL, _sceneID, coDsmApi, "", _forced);
    }
  }

  void DSBusInterface::handleBusCallSceneCallback(uint8_t _errorCode, void *_userData, dsuid_t _sourceID,
                                                  dsuid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                                  uint16_t _originDeviceId, uint8_t _sceneID) {
    static_cast<DSBusInterface*>(_userData)->handleBusCallScene(_errorCode, _sourceID, _zoneID, _groupID,
                                                                _originDeviceId, _sceneID, false);
  }

  void DSBusInterface::handleBusCallSceneForcedCallback(uint8_t _errorCode, void *_userData, dsuid_t _sourceID,
                                                  dsuid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                                  uint16_t _originDeviceId, uint8_t _sceneID) {
    static_cast<DSBusInterface*>(_userData)->handleBusCallScene(_errorCode, _sourceID, _zoneID, _groupID,
                                                                _originDeviceId, _sceneID, true);
  }

  void DSBusInterface::handleBusBlink(uint8_t _errorCode, dsuid_t _sourceID,
                                          uint16_t _zoneID, uint8_t _groupID,
                                          uint16_t _originDeviceId) {
    loginFromCallback();
    if(m_pBusEventSink != NULL) {
      m_pBusEventSink->onGroupBlink(this, _sourceID, _zoneID, _groupID, _originDeviceId, SAC_MANUAL, coDsmApi, "");
    }
  }

  void DSBusInterface::handleBusBlinkCallback(uint8_t _errorCode, void *_userData, dsuid_t _sourceID,
                                                  dsuid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                                  uint16_t _originDeviceId) {
    static_cast<DSBusInterface*>(_userData)->handleBusBlink(_errorCode, _sourceID, _zoneID, _groupID,
                                                            _originDeviceId);
  }

  void DSBusInterface::handleBusUndoScene(uint8_t _errorCode, dsuid_t _sourceID,
                                          uint16_t _zoneID, uint8_t _groupID,
                                          uint16_t _originDeviceId, uint8_t _sceneID,
                                          bool _explicit) {
    loginFromCallback();
    if(m_pBusEventSink != NULL) {
      m_pBusEventSink->onGroupUndoScene(this, _sourceID, _zoneID, _groupID, _originDeviceId, SAC_MANUAL, _sceneID, _explicit, coDsmApi, "");
    }
  }

  void DSBusInterface::handleBusUndoSceneCallback(uint8_t _errorCode, void *_userData, dsuid_t _sourceID,
                                                  dsuid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                                  uint16_t _originDeviceId) {
    static_cast<DSBusInterface*>(_userData)->handleBusUndoScene(_errorCode, _sourceID, _zoneID, _groupID,
                                                                _originDeviceId, 255, false);
  }

  void DSBusInterface::handleBusUndoSceneNumberCallback(uint8_t _errorCode, void *_userData, dsuid_t _sourceID,
                                                        dsuid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                                        uint16_t _originDeviceId, uint8_t _sceneID) {
    static_cast<DSBusInterface*>(_userData)->handleBusUndoScene(_errorCode, _sourceID, _zoneID, _groupID,
                                                                _originDeviceId, _sceneID, true);
  }

  void DSBusInterface::handleDeviceSetName(dsuid_t _destinationID,
                                           uint16_t _deviceID,
                                           std::string _name) {
    loginFromCallback();
    m_pModelMaintenance->onDeviceNameChanged(_destinationID, _deviceID, _name);
  } // handleDeviceSetName

  void DSBusInterface::handleDeviceSetNameCallback(uint8_t _errorCode,
                                                   void* _userData,
                                                   dsuid_t _sourceID,
                                                   dsuid_t _destinationID,
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

  void DSBusInterface::handleDsmSetName(dsuid_t _destinationID,
                                        std::string _name) {
    loginFromCallback();
    m_pModelMaintenance->onDsmNameChanged(_destinationID, _name);
  } //handleDsmSetName

  void DSBusInterface::handleDsmSetNameCallback(uint8_t _errorCode,
                                                void *_userData,
                                                dsuid_t _sourceID,
                                                dsuid_t _destinationID,
                                                const uint8_t *_name) {
    const char* name = reinterpret_cast<const char*>(_name);
    std::string nameStr;
    if(name != NULL) {
      nameStr = name;
    }
    static_cast<DSBusInterface*>(_userData)->handleDsmSetName(_destinationID,
                                                              nameStr);
  } // handleDsmSetNameCallback

  void DSBusInterface::handleDeviceLocalAction(dsuid_t _dsMeterID, uint16_t _deviceID, uint8_t _state) {
    loginFromCallback();
    if (_state > 2) {
      // invalid device local action value
      return;
    }
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etCallSceneDeviceLocal, _dsMeterID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(0); // originDeviceID
    if (_state == 0) {
      pEvent->addParameter(SceneLocalOff); // sceneID
    } else if (_state == 1) {
      pEvent->addParameter(SceneLocalOn);
    } else if (_state == 2) {
      pEvent->addParameter(SceneStop);
    }
    pEvent->addParameter(coDsmApi); // origin
    pEvent->addParameter(false); // force
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::handleDeviceLocalActionCallback(uint8_t _errorCode, void* _userData,
                                                       dsuid_t _sourceID, dsuid_t _destinationID,
                                                       uint16_t _deviceID, uint16_t _zoneID,
                                                       uint8_t _state) {
    static_cast<DSBusInterface*>(_userData)->handleDeviceLocalAction(_sourceID, _deviceID, _state);
  }

  void DSBusInterface::handleDeviceAction(dsuid_t _dsMeterID, uint16_t _deviceID,
                                          uint8_t _buttonNr, uint8_t _clickType) {
    loginFromCallback();
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etButtonClickDevice, _dsMeterID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(_buttonNr);
    pEvent->addParameter(_clickType);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::handleDeviceActionCallback(uint8_t _errorCode, void* _userData,
                                                       dsuid_t _sourceID, dsuid_t _destinationID,
                                                       uint16_t _deviceID, uint16_t _zoneID, uint8_t _groupID,
                                                       uint8_t _buttonNr, uint8_t _clickType) {
    static_cast<DSBusInterface*>(_userData)->handleDeviceAction(_sourceID, _deviceID, _buttonNr, _clickType);
  }

  void DSBusInterface::handleDeviceCallScene(dsuid_t _dsMeterID, uint16_t _deviceID,
                                             uint8_t _sceneID, bool _forced) {
    loginFromCallback();
    m_pBusEventSink->onDeviceCallScene(this, _dsMeterID, _deviceID, 0, SAC_MANUAL, _sceneID, coDsmApi, "", _forced);
  }

  void DSBusInterface::handleDeviceCallSceneCallback(uint8_t _errorCode, void* _userData,
                                                     dsuid_t _sourceID, dsuid_t _destinationID,
                                                     uint16_t _deviceID, uint8_t _sceneID) {
    static_cast<DSBusInterface*>(_userData)->handleDeviceCallScene(_destinationID, _deviceID, _sceneID, false);
  }

  void DSBusInterface::handleDeviceCallSceneForcedCallback(uint8_t _errorCode, void* _userData,
                                                     dsuid_t _sourceID, dsuid_t _destinationID,
                                                     uint16_t _deviceID, uint8_t _sceneID) {
    static_cast<DSBusInterface*>(_userData)->handleDeviceCallScene(_destinationID, _deviceID, _sceneID, true);
  }

  void DSBusInterface::handleDeviceBlink(dsuid_t _dsMeterID, uint16_t _deviceID) {
    loginFromCallback();
    m_pBusEventSink->onDeviceBlink(this, _dsMeterID, _deviceID, 0, SAC_MANUAL, coDsmApi, "");
  }

  void DSBusInterface::handleDeviceBlinkCallback(uint8_t _errorCode, void* _userData,
                                                 dsuid_t _sourceID, dsuid_t _destinationID,
                                                 uint16_t _deviceID) {
    static_cast<DSBusInterface*>(_userData)->handleDeviceBlink(_destinationID, _deviceID);
  }


  void DSBusInterface::handleCircuitEnergyData(uint8_t _errorCode,
                                               dsuid_t _sourceID, dsuid_t _destinationID,
                                               uint32_t _powerW, uint32_t _energyWs) {
    loginFromCallback();
    m_pBusEventSink->onMeteringEvent(this, _sourceID, _powerW, _energyWs);
  } // handleCircuitEnergyData

  void DSBusInterface::handleCircuitEnergyDataCallback(uint8_t _errorCode,
                                                void* _userData,
                                                dsuid_t _sourceID,
                                                dsuid_t _destinationID,
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
                                         dsuid_t _sourceID,
                                         dsuid_t _destinationID,
                                         uint16_t _deviceID,
                                         uint8_t _eventIndex) {
    loginFromCallback();
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceSensorEvent, _sourceID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(_eventIndex);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // handleSensorEvent

  void DSBusInterface::handleSensorEventCallback(uint8_t _errorCode,
                                                 void* _userData,
                                                 dsuid_t _sourceID,
                                                 dsuid_t _destinationID,
                                                 uint16_t _deviceID,
                                                 uint8_t _eventIndex) {
    if (_errorCode == 0) {
      static_cast<DSBusInterface*>(_userData)->
        handleSensorEvent(_errorCode, _sourceID, _destinationID,
                          _deviceID, _eventIndex);
    }
  }

  void DSBusInterface::handleBinaryInputEvent(uint8_t _errorCode,
      dsuid_t _sourceID, dsuid_t _destinationID,
      uint16_t _deviceID, uint8_t _eventIndex, uint8_t _eventType, uint8_t _state) {
    loginFromCallback();
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceBinaryStateEvent, _sourceID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(_eventIndex);
    pEvent->addParameter(_eventType);
    pEvent->addParameter(_state);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // handleSensorEvent

  void DSBusInterface::handleBinaryInputEventCallback(uint8_t _errorCode, void* _userData,
      dsuid_t _sourceID, dsuid_t _destinationID,
      uint16_t _deviceID, uint8_t _eventIndex, uint8_t _eventType, uint8_t _state) {
    if (_errorCode == 0) {
      static_cast<DSBusInterface*>(_userData)->
        handleBinaryInputEvent(_errorCode, _sourceID, _destinationID,
            _deviceID, _eventIndex, _eventType, _state);
    }
  }

  void DSBusInterface::handleSensorValueEvent(uint8_t _errorCode,
                                              dsuid_t _sourceID,
                                              dsuid_t _destinationID,
                                              uint16_t _deviceID,
                                              uint8_t _sensorIndex,
                                              uint16_t _sensorValue) {
    loginFromCallback();
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceSensorValue, _sourceID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(_sensorIndex);
    pEvent->addParameter(_sensorValue);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // handleSensorEvent

  void DSBusInterface::handleSensorValueCallback(uint8_t _errorCode,
                                                 void* _userData,
                                                 dsuid_t _sourceID,
                                                 dsuid_t _destinationID,
                                                 uint16_t _deviceID,
                                                 uint8_t _sensorIndex,
                                                 uint16_t _sensorValue) {
    if (_errorCode == 0) {
      static_cast<DSBusInterface*>(_userData)->
        handleSensorValueEvent(_errorCode, _sourceID, _destinationID,
                               _deviceID, _sensorIndex, _sensorValue);
    }
  }

  void DSBusInterface::handleZoneSensorValueEvent(uint8_t _errorCode,
                                                dsuid_t _sourceID,
                                                dsuid_t _destinationID,
                                                uint16_t _zoneID,
                                                uint8_t _groupID,
                                                dsuid_t _dSUID,
                                                uint8_t _SensorType,
                                                uint16_t _Value,
                                                uint8_t _Precision) {
    loginFromCallback();
    if (m_pBusEventSink != NULL) {
      m_pBusEventSink->onZoneSensorValue(this,
          _sourceID, _dSUID,
          _zoneID, _groupID, _SensorType, _Value, _Precision,
          SAC_MANUAL, coDsmApi);
    }
  } // handleZoneSensorValueEvent

  void DSBusInterface::handleZoneSensorValueCallback(uint8_t _errorCode,
      void* _userData,
      dsuid_t _sourceID,
      dsuid_t _destinationID,
      uint16_t _ZoneId,
      uint8_t _GroupId,
      dsuid_t _dSUID,
      uint8_t _SensorType,
      uint16_t _Value,
      uint8_t _Precision) {
    if (_errorCode == 0) {
      static_cast<DSBusInterface*>(_userData)->
          handleZoneSensorValueEvent(_errorCode, _sourceID, _destinationID,
              _ZoneId, _GroupId, _dSUID,
              _SensorType, _Value, _Precision);
    }
  }

  void DSBusInterface::handleHeatingControllerConfig(uint8_t _errorCode,
      dsuid_t _sourceID, dsuid_t _destinationID,
      uint16_t _ZoneId, uint8_t _ControllerMode,
      uint16_t _Kp, uint8_t _Ts, uint16_t _Ti, uint16_t _Kd,
      uint16_t _Imin, uint16_t _Imax, uint8_t _Ymin, uint8_t _Ymax,
      uint8_t _AntiWindUp, uint8_t _KeepFloorWarm, uint16_t _SourceZoneId,
      uint16_t _Offset, uint8_t _ManualValue, uint8_t _EmergencyValue) {
    loginFromCallback();

    boost::shared_ptr<ZoneHeatingConfigSpec_t> spec = boost::make_shared<ZoneHeatingConfigSpec_t>();
    spec->ControllerMode = _ControllerMode;
    spec->Kp = _Kp;
    spec->Ts = _Ts,
    spec->Ti = _Ti;
    spec->Kd = _Kd;
    spec->Imin = _Imin;
    spec->Imax = _Imax;
    spec->Ymin = _Ymin;
    spec->Ymax = _Ymax;
    spec->AntiWindUp = _AntiWindUp;
    spec->KeepFloorWarm = _KeepFloorWarm;
    spec->SourceZoneId = _SourceZoneId;
    spec->Offset = _Offset;
    spec->ManualValue = _ManualValue;
    spec->EmergencyValue = _EmergencyValue;

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etControllerConfig, _destinationID);
    pEvent->addParameter(_ZoneId);
    pEvent->setSingleObjectParameter(spec);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // handleHeatingControllerConfig

  void DSBusInterface::handleHeatingControllerConfigCallback(uint8_t _errorCode, void *_userData,
      dsuid_t _sourceID, dsuid_t _destinationID,
      uint16_t _ZoneId, uint8_t _ControllerMode,
      uint16_t _Kp, uint8_t _Ts, uint16_t _Ti, uint16_t _Kd,
      uint16_t _Imin, uint16_t _Imax, uint8_t _Ymin, uint8_t _Ymax,
      uint8_t _AntiWindUp, uint8_t _KeepFloorWarm, uint16_t _SourceZoneId,
      uint16_t _Offset, uint8_t _ManualValue, uint8_t _EmergencyValue) {
    if (_errorCode == 0) {
      static_cast<DSBusInterface*>(_userData)->
          handleHeatingControllerConfig(_errorCode, _sourceID, _destinationID, _ZoneId,
              _ControllerMode, _Kp, _Ts, _Ti, _Kd, _Imin, _Imax, _Ymin, _Ymax,
              _AntiWindUp, _KeepFloorWarm, _SourceZoneId, _Offset, _ManualValue, _EmergencyValue);
    }
  } // handleHeatingControllerConfigCallback

  void DSBusInterface::handleHeatingControllerOperationModes(uint8_t _errorCode,
      dsuid_t _sourceID, dsuid_t _destinationID,
      uint16_t _ZoneId,
      uint16_t _OperationMode0, uint16_t _OperationMode1, uint16_t _OperationMode2, uint16_t _OperationMode3,
      uint16_t _OperationMode4, uint16_t _OperationMode5, uint16_t _OperationMode6, uint16_t _OperationMode7,
      uint16_t _OperationMode8, uint16_t _OperationMode9, uint16_t _OperationModeA, uint16_t _OperationModeB,
      uint16_t _OperationModeC, uint16_t _OperationModeD, uint16_t _OperationModeE, uint16_t _OperationModeF) {
    loginFromCallback();

    boost::shared_ptr<ZoneHeatingOperationModeSpec_t> spec = boost::make_shared<ZoneHeatingOperationModeSpec_t>();
    spec->OpMode0 = _OperationMode0;
    spec->OpMode1 = _OperationMode1;
    spec->OpMode2 = _OperationMode2;
    spec->OpMode3 = _OperationMode3;
    spec->OpMode4 = _OperationMode4;
    spec->OpMode5 = _OperationMode5;
    spec->OpMode6 = _OperationMode6;
    spec->OpMode7 = _OperationMode7;
    spec->OpMode8 = _OperationMode8;
    spec->OpMode9 = _OperationMode9;
    spec->OpModeA = _OperationModeA;
    spec->OpModeB = _OperationModeB;
    spec->OpModeC = _OperationModeC;
    spec->OpModeD = _OperationModeD;
    spec->OpModeE = _OperationModeE;
    spec->OpModeF = _OperationModeF;

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etControllerValues, _destinationID);
    pEvent->addParameter(_ZoneId);
    pEvent->setSingleObjectParameter(spec);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // handleHeatingControllerOperationModes

  void DSBusInterface::handleHeatingControllerOperationModesCallback(uint8_t _errorCode, void *_userData,
      dsuid_t _sourceID, dsuid_t _destinationID,
      uint16_t _ZoneId,
      uint16_t _OperationMode0, uint16_t _OperationMode1, uint16_t _OperationMode2, uint16_t _OperationMode3,
      uint16_t _OperationMode4, uint16_t _OperationMode5, uint16_t _OperationMode6, uint16_t _OperationMode7,
      uint16_t _OperationMode8, uint16_t _OperationMode9, uint16_t _OperationModeA, uint16_t _OperationModeB,
      uint16_t _OperationModeC, uint16_t _OperationModeD, uint16_t _OperationModeE, uint16_t _OperationModeF) {
    if (_errorCode == 0) {
      static_cast<DSBusInterface*>(_userData)->
          handleHeatingControllerOperationModes(_errorCode, _sourceID, _destinationID, _ZoneId,
              _OperationMode0, _OperationMode1, _OperationMode2, _OperationMode3,
              _OperationMode4, _OperationMode5, _OperationMode6, _OperationMode7,
              _OperationMode8, _OperationMode9, _OperationModeA, _OperationModeB,
              _OperationModeC, _OperationModeD, _OperationModeE, _OperationModeF);
    }
  } // handleHeatingControllerOperationModesCallback

  void DSBusInterface::handleHeatingControllerState(uint8_t _errorCode,
      dsuid_t _sourceID, dsuid_t _destinationID,
      uint16_t _ZoneId, uint8_t _State) {
    loginFromCallback();
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etControllerState, _destinationID);
    pEvent->addParameter(_ZoneId);
    pEvent->addParameter(_State);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // handleHeatingControllerState

  void DSBusInterface::handleHeatingControllerStateCallback(uint8_t _errorCode, void *_userData,
      dsuid_t _sourceID, dsuid_t _destinationID,
      uint16_t _ZoneId, uint8_t _State) {
    if (_errorCode == 0) {
      static_cast<DSBusInterface*>(_userData)->
          handleHeatingControllerState(_errorCode, _sourceID, _destinationID,
              _ZoneId, _State);
    }
  } // handleHeatingControllerStateCallback

  void DSBusInterface::handleHeatingControllerStateEvent(uint8_t _errorCode,
      dsuid_t _sourceID, dsuid_t _destinationID,
      uint16_t _ZoneId, uint8_t _State) {
    loginFromCallback();
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etControllerState, _sourceID);
    pEvent->addParameter(_ZoneId);
    pEvent->addParameter(_State);
    m_pModelMaintenance->addModelEvent(pEvent);
  } // handleHeatingControllerStateEvent

  void DSBusInterface::handleHeatingControllerStateEventCallback(uint8_t _errorCode, void *_userData,
      dsuid_t _sourceID, dsuid_t _destinationID,
      uint16_t _ZoneId, uint8_t _State) {
    if (_errorCode == 0) {
      static_cast<DSBusInterface*>(_userData)->
          handleHeatingControllerStateEvent(_errorCode, _sourceID, _destinationID,
              _ZoneId, _State);
    }
  } // handleHeatingControllerStateEventCallback


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

  void DSBusInterface::handleDsmSetFlags(dsuid_t _destinationID,
                                         std::bitset<8> _flags) {
    loginFromCallback();
    m_pModelMaintenance->onDsmFlagsChanged(_destinationID, _flags);
  } //handleDsmSetName

  void DSBusInterface::handleDsmSetFlagsCallback(uint8_t _errorCode,
                                                void *_userData,
                                                dsuid_t _sourceID,
                                                dsuid_t _destinationID,
                                                uint8_t _flags) {

    static_cast<DSBusInterface*>(_userData)->handleDsmSetFlags(_destinationID,
                                                        std::bitset<8>(_flags));
  } // handleDsmSetNameCallback

  void DSBusInterface::handleClusterSetConfigrationLock(uint8_t _clusterID,
                                                        uint8_t _configurationLock) {
    loginFromCallback();
    ModelEvent* pEvent = new ModelEvent(ModelEvent::etClusterConfigLock);
    pEvent->addParameter(_clusterID);
    pEvent->addParameter(_configurationLock);
    m_pModelMaintenance->addModelEvent(pEvent);
  } //handleClusterSetConfigrationLock

  void DSBusInterface::handleClusterSetConfigrationLockCallback(uint8_t _errorCode,
                                                                void *_userData,
                                                                dsuid_t _sourceID,
                                                                dsuid_t _destinationID,
                                                                uint8_t _clusterID,
                                                                uint8_t _lock) {

    static_cast<DSBusInterface*>(_userData)->handleClusterSetConfigrationLock(_clusterID,
                                                                              _lock);
  } // handleClusterSetConfigrationLockCallback

  void DSBusInterface::handleClusterSetSceneLock(uint8_t _clusterID,
                                                 const uint8_t _lockedScenes[]) {
    loginFromCallback();
    ModelEvent* pEvent = new ModelEvent(ModelEvent::etClusterLockedScenes);
    pEvent->addParameter(_clusterID);
    for (int i = 0; i < 16; ++i) {
      pEvent->addParameter(_lockedScenes[i]);
    }
    m_pModelMaintenance->addModelEvent(pEvent);
  } //handleClusterSetSceneLock

  void DSBusInterface::handleClusterSetSceneLockCallback(uint8_t _errorCode,
                                                         void *_userData,
                                                         dsuid_t _sourceID,
                                                         dsuid_t _destinationID,
                                                         uint8_t _clusterID,
                                                         const uint8_t _lockedScenes[]) {

    static_cast<DSBusInterface*>(_userData)->handleClusterSetSceneLock(_clusterID,
                                                                       _lockedScenes);
  } // handleClusterSetConfigrationLockCallback

  void DSBusInterface::handleGenericEvent(uint8_t _errorCode,
                                          dsuid_t _sourceID, dsuid_t _destinationID,
                                          uint16_t _EventType, uint8_t _PayloadLength,
                                          const uint8_t _Payload[]) {
    loginFromCallback();

    if (_PayloadLength > PAYLOAD_LEN) {
      log("Payload of GenericEvent exceeded allowed size.", lsWarning);
      return;
    }

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etGenericEvent, _sourceID);
    pEvent->addParameter(_EventType);

    boost::shared_ptr<GenericEventPayload_t> pPayload = boost::make_shared<GenericEventPayload_t>();
    pPayload->length = _PayloadLength;
    memcpy(pPayload->payload, _Payload, std::min(static_cast<size_t>(_PayloadLength),
                                                 sizeof(pPayload->payload)));

    pEvent->setSingleObjectParameter(pPayload);

    m_pModelMaintenance->addModelEvent(pEvent);
  } // handleHeatingControllerStateEvent

  void DSBusInterface::handleGenericEventCallback(uint8_t _errorCode, void *_userData,
      dsuid_t _sourceID, dsuid_t _destinationID,
      uint16_t _EventType, uint8_t _PayloadLength, const uint8_t _Payload[]) {
    if (_errorCode == 0) {
      static_cast<DSBusInterface*>(_userData)->
          handleGenericEvent(_errorCode, _sourceID, _destinationID,
                             _EventType, _PayloadLength, _Payload);
    }
  } // handleHeatingControllerStateEventCallback

  void DSBusInterface::handleActionRequestExtraCmd(const dsuid_t &_sourceId,
                                                   const dsuid_t &_destinationId,
                                                   uint16_t _zoneId,
                                                   uint8_t _groupId,
                                                   uint16_t _originDeviceId,
                                                   uint8_t _command) {
    switch (_command) {
    case 0: // unlock
    case 1: // lock
    {
      ModelEvent* pEvent = new ModelEvent(ModelEvent::etOperationLock);
      pEvent->addParameter(_zoneId);
      pEvent->addParameter(_groupId);
      pEvent->addParameter(_command == 1);
      pEvent->addParameter(coDsmApi);
      m_pModelMaintenance->addModelEvent(pEvent);
      break;
    }
    default:
      log("Unknown Extra Command " + intToString(_command), lsInfo);
      break;
    }
  }

  void DSBusInterface::handleActionRequestExtraCmdCallback(uint8_t _error_code,
                                                           void *_arg,
                                                           dsuid_t _sourceId,
                                                           dsuid_t _destinationId,
                                                           uint16_t _zoneId,
                                                           uint8_t _groupId,
                                                           uint16_t _originDeviceId,
                                                           uint8_t _command) {
      static_cast<DSBusInterface*>(_arg)
          ->handleActionRequestExtraCmd(_sourceId, _destinationId, _zoneId,
                                        _groupId, _originDeviceId, _command);
  }
} // namespace dss
