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

#include "core/model.h"
#include "core/setbuilder.h"
#include "core/ds485const.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(Model)

BOOST_AUTO_TEST_CASE(testZoneMoving) {
  Apartment apt(NULL);
  apt.initialize();

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  DeviceReference devRef1(dev1, apt);
  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  DeviceReference devRef2(dev2, apt);
  Device& dev3 = apt.allocateDevice(dsid_t(0,3));
  dev3.setShortAddress(3);
  DeviceReference devRef3(dev3, apt);
  Device& dev4 = apt.allocateDevice(dsid_t(0,4));
  dev4.setShortAddress(4);
  DeviceReference devRef4(dev4, apt);

  Zone& zone1 = apt.allocateZone(1);
  zone1.addDevice(devRef1);
  zone1.addDevice(devRef2);
  zone1.addDevice(devRef3);
  zone1.addDevice(devRef4);

  Set allDevices = apt.getDevices();

  BOOST_CHECK_EQUAL(dev1, allDevices.getByBusID(1).getDevice());
  BOOST_CHECK_EQUAL(dev2, allDevices.getByBusID(2).getDevice());
  BOOST_CHECK_EQUAL(dev3, allDevices.getByBusID(3).getDevice());
  BOOST_CHECK_EQUAL(dev4, allDevices.getByBusID(4).getDevice());

  BOOST_CHECK_EQUAL(4, zone1.getDevices().length());

  Zone& zone2 = apt.allocateZone(2);
  zone2.addDevice(devRef2);
  zone2.addDevice(devRef4);

  BOOST_CHECK_EQUAL(2, zone1.getDevices().length());
  BOOST_CHECK_EQUAL(2, zone2.getDevices().length());

  // check that the devices are moved correctly in the zones datamodel
  Set zone1Devices = zone1.getDevices();
  BOOST_CHECK_EQUAL(dev1, zone1Devices.getByBusID(1).getDevice());
  BOOST_CHECK_EQUAL(dev3, zone1Devices.getByBusID(3).getDevice());

  Set zone2Devices = zone2.getDevices();
  BOOST_CHECK_EQUAL(dev2, zone2Devices.getByBusID(2).getDevice());
  BOOST_CHECK_EQUAL(dev4, zone2Devices.getByBusID(4).getDevice());

  // check the datamodel of the devices
  BOOST_CHECK_EQUAL(1, dev1.getZoneID());
  BOOST_CHECK_EQUAL(1, dev3.getZoneID());

  BOOST_CHECK_EQUAL(2, dev2.getZoneID());
  BOOST_CHECK_EQUAL(2, dev4.getZoneID());

  // check that the groups are set up correctly
  // (this should be the case if all test above passed)
  Set zone1Group0Devices = zone1.getGroup(GroupIDBroadcast)->getDevices();
  BOOST_CHECK_EQUAL(dev1, zone1Group0Devices.getByBusID(1).getDevice());
  BOOST_CHECK_EQUAL(dev3, zone1Group0Devices.getByBusID(3).getDevice());

  Set zone2Group0Devices = zone2.getGroup(GroupIDBroadcast)->getDevices();
  BOOST_CHECK_EQUAL(dev2, zone2Group0Devices.getByBusID(2).getDevice());
  BOOST_CHECK_EQUAL(dev4, zone2Group0Devices.getByBusID(4).getDevice());
} // testZoneMoving

