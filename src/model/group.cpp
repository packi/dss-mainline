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
#include "group.h"

#include <ds/str.h>

#include "zone.h"
#include "scenehelper.h"
#include "set.h"
#include "apartment.h"
#include "state.h"
#include "src/propertysystem.h"

#include "src/model/modelconst.h"
#include "status-bit.h"
#include "status.h"

namespace dss {

    //============================================= Group

__DEFINE_LOG_CHANNEL__(Group, lsNotice);

  Group::Group(const int _id, boost::shared_ptr<Zone> _pZone)
  : AddressableModelItem(&_pZone->getApartment()),
    m_ZoneID(_pZone->getID()),
    m_GroupID(_id),
    m_ApplicationType(ApplicationType::None),
    m_ApplicationConfiguration(0),
    m_LastCalledScene(SceneOff),
    m_LastButOneCalledScene(SceneOff),
    m_IsValid(false),
    m_SyncPending(false),
    m_connectedDevices(0)
  {
  } // ctor

  Group::~Group() = default;

  bool Group::isValid() const {
    if (isDefaultGroup(m_GroupID) || isAppUserGroup(m_GroupID)) {
      return m_IsValid && (m_ApplicationType != ApplicationType::None);
    }
    return m_IsValid;
  } // isValid

  void Group::setApplicationType(ApplicationType applicationType) {
    m_ApplicationType = applicationType;
    if (getZoneID() == 0) {
      return;
    }

    if (m_GroupID == GroupIDYellow) {
      // zone.123.light
      boost::shared_ptr<Group> me = sharedFromThis();
      boost::shared_ptr<State> state = boost::make_shared<State>(me, State::makeGroupName(*this));

      try {
        m_pApartment->allocateState(state);
      } catch (ItemDuplicateException& ex) {} // we only care that it exists
    } else if (m_GroupID == GroupIDHeating) {
      // zone.123.heating
      boost::shared_ptr<Group> me = sharedFromThis();
      boost::shared_ptr<State> state = boost::make_shared<State>(me, State::makeGroupName(*this));

      try {
        m_pApartment->allocateState(state);
      } catch (ItemDuplicateException& ex) {} // we only care that it exists
    }
  } // getID

  void Group::setApplicationConfiguration(const int applicationConfiguration) {
    m_ApplicationConfiguration = applicationConfiguration;
  }

  Set Group::getDevices() const {
    return m_pApartment->getZone(m_ZoneID)->getDevices().getByGroup(m_GroupID);
  } // getDevices

  Group& Group::operator=(const Group& _other) {
    m_GroupID = _other.m_GroupID;
    m_ZoneID = _other.m_ZoneID;
    return *this;
  } // operator=

  void Group::callScene(const callOrigin_t _origin, const SceneAccessCategory _category, const int _sceneNr, const std::string _token, const bool _force) {
    // this might be redundant, but since a set could be
    // optimized if it contains only one device its safer like that...
    if(SceneHelper::rememberScene(_sceneNr & 0x00ff)) {
      setLastCalledScene(_sceneNr & 0x00ff);
    }

    AddressableModelItem::callScene(_origin, _category, _sceneNr, _token, _force);

    // TODO: checkAccess might have blocked the actual scene call

    switch (m_GroupID) {
      case GroupIDControlTemperature:
        {
          boost::shared_ptr<Zone> pZone = m_pApartment->getZone(m_ZoneID);
          if (_sceneNr <= 15) {
            pZone->setHeatingOperationMode(_sceneNr);
          }
        }
        break;
      default:
        setOnState(_origin, _sceneNr);
        break;
    }
  } // callScene

  unsigned long Group::getPowerConsumption() {
    return getDevices().getPowerConsumption();
  } // getPowerConsumption

  void Group::nextScene(const callOrigin_t _origin, const SceneAccessCategory _category) {
    callScene(_origin, _category, SceneHelper::getNextScene(m_LastCalledScene),  "", false);
  } // nextScene

  void Group::previousScene(const callOrigin_t _origin, const SceneAccessCategory _category) {
    callScene(_origin, _category, SceneHelper::getPreviousScene(m_LastCalledScene), "", false);
  } // previousScene

  void Group::setSceneName(int _sceneNumber, const std::string& _name) {
    boost::mutex::scoped_lock lock(m_SceneNameMutex);
    m_SceneNames[_sceneNumber] = _name;
    if (m_pPropertyNode != NULL) {
      PropertyNodePtr scene = m_pPropertyNode->createProperty("scenes/scene" +
        intToString(_sceneNumber));

      scene->createProperty("scene")->setIntegerValue(_sceneNumber);
      scene->createProperty("name")->setStringValue(_name);
    }
  } // setSceneName

