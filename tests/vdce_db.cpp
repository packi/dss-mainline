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

#include <boost/algorithm/string/predicate.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <fstream>

#include "config.h"

#include "src/sqlite3_wrapper.h"
#include "src/foreach.h"
#include "src/model/device.h"
#include "src/model/vdce_db.h"
#include "src/propertysystem.h"
#include "src/web/handler/devicerequesthandler.h"
#include "tests/util/dss_instance_fixture.h"

using namespace dss;

// http://db.aizo.net/vdc-db.php

BOOST_AUTO_TEST_SUITE(VDCE)

static void dumpStates(std::vector<DeviceStateSpec_t> states) {
  foreach (const DeviceStateSpec_t &state, states) {
    std::string values;
    foreach (const std::string val, state.Values) {
      if (values.size() > 0) {
        values += ", ";
      }
      values += val;
    }
    Logger::getInstance()->log(state.Name + ": [" + values + "]", lsWarning);
  }
}

BOOST_FIXTURE_TEST_CASE(getStates, DSSInstanceFixture) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  propSystem.createProperty(pcn_vdce_db_name)->setStringValue("vdc.db");

  std::string gtin("7640113394226");
  std::string no_gtin("invalid_gtin");

  VdcDb db;

  std::vector<DeviceStateSpec_t> states_s;
  BOOST_CHECK_NO_THROW(states_s = db.getStatesLegacy(gtin));
  dumpStates(states_s);

  BOOST_CHECK_EQUAL(states_s[1].Name, "operationMode");
  BOOST_CHECK_EQUAL(states_s[1].Values.size(), 3);
  BOOST_CHECK_EQUAL(states_s[1].Values[1], "steaming");
  BOOST_CHECK_EQUAL(states_s[2].Name, "timer");
  BOOST_CHECK_EQUAL(states_s[2].Values[1], "running");

  std::vector<VdcDb::StateDesc> states_i;
  BOOST_CHECK_NO_THROW(states_i = db.getStates(gtin, ""));
  BOOST_CHECK_EQUAL(states_i[0].name, "fan");
  BOOST_CHECK_EQUAL(states_i[0].title, "fan");
  BOOST_CHECK_EQUAL(states_i[0].values.size(), 2);
  BOOST_CHECK_EQUAL(states_i[0].values[1].first, "off");
  BOOST_CHECK_EQUAL(states_i[0].values[1].second, "off");
  BOOST_CHECK_EQUAL(states_i[2].name, "timer");
  BOOST_CHECK_EQUAL(states_i[2].values[1].first, "running");
  BOOST_CHECK_EQUAL(states_i[2].values[1].second, "running");

  BOOST_CHECK_NO_THROW(states_i = db.getStates(gtin, "de_DE"));
  BOOST_CHECK_EQUAL(states_i[1].name, "operationMode");
  BOOST_CHECK_EQUAL(states_i[1].title, "Betriebszustand");
  BOOST_CHECK_EQUAL(states_i[1].values[1].first, "steaming");
  BOOST_CHECK_EQUAL(states_i[1].values[1].second, "dampft");
}

static void dumpProperties(const std::vector<VdcDb::PropertyDesc> &props) {
  std::string out;
  foreach (const VdcDb::PropertyDesc &prop, props) {
    out += "\tname: " + prop.name + ", title: " + prop.title + ", readonly: " + (prop.readonly ? "true" : "false") + "\n";
  }
  Logger::getInstance()->log("properties: \n" + out, lsWarning);
}

BOOST_FIXTURE_TEST_CASE(lookupProperties, DSSInstanceFixture) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  propSystem.createProperty(pcn_vdce_db_name)->setStringValue("vdc.db");

  std::string gtin("7640113394226");

  VdcDb db;
  std::vector<VdcDb::PropertyDesc> props;
  BOOST_CHECK_NO_THROW(props = db.getProperties(gtin));
  BOOST_CHECK(props[2].name == "temperature.sensor");
  BOOST_CHECK(props[2].title == "temperature.sensor");
  //dumpProperties(props);

  BOOST_CHECK_NO_THROW(props = db.getProperties(gtin, "de_DE"));
  BOOST_CHECK(props[2].name == "temperature.sensor");
  BOOST_CHECK(props[2].title == "Garguttemperatur");
  BOOST_CHECK(!props[2].readonly);
  dumpProperties(props);
}

static void dumpActionDesc(const std::vector<VdcDb::ActionDesc> &actions) {
  std::string out;
  foreach (const VdcDb::ActionDesc &action, actions) {
    out += "name: " + action.name + ", title: " + action.title + "\n";
    foreach (auto p, action.params) {
      out += "\t name: " + p.name + ", title: " + p.title + ", default: " + intToString(p.defaultValue) + "\n";
    }
  }
  Logger::getInstance()->log("actions: \n" + out, lsWarning);
}

BOOST_FIXTURE_TEST_CASE(lookupActions, DSSInstanceFixture) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  propSystem.createProperty(pcn_vdce_db_name)->setStringValue("vdc.db");

  std::string gtin("7640113394226");

  VdcDb db;
  std::vector<VdcDb::ActionDesc> actions;
  BOOST_CHECK_NO_THROW(actions = db.getActions(gtin, ""));
  BOOST_CHECK(actions[1].name == "steam");
  BOOST_CHECK(actions[1].title == "steam");
  BOOST_CHECK(actions[1].params[1].name == "duration");
  BOOST_CHECK(actions[1].params[1].title == "duration");
  //dumpActionDesc(actions);

  BOOST_CHECK_NO_THROW(actions = db.getActions(gtin, "de_DE"));
  BOOST_CHECK(actions[1].name == "steam");
  BOOST_CHECK(actions[1].title == "Dampfen");
  BOOST_CHECK(actions[1].params[1].name == "duration");
  BOOST_CHECK(actions[1].params[1].title == "Zeit");
  dumpActionDesc(actions);
}

