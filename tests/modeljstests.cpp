#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "../core/scripting/modeljs.h"
#include "../core/event.h"
#include "../core/eventinterpreterplugins.h"

#include <boost/scoped_ptr.hpp>
#include <memory>

using namespace std;
using namespace dss;

BOOST_AUTO_TEST_SUITE(ModelJS)

BOOST_AUTO_TEST_CASE(testBasics) {
  Apartment apt(NULL);
  apt.setName("my apartment");


  auto_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->loadFromMemory("getName()");
  string name = ctx->evaluate<string>();

  BOOST_CHECK_EQUAL(apt.getName(), name);

  ctx.reset(env->getContext());
  ctx->loadFromMemory("setName('hello'); getName()");

  name = ctx->evaluate<string>();
  ctx.reset();

  BOOST_CHECK_EQUAL(string("hello"), name);
  BOOST_CHECK_EQUAL(string("hello"), apt.getName());
} // testBasics

BOOST_AUTO_TEST_CASE(testSets) {
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

  BOOST_CHECK_EQUAL(2, length);

  ctx.reset(env->getContext());
  ctx->loadFromMemory("var devs = getDevices(); var devs2 = getDevices(); devs.combine(devs2)");
  ctx->evaluate<void>();
} // testSets

BOOST_AUTO_TEST_CASE(testDevices) {
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
} // testDevices

BOOST_AUTO_TEST_CASE(testEvents) {
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

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->loadFromMemory("var evt = new event('test');\n"
                      "evt.raise()\n"
                      "\n");
  ctx->evaluate<void>();
} // testEvents

BOOST_AUTO_TEST_CASE(testSubscriptions) {
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

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->loadFromMemory("var s = new subscription('test', 'test', { 'param1': 1, 'param2': 2, 'string': 'string'} );\n"
                      "s.subscribe();\n"
                      "\n");
  ctx->evaluate<void>();

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 1);
} // testSubscriptions

BOOST_AUTO_TEST_SUITE_END()
