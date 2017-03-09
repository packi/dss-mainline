/*
 *  Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "system_handlers.h"

#include <boost/make_shared.hpp>

#include "model/deviceinterface.h"
#include "scripting/jslogger.h"

namespace dss {
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
      void logDirectDeviceAction(boost::shared_ptr<ScriptLogger> _logger,
                                 boost::shared_ptr<const Device> _device);
      void logDeviceNamedAction(boost::shared_ptr<ScriptLogger> _logger,
                                 boost::shared_ptr<const Device> _device);
      void logDeviceNamedEvent(boost::shared_ptr<ScriptLogger> _logger,
                                 boost::shared_ptr<const Device> _device);
      void logDeviceNamedState(boost::shared_ptr<ScriptLogger> _logger,
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
          const std::string& _origin_device_id,
          const callOrigin_t _callOrigin);
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
      void logHighLevelEvent(boost::shared_ptr<ScriptLogger> _logger,
                             std::string& _id,
                             std::string& _name);

      void model_ready();
      void callScene();
      void blink();
      void undoScene();
      void buttonClick();
      void directDeviceAction();
      void deviceNamedAction();
      void deviceNamedEvent();
      void deviceNamedState();
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
      void highlevelevent();

      std::string m_evtName;

      EventRaiseLocation m_evtRaiseLocation;
      boost::shared_ptr<const Group> m_raisedAtGroup;
      boost::shared_ptr<const DeviceReference> m_raisedAtDevice;
      boost::shared_ptr<const State> m_raisedAtState;
  };
}; // namespace
