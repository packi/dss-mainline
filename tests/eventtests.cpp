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

using namespace dss;

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

class EventTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(EventTest);
  CPPUNIT_TEST(testSimpleEvent);
  CPPUNIT_TEST(testSubscription);
  CPPUNIT_TEST(testEmptySubscriptionXML);
  CPPUNIT_TEST(testNonExistingXML);
  CPPUNIT_TEST(testSubscriptionXML);
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

    Event* pEvent = new Event("event1");

    queue.PushEvent(pEvent);

    sleep(1);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetEventsProcessed(), 1);

    pEvent = new Event("event2");

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

    Event* pEvent = new Event("my_event");

    SubscriptionOptions* opts = new SubscriptionOptions();
    opts->SetParameter("event_name", "event1");
    EventSubscription* subscription = new EventSubscription("my_event", "raise_event", opts);
    interpreter.Subscribe(subscription);

    queue.PushEvent(pEvent);

    sleep(1);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetEventsProcessed(), 2);

    pEvent = new Event("event2");

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

    Event* evt = new Event("event1");
    queue.PushEvent(evt);

    sleep(1);

    runner.RunOnce();

    sleep(12);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetEventsProcessed(), 2);

    queue.Shutdown();
    interpreter.Terminate();
    sleep(1);
  } // testSubscriptionXML


};

CPPUNIT_TEST_SUITE_REGISTRATION(EventTest);
