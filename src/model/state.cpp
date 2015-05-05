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


#include "base.h"
#include "dss.h"
#include "logger.h"
#include "propertysystem.h"
#include "event.h"
#include "event/event_create.h"
#include "model/state.h"
#include "model/device.h"
#include "model/group.h"
#include "model/zone.h"
#include "model/apartment.h"
#include "model/modelconst.h"
#include "modelmaintenance.h"

#include <boost/shared_ptr.hpp>

namespace dss {

    //============================================= State

  State::State(const std::string& _name)
    : m_name(_name),
      m_IsPersistent(false),
      m_state(State_Inactive),
      m_type(StateType_Apartment)
  {
    load();
    publishToPropertyTree();
  } // ctor

  State::State(const std::string& _name, eState _state)
    : m_name(_name),
      m_IsPersistent(false),
      m_state(_state),
      m_type(StateType_Apartment)
  {
    load();
    publishToPropertyTree();
  } // ctor

  State::State(eStateType _type, const std::string& _name,
               const std::string& _identifier)
  : m_name(_name),
    m_IsPersistent(false),
    m_state(State_Inactive),
    m_type(_type),
    m_serviceName(_identifier)
  {
    load();
    publishToPropertyTree();
  }

  State::State(boost::shared_ptr<Device> _device, int _inputIndex) :
      m_name("dev." + dsuid2str(_device->getDSID()) + "." + intToString(_inputIndex)),
      m_IsPersistent(true),
      m_state(State_Invalid),
      m_type(StateType_Device),
      m_providerDev(_device),
      m_providerDevInput(_inputIndex)
  {
    load();
    publishToPropertyTree();
  }

  State::State(boost::shared_ptr<Group> _group)
      :
    m_IsPersistent(false),
    m_state(State_Unknown),
    m_type(StateType_Group),
    m_providerGroup(_group)
  {
    std::string logicalName = "unknown";

    switch (_group->getStandardGroupID()) {
      case GroupIDYellow:
        logicalName = "light";
        break;
      case GroupIDHeating:
        logicalName = "heating";
        break;
      default:
        break;
    }

    m_name = "zone." + intToString(_group->getZoneID()) + "." + logicalName;
    load();
    publishToPropertyTree();
  }

  State::~State() {
    removeFromPropertyTree();
  }

  void State::publishToPropertyTree() {
    removeFromPropertyTree();
    std::string basepath;
    switch (m_type) {
      case StateType_Script:
      case StateType_SensorDevice:
      case StateType_SensorZone:
        basepath = "/usr/addon-states/" + m_serviceName + "/";
        break;
      default:
        basepath = "/usr/states/";
        break;
    }
    if (m_pPropertyNode == NULL && DSS::hasInstance()) {
      if (DSS::getInstance()->getPropertySystem().getProperty(basepath + m_name) != NULL) {
        // avoid publishing duplicate state information when reading out devices multiple times
        return;
      }
      m_pPropertyNode = DSS::getInstance()->getPropertySystem().createProperty(basepath + m_name);
      m_pPropertyNode->createProperty("name")
        ->linkToProxy(PropertyProxyReference<std::string>(m_name, false));

      m_pPropertyNode->createProperty("value")
        ->linkToProxy(PropertyProxyReference<int>((int &) m_state, false));
      m_pPropertyNode->createProperty("state")
        ->linkToProxy(PropertyProxyMemberFunction<State, std::string, false>(*this, &State::toString));

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
        m_pPropertyNode->createProperty("device/inputIndex")
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
    if (!DSS::hasInstance()) {
      return "";
    }
    std::string dirname = DSS::getInstance()->getSavedPropsDirectory();
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
    fclose(fout);
  } // save

  void State::load() {
    FILE *fin = fopen(getStorageName().c_str(), "r");
    if (NULL == fin) {
      return;
    }
    int value = fgetc(fin);
    fclose(fin);
    if (value >= 0) {
      m_state = (eState) value;
    } else {
      return;
    }
  } // load

  bool State::getPersistence() const {
    return m_IsPersistent;
  } // getPersistence

  void State::setPersistence(bool _persistent) {
    if (!m_IsPersistent && _persistent) {
      load();
    }
    m_IsPersistent = _persistent;
  } // setPersistence

  std::string State::toString() const {
    if (!m_values.empty()) {
      assert(m_state >= 0 && m_state < m_values.size());
      return m_values[m_state];
    }
    switch(m_state) {
    case State_Unknown:
      return std::string("unknown");
    case State_Active:
      return std::string("active");
    case State_Inactive:
      return std::string("inactive");
    default:
      return std::string("invalid");
    }
  } // toString

  eState State::getState() const {
    return m_state;
  } // getState

  void State::setState(const callOrigin_t _origin, const eState _state) {
    if (_state != m_state) {
      eState oldstate = m_state;
      m_state = _state;

      if (m_IsPersistent) {
        save();
      }

      if (DSS::hasInstance() && DSS::getInstance()->getState() == ssRunning) {
        DSS::getInstance()->getEventQueue()
          .pushEvent(createStateChangeEvent(shared_from_this(), oldstate, _origin));
      }
    }
  } // setState

  void State::setState(const callOrigin_t _origin, const int _state) {
    switch (_state) {
      case 1: setState(_origin, State_Active); break;
      case 2: setState(_origin, State_Inactive); break;
      case 3: setState(_origin, State_Unknown); break;
      default: setState(_origin, (eState) _state); break;
    }
  }

  void State::setState(const callOrigin_t _origin, const std::string& _state) {
    if (_state == "active") {
      setState(_origin, State_Active);
    } else if (_state == "inactive") {
      setState(_origin, State_Inactive);
    } else if (_state == "unknown") {
      setState(_origin, State_Unknown);
    } else if (!m_values.empty()) {
      ValueRange_t::iterator it = std::find(m_values.begin(), m_values.end(), _state);
      if (it == m_values.end()) {
        Logger::getInstance()->log("State " + m_name + ": invalid value" + _state,
                                   lsWarning);
        return;
      }
      setState(_origin, it - m_values.begin());
    }
  }

  void State::setValueRange(const ValueRange_t &_values) {
    m_values = _values;
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

  StateSensor::StateSensor(const std::string& _identifier, boost::shared_ptr<Device> _device, int _sensorType,
      const std::string& _activateCondition, const std::string& _deactivateCondition)
  : State(StateType_SensorDevice, "dev." + dsuid2str(_device->getDSID()) + ".type" + intToString(_sensorType), _identifier),
    m_activateCondition(_activateCondition),
    m_deactivateCondition(_deactivateCondition)
  {
    parseCondition(m_activateCondition, m_activateComparator, m_activateValue);
    parseCondition(m_deactivateCondition, m_deactivateComparator, m_deactivateValue);
    setProviderDevice(_device);
  }

  StateSensor::StateSensor(const std::string& _identifier, boost::shared_ptr<Group> _group, int _sensorType,
      const std::string& _activateCondition, const std::string& _deactivateCondition)
  : State(StateType_SensorZone,
      "zone.zone" + intToString(_group->getZoneID()) +
      ".group"  + intToString(_group->getID()) +
      ".type" + intToString(_sensorType), _identifier),
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

    eState oldState = getState();
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
