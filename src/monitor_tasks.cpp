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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "logger.h"
#include "event.h"
#include "event/event_create.h"
#include "monitor_tasks.h"
#include "model/device.h"
#include "model/group.h"
#include "model/modulator.h"
#include "model/zone.h"
#include "model/set.h"
#include "model/state.h"
#include "model/modelconst.h"
#include "model/modelmaintenance.h"

namespace dss {

bool SensorMonitorTask::checkZoneValueDueTime(boost::shared_ptr<Group> _group, int _sensorType, DateTime _ts) {
  DateTime now;
  if (_ts != DateTime::NullDate) {
    int age = now.difference(_ts);
    return (age > SensorMaxLifeTime);
  }
  return false;
}

bool SensorMonitorTask::checkZoneValue(boost::shared_ptr<Group> _group, int _sensorType, DateTime _ts) {
  bool zoneTimeDue = checkZoneValueDueTime(_group, _sensorType, _ts);
  if (zoneTimeDue) {
    DateTime now;
    Logger::getInstance()->log(std::string("Sensor value (type: ") +
        intToString(_sensorType) + ") for zone #" +
        intToString(_group->getZoneID()) +
        " is too old: " + _ts.toISO8601_ms() +
        ", age in seconds is " + intToString(now.difference(_ts)), lsWarning);
    if (DSS::hasInstance()) {
      DSS::getInstance()->getEventQueue().
          pushEvent(createZoneSensorErrorEvent(_group, _sensorType, _ts));
    }
  }
  return zoneTimeDue;
}

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

        int age = 0;
        if (sensor->m_sensorValueTS == DateTime::NullDate) {
          if (DSS::hasInstance()) {
            age = DSS::getInstance()->getUptime();
          }
        } else {
          DateTime now = DateTime();
          age = now.difference(sensor->m_sensorValueTS);
        }

        Logger::getInstance()->log(std::string("Sensor #") +
                  intToString(s) + " of device " + dsuid2str(device->getDSID()) +
                  " value is from: " + sensor->m_sensorValueTS.toISO8601_ms() +
                  ", and age in seconds is " + intToString(age));

        // value is invalid because its older than its allowed lifetime
        if ((age > SensorMaxLifeTime) && device->isSensorDataValid(s)) {

          Logger::getInstance()->log(std::string("Sensor #") +
                    intToString(s) + " of device " + dsuid2str(device->getDSID()) +
                    " value is too old: " + sensor->m_sensorValueTS.toISO8601_ms() +
                    ", age in seconds is " + intToString(age), lsWarning);
          device->setSensorDataValidity(s, false);

          if (DSS::hasInstance()) {
            boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(device, m_Apartment);
            boost::shared_ptr<Event> pEvent =
                createDeviceInvalidSensorEvent(pDevRef, s, sensor->m_sensorType,
                                               sensor->m_sensorValueTS);
            DSS::getInstance()->getEventQueue().pushEvent(pEvent);
          }
        }
      }
    }
  } catch (...) {}

  try {
    std::vector<boost::shared_ptr<Zone> > zones = m_Apartment->getZones();
    for (std::vector<boost::shared_ptr<Zone> >::iterator it = zones.begin(); it != zones.end(); it ++) {
      boost::shared_ptr<Zone> pZone = *it;
      if (pZone->getID() > 0) {
        ZoneSensorStatus_t hSensors = pZone->getSensorStatus();
        if (checkZoneValue(pZone->getGroup(GroupIDBroadcast), SensorIDTemperatureIndoors, hSensors.m_TemperatureValueTS)) {
          pZone->setTemperature(hSensors.m_TemperatureValue, DateTime::NullDate);
        }
        if (checkZoneValue(pZone->getGroup(GroupIDBroadcast), SensorIDHumidityIndoors, hSensors.m_HumidityValueTS)) {
          pZone->setHumidityValue(hSensors.m_HumidityValue, DateTime::NullDate);
        }
        if (checkZoneValue(pZone->getGroup(GroupIDBroadcast), SensorIDTemperatureIndoors, hSensors.m_CO2ConcentrationValueTS)) {
          pZone->setCO2ConcentrationValue(hSensors.m_CO2ConcentrationValue, DateTime::NullDate);
        }
        if (checkZoneValue(pZone->getGroup(GroupIDBroadcast), SensorIDBrightnessIndoors, hSensors.m_BrightnessValueTS)) {
          pZone->setBrightnessValue(hSensors.m_BrightnessValue, DateTime::NullDate);
        }
        ZoneHeatingStatus_t hStatus = pZone->getHeatingStatus();
        if (checkZoneValue(pZone->getGroup(GroupIDControlTemperature), SensorIDRoomTemperatureControlVariable, hStatus.m_ControlValueTS)) {
          pZone->setControlValue(hStatus.m_ControlValue, DateTime::NullDate);
        }
      } else {
        ApartmentSensorStatus_t aSensors = m_Apartment->getSensorStatus();
        if (checkZoneValue(pZone->getGroup(GroupIDBroadcast), SensorIDTemperatureOutdoors, aSensors.m_TemperatureValueTS)) {
          pZone->setTemperature(aSensors.m_TemperatureValue, DateTime::NullDate);
        }
        if (checkZoneValue(pZone->getGroup(GroupIDBroadcast), SensorIDHumidityOutdoors, aSensors.m_HumidityValueTS)) {
          pZone->setHumidityValue(aSensors.m_HumidityValue, DateTime::NullDate);
        }
        if (checkZoneValue(pZone->getGroup(GroupIDBroadcast), SensorIDBrightnessOutdoors, aSensors.m_BrightnessValueTS)) {
          pZone->setBrightnessValue(aSensors.m_BrightnessValue,  DateTime::NullDate);
        }
      }
    }
  } catch (...) {}

  boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(EventName::CheckSensorValues);
  pEvent->setProperty("time", "+600");
  if (DSS::hasInstance()) {
    Logger::getInstance()->log("queued check_sensor_values event");
    DSS::getInstance()->getEventQueue().pushEvent(pEvent);
  }
}

