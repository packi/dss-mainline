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
    m_pDeviceBusInterface.reset(new DSDeviceBusInterface());
    m_pMeteringBusInterface.reset(new DSMeteringBusInterface());
    m_pStructureQueryBusInterface.reset(new DSStructureQueryBusInterface());
    m_pStructureModifyingBusInterface.reset(new DSStructureModifyingBusInterface());
  } // ctor

  void DSBusInterface::checkResultCode(const int _resultCode) {
    if(_resultCode != ERROR_OK) {
      std::string message = "Unknown Error (" + intToString(_resultCode) + ")";
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

  void DSBusInterface::checkBroadcastResultCode(const int _resultCode) {
    // to be replaced with include from libdsmapi
    static const int kERROR_OK_ASYNC_CALL = -2; 

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

    connectToDS485D();
  } // initialize

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

      // register callbacks
      DsmApiRegisterBusStateCallback(m_dsmApiHandle, DSBusInterface::busStateCallback, this);
      DsmApiRegisterBusChangeCallback(m_dsmApiHandle, DSBusInterface::busChangeCallback, this);

      EventDeviceAccessibility_on_event_callback_t evDevAccessOn = DSBusInterface::eventDeviceAccessibilityOnCallback;
      DsmApiRegisterCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT, EVENT_DEVICE_ACCESSIBILITY,
                            EVENT_DEVICE_ACCESSIBILITY_ON, (void*)evDevAccessOn, this);
      EventDeviceAccessibility_off_event_callback_t evDevAccessOff = DSBusInterface::eventDeviceAccessibilityOffCallback;
      DsmApiRegisterCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT, EVENT_DEVICE_ACCESSIBILITY,
                            EVENT_DEVICE_ACCESSIBILITY_OFF, (void*)evDevAccessOff, this);

      ZoneGroupActionRequest_action_call_scene_request_callback_t handleBusCall = DSBusInterface::handleBusCallSceneCallback;
      DsmApiRegisterCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                            ZONE_GROUP_ACTION_REQUEST, ZONE_GROUP_ACTION_REQUEST_ACTION_CALL_SCENE,
                            (void*)handleBusCall, this);

      EventDeviceLocalAction_event_callback_t localActionCallback = DSBusInterface::handleDeviceLocalActionCallback;
      DsmApiRegisterCallback(m_dsmApiHandle, DS485_CONTAINER_EVENT,
                            EVENT_DEVICE_LOCAL_ACTION, 0,
                            (void*)localActionCallback, this);

      DeviceActionRequest_action_call_scene_request_callback_t deviceCallSceneCallback = DSBusInterface::handleDeviceCallSceneCallback;
      DsmApiRegisterCallback(m_dsmApiHandle, DS485_CONTAINER_REQUEST,
                            DEVICE_ACTION_REQUEST, DEVICE_ACTION_REQUEST_ACTION_CALL_SCENE,
                            (void*)deviceCallSceneCallback, this);

      CircuitEnergyMeterValue_get_response_callback_t meteringCallback = DSBusInterface::handleCircuitEnergyDataCallback;
      DsmApiRegisterCallback(m_dsmApiHandle, DS485_CONTAINER_RESPONSE,
                            CIRCUIT_ENERGY_METER_VALUE, CIRCUIT_ENERGY_METER_VALUE_GET,
                            (void*)meteringCallback, this);

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
    dss_dsid_t dsMeterID;
    dsid_helper::toDssDsid(_dsMeterID, dsMeterID);

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etNewDevice,
                                                dsMeterID);
    pEvent->addParameter(_zoneID);
    pEvent->addParameter(_deviceID);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::handleBusCallScene(uint8_t _errorCode, dsid_t _sourceID, 
                                          uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneID) {
    if(m_pBusEventSink != NULL) {
      dss_dsid_t dsMeterID;
      dsid_helper::toDssDsid(_sourceID, dsMeterID);
      m_pBusEventSink->onGroupCallScene(this, dsMeterID, _zoneID, _groupID, _sceneID);
    }
  }

  void DSBusInterface::handleBusCallSceneCallback(uint8_t _errorCode, void *_userData, dsid_t _sourceID,
                                                  dsid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                                  uint8_t _sceneID) {
    static_cast<DSBusInterface*>(_userData)->handleBusCallScene(_errorCode, _sourceID, _zoneID, _groupID, _sceneID);
  }

  void DSBusInterface::handleDeviceLocalAction(dsid_t _sourceID, uint16_t _deviceID, uint8_t _state) {
    dss_dsid_t dsmDSID;
    dsid_helper::toDssDsid(_sourceID, dsmDSID);
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etCallSceneDevice, dsmDSID);
    pEvent->addParameter(_deviceID);
    if(_state == 0) {
      pEvent->addParameter(SceneLocalOff);
    } else {
      pEvent->addParameter(SceneLocalOn);
    }
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::handleDeviceLocalActionCallback(uint8_t _errorCode, void* _userData,
                                                       dsid_t _sourceID, dsid_t _destinationID,
                                                       uint16_t _deviceID, uint16_t _zoneID,
                                                       uint8_t _state) {
    static_cast<DSBusInterface*>(_userData)->handleDeviceLocalAction(_sourceID, _deviceID, _state);
  }

  void DSBusInterface::handleDeviceCallScene(dsid_t _destinationID, uint16_t _deviceID, uint8_t _sceneID) {
    dss_dsid_t dsmDSID;
    dsid_helper::toDssDsid(_destinationID, dsmDSID);
    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etCallSceneDevice, dsmDSID);
    pEvent->addParameter(_deviceID);
    pEvent->addParameter(_sceneID);
    m_pModelMaintenance->addModelEvent(pEvent);
  }

  void DSBusInterface::handleDeviceCallSceneCallback(uint8_t _errorCode, void* _userData,
                                                     dsid_t _sourceID, dsid_t _destinationID,
                                                     uint16_t _deviceID, uint8_t _sceneID) {
    static_cast<DSBusInterface*>(_userData)->handleDeviceCallScene(_destinationID, _deviceID, _sceneID);
  }

  void DSBusInterface::handleCircuitEnergyData(uint8_t _errorCode,
                                               dsid_t _sourceID, dsid_t _destinationID,
                                               uint32_t _powerW, uint32_t _energyWh) {
    dss_dsid_t dsMeterID;
    dsid_helper::toDssDsid(_sourceID, dsMeterID);
    m_pBusEventSink->onMeteringEvent(this, dsMeterID, _powerW, _energyWh);
  } // handleCircuitEnergyData

  void DSBusInterface::handleCircuitEnergyDataCallback(uint8_t _errorCode,
                                                void* _userData,
                                                dsid_t _sourceID,
                                                dsid_t _destinationID,
                                                uint32_t _powerW,
                                                uint32_t _energyWh) {
    static_cast<DSBusInterface*>(_userData)->
      handleCircuitEnergyData(_errorCode,
                              _sourceID, _destinationID,
                              _powerW, _energyWh);
  } // handleCircuitEnergyDataCallback


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
