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
#include "core/model/group.h"
#include "core/model/set.h"
#include "core/setbuilder.h"
#include "core/dss.h"
#include "core/model/modelconst.h"
#include "core/model/modelmaintenance.h"
#include "core/propertysystem.h"
#include "core/structuremanipulator.h"
#include "core/businterface.h"
#include "core/model/scenehelper.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(Model)

BOOST_AUTO_TEST_CASE(testApartmentAllocateDeviceReturnsTheSameDeviceForDSID) {
  Apartment apt(NULL);

  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dss_dsid_t(0,10));

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setShortAddress(1);
  dev1->setName("dev1");
  dev1->setDSMeter(meter);

  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,1));
  BOOST_CHECK_EQUAL(dev1->getShortAddress(), dev2->getShortAddress());
  BOOST_CHECK_EQUAL(dev1->getName(), dev2->getName());
  BOOST_CHECK_EQUAL(dev1->getDSMeterDSID().toString(), dev2->getDSMeterDSID().toString());
} // testApartmentAllocateDeviceReturnsTheSameDeviceForDSID

BOOST_AUTO_TEST_CASE(testSetGetByBusID) {
  Apartment apt(NULL);

  dss_dsid_t meter1DSID = dss_dsid_t(0,10);
  dss_dsid_t meter2DSID = dss_dsid_t(0,11);
  boost::shared_ptr<DSMeter> meter1 = apt.allocateDSMeter(meter1DSID);
  boost::shared_ptr<DSMeter> meter2 = apt.allocateDSMeter(meter2DSID);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setShortAddress(1);
  dev1->setName("dev1");
  dev1->setDSMeter(meter1);

  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->setShortAddress(1);
  dev2->setName("dev2");
  dev2->setDSMeter(meter2);

  BOOST_CHECK_EQUAL(apt.getDevices().getByBusID(1, meter1DSID).getName(), dev1->getName());
  BOOST_CHECK_THROW(apt.getDevices().getByBusID(2, meter1DSID), ItemNotFoundException);

  BOOST_CHECK_EQUAL(apt.getDevices().getByBusID(1, meter2DSID).getName(), dev2->getName());
  BOOST_CHECK_THROW(apt.getDevices().getByBusID(2, meter2DSID), ItemNotFoundException);
} // testSetGetByBusID

BOOST_AUTO_TEST_CASE(testSetRemoveDevice) {
  Apartment apt(NULL);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setShortAddress(1);
  dev1->setName("dev1");

  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->setShortAddress(2);
  dev2->setName("dev2");

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

BOOST_AUTO_TEST_CASE(testSetTags) {
  Apartment apt(NULL);
  PropertySystem propSys;
  apt.setPropertySystem(&propSys);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->addTag("dev1");
  dev1->addTag("device");

  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->addTag("dev2");
  dev2->addTag("device");

  Set set = apt.getDevices();

  Set subset = set.getByTag("dev1");
  BOOST_CHECK_EQUAL(subset.length(), 1);
  BOOST_CHECK_EQUAL(subset[0].getDevice(), dev1);

  subset = set.getByTag("dev2");
  BOOST_CHECK_EQUAL(subset.length(), 1);
  BOOST_CHECK_EQUAL(subset[0].getDevice(), dev2);

  subset = set.getByTag("device");
  BOOST_CHECK_EQUAL(subset.length(), 2);
  BOOST_CHECK_EQUAL(subset[0].getDevice(), dev1);
  BOOST_CHECK_EQUAL(subset[1].getDevice(), dev2);

  subset = set.getByTag("nonexisting");
  BOOST_CHECK_EQUAL(subset.length(), 0);
} // testSetTags

BOOST_AUTO_TEST_CASE(testDeviceLastKnownDSMeterDSIDWorks) {
  Apartment apt(NULL);

  dss_dsid_t dsmeterDSID = dss_dsid_t(0,10);
  boost::shared_ptr<DSMeter> mod = apt.allocateDSMeter(dsmeterDSID);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));

  BOOST_CHECK_EQUAL(dev1->getLastKnownDSMeterDSID().toString(), NullDSID.toString());

  dev1->setDSMeter(mod);

  BOOST_CHECK_EQUAL(dev1->getDSMeterDSID().toString(), dsmeterDSID.toString());
  BOOST_CHECK_EQUAL(dev1->getLastKnownDSMeterDSID().toString(), dsmeterDSID.toString());
} // testDeviceLastKnownDSMeterDSIDWorks

