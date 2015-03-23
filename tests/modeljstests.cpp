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

#include <boost/scoped_ptr.hpp>
#include <memory>

#include "event.h"
#include "eventinterpreterplugins.h"
#include "propertysystem.h"
#include "metering/metering.h"
#include "model/apartment.h"
#include "model/device.h"
#include "model/modulator.h"
#include "model/modelconst.h"
#include "model/zone.h"
#include "model/group.h"
#include "scripting/jsmodel.h"
#include "scripting/jsevent.h"
#include "scripting/jsmetering.h"
#include "scripting/scriptobject.h"

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

  dsuid_t dsuid1, dsuid2, dsuid3;
  SetNullDsuid(dsuid1);
  SetNullDsuid(dsuid2);
  SetNullDsuid(dsuid3);
  dsuid1.id[DSUID_SIZE - 1] = 10;
  dsuid2.id[DSUID_SIZE - 1] = 1;
  dsuid3.id[DSUID_SIZE - 1] = 2;

  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dsuid1);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid2);
  dev1->setShortAddress(1);
  dev1->setDSMeter(meter);
  dev1->addToGroup(1);
  dev1->setIsPresent(true);
  dev1->setIsConnected(true);
  dev1->setZoneID(1);
  dev1->setName("dev1");
  dev1->setFunctionID(1);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid3);
  dev2->setShortAddress(2);
  dev2->setDSMeter(meter);
  dev2->addToGroup(1);
  dev2->setIsPresent(false);
  dev2->setIsConnected(true);
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

  length = ctx->evaluate<int>("getDevices().byDSMeter('"+dsuid2str(dsuid1)+"').length()");
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

  name = ctx->evaluate<std::string>("getDevices().byDSID('"+dsuid2str(dsuid3)+"').name");
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

  dsuid_t dsuid1, dsuid2, dsuid3;
  dsuid1.id[DSUID_SIZE - 1] = 10;
  dsuid2.id[DSUID_SIZE - 1] = 1;
  dsuid3.id[DSUID_SIZE - 1] = 2;
  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dsuid1);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid2);
  dev1->addTag("dev1");
  dev1->addTag("device");
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid3);
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

  dsuid_t dsuid1, dsuid2;
  dsuid1.id[DSUID_SIZE - 1] = 1;
  dsuid2.id[DSUID_SIZE - 1] = 2;

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
  dev1->setShortAddress(1);
  dev1->setName("dev1");
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid2);
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

  dsuid_t dsuid1, dsuid2;
  SetNullDsuid(dsuid1);
  SetNullDsuid(dsuid2);
  dsuid1.id[DSUID_SIZE - 1] = 0xa;
  dsuid2.id[DSUID_SIZE - 1] = 0xb;

  boost::shared_ptr<DSMeter> meter1 = apt.allocateDSMeter(dsuid1);
  meter1->setName("meter1");
  boost::shared_ptr<DSMeter> meter2 = apt.allocateDSMeter(dsuid2);
  meter2->setName("meter2");

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ModelScriptContextExtension* ext = new ModelScriptContextExtension(apt);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  std::string name = ctx->evaluate<std::string>("getDSMeterByDSID('"+dsuid2str(dsuid1)+"').name");
  BOOST_CHECK_EQUAL(name, meter1->getName());

  name = ctx->evaluate<std::string>("getDSMeterByDSID('"+dsuid2str(dsuid2)+"').name");
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

  dsuid_t dsuid1;
  dsuid1.id[DSUID_SIZE - 1] = 1;

  boost::shared_ptr<Device> dev = apt.allocateDevice(dsuid1);
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

  dsuid_t dsuid1;
  dsuid1.id[DSUID_SIZE - 1] = 1;

  boost::shared_ptr<Device> dev = apt.allocateDevice(dsuid1);
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

BOOST_AUTO_TEST_CASE(testMeteringGetSeries) {
  Apartment apt(NULL);

  dsuid_t dsuid1;
  dsuid1.id[DSUID_SIZE - 1] = 13;

  apt.allocateDSMeter(dsuid1);
  Metering metering(NULL);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new MeteringScriptExtension(apt, metering);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  int num = ctx->evaluate<int>("Metering.getSeries().length");
  BOOST_CHECK_EQUAL(num, 3);
} // testPropertyGetSeries

BOOST_AUTO_TEST_CASE(testMeteringGetResolutions) {
  Apartment apt(NULL);
  
  dsuid_t dsuid1;
  dsuid1.id[DSUID_SIZE - 1] = 13;

  apt.allocateDSMeter(dsuid1);
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

  dsuid_t dsuid1;
  SetNullDsuid(dsuid1);
  dsuid1.id[DSUID_SIZE - 1] = 13;

  apt.allocateDSMeter(dsuid1);
  apt.getDSMeterByDSID(dsuid1)->setCapability_HasMetering(true);
  Metering metering(NULL);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new MeteringScriptExtension(apt, metering);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  int num = ctx->evaluate<int>("Metering.getValues('"+dsuid2str(dsuid1)+"',"
                               "                   'consumption',"
                               "                   1).length");
  BOOST_CHECK_EQUAL(num, 599);
} // testPropertyGetValues

BOOST_AUTO_TEST_CASE(testMeteringGetValuesWs) {
  Apartment apt(NULL);

  dsuid_t dsuid1;
  SetNullDsuid(dsuid1);
  dsuid1.id[DSUID_SIZE - 1] = 13;

  apt.allocateDSMeter(dsuid1);
  apt.getDSMeterByDSID(dsuid1)->setCapability_HasMetering(true);
  Metering metering(NULL);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new MeteringScriptExtension(apt, metering);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  int num = ctx->evaluate<int>("Metering.getValues('"+dsuid2str(dsuid1)+"',"
                               "                   'energy',"
                               "                   1,"
                               "                   'Ws').length");
  BOOST_CHECK_EQUAL(num, 599);
} // testPropertyGetValues

