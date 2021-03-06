/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Author: Michael Tross, aizo GmbH <michael.tross@aizo.com>

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

#include <ds/log.h>

#include "state.h"
#include "base.h"
#include "dss.h"
#include "logger.h"
#include "propertysystem.h"
#include "event.h"
#include "event/event_create.h"
#include "model/device.h"
#include "model/group.h"
#include "model/zone.h"
#include "model/apartment.h"
#include "model/modelconst.h"
#include "model/modulator.h"
#include "modelmaintenance.h"

#include <boost/shared_ptr.hpp>

namespace dss {

    //============================================= State

  const std::string State::INVALID("invalid");

  State::State(const std::string& name, eStateType type, eState initState, StatePersistency persistency)
    : m_name(name),
      m_persistency(persistency),
      m_callOrigin(coUnknown),
      m_originDeviceDSUID(DSUID_NULL),
      m_state(initState),
      m_type(type)
  {
    // common base class; no load/publishToPropertyTree until fully initialized
  }

  State::State(const std::string& _name)
    : m_name(_name),
      m_persistency(StatePersistency::volatile_),
      m_callOrigin(coUnknown),
      m_originDeviceDSUID(DSUID_NULL),
      m_state(State_Inactive),
      m_type(StateType_Apartment)
  {
    load();
    publishToPropertyTree();
  } // ctor

  State::State(eStateType _type, const std::string& _name, const std::string& _identifier)
  : m_name(_name),
    m_persistency(StatePersistency::volatile_),
    m_callOrigin(coUnknown),
    m_originDeviceDSUID(DSUID_NULL),
    m_state(State_Inactive),
    m_type(_type),
    m_serviceName(_identifier)
  {
    // TODO: check if _identifier is valid, e.g. the prop tree does not like
    //       an empty identifier
    load();
    publishToPropertyTree();
  }

  std::string BinaryInputState::makeName(const dsuid_t &dsuid, int inputIndex) {
    return "dev." + dsuid2str(dsuid) + "." + intToString(inputIndex);
  }

  BinaryInputState::BinaryInputState(boost::shared_ptr<Device> device, int inputIndex) :
    State(makeName(device->getDSID(), inputIndex),
          StateType_Device,
          State_Invalid,
          StatePersistency::persistent)
  {
    m_providerDev = device,
    m_providerDevInput = inputIndex;
    load();
    publishToPropertyTree();
  }

  BinaryInputState::~BinaryInputState() = default;

  State::State(boost::shared_ptr<Device> _device, const std::string& stateName) :
      m_name("dev." + dsuid2str(_device->getDSID()) + "." + stateName),
      m_persistency(StatePersistency::persistent),
      m_callOrigin(coUnknown),
      m_originDeviceDSUID(DSUID_NULL),
      m_state(State_Invalid),
      m_type(StateType_Device),
      m_providerDev(_device),
      m_providerDevInput(-1),
      m_providerDevStateName(stateName)
  {
    load();
    publishToPropertyTree();
  }

  State::State(boost::shared_ptr<DSMeter> _meter, int _inputIndex) :
      m_name("dsm." + dsuid2str(_meter->getDSID()) + "." + intToString(_inputIndex)),
      m_persistency(StatePersistency::persistent),
      m_callOrigin(coUnknown),
      m_originDeviceDSUID(DSUID_NULL),
      m_state(State_Invalid),
      m_type(StateType_Circuit),
      m_providerDevInput(_inputIndex),
      m_providerDsm(_meter)
  {
    load();
    publishToPropertyTree();
  }

  std::string State::makeGroupName(const Group& group) {
    auto&& logicalName = [&]() -> std::string {
      auto&& applicationType = group.getApplicationType();
      switch (applicationType) {
        case ApplicationType::Lights:
          return "light"; // ATTENTION: This is inconsistent with group.getName() == "yellow"
          break;
        case ApplicationType::Heating:
          return "heating";
          break;
        default:
          return "unknown";
          break;
      }
    }();
    return "zone." + intToString(group.getZoneID()) + "." + logicalName;
  }

