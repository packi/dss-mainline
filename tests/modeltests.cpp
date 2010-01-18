/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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

#include "core/model/device.h"
#include "core/model/apartment.h"
#include "core/model/modulator.h"
#include "core/model/devicereference.h"
#include "core/model/zone.h"
#include "core/model/set.h"
#include "core/setbuilder.h"
#include "core/sim/dssim.h"
#include "core/dss.h"
#include "core/ds485const.h"
#include "core/ds485/ds485proxy.h"
#include "core/ds485/ds485busrequestdispatcher.h"
#include "core/ds485/businterfacehandler.h"
#include "core/model/modelmaintenance.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(Model)

BOOST_AUTO_TEST_CASE(testApartmentAllocateDeviceReturnsTheSameDeviceForDSID) {
  Apartment apt(NULL);

  DSMeter& meter = apt.allocateDSMeter(dsid_t(0,10));
  meter.setBusID(1);

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.setName("dev1");
  dev1.setDSMeter(meter);

  Device& dev2 = apt.allocateDevice(dsid_t(0,1));
  BOOST_CHECK_EQUAL(dev1.getShortAddress(), dev2.getShortAddress());
  BOOST_CHECK_EQUAL(dev1.getName(), dev2.getName());
  BOOST_CHECK_EQUAL(dev1.getDSMeterID(), dev2.getDSMeterID());
} // testApartmentAllocateDeviceReturnsTheSameDeviceForDSID

BOOST_AUTO_TEST_CASE(testSetGetByBusID) {
  Apartment apt(NULL);

  DSMeter& meter1 = apt.allocateDSMeter(dsid_t(0,10));
  meter1.setBusID(1);
  DSMeter& meter2 = apt.allocateDSMeter(dsid_t(0,11));
  meter2.setBusID(2);

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.setName("dev1");
  dev1.setDSMeter(meter1);

  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(1);
  dev2.setName("dev2");
  dev2.setDSMeter(meter2);

  DSMeter& mod1 = apt.allocateDSMeter(dsid_t(0,3));
  mod1.setBusID(1);

  DSMeter& mod2 = apt.allocateDSMeter(dsid_t(0,4));
  mod2.setBusID(2);
  BOOST_CHECK_EQUAL(apt.getDevices().getByBusID(1, 1).getName(), dev1.getName());
  BOOST_CHECK_THROW(apt.getDevices().getByBusID(2, 1), ItemNotFoundException);

  BOOST_CHECK_EQUAL(apt.getDevices().getByBusID(1, mod1).getName(), dev1.getName());
  BOOST_CHECK_THROW(apt.getDevices().getByBusID(2, mod1), ItemNotFoundException);

  BOOST_CHECK_EQUAL(apt.getDevices().getByBusID(1, 2).getName(), dev2.getName());
  BOOST_CHECK_THROW(apt.getDevices().getByBusID(2, 2), ItemNotFoundException);

  BOOST_CHECK_EQUAL(apt.getDevices().getByBusID(1, mod2).getName(), dev2.getName());
  BOOST_CHECK_THROW(apt.getDevices().getByBusID(2, mod2), ItemNotFoundException);
} // testSetGetByBusID

BOOST_AUTO_TEST_CASE(testSetRemoveDevice) {
  Apartment apt(NULL);

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.setName("dev1");

  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  dev2.setName("dev2");
  
  Set set;
  set.addDevice(dev1);
  set.addDevice(dev2);
  
  BOOST_CHECK_EQUAL(set.length(), 2);
  BOOST_CHECK_EQUAL(set.get(0).getDevice(), dev1);
  BOOST_CHECK_EQUAL(set.get(1).getDevice(), dev2);
  
  set.removeDevice(dev1);
  
  BOOST_CHECK_EQUAL(set.length(), 1);
  BOOST_CHECK_EQUAL(set.get(0).getDevice(), dev2);
} // testSetRemoveDevice

BOOST_AUTO_TEST_CASE(testDeviceLastKnownDSMeterDSIDWorks) {
  Apartment apt(NULL);

  DSMeter& mod = apt.allocateDSMeter(dsid_t(0,10));
  mod.setBusID(1);

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));

  BOOST_CHECK_EQUAL(dev1.getLastKnownDSMeterDSID().toString(), NullDSID.toString());

  dev1.setDSMeter(mod);

  BOOST_CHECK_EQUAL(dev1.getDSMeterID(), 1);
  BOOST_CHECK_EQUAL(dev1.getLastKnownDSMeterDSID().toString(), dsid_t(0,10).toString());
} // testDeviceLastKnownDSMeterDSIDWorks

