/*
    Copyright (c) 2009,2011 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
            Michael Tross, aizo GmbH <michael.tross@aizo.com>

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

#include "core/scripting/jsmodel.h"
#include "core/scripting/jsevent.h"
#include "core/scripting/jsmetering.h"
#include "core/scripting/scriptobject.h"
#include "core/scripting/jsproperty.h"
#include "core/event.h"
#include "core/eventinterpreterplugins.h"
#include "core/propertysystem.h"
#include "core/model/apartment.h"
#include "core/model/device.h"
#include "core/model/modulator.h"
#include "core/model/modelconst.h"
#include "core/metering/metering.h"

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

  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dss_dsid_t(0,10));

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setShortAddress(1);
  dev1->setDSMeter(meter);
  dev1->addToGroup(1);
  dev1->setIsPresent(true);
  dev1->setZoneID(1);
  dev1->setName("dev1");
  dev1->setFunctionID(1);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->setShortAddress(2);
  dev2->setDSMeter(meter);
  dev2->addToGroup(1);
  dev2->setIsPresent(false);
  dev2->setZoneID(2);
  dev2->setName("dev2");
  dev2->setFunctionID(1);

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

  length = ctx->evaluate<int>("getDevices().byDSMeter('a').length()");
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

BOOST_AUTO_TEST_CASE(testSetTags) {
  Apartment apt(NULL);
  PropertySystem propSys;
  apt.setPropertySystem(&propSys);

  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dss_dsid_t(0,10));

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->addTag("dev1");
  dev1->addTag("device");
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->addTag("dev2");
  dev2->addTag("device");

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  int length = ctx->evaluate<int>("var devs = getDevices().byTag('dev1'); devs.length()");
  BOOST_CHECK_EQUAL(length, 1);

  length = ctx->evaluate<int>("var devs = getDevices().byTag('dev2'); devs.length()");
  BOOST_CHECK_EQUAL(length, 1);

  length = ctx->evaluate<int>("var devs = getDevices().byTag('device'); devs.length()");
  BOOST_CHECK_EQUAL(length, 2);

  length = ctx->evaluate<int>("var devs = getDevices().byTag('nonexisting'); devs.length()");
  BOOST_CHECK_EQUAL(length, 0);
}

BOOST_AUTO_TEST_CASE(testDevices) {
  Apartment apt(NULL);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setShortAddress(1);
  dev1->setName("dev1");
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->setShortAddress(2);
  dev2->setName("dev2");

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

BOOST_AUTO_TEST_CASE(testGlobalDSMeterGetByDSID) {
  Apartment apt(NULL);

  boost::shared_ptr<DSMeter> meter1 = apt.allocateDSMeter(dss_dsid_t(0,0xa));
  meter1->setName("meter1");
  boost::shared_ptr<DSMeter> meter2 = apt.allocateDSMeter(dss_dsid_t(0,0xb));
  meter2->setName("meter2");

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  std::string name = ctx->evaluate<std::string>("getDSMeterByDSID('a').name");
  BOOST_CHECK_EQUAL(name, meter1->getName());

  name = ctx->evaluate<std::string>("getDSMeterByDSID('b').name");
  BOOST_CHECK_EQUAL(name, meter2->getName());

  ctx->evaluate<void>("getDSMeterByDSID('123')");
} // testGlobalDSMeterGetByDSID

BOOST_AUTO_TEST_CASE(testSceneConstants) {
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ModelConstantsScriptExtension* ext = new ModelConstantsScriptExtension();
  env->addExtension(ext);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("var result = Scene.User1;");
  {
    JSContextThread thread(ctx);
    BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("result"), Scene1);
  }

  ctx->evaluate<void>("result = Scene.Alarm;");
  {
    JSContextThread thread(ctx);
    BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("result"), SceneAlarm);
  }
} // testSceneConstants

BOOST_AUTO_TEST_CASE(testEvents) {
  Apartment apt(NULL);

  boost::shared_ptr<Device> dev = apt.allocateDevice(dss_dsid_t(0,1));
  dev->setShortAddress(1);
  dev->setName("dev");

  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter);
  EventRunner runner(&interpreter);

  interpreter.initialize();
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
  ctx->evaluate<void>("var evt = new Event('test');\n"
                      "evt.raise()\n"
                      "\n");
  // TODO: add subscription to confirm the event actually got raised
} // testEvents

BOOST_AUTO_TEST_CASE(testTimedEvents) {
  Apartment apt(NULL);

  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter);
  EventRunner runner(&interpreter);

  interpreter.initialize();
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  queue.setEventRunner(&runner);
  runner.setEventQueue(&queue);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new EventScriptExtension(queue, interpreter);
  env->addExtension(ext);

  EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
  interpreter.addPlugin(plugin);

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  std::string id = ctx->evaluate<std::string>(
      "var evt = new TimedEvent('test', '+1'); evt.raise()\n");

  BOOST_CHECK(!id.empty());
  BOOST_CHECK(id.find("test") != std::string::npos);
} // testTimedEvents

BOOST_AUTO_TEST_CASE(testTimedEventsNoTimeParam) {
  Apartment apt(NULL);

  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter);
  EventRunner runner(&interpreter);

  interpreter.initialize();
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  queue.setEventRunner(&runner);
  runner.setEventQueue(&queue);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new EventScriptExtension(queue, interpreter);
  env->addExtension(ext);

  EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
  interpreter.addPlugin(plugin);

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<std::string>(
      "var evt; var err = 0; "
      "try { evt = new TimedEvent('test'); } "
      "catch(txt) { print('Forced Exception: ' + txt); err = 1;}\n");
  {
      JSContextThread thread(ctx);
      BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("err"), 1);
  }
} // testTimedEventsNoTimeParam

BOOST_AUTO_TEST_CASE(testTimedICalEvent) {
  Apartment apt(NULL);

  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter);
  EventRunner runner(&interpreter);

  interpreter.initialize();
  interpreter.setEventQueue(&queue);
  interpreter.setEventRunner(&runner);
  queue.setEventRunner(&runner);
  runner.setEventQueue(&queue);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new EventScriptExtension(queue, interpreter);
  env->addExtension(ext);

  EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&interpreter);
  interpreter.addPlugin(plugin);

  BOOST_CHECK_EQUAL(interpreter.getNumberOfSubscriptions(), 0);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  std::string id = ctx->evaluate<std::string>("var evt = new TimedICalEvent('test', '20110227T081600Z','FREQ=MINUTELY;INTERVAL=2');\n"
                                              "evt.raise()\n");
  BOOST_CHECK(!id.empty());
  BOOST_CHECK(id.find("0") == 0);
} // testTimedICalEvent

BOOST_AUTO_TEST_CASE(testSubscriptions) {
  Apartment apt(NULL);

  boost::shared_ptr<Device> dev = apt.allocateDevice(dss_dsid_t(0,1));
  dev->setShortAddress(1);
  dev->setName("dev");

  EventInterpreter interpreter(NULL);
  EventQueue queue(&interpreter);
  EventRunner runner(&interpreter);

  interpreter.initialize();
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
  ctx->evaluate<void>("var s = new Subscription('test', 'test', { 'param1': 1, 'param2': 2, 'string': 'string'} );\n"
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
  ctx->evaluate<void>("Property.setProperty('/testing', 1)");
//  BOOST_CHECK_EQUAL(ctx->evaluate<int>("getProperty('/testing')"), 1);
//  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 1);

//  propSys.setIntValue("/testing", 2);
//  BOOST_CHECK_EQUAL(ctx->evaluate<int>("getProperty('/testing')"), 2);
}

BOOST_AUTO_TEST_CASE(testPropertyListener) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/testing', 1); Property.setProperty('/triggered', false); "
                      "var listener_ident = Property.setListener('/testing', function(changedNode) { Property.setProperty('/triggered', true); }); "
      );

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 1);

  propSys.setIntValue("/testing", 2);
  sleep(1); // wait until listeners have run

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), true);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 2);

  // check that removing works
  ctx->evaluate<void>("Property.removeListener(listener_ident);");

  propSys.setBoolValue("/triggered", false);
  sleep(1); // wait until listeners have run

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);

  propSys.setIntValue("/testing", 2);
  sleep(1); // wait until listeners have run

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 2);

  // check that closures are working as expected
  ctx->evaluate<void>("Property.setProperty('/triggered', false); Property.setProperty('/testing', 1); "
                      "var ident = Property.setListener('/testing', function(changedNode) { Property.setProperty('/triggered', true); Property.removeListener(ident); }); "
      );
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);

  propSys.setIntValue("/testing", 2);
  sleep(1); // wait until listeners have run

  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), true);
  BOOST_CHECK_EQUAL(propSys.getIntValue("/testing"), 2);

  propSys.setBoolValue("/triggered", false);
  sleep(1); // wait until listeners have run
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/triggered"), false);

  propSys.setIntValue("/testing", 2);
  sleep(1); // wait until listeners have run
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
  ctx->evaluate<void>("Property.setProperty('/testing', 1); Property.setProperty('/triggered', false); "
                      "var other_ident = Property.setListener('/triggered', function() { Property.setProperty('/itWorks', true); } ); "
                      "var listener_ident = Property.setListener('/testing', function(changedNode) { Property.setProperty('/triggered', true); }); "
      );

  propSys.setBoolValue("/testing", true);
  sleep(1); // wait until listeners have run
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/itWorks"), true);

  ctx->evaluate<void>("Property.removeListener(other_ident); "
                      "Property.removeListener(listener_ident); "
      );

} // testReentrancy

BOOST_AUTO_TEST_CASE(testMultipleListeners) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>(
      "var result = 0; Property.setProperty('/triggered', false); "
      "var l1 = Property.setListener('/triggered', function(changedNode) { Property.setProperty('/l1', true); result ++; } ); "
      "var l2 = Property.setListener('/triggered', function(changedNode) { Property.setProperty('/l2', true); result ++; } ); "
      "var l3 = Property.setListener('/triggered', function(changedNode) { Property.setProperty('/l3', true); result ++; }); "
  );

  propSys.setBoolValue("/triggered", true);
  sleep(1); // wait until listeners have run
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/l1"), true);
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/l2"), true);
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/l3"), true);
  {
    JSContextThread thread(ctx);
    BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("result"), 3);
  }

  ctx->evaluate<void>(
      "Property.removeListener(l1); "
      "Property.removeListener(l2); "
      "Property.removeListener(l3); "
  );

} // testMultipleListeners

BOOST_AUTO_TEST_CASE(testRemoveListener) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>(
      "var result = 0; var listener; Property.setProperty('/triggered', false); "
      "function cb() { Property.removeListener(listener); Property.setProperty('/l1', true); result ++; }"
      "listener = Property.setListener('/triggered', cb); "
  );

  propSys.setBoolValue("/triggered", true);
  sleep(1); // wait until listeners have run
  BOOST_CHECK_EQUAL(propSys.getBoolValue("/l1"), true);
  {
    JSContextThread thread(ctx);
    BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("result"), 1);
  }

} // testRemoveListener

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
      sleepMS(100);
    }
    Logger::getInstance()->log("thread terminating");
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
  ctx->evaluate<void>("Property.setProperty('/testing1', 1); Property.setProperty('/testing2', 1); "
                      "var l1 = Property.setListener('/testing1', function() { Property.setProperty('/itworks', true); }); "
                      "var l2 = Property.setListener('/testing2', function() { Property.setProperty('/itworks', true); }); "
      );

  PropertyNodePtr node1 = propSys.getProperty("/testing1");
  PropertyNodePtr node2 = propSys.getProperty("/testing2");

  // different nodes
  {
    Logger::getInstance()->log("different nodes");
    TestThreadingThread t1(node1);
    TestThreadingThread t2(node2);
    t1.run();
    t2.run();

    sleepSeconds(1);

    t1.terminate();
    t2.terminate();
    sleepMS(100);
  }

  // same node
  {
    Logger::getInstance()->log("same node");
    TestThreadingThread t1(node1);
    TestThreadingThread t2(node1);
    t1.run();
    t2.run();

    sleepSeconds(1);

    t1.terminate();
    t2.terminate();
    sleepMS(100);
  }

  ctx->evaluate<void>("Property.removeListener(l1); Property.removeListener(l2);");
} // testThreading

BOOST_AUTO_TEST_CASE(testPropertyFlags) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  PropertyNodePtr node1 = propSys.createProperty("/testing");

  ctx->evaluate<void>("Property.setFlag('/testing', 'ARCHIVE', true);");
  BOOST_CHECK_EQUAL(node1->hasFlag(PropertyNode::Archive), true);
  bool res = ctx->evaluate<bool>("Property.hasFlag('/testing', 'ARCHIVE')");
  BOOST_CHECK_EQUAL(res, true);

  ctx->evaluate<void>("Property.setFlag('/testing', 'READABLE', false);");
  BOOST_CHECK_EQUAL(node1->hasFlag(PropertyNode::Readable), false);
  res = ctx->evaluate<bool>("Property.hasFlag('/testing', 'READABLE')");
  BOOST_CHECK_EQUAL(res, false);

  ctx->evaluate<void>("Property.setFlag('/testing', 'WRITEABLE', false);");
  BOOST_CHECK_EQUAL(node1->hasFlag(PropertyNode::Writeable), false);
  res = ctx->evaluate<bool>("Property.hasFlag('/testing', 'WRITEABLE')");
  BOOST_CHECK_EQUAL(res, false);
} // testPropertyFlags

BOOST_AUTO_TEST_CASE(testPropertyObjExisting) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  PropertyNodePtr node1 = propSys.createProperty("/testing");

  ctx->evaluate<void>("var prop = new Property('/testing');\n"
                      "prop.setValue('test');\n"
  );

  BOOST_CHECK_EQUAL(node1->getStringValue(), "test");
} // testPropertyObjExisting

BOOST_AUTO_TEST_CASE(testPropertyObjNonExisting) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());

  ctx->evaluate<void>("var prop = new Property('/testing');\n"
                      "prop.setValue('test');\n"
  );

  PropertyNodePtr node1 = propSys.getProperty("/testing");
  BOOST_CHECK_EQUAL(node1->getStringValue(), "test");
} // testPropertyObjNonExisting

BOOST_AUTO_TEST_CASE(testPropertyObjNonExistingInvalid) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<int>(
		  "var err = 0; \n"
		  "try { var prop = new Property('testing'); }\n"
          "catch (txt) { print(txt); err = 1; }\n");
  {
      JSContextThread thread(ctx);
      BOOST_CHECK_EQUAL(ctx->getRootObject().getProperty<int>("err"), 1);
  }
} // testPropertyObjNonExisting

BOOST_AUTO_TEST_CASE(testPropertyGetNodeExisting) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  PropertyNodePtr node1 = propSys.createProperty("/testing");

  ctx->evaluate<void>("var prop = Property.getNode('/testing');\n"
                      "prop.setValue('test');\n"
  );

  BOOST_CHECK_EQUAL(node1->getStringValue(), "test");
} // testPropertyGetNodeExisting

BOOST_AUTO_TEST_CASE(testPropertyGetChild) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  PropertyNodePtr node1 = propSys.createProperty("/testing/bla");

  ctx->evaluate<void>("var prop = Property.getNode('/testing');\n"
                      "prop.getChild('bla').setValue('test');\n"
  );

  BOOST_CHECK_EQUAL(node1->getStringValue(), "test");
} // testPropertyGetChild

BOOST_AUTO_TEST_CASE(testPropertyStoreReturnsFalse) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  bool res = ctx->evaluate<bool>("Property.store()");

  BOOST_CHECK_EQUAL(res, false);
} // testPropertyStoreReturnsFalse

BOOST_AUTO_TEST_CASE(testPropertyLoadReturnsFalse) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  bool res = ctx->evaluate<bool>("Property.load()");

  BOOST_CHECK_EQUAL(res, false);
} // testPropertyLoadReturnsFalse

BOOST_AUTO_TEST_CASE(testPropertyRemoveChild) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/test/subnode', 'test');");

  BOOST_CHECK(propSys.getProperty("/test/subnode") != NULL);

  ctx->evaluate<void>("var node = Property.getNode('/test');\n"
                      "node.removeChild('subnode');");

  BOOST_CHECK(propSys.getProperty("/test/subnode") == NULL);
} // testPropertyRemoveChild

BOOST_AUTO_TEST_CASE(testPropertyRemoveChildNonexisting) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/test', 'test');");

  ctx->evaluate<void>("var node = Property.getNode('/test');\n"
                      "node.removeChild('asdf');");
} // testPropertyRemoveChildNonexisting

BOOST_AUTO_TEST_CASE(testPropertyRemoveChildUndefined) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/test', 'test');");

  ctx->evaluate<void>("var node = Property.getNode('/test');\n"
                      "var nonexistingNode = node.getChild('testing');\n"
                      "node.removeChild(nonexistingNode);");
} // testPropertyRemoveChildUndefined

BOOST_AUTO_TEST_CASE(testPropertyRemoveChildNull) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/test', 'test');");

  ctx->evaluate<void>("var node = Property.getNode('/test');\n"
                      "node.removeChild(null);");
} // testPropertyRemoveChildNull

BOOST_AUTO_TEST_CASE(testPropertyGetParent) {
  PropertySystem propSys;
  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new PropertyScriptExtension(propSys);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Property.setProperty('/test/subnode', 'test');");

  BOOST_CHECK(propSys.getProperty("/test/subnode") != NULL);

  ctx->evaluate<void>("var subnode = Property.getNode('/test/subnode');\n"
                      "subnode.getParent().removeChild(subnode);");

  BOOST_CHECK(propSys.getProperty("/test/subnode") == NULL);
} // testPropertyGetParent

BOOST_AUTO_TEST_CASE(testMeteringGetSeries) {
  Apartment apt(NULL);
  apt.allocateDSMeter(dss_dsid_t(0,13));
  Metering metering(NULL);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new MeteringScriptExtension(apt, metering);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  int num = ctx->evaluate<int>("Metering.getSeries().length");
  BOOST_CHECK_EQUAL(num, 2);
} // testPropertyGetSeries

BOOST_AUTO_TEST_CASE(testMeteringGetResolutions) {
  Apartment apt(NULL);
  apt.allocateDSMeter(dss_dsid_t(0,13));
  Metering metering(NULL);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new MeteringScriptExtension(apt, metering);
  env->addExtension(ext);

  boost::shared_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("var energyMinR = 2^31; var consumptionMinR = 2^31;"
    "var mSeries = Metering.getResolutions();"
    "for (var index in mSeries) {"
      "var object = mSeries[index];"
      "if ((object.type == 'energy') && (energyMinR > object.resolution)) {"
        "energyMinR = object.resolution;"
      "}"
      "if ((object.type == 'consumption') && (consumptionMinR > object.resolution)) {"
        "consumptionMinR = object.resolution;"
      "}"
    "}"
    "print('Energy Min Resolution: ', energyMinR, ', Consumption Min Resolution: ', consumptionMinR);"
  );
  {
    JSContextThread thread(ctx);
    BOOST_CHECK_LT(ctx->getRootObject().getProperty<int>("energyMinR"), LONG_MAX);
    BOOST_CHECK_LT(ctx->getRootObject().getProperty<int>("consumptionMinR"), LONG_MAX);
  }
} // testPropertyGetResolutions

BOOST_AUTO_TEST_CASE(testMeteringGetValues) {
  Apartment apt(NULL);
  apt.allocateDSMeter(dss_dsid_t(0,0x13));
  Metering metering(NULL);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new MeteringScriptExtension(apt, metering);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  int num = ctx->evaluate<int>("Metering.getValues('13',"
                               "                   'consumption',"
                               "                   2).length");
  BOOST_CHECK_LE(num, 400);
  BOOST_CHECK_GE(num, 0);
} // testPropertyGetValues

BOOST_AUTO_TEST_CASE(testApartmentGetDSMeters) {
  Apartment apt(NULL);
  apt.allocateDSMeter(dss_dsid_t(0,0x13));
  Metering metering(NULL);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new ModelScriptContextExtension(apt);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  int num = ctx->evaluate<int>("Apartment.getDSMeters().length");
  BOOST_CHECK_EQUAL(num, 1);
} // testApartmentGetDSMeters

BOOST_AUTO_TEST_SUITE_END()
