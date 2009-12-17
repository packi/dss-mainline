/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#include "core/scripting/modeljs.h"
#include "core/event.h"
#include "core/eventinterpreterplugins.h"
#include "core/propertysystem.h"

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
  string name = ctx->evaluate<string>("getName()");

  BOOST_CHECK_EQUAL(apt.getName(), name);

  name = ctx->evaluate<string>("setName('hello'); getName()");

  BOOST_CHECK_EQUAL(string("hello"), name);
  BOOST_CHECK_EQUAL(string("hello"), apt.getName());
} // testBasics

BOOST_AUTO_TEST_CASE(testSets) {
  Apartment apt(NULL);
  apt.initialize();

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.setModulatorID(1);
  dev1.addToGroup(1);
  dev1.setIsPresent(true);
  dev1.setZoneID(1);
  dev1.setName("dev1");
  dev1.setFunctionID(1);
  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  dev2.setModulatorID(1);
  dev2.addToGroup(1);
  dev2.setIsPresent(false);
  dev2.setZoneID(2);
  dev2.setName("dev2");
  dev2.setFunctionID(1);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  int length = ctx->evaluate<int>("var devs = getDevices(); devs.length()");

  BOOST_CHECK_EQUAL(2, length);

  ctx->evaluate<void>("var devs = getDevices(); var devs2 = getDevices(); devs.combine(devs2)");

  length = ctx->evaluate<int>("getDevices().byZone(1).length()");
  BOOST_CHECK_EQUAL(1, length);

  length = ctx->evaluate<int>("getDevices().byZone(2).length()");
  BOOST_CHECK_EQUAL(1, length);

  length = ctx->evaluate<int>("getDevices().byGroup(1).length()");
  BOOST_CHECK_EQUAL(2, length);

  length = ctx->evaluate<int>("getDevices().byGroup('yellow').length()");
  BOOST_CHECK_EQUAL(2, length);

  length = ctx->evaluate<int>("getDevices().byGroup('asdf').length()");
  BOOST_CHECK_EQUAL(0, length);

  length = ctx->evaluate<int>("getDevices().byDSMeter(1).length()");
  BOOST_CHECK_EQUAL(2, length);

  length = ctx->evaluate<int>("getDevices().byPresence(false).length()");
  BOOST_CHECK_EQUAL(1, length);

  length = ctx->evaluate<int>("getDevices().byPresence(true).length()");
  BOOST_CHECK_EQUAL(1, length);

  length = ctx->evaluate<int>("getDevices().remove(getDevices().byZone(1)).byZone(1).length()");
  BOOST_CHECK_EQUAL(0, length);

  length = ctx->evaluate<int>("getDevices().remove(getDevices().byZone(1)).byZone(1).length()");
  BOOST_CHECK_EQUAL(0, length);

  std::string name = ctx->evaluate<std::string>("getDevices().byName('dev1').name");
  BOOST_CHECK_EQUAL("dev1", name);

  name = ctx->evaluate<std::string>("getDevices().byDSID('2').name");
  BOOST_CHECK_EQUAL("dev2", name);

  length = ctx->evaluate<int>("getDevices().byFunctionID(1).length()");
  BOOST_CHECK_EQUAL(2, length);

  // invalid types
  length = ctx->evaluate<int>("getDevices().byZone(1.1).length()");
  BOOST_CHECK_EQUAL(0, length);

  length = ctx->evaluate<int>("getDevices().byGroup(1.1).length()");
  BOOST_CHECK_EQUAL(0, length);
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
  ctx->evaluate<void>("var devs = getDevices();\n"
                      "var f = function(dev) {\n"
                      "  print(dev.name);\n"
                      "  print('lastCalledScene: ',dev.lastCalledScene);\n"
                      "}\n"
                      "devs.perform(f)\n");
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
  ctx->evaluate<void>("var evt = new event('test');\n"
                      "evt.raise()\n"
                      "\n");
  // TODO: add subscription to confirm the event actually got raised
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
  ctx->evaluate<void>("var s = new subscription('test', 'test', { 'param1': 1, 'param2': 2, 'string': 'string'} );\n"
                      "s.subscribe();\n"
                      "\n");

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 1);
} // testSubscriptions