BOOST_AUTO_TEST_CASE(testApartmentGetDeviceByShortAddress) {
  Apartment apt(NULL);

  DSMeter& mod = apt.allocateDSMeter(dsid_t(0,2));
  mod.setBusID(1);

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.setName("dev1");
  dev1.setDSMeter(mod);

  BOOST_CHECK_EQUAL("dev1", apt.getDeviceByShortAddress(mod, 1).getName());
} // testApartmentGetDeviceByShortAddress

BOOST_AUTO_TEST_CASE(testApartmentGetDeviceByName) {
  Apartment apt(NULL);

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setName("dev1");

  BOOST_CHECK_EQUAL("dev1", apt.getDeviceByName("dev1").getName());
} // testApartmentGetDeviceByName

BOOST_AUTO_TEST_CASE(testApartmentGetDSMeterByName) {
  Apartment apt(NULL);

  DSMeter& mod = apt.allocateDSMeter(dsid_t(0,2));
  mod.setName("mod1");

  BOOST_CHECK_EQUAL("mod1", apt.getDSMeter("mod1").getName());
} // testApartmentGetDSMeterByName

BOOST_AUTO_TEST_CASE(testApartmentGetDSMeterByBusID) {
  Apartment apt(NULL);

  DSMeter& mod = apt.allocateDSMeter(dsid_t(0,2));
  mod.setBusID(1);

  BOOST_CHECK_EQUAL(1, apt.getDSMeterByBusID(1).getBusID());
} // testApartmentGetDSMeterByBusID

BOOST_AUTO_TEST_CASE(testZoneMoving) {
  Apartment apt(NULL);

  DSMeter& meter = apt.allocateDSMeter(dsid_t(0,10));
  meter.setBusID(1);


  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.setDSMeter(meter);
  DeviceReference devRef1(dev1, &apt);
  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  dev2.setDSMeter(meter);
  DeviceReference devRef2(dev2, &apt);
  Device& dev3 = apt.allocateDevice(dsid_t(0,3));
  dev3.setShortAddress(3);
  dev3.setDSMeter(meter);
  DeviceReference devRef3(dev3, &apt);
  Device& dev4 = apt.allocateDevice(dsid_t(0,4));
  dev4.setShortAddress(4);
  dev4.setDSMeter(meter);
  DeviceReference devRef4(dev4, &apt);

  Zone& zone1 = apt.allocateZone(1);
  zone1.addDevice(devRef1);
  zone1.addDevice(devRef2);
  zone1.addDevice(devRef3);
  zone1.addDevice(devRef4);

  Set allDevices = apt.getDevices();

  BOOST_CHECK_EQUAL(dev1, allDevices.getByBusID(1, 1).getDevice());
  BOOST_CHECK_EQUAL(dev2, allDevices.getByBusID(2, 1).getDevice());
  BOOST_CHECK_EQUAL(dev3, allDevices.getByBusID(3, 1).getDevice());
  BOOST_CHECK_EQUAL(dev4, allDevices.getByBusID(4, 1).getDevice());

  BOOST_CHECK_EQUAL(4, zone1.getDevices().length());

  Zone& zone2 = apt.allocateZone(2);
  zone2.addDevice(devRef2);
  zone2.addDevice(devRef4);

  BOOST_CHECK_EQUAL(2, zone1.getDevices().length());
  BOOST_CHECK_EQUAL(2, zone2.getDevices().length());

  // check that the devices are moved correctly in the zones datamodel
  Set zone1Devices = zone1.getDevices();
  BOOST_CHECK_EQUAL(dev1, zone1Devices.getByBusID(1, 1).getDevice());
  BOOST_CHECK_EQUAL(dev3, zone1Devices.getByBusID(3, 1).getDevice());

  Set zone2Devices = zone2.getDevices();
  BOOST_CHECK_EQUAL(dev2, zone2Devices.getByBusID(2, 1).getDevice());
  BOOST_CHECK_EQUAL(dev4, zone2Devices.getByBusID(4, 1).getDevice());

  // check the datamodel of the devices
  BOOST_CHECK_EQUAL(1, dev1.getZoneID());
  BOOST_CHECK_EQUAL(1, dev3.getZoneID());

  BOOST_CHECK_EQUAL(2, dev2.getZoneID());
  BOOST_CHECK_EQUAL(2, dev4.getZoneID());

  // check that the groups are set up correctly
  // (this should be the case if all test above passed)
  Set zone1Group0Devices = zone1.getGroup(GroupIDBroadcast)->getDevices();
  BOOST_CHECK_EQUAL(dev1, zone1Group0Devices.getByBusID(1, 1).getDevice());
  BOOST_CHECK_EQUAL(dev3, zone1Group0Devices.getByBusID(3, 1).getDevice());

  Set zone2Group0Devices = zone2.getGroup(GroupIDBroadcast)->getDevices();
  BOOST_CHECK_EQUAL(dev2, zone2Group0Devices.getByBusID(2, 1).getDevice());
  BOOST_CHECK_EQUAL(dev4, zone2Group0Devices.getByBusID(4, 1).getDevice());
} // testZoneMoving

