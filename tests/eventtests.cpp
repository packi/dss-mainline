 /*
 * eventtests.cpp
 *
 *  Created on: Nov 7, 2008
 *      Author: patrick
 */

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>


#include "../core/event.h"
#include "../core/eventinterpreterplugins.h"
#include "../core/setbuilder.h"
#include "../core/sim/dssim.h"
#include "../unix/ds485proxy.h"

using namespace dss;

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

class EventTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(EventTest);
//  CPPUNIT_TEST(testSimpleEvent);
//  CPPUNIT_TEST(testSubscription);
//  CPPUNIT_TEST(testEmptySubscriptionXML);
//  CPPUNIT_TEST(testNonExistingXML);
//  CPPUNIT_TEST(testSubscriptionXML);
//  CPPUNIT_TEST(testSetBuilder);
//  CPPUNIT_TEST(testDS485Events);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {}

protected:

  void testSimpleEvent(void) {
    EventQueue queue;
    EventRunner runner;
    EventInterpreter interpreter(&queue);
    interpreter.SetEventRunner(&runner);
    interpreter.Run();

    boost::shared_ptr<Event> pEvent(new Event("event1"));

    queue.PushEvent(pEvent);

    sleep(1);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetEventsProcessed(), 1);

    pEvent.reset(new Event("event2"));

    queue.PushEvent(pEvent);

    sleep(1);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetEventsProcessed(), 2);

    queue.Shutdown();
    interpreter.Terminate();
    sleep(2);
  } // testSimpleEvent

  void testSubscription() {
    EventQueue queue;
    EventRunner runner;
    EventInterpreter interpreter(&queue);
    interpreter.SetEventRunner(&runner);
    EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
    interpreter.AddPlugin(plugin);

    interpreter.Run();

    boost::shared_ptr<Event> pEvent(new Event("my_event"));

    boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
    opts->SetParameter("event_name", "event1");
    boost::shared_ptr<EventSubscription> subscription(new EventSubscription("my_event", "raise_event", opts));
    interpreter.Subscribe(subscription);

    queue.PushEvent(pEvent);

    sleep(1);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetEventsProcessed(), 2);

    pEvent.reset(new Event("event2"));

    queue.PushEvent(pEvent);

    sleep(1);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetEventsProcessed(), 3);

    queue.Shutdown();
    interpreter.Terminate();
    sleep(2);
  } // testSubscription

  void testEmptySubscriptionXML() {
    EventQueue queue;
    EventInterpreter interpreter(&queue);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetNumberOfSubscriptions(), 0);

    interpreter.LoadFromXML("data/testsubscriptions_empty.xml");

    CPPUNIT_ASSERT_EQUAL(interpreter.GetNumberOfSubscriptions(), 0);
  } // testEmptySubscriptionXML

  void testNonExistingXML() {
    EventInterpreter interpreter;

    CPPUNIT_ASSERT_EQUAL(interpreter.GetNumberOfSubscriptions(), 0);

    try {
      interpreter.LoadFromXML("data/iwillnever_be_a_subscription.xml");
    } catch(runtime_error& e) {
    }

    CPPUNIT_ASSERT_EQUAL(interpreter.GetNumberOfSubscriptions(), 0);
  } // testNonExistingXML

  void testSubscriptionXML() {
    EventQueue queue;
    EventRunner runner;
    EventInterpreter interpreter(&queue);
    interpreter.SetEventRunner(&runner);
    queue.SetEventRunner(&runner);
    runner.SetEventQueue(&queue);
    EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
    interpreter.AddPlugin(plugin);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetNumberOfSubscriptions(), 0);

    try {
      interpreter.LoadFromXML("data/testsubscriptions.xml");
    } catch(runtime_error& e) {
    }

    CPPUNIT_ASSERT_EQUAL(interpreter.GetNumberOfSubscriptions(), 2);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetEventsProcessed(), 0);

    interpreter.Run();

    sleep(1);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetEventsProcessed(), 0);

    boost::shared_ptr<Event> evt(new Event("event1"));
    queue.PushEvent(evt);

    sleep(1);

    runner.RunOnce();

    sleep(12);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetEventsProcessed(), 2);

    queue.Shutdown();
    interpreter.Terminate();
    sleep(1);
  } // testSubscriptionXML


  void testSetBuilder() {
    SetBuilder setBuilder;

    Apartment apt;

    Device& dev1 = apt.AllocateDevice(1);
    dev1.SetName("dev1");
    dev1.SetShortAddress(1);
    Device& dev2 = apt.AllocateDevice(2);
    dev2.SetName("dev2");
    dev2.SetShortAddress(2);
    Device& dev3 = apt.AllocateDevice(3);
    dev3.SetName("dev3");
    dev3.SetShortAddress(3);
    Device& dev4 = apt.AllocateDevice(4);
    dev4.SetName("dev4");
    dev4.SetShortAddress(4);

    Set res = setBuilder.BuildSet("", &apt.GetZone(0));
    CPPUNIT_ASSERT_EQUAL(res.Length(), 4);

    res = setBuilder.BuildSet("dev1", &apt.GetZone(0));
    CPPUNIT_ASSERT_EQUAL(res.Length(), 1);
    CPPUNIT_ASSERT_EQUAL(res.Get(0).GetDevice().GetName(), string("dev1"));

    res = setBuilder.BuildSet("dev2", &apt.GetZone(0));
    CPPUNIT_ASSERT_EQUAL(res.Length(), 1);
    CPPUNIT_ASSERT_EQUAL(res.Get(0).GetDevice().GetName(), string("dev2"));
} // testSetBuilder

  void testDS485Events() {
    EventQueue queue;
    EventRunner runner;
    EventInterpreter interpreter(&queue);
    interpreter.SetEventRunner(&runner);
    queue.SetEventRunner(&runner);
    runner.SetEventQueue(&queue);

    DSModulatorSim modSim;
    modSim.Initialize();
    DS485Proxy proxy;

    Apartment apt;

    Device& dev1 = apt.AllocateDevice(1);
    dev1.SetName("dev1");
    dev1.SetShortAddress(1);
    Device& dev2 = apt.AllocateDevice(2);
    dev2.SetName("dev2");
    dev2.SetShortAddress(2);
    Device& dev3 = apt.AllocateDevice(3);
    dev3.SetName("dev3");
    dev3.SetShortAddress(3);
    Device& dev4 = apt.AllocateDevice(4);
    dev4.SetName("dev4");
    dev4.SetShortAddress(4);


    EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
    interpreter.AddPlugin(plugin);
    plugin = new EventInterpreterPluginDS485(&proxy, &interpreter);
    interpreter.AddPlugin(plugin);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetNumberOfSubscriptions(), 0);

    try {
      interpreter.LoadFromXML("data/testsubscriptions_DS485.xml");
    } catch(runtime_error& e) {
    }

    CPPUNIT_ASSERT_EQUAL(interpreter.GetNumberOfSubscriptions(), 3);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetEventsProcessed(), 0);

    interpreter.Run();

    sleep(1);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetEventsProcessed(), 0);

    boost::shared_ptr<Event> evt(new Event("brighter", &apt.GetZone(0)));
    evt->SetLocation("dev1");
    queue.PushEvent(evt);

    sleep(1);

    runner.RunOnce();

    sleep(600);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetEventsProcessed(), 2);

    queue.Shutdown();
    interpreter.Terminate();
    sleep(1);
  } // testDS485Events

};

CPPUNIT_TEST_SUITE_REGISTRATION(EventTest);
