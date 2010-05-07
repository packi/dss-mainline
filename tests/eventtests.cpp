/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#include "core/event.h"
#include "core/eventinterpreterplugins.h"
#include "core/internaleventrelaytarget.h"
#include "core/eventcollector.h"
#include "core/setbuilder.h"
#include "core/sim/dssim.h"
#include "core/model/apartment.h"
#include "core/model/modelmaintenance.h"
#include "core/model/device.h"
#include "core/model/devicereference.h"
#include "core/model/zone.h"
#include "core/model/group.h"
#include "core/model/set.h"
#include "core/ds485/ds485proxy.h"
#include "core/ds485/ds485busrequestdispatcher.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(Events)

BOOST_AUTO_TEST_CASE(testSimpleEvent) {
  EventQueue queue;
  EventRunner runner;
  EventInterpreter interpreter(NULL);
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  interpreter.run();

  boost::shared_ptr<Event> pEvent(new Event("event1"));

  queue.pushEvent(pEvent);

  sleep(1);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 1);

  pEvent.reset(new Event("event2"));

  queue.pushEvent(pEvent);

  sleep(1);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 2);

  queue.shutdown();
  interpreter.terminate();
  sleep(2);
} // testSimpleEvent

BOOST_AUTO_TEST_CASE(testSubscription) {
  EventQueue queue;
  EventRunner runner;
  EventInterpreter interpreter(NULL);
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
  interpreter.addPlugin(plugin);

  interpreter.run();

  boost::shared_ptr<Event> pEvent(new Event("my_event"));

  boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
  opts->setParameter("event_name", "event1");
  opts->setParameter("test_override", "always testing");
  opts->setParameter("test2_default", "defaults to that");
  boost::shared_ptr<EventSubscription> subscription(new EventSubscription("my_event", "raise_event", interpreter, opts));
  interpreter.subscribe(subscription);

  queue.pushEvent(pEvent);

  sleep(1);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 2);

  pEvent.reset(new Event("event2"));

  queue.pushEvent(pEvent);

  sleep(1);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 3);

  queue.shutdown();
  interpreter.terminate();
  sleep(2);
} // testSubscription

BOOST_AUTO_TEST_CASE(testEmptySubscriptionXML) {
  EventQueue queue;
  EventInterpreter interpreter(NULL);
  interpreter.setEventQueue(&queue);
  interpreter.initialize();

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

  std::string fileName = getTempDir() + "/testsubscriptions_empty.xml";
  std::ofstream ofs(fileName.c_str());
  ofs << "<?xml version=\"1.0\"?>\n<subscriptions version=\"1\">\n</subscriptions>";
  ofs.close();

  interpreter.loadFromXML(fileName);

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);
  boost::filesystem::remove_all(fileName);
} // testEmptySubscriptionXML

BOOST_AUTO_TEST_CASE(testNonExistingXML) {
  EventInterpreter interpreter(NULL);
  interpreter.initialize();

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

  try {
    interpreter.loadFromXML("data/iwillnever_be_a_subscription.xml");
  } catch(std::runtime_error& e) {
  }

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);
} // testNonExistingXML

BOOST_AUTO_TEST_CASE(testSubscriptionXML) {
  EventQueue queue;
  EventRunner runner;
  EventInterpreter interpreter(NULL);
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  queue.setEventRunner(&runner);
  runner.setEventQueue(&queue);
  EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
  interpreter.addPlugin(plugin);

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

  std::string fileName = getTempDir() + "/testsubscriptions.xml";
  std::ofstream ofs(fileName.c_str());
  ofs << "<?xml version=\"1.0\"?>\n"
         "<subscriptions version=\"1\">\n"
         "  <subscription event-name=\"event1\" handler-name=\"raise_event\">\n"
         "    <parameter>\n"
         "      <parameter name=\"event_name\">event2</parameter>\n"
         "      <parameter name=\"time\">+2</parameter>\n"
         "    </parameter>\n"
         "  </subscription>\n"
         "  <subscription event-name=\"event2\" handler-name=\"nonexisting\">\n"
         "    <filter match=\"all\">\n"
         "      <property-filter type=\"exists\" property=\"remote\" />\n"
         "    </filter>\n"
         "  </subscription>\n"
         "</subscriptions>\n";
  ofs.close();

  interpreter.loadFromXML(fileName);

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 2);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 0);

  interpreter.run();

  sleep(1);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 0);

  boost::shared_ptr<Event> evt(new Event("event1"));
  queue.pushEvent(evt);

  sleep(1);

  runner.runOnce();

  sleep(3);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 2);

  queue.shutdown();
  interpreter.terminate();
  sleep(1);
  boost::filesystem::remove_all(fileName);
} // testSubscriptionXML

