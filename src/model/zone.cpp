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
#include <ds/log.h>

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

#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
using rapidjson::Document;

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

  ModelMaintenance* Zone::tryGetModelMaintenance() {
    return m_pApartment ? m_pApartment->getModelMaintenance() : DS_NULLPTR;
  }

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
    if (_group->getID() > GroupIDGlobalAppDsVentilation) {
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

  boost::shared_ptr<const DSMeter> Zone::tryGetPresentTemperatureControlMeter() const {
    if (m_temperatureControlMeter && m_temperatureControlMeter->isPresent()) {
      DS_ASSERT(m_temperatureControlMeter->getCapability_HasTemperatureControl());
      return m_temperatureControlMeter;
    }
    return boost::shared_ptr<const DSMeter>();
  }

  void Zone::setTemperatureControlMeter(const boost::shared_ptr<const DSMeter>& meter) {
    DS_REQUIRE(meter->getCapability_HasTemperatureControl());
    DS_REQUIRE(meter->isPresent());
    m_temperatureControlMeter = meter;
  }

  void Zone::updateTemperatureControlMeter() {
    if (tryGetPresentTemperatureControlMeter()) {
      return;
    }

    foreach (const auto& meter, m_DSMeters) {
      if (meter->getCapability_HasTemperatureControl() && meter->isPresent()) {
        setTemperatureControlMeter(meter);
        break;
      }
    }
  }

  void Zone::removeFromDSMeter(boost::shared_ptr<DSMeter> _dsMeter) {
    m_DSMeters.erase(find(m_DSMeters.begin(), m_DSMeters.end(), _dsMeter));
    if(_dsMeter->getPropertyNode() != NULL) {
      PropertyNodePtr alias = _dsMeter->getPropertyNode()->getProperty("zones/" + intToString(m_ZoneID));
      if(alias != NULL) {
        alias->getParentNode()->removeChild(alias);
      }
    }
    if (m_temperatureControlMeter == _dsMeter) {
      m_temperatureControlMeter.reset();
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

  const ZoneHeatingProperties_t& Zone::getHeatingProperties() const {
    return m_HeatingProperties;
  }

  const ZoneHeatingStatus_t& Zone::getHeatingStatus() const {
    return m_HeatingStatus;
  }

  const ZoneSensorStatus_t& Zone::getSensorStatus() const {
    return m_SensorStatus;
  }

  void Zone::setHeatingProperties(ZoneHeatingProperties_t& config) {
    m_HeatingProperties = config;
    m_HeatingPropValid = true;
  }

  bool Zone::isHeatingPropertiesValid() const {
    return m_HeatingPropValid;
  }

  void Zone::setHeatingConfig(const ZoneHeatingConfigSpec_t _spec) {
    m_HeatingProperties.m_mode = _spec.mode;
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
    m_HeatingPropValid = true;
    dirty();
  }

  ZoneHeatingConfigSpec_t Zone::getHeatingConfig() {
    ZoneHeatingConfigSpec_t spec;
    spec.mode = m_HeatingProperties.m_mode;
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

  void Zone::setHeatingControlState(int _ctrlState) {
    m_HeatingProperties.m_HeatingControlState = _ctrlState;
    dirty();
  }

  void Zone::setHeatingControlOperationMode(const ZoneHeatingOperationModeSpec_t& operationModeValues) {
    for (int i = 0; i <= HeatingOperationModeIDMax; ++i) {
      m_HeatingProperties.m_TeperatureSetpoints[i] =
          sensorValueToDouble(SensorType::RoomTemperatureSetpoint, operationModeValues.opModes[i]);
    }
    dirty();
  }

  void Zone::setHeatingFixedOperationMode(const ZoneHeatingOperationModeSpec_t& operationModeValues) {
    for (int i = 0; i <= HeatingOperationModeIDMax; ++i) {
      m_HeatingProperties.m_FixedControlValues[i] =
          sensorValueToDouble(SensorType::RoomTemperatureControlVariable, operationModeValues.opModes[i]);
    }
    dirty();
  }

  void Zone::setHeatingOperationMode(const ZoneHeatingOperationModeSpec_t& operationModeValues) {
    switch (m_HeatingProperties.m_mode) {
      case HeatingControlMode::PID:
        setHeatingControlOperationMode(operationModeValues);
        break;
      case HeatingControlMode::FIXED:
        setHeatingFixedOperationMode(operationModeValues);
        break;
      default:
        break;
    }
  }

  ZoneHeatingOperationModeSpec_t Zone::getHeatingControlOperationModeValues() const {
    ZoneHeatingOperationModeSpec_t retVal;

    for (int i = 0; i <= HeatingOperationModeIDMax; ++i) {
      retVal.opModes[i] =
          doubleToSensorValue(SensorType::RoomTemperatureSetpoint, m_HeatingProperties.m_TeperatureSetpoints[i]);
    }

    return retVal;
  }

  ZoneHeatingOperationModeSpec_t Zone::getHeatingFixedOperationModeValues() const {
    ZoneHeatingOperationModeSpec_t retVal;

    for (int i = 0; i <= HeatingOperationModeIDMax; ++i) {
      retVal.opModes[i] =
          doubleToSensorValue(SensorType::RoomTemperatureControlVariable, m_HeatingProperties.m_FixedControlValues[i]);
    }

    return retVal;
  }

  ZoneHeatingOperationModeSpec_t Zone::getHeatingOperationModeValues() const {
    switch (m_HeatingProperties.m_mode) {
      case HeatingControlMode::PID:
        return getHeatingControlOperationModeValues();
      case HeatingControlMode::FIXED:
        return getHeatingFixedOperationModeValues();
      default:
        return ZoneHeatingOperationModeSpec_t();
    }
  }

  void Zone::setHeatingOperationMode(int _operationMode) {
    if (_operationMode > HeatingOperationModeIDMax) {
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
    if (auto&& modelMaintenance = tryGetModelMaintenance()) {
      modelMaintenance->addModelEvent(new ModelEvent(ModelEvent::etModelOperationModeChanged));
    }
  }

  ZoneHeatingProperties::ZoneHeatingProperties() {
    reset();
  }

  void ZoneHeatingProperties::reset() {
    m_mode = HeatingControlMode::OFF;
    m_Kp = 0;
    m_Ts = 0;
    m_Ti = 0;
    m_Kd = 0;
    m_Imin = 0;
    m_Imax = 0;
    m_Ymin =0;
    m_Ymax = 0;
    m_AntiWindUp =0;
    m_KeepFloorWarm =0;
    m_HeatingControlState = 0;
    m_HeatingMasterZone = 0;
    m_CtrlOffset = 0;
    m_EmergencyValue = 175;
    m_ManualValue = 0;
    memset(m_TeperatureSetpoints, 0, sizeof(m_TeperatureSetpoints));
    memset(m_FixedControlValues, 0, sizeof(m_FixedControlValues));
  }

  bool ZoneHeatingProperties::isEqual(const ZoneHeatingConfigSpec_t& config, const ZoneHeatingOperationModeSpec_t& operationMode) const {
    bool configEqual = ((config.mode == m_mode) &&
            (config.Kp == m_Kp) &&
            (config.Ts == m_Ts) &&
            (config.Ti == m_Ti) &&
            (config.Kd == m_Kd) &&
            (config.Imin == m_Imin) &&
            (config.Imax == m_Imax) &&
            (config.Ymin == m_Ymin) &&
            (config.Ymax == m_Ymax) &&
            (config.AntiWindUp == m_AntiWindUp) &&
            (config.KeepFloorWarm == m_KeepFloorWarm) &&
            (config.SourceZoneId == m_HeatingMasterZone) &&
            (config.Offset == m_CtrlOffset) &&
            (config.EmergencyValue == m_EmergencyValue) &&
            (config.ManualValue == m_ManualValue));

    // the operation mode is important only in control and fixed mode
    bool operationModeEqual = true;
    if (m_mode == HeatingControlMode::PID) {
      for (int i = 0; i < 16; ++i) {
        if ( doubleToSensorValue(SensorType::RoomTemperatureSetpoint, m_TeperatureSetpoints[i]) != operationMode.opModes[i]) {
          operationModeEqual = false;
          break;
        }
      }
    } else if (m_mode == HeatingControlMode::FIXED) {
      for (int i = 0; i < 16; ++i) {
        if ( doubleToSensorValue(SensorType::RoomTemperatureControlVariable, m_FixedControlValues[i]) != operationMode.opModes[i]) {
          operationModeEqual = false;
          break;
        }
      }
    }

    return configEqual && operationModeEqual;
  }

  void ZoneHeatingProperties::parseTargetTemperatures(
      const std::string& jsonObject, ZoneHeatingOperationModeSpec_t& hOpValues) {
    Document d;
    d.Parse(jsonObject.c_str());

    DS_REQUIRE(d.IsObject(), "Error during Json parsing");

    // try to get all valid passed values
    for (int i = 0; i <= HeatingOperationModeIDMax; ++i) {
      std::string strIdx = ds::str(i);
      if (d.HasMember(strIdx.c_str())) {
        DS_REQUIRE(d[strIdx.c_str()].IsNumber());
        hOpValues.opModes[i] = doubleToSensorValue(SensorType::RoomTemperatureSetpoint, d[strIdx.c_str()].GetDouble());
      }
    }
  }

  void ZoneHeatingProperties::parseFixedValues(
      const std::string& jsonObject, ZoneHeatingOperationModeSpec_t& hOpValues) {
    Document d;
    d.Parse(jsonObject.c_str());

    DS_REQUIRE(d.IsObject(), "Error during Json parsing");

    // try to get all valid passed values
    for (int i = 0; i <= HeatingOperationModeIDMax; ++i) {
      std::string strIdx = ds::str(i);
      if (d.HasMember(strIdx.c_str())) {
        DS_REQUIRE(d[strIdx.c_str()].IsNumber());
        hOpValues.opModes[i] = doubleToSensorValue(SensorType::RoomTemperatureControlVariable, d[strIdx.c_str()].GetDouble());
      }
    }
  }

  void ZoneHeatingProperties::parseControlMode(
      const std::string& jsonObject, ZoneHeatingConfigSpec_t& hConfig) {
    Document d;
    d.Parse(jsonObject.c_str());

    DS_REQUIRE(d.IsObject(), "Error during Json parsing");

    if (d.HasMember("emergencyValue")) {
      DS_REQUIRE(d["emergencyValue"].IsNumber());
      hConfig.EmergencyValue = d["emergencyValue"].GetInt() + 100;
    }
    if (d.HasMember("ctrlKp")) {
      DS_REQUIRE(d["ctrlKp"].IsNumber());
      hConfig.Kp = d["ctrlKp"].GetDouble() * 40;
    }
    if (d.HasMember("ctrlTs")) {
      DS_REQUIRE(d["ctrlTs"].IsNumber());
      hConfig.Ts = d["ctrlTs"].GetInt();
    }
    if (d.HasMember("ctrlTi")) {
      DS_REQUIRE(d["ctrlTi"].IsNumber());
      hConfig.Ti = d["ctrlTi"].GetInt();
    }
    if (d.HasMember("ctrlKd")) {
      DS_REQUIRE(d["ctrlKd"].IsNumber());
      hConfig.Kd = d["ctrlKd"].GetInt();
    }
    if (d.HasMember("ctrlImin")) {
      DS_REQUIRE(d["ctrlImin"].IsNumber());
      hConfig.Imin = d["ctrlImin"].GetDouble() * 40;
    }
    if (d.HasMember("ctrlImax")) {
      DS_REQUIRE(d["ctrlImax"].IsNumber());
      hConfig.Imax = d["ctrlImax"].GetDouble() * 40;
    }
    if (d.HasMember("ctrlYmin")) {
      DS_REQUIRE(d["ctrlYmin"].IsNumber());
      hConfig.Ymin = d["ctrlYmin"].GetInt() + 100;
    }
    if (d.HasMember("ctrlYmax")) {
      DS_REQUIRE(d["ctrlYmax"].IsNumber());
      hConfig.Ymax = d["ctrlYmax"].GetInt() + 100;
    }
    if (d.HasMember("ctrlAntiWindUp")) {
      DS_REQUIRE(d["ctrlAntiWindUp"].IsBool());
      hConfig.AntiWindUp = d["ctrlAntiWindUp"].GetBool() ? 1 : 0;
    }
  }

  void ZoneHeatingProperties::parseFollowerMode(
      const std::string& jsonObject, ZoneHeatingConfigSpec_t& hConfig) {
    Document d;
    d.Parse(jsonObject.c_str());

    DS_REQUIRE(d.IsObject(), "Error during Json parsing");

    if (d.HasMember("referenceZone")) {
      DS_REQUIRE(d["referenceZone"].IsNumber());
      hConfig.SourceZoneId = d["referenceZone"].GetInt();
    }
    if (d.HasMember("ctrlOffset")) {
      DS_REQUIRE(d["ctrlOffset"].IsNumber());
      hConfig.Offset = d["ctrlOffset"].GetInt();
    }
  }

  void ZoneHeatingProperties::parseManualMode(
      const std::string& jsonObject, ZoneHeatingConfigSpec_t& hConfig) {
    Document d;
    d.Parse(jsonObject.c_str());

    DS_REQUIRE(d.IsObject(), "Error during Json parsing");

    if (d.HasMember("controlValue")) {
      DS_REQUIRE(d["controlValue"].IsNumber());
      hConfig.ManualValue = d["controlValue"].GetInt() + 100;
    }
  }

} // namespace dss
