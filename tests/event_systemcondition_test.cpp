/*
 * Copyright (c) 2016 digitalSTROM.org, Zurich, Switzerland
 *
 * This file is part of digitalSTROM Server.
 *
 * digitalSTROM Server is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * digitalSTROM Server is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <iostream>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "config.h"

#include "src/dss.h"
#include "src/event.h"
#include "src/propertysystem.h"
#include "src/property-parser.h"
#include "src/systemcondition.h"

#include "tests/util/dss_instance_fixture.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(systemcondition)

static void initPropertyTree(std::string xmldesc)
{
  PropertySystem &ps = DSS::getInstance()->getPropertySystem();

  std::string fileName = TEST_DYNAMIC_DATADIR + "/sample.xml";
  std::ofstream ofs(fileName);
  ofs << xmldesc;
  ofs.close();

  PropertyParser pp;
  pp.loadFromXML(fileName, ps.createProperty("/"));
  boost::filesystem::remove_all(fileName);
}


BOOST_FIXTURE_TEST_CASE(testSystemStatesNumeric, DSSInstanceFixture) {
    const char xml[] =
R"xml(<?xml version="1.0" encoding="utf-8"?>
<properties version="1">
  <property name="test">
    <property name="conditions">
      <property type="boolean" name="enabled">
        <value>true</value>
      </property>
      <property name="states">
        <property type="string" name="state0">
          <value>0</value>
        </property>
        <property type="string" name="state1">
          <value>1</value>
        </property>
      </property>
    </property>
  </property>
  <property name="usr">
    <property name="states">
      <property type="string" name="state0">
        <!-- mind the extra name/value properties -->
        <property type="string" name="name">
          <value>state0</value>
        </property>
        <property type="integer" name="value">
          <value>0</value>
        </property>
      </property>
      <property type="string" name="state1">
        <property type="string" name="name">
          <value>state1</value>
        </property>
        <property type="integer" name="value">
          <value>1</value>
        </property>
      </property>
    </property>
  </property>
</properties>)xml";

  initPropertyTree(xml);
  BOOST_CHECK(checkSystemCondition("/test"));

  // ensure condition fails
  PropertySystem &ps = DSS::getInstance()->getPropertySystem();
  ps.setIntValue("/usr/states/state1/value", 5);
  BOOST_CHECK(!checkSystemCondition("/test"));
}

BOOST_FIXTURE_TEST_CASE(testSystemStatesString, DSSInstanceFixture) {
    const char xml[] =
R"xml(<?xml version="1.0" encoding="utf-8"?>
<properties version="1">
  <property name="test">
    <property name="conditions">
      <property type="boolean" name="enabled">
        <value>true</value>
      </property>
      <property name="states">
        <property type="string" name="state0">
          <value>heating</value>
        </property>
      </property>
    </property>
  </property>
  <property name="usr">
    <property name="states">
      <property type="string" name="state0">
        <!-- mind the extra name/value properties -->
        <property type="string" name="name">
          <value>state0</value>
        </property>
        <property type="string" name="state">
          <value>heating</value>
        </property>
        <property type="integer" name="value">
          <value>1</value>
        </property>
      </property>
    </property>
  </property>
</properties>)xml";

  initPropertyTree(xml);
  BOOST_CHECK(checkSystemCondition("/test"));

  PropertySystem &ps = DSS::getInstance()->getPropertySystem();
  ps.setStringValue("/usr/states/state0/state", "cake");
  BOOST_CHECK(!checkSystemCondition("/test"));
}

BOOST_AUTO_TEST_SUITE_END()
