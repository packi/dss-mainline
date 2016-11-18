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
#include "src/vdc-db.h"
#include "src/propertysystem.h"
#include "src/vdc-connection.h"
#include "src/web/webrequests.h"
#include "src/web/handler/devicerequesthandler.h"
#include "src/web/handler/vdc-info.h"
#include "tests/util/dss_instance_fixture.h"

using namespace dss;

// http://db.aizo.net/vdc-db.php

BOOST_AUTO_TEST_SUITE(VDC_DB)

namespace {
struct Recreate {
    Recreate() { VdcDb::recreate(*DSS::getInstance()); }
};

class Fixture : public DSSInstanceFixture {
public:
  Recreate recreate;
  VdcDb db;

  Fixture() : db(*DSS::getInstance()) {}
};
} // namespace

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

static const char *gtin = "1234567890123";

BOOST_FIXTURE_TEST_CASE(getStates, Fixture) {
  std::vector<DeviceStateSpec_t> states_s;
  states_s = db.getStatesLegacy(gtin);
  //dumpStates(states_s);

  BOOST_REQUIRE_EQUAL(states_s.size(), 1);
  BOOST_CHECK_EQUAL(states_s[0].Name, "dummyState");
  BOOST_REQUIRE_EQUAL(states_s[0].Values.size(), 4);
  BOOST_CHECK_EQUAL(states_s[0].Values[0], "d");

  std::vector<VdcDb::StateDesc> states_i;
  states_i = db.getStates(gtin, "base");
  BOOST_REQUIRE_EQUAL(states_i.size(), 1);
  BOOST_CHECK_EQUAL(states_i[0].name, "dummyState");
  BOOST_CHECK_EQUAL(states_i[0].title, "dummyState");
  BOOST_REQUIRE_EQUAL(states_i[0].values.size(), 4);
  BOOST_CHECK_EQUAL(states_i[0].values[0].first, "d");
  BOOST_CHECK_EQUAL(states_i[0].values[0].second, "d");
  BOOST_CHECK_EQUAL(states_i[0].values[1].first, "u");
  BOOST_CHECK_EQUAL(states_i[0].values[1].second, "u");
  BOOST_CHECK_EQUAL(states_i[0].values[2].first, "mm");
  BOOST_CHECK_EQUAL(states_i[0].values[2].second, "mm");
  BOOST_CHECK_EQUAL(states_i[0].values[3].first, "y");
  BOOST_CHECK_EQUAL(states_i[0].values[3].second, "y");

  states_i = db.getStates(gtin, "de_DE");
  BOOST_REQUIRE_EQUAL(states_i.size(), 1);
  BOOST_CHECK_EQUAL(states_i[0].name, "dummyState");
  BOOST_CHECK_EQUAL(states_i[0].title, "dummyState");
  BOOST_REQUIRE_EQUAL(states_i[0].values.size(), 4);
  BOOST_CHECK_EQUAL(states_i[0].values[0].first, "d");
  BOOST_CHECK_EQUAL(states_i[0].values[0].second, "d");
}

static void dumpProperties(const std::vector<VdcDb::PropertyDesc> &props) {
  std::string out;
  foreach (const VdcDb::PropertyDesc &prop, props) {
    out += "\tname: " + prop.name + ", title: " + prop.title + ", type: " + intToString(prop.typeId) + ", default: " + prop.defaultValue + "\n";
  }
  Logger::getInstance()->log("properties: \n" + out, lsWarning);
}

BOOST_FIXTURE_TEST_CASE(lookupProperties, Fixture) {
  std::vector<VdcDb::PropertyDesc> props;
  props = db.getProperties(gtin);
  BOOST_REQUIRE_EQUAL(props.size(), 1);
  BOOST_CHECK_EQUAL(props[0].name, "dummyProperty");
  BOOST_CHECK_EQUAL(props[0].title, "dummyProperty");
  //dumpProperties(props);

  props = db.getProperties(gtin, "de_DE");
  BOOST_REQUIRE_EQUAL(props.size(), 1);
  BOOST_CHECK_EQUAL(props[0].name, "dummyProperty");
  BOOST_CHECK_EQUAL(props[0].title, "dummyProperty");
  //dumpProperties(props);
}