BOOST_AUTO_TEST_CASE(testDS485Events) {
  EventQueue queue;
  EventRunner runner;
  EventInterpreter interpreter(NULL);
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  queue.setEventRunner(&runner);
  runner.setEventQueue(&queue);

  ModelMaintenance maintenance(NULL);
  Apartment apt(NULL);
  maintenance.setApartment(&apt);
  DSDSMeterSim modSim(NULL);
  DS485Proxy proxy(NULL, &maintenance);
  apt.setDS485Interface(&proxy);
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setFrameSender(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);


  proxy.initialize();

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setName("dev1");
  dev1.setShortAddress(1);
  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setName("dev2");
  dev2.setShortAddress(2);
  Device& dev3 = apt.allocateDevice(dsid_t(0,3));
  dev3.setName("dev3");
  dev3.setShortAddress(3);
  Device& dev4 = apt.allocateDevice(dsid_t(0,4));
  dev4.setName("dev4");
  dev4.setShortAddress(4);


  EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
  interpreter.addPlugin(plugin);
  plugin = new EventInterpreterPluginDS485(apt, &proxy, &interpreter);
  interpreter.addPlugin(plugin);

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

  std::string fileName = getTempDir() + "/testsubscriptions_DS485.xml";
  std::ofstream ofs(fileName.c_str());
  ofs << "<?xml version=\"1.0\"?>\n"
         "<subscriptions version=\"1\">\n"
         "  <subscription event-name=\"event1\" handler-name=\"raise_event\">\n"
         "    <parameter>\n"
         "      <parameter name=\"event_name\">event2</parameter>\n"
         "      <parameter name=\"time\">+10</parameter>\n"
         "    </parameter>\n"
         "  </subscription>\n"
         "</subscriptions>\n";
  ofs.close();

  interpreter.loadFromXML(fileName);

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 1);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 0);

  interpreter.run();

  sleep(1);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 0);

  boost::shared_ptr<Event> evt(new Event("brighter", &apt.getZone(0)));
  evt->setLocation("dev1");
  queue.pushEvent(evt);

  sleep(1);

  runner.runOnce();

  sleep(1);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 1);

  queue.shutdown();
  interpreter.terminate();
  sleep(1);
  boost::filesystem::remove_all(fileName);
} // testDS485Events

BOOST_AUTO_TEST_CASE(testEventHandlerJavascriptDoesntLeakExceptionsWithNonexistingFile) {
  EventInterpreter interpreter(NULL);
  EventInterpreterPluginJavascript* plugin = new EventInterpreterPluginJavascript(&interpreter);
  interpreter.addPlugin(plugin);

  boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
  opts->setParameter("filename", "idontexistandneverwill.js");
  EventSubscription subscription("testEvent", "javascript", interpreter, opts);
  Event evt("testEvent");
  plugin->handleEvent(evt, subscription);
} // testEventHandlerJavascriptDoesntLeakExceptionsWithNonexistingFile

BOOST_AUTO_TEST_CASE(testRemovingSubscription) {
  EventQueue queue;
  EventRunner runner;
  EventInterpreter interpreter(NULL);
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
  interpreter.addPlugin(plugin);

  interpreter.run();

  boost::shared_ptr<Event> pEvent(new Event("my_event"));

  boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
  opts->setParameter("event_name", "event1");
  boost::shared_ptr<EventSubscription> subscription(new EventSubscription("my_event", "raise_event", interpreter, opts));
  interpreter.subscribe(subscription);

  queue.pushEvent(pEvent);

  sleepMS(100);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 2);

  interpreter.unsubscribe(subscription->getID());

  queue.pushEvent(pEvent);

  sleepMS(100);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 3);

  queue.shutdown();
  interpreter.terminate();
  sleepMS(1200);
} // testRemovingSubscription

BOOST_AUTO_TEST_CASE(testUniqueEventsWork) {
  EventQueue queue;

  boost::shared_ptr<Event> pEvent(new Event("my_event"));

  queue.pushEvent(pEvent);

  boost::shared_ptr<Event> pEvent2(new Event("my_event"));
  pEvent2->setProperty("unique", "yes");

  queue.pushEvent(pEvent2);

  BOOST_CHECK(queue.popEvent() != NULL);
  BOOST_CHECK(queue.popEvent() == NULL);
} // testUniqueEventsWork

