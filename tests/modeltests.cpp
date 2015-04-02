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
#include <digitalSTROM/ds.h>
#include <iostream>
#include <fstream>

#include "src/model/device.h"
#include "src/model/apartment.h"
#include "src/model/modulator.h"
#include "src/model/devicereference.h"
#include "src/model/zone.h"
#include "src/model/group.h"
#include "src/model/set.h"
#include "src/model/modelpersistence.h"
#include "src/setbuilder.h"
#include "src/dss.h"
#include "src/model/modelconst.h"
#include "src/model/modelmaintenance.h"
#include "src/propertysystem.h"
#include "src/structuremanipulator.h"
#include "src/businterface.h"
#include "src/model/scenehelper.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(Model)

class DummyStructureModifyingInterface : public StructureModifyingBusInterface {
public:
  virtual void setZoneID(const dsuid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID) {
  }
  virtual void createZone(const dsuid_t& _dsMeterID, const int _zoneID) {
  }
  virtual void removeZone(const dsuid_t& _dsMeterID, const int _zoneID) {
  }
  virtual void addToGroup(const dsuid_t& _dsMeterID, const int _groupID, const int _deviceID) {
  }
  virtual void removeFromGroup(const dsuid_t& _dsMeterID, const int _groupID, const int _deviceID) {
  }
  virtual void removeInactiveDevices(const dsuid_t& _dsMeterID) {
  }
  virtual void removeDeviceFromDSMeter(const dsuid_t& _dsMeterID, const int _deviceID) {
  }
  virtual void removeDeviceFromDSMeters(const dsuid_t& _deviceDSID) {
  }
  virtual void sceneSetName(uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneNumber, const std::string& _name) {
  }
  virtual void deviceSetName(dsuid_t _meterDSID, devid_t _deviceID, const std::string& _name) {
  }
  virtual void meterSetName(dsuid_t _meterDSID, const std::string& _name) {
  }
  virtual void createGroup(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID, const std::string& _name) {
  }
  virtual void removeGroup(uint16_t _zoneID, uint8_t _groupID) {
  }
  virtual void groupSetName(uint16_t _zoneID, uint8_t _groupID, const std::string& _name) {
  }
  virtual void groupSetStandardID(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID) {
  }
  virtual void sensorPush(uint16_t _zoneID, uint8_t groupID, dsuid_t _sourceID, uint8_t _sensorType, uint16_t _sensorValue) {
  }
  virtual void setButtonSetsLocalPriority(const dsuid_t& _dsMeterID, const devid_t _deviceID, bool _setsPriority) {
  }
  virtual void setButtonCallsPresent(const dsuid_t& _dsMeterID, const devid_t _deviceID, bool _callsPresent) {
  }
  virtual void setZoneHeatingConfig(const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingConfigSpec_t _spec) {}
  virtual void setZoneHeatingState(const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingStateSpec_t _spec) {}
  virtual void setZoneHeatingOperationModes(const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingOperationModeSpec_t _spec) {}
  virtual void setZoneSensor(const uint16_t _zoneID, const uint8_t _sensorType, const dsuid_t& _sensorDSUID) {}
  virtual void resetZoneSensor(const uint16_t _zoneID, const uint8_t _sensorType) {}
};

BOOST_AUTO_TEST_CASE(testApartmentAllocateDeviceReturnsTheSameDeviceForDSID) {
  Apartment apt(NULL);

  dsuid_t dsuid1, dsuid2;
  dsuid1.id[DSUID_SIZE - 1] = 1;
  dsuid2.id[DSUID_SIZE - 1] = 2;

  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dsuid1);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid2);
  dev1->setShortAddress(1);
  dev1->setName("dev1");
  dev1->setDSMeter(meter);

  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid2);
  BOOST_CHECK_EQUAL(dev1->getShortAddress(), dev2->getShortAddress());
  BOOST_CHECK_EQUAL(dev1->getName(), dev2->getName());
  BOOST_CHECK_EQUAL(dsuid2str(dev1->getDSMeterDSID()), dsuid2str(dev2->getDSMeterDSID()));
} // testApartmentAllocateDeviceReturnsTheSameDeviceForDSID

