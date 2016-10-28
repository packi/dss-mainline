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
#include "src/vdc-db-fetcher.h"
#include "src/propertysystem.h"
#include "src/vdc-connection.h"
#include "src/web/webrequests.h"
#include "src/web/handler/devicerequesthandler.h"
#include "src/web/handler/vdc-info.h"
#include "tests/util/dss_instance_fixture.h"

using namespace dss;

// http://db.aizo.net/vdc-db.php

BOOST_AUTO_TEST_SUITE(VDC_DB)

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
  std::string gtin("7640156791914"); // VZug Steamer
  std::string no_gtin("invalid_gtin");

  VdcDbFetcher dbFetcher(*DSS::getInstance()); // recreate db
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
    out += "\tname: " + prop.name + ", title: " + prop.title + ", type: " + intToString(prop.typeId) + ", default: " + prop.defaultValue + "\n";
  }
  Logger::getInstance()->log("properties: \n" + out, lsWarning);
}

BOOST_FIXTURE_TEST_CASE(lookupProperties, DSSInstanceFixture) {
  std::string gtin("7640156791914"); // VZug Steamer

  VdcDbFetcher dbFetcher(*DSS::getInstance()); // recreate db
  VdcDb db;
  std::vector<VdcDb::PropertyDesc> props;
  BOOST_CHECK_NO_THROW(props = db.getProperties(gtin));
  BOOST_CHECK(props[2].name == "temperature.sensor");
  BOOST_CHECK(props[2].title == "temperature.sensor");
  //dumpProperties(props);

  BOOST_CHECK_NO_THROW(props = db.getProperties(gtin, "de_DE"));
  BOOST_CHECK(props[2].name == "temperature.sensor");
  BOOST_CHECK(props[2].title == "Garguttemperatur");
  dumpProperties(props);
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

BOOST_FIXTURE_TEST_CASE(lookupActions, DSSInstanceFixture) {
  std::string gtin("7640156791914"); // VZug Steamer

  VdcDbFetcher dbFetcher(*DSS::getInstance()); // recreate db
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
  BOOST_CHECK(actions[1].params[1].name == "temperature");
  BOOST_CHECK(actions[1].params[1].title == "Temperatur");
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
  std::string gtin("7640156791914"); // VZug Steamer

  VdcDbFetcher dbFetcher(*DSS::getInstance()); // recreate db
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
  VdcDbFetcher dbFetcher(*DSS::getInstance()); // recreate db
  VdcDb db;
  Device dev(DSUID_NULL, NULL);
  dev.setOemInfo(7640156791914, 0, 0, DEVICE_OEM_EAN_NO_INTERNET_ACCESS, 0);
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
  vdcInfo::addSpec(db, dev, "de_DE", json);
  vdcInfo::addStateDescriptions(db, dev, "de_DE", json);
  vdcInfo::addPropertyDescriptions(db, dev, "de_DE", json);
  vdcInfo::addActionDescriptions(db, dev, "de_DE", json);
  vdcInfo::addStandardActions(db, dev, "de_DE", json);
  std::string ret = json.successJSON();

  Logger::getInstance()->log("info: " + ret, lsWarning);

  std::string expect = R"expect({"result":{"spec":{"class":{"title":"Geräteklasse","tags":"invisible","value":"x-class"},"classVersion":{"title":"Geräteklassen version","tags":"invisible","value":"x-classVersion"},"dsDeviceGTIN":{"title":"dS Device GTIN","tags":"overview:2","value":""},"hardwareModelGuid":{"title":"Produkt Kennzeichnung","tags":"invisible","value":"x-hardwareModelGuid"},"model":{"title":"Modellbezeichnung","tags":"overview:3","value":"x-model"},"modelVersion":{"title":"Modellvariante","tags":"overview:4","value":"x-modelVersion"},"name":{"title":"Name","tags":"overview:1","value":""},"vendorId":{"title":"Hersteller Kennung","tags":"","value":"x-vendorId"},"vendorName":{"title":"Hersteller","tags":"overview:7","value":"x-vendorName"}},"stateDescriptions":{"fan":{"title":"Ventilator","tags":"","options":{"on":"an","off":"aus"}},"operationMode":{"title":"Betriebszustand","tags":"overview","options":{"heating":"heizt","steaming":"dampft","off":"ausgeschaltet"}},"timer":{"title":"Wecker","tags":"","options":{"inactive":"inaktiv","running":"läuft"}}},"propertyDescriptions":{"temperature":{"title":"Temperatur","tags":"","type":"numeric","min":"0","max":"250","resolution":"1","siunit":"celsius","default":"0"},"duration":{"title":"Endzeit","tags":"","type":"numeric","min":"0","max":"1800","resolution":"1","siunit":"second","default":"0"},"temperature.sensor":{"title":"Garguttemperatur","tags":"readonly","type":"numeric","min":"0","max":"250","resolution":"1","siunit":"celsius","default":"0"}},"actionDescriptions":{"bake":{"title":"Backen","params":{"duration":{"title":"Zeit","tags":"","type":"numeric","min":"60","max":"7200","resolution":"10","siunit":"second","default":"30"},"temperature":{"title":"Temperatur","tags":"","type":"numeric","min":"50","max":"240","resolution":"1","siunit":"celsius","default":"180"}}},"steam":{"title":"Dampfen","params":{"duration":{"title":"Zeit","tags":"","type":"numeric","min":"60","max":"7200","resolution":"10","siunit":"second","default":"30"},"temperature":{"title":"Temperatur","tags":"","type":"numeric","min":"50","max":"240","resolution":"1","siunit":"celsius","default":"180"}}},"stop":{"title":"Ausschalten","params":{}}},"standardActions":{"std.cake":{"title":"Kuchen","action":"bake","params":{"temperature":"160","duration":"3000"}},"std.pizza":{"title":"Pizza","action":"bake","params":{"temperature":"180","duration":"1200"}},"std.asparagus":{"title":"Spargel","action":"steam","params":{"temperature":"180","duration":"2520"}},"std.stop":{"title":"Stop","action":"stop","params":{}}}},"ok":true})expect";

  Logger::getInstance()->log("expect: " + expect, lsWarning);

  BOOST_CHECK(ret == expect);
}

BOOST_FIXTURE_TEST_CASE(checkNotFound, DSSInstanceFixture) {
  std::string gtin("0000000000000");
  // invalid gtin

  VdcDbFetcher dbFetcher(*DSS::getInstance()); // recreate db
  VdcDb db;
  BOOST_CHECK(db.getStates(gtin).empty());
  BOOST_CHECK(db.getProperties(gtin).empty());
  BOOST_CHECK(db.getActions(gtin).empty());
  BOOST_CHECK(db.getStandardActions(gtin, "de_DE").empty());

  Device dev(DSUID_NULL, NULL);
  dev.setOemInfo(strToInt(gtin), 0, 0, DEVICE_OEM_EAN_NO_INTERNET_ACCESS, 0);
  dev.setVdcSpec(VdsdSpec_t());

  JSONWriter json;
  vdcInfo::addSpec(db, dev, "de_DE", json);
  vdcInfo::addStateDescriptions(db, dev, "de_DE", json);
  vdcInfo::addPropertyDescriptions(db, dev, "de_DE", json);
  vdcInfo::addActionDescriptions(db, dev, "de_DE", json);
  vdcInfo::addStandardActions(db, dev, "de_DE", json);
  std::string ret = json.successJSON();

  std::string expect = R"expect({"result":{"spec":{"class":"","classVersion":"","dsDeviceGTIN":"0","model":"","modelVersion":"","hardwareGuid":"","hardwareModelGuid":"","vendorId":"","vendorName":""},"stateDescriptions":{},"propertyDescriptions":{},"actionDescriptions":{},"standardActions":{}},"ok":true})expect";

  // empty states/properties/actions
  BOOST_CHECK(ret == expect);
}

BOOST_FIXTURE_TEST_CASE(checkDeviceSupport, DSSInstanceFixture) {
  std::string gtins[] = {
    "7640156791914", // V-Zug MSLQ - aktiv
    "7640156791945" , // vDC smarter iKettle 2.0
  };

  VdcDbFetcher dbFetcher(*DSS::getInstance()); // recreate db
  VdcDb db;
  foreach (auto gtin, gtins) {
    BOOST_CHECK(!db.getStates(gtin, "de_DE").empty());
    BOOST_CHECK(!db.getProperties(gtin, "de_DE").empty());
    BOOST_CHECK(!db.getActions(gtin, "de_DE").empty());
    BOOST_CHECK(!db.getStandardActions(gtin, "de_DE").empty());
    BOOST_CHECK(!db.getEvents(gtin, "de_DE").empty());
  }
}

BOOST_AUTO_TEST_SUITE_END()
