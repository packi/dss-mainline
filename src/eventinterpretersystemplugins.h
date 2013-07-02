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
#include "taskprocessor.h"

#include <pthread.h>
#include "src/ds485types.h"

namespace dss {
  class Device;

  class SystemEvent : public Task {
    public:
      SystemEvent();
      virtual ~SystemEvent();
      virtual void run() = 0;
      virtual bool setup(Event& _event);
      std::string getActionName(PropertyNodePtr _actionNode);
    protected:
      Properties m_properties;
  };

  class SystemTrigger : public SystemEvent {
    public:
      SystemTrigger();
      virtual ~SystemTrigger();
      virtual void run();
      virtual bool setup(Event& _event);

    protected:
      std::string m_evtName;
      bool m_evtSrcIsGroup;
      bool m_evtSrcIsDevice;
      int m_evtSrcZone;
      int m_evtSrcGroup;
      dss_dsid_t m_evtSrcDSID;

      bool checkSceneZone(PropertyNodePtr _triggerProp);
      bool checkUndoSceneZone(PropertyNodePtr _triggerProp);
      bool checkDeviceScene(PropertyNodePtr _triggerProp);
      bool checkDeviceSensor(PropertyNodePtr _triggerProp);
      bool checkDeviceBinaryInput(PropertyNodePtr _triggerProp);
      bool checkDevice(PropertyNodePtr _tiggerProp);
      bool checkHighlevel(PropertyNodePtr _triggerProp);
      bool checkState(PropertyNodePtr _triggerProp);
      bool checkTrigger(std::string _path);
      void relayTrigger(PropertyNodePtr _releay);
  };

  class SystemEventActionExecute : public SystemEvent {
    public:
      SystemEventActionExecute();
      virtual ~SystemEventActionExecute();
      virtual void run();
      virtual bool setup(Event& _event);
    private:
      std::string m_delay;
      std::string m_path;

      static void *staticThreadProc(void *arg);
      boost::shared_ptr<Device> getDeviceFromNode(PropertyNodePtr _actionNode);
      void executeZoneScene(PropertyNodePtr _actionNode);
      void executeZoneUndoScene(PropertyNodePtr _actionNode);
      void executeDeviceScene(PropertyNodePtr _actionNode);
      void executeDeviceValue(PropertyNodePtr _actionNode);
      void executeDeviceBlink(PropertyNodePtr _actionNode);
      void executeZoneBlink(PropertyNodePtr _actionNode);
      void executeCustomEvent(PropertyNodePtr _actionNode);
      void executeURL(PropertyNodePtr _actionNode);
      void executeStateChange(PropertyNodePtr _actionNode);
      void executeAddonStateChange(PropertyNodePtr _actionNode);
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
      virtual bool setup(Event& _event);
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
                                            public TaskProcessor,
                                            public EventInterpreterPlugin {
    public:
      EventInterpreterPluginSystemTrigger(EventInterpreter* _pInterpreter);
      virtual ~EventInterpreterPluginSystemTrigger();
      virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };
}; // namespace

#endif
