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

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include "config.h"
#include "src/event.h"
#include "src/handler/db_fetch.h"

#include "tests/util/dss_instance_fixture.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(DB_FETCHER)

BOOST_FIXTURE_TEST_CASE(fileFetch, DSSInstanceFixture) {
  EventQueue &queue(DSS::getInstance()->getEventQueue());

  DbFetch fetcher("db-fetcher", "file://" + TEST_STATIC_DATADIR + "/db-fetch-ok.sql",
                  "unique-id");

  fetcher.run();

  boost::shared_ptr<Event> evt(queue.popEvent());
  BOOST_CHECK(evt);
  BOOST_CHECK_EQUAL(evt->getPropertyByName("success"), "1");
  BOOST_CHECK_EQUAL(evt->getPropertyByName("id"), "unique-id");
  BOOST_CHECK(queue.popEvent() == NULL);

  fetcher = DbFetch("db-fetcher", "file://" + TEST_STATIC_DATADIR + "/db-fetch-empty.sql",
                    "unique-id");
  fetcher.run();

  evt = queue.popEvent();
  BOOST_CHECK(evt);
  BOOST_CHECK_EQUAL(evt->getPropertyByName("success"), "0");
  BOOST_CHECK(queue.popEvent() == NULL);

  fetcher = DbFetch("db-fetcher", "file://" + TEST_STATIC_DATADIR + "/db-fetch-throws.sql",
          "unique-id");
  fetcher.run();

  evt = queue.popEvent();
  BOOST_CHECK(evt);
  BOOST_CHECK_EQUAL(evt->getPropertyByName("success"), "0");
  BOOST_CHECK(queue.popEvent() == NULL);
}

std::string addServiceConfig(bool valid = true)
{
  PropertySystem &props(DSS::getInstance()->getPropertySystem());
  static int serial = 0;

  // all nodes under pcn_dbu_list are link(=path) to name/url of
  // a DB that should be periodically downloaded

  std::string name("service" + intToString(serial++));
  PropertyNodePtr ref = props.createProperty(pcn_dbu_list + "/" + name);

  std::string configPath("/config/" + name + "/db");
  ref->setStringValue(configPath);

  if (!valid) {
    // dangling ref path
    return configPath;
  }

  PropertyNodePtr service = props.createProperty(configPath);
  service->createProperty("name")->setStringValue(name);
  service->createProperty("url")->setStringValue("file://" + TEST_STATIC_DATADIR + "/db-fetch-ok.sql");

  return configPath;
}

BOOST_FIXTURE_TEST_CASE(updatePlugin, DSSInstanceFixture) {
  EventQueue &queue(DSS::getInstance()->getEventQueue());

  addServiceConfig(false); // first config is incomplete
  std::string configPath = addServiceConfig();

  DbUpdatePlugin plugin(&DSS::getInstance()->getEventInterpreter());

  // TODO locked until initialized, needed by EventSubscription
  // calls EventInterpreter::uniqueSubscriptionID
  DSS::getInstance()->getEventInterpreter().initialize();

  EventSubscription subs("", "", DSS::getInstance()->getEventInterpreter(),
                         boost::make_shared<SubscriptionOptions>());

  Event evt(EventName::CheckDssDbUpdate);  //< handleEvent takes non-const
  plugin.handleEvent(evt, subs);

  queue.waitForEvent();
  boost::shared_ptr<Event> pEvt(queue.popEvent());
  BOOST_CHECK(queue.popEvent() == NULL);

  BOOST_CHECK_EQUAL(pEvt->getName(), EventName::DatabaseImported);
  BOOST_CHECK_EQUAL(pEvt->getPropertyByName("id"), configPath);
}

BOOST_FIXTURE_TEST_CASE(updatePluginIteration, DSSInstanceFixture) {
  EventQueue &queue(DSS::getInstance()->getEventQueue());
  std::vector<std::string> configPaths;

  configPaths.push_back(addServiceConfig());
  configPaths.push_back(addServiceConfig(false));
  configPaths.push_back(addServiceConfig());

  // TODO locked until initialized, needed by EventSubscription
  // calls EventInterpreter::uniqueSubscriptionID
  DSS::getInstance()->getEventInterpreter().initialize();

  DbUpdatePlugin plugin(&DSS::getInstance()->getEventInterpreter());
  EventSubscription subs("", "", DSS::getInstance()->getEventInterpreter(),
                         boost::make_shared<SubscriptionOptions>());

  Event evt(EventName::CheckDssDbUpdate);  //< handleEvent takes non-const
  plugin.handleEvent(evt, subs);
  queue.waitForEvent();
  boost::shared_ptr<Event> pEvt(queue.popEvent());
  BOOST_CHECK_EQUAL(pEvt->getPropertyByName("id"), configPaths[0]);
  BOOST_CHECK(queue.popEvent() == NULL); // only event in queue

  // now feed the loop to poll the next db link
  plugin.handleEvent(*pEvt, subs);
  queue.waitForEvent();
  pEvt = queue.popEvent();
  BOOST_CHECK_EQUAL(pEvt->getPropertyByName("id"), configPaths[2]);
  BOOST_CHECK(queue.popEvent() == NULL); // only event in queue

  // and again, but loop terminates
  plugin.handleEvent(*pEvt, subs);

  //queue.waitForEvent(); adds 1s timeout, avoid
  //BOOST_CHECK(queue.popEvent() == NULL); // only event in queue

  // ...instead we start over with another loop
  plugin.handleEvent(evt, subs);
  queue.waitForEvent();
  pEvt = queue.popEvent();
  BOOST_CHECK_EQUAL(pEvt->getPropertyByName("id"), configPaths[0]);
  BOOST_CHECK(queue.popEvent() == NULL); // only event in queue
}

BOOST_AUTO_TEST_SUITE_END()