void HeatingMonitorTask::syncZone(int _zoneID) {
  try {
    boost::shared_ptr<Zone> pZone = m_Apartment->getZone(_zoneID);
    boost::shared_ptr<Group> pGroup = pZone->getGroup(GroupIDControlTemperature);
    ZoneSensorStatus_t hSensors = pZone->getSensorStatus();
    ZoneHeatingProperties_t hConfig = pZone->getHeatingProperties();

    switch (hConfig.m_HeatingControlMode) {
      case HeatingControlModeIDPID:
        if (HeatingOperationModeInvalid != pZone->getHeatingOperationMode()) {
          pGroup->callScene(coSystem, SAC_MANUAL, pZone->getHeatingOperationMode(), "", false);
          usleep(1000 * 1000);
        }
        if (hSensors.m_TemperatureValueTS  != DateTime::NullDate) {
          pZone->pushSensor(coSystem, SAC_MANUAL, DSUID_NULL,
                            SensorIDTemperatureIndoors,
                            hSensors.m_TemperatureValue, "");
          usleep(1000 * 1000);
        }

        break;
      case HeatingControlModeIDFixed:
        if (HeatingOperationModeInvalid != pZone->getHeatingOperationMode()) {
          pGroup->callScene(coSystem, SAC_MANUAL, pZone->getHeatingOperationMode(), "", false);
          usleep(1000 * 1000);
        }
        break;
      case HeatingControlModeIDManual:
        if (HeatingOperationModeInvalid != pZone->getHeatingOperationMode()) {
          ZoneHeatingStatus_t hStatus = pZone->getHeatingStatus();
          if (hStatus.m_ControlValueTS != DateTime::NullDate) {
            pZone->pushSensor(coSystem, SAC_MANUAL, DSUID_NULL,
                              SensorIDRoomTemperatureControlVariable,
                              hStatus.m_ControlValue, "");
            usleep(1000 * 1000);
          }

        }
        break;
      case HeatingControlModeIDZoneFollower:
      case HeatingControlModeIDOff:
        break;
    }
  } catch (...) {
    Logger::getInstance()->log("HeatingMonitorTask: sync controller error", lsWarning);
  }
}

DateTime SensorMonitorTask::getDateTimeForSensor(const ZoneSensorStatus_t& _hSensors, const int _sensorType)
{
  switch (_sensorType) {
  case SensorIDTemperatureIndoors: {
    return _hSensors.m_TemperatureValueTS;
  }
  case SensorIDHumidityIndoors: {
    return _hSensors.m_HumidityValueTS;
  }
  case SensorIDBrightnessIndoors: {
    return _hSensors.m_BrightnessValueTS;
  }
  case SensorIDCO2Concentration: {
    return _hSensors.m_CO2ConcentrationValueTS;
  }
  }
  return DateTime::NullDate;
}

void SensorMonitorTask::checkZoneSensor(boost::shared_ptr<Zone> _zone, const int _sensorType, const ZoneSensorStatus_t& _hSensors) {

  boost::shared_ptr<Device> sensorDevice = _zone->getAssignedSensorDevice(_sensorType);

  bool sensorFault = (sensorDevice && !sensorDevice->isPresent());

  DateTime sensorTime = getDateTimeForSensor(_hSensors, _sensorType);
  if (checkZoneValueDueTime(_zone->getGroup(GroupIDBroadcast), _sensorType, sensorTime)) {
    sensorFault = true;
    switch (_sensorType) {
    case SensorIDTemperatureIndoors: {
      _zone->setTemperature(_hSensors.m_TemperatureValue, DateTime::NullDate);
      break;
    }
    case SensorIDHumidityIndoors: {
      _zone->setHumidityValue(_hSensors.m_HumidityValue, DateTime::NullDate);
      break;
    }
    case SensorIDBrightnessIndoors: {
      _zone->setBrightnessValue(_hSensors.m_BrightnessValue, DateTime::NullDate);
      break;
    }
    case SensorIDCO2Concentration: {
      _zone->setCO2ConcentrationValue(_hSensors.m_CO2ConcentrationValue, DateTime::NullDate);
      break;
    }
    }
  }

  if (sensorFault) {
    DateTime now;
    Logger::getInstance()->log(std::string("Sensor not available, or value (type: ") +
        intToString(_sensorType) + ") for zone #" +
        intToString(_zone->getID()) +
        " is too old: " + sensorTime.toISO8601_ms() +
        ", age in seconds is " + intToString(now.difference(sensorTime)), lsWarning);

    if (DSS::hasInstance()) {
      DSS::getInstance()->getEventQueue().
        pushEvent(createZoneSensorErrorEvent(_zone->getGroup(GroupIDBroadcast), _sensorType, DateTime::NullDate));
    }
  }
}

