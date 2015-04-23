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

#include <unistd.h>
#include <boost/make_shared.hpp>

#include "dsstructuremodifyingbusinterface.h"

#include "dsbusinterface.h"
#include "src/model/modulator.h"
#include "src/model/apartment.h"
#include "src/foreach.h"

#define BROADCAST_SLEEP_MICROSECONDS    50000 // 50ms

namespace dss {

  //================================================== DSStructureModifyingBusInterface

  void DSStructureModifyingBusInterface::setZoneID(const dsuid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    boost::shared_ptr<DSMeter> pMeter = DSS::getInstance()->getApartment().getDSMeterByDSID(_dsMeterID);
    int ret;
    if (pMeter->getApiVersion() >= 0x301) {
      ret = DeviceProperties_set_zone_sync(m_DSMApiHandle, _dsMeterID, _deviceID, _zoneID, 30);
    } else {
      ret = DeviceProperties_set_zone(m_DSMApiHandle, _dsMeterID, _deviceID, _zoneID);
    }
    DSBusInterface::checkResultCode(ret);
  } // setZoneID

  void DSStructureModifyingBusInterface::createZone(const dsuid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = ZoneModify_add(m_DSMApiHandle, _dsMeterID, _zoneID);
    DSBusInterface::checkResultCode(ret);
  } // createZone

  void DSStructureModifyingBusInterface::removeZone(const dsuid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = ZoneModify_remove(m_DSMApiHandle, _dsMeterID, _zoneID);

    if (_dsMeterID == DSUID_BROADCAST) {
      DSBusInterface::checkBroadcastResultCode(ret);
    } else {
      DSBusInterface::checkResultCode(ret);
    }
  } // removeZone

  void DSStructureModifyingBusInterface::addToGroup(const dsuid_t& _dsMeterID, const int _groupID, const int _deviceID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    boost::shared_ptr<DSMeter> pMeter = DSS::getInstance()->getApartment().getDSMeterByDSID(_dsMeterID);
    int ret = 0;
    if (pMeter->getApiVersion() >= 0x301) {
      ret = DeviceGroupMembershipModify_add_sync(m_DSMApiHandle, _dsMeterID, _deviceID, _groupID, 30);
    } else {
      ret = DeviceGroupMembershipModify_add(m_DSMApiHandle, _dsMeterID, _deviceID, _groupID);
    }
    sleep(1); // #2578 prevent dS485 bus flooding with multiple requests
    DSBusInterface::checkResultCode(ret);
  } // addToGroup

  void DSStructureModifyingBusInterface::removeFromGroup(const dsuid_t& _dsMeterID, const int _groupID, const int _deviceID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = DeviceGroupMembershipModify_remove(m_DSMApiHandle, _dsMeterID, _deviceID, _groupID);
    DSBusInterface::checkResultCode(ret);
  } // removeFromGroup

  void DSStructureModifyingBusInterface::removeDeviceFromDSMeter(const dsuid_t& _dsMeterID, const int _deviceID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    if (_dsMeterID == DSUID_NULL) {
      return;
    }

    int ret = CircuitRemoveDevice_by_id(m_DSMApiHandle, _dsMeterID, _deviceID);
    DSBusInterface::checkResultCode(ret);
  } // removeDeviceFromDSMeter

  void DSStructureModifyingBusInterface::removeDeviceFromDSMeters(const dsuid_t& _deviceDSID)
  {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = CircuitRemoveDevice_by_dsuid(m_DSMApiHandle, DSUID_BROADCAST, _deviceDSID);
    DSBusInterface::checkBroadcastResultCode(ret);
  } // removeDeviceFromDSMeters