BOOST_AUTO_TEST_CASE(testSetGetByBusID) {
  Apartment apt(NULL);

  dsuid_t meter1DSID, meter2DSID, dsuid1, dsuid2;
  meter1DSID.id[DSUID_SIZE - 1] = 10;
  meter2DSID.id[DSUID_SIZE - 1]  = 11;
  dsuid1.id[DSUID_SIZE - 1]  = 1;
  dsuid2.id[DSUID_SIZE - 1] = 2;

  boost::shared_ptr<DSMeter> meter1 = apt.allocateDSMeter(meter1DSID);
  boost::shared_ptr<DSMeter> meter2 = apt.allocateDSMeter(meter2DSID);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
  dev1->setShortAddress(1);
  dev1->setName("dev1");
  dev1->setDSMeter(meter1);

  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid2);
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

  dsuid_t dsuid1, dsuid2;
  dsuid1.id[DSUID_SIZE - 1] = 1;
  dsuid2.id[DSUID_SIZE - 1] = 2;

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
  dev1->setShortAddress(1);
  dev1->setName("dev1");

  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid2);
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

  dsuid_t dsuid1, dsuid2;
  dsuid1.id[DSUID_SIZE - 1] = 1;
  dsuid2.id[DSUID_SIZE - 1] = 2;

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
  dev1->addTag("dev1");
  dev1->addTag("device");

  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid2);
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

  dsuid_t dsuid1(DSUID_NULL), dsmeterDSID(DSUID_NULL);

  dsuid1.id[DSUID_SIZE - 1] = 1;
  dsmeterDSID.id[DSUID_SIZE - 1] = 10;
  
  boost::shared_ptr<DSMeter> mod = apt.allocateDSMeter(dsmeterDSID);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);

  BOOST_CHECK_EQUAL(dsuid2str(dev1->getLastKnownDSMeterDSID()), dsuid2str(DSUID_NULL));

  dev1->setDSMeter(mod);

  BOOST_CHECK_EQUAL(dsuid2str(dev1->getDSMeterDSID()), dsuid2str(dsmeterDSID));
  BOOST_CHECK_EQUAL(dsuid2str(dev1->getLastKnownDSMeterDSID()), dsuid2str(dsmeterDSID));
} // testDeviceLastKnownDSMeterDSIDWorks

BOOST_AUTO_TEST_CASE(testApartmentGetDeviceByName) {
  Apartment apt(NULL);

  dsuid_t dsuid1;
  dsuid1.id[DSUID_SIZE - 1] = 1;

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
  dev1->setName("dev1");

  BOOST_CHECK_EQUAL("dev1", apt.getDeviceByName("dev1")->getName());
} // testApartmentGetDeviceByName

BOOST_AUTO_TEST_CASE(testApartmentGetDSMeterByName) {
  Apartment apt(NULL);

  dsuid_t dsuid1;
  dsuid1.id[DSUID_SIZE - 1] = 2;

  boost::shared_ptr<DSMeter> mod = apt.allocateDSMeter(dsuid1);
  mod->setName("mod1");

  BOOST_CHECK_EQUAL("mod1", apt.getDSMeter("mod1")->getName());
} // testApartmentGetDSMeterByName

