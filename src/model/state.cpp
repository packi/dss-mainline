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

#include "base.h"
#include "dss.h"
#include "logger.h"
#include "propertysystem.h"
#include "event.h"
#include "model/state.h"
#include "model/device.h"
#include "model/apartment.h"
#include "model/modelconst.h"
#include "modelmaintenance.h"

#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr.hpp>

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

  State::State(const std::string& _name, const std::string& _serviceId)
  : m_name(_name),
    m_IsPersistent(false),
    m_state(State_Inactive),
    m_type(StateType_Service),
    m_serviceName(_serviceId)
  {
    load();
    publishToPropertyTree();
  }

  State::State(boost::shared_ptr<Device> _device, int _inputIndex) :
      m_name("dev." + _device->getDSID().toString() + "." + intToString(_inputIndex)),
      m_IsPersistent(true),
      m_state(State_Invalid),
      m_type(StateType_Device),
      m_providerDev(_device),
      m_providerDevInput(_inputIndex)
  {
    load();
    publishToPropertyTree();
  }

  State::~State() {
    removeFromPropertyTree();
  }

  void State::publishToPropertyTree() {
    removeFromPropertyTree();
    if (m_pPropertyNode == NULL && DSS::hasInstance()) {
      if (DSS::getInstance()->getPropertySystem().getProperty("/usr/states/" + m_name) != NULL) {
        // avoid publishing duplicate state information when reading out devices multiple times
        return;
      }
      m_pPropertyNode = DSS::getInstance()->getPropertySystem().createProperty("/usr/states/" + m_name);
      m_pPropertyNode->createProperty("name")
        ->linkToProxy(PropertyProxyReference<std::string>(m_name, false));
      m_pPropertyNode->createProperty("value")
        ->linkToProxy(PropertyProxyReference<int>((int &) m_state, false));
      m_pPropertyNode->createProperty("state")
        ->linkToProxy(PropertyProxyMemberFunction<State, std::string, false>(*this, &State::toString));

      if (m_providerDev != NULL) {
        PropertyNodePtr devNode = m_providerDev->getPropertyNode();
        PropertyNodePtr m_pAliasNode = m_pPropertyNode->createProperty("device/" + m_providerDev->getDSID().toString());
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
    switch(m_state) {
    case State_Unkown:
      return std::string("unknown");
      break;
    case State_Active:
      return std::string("active");
      break;
    case State_Inactive:
      return std::string("inactive");
      break;
    default:
      break;
    }
    return std::string("invalid");
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

      boost::shared_ptr<Event> pEvent;
      pEvent.reset(new Event("stateChange", shared_from_this()));

      pEvent->setProperty("statename", m_name);
      pEvent->setProperty("state", toString());
      pEvent->setProperty("value", intToString((int) m_state));
      pEvent->setProperty("oldvalue", intToString((int) oldstate));
      pEvent->setProperty("originDeviceID", intToString((int) _origin));

      if (DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(pEvent);
      }
    }
  } // setState

  void State::setState(const callOrigin_t _origin, const int _state) {
    switch (_state) {
      case 1: setState(_origin, State_Active); break;
      case 2: setState(_origin, State_Inactive); break;
      case 3: setState(_origin, State_Unkown); break;
      default: break;
    }
  }

  void State::setState(const callOrigin_t _origin, const std::string& _state) {
    if (_state == "active") {
      setState(_origin, State_Active);
    } else if (_state == "inactive") {
      setState(_origin, State_Inactive);
    } else if (_state == "unknown") {
      setState(_origin, State_Unkown);
    }
  }

} // namespace dss