BOOST_AUTO_TEST_CASE(testApartmentGetDeviceByName) {
  Apartment apt(NULL);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setName("dev1");

  BOOST_CHECK_EQUAL("dev1", apt.getDeviceByName("dev1")->getName());
} // testApartmentGetDeviceByName

BOOST_AUTO_TEST_CASE(testApartmentGetDSMeterByName) {
  Apartment apt(NULL);

  boost::shared_ptr<DSMeter> mod = apt.allocateDSMeter(dss_dsid_t(0,2));
  mod->setName("mod1");

  BOOST_CHECK_EQUAL("mod1", apt.getDSMeter("mod1")->getName());
} // testApartmentGetDSMeterByName

BOOST_AUTO_TEST_CASE(testZoneMoving) {
  Apartment apt(NULL);

  dss_dsid_t dsmeterDSID = dss_dsid_t(0,10);
  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dsmeterDSID);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setShortAddress(1);
  dev1->setDSMeter(meter);
  DeviceReference devRef1(dev1, &apt);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->setShortAddress(2);
  dev2->setDSMeter(meter);
  DeviceReference devRef2(dev2, &apt);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dss_dsid_t(0,3));
  dev3->setShortAddress(3);
  dev3->setDSMeter(meter);
  DeviceReference devRef3(dev3, &apt);
  boost::shared_ptr<Device> dev4 = apt.allocateDevice(dss_dsid_t(0,4));
  dev4->setShortAddress(4);
  dev4->setDSMeter(meter);
  DeviceReference devRef4(dev4, &apt);

  boost::shared_ptr<Zone> zone1 = apt.allocateZone(1);
  zone1->addDevice(devRef1);
  zone1->addDevice(devRef2);
  zone1->addDevice(devRef3);
  zone1->addDevice(devRef4);

  Set allDevices = apt.getDevices();

  BOOST_CHECK_EQUAL(dev1, allDevices.getByBusID(1, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev2, allDevices.getByBusID(2, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev3, allDevices.getByBusID(3, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev4, allDevices.getByBusID(4, dsmeterDSID).getDevice());

  BOOST_CHECK_EQUAL(4, zone1->getDevices().length());

  boost::shared_ptr<Zone> zone2 = apt.allocateZone(2);
  zone2->addDevice(devRef2);
  zone2->addDevice(devRef4);

  BOOST_CHECK_EQUAL(2, zone1->getDevices().length());
  BOOST_CHECK_EQUAL(2, zone2->getDevices().length());

  // check that the devices are moved correctly in the zones datamodel
  Set zone1Devices = zone1->getDevices();
  BOOST_CHECK_EQUAL(dev1, zone1Devices.getByBusID(1, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev3, zone1Devices.getByBusID(3, dsmeterDSID).getDevice());

  Set zone2Devices = zone2->getDevices();
  BOOST_CHECK_EQUAL(dev2, zone2Devices.getByBusID(2, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev4, zone2Devices.getByBusID(4, dsmeterDSID).getDevice());

  // check the datamodel of the devices
  BOOST_CHECK_EQUAL(1, dev1->getZoneID());
  BOOST_CHECK_EQUAL(1, dev3->getZoneID());

  BOOST_CHECK_EQUAL(2, dev2->getZoneID());
  BOOST_CHECK_EQUAL(2, dev4->getZoneID());

  // check that the groups are set up correctly
  // (this should be the case if all test above passed)
  Set zone1Group0Devices = zone1->getGroup(GroupIDBroadcast)->getDevices();
  BOOST_CHECK_EQUAL(dev1, zone1Group0Devices.getByBusID(1, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev3, zone1Group0Devices.getByBusID(3, dsmeterDSID).getDevice());

  Set zone2Group0Devices = zone2->getGroup(GroupIDBroadcast)->getDevices();
  BOOST_CHECK_EQUAL(dev2, zone2Group0Devices.getByBusID(2, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev4, zone2Group0Devices.getByBusID(4, dsmeterDSID).getDevice());
} // testZoneMoving

BOOST_AUTO_TEST_CASE(testSet) {
  Apartment apt(NULL);

  dss_dsid_t dsmeterDSID = dss_dsid_t(0,10);
  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dsmeterDSID);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setShortAddress(1);
  dev1->setDSMeter(meter);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->setShortAddress(2);
  dev2->setDSMeter(meter);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dss_dsid_t(0,3));
  dev3->setShortAddress(3);
  dev3->setDSMeter(meter);
  boost::shared_ptr<Device> dev4 = apt.allocateDevice(dss_dsid_t(0,4));
  dev4->setShortAddress(4);
  dev4->setDSMeter(meter);

  Set allDevices = apt.getDevices();

  BOOST_CHECK_EQUAL(dev1, allDevices.getByBusID(1, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev2, allDevices.getByBusID(2, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev3, allDevices.getByBusID(3, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev4, allDevices.getByBusID(4, dsmeterDSID).getDevice());

  Set setdev1 = Set(dev1);

  Set allMinusDev1 = allDevices.remove(setdev1);

  BOOST_CHECK_EQUAL(3, allMinusDev1.length());

  BOOST_CHECK_EQUAL(false, allMinusDev1.contains(dev1));

  BOOST_CHECK_EQUAL(dev2, allMinusDev1.getByBusID(2, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev3, allMinusDev1.getByBusID(3, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev4, allMinusDev1.getByBusID(4, dsmeterDSID).getDevice());

  // check that the other sets are not afected by our operation
  BOOST_CHECK_EQUAL(1, setdev1.length());
  BOOST_CHECK_EQUAL(dev1, setdev1.getByBusID(1, dsmeterDSID).getDevice());

  BOOST_CHECK_EQUAL(4, allDevices.length());
  BOOST_CHECK_EQUAL(dev1, allDevices.getByBusID(1, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev2, allDevices.getByBusID(2, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev3, allDevices.getByBusID(3, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev4, allDevices.getByBusID(4, dsmeterDSID).getDevice());

  Set allRecombined = allMinusDev1.combine(setdev1);

  BOOST_CHECK_EQUAL(4, allRecombined.length());
  BOOST_CHECK_EQUAL(dev1, allRecombined.getByBusID(1, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev2, allRecombined.getByBusID(2, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev3, allRecombined.getByBusID(3, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev4, allRecombined.getByBusID(4, dsmeterDSID).getDevice());

  BOOST_CHECK_EQUAL(1, setdev1.length());
  BOOST_CHECK_EQUAL(dev1, setdev1.getByBusID(1, dsmeterDSID).getDevice());

  allRecombined = allRecombined.combine(setdev1);
  BOOST_CHECK_EQUAL(4, allRecombined.length());

  allRecombined = allRecombined.combine(setdev1);
  BOOST_CHECK_EQUAL(4, allRecombined.length());

  allRecombined = allRecombined.combine(allRecombined);
  BOOST_CHECK_EQUAL(4, allRecombined.length());
  BOOST_CHECK_EQUAL(dev1, allRecombined.getByBusID(1, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev2, allRecombined.getByBusID(2, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev3, allRecombined.getByBusID(3, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev4, allRecombined.getByBusID(4, dsmeterDSID).getDevice());

  allMinusDev1 = allRecombined;
  allMinusDev1.removeDevice(dev1);
  BOOST_CHECK_EQUAL(3, allMinusDev1.length());
  BOOST_CHECK_EQUAL(dev2, allMinusDev1.getByBusID(2, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev3, allMinusDev1.getByBusID(3, dsmeterDSID).getDevice());
  BOOST_CHECK_EQUAL(dev4, allMinusDev1.getByBusID(4, dsmeterDSID).getDevice());
} // testSet

BOOST_AUTO_TEST_CASE(testSetBuilder) {
  Apartment apt(NULL);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setShortAddress(1);
  dev1->getGroupBitmask().set(GroupIDYellow - 1);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->setShortAddress(2);
  dev2->getGroupBitmask().set(GroupIDCyan - 1);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dss_dsid_t(0,3));
  dev3->setShortAddress(3);
  boost::shared_ptr<Device> dev4 = apt.allocateDevice(dss_dsid_t(0,4));
  dev4->setShortAddress(4);

  SetBuilder builder(apt);
  Set builderTest = builder.buildSet("yellow", apt.getZone(0));

  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("cyan", apt.getZone(0));

  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev2, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid(1)", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow.dsid(1)", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid(1).yellow", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow.yellow.yellow.yellow.yellow.yellow.dsid(1)", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet(" yellow ", apt.getZone(0));

  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("\t\n\rcyan", apt.getZone(0));

  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev2, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid( 1)", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid( 1 )", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid(1 )", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet(" dsid(1 ) ", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow .dsid(1)", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow. dsid(1)", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow . dsid(1)", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid(1) .yellow", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid(1). yellow", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid(1) . yellow", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow .yellow. yellow . yellow  .yellow.  yellow \n.dsid(\t1)    ", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("group(0).remove(dsid(1))", apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("remove(dsid(1))", apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("remove(yellow)", apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("group('broadcast')", apt.getZone(0));
  BOOST_CHECK_EQUAL(4, builderTest.length());

  builderTest = builder.buildSet("group('broadcast').remove(dsid(1))", apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("empty().addDevices(1,2,3)", apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("addDevices(1)", boost::shared_ptr<Zone>());
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("addDevices(1,2)", boost::shared_ptr<Zone>());
  BOOST_CHECK_EQUAL(2, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());
  BOOST_CHECK_EQUAL(dev2, builderTest.get(1).getDevice());

  builderTest = builder.buildSet("addDevices(1,2,3)", boost::shared_ptr<Zone>());
  BOOST_CHECK_EQUAL(3, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());
  BOOST_CHECK_EQUAL(dev2, builderTest.get(1).getDevice());
  BOOST_CHECK_EQUAL(dev3, builderTest.get(2).getDevice());
} // testSetBuilder

BOOST_AUTO_TEST_CASE(testSetBuilderDeviceByName) {
  Apartment apt(NULL);

  SetBuilder setBuilder(apt);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setName("dev1");
  dev1->setShortAddress(1);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->setName("dev2");
  dev2->setShortAddress(2);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dss_dsid_t(0,3));
  dev3->setName("dev3");
  dev3->setShortAddress(3);

  Set res = setBuilder.buildSet("", apt.getZone(0));
  BOOST_CHECK_EQUAL(res.length(), 3);

  res = setBuilder.buildSet("dev1", apt.getZone(0));
  BOOST_CHECK_EQUAL(res.length(), 1);
  BOOST_CHECK_EQUAL(res.get(0).getDevice()->getName(), std::string("dev1"));

  res = setBuilder.buildSet("dev2", apt.getZone(0));
  BOOST_CHECK_EQUAL(res.length(), 1);
  BOOST_CHECK_EQUAL(res.get(0).getDevice()->getName(), std::string("dev2"));
} // testSetBuilder

BOOST_AUTO_TEST_CASE(testSetBuilderTag) {
  Apartment apt(NULL);
  PropertySystem propSys;
  apt.setPropertySystem(&propSys);

  SetBuilder setBuilder(apt);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->addTag("dev1");
  dev1->addTag("device");
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->addTag("dev2");
  dev2->addTag("device");

  Set result = setBuilder.buildSet(".tag('dev1')", boost::shared_ptr<Zone>());
  BOOST_CHECK_EQUAL(result.length(), 1);
  BOOST_CHECK_EQUAL(result[0].getDevice(), dev1);

  result = setBuilder.buildSet(".tag('dev2')", boost::shared_ptr<Zone>());
  BOOST_CHECK_EQUAL(result.length(), 1);
  BOOST_CHECK_EQUAL(result[0].getDevice(), dev2);

  result = setBuilder.buildSet(".tag('device')", boost::shared_ptr<Zone>());
  BOOST_CHECK_EQUAL(result.length(), 2);
  BOOST_CHECK_EQUAL(result[0].getDevice(), dev1);
  BOOST_CHECK_EQUAL(result[1].getDevice(), dev2);

  result = setBuilder.buildSet(".tag('nonexisting')", boost::shared_ptr<Zone>());
  BOOST_CHECK_EQUAL(result.length(), 0);
} // testSetBuilderTag

BOOST_AUTO_TEST_CASE(testRemoval) {
  Apartment apt(NULL);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setShortAddress(1);
  dev1->getGroupBitmask().set(GroupIDYellow - 1);

  SetBuilder builder(apt);
  BOOST_CHECK_EQUAL(1, builder.buildSet(".yellow", boost::shared_ptr<Zone>()).length());

  apt.removeDevice(dev1->getDSID());
  BOOST_CHECK_EQUAL(0, builder.buildSet(".yellow", boost::shared_ptr<Zone>()).length());

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

  apt.allocateDSMeter(dss_dsid_t(1,0));
  try {
    apt.getDSMeterByDSID(dss_dsid_t(1,0));
    BOOST_CHECK(true);
  } catch(ItemNotFoundException&) {
    BOOST_CHECK_MESSAGE(false, "DSMeter not found");
  }

  apt.removeDSMeter(dss_dsid_t(1,0));
  try {
    apt.getDSMeterByDSID(dss_dsid_t(1,0));
    BOOST_CHECK_MESSAGE(false, "DSMeter still exists");
  } catch(ItemNotFoundException&) {
    BOOST_CHECK(true);
  }
} // testRemoval
/* TODO: libdsm
BOOST_AUTO_TEST_CASE(testCallScenePropagation) {
  Apartment apt(NULL);

  DS485BusRequestDispatcher dispatcher;
  ModelMaintenance maintenance(NULL, 2);
  maintenance.setApartment(&apt);
  maintenance.initialize();
  DS485Proxy proxy(NULL, &maintenance, NULL);
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
    sleepMS(2);
  }

  boost::shared_ptr<DSMeter> mod = apt.allocateDSMeter(dss_dsid_t(0,3));
  mod->setBusID(76);
  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setName("dev1");
  dev1->setShortAddress(1);
  dev1->setDSMeter(mod);
  DeviceReference devRef1(dev1, &apt);
  mod->addDevice(devRef1);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->setName("dev2");
  dev2->setShortAddress(2);
  dev2->setDSMeter(mod);
  DeviceReference devRef2(dev2, &apt);
  mod->addDevice(devRef2);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dss_dsid_t(0,3));
  dev3->setName("dev3");
  dev3->setShortAddress(3);
  dev3->setDSMeter(mod);
  DeviceReference devRef3(dev3, &apt);
  mod->addDevice(devRef3);

  dev1->callScene(Scene1);
  sleepMS(5);
  BOOST_CHECK_EQUAL(Scene1, dev1->getLastCalledScene());
  apt.getZone(0)->callScene(Scene2);
  sleepMS(5);
  BOOST_CHECK_EQUAL(Scene2, dev1->getLastCalledScene());
  BOOST_CHECK_EQUAL(Scene2, dev2->getLastCalledScene());

  Set set;
  set.addDevice(dev3);
  set.addDevice(dev2);
  set.callScene(Scene3);
  sleepMS(5);
  BOOST_CHECK_EQUAL(Scene2, dev1->getLastCalledScene());
  BOOST_CHECK_EQUAL(Scene3, dev2->getLastCalledScene());
  BOOST_CHECK_EQUAL(Scene3, dev3->getLastCalledScene());
  busHandler.shutdown();
  maintenance.shutdown();
  sleepMS(75);
  DSS::teardown();
} // testCallScenePropagation
*/

BOOST_AUTO_TEST_CASE(testMeteringDataFromUnknownMeter) {
  Apartment apt(NULL);

  ModelMaintenance maintenance(NULL, 2);
  maintenance.setApartment(&apt);
  maintenance.initialize();

  dss_dsid_t nonexistingDSMeterDSID(0, 55);
  ModelEventWithDSID* pEvent = new ModelEventWithDSID(ModelEvent::etMeteringValues, nonexistingDSMeterDSID);
  pEvent->addParameter(123); // power
  pEvent->addParameter(123); // meter value

  maintenance.addModelEvent(pEvent);

  maintenance.start();
  while(maintenance.isInitializing()) {
    sleepMS(2);
  }

  sleepMS(5);
  maintenance.shutdown();
  sleepMS(60);
} // testMeteringDataFromUnknownMeter

BOOST_AUTO_TEST_CASE(testEnergyMeterZero) {
  Apartment apt(NULL);

  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dss_dsid_t(0,10));
  meter->initializeEnergyMeterValue(0);
  meter->updateEnergyMeterValue(50);
  BOOST_CHECK_EQUAL(50, meter->getEnergyMeterValue());
}

BOOST_AUTO_TEST_CASE(testEnergyMeterWithMeteringData) {
  Apartment apt(NULL);

  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dss_dsid_t(0,10));
  meter->initializeEnergyMeterValue(50);
  meter->updateEnergyMeterValue(11);
  meter->updateEnergyMeterValue(12);
  BOOST_CHECK_EQUAL(51, meter->getEnergyMeterValue());
}

BOOST_AUTO_TEST_CASE(testEnergyMeterDropout) {
  Apartment apt(NULL);

  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dss_dsid_t(0,10));
  meter->initializeEnergyMeterValue(50);
  meter->updateEnergyMeterValue(11);
  meter->updateEnergyMeterValue(0);
  meter->updateEnergyMeterValue(1);
  BOOST_CHECK_EQUAL(51, meter->getEnergyMeterValue());
}

BOOST_AUTO_TEST_CASE(testApartmentCreatesRootNode) {
  Apartment apt(NULL);
  PropertySystem syst;
  apt.setPropertySystem(&syst);
  BOOST_CHECK(syst.getProperty("/apartment") != NULL);
} // testApartmentCreatesRootNode

BOOST_AUTO_TEST_CASE(testZoneNodes) {
  Apartment apt(NULL);
  PropertySystem syst;
  apt.setPropertySystem(&syst);
  BOOST_CHECK(syst.getProperty("/apartment/zones/zone0") != NULL);
} // testZoneNodes

BOOST_AUTO_TEST_CASE(testTags) {
  Apartment apt(NULL);
  PropertySystem syst;
  apt.setPropertySystem(&syst);
  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));

  BOOST_CHECK_EQUAL(dev1->hasTag("aTag"), false);

  dev1->addTag("aTag");
  BOOST_CHECK_EQUAL(dev1->hasTag("aTag"), true);
  BOOST_CHECK_EQUAL(dev1->hasTag("anotherTag"), false);

  dev1->addTag("anotherTag");
  BOOST_CHECK_EQUAL(dev1->hasTag("aTag"), true);
  BOOST_CHECK_EQUAL(dev1->hasTag("anotherTag"), true);

  dev1->removeTag("anotherTag");
  BOOST_CHECK_EQUAL(dev1->hasTag("aTag"), true);
  BOOST_CHECK_EQUAL(dev1->hasTag("anotherTag"), false);

  dev1->removeTag("aTag");
  BOOST_CHECK_EQUAL(dev1->hasTag("aTag"), false);

  dev1->addTag("one");
  dev1->addTag("two");
  std::vector<std::string> tags = dev1->getTags();
  BOOST_CHECK_EQUAL(tags.size(), 2);
  BOOST_CHECK_EQUAL(tags[0], "one");
  BOOST_CHECK_EQUAL(tags[1], "two");
}

BOOST_AUTO_TEST_CASE(testDeviceStateWhenRemovingMeter) {
  Apartment apt(NULL);

  ModelMaintenance maintenance(NULL, 2);
  maintenance.setApartment(&apt);
  maintenance.initialize();

  dss_dsid_t meter1DSID = dss_dsid_t(0,10);
  boost::shared_ptr<DSMeter> meter1 = apt.allocateDSMeter(meter1DSID);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setShortAddress(1);
  dev1->setDSMeter(meter1);

  ModelEventWithDSID* pEvent = new ModelEventWithDSID(ModelEvent::etLostDSMeter,
                                                      meter1DSID);

  maintenance.addModelEvent(pEvent);

  maintenance.start();
  while(maintenance.isInitializing()) {
    sleepMS(2);
  }

  sleepMS(5);

  maintenance.shutdown();
  sleepMS(60);

  BOOST_CHECK_EQUAL(meter1->isPresent(), false);
  BOOST_CHECK_EQUAL(dev1->isPresent(), false);
}

class DummyStructureModifyingInterface : public StructureModifyingBusInterface {
public:
  virtual void setZoneID(const dss_dsid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID) {
  }
  virtual void createZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
  }
  virtual void removeZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
  }
  virtual void addToGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
  }
  virtual void removeFromGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
  }
  virtual void removeInactiveDevices(const dss_dsid_t& _dsMeterID) {
  }
  virtual void removeDeviceFromDSMeter(const dss_dsid_t& _dsMeterID, const int _deviceID) {
  }
  virtual void sceneSetName(uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneNumber, const std::string& _name) {
  }
  virtual void deviceSetName(dss_dsid_t _meterDSID, devid_t _deviceID, const std::string& _name) {
  }
  virtual void meterSetName(dss_dsid_t _meterDSID, const std::string& _name) {
  }
  virtual void createGroup(uint16_t _zoneID, uint8_t _groupID) {
  }
  virtual void removeGroup(uint16_t _zoneID, uint8_t _groupID) {
  }
  virtual void setButtonSetsLocalPriority(const dss_dsid_t& _dsMeterID, const devid_t _deviceID, bool _setsPriority) {
  }
};

class DummyStructureQueryBusInterface: public StructureQueryBusInterface {
public:
  virtual std::vector<DSMeterSpec_t> getDSMeters() {
    return std::vector<DSMeterSpec_t>();
  }
  virtual DSMeterSpec_t getDSMeterSpec(const dss_dsid_t& _dsMeterID) {
    return DSMeterSpec_t();
  }
  virtual std::vector<int> getZones(const dss_dsid_t& _dsMeterID) {
    return std::vector<int>();
  }
  virtual std::vector<DeviceSpec_t> getDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    return std::vector<DeviceSpec_t>();
  }
  virtual std::vector<DeviceSpec_t> getInactiveDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    return std::vector<DeviceSpec_t>();
  }
  virtual std::vector<int> getGroups(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    return std::vector<int>();
  }
  virtual int getLastCalledScene(const dss_dsid_t& _dsMeterID, const int _zoneID, const int _groupID) {
    return 0;
  }
  virtual bool getEnergyBorder(const dss_dsid_t& _dsMeterID, int& _lower, int& _upper) {
    _lower = 0;
    _upper = 0;
    return false;
  }
  virtual DeviceSpec_t deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID) {
    return DeviceSpec_t();
  }
  virtual std::string getSceneName(dss_dsid_t _dsMeterID, boost::shared_ptr<Group> _group, const uint8_t _sceneNumber) {
    return "";
  }
}; // DummyStructureQueryBusInterface

class DummyActionRequestInterface : public ActionRequestInterface {
public:
  virtual void callScene(AddressableModelItem *pTarget, const uint16_t scene, const bool _force) {
    Group* pGroup = dynamic_cast<Group*>(pTarget);
    Device* pDevice = dynamic_cast<Device*>(pTarget);
    m_Log += "callScene(";
    if(pGroup != NULL) {
      m_Log += intToString(pGroup->getZoneID()) + "," + intToString(pGroup->getID());
    } else {
      m_Log += intToString(pDevice->getShortAddress());
    }
    m_Log += "," + intToString(scene) + ")";
  }
  virtual void saveScene(AddressableModelItem *pTarget, const uint16_t scene) {
  }
  virtual void undoScene(AddressableModelItem *pTarget) {
  }
  virtual void blink(AddressableModelItem *pTarget) {
  }
  virtual void setValue(AddressableModelItem *pTarget, const uint8_t _value) {
  }

  std::string getLog() {
    return m_Log;
  }

  void clearLog() {
    m_Log.clear();
  }
private:
  std::string m_Log;
};

class DummyBusInterface : public BusInterface {
public:
  DummyBusInterface(StructureModifyingBusInterface* _pStructureModifier,
                    StructureQueryBusInterface* _pStructureQuery,
                    ActionRequestInterface* _pActionRequest)
  : m_pStructureModifier(_pStructureModifier),
    m_pStructureQuery(_pStructureQuery),
    m_pActionRequest(_pActionRequest)
  { }

  virtual DeviceBusInterface* getDeviceBusInterface() {
    return NULL;
  }
  virtual StructureQueryBusInterface* getStructureQueryBusInterface() {
    return NULL;
  }
  virtual MeteringBusInterface* getMeteringBusInterface() {
    return NULL;
  }
  virtual StructureModifyingBusInterface* getStructureModifyingBusInterface() {
    return m_pStructureModifier;
  }
  virtual ActionRequestInterface* getActionRequestInterface() {
    return m_pActionRequest;
  }

  virtual void setBusEventSink(BusEventSink* _eventSink) {
  }
private:
  StructureModifyingBusInterface* m_pStructureModifier;
  StructureQueryBusInterface* m_pStructureQuery;
  ActionRequestInterface* m_pActionRequest;
};

BOOST_AUTO_TEST_CASE(testPersistSet) {
  Apartment apt(NULL);

  dss_dsid_t meter1DSID = dss_dsid_t(0,10);
  boost::shared_ptr<DSMeter> meter1 = apt.allocateDSMeter(meter1DSID);
  boost::shared_ptr<Zone> zone1 = apt.allocateZone(1);
  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setShortAddress(1);
  dev1->setDSMeter(meter1);
  dev1->setZoneID(1);
  dev1->addToGroup(1);
  DeviceReference devRef1 = DeviceReference(dev1, &apt);
  zone1->addDevice(devRef1);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->setShortAddress(2);
  dev2->setDSMeter(meter1);
  dev2->setZoneID(1);
  dev2->addToGroup(1);
  DeviceReference devRef2 = DeviceReference(dev2, &apt);
  zone1->addDevice(devRef2);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dss_dsid_t(0,3));
  dev3->setShortAddress(3);
  dev3->setDSMeter(meter1);
  dev3->setZoneID(1);
  dev3->addToGroup(1);
  DeviceReference devRef3 = DeviceReference(dev3, &apt);
  zone1->addDevice(devRef3);

  DummyStructureModifyingInterface modifyingInterface;
  DummyStructureQueryBusInterface queryInterface;
  DummyActionRequestInterface actionInterface;
  DummyBusInterface busInterface(&modifyingInterface, &queryInterface, &actionInterface);

  StructureManipulator manipulator(modifyingInterface, queryInterface, apt);
  apt.setBusInterface(&busInterface);

  Set set;
  set.addDevice(dev1);
  set.addDevice(dev2);

  set.callScene(5);
  BOOST_CHECK_EQUAL(actionInterface.getLog(), "callScene(1,5)callScene(2,5)");

  manipulator.persistSet(set, "");
  actionInterface.clearLog();
  set.callScene(5);

  BOOST_CHECK_EQUAL(actionInterface.getLog(), "callScene(0,16,5)");
}

BOOST_AUTO_TEST_CASE(testUnPersistSet) {
  Apartment apt(NULL);

  dss_dsid_t meter1DSID = dss_dsid_t(0,10);
  boost::shared_ptr<DSMeter> meter1 = apt.allocateDSMeter(meter1DSID);
  boost::shared_ptr<Zone> zone1 = apt.allocateZone(1);
  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dss_dsid_t(0,1));
  dev1->setShortAddress(1);
  dev1->setDSMeter(meter1);
  dev1->setZoneID(1);
  dev1->addToGroup(1);
  DeviceReference devRef1 = DeviceReference(dev1, &apt);
  zone1->addDevice(devRef1);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dss_dsid_t(0,2));
  dev2->setShortAddress(2);
  dev2->setDSMeter(meter1);
  dev2->setZoneID(1);
  dev2->addToGroup(1);
  DeviceReference devRef2 = DeviceReference(dev2, &apt);
  zone1->addDevice(devRef2);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dss_dsid_t(0,3));
  dev3->setShortAddress(3);
  dev3->setDSMeter(meter1);
  dev3->setZoneID(1);
  dev3->addToGroup(1);
  DeviceReference devRef3 = DeviceReference(dev3, &apt);
  zone1->addDevice(devRef3);

  DummyStructureModifyingInterface modifyingInterface;
  DummyStructureQueryBusInterface queryInterface;
  DummyActionRequestInterface actionInterface;
  DummyBusInterface busInterface(&modifyingInterface, &queryInterface, &actionInterface);

  StructureManipulator manipulator(modifyingInterface, queryInterface, apt);
  apt.setBusInterface(&busInterface);

  Set set;
  set.addDevice(dev1);
  set.addDevice(dev2);

  set.callScene(5);
  BOOST_CHECK_EQUAL(actionInterface.getLog(), "callScene(1,5)callScene(2,5)");

  std::string setDescription = "addDevices(1,2)";
  manipulator.persistSet(set, setDescription);
  actionInterface.clearLog();
  set.callScene(5);
  BOOST_CHECK_EQUAL(actionInterface.getLog(), "callScene(0,16,5)");

  manipulator.unpersistSet(setDescription);
  actionInterface.clearLog();
  set.callScene(5);
  BOOST_CHECK_EQUAL(actionInterface.getLog(), "callScene(1,5)callScene(2,5)");
}

BOOST_AUTO_TEST_CASE(testSceneHelperBounds) {
  BOOST_CHECK_EQUAL(SceneHelper::rememberScene(SceneBell), false);
  BOOST_CHECK_EQUAL(SceneHelper::rememberScene(Scene1), true);
  BOOST_CHECK_EQUAL(SceneHelper::rememberScene(MaxSceneNumber + 1), false);
}

BOOST_AUTO_TEST_CASE(testSceneHelperReachableScenes) {
  BOOST_CHECK(SceneHelper::getReachableScenesBitmapForButtonID(0) == 0x0E0001uLL);
  BOOST_CHECK_EQUAL(SceneHelper::getReachableScenesBitmapForButtonID(-1), 0);
}

BOOST_AUTO_TEST_SUITE_END()
