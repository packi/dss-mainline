#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "../core/event.h"
#include "../core/eventinterpreterplugins.h"
#include "../core/setbuilder.h"
#include "../core/sim/dssim.h"
#include "../unix/ds485proxy.h"

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
  boost::shared_ptr<EventSubscription> subscription(new EventSubscription("my_event", "raise_event", opts));
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

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

  interpreter.loadFromXML("data/testsubscriptions_empty.xml");

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);
} // testEmptySubscriptionXML

void testNonExistingXML() {
  EventInterpreter interpreter(NULL);

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

  try {
    interpreter.loadFromXML("data/iwillnever_be_a_subscription.xml");
  } catch(runtime_error& e) {
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

  try {
    interpreter.loadFromXML("data/testsubscriptions.xml");
  } catch(runtime_error& e) {
  }

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 2);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 0);

  interpreter.run();

  sleep(1);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 0);

  boost::shared_ptr<Event> evt(new Event("event1"));
  queue.pushEvent(evt);

  sleep(1);

  runner.runOnce();

  sleep(12);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 2);

  queue.shutdown();
  interpreter.terminate();
  sleep(1);
} // testSubscriptionXML


BOOST_AUTO_TEST_CASE(testSetBuilder) {
  SetBuilder setBuilder;

  Apartment apt(NULL);

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

  Set res = setBuilder.buildSet("", &apt.getZone(0));
  BOOST_CHECK_EQUAL(res.length(), 4);

  res = setBuilder.buildSet("dev1", &apt.getZone(0));
  BOOST_CHECK_EQUAL(res.length(), 1);
  BOOST_CHECK_EQUAL(res.get(0).getDevice().getName(), string("dev1"));

  res = setBuilder.buildSet("dev2", &apt.getZone(0));
  BOOST_CHECK_EQUAL(res.length(), 1);
  BOOST_CHECK_EQUAL(res.get(0).getDevice().getName(), string("dev2"));
} // testSetBuilder

BOOST_AUTO_TEST_CASE(testDS485Events) {
  EventQueue queue;
  EventRunner runner;
  EventInterpreter interpreter(NULL);
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  queue.setEventRunner(&runner);
  runner.setEventQueue(&queue);

  DSModulatorSim modSim(NULL);
  DS485Proxy proxy(NULL);

  Apartment apt(NULL);

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
  plugin = new EventInterpreterPluginDS485(&proxy, &interpreter);
  interpreter.addPlugin(plugin);

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

  try {
    interpreter.loadFromXML("data/testsubscriptions_DS485.xml");
  } catch(runtime_error& e) {
  }

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 3);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 0);

  interpreter.run();

  sleep(1);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 0);

  boost::shared_ptr<Event> evt(new Event("brighter", &apt.getZone(0)));
  evt->setLocation("dev1");
  queue.pushEvent(evt);

  sleep(1);

  runner.runOnce();

  sleep(600);

  BOOST_CHECK_EQUAL(interpreter.getEventsProcessed(), 2);

  queue.shutdown();
  interpreter.terminate();
  sleep(1);
} // testDS485Events

BOOST_AUTO_TEST_SUITE_END()
