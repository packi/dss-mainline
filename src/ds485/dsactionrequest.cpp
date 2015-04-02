/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "dsactionrequest.h"
#include "dsbusinterface.h"

#include "src/dsidhelper.h"
#include "src/logger.h"

#include "src/model/group.h"
#include "src/model/device.h"
#include "src/model/scenehelper.h"

namespace dss {

  //================================================== DSActionRequest

  void DSActionRequest::callScene(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint16_t _scene, const std::string _token, const bool _force) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      if(_force) {
        ret = ZoneGroupActionRequest_action_force_call_scene(m_DSMApiHandle, DSUID_BROADCAST, pGroup->getZoneID(), pGroup->getID(), 0, _scene);
      } else {
        ret = ZoneGroupActionRequest_action_call_scene(m_DSMApiHandle, DSUID_BROADCAST, pGroup->getZoneID(), pGroup->getID(), 0, _scene);
      }
      DSBusInterface::checkBroadcastResultCode(ret);
      if (m_pBusEventSink) {
        m_pBusEventSink->onGroupCallScene(NULL, DSUID_NULL, pGroup->getZoneID(),
                                          pGroup->getID(), 0, _category,
                                          _scene, _origin, _token, _force);
      }
    } else if(pDevice)  {
      if(_force) {
        ret = DeviceActionRequest_action_force_call_scene(m_DSMApiHandle, pDevice->getDSMeterDSID(), pDevice->getShortAddress(), _scene);
      } else {
        ret = DeviceActionRequest_action_call_scene(m_DSMApiHandle, pDevice->getDSMeterDSID(), pDevice->getShortAddress(), _scene);
      }
      DSBusInterface::checkResultCode(ret);
      if (m_pBusEventSink) {
        m_pBusEventSink->onDeviceCallScene(NULL, pDevice->getDSMeterDSID(),
                                           pDevice->getShortAddress(), 0,
                                            _category, _scene, _origin, _token,
                                           _force);
      }
    }
  }

  void DSActionRequest::callSceneMin(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint16_t _scene, const std::string _token) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_call_scene_min(m_DSMApiHandle, DSUID_BROADCAST, pGroup->getZoneID(), pGroup->getID(), 0, _scene);
      DSBusInterface::checkBroadcastResultCode(ret);
      if (m_pBusEventSink) {
        m_pBusEventSink->onGroupCallScene(NULL, DSUID_NULL, pGroup->getZoneID(),
                                          pGroup->getID(), 0, _category,
                                          _scene, _origin, _token, false);
      } else if(pDevice) {
        // TODO extend dsm api, dsm and vdsm to support command
        // DeviceActionRequest_action_call_sceneMin(...)
      }
    }
  }

  void DSActionRequest::saveScene(AddressableModelItem *pTarget, const callOrigin_t _origin, const uint16_t _scene, const std::string _token) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_save_scene(m_DSMApiHandle, DSUID_BROADCAST, pGroup->getZoneID(), pGroup->getID(), 0, _scene);
      DSBusInterface::checkBroadcastResultCode(ret);
    } else if(pDevice) {
      ret = DeviceActionRequest_action_save_scene(m_DSMApiHandle, pDevice->getDSMeterDSID(), pDevice->getShortAddress(), _scene);
      DSBusInterface::checkResultCode(ret);
    }
  }

  void DSActionRequest::increaseOutputChannelValue(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, uint8_t _channel, const std::string _token) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_opc_inc(m_DSMApiHandle, DSUID_BROADCAST, pGroup->getZoneID(), pGroup->getID(), 0, _channel);
      DSBusInterface::checkBroadcastResultCode(ret);
    } else if(pDevice) {
      ret = DeviceActionRequest_action_opc_inc(m_DSMApiHandle, pDevice->getDSMeterDSID(), pDevice->getShortAddress(), _channel);
      DSBusInterface::checkResultCode(ret);
    }
  }
  void DSActionRequest::decreaseOutputChannelValue(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, uint8_t _channel, const std::string _token) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_opc_dec(m_DSMApiHandle, DSUID_BROADCAST, pGroup->getZoneID(), pGroup->getID(), 0, _channel);
      DSBusInterface::checkBroadcastResultCode(ret);
    } else if(pDevice) {
      ret = DeviceActionRequest_action_opc_dec(m_DSMApiHandle, pDevice->getDSMeterDSID(), pDevice->getShortAddress(), _channel);
      DSBusInterface::checkResultCode(ret);
    }
  }
  void DSActionRequest::stopOutputChannelValue(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, uint8_t _channel, const std::string _token) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_opc_stop(m_DSMApiHandle, DSUID_BROADCAST, pGroup->getZoneID(), pGroup->getID(), 0, _channel);
      DSBusInterface::checkBroadcastResultCode(ret);
    } else if(pDevice) {
      ret = DeviceActionRequest_action_opc_stop(m_DSMApiHandle, pDevice->getDSMeterDSID(), pDevice->getShortAddress(), _channel);
      DSBusInterface::checkResultCode(ret);
    }
  }

  void DSActionRequest::undoScene(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint16_t _scene, const std::string _token) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup = dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_undo_scene_number(m_DSMApiHandle, DSUID_BROADCAST, pGroup->getZoneID(), pGroup->getID(), 0, _scene);
      DSBusInterface::checkBroadcastResultCode(ret);
      if (m_pBusEventSink) {
        m_pBusEventSink->onGroupUndoScene(NULL, DSUID_NULL, pGroup->getZoneID(),
                                          pGroup->getID(), 0, _category,
                                          _scene, true, _origin, _token);
      }
    } else if(pDevice)  {
      ret = DeviceActionRequest_action_undo_scene_number(m_DSMApiHandle, pDevice->getDSMeterDSID(), pDevice->getShortAddress(), _scene);
      DSBusInterface::checkResultCode(ret);
      if (m_pBusEventSink) {
        m_pBusEventSink->onDeviceUndoScene(NULL, pDevice->getDSMeterDSID(),
                                           pDevice->getShortAddress(),
                                           0, _category, _scene, true, _origin,
                                           _token);
      }
    }
  }

  void DSActionRequest::undoSceneLast(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup = dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_undo_scene(m_DSMApiHandle, DSUID_BROADCAST, pGroup->getZoneID(), pGroup->getID(), 0);
      DSBusInterface::checkBroadcastResultCode(ret);
      if (m_pBusEventSink) {
        m_pBusEventSink->onGroupUndoScene(NULL, DSUID_NULL, pGroup->getZoneID(),
                                          pGroup->getID(), 0, _category,
                                          -1, false, _origin, _token);
      }
    } else if(pDevice)  {
      ret = DeviceActionRequest_action_undo_scene(m_DSMApiHandle, pDevice->getDSMeterDSID(), pDevice->getShortAddress());
      DSBusInterface::checkResultCode(ret);
      if (m_pBusEventSink) {
        m_pBusEventSink->onDeviceUndoScene(NULL, pDevice->getDSMeterDSID(),
                                           pDevice->getShortAddress(),
                                           0, _category, -1, false, _origin,
                                           _token);
      }
    }
  }

  void DSActionRequest::blink(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_blink(m_DSMApiHandle, DSUID_BROADCAST, pGroup->getZoneID(), pGroup->getID(), _origin);
      DSBusInterface::checkBroadcastResultCode(ret);
      if (m_pBusEventSink) {
        m_pBusEventSink->onGroupBlink(NULL, DSUID_NULL, pGroup->getZoneID(),
                                      pGroup->getID(), 0, _category, _origin,
                                      _token);
      }
    } else if(pDevice) {
      ret = DeviceActionRequest_action_blink(m_DSMApiHandle, pDevice->getDSMeterDSID(), pDevice->getShortAddress());
      DSBusInterface::checkResultCode(ret);
      if (m_pBusEventSink) {
        m_pBusEventSink->onDeviceBlink(NULL, pDevice->getDSMeterDSID(),
                                       pDevice->getShortAddress(),
                                       0, _category, _origin, _token);
      }
    }
  }

  void DSActionRequest::setValue(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _value, const std::string _token) {
    int ret;

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    Group *pGroup= dynamic_cast<Group*>(pTarget);
    Device *pDevice = dynamic_cast<Device*>(pTarget);

    if(pGroup) {
      ret = ZoneGroupActionRequest_action_set_outval(m_DSMApiHandle, DSUID_BROADCAST, pGroup->getZoneID(), pGroup->getID(), 0, _value);
      DSBusInterface::checkBroadcastResultCode(ret);
    } else if(pDevice) {
      ret = DeviceActionRequest_action_set_outval(m_DSMApiHandle, pDevice->getDSMeterDSID(), pDevice->getShortAddress(), _value);
      DSBusInterface::checkResultCode(ret);
    }
  }

  void DSActionRequest::pushSensor(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, dsuid_t _sourceID, uint8_t _sensorType, float _sensorValueFloat, const std::string _token) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if (m_DSMApiHandle == NULL) {
      return;
    }

    Group *pGroup= dynamic_cast<Group*>(pTarget);
    if (pGroup) {
      uint16_t convertedSensorValue = SceneHelper::sensorToSystem(_sensorType, _sensorValueFloat);
      uint8_t precisionvalue = SceneHelper::sensorToPrecision(_sensorType);

      int ret = ZoneGroupSensorPush(m_DSMApiHandle, DSUID_BROADCAST, pGroup->getZoneID(), pGroup->getID(),
          _sourceID, _sensorType, convertedSensorValue, precisionvalue);
      DSBusInterface::checkBroadcastResultCode(ret);

      if (m_pBusEventSink) {
        m_pBusEventSink->onZoneSensorValue(NULL, DSUID_NULL, dsuid2str(_sourceID),
            pGroup->getZoneID(), pGroup->getID(), _sensorType,
            convertedSensorValue, precisionvalue,
            _category, _origin);
      }
    }
  } // pushSensor

  void DSActionRequest::setBusEventSink(BusEventSink* _eventSink) {
      m_pBusEventSink = _eventSink;
  }
} // namespace dss
