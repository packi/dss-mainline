/*
    Copyright (c) 2014 digitalSTROM AG, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>
            Michael Tross <michael.tross@digitalstrom.com>

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
#include "monitor_tasks.h"
#include "model/device.h"
#include "model/group.h"
#include "model/modulator.h"
#include "model/zone.h"
#include "model/set.h"
#include "model/state.h"
#include "model/modelconst.h"
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

void HeatingMonitorTask::syncZone(int _zoneID) {
  try {
    dsuid_t sourceDSID;
    boost::shared_ptr<Zone> pZone = m_Apartment->getZone(_zoneID);
    boost::shared_ptr<Group> pGroup = pZone->getGroup(GroupIDControlTemperature);
    ZoneHeatingStatus_t hStatus = pZone->getHeatingStatus();
    ZoneHeatingProperties_t hConfig = pZone->getHeatingProperties();

    SetNullDsuid(sourceDSID);
    switch (hConfig.m_HeatingControlMode) {
      case HeatingControlModeIDPID:
        pZone->pushSensor(coSystem, SAC_MANUAL, sourceDSID, SensorIDTemperatureIndoors,
            hStatus.m_TemperatureValue, "");
        usleep(1000 * 1000);
        pZone->pushSensor(coSystem, SAC_MANUAL, sourceDSID, SensorIDRoomTemperatureSetpoint,
            hStatus.m_NominalValue, "");
        usleep(1000 * 1000);
        pGroup->callScene(coSystem, SAC_MANUAL, pGroup->getLastCalledScene(), "", false);
        usleep(1000 * 1000);
        break;
      case HeatingControlModeIDFixed:
        pGroup->callScene(coSystem, SAC_MANUAL, pGroup->getLastCalledScene(), "", false);
        usleep(1000 * 1000);
        break;
      case HeatingControlModeIDManual:
        pZone->pushSensor(coSystem, SAC_MANUAL, sourceDSID, SensorIDRoomTemperatureControlVariable,
            hStatus.m_ControlValue, "");
        usleep(1000 * 1000);
        break;
      case HeatingControlModeIDZoneFollower:
      case HeatingControlModeIDOff:
        break;
    }
  } catch (...) {
    Logger::getInstance()->log("HeatingMonitorTask: sync controller error", lsWarning);
  }
}

void HeatingMonitorTask::run() {

  if (m_event->getName() == "model_ready") {
    boost::shared_ptr<Event> pEvent(new Event("check_heating_groups"));
    pEvent->setProperty("time", "+3600");
    if (DSS::hasInstance()) {
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
    return;
  }

  if (m_event->getName() == "check_heating_groups") {
    try {
      // do nothing if at home
      boost::shared_ptr<State> presence = m_Apartment->getState(StateType_Apartment, "present");
      if (presence && (presence->getState() == State_Active)) {
        Logger::getInstance()->log("HeatingMonitorTask: present - nothing to do", lsInfo);
      } else {
        // check all zones for device in heating group
        std::vector<boost::shared_ptr<Zone> > zones = m_Apartment->getZones();
        for (size_t i = 0; i < zones.size(); i++) {
          if (zones[i]->getID() == 0) {
            continue;
          }
          boost::shared_ptr<Group> group = zones[i]->getGroup(GroupIDHeating);
          Set devices = group->getDevices();
          if (devices.isEmpty()) {
            continue;
          }
          group->callScene(coSystem, SAC_MANUAL, group->getLastCalledScene(), "", false);
        }
      }
    } catch (...) {
      Logger::getInstance()->log("HeatingMonitorTask: check heating groups error", lsWarning);
    }

    boost::shared_ptr<Event> pEvent(new Event("check_heating_groups"));
    pEvent->setProperty("time", "+3600");
    if (DSS::hasInstance()) {
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
    return;
  }

  int zoneID;
  if (m_event->getName() == EventName::HeatingControllerState) {
    if (m_event->getPropertyByName("CtrlState") == intToString(HeatingControlStateIDEmergency)) {
      zoneID = strToInt(m_event->getPropertyByName("zoneID"));
      dsuid_t dsmdsuid = str2dsuid(m_event->getPropertyByName("CtrlDSUID"));

      Logger::getInstance()->log("HeatingMonitorTask: emergency state in zone " +
          intToString(zoneID) + " and controller " + dsuid2str(dsmdsuid),
          lsWarning);
      syncZone(zoneID);
    }
  } else if (m_event->getName() == "dsMeter_ready") {
    dsuid_t dsmdsuid = str2dsuid(m_event->getPropertyByName("dsMeter"));
    boost::shared_ptr<DSMeter> pMeter = m_Apartment->getDSMeterByDSID(dsmdsuid);
    Set devList = pMeter->getDevices();
    std::vector<boost::shared_ptr<Zone> > zones;
    for (size_t i = 0; i < zones.size(); i++) {
      if (zones[i]->getID() == 0) {
        continue;
      }
      Set devices = devList.getByZone(zones[i]->getID());
      if (devices.isEmpty()) {
        continue;
      }
      syncZone(zones[i]->getID());
    }
  }
}

}// namespace
