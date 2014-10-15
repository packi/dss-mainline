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

#include "group.h"
#include "zone.h"
#include "scenehelper.h"
#include "set.h"
#include "apartment.h"
#include "state.h"
#include "src/propertysystem.h"

#include "src/model/modelconst.h"
namespace dss {

    //============================================= Group

  Group::Group(const int _id, boost::shared_ptr<Zone> _pZone, Apartment& _apartment)
  : AddressableModelItem(&_apartment),
    m_ZoneID(_pZone->getID()),
    m_GroupID(_id),
    m_StandardGroupID(0),
    m_LastCalledScene(SceneOff),
    m_LastButOneCalledScene(SceneOff),
    m_IsValid(false),
    m_SyncPending(false),
    m_connectedDevices(0)
  {
  } // ctor

  bool Group::isValid() const {
    if (m_GroupID <= 23) {
      return m_IsValid && (m_StandardGroupID > 0);
    }
    return m_IsValid;
  } // isValid

  void Group::setStandardGroupID(const int _standardGroupNumber) {
    m_StandardGroupID = _standardGroupNumber;
    if (getZoneID() == 0) {
      return;
    }

    if (m_GroupID == GroupIDYellow) {
      // zone.123.light
      boost::shared_ptr<Group> me =
          boost::static_pointer_cast<Group>(shared_from_this());
      boost::shared_ptr<State> state(new State(me));

      try {
        m_pApartment->allocateState(state);
      } catch (ItemDuplicateException& ex) {} // we only care that it exists
    } else if (m_GroupID == GroupIDHeating) {
      // zone.123.heating
      boost::shared_ptr<Group> me =
          boost::static_pointer_cast<Group>(shared_from_this());
      boost::shared_ptr<State> state(new State(me));

      try {
        m_pApartment->allocateState(state);
      } catch (ItemDuplicateException& ex) {} // we only care that it exists
    }
  } // getID

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
      m_LastCalledScene = _sceneNr & 0x00ff;
    }

    AddressableModelItem::callScene(_origin, _category, _sceneNr, _token, _force);

    // TODO: checkAccess might have blocked the actual scene call

    switch (m_GroupID) {
      case GroupIDControlTemperature:
        {
          boost::shared_ptr<Zone> pZone = m_pApartment->getZone(m_ZoneID);
          pZone->setHeatingOperationMode(_sceneNr);
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
        state->setState(_origin, _on == true ? 1 : 2);
      } catch (ItemNotFoundException& ex) {} // should never happen
    } else if (m_GroupID == GroupIDHeating) {
      try {
        boost::shared_ptr<State> state = m_pApartment->getNonScriptState("zone." +
                                               intToString(getZoneID()) +
                                                                ".heating");
        state->setState(_origin, _on == true ? 1 : 2);
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
          state->setState(_origin, isOn == SceneHelper::True ? 1 : 2);
        } catch (ItemNotFoundException& ex) {} // should never happen
      }
    } else if (m_GroupID == GroupIDHeating) {
      SceneHelper::SceneOnState isOn = SceneHelper::isOnScene(m_GroupID, _sceneId);
      if (isOn != SceneHelper::DontCare) {
        try {
          boost::shared_ptr<State> state = m_pApartment->getNonScriptState("zone." +
                                                 intToString(getZoneID()) +
                                                                ".heating");
          state->setState(_origin, isOn == SceneHelper::True ? 1 : 2);
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
        return state->getState();
      } catch (ItemNotFoundException& ex) {} // should never happen
    } else if (m_GroupID == GroupIDHeating) {
      try {
        boost::shared_ptr<State> state = m_pApartment->getNonScriptState("zone." +
                                               intToString(getZoneID()) +
                                                                ".heating");
        return state->getState();
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
          ->linkToProxy(PropertyProxyMemberFunction<Group, int>(*this, &Group::getStandardGroupID, &Group::setStandardGroupID));
        m_pPropertyNode->createProperty("valid")
          ->linkToProxy(PropertyProxyMemberFunction<Group, bool>(*this, &Group::isValid, &Group::setIsValid));
        m_pPropertyNode->createProperty("sync")
          ->linkToProxy(PropertyProxyMemberFunction<Group, bool>(*this, &Group::isSynchronized, &Group::setIsSynchronized));
        m_pPropertyNode->createProperty("name")
          ->linkToProxy(PropertyProxyMemberFunction<Group, std::string>(*this, &Group::getName, &Group::setName));
        m_pPropertyNode->createProperty("lastCalledScene")
          ->linkToProxy(PropertyProxyMemberFunction<Group, int>(*this, &Group::getLastCalledScene));
        m_pPropertyNode->createProperty("connectedDevices")
          ->linkToProxy(PropertyProxyReference<int>(m_connectedDevices, false));
        m_pPropertyNode->createProperty("devices");
        m_pPropertyNode->createProperty("scenes");
        m_pPropertyNode->createProperty("sensor");
      }
    }
  } // publishToPropertyTree

  void Group::sensorPush(const std::string& _sourceID, const int _sensorType, const double _sensorValue) {
    DateTime now;
    boost::shared_ptr<Zone> pZone = m_pApartment->getZone(m_ZoneID);

    if (m_ZoneID == 0) {
      switch (_sensorType) {
        case SensorIDTemperatureOutdoors: m_pApartment->setTemperature(_sensorValue, now); break;
        case SensorIDHumidityOutdoors: m_pApartment->setHumidityValue(_sensorValue, now); break;
        case SensorIDBrightnessOutdoors: m_pApartment->setBrightnessValue(_sensorValue, now); break;
        default: break;
      }
    } else {
      switch (_sensorType) {
        case SensorIDTemperatureIndoors: pZone->setTemperature(_sensorValue, now); break;
        case SensorIDRoomTemperatureSetpoint: pZone->setNominalValue(_sensorValue, now); break;
        case SensorIDRoomTemperatureControlVariable: pZone->setControlValue(_sensorValue, now); break;
        case SensorIDHumidityIndoors: pZone->setHumidityValue(_sensorValue, now); break;
        case SensorIDBrightnessIndoors: pZone->setBrightnessValue(_sensorValue, now); break;
        case SensorIDCO2Concentration: pZone->setCO2ConcentrationValue(_sensorValue, now); break;
        default: break;
      }
    }

    if (m_pPropertyNode != NULL) {
      PropertyNodePtr node = m_pPropertyNode->createProperty("sensor/type" + intToString(_sensorType));
      node->createProperty("value")->setFloatingValue(_sensorValue);
      node->createProperty("sourcedsuid")->setStringValue(_sourceID);
      node->createProperty("time")->setIntegerValue(now.secondsSinceEpoch());
      node->createProperty("timestamp")->setStringValue(now.toString());
    }
  } // sensorPush

  boost::mutex Group::m_SceneNameMutex;

  void Group::addConnectedDevice() {
    ++m_connectedDevices;
  }

  void Group::removeConnectedDevice() {
    if (m_connectedDevices > 0) {
      --m_connectedDevices;
    }
  }

} // namespace dss
