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
  const std::string DeviceSensorEvent = "deviceSensorEvent";
  const std::string DeviceStatus = "deviceStatusEvent";
  const std::string DeviceInvalidSensor = "deviceInvalidSensor";
  const std::string DeviceBinaryInputEvent = "deviceBinaryInputEvent";
  const std::string DeviceButtonClick = "buttonClick";
  const std::string IdentifyBlink = "blink";
  const std::string ExecutionDenied = "executionDenied";
  const std::string Running = "running";
  const std::string ModelReady = "model_ready";
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
  const std::string DebugMonitorUpdate = "debugMonitorUpdate";
  const std::string Sunshine = "sunshine";
  const std::string FrostProtection = "frostprotection";
  const std::string HeatingModeSwitch = "heating_mode_switch";
  const std::string BuildingService = "building_service";
  const std::string OperationLock = "operation_lock";
  const std::string ClusterConfigLock = "cluster_config_lock";
  const std::string CheckSensorValues = "check_sensor_values";
  const std::string DeviceEvent = "DeviceEvent";
  const std::string CheckHeatingGroups = "check_heating_groups";
  const std::string Signal = "SIGNAL";
  const std::string WebSessionCleanup = "webSessionCleanup";
  const std::string SendMail = "sendmail";
  const std::string ButtonClickBus = "buttonClickBus";
  const std::string DSMeterReady = "dsMeter_ready";
  const std::string ExecutionDeniedDigestCheck = "execution_denied_digest_check";
  const std::string DevicesFirstSeen = "devices_first_seen";
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

boost::shared_ptr<Event>
createHeatingControllerConfig(int _zoneID, const dsuid_t &_ctrlDsuid,
                              const ZoneHeatingConfigSpec_t &_config)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::HeatingControllerSetup);

  event->setProperty("ZoneID", intToString(_zoneID));
  event->setProperty("ControlDSUID", dsuid2str(_ctrlDsuid));
  event->setProperty("ControlMode", intToString(_config.ControllerMode));
  event->setProperty("EmergencyValue", intToString(_config.EmergencyValue - 100));
  if (_config.ControllerMode == HeatingControlModeIDPID) {
    event->setProperty("CtrlKp", doubleToString((double)_config.Kp * 0.025));
    event->setProperty("CtrlTs", intToString(_config.Ts));
    event->setProperty("CtrlTi", intToString(_config.Ti));
    event->setProperty("CtrlKd", intToString(_config.Kd));
    event->setProperty("CtrlImin", doubleToString((double)_config.Imin * 0.025));
    event->setProperty("CtrlImax", doubleToString((double)_config.Imax * 0.025));
    event->setProperty("CtrlYmin", intToString(_config.Ymin - 100));
    event->setProperty("CtrlYmax", intToString(_config.Ymax - 100));
    event->setProperty("CtrlAntiWindUp", (_config.AntiWindUp > 0) ? "true" : "false");
    event->setProperty("CtrlKeepFloorWarm", (_config.KeepFloorWarm > 0) ? "true" : "false");
  } else if (_config.ControllerMode == HeatingControlModeIDZoneFollower) {
    event->setProperty("ReferenceZone", intToString(_config.SourceZoneId));
    event->setProperty("CtrlOffset", intToString(_config.Offset));
  } else if (_config.ControllerMode == HeatingControlModeIDManual) {
    event->setProperty("ManualValue", intToString(_config.ManualValue - 100));
  }
  return event;
}

boost::shared_ptr<Event>
createHeatingControllerValue(int _zoneID, const dsuid_t &_ctrlDsuid,
                             const ZoneHeatingProperties_t &_properties,
                             const ZoneHeatingOperationModeSpec_t &_mode)
{
    boost::shared_ptr<Event> event;
    event = boost::make_shared<Event>(EventName::HeatingControllerValue);

    event->setProperty("ZoneID", intToString(_zoneID));
    event->setProperty("ControlDSUID", dsuid2str(_ctrlDsuid));
    if (_properties.m_HeatingControlMode == HeatingControlModeIDPID) {
      event->setProperty("NominalTemperature_Off",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, _mode.OpMode0)));
      event->setProperty("NominalTemperature_Comfort",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, _mode.OpMode1)));
      event->setProperty("NominalTemperature_Economy",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, _mode.OpMode2)));
      event->setProperty("NominalTemperature_NotUsed",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, _mode.OpMode3)));
      event->setProperty("NominalTemperature_Night",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, _mode.OpMode4)));
      event->setProperty("NominalTemperature_Holiday",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, _mode.OpMode5)));
    } else if (_properties.m_HeatingControlMode == HeatingControlModeIDFixed) {
      event->setProperty("ControlValue_Off",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, _mode.OpMode0)));
      event->setProperty("ControlValue_Comfort",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, _mode.OpMode1)));
      event->setProperty("ControlValue_Economy",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, _mode.OpMode2)));
      event->setProperty("ControlValue_NotUsed",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, _mode.OpMode3)));
      event->setProperty("ControlValue_Night",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, _mode.OpMode4)));
      event->setProperty("ControlValue_Holiday",
          doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, _mode.OpMode5)));
    }
    return event;
}