BOOST_AUTO_TEST_CASE(testZoneMoving) {
  Apartment apt(NULL);

  dsuid_t dsuid1, dsuid2, dsuid3, dsuid4, dsmeterDSID;
  dsmeterDSID.id[DSUID_SIZE - 1] = 10;
  dsuid1.id[DSUID_SIZE - 1] = 1;
  dsuid2.id[DSUID_SIZE - 1] = 2;
  dsuid3.id[DSUID_SIZE - 1] = 3;
  dsuid4.id[DSUID_SIZE - 1] = 4;

  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dsmeterDSID);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
  dev1->setShortAddress(1);
  dev1->setDSMeter(meter);
  DeviceReference devRef1(dev1, &apt);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid2);
  dev2->setShortAddress(2);
  dev2->setDSMeter(meter);
  DeviceReference devRef2(dev2, &apt);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dsuid3);
  dev3->setShortAddress(3);
  dev3->setDSMeter(meter);
  DeviceReference devRef3(dev3, &apt);
  boost::shared_ptr<Device> dev4 = apt.allocateDevice(dsuid4);
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

  dsuid_t dsuid1, dsuid2, dsuid3, dsuid4, dsmeterDSID;
  dsmeterDSID.id[DSUID_SIZE - 1] = 10;
  dsuid1.id[DSUID_SIZE - 1] = 1;
  dsuid2.id[DSUID_SIZE - 1] = 2;
  dsuid3.id[DSUID_SIZE - 1] = 3;
  dsuid4.id[DSUID_SIZE - 1] = 4;

  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dsmeterDSID);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
  dev1->setShortAddress(1);
  dev1->setDSMeter(meter);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid2);
  dev2->setShortAddress(2);
  dev2->setDSMeter(meter);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dsuid3);
  dev3->setShortAddress(3);
  dev3->setDSMeter(meter);
  boost::shared_ptr<Device> dev4 = apt.allocateDevice(dsuid4);
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

  dsuid_t dsuid1, dsuid2, dsuid3, dsuid4;
  dsuid1 = dsuid2 = dsuid3 = dsuid4 = DSUID_NULL;
  dsuid1.id[DSUID_SIZE - 1] = 1;
  dsuid2.id[DSUID_SIZE - 1] = 2;
  dsuid3.id[DSUID_SIZE - 1] = 3;
  dsuid4.id[DSUID_SIZE - 1] = 4;

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
  dev1->setShortAddress(1);
  dev1->getGroupBitmask().set(GroupIDYellow - 1);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid2);
  dev2->setShortAddress(2);
  dev2->getGroupBitmask().set(GroupIDCyan - 1);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dsuid3);
  dev3->setShortAddress(3);
  boost::shared_ptr<Device> dev4 = apt.allocateDevice(dsuid4);
  dev4->setShortAddress(4);

  SetBuilder builder(apt);
  Set builderTest = builder.buildSet("yellow", apt.getZone(0));

  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("cyan", apt.getZone(0));

  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev2, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid("+dsuid2str(dsuid1)+")", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow.dsid("+dsuid2str(dsuid1)+")", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid("+dsuid2str(dsuid1)+").yellow", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow.yellow.yellow.yellow.yellow.yellow.dsid("+dsuid2str(dsuid1)+")", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet(" yellow ", apt.getZone(0));

  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("\t\n\rcyan", apt.getZone(0));

  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev2, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid( "+dsuid2str(dsuid1)+")", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid( "+dsuid2str(dsuid1)+" )", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid("+dsuid2str(dsuid1)+" )", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet(" dsid("+dsuid2str(dsuid1)+" ) ", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow .dsid("+dsuid2str(dsuid1)+")", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow. dsid("+dsuid2str(dsuid1)+")", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow . dsid("+dsuid2str(dsuid1)+")", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid("+dsuid2str(dsuid1)+") .yellow", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid("+dsuid2str(dsuid1)+"). yellow", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("dsid("+dsuid2str(dsuid1)+") . yellow", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("yellow .yellow. yellow . yellow  .yellow.  yellow \n.dsid(\t"+dsuid2str(dsuid1)+")    ", apt.getZone(0));
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("group(0).remove(dsid("+dsuid2str(dsuid1)+"))", apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("remove(dsid("+dsuid2str(dsuid1)+"))", apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("remove(yellow)", apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("group('broadcast')", apt.getZone(0));
  BOOST_CHECK_EQUAL(4, builderTest.length());

  builderTest = builder.buildSet("group('broadcast').remove(dsid("+dsuid2str(dsuid1)+"))", apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("empty().addDevices("+dsuid2str(dsuid1)+","+dsuid2str(dsuid2)+","+dsuid2str(dsuid3)+")", apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());

  builderTest = builder.buildSet("addDevices("+dsuid2str(dsuid1)+")", boost::shared_ptr<Zone>());
  BOOST_CHECK_EQUAL(1, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());

  builderTest = builder.buildSet("addDevices("+dsuid2str(dsuid1)+","+dsuid2str(dsuid2)+")", boost::shared_ptr<Zone>());
  BOOST_CHECK_EQUAL(2, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());
  BOOST_CHECK_EQUAL(dev2, builderTest.get(1).getDevice());

  builderTest = builder.buildSet("addDevices("+dsuid2str(dsuid1)+","+dsuid2str(dsuid2)+","+dsuid2str(dsuid3)+")", boost::shared_ptr<Zone>());
  BOOST_CHECK_EQUAL(3, builderTest.length());
  BOOST_CHECK_EQUAL(dev1, builderTest.get(0).getDevice());
  BOOST_CHECK_EQUAL(dev2, builderTest.get(1).getDevice());
  BOOST_CHECK_EQUAL(dev3, builderTest.get(2).getDevice());
} // testSetBuilder

BOOST_AUTO_TEST_CASE(testSetBuilderDeviceByName) {
  Apartment apt(NULL);

  SetBuilder setBuilder(apt);

  dsuid_t dsuid1, dsuid2, dsuid3;
  dsuid1.id[DSUID_SIZE - 1] = 1;
  dsuid2.id[DSUID_SIZE - 1] = 2;
  dsuid3.id[DSUID_SIZE - 1] = 3;

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
  dev1->setName("dev1");
  dev1->setShortAddress(1);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid2);
  dev2->setName("dev2");
  dev2->setShortAddress(2);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dsuid3);
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

  dsuid_t dsuid1, dsuid2;
  dsuid1.id[DSUID_SIZE - 1] = 1;
  dsuid2.id[DSUID_SIZE - 1] = 2;

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
  dev1->addTag("dev1");
  dev1->addTag("device");
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid2);
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

BOOST_AUTO_TEST_CASE(testMeterSetBuilder) {
  Apartment apt(NULL);

  dsuid_t dsuid1, dsuid2, dsuid3, dsuid4;
  dsuid1 = dsuid2 = dsuid3 = dsuid4 = DSUID_NULL;
  dsuid1.id[DSUID_SIZE - 1] = 1;
  dsuid2.id[DSUID_SIZE - 1] = 2;
  dsuid3.id[DSUID_SIZE - 1] = 3;
  dsuid4.id[DSUID_SIZE - 1] = 4;

  apt.allocateDSMeter(dsuid1);
  apt.allocateDSMeter(dsuid2);
  apt.allocateDSMeter(dsuid3);
  apt.allocateDSMeter(dsuid4);

  MeterSetBuilder builder(apt);
  std::vector<boost::shared_ptr<DSMeter> > meters;
  meters = builder.buildSet(".meters(all)");
  BOOST_CHECK_EQUAL(4, meters.size());

  meters = builder.buildSet(".meters("+dsuid2str(dsuid1)+")");
  BOOST_CHECK_EQUAL(1, meters.size());

  meters = builder.buildSet(".meters("+dsuid2str(dsuid1)+","+dsuid2str(dsuid2)+")");
  BOOST_CHECK_EQUAL(2, meters.size());

  meters = builder.buildSet(".meters("+dsuid2str(dsuid1)+","+dsuid2str(dsuid2)+","+dsuid2str(dsuid3)+")");
  BOOST_CHECK_EQUAL(3, meters.size());

  meters = builder.buildSet(".meters("+dsuid2str(dsuid1)+","+dsuid2str(dsuid2)+","+dsuid2str(dsuid3)+","+dsuid2str(dsuid4)+")");
  BOOST_CHECK_EQUAL(4, meters.size());

  BOOST_CHECK_THROW(builder.buildSet(""), std::runtime_error);
  BOOST_CHECK_THROW(builder.buildSet(".meters("+dsuid2str(dsuid1)+",)"), std::runtime_error);
  BOOST_CHECK_THROW(builder.buildSet(".meters(,"+dsuid2str(dsuid1)+")"), std::runtime_error);
  BOOST_CHECK_THROW(builder.buildSet(".meters(7)"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(testRemoval) {
  Apartment apt(NULL);

  dsuid_t dsuid1, meterDSID;
  dsuid1.id[DSUID_SIZE - 1] = 1;
  meterDSID.id[DSUID_SIZE - 1] = 10;
  

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
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

  apt.allocateDSMeter(meterDSID);
  try {
    apt.getDSMeterByDSID(meterDSID);
    BOOST_CHECK(true);
  } catch(ItemNotFoundException&) {
    BOOST_CHECK_MESSAGE(false, "DSMeter not found");
  }

  apt.removeDSMeter(meterDSID);
  try {
    apt.getDSMeterByDSID(meterDSID);
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

  boost::shared_ptr<DSMeter> mod = apt.allocateDSMeter(dsuid_t(0,3));
  mod->setBusID(76);
  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid_t(0,1));
  dev1->setName("dev1");
  dev1->setShortAddress(1);
  dev1->setDSMeter(mod);
  DeviceReference devRef1(dev1, &apt);
  mod->addDevice(devRef1);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid_t(0,2));
  dev2->setName("dev2");
  dev2->setShortAddress(2);
  dev2->setDSMeter(mod);
  DeviceReference devRef2(dev2, &apt);
  mod->addDevice(devRef2);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dsuid_t(0,3));
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
  maintenance.setStructureModifyingBusInterface(new DummyStructureModifyingInterface());
  maintenance.initialize();

  dsuid_t nonexistingDSMeterDSID;
  nonexistingDSMeterDSID.id[DSUID_SIZE - 1] = 55;

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

  dsuid_t dsmeterDSID;
  dsmeterDSID.id[DSUID_SIZE - 1] = 10;

  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dsmeterDSID);
  meter->initializeEnergyMeterValue(0);
  meter->updateEnergyMeterValue(50);
  BOOST_CHECK_EQUAL(50, meter->getEnergyMeterValue());
}

BOOST_AUTO_TEST_CASE(testEnergyMeterWithMeteringData) {
  Apartment apt(NULL);

  dsuid_t dsmeterDSID;
  dsmeterDSID.id[DSUID_SIZE - 1] = 10;

  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dsmeterDSID);
  meter->initializeEnergyMeterValue(50);
  meter->updateEnergyMeterValue(11);
  meter->updateEnergyMeterValue(12);
  BOOST_CHECK_EQUAL(51, meter->getEnergyMeterValue());
}

BOOST_AUTO_TEST_CASE(testEnergyMeterDropout) {
  Apartment apt(NULL);

  dsuid_t dsmeterDSID;
  dsmeterDSID.id[DSUID_SIZE - 1] = 10;

  boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dsmeterDSID);
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

  dsuid_t dsuid1;
  dsuid1.id[DSUID_SIZE - 1] = 1;

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);

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
  maintenance.setStructureModifyingBusInterface(new DummyStructureModifyingInterface());
  maintenance.initialize();

  dsuid_t dsuid1, meter1DSID;
  meter1DSID.id[DSUID_SIZE - 1] = 10;
  dsuid1.id[DSUID_SIZE - 1] = 1;

  boost::shared_ptr<DSMeter> meter1 = apt.allocateDSMeter(meter1DSID);

  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
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