  State::State(boost::shared_ptr<Group> _group, const std::string& name) :
    m_name(name),
    m_persistency(StatePersistency::volatile_),
    m_callOrigin(coUnknown),
    m_originDeviceDSUID(DSUID_NULL),
    m_state(State_Unknown),
    m_type(StateType_Group),
    m_providerGroup(_group)
  {
    load();
    publishToPropertyTree();
  }

  State::~State() {
    removeFromPropertyTree();
  }

  void State::publishToPropertyTree() {
    removeFromPropertyTree();
    std::string path;
    switch (m_type) {
      case StateType_Script:
      case StateType_SensorDevice:
      case StateType_SensorZone:
        path = "/usr/addon-states/" + m_serviceName + "/";
        break;
      default:
        path = "/usr/states/";
        break;
    }
    path += m_name;
    if (m_pPropertyNode == NULL && DSS::hasInstance()) {
      if (DSS::getInstance()->getPropertySystem().getProperty(path) != NULL) {
        // avoid publishing duplicate state information when reading out devices multiple times
        return;
      }
      m_pPropertyNode = DSS::getInstance()->getPropertySystem().createProperty(path);
      m_pPropertyNode->createProperty("name")
        ->linkToProxy(PropertyProxyReference<std::string>(m_name, false));
      m_pPropertyNode->createProperty("value")
        ->linkToProxy(PropertyProxyReference<int, int>(m_state, false));
      m_pPropertyNode->createProperty("state")
        ->linkToProxy(PropertyProxyMemberFunction<State, std::string, false>(*this, &State::toString));
      if (m_callOrigin != coUnknown) {
        m_pPropertyNode->createProperty("callOrigin")
          ->linkToProxy(PropertyProxyReference<int, callOrigin_t>(m_callOrigin, false));
      }
      if (m_originDeviceDSUID != DSUID_NULL) {
        m_pPropertyNode->createProperty("originDeviceDSUID")
          ->linkToProxy(PropertyProxyMemberFunction<State, std::string, false>(*this, &State::getOriginDeviceDSUIDString));
      }

      // #5870 - keep compatibility for "presence" and "hibernation"
      if (m_name == "presence" || m_name == "hibernation") {
        m_pPropertyNode->removeChild(m_pPropertyNode->getPropertyByName("value"));
        m_pPropertyNode->createProperty("value")
          ->linkToProxy(PropertyProxyMemberFunction<State, std::string, false>(*this, &State::toString));
      }

      if (m_providerDev != NULL) {
        PropertyNodePtr devNode = m_providerDev->getPropertyNode();
        PropertyNodePtr m_pAliasNode = m_pPropertyNode->createProperty("device/" + dsuid2str(m_providerDev->getDSID()));
        m_pAliasNode->alias(devNode);
        if (m_providerDevInput != -1) {
          m_pPropertyNode->createProperty("device/inputIndex")
            ->linkToProxy(PropertyProxyReference<int>(m_providerDevInput, false));
        }
        if (!m_providerDevStateName.empty()) {
          m_pPropertyNode->createProperty("device/stateName")
            ->linkToProxy(PropertyProxyReference<std::string>(m_providerDevStateName, false));
        }
      }

      if (m_providerDsm != NULL) {
        PropertyNodePtr devNode = m_providerDsm->getPropertyNode();
        PropertyNodePtr m_pAliasNode = m_pPropertyNode->createProperty("dSMeter/" + dsuid2str(m_providerDsm->getDSID()));
        m_pAliasNode->alias(devNode);
        m_pPropertyNode->createProperty("dSMeter/inputIndex")
          ->linkToProxy(PropertyProxyReference<int>(m_providerDevInput, false));
      }

      if (m_serviceName.length() > 0) {
        m_pPropertyNode->createProperty("service/name")
            ->linkToProxy(PropertyProxyReference<std::string>(m_serviceName, false));
      }
    }
  } // publishToPropertyTree

  void State::removeFromPropertyTree() {
    if(m_pPropertyNode != NULL) {
      m_pPropertyNode->unlinkProxy(true);
      PropertyNode *parent = m_pPropertyNode->getParentNode();
      if (parent != NULL) {
        parent->removeChild(m_pPropertyNode);
      }
      m_pPropertyNode.reset();
    }
  } // removeFromPropertyTree

