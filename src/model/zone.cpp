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


#include "zone.h"

#include <vector>

#include "src/dss.h"
#include "src/businterface.h"
#include "src/base.h"
#include "src/util.h"
#include "src/foreach.h"
#include "src/logger.h"
#include "src/model/modelconst.h"
#include "src/model/modelmaintenance.h"
#include "src/propertysystem.h"
#include "src/model/set.h"
#include "src/model/device.h"
#include "src/model/group.h"
#include "src/model/apartment.h"
#include "src/model/modulator.h"

namespace dss {

  static const std::string HeatingSuffix("heatingOperation_");

  //================================================== Zone

  Zone::Zone(const int _id, Apartment* _pApartment)
  : m_ZoneID(_id),
    m_HeatingOperationMode(
      HeatingSuffix+intToString(m_ZoneID),
      HeatingOperationModeInvalid),
    m_pApartment(_pApartment),
    m_HeatingPropValid(false)
  {}

  Zone::~Zone() {
    // we don't own our dsMeters
    m_DSMeters.clear();
  } // dtor

  Set Zone::getDevices() const {
    return Set(m_Devices);
  } // getDevices

  void Zone::addDevice(DeviceReference& _device) {
    boost::shared_ptr<const Device> dev = _device.getDevice();
    int oldZoneID = dev->getZoneID();
    if((oldZoneID != -1) && (oldZoneID != 0)) {
      try {
        boost::shared_ptr<Zone> oldZone = dev->getApartment().getZone(oldZoneID);
        oldZone->removeDevice(_device);
      } catch(std::runtime_error&) {
      }
    }
    if(!contains(m_Devices, _device)) {
      m_Devices.push_back(_device);
    } else {
      // don't warn about multiple additions to zone 0
      if(m_ZoneID != 0) {
        Logger::getInstance()->log("Zone::addDevice: DUPLICATE DEVICE Detected Zone: " + intToString(m_ZoneID) + " device: " + dsuid2str(_device.getDSID()), lsWarning);
      }
    }
    _device.getDevice()->setZoneID(m_ZoneID);
  } // addDevice

  void Zone::addGroup(boost::shared_ptr<Group> _group) {
    // TODO(soon): remove again before R1705
    if (_group->getID() == GroupIDGlobalAppDsRecirculation) {
      return;
    }
    // until here
    if(_group->getZoneID() != m_ZoneID) {
      throw std::runtime_error("Zone::addGroup: ZoneID of _group does not match own");
    }
    m_Groups.push_back(_group);
    if(m_pPropertyNode != NULL) {
      _group->publishToPropertyTree();
    }

  } // addGroup

  void Zone::removeGroup(boost::shared_ptr<Group> _group) {
    std::vector<boost::shared_ptr<Group> >::iterator it = find(m_Groups.begin(), m_Groups.end(), _group);
    if(it != m_Groups.end()) {
      m_Groups.erase(it);
    }
    if(m_pPropertyNode != NULL) {
      PropertyNodePtr groupNode = m_pPropertyNode->getProperty("groups/group" + intToString(_group->getID()));
      if(groupNode != NULL) {
        groupNode->getParentNode()->removeChild(groupNode);
      }
    }
  } // removeGroup

  void Zone::removeDevice(const DeviceReference& _device) {
    auto pos = find(m_Devices.begin(), m_Devices.end(), _device);
    if(pos != m_Devices.end()) {
      if(m_pPropertyNode != NULL) {
        for(std::vector<boost::shared_ptr<Group> >::const_iterator ipGroup = m_Groups.begin(), e = m_Groups.end();
            ipGroup != e; ++ipGroup)
        {
          PropertyNodePtr groupNode = m_pPropertyNode->getProperty("groups/group" + intToString((*ipGroup)->getID()) + "/devices/" + dsuid2str(_device.getDSID()));
          if(groupNode != NULL) {
            groupNode->getParentNode()->removeChild(groupNode);
          }
        }
      }
      m_Devices.erase(pos);
    }
  } // removeDevice