void HeatingMonitorTask::run() {

  if (m_event->getName() == EventName::ModelReady) {
    boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(EventName::CheckHeatingGroups);
    pEvent->setProperty("time", "+3600");
    if (DSS::hasInstance()) {
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
    return;
  }

  if (m_event->getName() == EventName::CheckHeatingGroups) {
    try {
      bool atHome;
      boost::shared_ptr<State> presence;
      try {
        presence = m_Apartment->getState(StateType_Service, "presence");
        atHome = presence->getState() == State_Active;
      } catch (ItemNotFoundException& e) {
        // default to false to be on the safe side
        atHome = false;
      }
      if (atHome) {
        // do nothing if at home
        Logger::getInstance()->log("HeatingMonitorTask: present - nothing to do", lsDebug);
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

    boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(EventName::CheckHeatingGroups);
    pEvent->setProperty("time", "+3600");
    if (DSS::hasInstance()) {
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
    return;
  }

  int zoneID;
  if (m_event->getName() == EventName::HeatingControllerState) {
    if (m_event->getPropertyByName("ControlState") == intToString(HeatingControlStateIDEmergency)) {
      zoneID = strToInt(m_event->getPropertyByName("zoneID"));
      dsuid_t dsmdsuid = str2dsuid(m_event->getPropertyByName("ControlDSUID"));

      Logger::getInstance()->log("HeatingMonitorTask: emergency state in zone " +
          intToString(zoneID) + " and controller " + dsuid2str(dsmdsuid),
          lsWarning);
      syncZone(zoneID);
    }
  } else if (m_event->getName() == EventName::DSMeterReady) {
    dsuid_t dsmdsuid = str2dsuid(m_event->getPropertyByName("dsMeter"));
    boost::shared_ptr<DSMeter> pMeter = m_Apartment->getDSMeterByDSID(dsmdsuid);
    Set devList = pMeter->getDevices();
    std::vector<boost::shared_ptr<Zone> > zones = m_Apartment->getZones();
    for (size_t i = 0; i < zones.size(); i++) {
      if (zones[i]->getID() == 0) {
        continue;
      }
      Set devices = devList.getByZone(zones[i]->getID());
      ZoneHeatingProperties_t hConfig = zones[i]->getHeatingProperties();
      if (!devices.isEmpty() || (hConfig.m_HeatingControlDSUID == dsmdsuid)) {
        syncZone(zones[i]->getID());
      }
    }
  }
}

size_t HeatingValveProtectionTask::m_zoneIndex = 0;

/**
 * Every Thursday around 15:00 send a HeatingOperationModeIDValveProtection scene call
 * to all rooms that contain devices in the temperature control group. The calls into
 * the zones are delayed by 15 minutes each.
 */
void HeatingValveProtectionTask::run() {

  if (m_event->getName() == EventName::ModelReady) {
    boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(EventName::HeatingValveProtection);
    // randomize the valve protection over two hours
    int randomizeStartMinutes = rand() % 120;
    DateTime start = DateTime::parseRFC2445("19700101T140000");
    start = start.addMinute(randomizeStartMinutes);
    pEvent->setProperty(EventProperty::ICalStartTime, start.toRFC2445IcalDataTime());
    pEvent->setProperty(EventProperty::ICalRRule, "FREQ=WEEKLY;BYDAY=TH");
    if (DSS::hasInstance()) {
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
    return;
  }

  if (m_event->getName() == EventName::HeatingValveProtection) {
    try {
      std::vector<boost::shared_ptr<Zone> > zones = m_Apartment->getZones();
      for ( ; m_zoneIndex < zones.size(); m_zoneIndex++) {
        if (zones[m_zoneIndex]->getID() == 0) {
          continue;
        }
        boost::shared_ptr<Group> tempControlGroup = zones[m_zoneIndex]->getGroup(GroupIDControlTemperature);
        Set devices = tempControlGroup->getDevices();
        if (devices.isEmpty()) {
          continue;
        }
        tempControlGroup->callScene(coSystem, SAC_MANUAL, HeatingOperationModeIDValveProtection, "", false);
        boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(EventName::HeatingValveProtection);
        pEvent->setProperty("time", "+900");
        if (DSS::hasInstance()) {
          DSS::getInstance()->getEventQueue().pushEvent(pEvent);
        }
        m_zoneIndex++;
        return;
      }
      m_zoneIndex = 0;

    } catch (...) {
      Logger::getInstance()->log("HeatingValveProtectionTask: error executing valve protection", lsWarning);
    }
  }
}

}// namespace