  std::string State::getStorageName() {
    std::string dirname;
    if (!DSS::hasInstance()) {
      dirname = "/tmp";
    } else {
      dirname = DSS::getInstance()->getSavedPropsDirectory();
    }
    std::string filename = "state." + getName();
    return dirname + "/" + filename;
  }

  void State::save() {
    FILE *fout = fopen(getStorageName().c_str(), "w");
    if (NULL == fout) {
      Logger::getInstance()->log("State " + m_name + " could not be written", lsError);
      return;
    }
    fputc((int) m_state, fout);
    fputc(static_cast<int>(m_callOrigin), fout);
    for (int i = 0; i < DSUID_SIZE; i++) {
      fputc(static_cast<int>(m_originDeviceDSUID.id[i]), fout);
    }
    fclose(fout);
  } // save

  void State::load() {
    FILE *fin = fopen(getStorageName().c_str(), "r");
    if (NULL == fin) {
      return;
    }
    int value = fgetc(fin);
    if (value != EOF) {
      m_state = (eState) value;
    }
    value = fgetc(fin);
    if (value != EOF) {
      m_callOrigin = static_cast<callOrigin_t>(value);
    }
    for (int i = 0; i < DSUID_SIZE; i++) {
      value = fgetc(fin);
      if (value != EOF) {
        m_originDeviceDSUID.id[i] = static_cast<char>(value);
      } else {
        m_originDeviceDSUID = DSUID_NULL;
        break;
      }
    }
    fclose(fin);
    return;
  } // load

  bool State::getPersistence() const {
    return m_persistency == StatePersistency::persistent;
  } // getPersistence

  void State::setPersistence(bool _persistent) {
    if (m_persistency == StatePersistency::volatile_ && _persistent) {
      load();
    }
    m_persistency = StatePersistency::persistent;
  } // setPersistence

  bool State::hasPersistentData() {
    std::string fname(getStorageName());
    if (access(fname.c_str(), F_OK) == 0) {
      return true;
    }
    return false;
  } // hasPersistentData

  std::string State::toString() const {
    if (!m_values.empty()) {
      if (m_state >= 0 && m_state < (int) m_values.size()) {
        return m_values[m_state];
      } else {
        return INVALID;
      }
    }
    switch(m_state) {
    case State_Unknown:
      return std::string("unknown");
    case State_Active:
      return std::string("active");
    case State_Inactive:
      return std::string("inactive");
    default:
      return INVALID;
    }
  } // toString

  boost::optional<int> State::tryValueFromName(const std::string& valueName) {
    if (!m_values.empty()) {
      auto it = std::find(m_values.begin(), m_values.end(), valueName);
      if (it != m_values.end()) {
        return it - m_values.begin();
      }
      return boost::none;
    }

    if (valueName == "active") {
      return State_Active;
    } else if (valueName == "inactive") {
      return State_Inactive;
    } else if (valueName == "unknown") {
      return State_Unknown;
    } else if (valueName == INVALID) {
      return State_Invalid;
    } else {
      return boost::none;
    }
  }

  int State::valueFromName(const std::string& valueName) {
    if (auto&& value = tryValueFromName(valueName)) {
      return *value;
    }
    DS_FAIL_REQUIRE("Unrecorgnized state value", valueName);
  }

  int State::getState() const {
    return m_state;
  } // getState

  void State::setState(const callOrigin_t _origin, const int _state) {
    if (_state != m_state) {
      int oldstate = m_state;
      m_state = _state;
      m_callOrigin = _origin;

      if (m_pPropertyNode != NULL && DSS::hasInstance() && (m_callOrigin != coUnknown)) {
        m_pPropertyNode->createProperty("callOrigin")
          ->linkToProxy(PropertyProxyReference<int, callOrigin_t>(m_callOrigin, false));
      }

      if (m_persistency == StatePersistency::persistent) {
        save();
      }

      if (DSS::hasInstance() && DSS::getInstance()->getState() == ssRunning) {
        DSS::getInstance()->getEventQueue()
          .pushEvent(createStateChangeEvent(shared_from_this(), oldstate, _origin));
      }
    }
  } // setState