static void dumpDesc(const std::vector<VdcDb::StandardActionDesc> &actions) {
  std::string out;
  foreach (auto action, actions) {
    out += "name: " + action.name + ", title: " + action.title + ", action_name: " + action.action_name + "\n";
    foreach (auto arg, action.args) {
        out += "\t " + arg.first + ": " + arg.second + "\n";
    }
  }
  Logger::getInstance()->log("standard actions: \n" + out, lsWarning);
}

BOOST_FIXTURE_TEST_CASE(lookupStandardActions, DSSInstanceFixture) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  propSystem.createProperty(pcn_vdce_db_name)->setStringValue("vdc.db");

  std::string gtin("7640113394226");

  VdcDb db;
  std::vector<VdcDb::StandardActionDesc> stdActions;
  BOOST_CHECK_NO_THROW(stdActions = db.getStandardActions(gtin, "de_DE"));
  //dumpDesc(stdActions);
  BOOST_CHECK(stdActions[1].name == "std.pizza");
  BOOST_CHECK(stdActions[1].title == "Pizza");
  BOOST_CHECK(stdActions[0].args[1].first == "duration");

  BOOST_CHECK_NO_THROW(stdActions = db.getStandardActions(gtin, ""));
  //dumpDesc(stdActions);
  BOOST_CHECK(stdActions[1].name == "std.pizza");
  BOOST_CHECK(stdActions[1].title == "std.pizza");
  BOOST_CHECK(stdActions[0].args[1].first == "duration");
}

BOOST_FIXTURE_TEST_CASE(getStaticInfo, DSSInstanceFixture) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  propSystem.createProperty(pcn_vdce_db_name)->setStringValue("vdc.db");

  Device dev(DSUID_NULL, NULL);
  dev.setOemInfo(7640113394226, 0, 0, DEVICE_OEM_EAN_NO_INTERNET_ACCESS, 0);

  DeviceRequestHandler handler(DSS::getInstance()->getApartment(), NULL, NULL);
  std::string ret = handler.getInfoStatic(dev, "de_DE");

  std::string expect = R"expect({"result":{"class":"oven (tbd)","classVersion":"1 (tbd)","oemEanNumber":"7640113394226","model":"Combi-Steam MSLQ (tbd)","modelVersion":"0.1 (tbd)","modelId":"gs1:(01)7640156791914 (tbd)","vendorId":"vendorname:V-Zug (tbd)","vendorName":"V-Zug (tbd)","stateDescriptions":{"fan":{"title":"Ventilator","options":{"on":"an","off":"aus"}},"operationMode":{"title":"Betriebszustand","options":{"heating":"heizt","steaming":"dampft","off":"ausgeschaltet"}},"timer":{"title":"Wecker","options":{"inactive":"inaktiv","running":"l&auml;ft"}}},"propertyDescriptions":{"temperature":{"title":"Temperatur","readOnly":false},"duration":{"title":"Endzeit","readOnly":false},"temperature.sensor":{"title":"Garguttemperatur","readOnly":false}},"actionDescriptions":{"bake":{"title":"Backen","params":{"temperature":{"title":"Temperatur","default":180},"duration":{"title":"Zeit","default":30}}},"steam":{"title":"Dampfen","params":{"temperature":{"title":"Temperatur","default":180},"duration":{"title":"Zeit","default":30}}}},"standardActions":{"std.cake":{"title":"Kuchen","params":{"temperature":"160","duration":"3000"}},"std.pizza":{"title":"Pizza","params":{"duration":"1200","temperature":"180"}},"std.asparagus":{"title":"Spargel","params":{"temperature":"180","duration":"2520"}}}},"ok":true})expect";
  //Logger::getInstance()->log("info: " + ret, lsWarning);
  //Logger::getInstance()->log("expect: " + expect, lsWarning);

  BOOST_CHECK(ret == expect);
}

BOOST_FIXTURE_TEST_CASE(checkNotFound, DSSInstanceFixture) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  propSystem.createProperty(pcn_vdce_db_name)->setStringValue("vdc.db");

  std::string gtin("0000000000000");
  // invalid gtin

  VdcDb db;
  BOOST_CHECK_THROW(db.getStates(gtin), std::exception);
  BOOST_CHECK_THROW(db.getProperties(gtin), std::exception);
  BOOST_CHECK_THROW(db.getActions(gtin), std::exception);
  BOOST_CHECK_THROW(db.getStandardActions(gtin, "de_DE"), std::exception);

  Device dev(DSUID_NULL, NULL);
  dev.setOemInfo(strToInt(gtin), 0, 0, DEVICE_OEM_EAN_NO_INTERNET_ACCESS, 0);

  DeviceRequestHandler handler(DSS::getInstance()->getApartment(), NULL, NULL);
  std::string ret = handler.getInfoStatic(dev, "de_DE");
  //Logger::getInstance()->log("info: " + ret, lsWarning);
  BOOST_CHECK(ret == R"expect({"ok":false,"message":"unknown device"})expect");
}

BOOST_AUTO_TEST_SUITE_END()