BOOST_AUTO_TEST_CASE(testMeteringGetValuesCount) {
  Apartment apt(NULL);

  dsuid_t dsuid1;
  SetNullDsuid(dsuid1);
  dsuid1.id[DSUID_SIZE - 1] = 13;

  apt.allocateDSMeter(dsuid1);
  apt.getDSMeterByDSID(dsuid1)->setCapability_HasMetering(true);
  Metering metering(NULL);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new MeteringScriptExtension(apt, metering);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  int num = ctx->evaluate<int>("Metering.getValues('"+dsuid2str(dsuid1)+"',"
                               "                   'energy',"
                               "                   1,"
                               "                   'Wh',"
                               "                   0,"
                               "                   0,"
                               "                   10).length");
  BOOST_CHECK_EQUAL(num, 9);
} // testPropertyGetValues

BOOST_AUTO_TEST_CASE(testApartmentGetDSMeters) {
  Apartment apt(NULL);
  
  dsuid_t dsuid1;
  SetNullDsuid(dsuid1);
  dsuid1.id[DSUID_SIZE - 1] = 13;

  apt.allocateDSMeter(dsuid1);
  apt.getDSMeterByDSID(dsuid1)->setCapability_HasMetering(true);
  Metering metering(NULL);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new ModelScriptContextExtension(apt);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  int num = ctx->evaluate<int>("Apartment.getDSMeters().length");
  BOOST_CHECK_EQUAL(num, 1);
} // testApartmentGetDSMeters

BOOST_AUTO_TEST_CASE(testConnectedDevice) {
  Apartment apt(NULL);
  PropertySystem propSys;
  apt.setPropertySystem(&propSys);
  apt.allocateZone(42);
  apt.allocateZone(43);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new ModelScriptContextExtension(apt);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());

  BOOST_CHECK_EQUAL(apt.getZone(42)->getGroup(4)->hasConnectedDevices(), false);
  PropertyNodePtr connectedNode = propSys.getProperty("/apartment/zones/zone42/groups/group4/connectedDevices");
  BOOST_CHECK(connectedNode);
  BOOST_CHECK_EQUAL(connectedNode->getIntegerValue(), 0);

  ctx->evaluate<void>("getZoneByID(42).addConnectedDevice(4)");

  BOOST_CHECK_EQUAL(apt.getZone(42)->getGroup(4)->hasConnectedDevices(), true);
  BOOST_CHECK_EQUAL(connectedNode->getIntegerValue(), 1);

  ctx->evaluate<void>("getZoneByID(43).removeConnectedDevice(4)");

  BOOST_CHECK_EQUAL(apt.getZone(42)->getGroup(4)->hasConnectedDevices(), true);
  BOOST_CHECK_EQUAL(connectedNode->getIntegerValue(), 1);

  ctx->evaluate<void>("getZoneByID(42).removeConnectedDevice(4)");

  BOOST_CHECK_EQUAL(apt.getZone(42)->getGroup(4)->hasConnectedDevices(), false);
  BOOST_CHECK_EQUAL(connectedNode->getIntegerValue(), 0);
} // testConnectedDevice

BOOST_AUTO_TEST_CASE(testStateSensors) {
  Apartment apt(NULL);
  PropertySystem propSys;
  apt.setPropertySystem(&propSys);
  apt.allocateZone(42);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();
  ScriptExtension* ext = new ModelScriptContextExtension(apt);
  env->addExtension(ext);
  boost::scoped_ptr<ScriptContext> ctx(env->getContext());

  std::string id = ctx->evaluate<std::string>(
      "var z = getZoneByID(42);\n"
      "var s = z.addStateSensor(0, 9, \"> ; 24.99\", \"< ; 20.0\");\n"
      "print(s);\n");

  /*
   * Pushing a sensor value is not supported by our testing environment:
   * the ActionInterface requires a valid apartment object, which is not
   * supported by this test mockup.
   * Alternatively set the new current value directly and use the newValue
   * method of the state object.
   */

  boost::shared_ptr<State> pState = apt.getState(StateType_SensorZone, "zone.zone42.group0.type9");
  BOOST_CHECK(pState != boost::shared_ptr<State> ());
  BOOST_CHECK_EQUAL(pState->getState(), 2);

  boost::shared_ptr<StateSensor> pSensor = boost::dynamic_pointer_cast <StateSensor> (pState);
  pSensor->newValue(coSystem, 25.0);
  BOOST_CHECK_EQUAL(pState->getState(), 1);
  pSensor->newValue(coSystem, 23.0);
  BOOST_CHECK_EQUAL(pState->getState(), 1);
  pSensor->newValue(coSystem, 20.0);
  BOOST_CHECK_EQUAL(pState->getState(), 1);
  pSensor->newValue(coSystem, 19.99);
  BOOST_CHECK_EQUAL(pState->getState(), 2);
  pSensor->newValue(coSystem, 20.0);
  BOOST_CHECK_EQUAL(pState->getState(), 2);
  pSensor->newValue(coSystem, 23.0);
  BOOST_CHECK_EQUAL(pState->getState(), 2);
  pSensor->newValue(coSystem, 25.0);
  BOOST_CHECK_EQUAL(pState->getState(), 1);
} // testStates

BOOST_AUTO_TEST_SUITE_END()
