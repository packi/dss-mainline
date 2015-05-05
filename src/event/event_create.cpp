/*
 *  Copyright (c) 2015 digitalSTROM.org, Zurich, Switzerland
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

#include "event_create.h"

#include <boost/make_shared.hpp>

#include "event/event_fields.h"
#include "ds485types.h"
#include "model/scenehelper.h"
#include "model/state.h"

namespace dss {

namespace EventName {
  const std::string CallScene = "callScene";
  const std::string CallSceneBus = "callSceneBus";
  const std::string DeviceSensorValue = "deviceSensorValue";
  const std::string DeviceStatus = "deviceStatusEvent";
  const std::string DeviceInvalidSensor = "deviceInvalidSensor";
  const std::string DeviceBinaryInputEvent = "deviceBinaryInputEvent";
  const std::string ExecutionDenied = "executionDenied";
  const std::string Running = "running";
  const std::string UndoScene = "undoScene";
  const std::string ZoneSensorValue = "zoneSensorValue";
  const std::string ZoneSensorError = "zoneSensorError";
  const std::string StateChange = "stateChange";
  const std::string AddonStateChange = "addonStateChange";
  const std::string HeatingEnabled = "HeatingEnabled";
  const std::string HeatingControllerSetup = "HeatingControllerSetup";
  const std::string HeatingControllerValue = "HeatingControllerValue";
  const std::string HeatingControllerValueDsHub = "HeatingControllerValueDsHub";
  const std::string HeatingControllerState = "HeatingControllerState";
  const std::string OldStateChange = "oldStateChange";
  const std::string AddonToCloud = "AddonToCloud";
  const std::string HeatingValveProtection = "execute_valve_protection";
  const std::string DeviceHeatingTypeChanged = "DeviceHeatingTypeChanged";
  const std::string LogFileData = "logFileData";
}

boost::shared_ptr<Event>
createDeviceStatusEvent(boost::shared_ptr<DeviceReference> _devRef,
                        int _index, int _value)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::DeviceStatus, _devRef);
  event->setProperty("statusIndex", intToString(_index));
  event->setProperty("statusValue", intToString(_value));
  return event;
}

boost::shared_ptr<Event>
createDeviceBinaryInputEvent(boost::shared_ptr<DeviceReference> _devRef,
                             int _index, int _type, int _state)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::DeviceBinaryInputEvent, _devRef);
  event->setProperty("inputIndex", intToString(_index));
  event->setProperty("inputType", intToString(_type));
  event->setProperty("inputState", intToString(_state));
  return event;
}

boost::shared_ptr<Event>
createDeviceSensorValueEvent(boost::shared_ptr<DeviceReference> _devRef,
                             int _index, int _type, int _value)
{
  boost::shared_ptr<Event> event;
  // TODO ensure sensorType valid or implement fallback
  double floatValue = SceneHelper::sensorToFloat12(_type, _value);

  event = boost::make_shared<Event>(EventName::DeviceSensorValue, _devRef);
  event->setProperty(ef_sensorIndex, intToString(_index));
  event->setProperty("sensorType", intToString(_type));
  event->setProperty("sensorValue", intToString(_value));
  event->setProperty("sensorValueFloat", doubleToString(floatValue));
  return event;
}

boost::shared_ptr<Event>
createDeviceInvalidSensorEvent(boost::shared_ptr<DeviceReference> _devRef,
                               int _index, int _type, const DateTime& _ts)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::DeviceInvalidSensor, _devRef);
  event->setProperty(ef_sensorIndex, intToString(_index));
  event->setProperty("sensorType", intToString(_type));
  event->setProperty("lastValueTS", _ts.toISO8601_ms());
  return event;
}

boost::shared_ptr<Event>
createZoneSensorValueEvent(boost::shared_ptr<Group> _group, int _type,
                           int _value, const dsuid_t &_sourceDevice)
{
  boost::shared_ptr<Event> event;
  // TODO ensure sensorType valid or implement fallback
  double floatValue = SceneHelper::sensorToFloat12(_type, _value);

  event = boost::make_shared<Event>(EventName::ZoneSensorValue, _group);
  event->setProperty("sensorType", intToString(_type));
  event->setProperty("sensorValue", intToString(_value));
  event->setProperty("sensorValueFloat", doubleToString(floatValue));
  event->setProperty("originDSID", dsuid2str(_sourceDevice));
  return event;
}

boost::shared_ptr<Event>
createZoneSensorErrorEvent(boost::shared_ptr<Group> _group, int _type,
                      const DateTime& _ts)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::ZoneSensorError, _group);
  event->setProperty("sensorType", intToString(_type));
  event->setProperty("lastValueTS", _ts.toISO8601_ms());
  return event;
}

boost::shared_ptr<Event>
createGroupCallSceneEvent(boost::shared_ptr<Group> _group, int _sceneID,
                          int _groupID, int _zoneID,
                          const callOrigin_t& _callOrigin,
                          const dsuid_t& _originDSUID,
                          const std::string& _originToken,
                          bool _forced)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::CallScene, _group);
  event->setProperty(ef_sceneID, intToString(_sceneID));
  event->setProperty("groupID", intToString(_groupID));
  event->setProperty("zoneID", intToString(_zoneID));
  event->setProperty(ef_originDSUID, dsuid2str(_originDSUID));
  event->setProperty(ef_callOrigin, intToString(_callOrigin));
  event->setProperty("originToken", _originToken);
  if (_forced) {
    event->setProperty(ef_forced, "true");
  }
  return event;
}

boost::shared_ptr<Event>
createGroupUndoSceneEvent(boost::shared_ptr<Group> _group, int _sceneID,
                          int _groupID, int _zoneID,
                          const callOrigin_t& _callOrigin,
                          const dsuid_t& _originDSUID,
                          const std::string& _originToken)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::UndoScene, _group);
  event->setProperty(ef_sceneID, intToString(_sceneID));
  event->setProperty("groupID", intToString(_groupID));
  event->setProperty("zoneID", intToString(_zoneID));
  event->setProperty(ef_originDSUID, dsuid2str(_originDSUID));
  event->setProperty(ef_callOrigin, intToString(_callOrigin));
  event->setProperty("originToken", _originToken);
  return event;
}

boost::shared_ptr<Event>
createStateChangeEvent(boost::shared_ptr<State> _state, int _oldstate,
                       callOrigin_t _callOrigin)
{
  boost::shared_ptr<Event> event;

  if (_state->getType() == StateType_Script) {
    event = boost::make_shared<Event>(EventName::AddonStateChange, _state);
    event->setProperty("scriptID", _state->getProviderService());
  } else {
    event = boost::make_shared<Event>(EventName::StateChange, _state);
  }

  event->setProperty("statename", _state->getName());
  event->setProperty("state", _state->toString());
  event->setProperty("value", intToString(_state->getState()));
  event->setProperty("oldvalue", intToString(_oldstate));
  event->setProperty("originDeviceID", intToString(_callOrigin));
  return event;
}

boost::shared_ptr<Event>
createActionDenied(const std::string &_type, const std::string &_name,
                   const std::string &_source, const std::string &_reason)
{
  boost::shared_ptr<Event> event;

  event = boost::make_shared<Event>(EventName::ExecutionDenied);
  event->setProperty("action-type", _type);
  event->setProperty("action-name", _name);
  event->setProperty("source-name", _source);
  event->setProperty("reason", _reason);
  return event;
}

/**
 * this function is used only by unit tests
 * the real event is created from java script
 * TODO: - add meta-event description of required fileds
 *       - upon raise event verify event matches description
 */
boost::shared_ptr<Event>
createHeatingEnabled(int _zoneID, bool _enabled)
{
  boost::shared_ptr<Event> event;

  event = boost::make_shared<Event>(EventName::HeatingEnabled);
  event->setProperty("zoneID", intToString(_zoneID));
  event->setProperty("HeatingEnabled", _enabled ? "true" : "false");
  return event;
}

}
