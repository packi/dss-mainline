/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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


#include <signal.h>
#include <set>

#include "ds/log.h"

#include "base.h"
#include "dss.h"
#include "event/event_create.h"
#include "event/event_fields.h"
#include "eventinterpretersystemplugins.h"
#include "handler/action-execute.h"
#include "handler/system_triggers.h"
#include "handler/event-logger.h"
#include "http_client.h"
#include "internaleventrelaytarget.h"
#include "logger.h"
#include "model/set.h"
#include "model/zone.h"
#include "model/group.h"
#include "model/cluster.h"
#include "model/device.h"
#include "model/apartment.h"
#include "model/state.h"
#include "model/modelconst.h"
#include "model/modulator.h"
#include "model/data_types.h"
#include "propertysystem.h"
#include "security/security.h"
#include "util.h"

namespace dss {
  SystemEventActionExecute::SystemEventActionExecute(const Event &event) : SystemEvent(event) {
    if (event.hasPropertySet("path")) {
      m_path = event.getPropertyByName("path");
      if (event.hasPropertySet("delay")) {
        m_delay = event.getPropertyByName("delay");
      }
    } else {
      DS_FAIL_REQUIRE("missing 'path' property ignoring event");
    }
  }

  SystemEventActionExecute::~SystemEventActionExecute() = default;

  void SystemEventActionExecute::run() {
    if (DSS::hasInstance()) {
      DSS::getInstance()->getSecurity().loginAsSystemUser(
        "EventInterpreterPluginActionExecute needs system rights");
    } else {
      return;
    }

    ActionExecute actionExecute(m_properties);
    if (!m_delay.empty()) {
      actionExecute.executeWithDelay(m_path, m_delay);
    } else {
      actionExecute.execute(m_path);
    }
  }

  SystemEventHighlevel::SystemEventHighlevel(const Event &event) : SystemEvent(event) {}
  SystemEventHighlevel::~SystemEventHighlevel() = default;

  void SystemEventHighlevel::run() {
    if (!m_properties.has("id")) {
      return;
    }

    std::string id = m_properties.get("id");
    if (id.empty()) {
      return;
    }

    if (!DSS::hasInstance()) {
      return;
    }
    // check for user defined actions
    PropertyNodePtr baseNode =
        DSS::getInstance()->getPropertySystem().getProperty("/usr/events");
    if (baseNode == NULL) {
      // ok if no hle's found
      return;
    }

    if (baseNode->getChildCount() == 0) {
      return;
    }
    for (int i = 0; i < baseNode->getChildCount(); i++) {
      PropertyNodePtr parent = baseNode->getChild(i);
      if (parent != NULL) {
        PropertyNodePtr idNode = parent->getPropertyByName("id");
        if ((idNode != NULL) &&
            (idNode->getStringValue() == id)) {
          std::string fullName = parent->getDisplayName();
          PropertyNode* nextNode = parent->getParentNode();
          while(nextNode != NULL) {
            if(nextNode->getParentNode() != NULL) {
              fullName = nextNode->getDisplayName() + "/" + fullName;
            } else {
              fullName = "/" + fullName;
            }
            nextNode = nextNode->getParentNode();
          }
          ActionExecute(m_properties).execute(fullName);
          return;
        }
      }
    } // for loop
    Logger::getInstance()->log("SystemEventHighlevel::"
            "run: event for id " + id + " not found", lsError);

  }