  std::string Group::getSceneName(int _sceneNumber) {
    boost::mutex::scoped_lock lock(m_SceneNameMutex);
    m_SceneNames_t::iterator it = m_SceneNames.find(_sceneNumber);
    if (it == m_SceneNames.end()) {
      return std::string();
    } else {
      return m_SceneNames[_sceneNumber];
    }
  } // getSceneName

  void Group::setOnState(const callOrigin_t _origin, bool _on) {
    if (getZoneID() == 0) {
      return;
    }
    if (m_GroupID == GroupIDYellow) {
      try {
        boost::shared_ptr<State> state = m_pApartment->getNonScriptState("zone." +
                                               intToString(getZoneID()) +
                                                                ".light");
        state->setState(_origin, _on == true ? State_Active : State_Inactive);
      } catch (ItemNotFoundException& ex) {} // should never happen
    } else if (m_GroupID == GroupIDHeating) {
      try {
        boost::shared_ptr<State> state = m_pApartment->getNonScriptState("zone." +
                                               intToString(getZoneID()) +
                                                                ".heating");
        state->setState(_origin, _on == true ? State_Active : State_Inactive);
      } catch (ItemNotFoundException& ex) {} // should never happen
    }
  }

  void Group::setOnState(const callOrigin_t _origin, const int _sceneId) {
    if (getZoneID() == 0) {
      return;
    }
    if (m_GroupID == GroupIDYellow) {
      SceneHelper::SceneOnState isOn = SceneHelper::isOnScene(m_GroupID, _sceneId);
      if (isOn != SceneHelper::DontCare) {
        try {
          boost::shared_ptr<State> state = m_pApartment->getNonScriptState("zone." +
                                                 intToString(getZoneID()) +
                                                                ".light");
          state->setState(_origin, isOn == SceneHelper::True ? State_Active : State_Inactive);
        } catch (ItemNotFoundException& ex) {} // should never happen
      }
    } else if (m_GroupID == GroupIDHeating) {
      SceneHelper::SceneOnState isOn = SceneHelper::isOnScene(m_GroupID, _sceneId);
      if (isOn != SceneHelper::DontCare) {
        try {
          boost::shared_ptr<State> state = m_pApartment->getNonScriptState("zone." +
                                                 intToString(getZoneID()) +
                                                                ".heating");
          state->setState(_origin, isOn == SceneHelper::True ? State_Active : State_Inactive);
        } catch (ItemNotFoundException& ex) {} // should never happen
      }
    }
  }

  eState Group::getState() {
    if (getZoneID() == 0) {
      return State_Unknown;
    }
   
    // only publish states for light
    if (m_GroupID == GroupIDYellow) {
      try {
        boost::shared_ptr<State> state = m_pApartment->getNonScriptState("zone." +
                                               intToString(getZoneID()) +
                                                                ".light");
        return static_cast<eState>(state->getState());
      } catch (ItemNotFoundException& ex) {} // should never happen
    } else if (m_GroupID == GroupIDHeating) {
      try {
        boost::shared_ptr<State> state = m_pApartment->getNonScriptState("zone." +
                                               intToString(getZoneID()) +
                                                                ".heating");
        return static_cast<eState>(state->getState());
      } catch (ItemNotFoundException& ex) {} // should never happen
    }

    return State_Unknown;
  }


  void Group::publishToPropertyTree() {
    if(m_pPropertyNode == NULL) {
      if(m_pApartment->getPropertyNode() != NULL) {
        m_pPropertyNode = m_pApartment->getPropertyNode()->createProperty("zones/zone" + intToString(m_ZoneID) + "/groups/group" + intToString(m_GroupID));
        m_pPropertyNode->createProperty("group")->setIntegerValue(m_GroupID);
        m_pPropertyNode->createProperty("color")
          ->linkToProxy(PropertyProxyMemberFunction<Group, int>(*this, &Group::getApplicationTypeInt, &Group::setApplicationTypeInt));
        m_pPropertyNode->createProperty("name")
          ->linkToProxy(PropertyProxyMemberFunction<Group, std::string>(*this, &Group::getName, &Group::setName));
        m_pPropertyNode->createProperty("lastCalledScene")
          ->linkToProxy(PropertyProxyMemberFunction<Group, int>(*this, &Group::getLastCalledScene));
        m_pPropertyNode->createProperty("connectedDevices")
          ->linkToProxy(PropertyProxyReference<int>(m_connectedDevices, false));
        m_pPropertyNode->createProperty("configuration")
          ->linkToProxy(PropertyProxyMemberFunction<Group, int>(*this, &Group::getApplicationConfiguration, &Group::setApplicationConfiguration));          
      }
    }
  } // publishToPropertyTree

