/*
    Copyright (c) 2009, 2010 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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
#include "src/subscription.h"
#include "src/eventinterpreterplugins.h"
#include "src/internaleventrelaytarget.h"
#include "src/eventcollector.h"
#include "src/propertysystem.h"
#include "src/setbuilder.h"
#include "src/model/apartment.h"
#include "src/model/modelmaintenance.h"
#include "src/model/device.h"
#include "src/model/devicereference.h"
#include "src/model/zone.h"
#include "src/model/group.h"
#include "src/model/set.h"
#include "src/dss.h"
#include "dss_life_cycle.h"
#include "src/eventinterpretersystemplugins.h"
#include "src/security/security.h"
#include "src/security/user.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(Events)

class NonRunningFixture {
public:
  NonRunningFixture() {
    m_pEventInterpreter.reset(new EventInterpreter(NULL));
    m_pQueue.reset(new EventQueue(m_pEventInterpreter.get(), 2));
    m_pRunner.reset(new EventRunner(m_pEventInterpreter.get()));
    m_propSys.reset(new PropertySystem());
    m_pEventInterpreter->setEventQueue(m_pQueue.get());
    m_pEventInterpreter->setEventRunner(m_pRunner.get());
    m_pQueue->setEventRunner(m_pRunner.get());
    m_pRunner->setEventQueue(m_pQueue.get());
    m_pEventInterpreter->initialize();
  }

protected:
  boost::shared_ptr<EventQueue> m_pQueue;
  boost::shared_ptr<EventRunner> m_pRunner;
  boost::shared_ptr<EventInterpreter> m_pEventInterpreter;
  boost::shared_ptr<PropertySystem> m_propSys;
};

BOOST_FIXTURE_TEST_CASE(testSimpleEvent, NonRunningFixture) {
  boost::shared_ptr<Event> pEvent(new Event("event1"));

  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 1);

  pEvent.reset(new Event("event2"));

  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 2);
} // testSimpleEvent

BOOST_FIXTURE_TEST_CASE(testSubscription, NonRunningFixture) {
  EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(m_pEventInterpreter.get());
  m_pEventInterpreter->addPlugin(plugin);

  boost::shared_ptr<Event> pEvent(new Event("my_event"));

  boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
  opts->setParameter("event_name", "event1");
  opts->setParameter("test_override", "always testing");
  opts->setParameter("test2_default", "defaults to that");
  boost::shared_ptr<EventSubscription> subscription(new EventSubscription("my_event", "raise_event", *m_pEventInterpreter, opts));
  m_pEventInterpreter->subscribe(subscription);

  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();
  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 2);

  pEvent.reset(new Event("event2"));

  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 3);
} // testSubscription

BOOST_FIXTURE_TEST_CASE(testEmptySubscriptionXML, NonRunningFixture) {
  BOOST_CHECK_EQUAL(m_pEventInterpreter->getNumberOfSubscriptions(), 0);

  std::string fileName = getTempDir() + "/testsubscriptions_empty.xml";
  std::ofstream ofs(fileName.c_str());
  ofs << "<?xml version=\"1.0\"?>\n<subscriptions version=\"1\">\n</subscriptions>";
  ofs.close();

  boost::shared_ptr<SubscriptionParserProxy> subParser(new SubscriptionParserProxy(
      m_propSys->createProperty("/temp"),
      PropertyNodePtr() ));

  subParser->loadFromXML(fileName);
  m_pEventInterpreter->loadSubscriptionsFromProperty(m_propSys->getProperty("/temp"));

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getNumberOfSubscriptions(), 0);
  boost::filesystem::remove_all(fileName);
} // testEmptySubscriptionXML

BOOST_FIXTURE_TEST_CASE(testNonExistingXML, NonRunningFixture) {
  BOOST_CHECK_EQUAL(m_pEventInterpreter->getNumberOfSubscriptions(), 0);

  boost::shared_ptr<SubscriptionParserProxy> subParser(new SubscriptionParserProxy(
      m_propSys->createProperty("/temp"),
      PropertyNodePtr() ));

  try {
    subParser->loadFromXML("data/iwillnever_be_a_subscription.xml");
  } catch(std::runtime_error& e) {
  }
  m_pEventInterpreter->loadSubscriptionsFromProperty(m_propSys->getProperty("/temp"));

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getNumberOfSubscriptions(), 0);
} // testNonExistingXML

BOOST_FIXTURE_TEST_CASE(testSubscriptionXML, NonRunningFixture) {
  EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(m_pEventInterpreter.get());
  m_pEventInterpreter->addPlugin(plugin);

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getNumberOfSubscriptions(), 0);

  std::string fileName = getTempDir() + "/testsubscriptions.xml";
  std::ofstream ofs(fileName.c_str());
  ofs << "<?xml version=\"1.0\"?>\n"
         "<subscriptions version=\"1\">\n"
         "  <subscription event-name=\"event1\" handler-name=\"raise_event\">\n"
         "    <parameter>\n"
         "      <parameter name=\"event_name\">event2</parameter>\n"
         "      <parameter name=\"time\">+1</parameter>\n"
         "    </parameter>\n"
         "    <filter match=\"all\">\n"
         "      <property-filter type=\"exists\" property=\"local\" />\n"
         "    </filter>\n"
         "  </subscription>\n"
         "  <subscription event-name=\"event2\" handler-name=\"nonexisting\">\n"
         "    <filter match=\"all\">\n"
         "      <property-filter type=\"exists\" property=\"remote\" />\n"
         "    </filter>\n"
         "  </subscription>\n"
         "</subscriptions>\n";
  ofs.close();

  boost::shared_ptr<SubscriptionParserProxy> subParser(new SubscriptionParserProxy(
      m_propSys->createProperty("/usr/subs"),
      PropertyNodePtr()));

  subParser->loadFromXML(fileName);
  m_pEventInterpreter->loadSubscriptionsFromProperty(m_propSys->getProperty("/usr/subs"));

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getNumberOfSubscriptions(), 2);
  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 0);
  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 0);

  boost::shared_ptr<Event> evt(new Event("event1"));
  evt->setProperty("local", "true");
  m_pQueue->pushEvent(evt);
  m_pEventInterpreter->executePendingEvent();

  sleepMS(501);
  m_pRunner->raisePendingEvents();
  sleepMS(500);
  m_pRunner->raisePendingEvents();
  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 2);

  boost::shared_ptr<Event> evt2(new Event("event1"));
  evt2->setProperty("remote", "true");
  m_pQueue->pushEvent(evt2);
  m_pEventInterpreter->executePendingEvent();

  sleepMS(501);
  m_pRunner->raisePendingEvents();
  sleepMS(500);
  m_pRunner->raisePendingEvents();
  m_pEventInterpreter->executePendingEvent();

  // check that the filter works and event2 has not been raised
  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 3);

  boost::filesystem::remove_all(fileName);
} // testSubscriptionXML

BOOST_AUTO_TEST_CASE(testEventHandlerJavascriptDoesntLeakExceptionsWithNonexistingFile) {
  EventInterpreter interpreter(NULL);
  interpreter.initialize();
  EventInterpreterPluginJavascript* plugin = new EventInterpreterPluginJavascript(&interpreter);
  interpreter.addPlugin(plugin);

  boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
  opts->setParameter("filename1", "idontexistandneverwill.js");
  EventSubscription subscription("testEvent", "javascript", interpreter, opts);
  Event evt("testEvent");
  plugin->handleEvent(evt, subscription);
} // testEventHandlerJavascriptDoesntLeakExceptionsWithNonexistingFile

BOOST_FIXTURE_TEST_CASE(testRemovingSubscription, NonRunningFixture) {
  EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(m_pEventInterpreter.get());
  m_pEventInterpreter->addPlugin(plugin);

  boost::shared_ptr<Event> pEvent(new Event("my_event"));

  boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
  opts->setParameter("event_name", "event1");
  boost::shared_ptr<EventSubscription> subscription(new EventSubscription("my_event", "raise_event", *m_pEventInterpreter, opts));
  m_pEventInterpreter->subscribe(subscription);

  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();
  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 2);

  m_pEventInterpreter->unsubscribe(subscription->getID());

  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();
  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 3);
} // testRemovingSubscription

BOOST_AUTO_TEST_CASE(testUniqueEventsWork) {
  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter);
  interpreter.initialize();

  boost::shared_ptr<Event> pEvent(new Event("my_event"));

  queue.pushEvent(pEvent);

  boost::shared_ptr<Event> pEvent2(new Event("my_event"));
  pEvent2->setProperty("unique", "yes");

  queue.pushEvent(pEvent2);

  BOOST_CHECK(queue.popEvent() != NULL);
  BOOST_CHECK(queue.popEvent() == NULL);
} // testUniqueEventsWork

BOOST_AUTO_TEST_CASE(testUniqueEventsDontBreakRegularOnes) {
  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter);
  interpreter.initialize();

  boost::shared_ptr<Event> pEvent(new Event("my_event"));

  queue.pushEvent(pEvent);

  boost::shared_ptr<Event> pEvent2(new Event("my_other_event"));
  pEvent2->setProperty("unique", "yes");

  queue.pushEvent(pEvent2);

  BOOST_CHECK(queue.popEvent() != NULL);
  BOOST_CHECK(queue.popEvent() != NULL);
} // testUniqueEventsDontBreakRegularOnes

BOOST_AUTO_TEST_CASE(testUniqueEventsCopyProperties) {
  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter);
  interpreter.initialize();

  boost::shared_ptr<Event> pEvent(new Event("my_event"));

  boost::shared_ptr<Event> pEvent2(new Event("my_event"));
  pEvent2->setProperty("unique", "yes");
  pEvent2->setProperty("aProperty", "aValue");

  queue.pushEvent(pEvent2);

  boost::shared_ptr<Event> pEventFromQueue = queue.popEvent();
  BOOST_CHECK(pEventFromQueue != NULL);
  BOOST_CHECK_EQUAL(pEventFromQueue->hasPropertySet("aProperty"), true);
  BOOST_CHECK_EQUAL(pEventFromQueue->getPropertyByName("aProperty"), "aValue");
} // testUniqueEventsCopyProperties

BOOST_AUTO_TEST_CASE(testUniqueEventsOverwriteProperties) {
  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter);
  interpreter.initialize();

  boost::shared_ptr<Event> pEvent(new Event("my_event"));
  pEvent->setProperty("aProperty", "SomeValue");

  queue.pushEvent(pEvent);

  boost::shared_ptr<Event> pEvent2(new Event("my_event"));
  pEvent2->setProperty("unique", "yes");
  pEvent2->setProperty("aProperty", "aValue");

  queue.pushEvent(pEvent2);

  boost::shared_ptr<Event> pEventFromQueue = queue.popEvent();
  BOOST_CHECK(pEventFromQueue != NULL);
  BOOST_CHECK_EQUAL(pEventFromQueue->hasPropertySet("aProperty"), true);
  BOOST_CHECK_EQUAL(pEventFromQueue->getPropertyByName("aProperty"), "aValue");
} // testUniqueEventsOverwriteProperties

BOOST_AUTO_TEST_CASE(testUniqueEventsOverwritesTimeProperty) {
  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter);
  EventRunner runner(&interpreter);
  interpreter.initialize();
  queue.setEventRunner(&runner);

  boost::shared_ptr<Event> pEvent(new Event("my_event"));
  pEvent->setProperty("time", "+2");

  queue.pushEvent(pEvent);
  BOOST_CHECK_EQUAL(runner.getSize(), 1);

  boost::shared_ptr<Event> pEvent2(new Event("my_event"));
  pEvent2->setProperty("unique", "yes");
  pEvent2->setProperty("time", "+2");

  queue.pushEvent(pEvent2);

  std::vector<std::string> ids = runner.getEventIDs();
  BOOST_CHECK_EQUAL(runner.getSize(), 1);
  BOOST_CHECK_EQUAL(ids.size(), 1);

  const ScheduledEvent& eventFromQueue = runner.getEvent(ids.at(0));
  BOOST_CHECK_EQUAL(eventFromQueue.getEvent()->hasPropertySet("time"), true);
  BOOST_CHECK_EQUAL(eventFromQueue.getEvent()->getPropertyByName("time"), "+2");
} // testUniqueEventsOverwritesTimeProperty

BOOST_AUTO_TEST_CASE(testRemoveAndGetEvents) {
  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter);
  EventRunner runner(&interpreter);
  interpreter.initialize();
  queue.setEventRunner(&runner);

  boost::shared_ptr<Event> pEvent(new Event("my_event"));
  pEvent->setProperty("time", "+2");

  queue.pushEvent(pEvent);
  BOOST_CHECK_EQUAL(runner.getSize(), 1);

  boost::shared_ptr<Event> pEvent2(new Event("my_event"));
  pEvent2->setProperty("time", "+4");

  queue.pushEvent(pEvent2);

  std::vector<std::string> ids = runner.getEventIDs();
  BOOST_CHECK_EQUAL(runner.getSize(), 2);
  BOOST_CHECK_EQUAL(ids.size(), 2);

  const ScheduledEvent& eventFromQueue = runner.getEvent(ids.at(0));
  BOOST_CHECK_EQUAL(eventFromQueue.getID(), ids.at(0));
  BOOST_CHECK_EQUAL(eventFromQueue.getEvent()->hasPropertySet("time"), true);
  BOOST_CHECK_THROW(runner.getEvent("idontexist"), std::runtime_error);

  runner.removeEvent(ids.at(1));
  BOOST_CHECK_EQUAL(runner.getSize(), 1);
  runner.removeEvent(ids.at(0));
  BOOST_CHECK_EQUAL(runner.getSize(), 0);
} // testUniqueEventsOverwritesTimeProperty

BOOST_FIXTURE_TEST_CASE(testEventCollector, NonRunningFixture) {
  EventInterpreterInternalRelay* relay = new EventInterpreterInternalRelay(m_pEventInterpreter.get());
  m_pEventInterpreter->addPlugin(relay);

  EventCollector collector(*relay);
  std::string subscriptionID = collector.subscribeTo("my_event");

  boost::shared_ptr<Event> pEvent(new Event("my_event"));
  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 1);

  BOOST_CHECK_EQUAL(collector.hasEvent(), true);

  Event evt = collector.popEvent();
  BOOST_CHECK_EQUAL(evt.getName(), "my_event");

  pEvent.reset(new Event("event2"));
  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 2);

  BOOST_CHECK_EQUAL(collector.hasEvent(), false);

  collector.unsubscribeFrom(subscriptionID);

  pEvent.reset(new Event("my_event"));
  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 3);

  BOOST_CHECK_EQUAL(collector.hasEvent(), false);
} // testEventCollector

class InternalEventRelayTester {
public:
  InternalEventRelayTester()
  : m_Counter(0)
  {}

  void onEvent(Event& _event, const EventSubscription& _subscription) {
    if(_event.getName() == "one") {
      m_Counter++;
    } else if(_event.getName() == "two") {
      m_Counter += 2;
    } else {
      BOOST_CHECK(false);
    }
  }

  int getCounter() const { return m_Counter; }
private:
  int m_Counter;
}; // InternalEventRelayTester

BOOST_FIXTURE_TEST_CASE(testInternalEventRelay, NonRunningFixture) {
  EventInterpreterInternalRelay* relay = new EventInterpreterInternalRelay(m_pEventInterpreter.get());
  m_pEventInterpreter->addPlugin(relay);

  InternalEventRelayTarget target(*relay);
  boost::shared_ptr<EventSubscription> subscriptionOne(
    new dss::EventSubscription(
               "one",
               EventInterpreterInternalRelay::getPluginName(),
               *m_pEventInterpreter,
               boost::shared_ptr<SubscriptionOptions>())
  );
  target.subscribeTo(subscriptionOne);
  boost::shared_ptr<EventSubscription> subscriptionTwo(
    new dss::EventSubscription(
               "two",
               EventInterpreterInternalRelay::getPluginName(),
               *m_pEventInterpreter,
               boost::shared_ptr<SubscriptionOptions>())
  );
  target.subscribeTo(subscriptionTwo);

  InternalEventRelayTester tester;
  target.setCallback(boost::bind(&InternalEventRelayTester::onEvent, &tester, _1, _2));

  boost::shared_ptr<Event> pEvent(new Event("one"));
  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 1);
  BOOST_CHECK_EQUAL(tester.getCounter(), 1);

  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 2);
  BOOST_CHECK_EQUAL(tester.getCounter(), 2);

  pEvent.reset(new Event("two"));
  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 3);
  BOOST_CHECK_EQUAL(tester.getCounter(), 4);
}

class InternalEventRelayCollector {
public:
  InternalEventRelayCollector()
  {}

  void onEvent(Event& _event, const EventSubscription& _subscription) {
    m_pCoughtEvent.reset(new Event());
    *m_pCoughtEvent = _event;
  }

  boost::shared_ptr<Event> getCoughtEvent() { return m_pCoughtEvent; }
private:
  boost::shared_ptr<Event> m_pCoughtEvent;
}; // InternalEventRelayTester

class RelayEventFixture : public NonRunningFixture {
public:
  static const std::string kReraisedName;

  RelayEventFixture() {
    m_pRelay = new EventInterpreterInternalRelay(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(m_pRelay);
    EventInterpreterPluginRaiseEvent* plugin = new EventInterpreterPluginRaiseEvent(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    m_pTarget.reset( new InternalEventRelayTarget(*m_pRelay));
    m_pSubscription.reset(
      new dss::EventSubscription(
                kReraisedName,
                EventInterpreterInternalRelay::getPluginName(),
                *m_pEventInterpreter,
                boost::shared_ptr<SubscriptionOptions>())
    );

    m_pTarget->subscribeTo(m_pSubscription);

    m_pTarget->setCallback(boost::bind(&InternalEventRelayCollector::onEvent, &m_Tester, _1, _2));
  }
protected:
  EventInterpreterInternalRelay* m_pRelay;
  boost::shared_ptr<InternalEventRelayTarget> m_pTarget;
  boost::shared_ptr<EventSubscription> m_pSubscription;
  InternalEventRelayCollector m_Tester;
};

const std::string RelayEventFixture::kReraisedName = "reraised_event";

BOOST_FIXTURE_TEST_CASE(testOverrideOnlyOverrides, RelayEventFixture) {
  boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
  opts->setParameter("event_name", kReraisedName);
  opts->setParameter("test_override", "always testing");
  boost::shared_ptr<EventSubscription> subscription(new EventSubscription("my_event", "raise_event", *m_pEventInterpreter, opts));
  m_pEventInterpreter->subscribe(subscription);

  boost::shared_ptr<Event> pEvent(new Event("my_event"));
  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();
  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 2);
  BOOST_CHECK(m_Tester.getCoughtEvent()->getPropertyByName("test").empty());
}

BOOST_FIXTURE_TEST_CASE(testOverrideOverrides, RelayEventFixture) {
  boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
  opts->setParameter("event_name", kReraisedName);
  const std::string kTestValue = "always testing";
  opts->setParameter("test_override", kTestValue);
  boost::shared_ptr<EventSubscription> subscription(new EventSubscription("my_event", "raise_event", *m_pEventInterpreter, opts));
  m_pEventInterpreter->subscribe(subscription);

  boost::shared_ptr<Event> pEvent(new Event("my_event"));
  pEvent->setProperty("test", "bla");
  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();
  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 2);
  BOOST_CHECK_EQUAL(m_Tester.getCoughtEvent()->getPropertyByName("test"), kTestValue);
}

BOOST_FIXTURE_TEST_CASE(testDefaultDoesntOverride, RelayEventFixture) {
  boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
  opts->setParameter("event_name", kReraisedName);
  const std::string kTestValue = "always testing";
  opts->setParameter("test_default", kTestValue);
  boost::shared_ptr<EventSubscription> subscription(new EventSubscription("my_event", "raise_event", *m_pEventInterpreter, opts));
  m_pEventInterpreter->subscribe(subscription);

  boost::shared_ptr<Event> pEvent(new Event("my_event"));
  const std::string kOriginalValue = "bla";
  pEvent->setProperty("test", kOriginalValue);
  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();
  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 2);
  BOOST_CHECK_EQUAL(m_Tester.getCoughtEvent()->getPropertyByName("test"), kOriginalValue);
}

BOOST_FIXTURE_TEST_CASE(testDefaultSetsDefault, RelayEventFixture) {
  boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
  opts->setParameter("event_name", kReraisedName);
  const std::string kTestValue = "always testing";
  opts->setParameter("test_default", kTestValue);
  boost::shared_ptr<EventSubscription> subscription(new EventSubscription("my_event", "raise_event", *m_pEventInterpreter, opts));
  m_pEventInterpreter->subscribe(subscription);

  boost::shared_ptr<Event> pEvent(new Event("my_event"));
  const std::string kOriginalValue = "bla";
  m_pQueue->pushEvent(pEvent);

  m_pEventInterpreter->executePendingEvent();
  m_pEventInterpreter->executePendingEvent();

  BOOST_CHECK_EQUAL(m_pEventInterpreter->getEventsProcessed(), 2);
  BOOST_CHECK_EQUAL(m_Tester.getCoughtEvent()->getPropertyByName("test"), kTestValue);
}

// TODO: in case a script does not run through, the exception is caught in the
// event interpreter and our test can not evaluate the result; figure out a way
// to test
BOOST_AUTO_TEST_CASE(testMultipleScriptFiles) {
  std::string fileName1 = getTempDir() + "s1.js";
  std::string fileName2 = getTempDir() + "s2.js";

  std::ofstream ofs1(fileName1.c_str());
  ofs1 << "function niceprint(text) { print('[NICE]: ' + text); }\n";
  ofs1.close();

  std::ofstream ofs2(fileName2.c_str());
  ofs2 << "niceprint('kraaah!');\n";
  ofs2.close();

  EventInterpreter interpreter(NULL);
  interpreter.initialize();
  EventInterpreterPluginJavascript* plugin = new EventInterpreterPluginJavascript(&interpreter);
  interpreter.addPlugin(plugin);

  boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
  opts->setParameter("filename1", fileName1);
  opts->setParameter("filename2", fileName2);
  EventSubscription subscription("testEvent", "javascript", interpreter, opts);
  Event evt("testEvent");
  plugin->handleEvent(evt, subscription);

  boost::filesystem::remove(fileName1);
  boost::filesystem::remove(fileName2);
} // testMultipleScriptFiles


// TODO: we need assertions in javascript sometime to make this test useful
BOOST_AUTO_TEST_CASE(testEventSourceGetsPassed) {
  std::string fileName1 = getTempDir() + "testeventsourcegetspassed.js";

  std::ofstream ofs1(fileName1.c_str());
  ofs1 << "print('isZone:      ' + raisedEvent.source.isZone);\n";
  ofs1 << "print('isDevice:    ' + raisedEvent.source.isDevice);\n";
  ofs1 << "print('isApartment: ' + raisedEvent.source.isApartment);\n";
  ofs1.close();

  EventInterpreter interpreter(NULL);
  interpreter.initialize();
  EventInterpreterPluginJavascript* plugin = new EventInterpreterPluginJavascript(&interpreter);
  interpreter.addPlugin(plugin);

  boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
  opts->setParameter("filename1", fileName1);
  EventSubscription subscription("testEvent", "javascript", interpreter, opts);
  Event evt("testEvent");
  plugin->handleEvent(evt, subscription);

  boost::filesystem::remove(fileName1);
}


static std::string pathSystemUser = "/system/security/users/system";

#ifdef WITH_DSSTEST_FILES
const char* kDsstestsFilesDirectory = WITH_DSSTEST_FILES;
#else
const char* kDsstestsFilesDirectory = TEST_TRIGGERS_PATH "data";
#endif

BOOST_AUTO_TEST_CASE(testSystemTriggerSpeed) {
  PropertyParser parser;
  boost::scoped_ptr<DSSLifeCycle> guard;
  guard.reset(new DSSLifeCycle());
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
  pEvent.reset(new Event("callSceneBus", group));
  pEvent->setProperty("sceneID", intToString(11));
  pEvent->setProperty("groupID", intToString(1));
  pEvent->setProperty("zoneID", intToString(9492));
  pEvent->setProperty("token", "wuafhawepufhasuofhawuopfhweaopfuhaweufhaweufhaweufhesufhweaupfhawufhw");

  boost::shared_ptr<SystemTrigger> trigger(new SystemTrigger());

  BOOST_CHECK_EQUAL(true, trigger->setup(*pEvent.get()));

  for (int i = 0; i < 10; ++i) {
    trigger->run();
    BOOST_CHECK(DSS::getInstance()->getEventQueue().popEvent().get());
  }
} // testBoolProxy

BOOST_AUTO_TEST_SUITE_END()