  EventInterpreterPluginActionExecute::EventInterpreterPluginActionExecute(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("action_execute", _pInterpreter)
  { }

  EventInterpreterPluginActionExecute::~EventInterpreterPluginActionExecute()
  { }

  void EventInterpreterPluginActionExecute::handleEvent(Event& _event, const EventSubscription& _subscription) {
    Logger::getInstance()->log("EventInterpreterPluginActionExecute::"
            "handleEvent: processing event " + _event.getName());

    auto action = boost::make_shared<SystemEventActionExecute>(_event);
    addEvent(action);
  }


  EventInterpreterPluginSystemTrigger::EventInterpreterPluginSystemTrigger(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("system_trigger", _pInterpreter)
  { }

  EventInterpreterPluginSystemTrigger::~EventInterpreterPluginSystemTrigger()
  { }

  void EventInterpreterPluginSystemTrigger::handleEvent(Event& _event, const EventSubscription& _subscription) {
    Logger::getInstance()->log("EventInterpreterPluginSystemTrigger::"
            "handleEvent: processing event \'" + _event.getName() + "\'",
            lsDebug);

    auto trigger = boost::make_shared<SystemTrigger>(_event);
    trigger->run();
  }

  EventInterpreterPluginHighlevelEvent::EventInterpreterPluginHighlevelEvent(EventInterpreter* _pInterpreter) : 
      EventInterpreterPlugin("highlevelevent", _pInterpreter) {
  }

  EventInterpreterPluginHighlevelEvent::~EventInterpreterPluginHighlevelEvent()
  { }

  void EventInterpreterPluginHighlevelEvent::handleEvent(Event& _event, const EventSubscription& _subscription) {
    Logger::getInstance()->log("EventInterpreterPluginHighlevelEvent::"
            "handleEvent " + _event.getName(), lsDebug);

    // triggers might be launched on any event, no filter on event name
    auto trigger = boost::make_shared<SystemTrigger>(_event);

    // TODO(cleanup): probably filter on 'highlevelevent' or merge with 'action_execute' plugin
    auto hl = boost::make_shared<SystemEventHighlevel>(_event);
    addEvent(trigger);
    addEvent(hl);

    // TODO(cleanup): plugin 'system_event_log' not registered on 'highlevelevent' in subscriptions.xml
    auto log = boost::make_shared<SystemEventLog>(_event);
    addEvent(log);
  }

  EventInterpreterPluginSystemEventLog::EventInterpreterPluginSystemEventLog(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("system_event_log", _pInterpreter)
  { }

  EventInterpreterPluginSystemEventLog::~EventInterpreterPluginSystemEventLog()
  { }

  void EventInterpreterPluginSystemEventLog::handleEvent(Event& _event, const EventSubscription& _subscription) {
    Logger::getInstance()->log("EventInterpreterPluginSystemEventLog::"
            "handleEvent: processing event \'" + _event.getName() + "\'",
            lsDebug);

    auto log = boost::make_shared<SystemEventLog>(_event);
    addEvent(log);
  }

  EventInterpreterPluginSystemZoneSensorForward::EventInterpreterPluginSystemZoneSensorForward(EventInterpreter* _pInterpreter)
    : EventInterpreterPlugin("system_zonesensor_forward", _pInterpreter)
    { }

  EventInterpreterPluginSystemZoneSensorForward::~EventInterpreterPluginSystemZoneSensorForward()
   { }

  void EventInterpreterPluginSystemZoneSensorForward::subscribe() {
    auto&& interpreter = getEventInterpreter();
    interpreter.subscribe(boost::make_shared<EventSubscription>(*this, "deviceSensorValue"));
  }

  void EventInterpreterPluginSystemZoneSensorForward::handleEvent(Event& _event, const EventSubscription& _subscription) {
    Logger::getInstance()->log("EventInterpreterPluginSystemZoneSensorForward::"
            "handleEvent: processing event \'" + _event.getName() + "\'",
            lsDebug);

    auto handler = boost::make_shared<SystemZoneSensorForward>(_event);
    addEvent(handler);
  }

  SystemZoneSensorForward::SystemZoneSensorForward(const Event &event) : SystemEvent(event) {}
  SystemZoneSensorForward::~SystemZoneSensorForward() = default;

  void SystemZoneSensorForward::run() {
    if (DSS::hasInstance()) {
      DSS::getInstance()->getSecurity().loginAsSystemUser(
        "SystemZoneSensorForward needs system rights");
    } else {
      return;
    }

    if (m_evtName == "deviceSensorValue") {
      deviceSensorValue();
    }
  }

  void SystemZoneSensorForward::deviceSensorValue() {
    if (m_raisedAtDevice != NULL) {
      try {
        auto sensorType = SensorType::UnknownType;
        boost::shared_ptr<const Device> pDevice = m_raisedAtDevice->getDevice();

        if (m_properties.has("sensorType")) {
          sensorType = static_cast<SensorType>(strToInt(m_properties.get("sensorType")));
        } else {
          try {
            std::string sensorIndex("-1");
            if (m_properties.has("sensorIndex")) {
              sensorIndex = m_properties.get("sensorIndex");
            }
            boost::shared_ptr<DeviceSensor_t> pSensor = pDevice->getSensor(strToInt(sensorIndex));
            sensorType = pSensor->m_sensorType;
          } catch (ItemNotFoundException& ex) {}
        }

        // filter out sensor types that are handled by the (v)dSM
        int zoneId = m_raisedAtDevice->getDevice()->getZoneID();
        switch (sensorType) {
          case SensorType::TemperatureIndoors:
          case SensorType::HumidityIndoors:
          case SensorType::BrightnessIndoors:
          case SensorType::CO2Concentration:
            return;
          case SensorType::TemperatureOutdoors:
          case SensorType::BrightnessOutdoors:
          case SensorType::HumidityOutdoors:
          case SensorType::WindSpeed:
          case SensorType::WindDirection:
          case SensorType::GustSpeed:
          case SensorType::GustDirection:
          case SensorType::Precipitation:
          case SensorType::AirPressure:
            zoneId = 0;
            break;
          default:
            // for other types we just try to find proper device if it exist
            break;
        }
        boost::shared_ptr<Zone> pZone = DSS::getInstance()->getApartment().getZone(zoneId);
        boost::shared_ptr<Device> sDevice = pZone->getAssignedSensorDevice(sensorType);

        std::string sensorValue;
        if (m_properties.has("sensorValue")) {
          sensorValue = m_properties.get("sensorValue");
        }
        std::string sensorValueFloat;
        if (m_properties.has("sensorValueFloat")) {
          sensorValueFloat = m_properties.get("sensorValueFloat");
        }

        if (sDevice && sDevice->getDSID() == pDevice->getDSID()) {
          pZone->pushSensor(coSystem, SAC_MANUAL, pDevice->getDSID(), sensorType, strToDouble(sensorValueFloat), "");
        }
      } catch (ItemNotFoundException &ex) {}
    }
  }

} // namespace dss
