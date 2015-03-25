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

namespace dss {

boost::shared_ptr<Event>
createDeviceStatusEvent(boost::shared_ptr<DeviceReference> pDevRev,
                        int statusIndex, int statusValue)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::DeviceStatus, pDevRev);
  event->setProperty("statusIndex", intToString(statusIndex));
  event->setProperty("statusValue", intToString(statusValue));
  return event;
}

boost::shared_ptr<Event>
createDeviceBinaryInputEvent(boost::shared_ptr<DeviceReference> pDevRev,
                             int inputIndex, int inputType, int inputState)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::DeviceBinaryInputEvent, pDevRev);
  event->setProperty("inputIndex", intToString(inputIndex));
  event->setProperty("inputType", intToString(inputType));
  event->setProperty("inputState", intToString(inputState));
  return event;
}

boost::shared_ptr<Event>
createDeviceSensorValueEvent(boost::shared_ptr<DeviceReference> pDevRev, int
                             sensorIndex, int sensorType, int sensorValue)
{
  boost::shared_ptr<Event> event;
  // TODO ensure sensorType valid or implement fallback
  double floatValue = SceneHelper::sensorToFloat12(sensorType, sensorValue);

  event = boost::make_shared<Event>(EventName::DeviceSensorValue, pDevRev);
  event->setProperty("sensorIndex", intToString(sensorIndex));
  event->setProperty("sensorType", intToString(sensorType));
  event->setProperty("sensorValue", intToString(sensorValue));
  event->setProperty("sensorValueFloat", doubleToString(floatValue));
  return event;
}

boost::shared_ptr<Event>
createDeviceInvalidSensorEvent(boost::shared_ptr<DeviceReference> pDevRev,
                               int sensorIndex, int sensorType,
                               const DateTime& timestamp)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::DeviceInvalidSensor, pDevRev);
  event->setProperty("sensorIndex", intToString(sensorIndex));
  event->setProperty("sensorType", intToString(sensorType));
  event->setProperty("lastValueTS", timestamp.toISO8601_ms());
  return event;
}

boost::shared_ptr<Event>
createZoneSensorValueEvent(boost::shared_ptr<Group> group, int sensorType,
                           int sensorValue, const std::string sourceDevice)
{
  boost::shared_ptr<Event> event;
  // TODO ensure sensorType valid or implement fallback
  double floatValue = SceneHelper::sensorToFloat12(sensorType, sensorValue);

  event = boost::make_shared<Event>(EventName::ZoneSensorValue, group);
  event->setProperty("sensorType", intToString(sensorType));
  event->setProperty("sensorValue", intToString(sensorValue));
  event->setProperty("sensorValueFloat", doubleToString(floatValue));
  event->setProperty("originDSID", sourceDevice);
  return event;
}

boost::shared_ptr<Event>
createZoneSensorErrorEvent(boost::shared_ptr<Group> group, int sensorType,
                      const DateTime& timestamp)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::ZoneSensorError, group);
  event->setProperty("sensorType", intToString(sensorType));
  event->setProperty("lastValueTS", timestamp.toISO8601_ms());
  return event;
}

boost::shared_ptr<Event>
createGroupCallSceneEvent(boost::shared_ptr<Group> group, int sceneID,
                          int groupID, int zoneID,
                          const callOrigin_t& callOrigin,
                          const dsuid_t& originDSUID,
                          const std::string& originToken,
                          bool forced)
{
  boost::shared_ptr<Event> event;
  event = boost::make_shared<Event>(EventName::CallScene, group);
  event->setProperty("sceneID", intToString(sceneID));
  event->setProperty("groupID", intToString(groupID));
  event->setProperty("zoneID", intToString(zoneID));
  event->setProperty("originDSUID", dsuid2str(originDSUID));
  event->setProperty("callOrigin", intToString(callOrigin));
  event->setProperty("originToken", originToken);
  if (forced) {
    event->setProperty("forced", "true");
  }
  return event;
}

}