BOOST_AUTO_TEST_CASE(testSet) {
  Apartment apt(NULL);

  DSMeter& meter = apt.allocateDSMeter(dsid_t(0,10));
  meter.setBusID(1);

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.setDSMeter(meter);
  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  dev2.setDSMeter(meter);
  Device& dev3 = apt.allocateDevice(dsid_t(0,3));
  dev3.setShortAddress(3);
  dev3.setDSMeter(meter);
  Device& dev4 = apt.allocateDevice(dsid_t(0,4));
  dev4.setShortAddress(4);
  dev4.setDSMeter(meter);

  Set allDevices = apt.getDevices();

  BOOST_CHECK_EQUAL(dev1, allDevices.getByBusID(1, 1).getDevice());
  BOOST_CHECK_EQUAL(dev2, allDevices.getByBusID(2, 1).getDevice());
  BOOST_CHECK_EQUAL(dev3, allDevices.getByBusID(3, 1).getDevice());
  BOOST_CHECK_EQUAL(dev4, allDevices.getByBusID(4, 1).getDevice());

  Set setdev1 = Set(dev1);

  Set allMinusDev1 = allDevices.remove(setdev1);

  BOOST_CHECK_EQUAL(3, allMinusDev1.length());

  BOOST_CHECK_EQUAL(false, allMinusDev1.contains(dev1));

  BOOST_CHECK_EQUAL(dev2, allMinusDev1.getByBusID(2, 1).getDevice());
  BOOST_CHECK_EQUAL(dev3, allMinusDev1.getByBusID(3, 1).getDevice());
  BOOST_CHECK_EQUAL(dev4, allMinusDev1.getByBusID(4, 1).getDevice());

  // check that the other sets are not afected by our operation
  BOOST_CHECK_EQUAL(1, setdev1.length());
  BOOST_CHECK_EQUAL(dev1, setdev1.getByBusID(1, 1).getDevice());

  BOOST_CHECK_EQUAL(4, allDevices.length());
  BOOST_CHECK_EQUAL(dev1, allDevices.getByBusID(1, 1).getDevice());
  BOOST_CHECK_EQUAL(dev2, allDevices.getByBusID(2, 1).getDevice());
  BOOST_CHECK_EQUAL(dev3, allDevices.getByBusID(3, 1).getDevice());
  BOOST_CHECK_EQUAL(dev4, allDevices.getByBusID(4, 1).getDevice());

  Set allRecombined = allMinusDev1.combine(setdev1);

  BOOST_CHECK_EQUAL(4, allRecombined.length());
  BOOST_CHECK_EQUAL(dev1, allRecombined.getByBusID(1, 1).getDevice());
  BOOST_CHECK_EQUAL(dev2, allRecombined.getByBusID(2, 1).getDevice());
  BOOST_CHECK_EQUAL(dev3, allRecombined.getByBusID(3, 1).getDevice());
  BOOST_CHECK_EQUAL(dev4, allRecombined.getByBusID(4, 1).getDevice());

  BOOST_CHECK_EQUAL(1, setdev1.length());
  BOOST_CHECK_EQUAL(dev1, setdev1.getByBusID(1, 1).getDevice());

  allRecombined = allRecombined.combine(setdev1);
  BOOST_CHECK_EQUAL(4, allRecombined.length());

  allRecombined = allRecombined.combine(setdev1);
  BOOST_CHECK_EQUAL(4, allRecombined.length());

  allRecombined = allRecombined.combine(allRecombined);
  BOOST_CHECK_EQUAL(4, allRecombined.length());
  BOOST_CHECK_EQUAL(dev1, allRecombined.getByBusID(1, 1).getDevice());
  BOOST_CHECK_EQUAL(dev2, allRecombined.getByBusID(2, 1).getDevice());
  BOOST_CHECK_EQUAL(dev3, allRecombined.getByBusID(3, 1).getDevice());
  BOOST_CHECK_EQUAL(dev4, allRecombined.getByBusID(4, 1).getDevice());

  allMinusDev1 = allRecombined;
  allMinusDev1.removeDevice(dev1);
  BOOST_CHECK_EQUAL(3, allMinusDev1.length());
  BOOST_CHECK_EQUAL(dev2, allMinusDev1.getByBusID(2, 1).getDevice());
  BOOST_CHECK_EQUAL(dev3, allMinusDev1.getByBusID(3, 1).getDevice());
  BOOST_CHECK_EQUAL(dev4, allMinusDev1.getByBusID(4, 1).getDevice());
} // testSet