class DummyStructureQueryBusInterface: public StructureQueryBusInterface {
public:
  virtual std::vector<DSMeterSpec_t> getBusMembers() {
    return std::vector<DSMeterSpec_t>();
  }
  virtual DSMeterSpec_t getDSMeterSpec(const dsuid_t& _dsMeterID) {
    return DSMeterSpec_t();
  }
  virtual std::vector<int> getZones(const dsuid_t& _dsMeterID) {
    return std::vector<int>();
  }
  virtual std::vector<DeviceSpec_t> getDevicesInZone(const dsuid_t& _dsMeterID, const int _zoneID, bool complete = true) {
    return std::vector<DeviceSpec_t>();
  }
  virtual std::vector<DeviceSpec_t> getInactiveDevicesInZone(const dsuid_t& _dsMeterID, const int _zoneID) {
    return std::vector<DeviceSpec_t>();
  }
  virtual std::vector<GroupSpec_t> getGroups(const dsuid_t& _dsMeterID, const int _zoneID) {
    return std::vector<GroupSpec_t>();
  }
  virtual std::vector<std::pair<int,int> > getLastCalledScenes(const dsuid_t& _dsMeterID, const int _zoneID) {
    return std::vector<std::pair<int,int> >();
  }
  virtual std::bitset<7> getZoneStates(const dsuid_t& _dsMeterID, const int _zoneID) {
      return std::bitset<7>(0);
  }