BOOST_AUTO_TEST_CASE(testUniqueEventsDontBreakRegularOnes) {
  EventQueue queue;

  boost::shared_ptr<Event> pEvent(new Event("my_event"));

  queue.pushEvent(pEvent);

  boost::shared_ptr<Event> pEvent2(new Event("my_other_event"));
  pEvent2->setProperty("unique", "yes");

  queue.pushEvent(pEvent2);

  BOOST_CHECK(queue.popEvent() != NULL);
  BOOST_CHECK(queue.popEvent() != NULL);
} // testUniqueEventsDontBreakRegularOnes

BOOST_AUTO_TEST_CASE(testUniqueEventsCopyProperties) {
  EventQueue queue;

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
  EventQueue queue;

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
  EventQueue queue;
  EventRunner runner;
  queue.setEventRunner(&runner);

  boost::shared_ptr<Event> pEvent(new Event("my_event"));
  pEvent->setProperty("time", "+2");

  queue.pushEvent(pEvent);
  BOOST_CHECK_EQUAL(runner.getSize(), 1);

  boost::shared_ptr<Event> pEvent2(new Event("my_event"));
  pEvent2->setProperty("unique", "yes");
  pEvent2->setProperty("time", "+2");

  queue.pushEvent(pEvent2);

  BOOST_CHECK_EQUAL(runner.getSize(), 1);
  const ScheduledEvent& eventFromQueue = runner.getEvent(0);
  BOOST_CHECK_EQUAL(eventFromQueue.getEvent()->hasPropertySet("time"), true);
  BOOST_CHECK_EQUAL(eventFromQueue.getEvent()->getPropertyByName("time"), "+2");
} // testUniqueEventsOverwritesTimeProperty

BOOST_AUTO_TEST_CASE(testEventCollector) {
  EventQueue queue;
  EventRunner runner;
  EventInterpreter interpreter(NULL);
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  EventInterpreterInternalRelay* relay = new EventInterpreterInternalRelay(&interpreter);
  interpreter.addPlugin(relay);

  interpreter.run();

  EventCollector collector(*relay);
  std::string subscriptionID = collector.subscribeTo("my_event");

  boost::shared_ptr<Event> pEvent(new Event("my_event"));
  queue.pushEvent(pEvent);

  sleepMS(20);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 1);

  BOOST_CHECK_EQUAL(collector.hasEvent(), true);

  Event evt = collector.popEvent();
  BOOST_CHECK_EQUAL(evt.getName(), "my_event");

  pEvent.reset(new Event("event2"));
  queue.pushEvent(pEvent);

  sleepMS(20);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 2);

  BOOST_CHECK_EQUAL(collector.hasEvent(), false);

  collector.unsubscribeFrom(subscriptionID);

  pEvent.reset(new Event("my_event"));
  queue.pushEvent(pEvent);

  sleepMS(20);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 3);

  BOOST_CHECK_EQUAL(collector.hasEvent(), false);

  queue.shutdown();
  interpreter.terminate();
  sleepMS(1500);
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

BOOST_AUTO_TEST_CASE(testInternalEventRelay) {
  EventQueue queue;
  EventRunner runner;
  EventInterpreter interpreter(NULL);
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  EventInterpreterInternalRelay* relay = new EventInterpreterInternalRelay(&interpreter);
  interpreter.addPlugin(relay);

  interpreter.run();

  InternalEventRelayTarget target(*relay);
  boost::shared_ptr<EventSubscription> subscriptionOne(
    new dss::EventSubscription(
               "one",
               EventInterpreterInternalRelay::getPluginName(),
               interpreter,
               boost::shared_ptr<SubscriptionOptions>())
  );
  target.subscribeTo(subscriptionOne);
  boost::shared_ptr<EventSubscription> subscriptionTwo(
    new dss::EventSubscription(
               "two",
               EventInterpreterInternalRelay::getPluginName(),
               interpreter,
               boost::shared_ptr<SubscriptionOptions>())
  );
  target.subscribeTo(subscriptionTwo);

  InternalEventRelayTester tester;
  target.setCallback(boost::bind(&InternalEventRelayTester::onEvent, &tester, _1, _2));

  boost::shared_ptr<Event> pEvent(new Event("one"));
  queue.pushEvent(pEvent);

  sleepMS(20);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 1);
  BOOST_CHECK_EQUAL(tester.getCounter(), 1);

  queue.pushEvent(pEvent);

  sleepMS(20);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 2);
  BOOST_CHECK_EQUAL(tester.getCounter(), 2);

  pEvent.reset(new Event("two"));
  queue.pushEvent(pEvent);

  sleepMS(20);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 3);
  BOOST_CHECK_EQUAL(tester.getCounter(), 4);

  queue.shutdown();
  interpreter.terminate();
  sleepMS(1500);
}

BOOST_AUTO_TEST_SUITE_END()