  boost::shared_ptr<Group> Zone::getGroup(const std::string& _name) const {
    for(std::vector<boost::shared_ptr<Group> >::const_iterator ipGroup = m_Groups.begin(), e = m_Groups.end();
        ipGroup != e; ++ipGroup)
    {
        if((*ipGroup)->getName() == _name) {
          return *ipGroup;
        }
    }
    return boost::shared_ptr<Group>();
  } // getGroup

  boost::weak_ptr<Group> Zone::tryGetGroup(const int id) const {
    foreach (auto&& group, m_Groups) {
        if (group->getID() == id) {
          return group;
        }
    }
    return boost::weak_ptr<Group>();
  } // getGroup

  boost::shared_ptr<Group> Zone::getGroup(const int id) const {
    if (auto&& group = tryGetGroup(id).lock()) {
      return group;
    }
    return boost::shared_ptr<Group>(); // TODO(someday): throw
  } // getGroup

  int Zone::getID() const {
    return m_ZoneID;
  } // getID

  void Zone::setZoneID(const int _value) {
    m_ZoneID = _value;
    // key changed -> trigger storage at new location
    m_HeatingOperationMode.store(HeatingSuffix+intToString(m_ZoneID));
  } // setZoneID

  void Zone::addToDSMeter(boost::shared_ptr<DSMeter> _dsMeter) {
    // make sure the zone is connected to the dsMeter
    if(find(m_DSMeters.begin(), m_DSMeters.end(), _dsMeter) == m_DSMeters.end()) {
      m_DSMeters.push_back(_dsMeter);
      if(_dsMeter->getPropertyNode() != NULL) {
        PropertyNodePtr alias = _dsMeter->getPropertyNode()->createProperty("zones/" + intToString(m_ZoneID));
        alias->alias(m_pPropertyNode);
      }
    }
  } // addToDSMeter

  void Zone::removeFromDSMeter(boost::shared_ptr<DSMeter> _dsMeter) {
    m_DSMeters.erase(find(m_DSMeters.begin(), m_DSMeters.end(), _dsMeter));
    if(_dsMeter->getPropertyNode() != NULL) {
      PropertyNodePtr alias = _dsMeter->getPropertyNode()->getProperty("zones/" + intToString(m_ZoneID));
      if(alias != NULL) {
        alias->getParentNode()->removeChild(alias);
      }
    }
    ZoneHeatingProperties_t prop = getHeatingProperties();
    if (prop.m_HeatingControlDSUID == _dsMeter->getDSID()) {
      clearHeatingControlMode();
    }

  } // removeFromDSMeter

  bool Zone::registeredOnDSMeter(boost::shared_ptr<const DSMeter> _dsMeter) const {
    return find(m_DSMeters.begin(), m_DSMeters.end(), _dsMeter) != m_DSMeters.end();
  } // registeredOnDSMeter

  bool Zone::isRegisteredOnAnyMeter() const {
    return !m_DSMeters.empty();
  } // isRegisteredOnAnyMeter

  unsigned long Zone::getPowerConsumption() {
    return getDevices().getPowerConsumption();
  } // getPowerConsumption

  void Zone::nextScene(const callOrigin_t _origin, const SceneAccessCategory _category) {
    getGroup(GroupIDBroadcast)->nextScene(_origin, _category);
  } // nextScene

  void Zone::previousScene(const callOrigin_t _origin, const SceneAccessCategory _category) {
    getGroup(GroupIDBroadcast)->previousScene(_origin, _category);
  } // previousScene

  std::vector<boost::shared_ptr<AddressableModelItem> > Zone::splitIntoAddressableItems() {
    std::vector<boost::shared_ptr<AddressableModelItem> > result;
    result.push_back(getGroup(GroupIDBroadcast));
    return result;
  } // splitIntoAddressableItems

  void Zone::publishToPropertyTree() {
    if(m_pPropertyNode == NULL) {
      if(m_pApartment->getPropertyNode() != NULL) {
        m_pPropertyNode = m_pApartment->getPropertyNode()->createProperty("zones/zone" + intToString(m_ZoneID));
        m_pPropertyNode->createProperty("ZoneID")->setIntegerValue(m_ZoneID);
        m_pPropertyNode->createProperty("name")
          ->linkToProxy(PropertyProxyMemberFunction<Zone, std::string>(*this, &Zone::getName, &Zone::setName));
        m_pPropertyNode->createProperty("devices/");

        foreach(boost::shared_ptr<Group> pGroup, m_Groups) {
          pGroup->publishToPropertyTree();
        }
      }
    }
  } // publishToPropertyTree