  virtual bool getEnergyBorder(const dsuid_t& _dsMeterID, int& _lower, int& _upper) {
    _lower = 0;
    _upper = 0;
    return false;
  }
  virtual DeviceSpec_t deviceGetSpec(devid_t _id, dsuid_t _dsMeterID) {
    return DeviceSpec_t();
  }
  virtual std::string getSceneName(dsuid_t _dsMeterID, boost::shared_ptr<Group> _group, const uint8_t _sceneNumber) {
    return "";
  }
  virtual DSMeterHash_t getDSMeterHash(const dsuid_t& _dsMeterID) {
    return DSMeterHash_t();
  }
  virtual ZoneHeatingConfigSpec_t getZoneHeatingConfig(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) {
    return ZoneHeatingConfigSpec_t();
  }
  virtual ZoneHeatingInternalsSpec_t getZoneHeatingInternals(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) {
    return ZoneHeatingInternalsSpec_t();
  }
  virtual ZoneHeatingStateSpec_t getZoneHeatingState(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) {
    return ZoneHeatingStateSpec_t();
  }
  virtual ZoneHeatingOperationModeSpec_t getZoneHeatingOperationModes(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) {
    return ZoneHeatingOperationModeSpec_t();
  }
  virtual dsuid_t getZoneSensor(const dsuid_t& _meterDSUID, const uint16_t _zoneID, const uint8_t _sensorType) {
    return DSUID_NULL;
  }
  virtual void getZoneSensorValue(const dsuid_t& _meterDSUID, const uint16_t _zoneID, const uint8_t _sensorType, uint16_t *_sensorValue, uint32_t *_sensorAge) {
  }
  virtual void protobufMessageRequest(const dsuid_t _dSMdSUID, const uint16_t _request_size, const uint8_t *_request, uint16_t *_response_size, uint8_t *_response) {
  }
}; // DummyStructureQueryBusInterface