BOOST_AUTO_TEST_CASE(testSetBuilder) {
  Apartment apt(NULL);

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.getGroupBitmask().set(GroupIDYellow - 1);
  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  dev2.getGroupBitmask().set(GroupIDCyan - 1);
  Device& dev3 = apt.allocateDevice(dsid_t(0,3));
  dev3.setShortAddress(3);
  Device& dev4 = apt.allocateDevice(dsid_t(0,4));
  dev4.setShortAddress(4);

  SetBuilder builder(apt);
  Set builderTest = builder.buildSet("yellow", &apt.getZone(0));

  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("cyan", &apt.getZone(0));

  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev2, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid(1)", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow.dsid(1)", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid(1).yellow", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow.yellow.yellow.yellow.yellow.yellow.dsid(1)", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet(" yellow ", &apt.getZone(0));

  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("\t\n\rcyan", &apt.getZone(0));

  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev2, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid( 1)", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid( 1 )", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid(1 )", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet(" dsid(1 ) ", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow .dsid(1)", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow. dsid(1)", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow . dsid(1)", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid(1) .yellow", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid(1). yellow", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid(1) . yellow", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow .yellow. yellow . yellow  .yellow.  yellow \n.dsid(\t1)    ", &apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("group(0).remove(dsid(1))", &apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("remove(dsid(1))", &apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("remove(yellow)", &apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("group('broadcast')", &apt.getZone(0));
  BOOST_CHECK_EQUAL(4, builderTest.length());

  builderTest = builder.buildSet("group('broadcast').remove(dsid(1))", &apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("empty().addDevices(1,2,3)", &apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("addDevices(1)", NULL);
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("addDevices(1,2)", NULL);
  BOOST_CHECK_EQUAL(2, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());
  BOOST_CHECK_EQUAL(dev2, builderTest.get(1).getDevice());

  builderTest = builder.buildSet("addDevices(1,2,3)", NULL);
  BOOST_CHECK_EQUAL(3, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());
  BOOST_CHECK_EQUAL(dev2, builderTest.get(1).getDevice());
  BOOST_CHECK_EQUAL(dev3, builderTest.get(2).getDevice());
} // testSetBuilder

BOOST_AUTO_TEST_CASE(testRemoval) {
  Apartment apt(NULL);

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.getGroupBitmask().set(GroupIDYellow - 1);
  
  SetBuilder builder(apt);
  BOOST_CHECK_EQUAL(1, builder.buildSet(".yellow", NULL).length());
  
  apt.removeDevice(dev1.getDSID());
  BOOST_CHECK_EQUAL(0, builder.buildSet(".yellow", NULL).length()); 
  
  apt.allocateZone(1);
  try {
    apt.getZone(1);
    BOOST_CHECK(true);
  } catch(ItemNotFoundException&) {
    BOOST_CHECK_MESSAGE(false, "Zone does not exist");
  }
  
  apt.removeZone(1);
  try {
    apt.getZone(1);
    BOOST_CHECK_MESSAGE(false, "Zone still exists");
  } catch(ItemNotFoundException&) {
    BOOST_CHECK(true);
  }
  
  apt.allocateDSMeter(dsid_t(1,0));
  try {
    apt.getDSMeterByDSID(dsid_t(1,0));
    BOOST_CHECK(true);
  } catch(ItemNotFoundException&) {
    BOOST_CHECK_MESSAGE(false, "DSMeter not found");
  }
  
  apt.removeDSMeter(dsid_t(1,0));
  try {
    apt.getDSMeterByDSID(dsid_t(1,0));
    BOOST_CHECK_MESSAGE(false, "DSMeter still exists");
  } catch(ItemNotFoundException&) {
    BOOST_CHECK(true);
  }
} // testRemoval

BOOST_AUTO_TEST_CASE(testCallScenePropagation) {
  Apartment apt(NULL);

  DSDSMeterSim modSim(NULL);
  DS485BusRequestDispatcher dispatcher;
  ModelMaintenance maintenance(NULL);
  maintenance.setApartment(&apt);
  maintenance.initialize();
  DS485Proxy proxy(NULL, &maintenance);
  dispatcher.setFrameSender(proxy.getFrameSenderInterface());
  BusInterfaceHandler busHandler(NULL, maintenance);
  proxy.setBusInterfaceHandler(&busHandler);
  proxy.setInitializeDS485Controller(false);
  proxy.initialize();
  apt.setBusRequestDispatcher(&dispatcher);
  busHandler.initialize();

  busHandler.start();
  maintenance.start();
  while(maintenance.isInitializing()) {
    sleepMS(100);
  }

  DSMeter& mod = apt.allocateDSMeter(dsid_t(0,3));
  mod.setBusID(76);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setName("dev1");
  dev1.setShortAddress(1);
  dev1.setDSMeter(mod);
  DeviceReference devRef1(dev1, &apt);
  mod.addDevice(devRef1);
  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setName("dev2");
  dev2.setShortAddress(2);
  dev2.setDSMeter(mod);
  DeviceReference devRef2(dev2, &apt);
  mod.addDevice(devRef2);
  Device& dev3 = apt.allocateDevice(dsid_t(0,3));
  dev3.setName("dev3");
  dev3.setShortAddress(3);
  dev3.setDSMeter(mod);
  DeviceReference devRef3(dev3, &apt);
  mod.addDevice(devRef3);

  dev1.callScene(Scene1);
  sleepMS(500);
  BOOST_CHECK_EQUAL(Scene1, dev1.getLastCalledScene());
  apt.getZone(0).callScene(Scene2);
  sleepMS(500);
  BOOST_CHECK_EQUAL(Scene2, dev1.getLastCalledScene());
  BOOST_CHECK_EQUAL(Scene2, dev2.getLastCalledScene());
  
  Set set;
  set.addDevice(dev3);
  set.addDevice(dev2);
  set.callScene(Scene3);
  sleepMS(500);
  BOOST_CHECK_EQUAL(Scene2, dev1.getLastCalledScene());
  BOOST_CHECK_EQUAL(Scene3, dev2.getLastCalledScene());
  BOOST_CHECK_EQUAL(Scene3, dev3.getLastCalledScene());  
  busHandler.terminate();
  maintenance.terminate();
  sleepMS(1500);
  DSS::teardown();
} // testCallScenePropagation


class FrameSenderTester : public FrameSenderInterface {
public:
  int getLastFunctionID() const { return m_LastFunctionID; }
  void setLastFunctionID(const int _value) { m_LastFunctionID = _value; }
  int getParameter1() const { return m_Parameter1; }
  int getParameter2() const { return m_Parameter2; }
  int getParameter3() const { return m_Parameter3; }
  virtual void sendFrame(DS485CommandFrame& _frame) {
    PayloadDissector pd(_frame.getPayload());
    m_LastFunctionID = pd.get<uint8_t>();
    if(!pd.isEmpty()) {
      m_Parameter1 = pd.get<uint16_t>();
    } else {
      m_Parameter1 = 0xffff;
    }
    if(!pd.isEmpty()) {
      m_Parameter2 = pd.get<uint16_t>();
    } else {
      m_Parameter2 = 0xffff;
    }
    if(!pd.isEmpty()) {
      m_Parameter3 = pd.get<uint16_t>();
    } else {
      m_Parameter3 = 0xffff;
    }
  }
private:
  int m_LastFunctionID;
  uint16_t m_Parameter1;
  uint16_t m_Parameter2;
  uint16_t m_Parameter3;
}; // FrameSenderTester

class TestModelFixture {
public:
  TestModelFixture() {
    m_pApartment.reset(new Apartment(NULL));
    m_pFrameSender.reset(new FrameSenderTester());
    m_pDispatcher.reset(new DS485BusRequestDispatcher());
    m_pDispatcher->setFrameSender(m_pFrameSender.get());
    m_pApartment->setBusRequestDispatcher(m_pDispatcher.get());
    m_pFrameSender->setLastFunctionID(-1);
  }

  boost::scoped_ptr<Apartment> m_pApartment;
  boost::scoped_ptr<FrameSenderTester> m_pFrameSender;
  boost::scoped_ptr<DS485BusRequestDispatcher> m_pDispatcher;
};

BOOST_FIXTURE_TEST_CASE(testTurnOnDevice, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.turnOn();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  BOOST_CHECK_EQUAL(SceneMax, m_pFrameSender->getParameter2());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.turnOn();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  BOOST_CHECK_EQUAL(SceneMax, m_pFrameSender->getParameter2());
}

BOOST_FIXTURE_TEST_CASE(testTurnOnGroup, TestModelFixture) {
  Group& group = m_pApartment->getGroup(1);
  group.turnOn();
  BOOST_CHECK_EQUAL(FunctionGroupCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x0, m_pFrameSender->getParameter1()); // zone 0
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter2()); // group 1
  BOOST_CHECK_EQUAL(SceneMax, m_pFrameSender->getParameter3());
}

BOOST_FIXTURE_TEST_CASE(testTurnOffDevice, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.turnOff();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  BOOST_CHECK_EQUAL(SceneMin, m_pFrameSender->getParameter2());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.turnOff();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  BOOST_CHECK_EQUAL(SceneMin, m_pFrameSender->getParameter2());
}

BOOST_FIXTURE_TEST_CASE(testTurnOffGroup, TestModelFixture) {
  Group& group = m_pApartment->getGroup(1);
  group.turnOff();
  BOOST_CHECK_EQUAL(FunctionGroupCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x0, m_pFrameSender->getParameter1()); // zone 0
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter2()); // group 1
  BOOST_CHECK_EQUAL(SceneMin, m_pFrameSender->getParameter3());
}

BOOST_FIXTURE_TEST_CASE(testEnableDevice, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.enable();
  BOOST_CHECK_EQUAL(FunctionDeviceEnable, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.enable();
  BOOST_CHECK_EQUAL(FunctionDeviceEnable, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
}

BOOST_FIXTURE_TEST_CASE(testDisableDevice, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.disable();
  BOOST_CHECK_EQUAL(FunctionDeviceDisable, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.disable();
  BOOST_CHECK_EQUAL(FunctionDeviceDisable, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
}

BOOST_FIXTURE_TEST_CASE(testIncreaseValueDevice, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.increaseValue();
  BOOST_CHECK_EQUAL(FunctionDeviceIncreaseValue, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.increaseValue();
  BOOST_CHECK_EQUAL(FunctionDeviceIncreaseValue, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
}

BOOST_FIXTURE_TEST_CASE(testDecreaseValueDevice, TestModelFixture) {
  m_pFrameSender->setLastFunctionID(-1);
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.decreaseValue();
  BOOST_CHECK_EQUAL(FunctionDeviceDecreaseValue, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.decreaseValue();
  BOOST_CHECK_EQUAL(FunctionDeviceDecreaseValue, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
}

BOOST_FIXTURE_TEST_CASE(testDecreaseValueZone, TestModelFixture) {
  Zone& zone = m_pApartment->allocateZone(1);
  zone.decreaseValue();
  BOOST_CHECK_EQUAL(FunctionGroupDecreaseValue, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1()); // zone id
  BOOST_CHECK_EQUAL(0x0, m_pFrameSender->getParameter2()); // group id (broadcast)
}

BOOST_FIXTURE_TEST_CASE(testIncreaseValueZone, TestModelFixture) {
  Zone& zone = m_pApartment->allocateZone(1);
  zone.increaseValue();
  BOOST_CHECK_EQUAL(FunctionGroupIncreaseValue, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1()); // zone id
  BOOST_CHECK_EQUAL(0x0, m_pFrameSender->getParameter2()); // group id (broadcast)
}

BOOST_FIXTURE_TEST_CASE(testStartDimUp, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.startDim(true);
  BOOST_CHECK_EQUAL(FunctionDeviceStartDimInc, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.startDim(true);
  BOOST_CHECK_EQUAL(FunctionDeviceStartDimInc, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
}

BOOST_FIXTURE_TEST_CASE(testStartDimDown, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.startDim(false);
  BOOST_CHECK_EQUAL(FunctionDeviceStartDimDec, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.startDim(false);
  BOOST_CHECK_EQUAL(FunctionDeviceStartDimDec, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
}

BOOST_FIXTURE_TEST_CASE(testEndDim, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.endDim();
  BOOST_CHECK_EQUAL(FunctionDeviceEndDim, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.endDim();
  BOOST_CHECK_EQUAL(FunctionDeviceEndDim, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
}

BOOST_FIXTURE_TEST_CASE(testCallSceneDevice, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.callScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter2());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.callScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter2());
}

BOOST_FIXTURE_TEST_CASE(testSaveSceneDevice, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.saveScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceSaveScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter2());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.saveScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceSaveScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter2());
}

BOOST_FIXTURE_TEST_CASE(testUndoSceneDevice, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.undoScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceUndoScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter2());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.undoScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceUndoScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter2());
}

BOOST_FIXTURE_TEST_CASE(testCallSceneGroup, TestModelFixture) {
  Group& group = m_pApartment->getGroup(1);
  group.callScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionGroupCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x0, m_pFrameSender->getParameter1()); // zone 0
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter2()); // group 1
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter3());
}

BOOST_FIXTURE_TEST_CASE(testSaveSceneGroup, TestModelFixture) {
  Group& group = m_pApartment->getGroup(1);
  group.saveScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionGroupSaveScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x0, m_pFrameSender->getParameter1()); // zone 0
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter2()); // group 1
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter3());
}

BOOST_FIXTURE_TEST_CASE(testUndoSceneGroup, TestModelFixture) {
  Group& group = m_pApartment->getGroup(1);
  group.undoScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionGroupUndoScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x0, m_pFrameSender->getParameter1()); // zone 0
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter2()); // group 1
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter3());
}

BOOST_FIXTURE_TEST_CASE(testCallSceneZone, TestModelFixture) {
  Zone& zone = m_pApartment->allocateZone(1);
  zone.callScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionGroupCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1()); // zone 1
  BOOST_CHECK_EQUAL(0x0, m_pFrameSender->getParameter2()); // group 0
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter3());
}

