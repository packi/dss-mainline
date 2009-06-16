/*
 *  modeljstests.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/23/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "../core/scripting/modeljs.h"
#include "../core/event.h"
#include "../core/eventinterpreterplugins.h"

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <boost/scoped_ptr.hpp>

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

#include <memory>

using namespace std;

using namespace dss;

class ModelTestJS : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ModelTestJS);
  CPPUNIT_TEST(testBasics);
  CPPUNIT_TEST(testSets);
  CPPUNIT_TEST(testDevices);
  CPPUNIT_TEST(testEvents);
  CPPUNIT_TEST(testSubscriptions);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {}


protected:
  void testBasics(void) {
    Apartment apt(NULL);
    apt.setName("my apartment");


    auto_ptr<ScriptEnvironment> env(new ScriptEnvironment());
    env->initialize();
    ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
    env->addExtension(ext);

    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->loadFromMemory("getName()");
    string name = ctx->evaluate<string>();

    CPPUNIT_ASSERT_EQUAL(apt.getName(), name);

    ctx.reset(env->getContext());
    ctx->loadFromMemory("setName('hello'); getName()");

    name = ctx->evaluate<string>();
    ctx.reset();

    CPPUNIT_ASSERT_EQUAL(string("hello"), name);
    CPPUNIT_ASSERT_EQUAL(string("hello"), apt.getName());
  } // testBasics

  void testSets() {
    Apartment apt(NULL);
    apt.initialize();

    Device& dev1 = apt.allocateDevice(dsid_t(0,1));
    dev1.setShortAddress(1);
    Device& dev2 = apt.allocateDevice(dsid_t(0,2));
    dev2.setShortAddress(2);

    boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
    env->initialize();
    ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
    env->addExtension(ext);

    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->loadFromMemory("var devs = getDevices(); devs.length()");
    int length = ctx->evaluate<int>();

    CPPUNIT_ASSERT_EQUAL(2, length);

    ctx.reset(env->getContext());
    ctx->loadFromMemory("var devs = getDevices(); var devs2 = getDevices(); devs.combine(devs2)");
    ctx->evaluate<void>();
  } // testSets

  void testDevices() {
    Apartment apt(NULL);
    apt.initialize();

    Device& dev1 = apt.allocateDevice(dsid_t(0,1));
    dev1.setShortAddress(1);
    dev1.setName("dev1");
    Device& dev2 = apt.allocateDevice(dsid_t(0,2));
    dev2.setShortAddress(2);
    dev2.setName("dev2");

    boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
    env->initialize();
    ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
    env->addExtension(ext);

    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->loadFromMemory("var devs = getDevices();\n"
                        "var f = function(dev) { print(dev.name); }\n"
                        "devs.perform(f)\n");
    ctx->evaluate<void>();
  }

  void testEvents() {
    Apartment apt(NULL);
    apt.initialize();

    Device& dev = apt.allocateDevice(dsid_t(0,1));
    dev.setShortAddress(1);
    dev.setName("dev");

    EventQueue queue;
    EventRunner runner;
    EventInterpreter interpreter(NULL);
    interpreter.setEventQueue(&queue);
    interpreter.setEventRunner(&runner);
    queue.setEventRunner(&runner);
    runner.setEventQueue(&queue);

    boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
    env->initialize();
    ScriptExtension* ext = new ModelScriptContextExtension(apt);
    env->addExtension(ext);
    ext = new EventScriptExtension(queue, interpreter);
    env->addExtension(ext);

    EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
    interpreter.addPlugin(plugin);

    CPPUNIT_ASSERT_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->loadFromMemory("var evt = new event('test');\n"
                        "evt.raise()\n"
                        "\n");
    ctx->evaluate<void>();
  } // testEvents

  void testSubscriptions() {
    Apartment apt(NULL);
    apt.initialize();

    Device& dev = apt.allocateDevice(dsid_t(0,1));
    dev.setShortAddress(1);
    dev.setName("dev");

    EventQueue queue;
    EventRunner runner;
    EventInterpreter interpreter(NULL);
    interpreter.setEventQueue(&queue);
    interpreter.setEventRunner(&runner);
    queue.setEventRunner(&runner);
    runner.setEventQueue(&queue);

    boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
    env->initialize();
    ScriptExtension* ext = new ModelScriptContextExtension(apt);
    env->addExtension(ext);
    ext = new EventScriptExtension(queue, interpreter);
    env->addExtension(ext);

    EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
    interpreter.addPlugin(plugin);

    CPPUNIT_ASSERT_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

    boost::scoped_ptr<ScriptContext> ctx(env->getContext());
    ctx->loadFromMemory("var s = new subscription('test', 'test', { 'param1': 1, 'param2': 2, 'string': 'string'} );\n"
                        "s.subscribe();\n"
                        "\n");
    ctx->evaluate<void>();

    CPPUNIT_ASSERT_EQUAL(interpreter.getNumberOfSubscriptions(), 1);
  } // testSubscriptions

};

CPPUNIT_TEST_SUITE_REGISTRATION(ModelTestJS);