class DummyActionRequestInterface : public ActionRequestInterface {
public:
  virtual void callScene(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint16_t scene, const std::string _token, const bool _force) {
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
  virtual void callSceneMin(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint16_t _scene, const std::string _token) {
  }
  virtual void saveScene(AddressableModelItem *pTarget, const callOrigin_t _origin, const uint16_t scene, const std::string _token) {
  }
  virtual void undoScene(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint16_t scene, const std::string _token) {
  }
  virtual void undoSceneLast(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token) {
  }
  virtual void blink(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token) {
  }
  virtual void setValue(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _value, const std::string _token) {
  }
  virtual void increaseOutputChannelValue(AddressableModelItem*, const callOrigin_t _origin, SceneAccessCategory, uint8_t, std::string) {
  }
  virtual void decreaseOutputChannelValue(AddressableModelItem*, const callOrigin_t _origin, SceneAccessCategory, uint8_t, std::string) {
  }
  virtual void stopOutputChannelValue(AddressableModelItem*, const callOrigin_t _origin, SceneAccessCategory, uint8_t, std::string) {
  }
  virtual void pushSensor(AddressableModelItem*, const callOrigin_t _origin, const SceneAccessCategory _category, dsuid_t _sourceID, uint8_t _sensorType, float _sensorValueFloat, const std::string _token) {
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
    return m_pStructureQuery;
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

  dsuid_t dsuid1, dsuid2, dsuid3, meter1DSID;
  meter1DSID.id[DSUID_SIZE - 1] = 10;
  dsuid1.id[DSUID_SIZE - 1] = 1;
  dsuid2.id[DSUID_SIZE - 1] = 2;
  dsuid3.id[DSUID_SIZE - 1] = 3;

  boost::shared_ptr<DSMeter> meter1 = apt.allocateDSMeter(meter1DSID);
  boost::shared_ptr<Zone> zone1 = apt.allocateZone(1);
  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
  dev1->setShortAddress(1);
  dev1->setDSMeter(meter1);
  dev1->setZoneID(1);
  dev1->addToGroup(1);
  DeviceReference devRef1 = DeviceReference(dev1, &apt);
  zone1->addDevice(devRef1);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid2);
  dev2->setShortAddress(2);
  dev2->setDSMeter(meter1);
  dev2->setZoneID(1);
  dev2->addToGroup(1);
  DeviceReference devRef2 = DeviceReference(dev2, &apt);
  zone1->addDevice(devRef2);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dsuid3);
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

  set.callScene(coTest, SAC_MANUAL, 5, "");
  BOOST_CHECK_EQUAL(actionInterface.getLog(), "callScene(1,5)callScene(2,5)");

  manipulator.persistSet(set, "");
  actionInterface.clearLog();
  set.callScene(coTest, SAC_MANUAL, 5, "");

  BOOST_CHECK_EQUAL(actionInterface.getLog(), "callScene(0,24,5)");
}