  void Group::sensorPush(const dsuid_t& _sourceID, SensorType _type, double _value) {
    DateTime now;
    boost::shared_ptr<Zone> pZone = m_pApartment->getZone(m_ZoneID);

    if (m_ZoneID == 0) {
      switch (_type) {
        case SensorType::TemperatureOutdoors: m_pApartment->setTemperature(_value, now); break;
        case SensorType::HumidityOutdoors: m_pApartment->setHumidityValue(_value, now); break;
        case SensorType::BrightnessOutdoors: m_pApartment->setBrightnessValue(_value, now); break;
        case SensorType::WindSpeed: m_pApartment->setWindSpeed(_value, now); break;
        case SensorType::WindDirection: m_pApartment->setWindDirection(_value, now); break;
        case SensorType::GustSpeed: m_pApartment->setGustSpeed(_value, now); break;
        case SensorType::GustDirection: m_pApartment->setGustDirection(_value, now); break;
        case SensorType::Precipitation: m_pApartment->setPrecipitation(_value, now); break;
        case SensorType::AirPressure: m_pApartment->setAirPressure(_value, now); break;
        default: break;
      }
    } else {
      switch (_type) {
        case SensorType::TemperatureIndoors: pZone->setTemperature(_value, now); break;
        case SensorType::RoomTemperatureSetpoint: pZone->setNominalValue(_value, now); break;
        case SensorType::RoomTemperatureControlVariable: pZone->setControlValue(_value, now); break;
        case SensorType::HumidityIndoors: pZone->setHumidityValue(_value, now); break;
        case SensorType::BrightnessIndoors: pZone->setBrightnessValue(_value, now); break;
        case SensorType::CO2Concentration: pZone->setCO2ConcentrationValue(_value, now); break;
        default: break;
      }
    }

    if (m_pPropertyNode != NULL) {
      PropertyNodePtr node = m_pPropertyNode->createProperty("sensor/type" + intToString(static_cast<int>(_type)));
      node->createProperty("type")->setIntegerValue(static_cast<int>(_type));
      node->createProperty("value")->setFloatingValue(_value);
      node->createProperty("sourcedsuid")->setStringValue(dsuid2str(_sourceID));
      node->createProperty("time")->setIntegerValue(now.secondsSinceEpoch());
      node->createProperty("timestamp")->setStringValue(now.toString());
    }
  } // sensorPush

  void Group::sensorInvalid(SensorType _type) {
    if (m_pPropertyNode != NULL) {
      PropertyNodePtr node = m_pPropertyNode->getProperty("sensor/type" + intToString(static_cast<int>(_type)));
      if (node) {
        node->getParentNode()->removeChild(node);
      }
    }
  }

  StatusBit& Group::getStatusBit(StatusBitType type) {
    if (!m_status) {
      m_status.reset(new Status(*this));
    }
    if (StatusBit* bit = m_status->tryGetBit(type)) {
      return *bit;
    }
    auto&& name = ds::str("zone.", getZoneID(), ".group.", getID(), ".status.", static_cast<int>(type));
    log(ds::str("New status bit name:", name), lsNotice);
    auto&& bit = std::unique_ptr<StatusBit>(new StatusBit(*m_status, type, std::move(name)));
    auto&& bitRef = *bit;
    m_status->insertBit(type, std::move(bit));
    return bitRef;
  }

  boost::mutex Group::m_SceneNameMutex;

  void Group::addConnectedDevice() {
    ++m_connectedDevices;
  }

  void Group::removeConnectedDevice() {
    if (m_connectedDevices > 0) {
      --m_connectedDevices;
    }
  }

  boost::shared_ptr<Group> Group::make(const GroupSpec_t& _groupSpec, boost::shared_ptr<Zone> _pZone)
  {
    boost::shared_ptr<Group> pGroup(new Group(_groupSpec.GroupID, _pZone));

    pGroup->setName(_groupSpec.Name);
    pGroup->setApplicationType(_groupSpec.applicationType);
    pGroup->setApplicationConfiguration(_groupSpec.applicationConfiguration);

    return pGroup;
  }

  std::ostream& operator<<(std::ostream& stream, const Group &x) {
    return stream << "zone." << x.getZoneID() << ".group." << x.getID();
  }

} // namespace dss