BOOST_FIXTURE_TEST_CASE(testSaveSceneZone, TestModelFixture) {
  Zone& zone = m_pApartment->allocateZone(1);
  zone.saveScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionGroupSaveScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1()); // zone 1
  BOOST_CHECK_EQUAL(0x0, m_pFrameSender->getParameter2()); // group 0
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter3());
}

BOOST_FIXTURE_TEST_CASE(testUndoSceneZone, TestModelFixture) {
  Zone& zone = m_pApartment->allocateZone(1);
  zone.undoScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionGroupUndoScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1()); // zone 1
  BOOST_CHECK_EQUAL(0x0, m_pFrameSender->getParameter2()); // group 0
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter3());
}

BOOST_FIXTURE_TEST_CASE(callUndoSceneSet, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  Device& dev2 = m_pApartment->allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  Set set;
  set.addDevice(dev1);
  set.callScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter2());
  m_pFrameSender->setLastFunctionID(-1);
}

BOOST_FIXTURE_TEST_CASE(testSaveSceneSet, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  Device& dev2 = m_pApartment->allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  Set set;
  set.addDevice(dev1);
  set.saveScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceSaveScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter2());
  m_pFrameSender->setLastFunctionID(-1);
}

BOOST_FIXTURE_TEST_CASE(testUndoSceneSet, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  Device& dev2 = m_pApartment->allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  Set set;
  set.addDevice(dev1);
  set.undoScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceUndoScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  BOOST_CHECK_EQUAL(Scene1, m_pFrameSender->getParameter2());
  m_pFrameSender->setLastFunctionID(-1);
}

BOOST_FIXTURE_TEST_CASE(testNextSceneDevice, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.nextScene();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.nextScene();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
}

BOOST_FIXTURE_TEST_CASE(testPreviousSceneDevice, TestModelFixture) {
  Device& dev1 = m_pApartment->allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.previousScene();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
  m_pFrameSender->setLastFunctionID(-1);
  DeviceReference devRef1(dev1, m_pApartment.get());
  devRef1.previousScene();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, m_pFrameSender->getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, m_pFrameSender->getParameter1());
}

BOOST_AUTO_TEST_SUITE_END()
