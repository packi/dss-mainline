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
  /// generic mechanism to perform action
  /// event must contain 'path' property to pass remaining action paramters
  class SystemEventActionExecute : public SystemEvent {
    public:
      SystemEventActionExecute(const Event &event);
      virtual ~SystemEventActionExecute();
      void run() DS_OVERRIDE;
    private:
      std::string m_delay;
      std::string m_path;
  };

  /// UDA: expects action paramters in /usr/events/<id>/
  /// event must contain 'path' property
  class SystemEventHighlevel : public SystemEvent {
    public:
      SystemEventHighlevel(const Event &event);
      virtual ~SystemEventHighlevel();
      void run() DS_OVERRIDE;
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
      SystemZoneSensorForward(const Event &event);
      virtual ~SystemZoneSensorForward();
      void run() DS_OVERRIDE;
    private:
      void deviceSensorValue();
  };

}; // namespace

#endif
