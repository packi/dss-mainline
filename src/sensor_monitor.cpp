/*
    Copyright (c) 2014 digitalSTROM AG, Zurich, Switzerland

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

#include "config.h"

#include "logger.h"
#include "sensor_monitor.h"
#include "model/device.h"
#include "model/modelmaintenance.h"
#include "event.h"

namespace dss {

void SensorMonitorTask::run() {
  try {
    std::vector<boost::shared_ptr<Device> > devices = m_Apartment->getDevicesVector();
    for (size_t i = 0; i < devices.size(); i++) {
      boost::shared_ptr<Device>& device = devices.at(i);
      if (!device) {
        continue;
      }

      if (!device->isValid() || !device->isPresent()) {
        continue;
      }

      uint8_t sensorCount = device->getSensorCount();
      for (uint8_t s = 0; s < sensorCount; s++) {
        const boost::shared_ptr<DeviceSensor_t> sensor = device->getSensor(s);
        // as specced in #6371
        if (sensor->m_sensorPollInterval == 0) {
          continue;
        }

        int lifetime = sensor->m_sensorPollInterval * 3;
        DateTime sensor_ts = sensor->m_sensorValueTS;
        sensor_ts.addSeconds(lifetime);
        DateTime now = DateTime();
        // value is invalid because its older than its allowed lifeime
        if (sensor_ts < now) {
          Logger::getInstance()->log(std::string("Sensor #") +
                    intToString(s) + " of device " +
                    dsuid2str(device->getDSID()) + " is invalid, firing event");
          device->setSensorDataValidity(s, false);

          if (DSS::hasInstance()) {
            boost::shared_ptr<Event> pEvent(new Event("invalidSensorData"));
            pEvent->setProperty("dsuid", dsuid2str(device->getDSID()));
            pEvent->setProperty("sensorIndex", intToString(s));
            pEvent->setProperty("sensorType",
                                intToString(sensor->m_sensorType));
            DSS::getInstance()->getEventQueue().pushEvent(pEvent);
          }
        }
      }
    }
  } catch (...) {}
  boost::shared_ptr<Event> pEvent(new Event("check_sensor_values"));
  pEvent->setProperty("time", "+600");
  if (DSS::hasInstance()) {
      Logger::getInstance()->log("queued check_sensor_values event");
    DSS::getInstance()->getEventQueue().pushEvent(pEvent);
  }
}

}// namespace