static void dumpActionDesc(const std::vector<VdcDb::ActionDesc> &actions) {
  std::string out;
  foreach (const VdcDb::ActionDesc &action, actions) {
    out += "name: " + action.name + ", title: " + action.title + "\n";
    foreach (auto p, action.params) {
      out += "\t name: " + p.name + ", title: " + p.title + ", default: " + p.defaultValue + "\n";
    }
  }
  Logger::getInstance()->log("actions: \n" + out, lsWarning);
}

BOOST_FIXTURE_TEST_CASE(lookupActions, Fixture) {
  std::vector<VdcDb::ActionDesc> actions;
  actions = db.getActions(gtin, "");
  BOOST_REQUIRE_EQUAL(actions.size(), 2);
  BOOST_CHECK_EQUAL(actions[0].name, "dummyAction1");
  BOOST_CHECK_EQUAL(actions[0].title, "dummyAction1");
  BOOST_REQUIRE_EQUAL(actions[0].params.size(), 2);
  BOOST_CHECK_EQUAL(actions[0].params[0].name, "dummyActionParam1");
  BOOST_CHECK_EQUAL(actions[0].params[0].title, "dummyActionParam1");
  dumpActionDesc(actions);

  actions = db.getActions(gtin, "de_DE");
  BOOST_REQUIRE_EQUAL(actions.size(), 2);
  BOOST_CHECK_EQUAL(actions[0].name, "dummyAction1");
  BOOST_CHECK_EQUAL(actions[0].title, "dummyAction1");
  BOOST_REQUIRE_EQUAL(actions[0].params.size(), 2);
  BOOST_CHECK_EQUAL(actions[0].params[0].name, "dummyActionParam1");
  BOOST_CHECK_EQUAL(actions[0].params[0].title, "dummyActionParam1");
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

BOOST_FIXTURE_TEST_CASE(lookupStandardActions, Fixture) {
  std::vector<VdcDb::StandardActionDesc> stdActions;
  stdActions = db.getStandardActions(gtin, "de_DE");
  //dumpDesc(stdActions);
  BOOST_REQUIRE_EQUAL(stdActions.size(), 2);
  BOOST_CHECK_EQUAL(stdActions[0].name, "std.dummy");
  BOOST_CHECK_EQUAL(stdActions[0].title, "std.dummy");
  BOOST_CHECK_EQUAL(stdActions[1].name, "std.moreDummy");
  BOOST_CHECK_EQUAL(stdActions[1].title, "std.moreDummy");

  BOOST_CHECK_NO_THROW(stdActions = db.getStandardActions(gtin, ""));
  //dumpDesc(stdActions);
  BOOST_REQUIRE_EQUAL(stdActions.size(), 2);
  BOOST_CHECK_EQUAL(stdActions[0].name, "std.dummy");
  BOOST_CHECK_EQUAL(stdActions[0].title, "std.dummy");
  BOOST_CHECK_EQUAL(stdActions[1].name, "std.moreDummy");
  BOOST_CHECK_EQUAL(stdActions[1].title, "std.moreDummy");
}

BOOST_FIXTURE_TEST_CASE(getStaticInfo, Fixture) {
  Device dev(DSUID_NULL, NULL);
  dev.setOemInfo(1234567890123, 0, 0, DEVICE_OEM_EAN_NO_INTERNET_ACCESS, 0);
  VdsdSpec_t vdcSpec;
  vdcSpec.oemGuid = "x-oemGuid";
  vdcSpec.oemModelGuid = "x-oemModelGuid";
  vdcSpec.vendorGuid = "x-vendorGuid";
  vdcSpec.vendorId = "x-vendorId";
  vdcSpec.hardwareGuid = "x-hardwareGuid";
  vdcSpec.hardwareModelGuid = "x-hardwareModelGuid";
  vdcSpec.modelUID = "x-modelUID";
  vdcSpec.deviceClass = "x-class";
  vdcSpec.deviceClassVersion = "x-classVersion";
  vdcSpec.name = "x-name";
  vdcSpec.model = "x-model";
  vdcSpec.hardwareVersion = "x-hardwareVersion";
  vdcSpec.modelVersion = "x-modelVersion";
  vdcSpec.vendorName = "x-vendorName";
  dev.setVdcSpec(std::move(vdcSpec));

  JSONWriter json;
  vdcInfo::addSpec(db, dev, "", json);
  vdcInfo::addStateDescriptions(db, dev, "", json);
  vdcInfo::addPropertyDescriptions(db, dev, "", json);
  vdcInfo::addActionDescriptions(db, dev, "", json);
  vdcInfo::addStandardActions(db, dev, "", json);
  std::string ret = json.successJSON();

  //Logger::getInstance()->log("info: " + ret, lsWarning);
  std::string expect = R"expect({"result":{"spec":{"class":{"title":"class","tags":"invisible","value":"x-class"},"classVersion":{"title":"classVersion","tags":"invisible","value":"x-classVersion"},"dsDeviceGTIN":{"title":"dsDeviceGTIN","tags":"settings:5","value":""},"dummyNode":{"title":"dummyNode","tags":"overview","value":""},"hardwareGuid":{"title":"hardwareGuid","tags":"settings:4","value":"x-hardwareGuid"},"hardwareModelGuid":{"title":"hardwareModelGuid","tags":"invisible","value":"x-hardwareModelGuid"},"model":{"title":"model","tags":"overview:2;settings:2","value":"x-model"},"modelVersion":{"title":"modelVersion","tags":"invisible","value":"x-modelVersion"},"name":{"title":"name","tags":"overview:1;settings:1","value":""},"vendorId":{"title":"vendorId","tags":"invisible","value":"x-vendorId"},"vendorName":{"title":"vendorName","tags":"overview:3;settings:3","value":"x-vendorName"}},"stateDescriptions":{"dummyState":{"title":"dummyState","tags":"overview","options":{"d":"d","u":"u","mm":"mm","y":"y"}}},"propertyDescriptions":{"dummyProperty":{"title":"dummyProperty","tags":"","type":"string","default":""}},"actionDescriptions":{"dummyAction1":{"title":"dummyAction1","params":{"dummyActionParam1":{"title":"dummyActionParam1","tags":"","type":"string","default":""},"dummyActionParam2":{"title":"dummyActionParam2","tags":"","type":"numeric","min":"1","max":"12","resolution":"0,01","siunit":"liter","default":"1"}}},"dummyAction2":{"title":"dummyAction2","params":{}}},"standardActions":{"std.dummy":{"title":"std.dummy","action":"dummyAction1","params":{}},"std.moreDummy":{"title":"std.moreDummy","action":"dummyAction2","params":{}}}},"ok":true})expect";
  //Logger::getInstance()->log("expect: " + expect, lsWarning);
  BOOST_CHECK_EQUAL(ret, expect);
}

BOOST_FIXTURE_TEST_CASE(checkNotFound, Fixture) {
  auto invalidGtin = "0000000000000";

  BOOST_CHECK(db.getStates(invalidGtin).empty());
  BOOST_CHECK(db.getProperties(invalidGtin).empty());
  BOOST_CHECK(db.getActions(invalidGtin).empty());
  BOOST_CHECK(db.getEvents(invalidGtin).empty());
  BOOST_CHECK(db.getStandardActions(invalidGtin, "").empty());

  Device dev(DSUID_NULL, NULL);
  dev.setOemInfo(strToInt(invalidGtin), 0, 0, DEVICE_OEM_EAN_NO_INTERNET_ACCESS, 0);
  dev.setVdcSpec(VdsdSpec_t());

  JSONWriter json;
  vdcInfo::addStateDescriptions(db, dev, "de_DE", json);
  vdcInfo::addPropertyDescriptions(db, dev, "de_DE", json);
  vdcInfo::addActionDescriptions(db, dev, "de_DE", json);
  vdcInfo::addStandardActions(db, dev, "de_DE", json);
  vdcInfo::addEventDescriptions(db, dev, "de_DE", json);
  std::string ret = json.successJSON();

  //Logger::getInstance()->log("info: " + ret, lsWarning);
  std::string expect = R"expect({"result":{"stateDescriptions":{},"propertyDescriptions":{},"actionDescriptions":{},"standardActions":{},"eventDescriptions":{}},"ok":true})expect";
  //Logger::getInstance()->log("expect: " + expect, lsWarning);
  BOOST_CHECK_EQUAL(ret, expect);
}

BOOST_AUTO_TEST_SUITE_END()