  void Zone::removeFromPropertyTree() {
    if(m_pPropertyNode != NULL) {
      m_pPropertyNode->getParentNode()->removeChild(m_pPropertyNode);
      m_pPropertyNode.reset();
    }
  } // removeFromPropertyTree

  ZoneHeatingProperties_t Zone::getHeatingProperties() const {
    return m_HeatingProperties;
  }

  ZoneHeatingStatus_t Zone::getHeatingStatus() const {
    return m_HeatingStatus;
  }

  ZoneSensorStatus_t Zone::getSensorStatus() const {
    return m_SensorStatus;
  }

  bool Zone::isHeatingEnabled() const {
    return (m_HeatingProperties.m_HeatingControlMode != 0); // 0 heating controller off
  }

  void Zone::setHeatingProperties(ZoneHeatingProperties_t& config) {
    m_HeatingProperties = config;
    m_HeatingPropValid = true;
  }

  bool Zone::isHeatingPropertiesValid() const {
    return m_HeatingPropValid;
  }

  void Zone::setHeatingControlMode(const ZoneHeatingConfigSpec_t _spec, dsuid_t ctrlDevice) {
    m_HeatingProperties.m_HeatingControlMode = _spec.ControllerMode;
    m_HeatingProperties.m_Kp = _spec.Kp;
    m_HeatingProperties.m_Ts = _spec.Ts;
    m_HeatingProperties.m_Ti = _spec.Ti;
    m_HeatingProperties.m_Kd = _spec.Kd;
    m_HeatingProperties.m_Imin = _spec.Imin;
    m_HeatingProperties.m_Imax = _spec.Imax;
    m_HeatingProperties.m_Ymin = _spec.Ymin;
    m_HeatingProperties.m_Ymax = _spec.Ymax;
    m_HeatingProperties.m_AntiWindUp = _spec.AntiWindUp;
    m_HeatingProperties.m_KeepFloorWarm = _spec.KeepFloorWarm;
    m_HeatingProperties.m_HeatingMasterZone = _spec.SourceZoneId;
    m_HeatingProperties.m_CtrlOffset = _spec.Offset;
    m_HeatingProperties.m_EmergencyValue = _spec.EmergencyValue;
    m_HeatingProperties.m_ManualValue = _spec.ManualValue;
    m_HeatingProperties.m_HeatingControlDSUID = ctrlDevice;
    m_HeatingPropValid = true;
    dirty();
  }

  ZoneHeatingConfigSpec_t Zone::getHeatingControlMode() {
    ZoneHeatingConfigSpec_t spec;
    spec.ControllerMode = m_HeatingProperties.m_HeatingControlMode;
    spec.Kp = m_HeatingProperties.m_Kp;
    spec.Ts = m_HeatingProperties.m_Ts;
    spec.Ti = m_HeatingProperties.m_Ti;
    spec.Kd = m_HeatingProperties.m_Kd;
    spec.Imin = m_HeatingProperties.m_Imin;
    spec.Imax = m_HeatingProperties.m_Imax;
    spec.Ymin = m_HeatingProperties.m_Ymin;
    spec.Ymax = m_HeatingProperties.m_Ymax;
    spec.AntiWindUp = m_HeatingProperties.m_AntiWindUp;
    spec.KeepFloorWarm = m_HeatingProperties.m_KeepFloorWarm;
    spec.SourceZoneId = m_HeatingProperties.m_HeatingMasterZone;
    spec.Offset = m_HeatingProperties.m_CtrlOffset;
    spec.EmergencyValue = m_HeatingProperties.m_EmergencyValue;
    spec.ManualValue = m_HeatingProperties.m_ManualValue;
    return spec;
  }

  void Zone::clearHeatingControlMode() {
    m_HeatingProperties.reset();
  }

