/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Authors: Sergey 'Jin' Bostandznyan <jin@dev.digitalstrom.org>

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

#ifndef __EVENTINTERPRETERSYSTEMPLUGINS_H__
#define __EVENTINTERPRETERSYSTEMPLUGINS_H__

#include "eventinterpreterplugins.h"
#include <pthread.h>

#include "ds485types.h"
#include "handler/system_handlers.h"
#include "scripting/jslogger.h"
#include "model/deviceinterface.h"

namespace dss {
  class Device;

  class SystemEventActionExecute : public SystemEvent {
    public:
      SystemEventActionExecute();
      virtual ~SystemEventActionExecute();
      virtual void run();
      virtual bool setup(Event& _event);
    private:
      std::string m_delay;
      std::string m_path;

      std::string getActionName(PropertyNodePtr _actionNode);
      boost::shared_ptr<Device> getDeviceFromNode(PropertyNodePtr _actionNode);
      void executeZoneScene(PropertyNodePtr _actionNode);
      void executeZoneUndoScene(PropertyNodePtr _actionNode);
      void executeDeviceScene(PropertyNodePtr _actionNode);
      void executeDeviceChannelValue(PropertyNodePtr _actionNode);
      void executeDeviceValue(PropertyNodePtr _actionNode);
      void executeDeviceBlink(PropertyNodePtr _actionNode);
      void executeDeviceAction(PropertyNodePtr _actionNode);
      void executeZoneBlink(PropertyNodePtr _actionNode);
      void executeCustomEvent(PropertyNodePtr _actionNode);
      void executeURL(PropertyNodePtr _actionNode);
      void executeStateChange(PropertyNodePtr _actionNode);
      void executeAddonStateChange(PropertyNodePtr _actionNode);
      void executeHeatingMode(PropertyNodePtr _actionNode);
      unsigned int executeOne(PropertyNodePtr _actionNode);
      void executeStep(std::vector<PropertyNodePtr> _actionNodes);
      std::vector<PropertyNodePtr> filterActionsWithDelay(PropertyNodePtr _actionNode, int _delayValue);
      void executeWithDelay(std::string _path, std::string _delay);
    protected:
      void execute(std::string _path);

  };

  class SystemEventHighlevel : public SystemEventActionExecute {
    public:
      SystemEventHighlevel();
      virtual ~SystemEventHighlevel();
      virtual void run();
  };

  class EventInterpreterPluginActionExecute : public TaskProcessor,
                                              public EventInterpreterPlugin {
    public:
      EventInterpreterPluginActionExecute(EventInterpreter* _pInterpreter);
      virtual ~EventInterpreterPluginActionExecute();
      virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };

  class EventInterpreterPluginHighlevelEvent : public TaskProcessor,
                                               public EventInterpreterPlugin {
    public:
      EventInterpreterPluginHighlevelEvent(EventInterpreter* _pInterpreter);
      virtual ~EventInterpreterPluginHighlevelEvent();
      virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };

  class EventInterpreterPluginSystemTrigger : 
                                            public EventInterpreterPlugin {
    public:
      EventInterpreterPluginSystemTrigger(EventInterpreter* _pInterpreter);
      virtual ~EventInterpreterPluginSystemTrigger();
      virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };

  class EventInterpreterPluginSystemEventLog :
                                            public TaskProcessor,
                                            public EventInterpreterPlugin {
    public:
      EventInterpreterPluginSystemEventLog(EventInterpreter* _pInterpreter);
      virtual ~EventInterpreterPluginSystemEventLog();
      virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };


  class EventInterpreterPluginSystemZoneSensorForward :
                                            public TaskProcessor,
                                            public EventInterpreterPlugin {
    public:
      EventInterpreterPluginSystemZoneSensorForward(EventInterpreter* _pInterpreter);
      virtual ~EventInterpreterPluginSystemZoneSensorForward();
      virtual void subscribe();
      virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };

  class SystemZoneSensorForward : public SystemEvent {
    public:
      SystemZoneSensorForward();
      virtual ~SystemZoneSensorForward();
      virtual void run();
    private:
      void deviceSensorValue();
  };

}; // namespace

#endif
