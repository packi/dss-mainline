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

#include "zone.h"

#include <vector>

#include "src/dss.h"
#include "src/businterface.h"
#include "src/base.h"
#include "src/util.h"
#include "src/foreach.h"
#include "src/logger.h"
#include "src/model/modelconst.h"
#include "src/propertysystem.h"
#include "src/model/set.h"
#include "src/model/device.h"
#include "src/model/group.h"
#include "src/model/apartment.h"
#include "src/model/modulator.h"

namespace dss {

  //================================================== Zone

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
    DeviceIterator pos = find(m_Devices.begin(), m_Devices.end(), _device);
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

  boost::shared_ptr<Group> Zone::getGroup(const int _id) const {
    for(std::vector<boost::shared_ptr<Group> >::const_iterator ipGroup = m_Groups.begin(), e = m_Groups.end();
        ipGroup != e; ++ipGroup)
    {
        if((*ipGroup)->getID() == _id) {
          return *ipGroup;
        }
    }
    return boost::shared_ptr<Group>();
  } // getGroup

  int Zone::getID() const {
    return m_ZoneID;
  } // getID

  void Zone::setZoneID(const int _value) {
    m_ZoneID = _value;
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
    if (IsEqualDsuid(prop.m_HeatingControlDSUID, _dsMeter->getDSID())) {
      dsuid_t null;
      SetNullDsuid(null);
      setHeatingControlMode(0, 0, 0, 0, null);
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
        if (m_ZoneID > 0) {
          m_pPropertyNode->createProperty("heating/");
          m_pPropertyNode->createProperty("heating/OperationMode")
              ->linkToProxy(PropertyProxyReference<int>(m_HeatingStatus.m_OperationMode));
          m_pPropertyNode->createProperty("heating/ControlMode")
              ->linkToProxy(PropertyProxyReference<int>(m_HeatingProperties.m_HeatingControlMode));
          m_pPropertyNode->createProperty("heating/ControlState")
              ->linkToProxy(PropertyProxyReference<int>(m_HeatingProperties.m_HeatingControlState));
        }
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

  void Zone::setHeatingControlMode(int _ctrlMode, int _offset, int _masterZone, int _manualValue, dsuid_t ctrlDevice) {
    m_HeatingProperties.m_HeatingControlMode = _ctrlMode;
    m_HeatingProperties.m_HeatingControlDSUID = ctrlDevice;
    m_HeatingProperties.m_CtrlOffset = _offset;
    m_HeatingProperties.m_HeatingMasterZone = _masterZone;
    m_HeatingProperties.m_ManualValue = _manualValue;
  }

  void Zone::setHeatingControlState(int _ctrlState) {
    m_HeatingProperties.m_HeatingControlState = _ctrlState;
  }

  void Zone::setHeatingOperationMode(int _operationMode) {
    m_HeatingStatus.m_OperationMode = _operationMode;
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

  bool Zone::isAllowedSensorType(int _sensorType) {
    if (m_ZoneID == 0) {
      switch (_sensorType) {
        case SensorIDTemperatureOutdoors:
        case SensorIDBrightnessOutdoors:
        case SensorIDHumidityOutdoors:
          return true;
        default:
          return false;
      }
    } else {
      switch (_sensorType) {
        case SensorIDTemperatureIndoors:
        case SensorIDBrightnessIndoors:
        case SensorIDHumidityIndoors:
        case SensorIDCO2Concentration:
          return true;
        default:
          return false;
      }
    }
  }

  void Zone::setSensor(boost::shared_ptr<const Device> _device,
                          uint8_t _sensorType) {
    const boost::shared_ptr<DeviceSensor_t> sensor =
                                       _device->getSensorByType(_sensorType);

    if (!isAllowedSensorType(sensor->m_sensorType)) {
      throw std::runtime_error("Assignment of sensor type " + intToString(sensor->m_sensorType) + " is not allowed!");
    }

    for (size_t i = 0; i < m_MainSensors.size(); i++) {
      boost::shared_ptr<MainZoneSensor_t> ms = m_MainSensors.at(i);
      if (ms && (ms->m_sensorType == sensor->m_sensorType)) {
        ms->m_DSUID = _device->getDSID();
        ms->m_sensorIndex = sensor->m_sensorIndex;
        return;
      }
    }

    boost::shared_ptr<MainZoneSensor_t> ms(new MainZoneSensor_t());
    ms->m_DSUID = _device->getDSID();
    ms->m_sensorIndex = sensor->m_sensorIndex;
    ms->m_sensorType = sensor->m_sensorType;
    m_MainSensors.push_back(ms);
  }

  void Zone::setSensor(boost::shared_ptr<MainZoneSensor_t> _mainZoneSensor) {
    if (!isAllowedSensorType(_mainZoneSensor->m_sensorType)) {
      throw std::runtime_error("Assignment of sensor type " + intToString(_mainZoneSensor->m_sensorType) + " is not allowed!");
    }

    for (size_t i = 0; i < m_MainSensors.size(); i++) {
      boost::shared_ptr<MainZoneSensor_t> ms = m_MainSensors.at(i);
      if (ms && (ms->m_sensorType == _mainZoneSensor->m_sensorType)) {
        ms->m_DSUID = _mainZoneSensor->m_DSUID;
        ms->m_sensorIndex = _mainZoneSensor->m_sensorIndex;
        return;
      }
    }
    m_MainSensors.push_back(_mainZoneSensor);
  }

  void Zone::resetSensor(uint8_t _sensorType) {
    if (!isAllowedSensorType(_sensorType)) {
      return;
    }

    std::vector<boost::shared_ptr<MainZoneSensor_t> >::iterator it;
    for (it = m_MainSensors.begin(); it != m_MainSensors.end();) {
      if ((*it)->m_sensorType == _sensorType) {
          (*it).reset();
        it = m_MainSensors.erase(it);
        return;
      } else {
        it++;
      }
    }
  }

  bool Zone::isSensorAssigned(uint8_t _sensorType) const {
    for (std::vector<boost::shared_ptr<MainZoneSensor_t> >::const_iterator it = m_MainSensors.begin();
        it != m_MainSensors.end();
        it ++) {
      boost::shared_ptr<MainZoneSensor_t> devSensor = *it;
      if (devSensor->m_sensorType == _sensorType) {
        return true;
      }
    }
    return false;
  }

  boost::shared_ptr<std::vector<int> > Zone::getUnassignedSensorTypes() const {
    int sensorTypes[] = {
      SensorIDTemperatureIndoors,
      SensorIDBrightnessIndoors,
      SensorIDHumidityIndoors,
      SensorIDCO2Concentration
    };

    boost::shared_ptr<std::vector<int> >ret(
        new std::vector<int>(sensorTypes,
                        sensorTypes + sizeof(sensorTypes) / sizeof(int)));

    for (size_t i = 0; i < m_MainSensors.size(); i++) {
      boost::shared_ptr<MainZoneSensor_t> s = m_MainSensors.at(i);

      std::vector<int>::iterator it;
      for (it = ret->begin(); it != ret->end();) {
        if (*it == s->m_sensorType) {
          it = ret->erase(it);
        } else {
          it++;
        }
      }
    }

    return ret;
  }

  boost::shared_ptr<std::vector<int> > Zone::getAssignedSensorTypes(boost::shared_ptr<const Device> _device) const {
    boost::shared_ptr<std::vector<int> >ret(new std::vector<int>());
    dsuid_t dev_dsuid = _device->getDSID();

    for (size_t i = 0; i < m_MainSensors.size(); i++) {
      boost::shared_ptr<MainZoneSensor_t> s = m_MainSensors.at(i);
      if (!s) {
        continue;
      }
      dsuid_t zone_dsuid = s->m_DSUID;
      if (IsEqualDsuid(zone_dsuid, dev_dsuid)) {
        ret->push_back(s->m_sensorType);
      }
    }
    return ret;
  }

  bool Zone::isDeviceZoneMember(const DeviceReference& _device) const {
    return contains(m_Devices, _device);
  }

  void Zone::removeInvalidZoneSensors() {
    std::vector<boost::shared_ptr<MainZoneSensor_t> >::iterator it;
    for (it = m_MainSensors.begin(); it != m_MainSensors.end();) {
      DeviceReference devRef ((*it)->m_DSUID, m_pApartment);
      if (isDeviceZoneMember(devRef)) {
        ++it;
      } else {
        (*it).reset();
        it = m_MainSensors.erase(it);
      }
    }
  }

  boost::shared_ptr<Device> Zone::getAssignedSensorDevice(int _sensorType) const {
    for (size_t i = 0; i < m_MainSensors.size(); i++) {
      boost::shared_ptr<MainZoneSensor_t> s = m_MainSensors.at(i);
      if (!s) {
        continue;
      }
      if (s->m_sensorType == _sensorType) {
        return getDevices().getByDSID(s->m_DSUID).getDevice();
      }
    }
    return boost::shared_ptr<Device> ();
  }

} // namespace dss
