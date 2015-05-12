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


#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include <digitalSTROM/dsuid.h>
#include "src/ds485types.h"
#include "src/model/scenehelper.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(sensorConversion)

BOOST_AUTO_TEST_CASE(test_sensor_conversion)
{
#define SENSOR_MIN 0
#define SENSOR_MAX 0xfff
  BOOST_CHECK_NO_THROW(SceneHelper::sensorToSystem(SensorIDTemperatureIndoors, SceneHelper::sensorToFloat12(SensorIDTemperatureIndoors, SENSOR_MIN)));
  BOOST_CHECK_NO_THROW(SceneHelper::sensorToSystem(SensorIDTemperatureIndoors, SceneHelper::sensorToFloat12(SensorIDTemperatureIndoors, SENSOR_MAX)));

  BOOST_CHECK_THROW(SceneHelper::sensorToSystem(SensorIDTemperatureIndoors, (float)SceneHelper::sensorToFloat12(SensorIDTemperatureIndoors, SENSOR_MIN)), SensorOutOfRangeException);
  BOOST_CHECK_NO_THROW(SceneHelper::sensorToSystem(SensorIDTemperatureIndoors, (float)SceneHelper::sensorToFloat12(SensorIDTemperatureIndoors, SENSOR_MAX)));
}

BOOST_AUTO_TEST_SUITE_END()