  void Zone::setHeatingControlState(int _ctrlState) {
    m_HeatingProperties.m_HeatingControlState = _ctrlState;
    dirty();
  }

  void Zone::setHeatingOperationMode(int _operationMode) {
    if (_operationMode >= 16) {
      Logger::getInstance()->log("Zone::setHeatingOperationMode: zone-id:" + intToString(m_ZoneID) +
          " mode: " + intToString(_operationMode) + " is invalid", lsWarning);
      return;
    }
    m_HeatingOperationMode.setValue(_operationMode);
  }

  int Zone::getHeatingOperationMode() const {
    return m_HeatingOperationMode.getValue();
  }

  void Zone::setTemperature(double _value, DateTime& _ts) {
    m_SensorStatus.m_TemperatureValue = _value;
    m_SensorStatus.m_TemperatureValueTS = _ts;
  }

  void Zone::setNominalValue(double _value, DateTime& _ts) {
    m_HeatingStatus.m_NominalValue = _value;
    m_HeatingStatus.m_NominalValueTS = _ts;
  }

  void Zone::setControlValue(double _value, DateTime& _ts) {
    m_HeatingStatus.m_ControlValue = _value;
    m_HeatingStatus.m_ControlValueTS = _ts;
  }

  void Zone::setHumidityValue(double _value, DateTime& _ts) {
    m_SensorStatus.m_HumidityValue = _value;
    m_SensorStatus.m_HumidityValueTS = _ts;
  }

  void Zone::setBrightnessValue(double _value, DateTime& _ts) {
    m_SensorStatus.m_BrightnessValue = _value;
    m_SensorStatus.m_BrightnessValueTS = _ts;
  }

  void Zone::setCO2ConcentrationValue(double _value, DateTime& _ts) {
    m_SensorStatus.m_CO2ConcentrationValue = _value;
    m_SensorStatus.m_CO2ConcentrationValueTS = _ts;
  }

  bool Zone::isAllowedSensorType(SensorType _sensorType) {
    if (m_ZoneID == 0) {
      switch (_sensorType) {
        case SensorType::TemperatureOutdoors:
        case SensorType::BrightnessOutdoors:
        case SensorType::HumidityOutdoors:
        case SensorType::WindSpeed:
        case SensorType::WindDirection:
        case SensorType::GustSpeed:
        case SensorType::GustDirection:
        case SensorType::Precipitation:
        case SensorType::AirPressure:
          return true;
        default:
          return false;
      }
    } else {
      switch (_sensorType) {
        case SensorType::TemperatureIndoors:
        case SensorType::BrightnessIndoors:
        case SensorType::HumidityIndoors:
        case SensorType::CO2Concentration:
          return true;
        default:
          return false;
      }
    }
  }

  void Zone::setSensor(const Device &_device, SensorType _sensorType) {
    const boost::shared_ptr<DeviceSensor_t> sensor =
                                       _device.getSensorByType(_sensorType);

    if (!isAllowedSensorType(sensor->m_sensorType)) {
      throw std::runtime_error("Assignment of sensor type " + sensorTypeName(sensor->m_sensorType) + " is not allowed!");
    }

    foreach (auto&& ms, m_MainSensors) {
      if (ms.m_sensorType == sensor->m_sensorType) {
        ms.m_DSUID = _device.getDSID();
        ms.m_sensorIndex = sensor->m_sensorIndex;
        return;
      }
    }

    m_MainSensors.push_back(MainZoneSensor_t());
    m_MainSensors.back().m_DSUID = _device.getDSID();
    m_MainSensors.back().m_sensorIndex = sensor->m_sensorIndex;
    m_MainSensors.back().m_sensorType = sensor->m_sensorType;
  }

  void Zone::setSensor(const MainZoneSensor_t &_mainZoneSensor) {
    if (!isAllowedSensorType(_mainZoneSensor.m_sensorType)) {
      throw std::runtime_error("Assignment of sensor type " + sensorTypeName(_mainZoneSensor.m_sensorType) + " is not allowed!");
    }

    foreach (auto&& ms, m_MainSensors) {
      if (ms.m_sensorType == _mainZoneSensor.m_sensorType) {
        ms.m_DSUID = _mainZoneSensor.m_DSUID;
        ms.m_sensorIndex = _mainZoneSensor.m_sensorIndex;
        return;
      }
    }
    m_MainSensors.push_back(_mainZoneSensor);
  }

