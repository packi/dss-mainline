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
    apt.SetName("my apartment");


    auto_ptr<ScriptEnvironment> env(new ScriptEnvironment());
    env->Initialize();
    ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
    env->AddExtension(ext);

    boost::scoped_ptr<ScriptContext> ctx(env->GetContext());
    ctx->LoadFromMemory("getName()");
    string name = ctx->Evaluate<string>();

    CPPUNIT_ASSERT_EQUAL(apt.GetName(), name);

    ctx.reset(env->GetContext());
    ctx->LoadFromMemory("setName('hello'); getName()");

    name = ctx->Evaluate<string>();
    ctx.reset();

    CPPUNIT_ASSERT_EQUAL(string("hello"), name);
    CPPUNIT_ASSERT_EQUAL(string("hello"), apt.GetName());
  } // testBasics

  void testSets() {
    Apartment apt(NULL);
    apt.Initialize();

    Device& dev1 = apt.AllocateDevice(dsid_t(0,1));
    dev1.SetShortAddress(1);
    Device& dev2 = apt.AllocateDevice(dsid_t(0,2));
    dev2.SetShortAddress(2);

    boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
    env->Initialize();
    ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
    env->AddExtension(ext);

    boost::scoped_ptr<ScriptContext> ctx(env->GetContext());
    ctx->LoadFromMemory("var devs = getDevices(); devs.length()");
    int length = ctx->Evaluate<int>();

    CPPUNIT_ASSERT_EQUAL(2, length);

    ctx.reset(env->GetContext());
    ctx->LoadFromMemory("var devs = getDevices(); var devs2 = getDevices(); devs.combine(devs2)");
    ctx->Evaluate<void>();
  } // testSets

  void testDevices() {
    Apartment apt(NULL);
    apt.Initialize();

    Device& dev1 = apt.AllocateDevice(dsid_t(0,1));
    dev1.SetShortAddress(1);
    dev1.SetName("dev1");
    Device& dev2 = apt.AllocateDevice(dsid_t(0,2));
    dev2.SetShortAddress(2);
    dev2.SetName("dev2");

    boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
    env->Initialize();
    ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
    env->AddExtension(ext);

    boost::scoped_ptr<ScriptContext> ctx(env->GetContext());
    ctx->LoadFromMemory("var devs = getDevices();\n"
                        "var f = function(dev) { print(dev.name); }\n"
                        "devs.perform(f)\n");
    ctx->Evaluate<void>();
  }

  void testEvents() {
    Apartment apt(NULL);
    apt.Initialize();

    Device& dev = apt.AllocateDevice(dsid_t(0,1));
    dev.SetShortAddress(1);
    dev.SetName("dev");

    EventQueue queue;
    EventRunner runner;
    EventInterpreter interpreter(NULL);
    interpreter.SetEventQueue(&queue);
    interpreter.SetEventRunner(&runner);
    queue.SetEventRunner(&runner);
    runner.SetEventQueue(&queue);

    boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
    env->Initialize();
    ScriptExtension* ext = new ModelScriptContextExtension(apt);
    env->AddExtension(ext);
    ext = new EventScriptExtension(queue, interpreter);
    env->AddExtension(ext);

    EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
    interpreter.AddPlugin(plugin);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetNumberOfSubscriptions(), 0);

    boost::scoped_ptr<ScriptContext> ctx(env->GetContext());
    ctx->LoadFromMemory("var evt = new event('test');\n"
                        "evt.raise()\n"
                        "\n");
    ctx->Evaluate<void>();
  } // testEvents

  void testSubscriptions() {
    Apartment apt(NULL);
    apt.Initialize();

    Device& dev = apt.AllocateDevice(dsid_t(0,1));
    dev.SetShortAddress(1);
    dev.SetName("dev");

    EventQueue queue;
    EventRunner runner;
    EventInterpreter interpreter(NULL);
    interpreter.SetEventQueue(&queue);
    interpreter.SetEventRunner(&runner);
    queue.SetEventRunner(&runner);
    runner.SetEventQueue(&queue);

    boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
    env->Initialize();
    ScriptExtension* ext = new ModelScriptContextExtension(apt);
    env->AddExtension(ext);
    ext = new EventScriptExtension(queue, interpreter);
    env->AddExtension(ext);

    EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
    interpreter.AddPlugin(plugin);

    CPPUNIT_ASSERT_EQUAL(interpreter.GetNumberOfSubscriptions(), 0);

    boost::scoped_ptr<ScriptContext> ctx(env->GetContext());
    ctx->LoadFromMemory("var s = new subscription('test', 'test', { 'param1': 1, 'param2': 2, 'string': 'string'} );\n"
                        "s.subscribe();\n"
                        "\n");
    ctx->Evaluate<void>();

    CPPUNIT_ASSERT_EQUAL(interpreter.GetNumberOfSubscriptions(), 1);
  } // testSubscriptions

};

CPPUNIT_TEST_SUITE_REGISTRATION(ModelTestJS);