BOOST_AUTO_TEST_CASE(testUnPersistSet) {
  Apartment apt(NULL);

  dsuid_t dsuid1, dsuid2, dsuid3, meter1DSID;
  dsuid1 = dsuid2 = dsuid3 = meter1DSID = DSUID_NULL;
  meter1DSID.id[DSUID_SIZE - 1] = 10;
  dsuid1.id[DSUID_SIZE - 1] = 1;
  dsuid2.id[DSUID_SIZE - 1] = 2;
  dsuid3.id[DSUID_SIZE - 1] = 3;

  boost::shared_ptr<DSMeter> meter1 = apt.allocateDSMeter(meter1DSID);
  boost::shared_ptr<Zone> zone1 = apt.allocateZone(1);
  boost::shared_ptr<Device> dev1 = apt.allocateDevice(dsuid1);
  dev1->setShortAddress(1);
  dev1->setDSMeter(meter1);
  dev1->setZoneID(1);
  dev1->addToGroup(1);
  DeviceReference devRef1 = DeviceReference(dev1, &apt);
  zone1->addDevice(devRef1);
  boost::shared_ptr<Device> dev2 = apt.allocateDevice(dsuid2);
  dev2->setShortAddress(2);
  dev2->setDSMeter(meter1);
  dev2->setZoneID(1);
  dev2->addToGroup(1);
  DeviceReference devRef2 = DeviceReference(dev2, &apt);
  zone1->addDevice(devRef2);
  boost::shared_ptr<Device> dev3 = apt.allocateDevice(dsuid3);
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

  set.callScene(coTest, SAC_MANUAL, 5, "");
  BOOST_CHECK_EQUAL(actionInterface.getLog(), "callScene(1,5)callScene(2,5)");

  std::string setDescription = "addDevices(1,2)";
  manipulator.persistSet(set, setDescription);
  actionInterface.clearLog();
  set.callScene(coTest, SAC_MANUAL, 5, "");
  BOOST_CHECK_EQUAL(actionInterface.getLog(), "callScene(0,24,5)");

  manipulator.unpersistSet(setDescription);
  actionInterface.clearLog();
  set.callScene(coTest, SAC_MANUAL, 5, "");
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

BOOST_AUTO_TEST_CASE(testApartmentXML) {
  Apartment apt(NULL);
  PropertySystem propSys;
  apt.setPropertySystem(&propSys);

  ModelPersistence model(apt);

  std::string fileName = getTempDir() + "/apt.xml";
  std::string invalidBackup = getTempDir() + "/invalid.xml";
  std::ofstream ofs(fileName.c_str());
  ofs <<
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<config version=\"1\">\n"
    "  <apartment>\n"
    "    <name>UAZ</name>\n"
    "  </apartment>\n"
    "  <devices>\n"
    "    <device dsid=\"3504175fe0000000ffc00024\" firstSeen=\"1327676779\" isPresent=\"1\" lastKnownDSMeter=\"3504175fe0000000ffc00010\" lastKnownShortAddress=\"24\" lastKnownZoneID=\"1\">\n"
    "      <name>UAZ-469</name>\n"
    "      <properties>\n"
    "        <property archive=\"true\" name=\"my/custom/setting\" readable=\"true\" type=\"integer\" writeable=\"true\">\n"
    "          <value>31512</value>\n"
    "        </property>\n"
    "      </properties>\n"
    "    </device>\n"
    "  </devices>\n"
    "  <zones>\n"
    "    <zone id=\"1\">\n"
    "      <name>garazh</name>\n"
    "    </zone>\n"
    "  </zones>\n"
    "  <dsMeters>\n"
    "    <dsMeter id=\"3504175fe0000000ffc00010\">\n"
    "      <name>UMZ-451</name>\n"
    "      <datamodelHash>559038737</datamodelHash>\n"
    "      <datamodelModification>0</datamodelModification>\n"
    "    </dsMeter>\n"
    "  </dsMeters>\n"
    "</config>";

  ofs.close();

  Logger::getInstance()->log("Checking apartment XML");

  model.readConfigurationFromXML(fileName, invalidBackup);
  BOOST_CHECK_EQUAL(apt.getName(), "UAZ");

  boost::shared_ptr<Device> dev = apt.getDeviceByName("UAZ-469");
  BOOST_CHECK(dev != NULL);

  boost::shared_ptr<DSMeter> meter = apt.getDSMeter("UMZ-451");
  BOOST_CHECK(meter != NULL);

  boost::shared_ptr<Zone> zone = apt.getZone("garazh");
  BOOST_CHECK(zone != NULL);

  BOOST_CHECK_EQUAL(dev->getZoneID(), 1);
  BOOST_CHECK_EQUAL(zone->getID(), 1);

  PropertyNodePtr prop = dev->getPropertyNode();
  BOOST_CHECK(prop != NULL);
  BOOST_CHECK_EQUAL(prop->getProperty("my/custom/setting")->getIntegerValue(),
                    31512);

  boost::filesystem::remove_all(fileName);
}


BOOST_AUTO_TEST_SUITE_END()
