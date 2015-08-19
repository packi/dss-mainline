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
      dsuid_t m_evtSrcDSID;

      bool checkSceneZone(PropertyNodePtr _triggerProp);
      bool checkUndoSceneZone(PropertyNodePtr _triggerProp);
      bool checkDeviceScene(PropertyNodePtr _triggerProp);
      bool checkDeviceSensor(PropertyNodePtr _triggerProp);
      bool checkDeviceBinaryInput(PropertyNodePtr _triggerProp);
      bool checkDevice(PropertyNodePtr _tiggerProp);
      bool checkHighlevel(PropertyNodePtr _triggerProp);
      bool checkState(PropertyNodePtr _triggerProp);
      bool checkSensorValue(PropertyNodePtr _triggerProp);
      bool checkEvent(PropertyNodePtr _triggerProp);
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
      virtual void subscribe();
  };

  class SystemEventLog : public SystemEvent {
    public:
      SystemEventLog();
      virtual ~SystemEventLog();
      virtual void run();
      virtual bool setup(Event& _event);
    private:
      std::string getZoneName(boost::shared_ptr<Zone> _zone);
      std::string getGroupName(boost::shared_ptr<Group> _group);
      std::string getSceneName(int _sceneID, int _groupID);
      std::string getDeviceName(std::string _orgin_device_id);
      std::string getCallOrigin(callOrigin_t _call_origin);
      void logDeviceLocalScene(boost::shared_ptr<ScriptLogger> _logger,
                               int _scene_id, std::string _origin_dsid,
                               callOrigin_t _call_origin);
      void logZoneGroupScene(boost::shared_ptr<ScriptLogger> _logger,
                             boost::shared_ptr<Zone> _zone,
                             int _group_id, int _scene_id,
                             bool _is_forced, std::string _origin_dsid,
                             callOrigin_t _call_origin,
                             std::string _origin_token);
      void logDeviceScene(boost::shared_ptr<ScriptLogger> _logger,
                          boost::shared_ptr<const Device> _device,
                          boost::shared_ptr<Zone> _zone,
                          int scene_id, bool _is_forced,
                          std::string _origin_dsid,
                          callOrigin_t _call_origin,
                          std::string _token);
      void logLastScene(boost::shared_ptr<ScriptLogger> _logger,
                        boost::shared_ptr<Zone> _zone,
                        boost::shared_ptr<Group> _group, int _scene_id);
      void logZoneGroupBlink(boost::shared_ptr<ScriptLogger> _logger,
                             boost::shared_ptr<Zone> _zone,
                             int _group_id, std::string _origin_dsid,
                             callOrigin_t _call_origin,
                             std::string _origin_token);
      void logDeviceBlink(boost::shared_ptr<ScriptLogger> _logger,
                          boost::shared_ptr<const Device> _device,
                          boost::shared_ptr<Zone> _zone,
                          std::string _origin_device_id,
                          callOrigin_t _call_origin,
                          std::string _token);
      void logZoneGroupUndo(boost::shared_ptr<ScriptLogger> _logger,
                            boost::shared_ptr<Zone> _zone,
                            int _group_id, int _scene_id,
                            std::string _origin_dsid,
                            callOrigin_t _call_origin,
                            std::string _origin_token);
      void logDeviceButtonClick(boost::shared_ptr<ScriptLogger> _logger,
                                boost::shared_ptr<const Device> _device);
      void logDeviceBinaryInput(boost::shared_ptr<ScriptLogger> _logger,
                                boost::shared_ptr<const Device> _device);
      void logDeviceSensorEvent(boost::shared_ptr<ScriptLogger> _logger,
                                boost::shared_ptr<const Device> _device);
      void logDeviceSensorValue(boost::shared_ptr<ScriptLogger> _logger,
                                boost::shared_ptr<const Device> _device,
                                boost::shared_ptr<Zone> _zone);
      void logZoneSensorValue(boost::shared_ptr<ScriptLogger> _logger,
                              boost::shared_ptr<Zone> _zone,
                              int _groupId);
      void logStateChange(boost::shared_ptr<ScriptLogger> _logger,
          boost::shared_ptr<const State> _st,
          const std::string& _statename,
          const std::string& _state,
          const std::string& _value,
          const std::string& _origin_device_id);
      void logSunshine(boost::shared_ptr<ScriptLogger> _logger,
                       std::string _value,
                       std::string _direction,
                       std::string _originDeviceID);
      void logFrostProtection(boost::shared_ptr<ScriptLogger> _logger,
                              std::string _value,
                              std::string _originDeviceID);
      void logHeatingModeSwitch(boost::shared_ptr<ScriptLogger> _logger,
                                std::string _value,
                                std::string _originDeviceID);
      void logBuildingService(boost::shared_ptr<ScriptLogger> _logger,
                              std::string _value,
                              std::string _originDeviceID);
      void logExecutionDenied(boost::shared_ptr<ScriptLogger> _logger,
                              std::string _action, std::string _reason);
      void logOperationLock(boost::shared_ptr<ScriptLogger> _logger,
                            boost::shared_ptr<Zone> _zone,
                            int _groupId,
                            int _lock,
                            callOrigin_t _call_origin);
      void logDevicesFirstSeen(boost::shared_ptr<ScriptLogger> _logger,
                               std::string& dateTime, 
                               std::string& token,
                               callOrigin_t _call_origin);

      void model_ready();
      void callScene();
      void blink();
      void undoScene();
      void buttonClick();
      void deviceBinaryInputEvent();
      void deviceSensorEvent();
      void deviceSensorValue();
      void zoneSensorValue();
      void stateChange();
      void sunshine();
      void frostProtection();
      void heatingModeSwitch();
      void buildingService();
      void executionDenied();
      void operationLock(const std::string& _evtName);
      void devicesFirstSeen();

      std::string m_evtName;

      EventRaiseLocation m_evtRaiseLocation;
      boost::shared_ptr<const Group> m_raisedAtGroup;
      boost::shared_ptr<const DeviceReference> m_raisedAtDevice;
      boost::shared_ptr<const State> m_raisedAtState;
  };
}; // namespace

#endif