boost::shared_ptr<Event>
createHeatingControllerValueDsHub(int _zoneID, int _operationMode,
                                  const ZoneHeatingProperties_t &_props,
                                  const ZoneHeatingStatus_t &_stat)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::HeatingControllerValueDsHub);

  event->setProperty("ZoneID", intToString(_zoneID));
  switch (_operationMode) {
  case 0: event->setProperty("OperationMode", "Off"); break;
  case 1: event->setProperty("OperationMode", "Comfort"); break;
  case 2: event->setProperty("OperationMode", "Eco"); break;
  case 3: event->setProperty("OperationMode", "NotUsed"); break;
  case 4: event->setProperty("OperationMode", "Night"); break;
  case 5: event->setProperty("OperationMode", "Holiday"); break;
  }
  if (_props.m_HeatingControlMode == HeatingControlModeIDPID) {
    event->setProperty("NominalTemperature",
                       doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, _stat.m_NominalValue)));
  } else if (_props.m_HeatingControlMode == HeatingControlModeIDFixed) {
    event->setProperty("ControlValue",
                       doubleToString(SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, _stat.m_ControlValue)));
  }
  return event;
}

boost::shared_ptr<Event>
  createHeatingControllerState(int _zoneID, const dsuid_t &_ctrlDsuid, int _ctrlState)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::HeatingControllerState);

  event->setProperty("ZoneID", intToString(_zoneID));
  event->setProperty("ControlDSUID", dsuid2str(_ctrlDsuid));
  event->setProperty("ControlState", intToString(_ctrlState));
  return event;
}

boost::shared_ptr<Event>
  createOldStateChange(const std::string &_scriptId, const std::string &_name,
                       const std::string &_value, callOrigin_t _origin)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::OldStateChange);

  event->setProperty("scriptID", _scriptId);
  event->setProperty("statename", _name);
  event->setProperty("state", _value);
  event->setProperty("originDeviceID", intToString(static_cast<int>(_origin)));
  return event;
}

boost::shared_ptr<Event> createGenericSignalSunshine(const uint8_t &_value,
    const CardinalDirection_t &_direction, callOrigin_t _origin) {
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::Sunshine);

  // value: {active=1, inactive=2}
  assert(_value == 1 || _value == 2);
  assert(validOrigin(_origin));
  assert(valid(_direction));
  event->setProperty("value", intToString(_value));
  event->setProperty("direction", toString(_direction));
  event->setProperty("originDeviceID", intToString(static_cast<int>(_origin)));
  return event;
}

boost::shared_ptr<Event>
  createGenericSignalFrostProtection(const uint8_t &_value, callOrigin_t _origin) {
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::FrostProtection);

  // value: {active=1, inactive=2}
  assert(_value == 1 || _value == 2);
  assert(validOrigin(_origin));
  event->setProperty("value", intToString(_value));
  event->setProperty("originDeviceID", intToString(static_cast<int>(_origin)));
  return event;
}

boost::shared_ptr<Event>
  createGenericSignalHeatingModeSwitch(const uint8_t &_value, callOrigin_t _origin) {
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::HeatingModeSwitch);

  // value: {Off=0, Heat=1, Cold=2, Auto=3}
  assert(_value <= 3);
  assert(validOrigin(_origin));
  event->setProperty("value", intToString(_value));
  event->setProperty("originDeviceID", intToString(static_cast<int>(_origin)));
  return event;
}

boost::shared_ptr<Event>
  createGenericSignalBuildingService(const uint8_t &_value, callOrigin_t _origin) {
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::BuildingService);

  // value: {active=1, inactive=2}
  assert(_value == 1 || _value == 2);
  assert(validOrigin(_origin));
  event->setProperty("value", intToString(_value));
  event->setProperty("originDeviceID", intToString(static_cast<int>(_origin)));
  return event;
}

boost::shared_ptr<Event>
createOperationLockEvent(boost::shared_ptr<Group> _group, const int _zoneID, const int _groupID,
    const bool _lock, callOrigin_t _origin) {
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::OperationLock, _group);
  event->setProperty("zoneID", intToString(_zoneID));
  event->setProperty("groupID", intToString(_groupID));
  event->setProperty("lock", intToString(_lock));
  event->setProperty("originDeviceID", intToString(static_cast<int>(_origin)));
  return event;
}

boost::shared_ptr<Event>
createClusterConfigLockEvent(boost::shared_ptr<Group> _group, const int _zoneID, const int _groupID,
    const bool _lock, callOrigin_t _origin) {
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::ClusterConfigLock, _group);
  event->setProperty("zoneID", intToString(_zoneID));
  event->setProperty("groupID", intToString(_groupID));
  event->setProperty("lock", intToString(_lock));
  event->setProperty(ef_callOrigin, intToString(static_cast<int>(_origin)));
  return event;
}

}