  void DSStructureModifyingBusInterface::sceneSetName(uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneNumber, const std::string& _name) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::string nameStr = truncateUTF8String(_name, 19);
    int ret = ZoneGroupSceneProperties_set_name(m_DSMApiHandle, DSUID_BROADCAST, _zoneID, _groupID, _sceneNumber, (unsigned char*)nameStr.c_str());
    DSBusInterface::checkBroadcastResultCode(ret);
  } // sceneSetName
  
  void DSStructureModifyingBusInterface::deviceSetName(dsuid_t _meterDSID, devid_t _deviceID, const std::string& _name) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::string nameStr = truncateUTF8String(_name, 19);
    int ret = DeviceProperties_set_name(m_DSMApiHandle, _meterDSID, _deviceID, (unsigned char*)nameStr.c_str());
    DSBusInterface::checkResultCode(ret);
  } // deviceSetName

  void DSStructureModifyingBusInterface::meterSetName(dsuid_t _meterDSID, const std::string& _name) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::string nameStr = truncateUTF8String(_name, 19);
    int ret = dSMProperties_set_name(m_DSMApiHandle, _meterDSID, (unsigned char*)nameStr.c_str());
    DSBusInterface::checkResultCode(ret);
  } // meterSetName

  void DSStructureModifyingBusInterface::createGroup(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID, const std::string& _name) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if (m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = ZoneGroupModify_add(m_DSMApiHandle, DSUID_BROADCAST, _zoneID, _groupID, _standardGroupID);
    DSBusInterface::checkBroadcastResultCode(ret);
    std::string nameStr = truncateUTF8String(_name, 19);
    ret = ZoneGroupProperties_set_name(m_DSMApiHandle, DSUID_BROADCAST, _zoneID, _groupID, (unsigned char*)nameStr.c_str());
    DSBusInterface::checkBroadcastResultCode(ret);
  } // createGroup

  void DSStructureModifyingBusInterface::groupSetStandardID(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    int ret;
    if (m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    ret = ZoneGroupProperties_set_state_machine(m_DSMApiHandle, DSUID_BROADCAST, _zoneID, _groupID, _standardGroupID);
    DSBusInterface::checkBroadcastResultCode(ret);
    usleep(BROADCAST_SLEEP_MICROSECONDS);
  } // groupSetStandardID

  void DSStructureModifyingBusInterface::groupSetName(uint16_t _zoneID, uint8_t _groupID, const std::string& _name) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    int ret;
    if (m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::string nameStr = truncateUTF8String(_name, 19);
    ret = ZoneGroupProperties_set_name(m_DSMApiHandle, DSUID_BROADCAST, _zoneID, _groupID, (unsigned char*)nameStr.c_str());
    DSBusInterface::checkBroadcastResultCode(ret);
    usleep(BROADCAST_SLEEP_MICROSECONDS);
  } // groupSetName

  void DSStructureModifyingBusInterface::removeGroup(uint16_t _zoneID, uint8_t _groupID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = ZoneGroupModify_remove(m_DSMApiHandle, DSUID_BROADCAST, _zoneID, _groupID);
    DSBusInterface::checkBroadcastResultCode(ret);
  } // removeGroup

  void DSStructureModifyingBusInterface::setButtonSetsLocalPriority(const dsuid_t& _dsMeterID, const devid_t _deviceID, bool _setsPriority) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = DeviceProperties_set_button_set_local_priority(m_DSMApiHandle, _dsMeterID, _deviceID, _setsPriority ? 1 : 0);
    DSBusInterface::checkResultCode(ret);
  } // setButtonSetsLocalPriority

  void DSStructureModifyingBusInterface::setButtonCallsPresent(const dsuid_t& _dsMeterID, const devid_t _deviceID, bool _callsPresent) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = DeviceProperties_set_button_set_no_coming_home_call(m_DSMApiHandle, _dsMeterID, _deviceID, _callsPresent ? 0 : 1);
    DSBusInterface::checkResultCode(ret);
  } // setButtonCallsPresent

  void DSStructureModifyingBusInterface::setZoneHeatingConfig(const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingConfigSpec_t _spec)
  {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = ControllerHeating_set_config(m_DSMApiHandle, DSUID_BROADCAST, _ZoneID,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            (_spec.EmergencyValue >= 100) ? _spec.EmergencyValue : 150);
    DSBusInterface::checkBroadcastResultCode(ret);
    usleep(BROADCAST_SLEEP_MICROSECONDS);
    ret = ControllerHeating_set_config(m_DSMApiHandle, _dsMeterID, _ZoneID,
        _spec.ControllerMode, _spec.Kp, _spec.Ts, _spec.Ti, _spec.Kd,
        _spec.Imin, _spec.Imax, _spec.Ymin, _spec.Ymax,
        _spec.AntiWindUp, _spec.KeepFloorWarm, _spec.SourceZoneId,
        _spec.Offset, _spec.ManualValue, _spec.EmergencyValue);
    DSBusInterface::checkResultCode(ret);

    if (m_pModelMaintenance) {
      boost::shared_ptr<ZoneHeatingConfigSpec_t> spec = boost::make_shared<ZoneHeatingConfigSpec_t>();
      *spec = _spec;

      ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etControllerConfig, _dsMeterID);
      pEvent->addParameter(_ZoneID);
      pEvent->setSingleObjectParameter(spec);
      m_pModelMaintenance->addModelEvent(pEvent);
    }
  } // setZoneHeatingConfig

  void DSStructureModifyingBusInterface::setZoneHeatingState(const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingStateSpec_t _spec)
  {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = ControllerHeating_set_state(m_DSMApiHandle, _dsMeterID, _ZoneID,
        _spec.State);
    DSBusInterface::checkResultCode(ret);

    if (m_pModelMaintenance) {
      ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etControllerState, _dsMeterID);
      pEvent->addParameter(_ZoneID);
      pEvent->addParameter(_spec.State);
      m_pModelMaintenance->addModelEvent(pEvent);
    }
  } // setZoneHeatingState

  void DSStructureModifyingBusInterface::setZoneHeatingOperationModes(const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingOperationModeSpec_t _spec)
  {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = ControllerHeating_set_operation_modes(m_DSMApiHandle, _dsMeterID, _ZoneID,
        _spec.OpMode0, _spec.OpMode1, _spec.OpMode2, _spec.OpMode3,
        _spec.OpMode4, _spec.OpMode5, _spec.OpMode6, _spec.OpMode7,
        _spec.OpMode8, _spec.OpMode9, _spec.OpModeA, _spec.OpModeB,
        _spec.OpModeC, _spec.OpModeD, _spec.OpModeE, _spec.OpModeF
        );
    DSBusInterface::checkResultCode(ret);

    if (m_pModelMaintenance) {
      boost::shared_ptr<ZoneHeatingOperationModeSpec_t> spec = boost::make_shared<ZoneHeatingOperationModeSpec_t>();
      *spec = _spec;

      ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etControllerValues, _dsMeterID);
      pEvent->addParameter(_ZoneID);
      pEvent->setSingleObjectParameter(spec);
      m_pModelMaintenance->addModelEvent(pEvent);
    }
  } // setZoneHeatingOperationModes

  void DSStructureModifyingBusInterface::setZoneSensor(
                                const uint16_t _zoneID,
                                const uint8_t _sensorType,
                                const dsuid_t& _sensorDSUID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }

    int ret = ZoneProperties_set_zone_sensor(m_DSMApiHandle, DSUID_BROADCAST,
                                             _zoneID, _sensorType,
                                             _sensorDSUID);
    DSBusInterface::checkBroadcastResultCode(ret);
    usleep(BROADCAST_SLEEP_MICROSECONDS);
    if (m_pModelMaintenance) {
      m_pModelMaintenance->addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    }
  }

  void DSStructureModifyingBusInterface::resetZoneSensor(
                                            const uint16_t _zoneID,
                                            const uint8_t _sensorType) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }

    int ret = ZoneProperties_reset_zone_sensor(m_DSMApiHandle, DSUID_BROADCAST,
                                             _zoneID, _sensorType);
    DSBusInterface::checkBroadcastResultCode(ret);
    usleep(BROADCAST_SLEEP_MICROSECONDS);
    if (m_pModelMaintenance) {
      m_pModelMaintenance->addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    }
  }

  void DSStructureModifyingBusInterface::setModelMaintenace(ModelMaintenance* _modelMaintenance) {
      m_pModelMaintenance = _modelMaintenance;
  }

  void DSStructureModifyingBusInterface::clusterSetName(uint8_t _clusterID, const std::string& _name) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    int ret;
    if (m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::string nameStr = truncateUTF8String(_name, 19);
    ret = ClusterProperties_set_name(m_DSMApiHandle, DSUID_BROADCAST, _clusterID, (unsigned char*)nameStr.c_str());
    DSBusInterface::checkBroadcastResultCode(ret);
    usleep(BROADCAST_SLEEP_MICROSECONDS);
  }

  void DSStructureModifyingBusInterface::clusterSetStandardID(uint8_t _clusterID, uint8_t _standardGroupID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    int ret;
    if (m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    ret = ClusterProperties_set_state_machine(m_DSMApiHandle, DSUID_BROADCAST, _clusterID, _standardGroupID);
    DSBusInterface::checkBroadcastResultCode(ret);
    usleep(BROADCAST_SLEEP_MICROSECONDS);
  }

  void DSStructureModifyingBusInterface::clusterSetProperties(uint8_t _clusterID, uint16_t _location,
                                                              uint16_t _floor, uint16_t _protectionClass) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    int ret;
    if (m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    ret = ClusterProperties_set_location_class(m_DSMApiHandle, DSUID_BROADCAST, _clusterID, _location, _floor, _protectionClass);
    DSBusInterface::checkBroadcastResultCode(ret);
    usleep(BROADCAST_SLEEP_MICROSECONDS);
  }

  void DSStructureModifyingBusInterface::clusterSetLockedScenes(uint8_t _clusterID,
                                                                const std::vector<int> _lockedScenes) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    int ret;
    if (m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    uint8_t lockedScenes[16] = {};
    foreach (int scene, _lockedScenes) {
      lockedScenes[scene / 8] |= 1 << (scene % 8);
    }
    ret = ClusterProperties_set_scene_lock(m_DSMApiHandle, DSUID_BROADCAST, _clusterID, lockedScenes);
    DSBusInterface::checkBroadcastResultCode(ret);
    usleep(BROADCAST_SLEEP_MICROSECONDS);
  }

}  // namespace dss