BOOST_AUTO_TEST_CASE(testSet) {
  Apartment apt(NULL);
  apt.initialize();

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  Device& dev3 = apt.allocateDevice(dsid_t(0,3));
  dev3.setShortAddress(3);
  Device& dev4 = apt.allocateDevice(dsid_t(0,4));
  dev4.setShortAddress(4);

  Set allDevices = apt.getDevices();

  BOOST_CHECK_EQUAL(dev1, allDevices.getByBusID(1).getDevice());
  BOOST_CHECK_EQUAL(dev2, allDevices.getByBusID(2).getDevice());
  BOOST_CHECK_EQUAL(dev3, allDevices.getByBusID(3).getDevice());
  BOOST_CHECK_EQUAL(dev4, allDevices.getByBusID(4).getDevice());

  Set setdev1 = Set(dev1);

  Set allMinusDev1 = allDevices.remove(setdev1);

  BOOST_CHECK_EQUAL(3, allMinusDev1.length());

  BOOST_CHECK_EQUAL(false, allMinusDev1.contains(dev1));

  /*
  try {
    allMinusDev1.getByBusID(1);
    BOOST_CHECK(false);
  } catch(ItemNotFoundException& e) {
    BOOST_CHECK(true);
  }*/

  BOOST_CHECK_EQUAL(dev2, allMinusDev1.getByBusID(2).getDevice());
  BOOST_CHECK_EQUAL(dev3, allMinusDev1.getByBusID(3).getDevice());
  BOOST_CHECK_EQUAL(dev4, allMinusDev1.getByBusID(4).getDevice());

  // check that the other sets are not afected by our operation
  BOOST_CHECK_EQUAL(1, setdev1.length());
  BOOST_CHECK_EQUAL(dev1, setdev1.getByBusID(1).getDevice());

  BOOST_CHECK_EQUAL(4, allDevices.length());
  BOOST_CHECK_EQUAL(dev1, allDevices.getByBusID(1).getDevice());
  BOOST_CHECK_EQUAL(dev2, allDevices.getByBusID(2).getDevice());
  BOOST_CHECK_EQUAL(dev3, allDevices.getByBusID(3).getDevice());
  BOOST_CHECK_EQUAL(dev4, allDevices.getByBusID(4).getDevice());

  Set allRecombined = allMinusDev1.combine(setdev1);

  BOOST_CHECK_EQUAL(4, allRecombined.length());
  BOOST_CHECK_EQUAL(dev1, allRecombined.getByBusID(1).getDevice());
  BOOST_CHECK_EQUAL(dev2, allRecombined.getByBusID(2).getDevice());
  BOOST_CHECK_EQUAL(dev3, allRecombined.getByBusID(3).getDevice());
  BOOST_CHECK_EQUAL(dev4, allRecombined.getByBusID(4).getDevice());

  BOOST_CHECK_EQUAL(1, setdev1.length());
  BOOST_CHECK_EQUAL(dev1, setdev1.getByBusID(1).getDevice());

  allRecombined = allRecombined.combine(setdev1);
  BOOST_CHECK_EQUAL(4, allRecombined.length());

  allRecombined = allRecombined.combine(setdev1);
  BOOST_CHECK_EQUAL(4, allRecombined.length());

  allRecombined = allRecombined.combine(allRecombined);
  BOOST_CHECK_EQUAL(4, allRecombined.length());
  BOOST_CHECK_EQUAL(dev1, allRecombined.getByBusID(1).getDevice());
  BOOST_CHECK_EQUAL(dev2, allRecombined.getByBusID(2).getDevice());
  BOOST_CHECK_EQUAL(dev3, allRecombined.getByBusID(3).getDevice());
  BOOST_CHECK_EQUAL(dev4, allRecombined.getByBusID(4).getDevice());

  allMinusDev1 = allRecombined;
  allMinusDev1.removeDevice(dev1);
  BOOST_CHECK_EQUAL(3, allMinusDev1.length());
  BOOST_CHECK_EQUAL(dev2, allMinusDev1.getByBusID(2).getDevice());
  BOOST_CHECK_EQUAL(dev3, allMinusDev1.getByBusID(3).getDevice());
  BOOST_CHECK_EQUAL(dev4, allMinusDev1.getByBusID(4).getDevice());
} // testSet

BOOST_AUTO_TEST_CASE(testSetBuilder) {
  Apartment apt(NULL);
  apt.initialize();

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
} // testSetBuilder

BOOST_AUTO_TEST_CASE(testRemoval) {
  Apartment apt(NULL);
  apt.initialize();

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
  
  apt.allocateModulator(dsid_t(1,0));
  try {
    apt.getModulatorByDSID(dsid_t(1,0));
    BOOST_CHECK(true);
  } catch(ItemNotFoundException&) {
    BOOST_CHECK_MESSAGE(false, "Modulator not found");
  }
  
  apt.removeModulator(dsid_t(1,0));
  try {
    apt.getModulatorByDSID(dsid_t(1,0));
    BOOST_CHECK_MESSAGE(false, "Modulator still exists");
  } catch(ItemNotFoundException&) {
    BOOST_CHECK(true);
  }
} // testRemoval

BOOST_AUTO_TEST_SUITE_END()
