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

#include "ds485types.h"
#include "model/scenehelper.h"
#include "model/state.h"

namespace dss {

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
  event->setProperty("sensorIndex", intToString(_index));
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
  event->setProperty("sensorIndex", intToString(_index));
  event->setProperty("sensorType", intToString(_type));
  event->setProperty("lastValueTS", _ts.toISO8601_ms());
  return event;
}

boost::shared_ptr<Event>
createZoneSensorValueEvent(boost::shared_ptr<Group> _group, int _type,
                           int _value, const std::string _sourceDevice)
{
  boost::shared_ptr<Event> event;
  // TODO ensure sensorType valid or implement fallback
  double floatValue = SceneHelper::sensorToFloat12(_type, _value);

  event = boost::make_shared<Event>(EventName::ZoneSensorValue, _group);
  event->setProperty("sensorType", intToString(_type));
  event->setProperty("sensorValue", intToString(_value));
  event->setProperty("sensorValueFloat", doubleToString(floatValue));
  event->setProperty("originDSID", _sourceDevice);
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
  event->setProperty("sceneID", intToString(_sceneID));
  event->setProperty("groupID", intToString(_groupID));
  event->setProperty("zoneID", intToString(_zoneID));
  event->setProperty("originDSUID", dsuid2str(_originDSUID));
  event->setProperty("callOrigin", intToString(_callOrigin));
  event->setProperty("originToken", _originToken);
  if (_forced) {
    event->setProperty("forced", "true");
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
  event->setProperty("sceneID", intToString(_sceneID));
  event->setProperty("groupID", intToString(_groupID));
  event->setProperty("zoneID", intToString(_zoneID));
  event->setProperty("originDSUID", dsuid2str(_originDSUID));
  event->setProperty("callOrigin", intToString(_callOrigin));
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

}
