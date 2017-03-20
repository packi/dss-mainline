/*
 *  Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland
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

#include <cstdio>

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional/optional_io.hpp>
#include "foreach.h"

#include "src/model/apartment.h"
#include "src/model/group.h"
#include "src/model/state.h"
#include "src/model/zone.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(states)

BOOST_AUTO_TEST_CASE(test_setState)
{
  boost::shared_ptr<State> role = boost::make_shared<State>("role");

  State::ValueRange_t values;
  values.push_back("good");
  values.push_back("bad");
  values.push_back("ugly");
  role->setValueRange(values);

  foreach (std::string cur, values) {
    role->setState(coTest, cur);
    BOOST_CHECK_EQUAL(role->toString(), cur);
  }

  // tryValueFromName returns int value for existing string value
  BOOST_CHECK_EQUAL(role->tryValueFromName("good"), 0);

  // tryValueFromName returns none for non existing string value
  BOOST_CHECK_EQUAL(role->tryValueFromName("notAValue"), boost::none);

  // valueFromName returns int value for existing string value
  BOOST_CHECK_EQUAL(role->valueFromName("good"), 0);

  // valueFromName throw for non existing string value
  BOOST_CHECK_THROW(role->valueFromName("notAValue"), std::exception);

}

BOOST_AUTO_TEST_CASE(testCreateDestroyState) {
  Apartment apt(NULL);
  auto state = apt.allocateState(StateType_Apartment, "foo", "<test>");
  BOOST_CHECK_EQUAL(state, apt.getState(StateType_Apartment, "<test>", "foo"));
  apt.removeState(state);
  BOOST_CHECK_THROW(apt.getState(StateType_Apartment, "foo", "<test>"), ItemNotFoundException);
}

DSUID_DEFINE(dsuid1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);

BOOST_AUTO_TEST_CASE(createDeviceSensors)
{
  Apartment apt(NULL);
  auto dev = apt.allocateDevice(dsuid1);

  std::string activeCondition = "> ; 24.99";
  std::string inactiveCondition = "< ; 20.0";
  auto sensorType = SensorType::GustDirection;

  StateSensor state("opaqueString", "scriptId", dev, sensorType, activeCondition, inactiveCondition);
  state.newValue(coDsmApi, 30.0);
  BOOST_CHECK_EQUAL(state.getState(), State_Active);
  state.newValue(coDsmApi, 12.0);
  BOOST_CHECK_EQUAL(state.getState(), State_Inactive);
}

BOOST_AUTO_TEST_CASE(createGroupSensor)
{
  Apartment apt(NULL);
  auto zone1 = apt.allocateZone(1);

  std::string activeCondition = "> ; 24.99";
  std::string inactiveCondition = "< ; 20.0";
  auto sensorType = SensorType::GustDirection;

  StateSensor state("opaqueString", "scriptId", zone1->getGroup(1), sensorType, activeCondition, inactiveCondition);
  state.newValue(coDsmApi, 30.0);
  BOOST_CHECK_EQUAL(state.getState(), State_Active);
  state.newValue(coDsmApi, 12.0);
  BOOST_CHECK_EQUAL(state.getState(), State_Inactive);
}

BOOST_AUTO_TEST_CASE(updateDeviceSensorStates)
{
  Apartment apt(NULL);
  auto dev = apt.allocateDevice(dsuid1);

  std::string activeCondition = "> ; 24.99";
  std::string inactiveCondition = "< ; 20.0";
  auto sensorType = SensorType::GustDirection;

  auto state1 = boost::make_shared<StateSensor>("opaqueString", "scriptId", dev, sensorType, activeCondition, inactiveCondition);
  auto state2 = boost::make_shared<StateSensor>("_opaqueString2", "scriptId", dev, sensorType, activeCondition, inactiveCondition);
  auto state3 = boost::make_shared<StateSensor>("opaqueString", "scriptId", dev, SensorType::SoundPressureLevel , activeCondition, inactiveCondition);
  apt.allocateState(state1);
  apt.allocateState(state2);
  apt.allocateState(state3);

  BOOST_CHECK_EQUAL(state1->getState(), State_Inactive);
  BOOST_CHECK_EQUAL(state2->getState(), State_Inactive);
  BOOST_CHECK_EQUAL(state3->getState(), State_Inactive);
  apt.updateSensorStates(dsuid1, sensorType, 30.0, coDsmApi);
  // state2 is identical to state1 except the opaqueString, should be updated
  // state3 depends on different sensorType hence not modified
  BOOST_CHECK_EQUAL(state1->getState(), State_Active);
  BOOST_CHECK_EQUAL(state2->getState(), State_Active);
  BOOST_CHECK_EQUAL(state3->getState(), State_Inactive);
}

BOOST_AUTO_TEST_CASE(updateZoneSensorStates)
{
  Apartment apt(NULL);

  int zoneId = 1;
  int groupId = 2;
  auto sensorType = SensorType::GustDirection;
  std::string activeCondition = "> ; 24.99";
  std::string inactiveCondition = "< ; 20.0";

  auto group = apt.allocateZone(zoneId)->getGroup(groupId);
  auto state1 = boost::make_shared<StateSensor>("opaqueString", "scriptId", group, sensorType, activeCondition, inactiveCondition);
  auto state2 = boost::make_shared<StateSensor>("_opaqueString2", "scriptId", group, sensorType, activeCondition, inactiveCondition);
  auto state3 = boost::make_shared<StateSensor>("opaqueString", "scriptId", apt.allocateZone(zoneId + 1)->getGroup(groupId + 1), sensorType, activeCondition, inactiveCondition);
  apt.allocateState(state1);
  apt.allocateState(state2);
  apt.allocateState(state3);

  BOOST_CHECK_EQUAL(state1->getState(), State_Inactive);
  BOOST_CHECK_EQUAL(state2->getState(), State_Inactive);
  BOOST_CHECK_EQUAL(state3->getState(), State_Inactive);
  apt.updateSensorStates(zoneId, groupId, sensorType, 30.0, coDsmApi);
  BOOST_CHECK_EQUAL(state1->getState(), State_Active);
  BOOST_CHECK_EQUAL(state2->getState(), State_Active);
  BOOST_CHECK_EQUAL(state3->getState(), State_Inactive);
}

BOOST_AUTO_TEST_SUITE_END()