  void State::setState(const callOrigin_t _origin, const std::string& valueName) {
    if (auto&& value = tryValueFromName(valueName)) {
      setState(_origin, *value);
    }
  }

  unsigned int State::getValueRangeSize() const {
    return (!m_values.empty()) ? m_values.size() : (State_Unknown + 1);
  }

  void State::setOriginDeviceDSUID(const dsuid_t _dsuid) {
    m_originDeviceDSUID = _dsuid;
    if (m_pPropertyNode != NULL && DSS::hasInstance() &&
        (m_originDeviceDSUID != DSUID_NULL) &&
        m_pPropertyNode->getProperty("originDeviceDSUID") == NULL) {
      m_pPropertyNode->createProperty("originDeviceDSUID")
        ->linkToProxy(PropertyProxyMemberFunction<State, std::string, false>(*this, &State::getOriginDeviceDSUIDString));
    }
  }

  void StateSensor::parseCondition(const std::string& _input, eValueComparator& _comp, double& _threshold) {
    _comp = eComp_undef;
    _threshold = 0;
    if (_input.empty()) {
      return;
    }
    std::vector<std::string> tokens = splitString(_input, ';', true);
    if (tokens[0].length() > 0) {
      if (tokens[0][0] == '>') {
        _comp = eComp_higher;
      } else if (tokens[0][0] == '<') {
        _comp = eComp_lower;
      }
    }
    try {
      _threshold = strToDouble(tokens[1]);
    } catch(std::invalid_argument& ex) {
    }
  }

  StateSensor::StateSensor(const std::string& _identifier, const std::string& _scriptId,
      boost::shared_ptr<Device> _device, SensorType _sensorType,
      const std::string& _activateCondition, const std::string& _deactivateCondition)
  : State(StateType_SensorDevice,
      "dev." + dsuid2str(_device->getDSID()) +
      ".type" + intToString(static_cast<int>(_sensorType)) +
      "." + _identifier,
      _scriptId),
    m_activateCondition(_activateCondition),
    m_deactivateCondition(_deactivateCondition)
  {
    parseCondition(m_activateCondition, m_activateComparator, m_activateValue);
    parseCondition(m_deactivateCondition, m_deactivateComparator, m_deactivateValue);
    setProviderDevice(_device);
  }

  StateSensor::StateSensor(const std::string& _identifier, const std::string& _scriptId,
      boost::shared_ptr<Group> _group, SensorType _sensorType,
      const std::string& _activateCondition, const std::string& _deactivateCondition)
  : State(StateType_SensorZone,
      "zone.zone" + intToString(_group->getZoneID()) +
      ".group"  + intToString(_group->getID()) +
      ".type" + intToString(static_cast<int>(_sensorType)) +
      "." + _identifier,
      _scriptId),
    m_activateCondition(_activateCondition),
    m_deactivateCondition(_deactivateCondition)
  {
    parseCondition(m_activateCondition, m_activateComparator, m_activateValue);
    parseCondition(m_deactivateCondition, m_deactivateComparator, m_deactivateValue);
    setProviderGroup(_group);
  }

  void StateSensor::newValue(const callOrigin_t _origin, double _value) {
    bool cond0, cond1;

    if (m_activateComparator == eComp_lower) {
      cond1 = _value < m_activateValue;
    } else if (m_activateComparator == eComp_higher) {
      cond1 = _value > m_activateValue;
    } else {
      cond1 = false;
    }
    if (m_deactivateComparator == eComp_lower) {
      cond0 = _value < m_deactivateValue;
    } else if (m_deactivateComparator == eComp_higher) {
      cond0 = _value > m_deactivateValue;
    } else {
      cond0 = false;
    }

    int oldState = getState();
    if (oldState == State_Active) {
      if (cond1 == false && cond0 == true) {
        setState(_origin, State_Inactive);
      } else if (cond1 == true && cond0 == true) {
        setState(_origin, State_Unknown);
      }
    } else {
      if (cond1 == true && cond0 == false) {
        setState(_origin, State_Active);
      } else if (cond1 == true && cond0 == true) {
        setState(_origin, State_Unknown);
      }
    }
  }

  StateSensor::~StateSensor() {
  }

} // namespace dss
