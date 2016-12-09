/*
    Copyright (c) 2009,2011 digitalSTROM.org, Zurich, Switzerland

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

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/make_shared.hpp>

#include "model/apartment.h"
#include "model/zone.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(zone)

DSUID_DEFINE(dsuid1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
DSUID_DEFINE(dsuid2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2);
DSUID_DEFINE(dsuid3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3);

BOOST_AUTO_TEST_CASE(testSensorSetReset) {
  Zone zone(2, NULL);
  MainZoneSensor_t ms1 = { dsuid1, SensorType::CO2Concentration, 1 };
  MainZoneSensor_t ms3 = { dsuid3, SensorType::HumidityIndoors, 3 };

  zone.setSensor(ms1);
  zone.setSensor(ms3);

  BOOST_CHECK(zone.isSensorAssigned(ms1.m_sensorType));
  BOOST_CHECK(zone.isSensorAssigned(ms3.m_sensorType));
  zone.resetSensor(ms1.m_sensorType);
  BOOST_CHECK(!zone.isSensorAssigned(ms1.m_sensorType));
  BOOST_CHECK(zone.isSensorAssigned(ms3.m_sensorType));
}

BOOST_AUTO_TEST_CASE(testRemovInvalidSensors) {
  // test: remove sensors that don't have the hosting device in the zone

  Apartment apt(NULL);

  Zone zone(7, &apt);
  auto dev1 = DeviceReference(apt.allocateDevice(dsuid1), &apt);
  zone.addDevice(dev1);
  auto dev3 = DeviceReference(apt.allocateDevice(dsuid3), &apt);
  zone.addDevice(dev3);

  MainZoneSensor_t ms1 = { dsuid1, SensorType::CO2Concentration, 1 };
  MainZoneSensor_t ms2 = { dsuid2, SensorType::HumidityIndoors, 2 };
  MainZoneSensor_t ms3 = { dsuid3, SensorType::TemperatureIndoors, 3 };

  zone.setSensor(ms1);
  zone.setSensor(ms2);
  zone.setSensor(ms3);

  // device with dsuid2 is not in zone
  BOOST_CHECK_EQUAL(zone.getAssignedSensors().size(), 3);
  BOOST_CHECK(zone.isSensorAssigned(SensorType::HumidityIndoors));
  zone.removeInvalidZoneSensors();
  BOOST_CHECK(zone.isSensorAssigned(SensorType::CO2Concentration));
  BOOST_CHECK(zone.isSensorAssigned(SensorType::TemperatureIndoors));
  BOOST_CHECK(!zone.isSensorAssigned(SensorType::HumidityIndoors));
}

BOOST_AUTO_TEST_SUITE_END()
