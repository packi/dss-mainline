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

using namespace dss;

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

class EventTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(EventTest);
  CPPUNIT_TEST(testSimpleEvent);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {}

protected:
  void testSimpleEvent(void) {
    EventQueue queue;
    EventInterpreter interpreter(&queue);
    interpreter.Execute();

    Event* pEvent = new Event("event1");

    queue.PushEvent(pEvent);


/*
    ScriptEnvironment env;
    env.Initialize();
    ScriptContext* ctx = env.GetContext();
    ctx->LoadFromMemory("x = 10; print(x); x = x * x;");
    double result = ctx->Evaluate<double>();
    CPPUNIT_ASSERT_EQUAL(result, 100.0);
    delete ctx;

    ctx = env.GetContext();
    ctx->LoadFromFile("data/test.js");
    result = ctx->Evaluate<double>();
    CPPUNIT_ASSERT_EQUAL(result, 100.0);
    delete ctx;

    ctx = env.GetContext();
    ctx->LoadFromMemory("x = 'bla'; x = x + 'bla';");
    string sres = ctx->Evaluate<string>();
    CPPUNIT_ASSERT_EQUAL(sres, string("blabla"));
    delete ctx;
    */
    interpreter.Terminate();
    sleep(2);
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION(EventTest);