  void Zone::resetSensor(SensorType _sensorType) {
    if (!isAllowedSensorType(_sensorType)) {
      return;
    }

    auto it = std::remove_if(m_MainSensors.begin(), m_MainSensors.end(),
                             [&](MainZoneSensor_t &sensorDesc) {
                                return sensorDesc.m_sensorType == _sensorType;
                             });
    m_MainSensors.erase(it, m_MainSensors.end());
  }

  bool Zone::isSensorAssigned(SensorType _sensorType) const {
    foreach (auto&& ms, m_MainSensors) {
      if (ms.m_sensorType == _sensorType) {
        return true;
      }
    }
    return false;
  }

  std::vector<SensorType> Zone::getUnassignedSensorTypes() const {
    SensorType sensorTypesIndoor[] = {
      SensorType::TemperatureIndoors,
      SensorType::BrightnessIndoors,
      SensorType::HumidityIndoors,
      SensorType::CO2Concentration
    };
    SensorType sensorTypesOutdoor[] = {
      SensorType::TemperatureOutdoors,
      SensorType::BrightnessOutdoors,
      SensorType::HumidityOutdoors,
      SensorType::WindSpeed,
      SensorType::WindDirection,
      SensorType::GustSpeed,
      SensorType::GustDirection,
      SensorType::Precipitation,
      SensorType::AirPressure
    };

    std::vector<SensorType> ret;
    if (m_ZoneID == 0) {
      ret.assign(sensorTypesOutdoor, sensorTypesOutdoor + ARRAY_SIZE(sensorTypesOutdoor));
    } else {
      ret.assign(sensorTypesIndoor, sensorTypesIndoor + ARRAY_SIZE(sensorTypesIndoor));
    }

    auto it = std::remove_if(ret.begin(), ret.end(),
                             [this](SensorType& sensorType) {
                                return isSensorAssigned(sensorType);
                             });
    ret.erase(it, ret.end());

    return ret;
  }

  std::vector<SensorType> Zone::getAssignedSensorTypes(const Device& _device) const {
    std::vector<SensorType> ret;
    foreach (auto&& s, m_MainSensors) {
      if (s.m_DSUID == _device.getDSID()) {
        ret.push_back(s.m_sensorType);
      }
    }
    return ret;
  }

  bool Zone::isDeviceZoneMember(const DeviceReference& _device) const {
    return contains(m_Devices, _device);
  }

  void Zone::removeInvalidZoneSensors() {
    // bring sensors without a device to the end of the vector
    auto it = std::remove_if(m_MainSensors.begin(), m_MainSensors.end(),
                             [this](MainZoneSensor_t& sensorDesc) {
                                return !isDeviceZoneMember(DeviceReference(sensorDesc.m_DSUID, m_pApartment));
                             });

    // now remove the tail
    m_MainSensors.erase(it, m_MainSensors.end());
  }

  boost::shared_ptr<Device> Zone::getAssignedSensorDevice(SensorType _sensorType) const {
    foreach (auto&& s, m_MainSensors) {
      if (s.m_sensorType == _sensorType) {
        return getDevices().getByDSID(s.m_DSUID).getDevice();
      }
    }
    return boost::shared_ptr<Device> ();
  }

  bool Zone::isZoneSensor(const Device &_device, SensorType _sensorType) const {
    boost::shared_ptr<Device> zoneSensor = getAssignedSensorDevice(_sensorType);
    if (!zoneSensor) {
      return false;
    }

    return (_device.getDSID() == zoneSensor->getDSID());
  }

  void Zone::dirty() {
    if((m_pApartment != NULL) && (m_pApartment->getModelMaintenance() != NULL)) {
      m_pApartment->getModelMaintenance()->addModelEvent(new ModelEvent(ModelEvent::etModelOperationModeChanged));
    }
  }

} // namespace dss
