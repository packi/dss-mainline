/*
    Copyright (c) 2009, 2010 digitalSTROM.org, Zurich, Switzerland

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
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>

#include <iostream>

#include "config.h"

#include "src/event.h"
#include "src/event/event_create.h"
#include "src/event/event_fields.h"
#include "src/subscription.h"
#include "src/propertysystem.h"
#include "src/setbuilder.h"
#include "src/model/apartment.h"
#include "src/model/modelmaintenance.h"
#include "src/model/zone.h"
#include "src/model/group.h"
#include "src/model/set.h"
#include "src/handler/system_triggers.h"
#include "src/dss.h"
#include "src/eventinterpretersystemplugins.h"
#include "src/security/security.h"
#include "src/security/user.h"

#include "tests/util/dss_instance_fixture.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(Triggers)

static std::string pathSystemUser = "/system/security/users/system";

#ifdef WITH_DSSTEST_FILES
const char* kDsstestsFilesDirectory = WITH_DSSTEST_FILES;
#else
const char* kDsstestsFilesDirectory = TEST_TRIGGERS_PATH "data";
#endif

BOOST_FIXTURE_TEST_CASE(testSpeed, DSSInstanceFixture) {
  PropertyParser parser;
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
  PropertyNodePtr prop = propSystem.createProperty("/");
  BOOST_CHECK_EQUAL(true, parser.loadFromXML(std::string(kDsstestsFilesDirectory) + std::string("/triggers.xml"), prop));
  Security &security = DSS::getInstance()->getSecurity();

  {
    // set up systemUser
    //pSecurity.reset(new Security(propSystem.createProperty(pathSecurity)));
    PropertyNodePtr systemUserNode = propSystem.createProperty(pathSystemUser);
    security.addSystemRole(systemUserNode);

    /* this will enable loginAsSystemUser */
    security.setSystemUser(new User(systemUserNode));
  }

  DSS::getInstance()->getApartment().allocateZone(9492);
  boost::shared_ptr<Zone> zone = DSS::getInstance()->getApartment().getZone(9492);
  boost::shared_ptr<Group> group = zone->getGroup(1);
  boost::shared_ptr<Event> pEvent;
  pEvent.reset(new Event(EventName::CallSceneBus, group));
  pEvent->setProperty("sceneID", intToString(11));
  pEvent->setProperty("groupID", intToString(1));
  pEvent->setProperty("zoneID", intToString(9492));
  pEvent->setProperty("token", "wuafhawepufhasuofhawuopfhweaopfuhaweufhaweufhaweufhesufhweaupfhawufhw");

  boost::shared_ptr<SystemTrigger> trigger = boost::make_shared<SystemTrigger>();

  BOOST_CHECK_EQUAL(true, trigger->setup(*pEvent.get()));

  for (int i = 0; i < 10; ++i) {
    trigger->run();
    BOOST_CHECK(DSS::getInstance()->getEventQueue().popEvent().get());
  }
}


// taken from webservice_api_tests
boost::shared_ptr<Group> createGroup(int id) {
  Apartment &apartment(DSS::getInstance()->getApartment());
  boost::shared_ptr<Zone> zone = apartment.getZone(0);
  return zone->getGroup(id);
}

BOOST_FIXTURE_TEST_CASE(testRateLimit, DSSInstanceFixture) {
  PropertyParser parser;
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();

  parser.loadFromXML(kDsstestsFilesDirectory + std::string("/rate_triggers.xml"),
                     propSystem.createProperty("/"));

  boost::shared_ptr<Event> pEvent;
  pEvent = createGroupCallSceneEvent(createGroup(1), 1, 0, 1,
                                     callOrigin_t(2), dsuid_t(),
                                     "fake-token", false);

  SystemTrigger trigger;
  BOOST_CHECK(trigger.setup(*pEvent));

  trigger.run();

  // only one event in queue
  EventQueue &queue(DSS::getInstance()->getEventQueue());
  BOOST_CHECK(queue.popEvent() != NULL);

  trigger.run(); // should be damped
  BOOST_CHECK(queue.popEvent() == NULL);
}

BOOST_FIXTURE_TEST_CASE(testRateLimitRewind, DSSInstanceFixture) {
  PropertyParser parser;
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();

  parser.loadFromXML(kDsstestsFilesDirectory + std::string("/rate_triggers.xml"),
                     propSystem.createProperty("/"));

  // same as referenced by /usr/triggers/0/triggerPath
  std::string triggerPath("/scripts/foo/entries/0/");
  PropertyNodePtr dampNode =
    propSystem.getProperty(triggerPath + pn_triggers + "/" + pn_damping);
  BOOST_CHECK(dampNode);

  DateTime fakeTS = DateTime().addSeconds(-dampNode->getProperty(pn_delay)->getIntegerValue() / 2);
  dampNode->createProperty(pn_last_matched)->setStringValue(fakeTS.toISO8601());

  boost::shared_ptr<Event> pEvent;
  pEvent = createGroupCallSceneEvent(createGroup(1), 1, 0, 1,
                                     callOrigin_t(2), dsuid_t(),
                                     "fake-token", false);

  SystemTrigger trigger;
  trigger.setup(*pEvent);
  trigger.run();

  DateTime afterTS = DateTime::parseISO8601(dampNode->getProperty(pn_last_matched)->getStringValue());
  BOOST_CHECK(fakeTS == afterTS);

  // now rewind the damping timer with each event
  dampNode->getProperty(pn_rewind_timer)->setBooleanValue(true);
  trigger.run();

  afterTS = DateTime::parseISO8601(dampNode->getProperty(pn_last_matched)->getStringValue());

  BOOST_CHECK(fakeTS != afterTS);
}

BOOST_AUTO_TEST_SUITE_END()