BOOST_AUTO_TEST_CASE(testProperties) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("setProperty('/testing', 1)");
  BOOST_CHECK_EQUAL(ctx->evaluate<int>("getProperty('/testing')"), 1);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 1);
  
  propSys.setIntValue("/testing", 2);
  BOOST_CHECK_EQUAL(ctx->evaluate<int>("getProperty('/testing')"), 2);
}

BOOST_AUTO_TEST_CASE(testPropertyListener) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("setProperty('/testing', 1); setProperty('/triggered', false); "
                      "listener_ident = setListener('/testing', function(changedNode) { setProperty('/triggered', true); }); "
      );

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 1);

  propSys.setIntValue("/testing", 2);

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), true);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 2);

  // check that removing works
  ctx->evaluate<void>("removeListener(listener_ident);");

  propSys.setBoolValue("/triggered", false);
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);

  propSys.setIntValue("/testing", 2);

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 2);

  // check that closures are working as expected
  ctx->evaluate<void>("setProperty('/triggered', false); "
                      "var ident = setListener('/testing', function(changedNode) { setProperty('/triggered', true); removeListener(ident); }); "
      );

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);

  propSys.setIntValue("/testing", 2);

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), true);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 2);

  propSys.setBoolValue("/triggered", false);
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);

  propSys.setIntValue("/testing", 2);

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 2);
}

BOOST_AUTO_TEST_CASE(testReentrancy) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("setProperty('/testing', 1); setProperty('/triggered', false); "
                            "other_ident = setListener('/triggered', function() { setProperty('/itWorks', true); } ); "
                            "listener_ident = setListener('/testing', function(changedNode) { setProperty('/triggered', true); }); "
      );
      
  propSys.setBoolValue("/testing", true);
  
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/itWorks"), true);

  // TODO: find out why it crashes w/o those lines
  ctx->evaluate<void>("removeListener(other_ident); "
                      "removeListener(listener_ident); "
      );

  ctx.reset();
} // testReentrancy

class TestThreadingThread : public Thread {
public:
  TestThreadingThread(PropertyNodePtr _node) 
  : Thread("TestThreadingThread"),
    m_pNode(_node)
  {
  }
  
  virtual void execute() {
    while(!m_Terminated) {
      m_pNode->setIntegerValue(2);
    }
  }
  
private:
  PropertyNodePtr m_pNode;
}; // TestThreadingThread

BOOST_AUTO_TEST_CASE(testThreading) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("var func = setProperty('/testing1', 1); setProperty('/testing2', 1); "
                      "l1 = setListener('/testing1', function() { setProperty('/itWorks', true); } ); "
                      "l2 = setListener('/testing2', function() { setProperty('/itWorks', true); } ); "
      );
      
  PropertyNodePtr node1 = propSys.getProperty("/testing1");
  PropertyNodePtr node2 = propSys.getProperty("/testing2");
  
  // different nodes
  {
    TestThreadingThread t1(node1);
    TestThreadingThread t2(node2);
    t1.run();
    t2.run();
  
    sleepSeconds(1);
  
    t1.terminate();
    t2.terminate();
    sleepMS(500);
  }
  
  // same node
  {
    TestThreadingThread t1(node1);
    TestThreadingThread t2(node1);
    t1.run();
    t2.run();
  
    sleepSeconds(1);
  
    t1.terminate();
    t2.terminate();
    sleepMS(500);
  }

  // TODO: find out why it crashes w/o those lines
  ctx->evaluate<void>("removeListener(l1); removeListener(l2);");
} // testThreading

BOOST_AUTO_TEST_SUITE_END()
