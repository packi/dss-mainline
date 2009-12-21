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
#include "unix/ds485proxy.h"
#include "core/sim/dssim.h"
#include "core/dss.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(Model)

BOOST_AUTO_TEST_CASE(testApartmentAllocateDeviceReturnsTheSameDeviceForDSID) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.setName("dev1");
  dev1.setModulatorID(1);

  Device& dev2 = apt.allocateDevice(dsid_t(0,1));
  BOOST_CHECK_EQUAL(dev1.getShortAddress(), dev2.getShortAddress());
  BOOST_CHECK_EQUAL(dev1.getName(), dev2.getName());
  BOOST_CHECK_EQUAL(dev1.getModulatorID(), dev2.getModulatorID());
} // testApartmentAllocateDeviceReturnsTheSameDeviceForDSID

BOOST_AUTO_TEST_CASE(testApartmentGetDeviceByShortAddress) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  Modulator& mod = apt.allocateModulator(dsid_t(0,2));
  mod.setBusID(1);

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.setName("dev1");
  dev1.setModulatorID(1);

  BOOST_CHECK_EQUAL("dev1", apt.getDeviceByShortAddress(mod, 1).getName());
} // testApartmentGetDeviceByShortAddress

BOOST_AUTO_TEST_CASE(testApartmentGetDeviceByName) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setName("dev1");

  BOOST_CHECK_EQUAL("dev1", apt.getDeviceByName("dev1").getName());
} // testApartmentGetDeviceByName

BOOST_AUTO_TEST_CASE(testApartmentGetModulatorByName) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  Modulator& mod = apt.allocateModulator(dsid_t(0,2));
  mod.setName("mod1");

  BOOST_CHECK_EQUAL("mod1", apt.getModulator("mod1").getName());
} // testApartmentGetModulatorByName

BOOST_AUTO_TEST_CASE(testApartmentGetModulatorByBusID) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  Modulator& mod = apt.allocateModulator(dsid_t(0,2));
  mod.setBusID(1);

  BOOST_CHECK_EQUAL(1, apt.getModulatorByBusID(1).getBusID());
} // testApartmentGetModulatorByBusID

BOOST_AUTO_TEST_CASE(testZoneMoving) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  DeviceReference devRef1(dev1, &apt);
  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  DeviceReference devRef2(dev2, &apt);
  Device& dev3 = apt.allocateDevice(dsid_t(0,3));
  dev3.setShortAddress(3);
  DeviceReference devRef3(dev3, &apt);
  Device& dev4 = apt.allocateDevice(dsid_t(0,4));
  dev4.setShortAddress(4);
  DeviceReference devRef4(dev4, &apt);

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
  Apartment apt(NULL, NULL);
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
  Apartment apt(NULL, NULL);
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

  builderTest = builder.buildSet("empty().addDevices(1,2,3)", &apt.getZone(0));
  BOOST_CHECK_EQUAL(3, builderTest.length());
} // testSetBuilder

BOOST_AUTO_TEST_CASE(testRemoval) {
  Apartment apt(NULL, NULL);
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

BOOST_AUTO_TEST_CASE(testCallScenePropagation) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485Proxy proxy(NULL, &apt);
  proxy.setInitializeDS485Controller(false);
  proxy.initialize();

  proxy.start();
  apt.start();
  while(apt.isInitializing()) {
    sleepMS(100);
  }

  Modulator& mod = apt.allocateModulator(dsid_t(0,3));
  mod.setBusID(76);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setName("dev1");
  dev1.setShortAddress(1);
  dev1.setModulatorID(76);
  DeviceReference devRef1(dev1, &apt);
  mod.addDevice(devRef1);
  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setName("dev2");
  dev2.setShortAddress(2);
  dev2.setModulatorID(76);

//  dev1.callScene(Scene1);
  proxy.sendCommand(cmdCallScene, dev1, Scene1);
  sleepMS(500);
  BOOST_CHECK_EQUAL(Scene1, dev1.getLastCalledScene());
  proxy.sendCommand(cmdCallScene, apt.getZone(0), apt.getGroup(0), Scene2);
  sleepMS(500);
  BOOST_CHECK_EQUAL(Scene2, dev1.getLastCalledScene());
  BOOST_CHECK_EQUAL(Scene2, dev2.getLastCalledScene());
  proxy.terminate();
  apt.terminate();
  sleepMS(1500);
  DSS::teardown();
} // testCallScenePropagation


  /** Interface to be implemented by any implementation of the DS485 interface */
  class DS485InterfaceTest : public DS485Interface {
  public:
    /** Returns true when the interface is ready to transmit user generated DS485Packets */
    virtual bool isReady() { return true; }

    virtual void sendFrame(DS485CommandFrame& _frame) {}

    //------------------------------------------------ Specialized Commands (system)
    /** Returns an std::vector containing the modulator-spec of all modulators present. */
    virtual std::vector<ModulatorSpec_t> getModulators() { return std::vector<ModulatorSpec_t>(); }

    /** Returns the modulator-spec for a modulator */
    virtual ModulatorSpec_t getModulatorSpec(const int _modulatorID) { return ModulatorSpec_t(); }

    /** Returns a std::vector conatining the zone-ids of the specified modulator */
    virtual std::vector<int> getZones(const int _modulatorID) { return std::vector<int>(); }
    /** Returns the count of the zones of the specified modulator */
    virtual int getZoneCount(const int _modulatorID) { return 0; }
    /** Returns the bus-ids of the devices present in the given zone of the specified modulator */
    virtual std::vector<int> getDevicesInZone(const int _modulatorID, const int _zoneID) { return std::vector<int>(); }
    /** Returns the count of devices present in the given zone of the specified modulator */
    virtual int getDevicesCountInZone(const int _modulatorID, const int _zoneID) { return 0; }

    /** Adds the given device to the specified zone. */
    virtual void setZoneID(const int _modulatorID, const devid_t _deviceID, const int _zoneID) {}

    /** Creates a new Zone on the given modulator */
    virtual void createZone(const int _modulatorID, const int _zoneID) {}

    /** Removes the zone \a _zoneID on the modulator \a _modulatorID */
    virtual void removeZone(const int _modulatorID, const int _zoneID) {}

    /** Returns the count of groups present in the given zone of the specifid modulator */
    virtual int getGroupCount(const int _modulatorID, const int _zoneID) { return 0; }
    /** Returns the a std::vector containing the group-ids of the given zone on the specified modulator */
    virtual std::vector<int> getGroups(const int _modulatorID, const int _zoneID) { return std::vector<int>(); }
    /** Returns the count of devices present in the given group */
    virtual int getDevicesInGroupCount(const int _modulatorID, const int _zoneID, const int _groupID) { return 0; }
    /** Returns a std::vector containing the bus-ids of the devices present in the given group */
    virtual std::vector<int> getDevicesInGroup(const int _modulatorID, const int _zoneID, const int _groupID) { return std::vector<int>(); }

    virtual std::vector<int> getGroupsOfDevice(const int _modulatorID, const int _deviceID) { return std::vector<int>(); }

    /** Adds a device to a given group */
    virtual void addToGroup(const int _modulatorID, const int _groupID, const int _deviceID) {}
    /** Removes a device from a given group */
    virtual void removeFromGroup(const int _modulatorID, const int _groupID, const int _deviceID) {}

    /** Adds a user group */
    virtual int addUserGroup(const int _modulatorID) { return 0; }
    /** Removes a user group */
    virtual void removeUserGroup(const int _modulatorID, const int _groupID) {}

    /** Returns the DSID of a given device */
    virtual dsid_t getDSIDOfDevice(const int _modulatorID, const int _deviceID) { return NullDSID; }
    /** Returns the DSID of a given modulator */
    virtual dsid_t getDSIDOfModulator(const int _modulatorID) { return NullDSID; }

    virtual int getLastCalledScene(const int _modulatorID, const int _zoneID, const int _groupID) { return 0;}

    //------------------------------------------------ Metering

    /** Returns the current power-consumption in mW */
    virtual unsigned long getPowerConsumption(const int _modulatorID) { return 0; }

    /** Returns the meter value in Wh */
    virtual unsigned long getEnergyMeterValue(const int _modulatorID) { return 0; }

    virtual bool getEnergyBorder(const int _modulatorID, int& _lower, int& _upper) { _lower = 0; _upper = 0; return false; }

    //------------------------------------------------ UDI
    virtual uint8_t dSLinkSend(const int _modulatorID, devid_t _devAdr, uint8_t _value, uint8_t _flags) { return 0; }

    //------------------------------------------------ Device manipulation

    DS485Command m_LastCommand;
    DS485Command getLastCommand() const { return m_LastCommand; }
    void setLastCommand(DS485Command _value) { m_LastCommand = _value; }
    virtual std::vector<int> sendCommand(DS485Command _cmd, const Set& _set, int _param = -1) { return std::vector<int>(); }
    virtual std::vector<int> sendCommand(DS485Command _cmd, const Device& _device, int _param = -1) {
      setLastCommand(_cmd);
      return std::vector<int>();
    }
    virtual std::vector<int> sendCommand(DS485Command _cmd, devid_t _id, uint8_t _modulatorID, int _param = -1) { return std::vector<int>(); }
    virtual std::vector<int> sendCommand(DS485Command _cmd, const Zone& _zone, Group& _group, int _param = -1) { return std::vector<int>(); }
    virtual std::vector<int> sendCommand(DS485Command _cmd, const Zone& _zone, uint8_t _groupID, int _param = -1) { return std::vector<int>(); }

    virtual void setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size) {}
    virtual int getSensorValue(const Device& _device, const int _sensorID) { return 0; }
  };


BOOST_AUTO_TEST_CASE(testTurnOn) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOff);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.turnOn();
  BOOST_CHECK_EQUAL(cmdTurnOn, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOff);
  DeviceReference devRef1(dev1, &apt);
  devRef1.turnOn();
  BOOST_CHECK_EQUAL(cmdTurnOn, proxy.getLastCommand());
}

BOOST_AUTO_TEST_CASE(testTurnOff) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOn);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.turnOff();
  BOOST_CHECK_EQUAL(cmdTurnOff, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOn);
  DeviceReference devRef1(dev1, &apt);
  devRef1.turnOff();
  BOOST_CHECK_EQUAL(cmdTurnOff, proxy.getLastCommand());
}

BOOST_AUTO_TEST_CASE(testDisable) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOff);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.disable();
  BOOST_CHECK_EQUAL(cmdDisable, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOff);
  DeviceReference devRef1(dev1, &apt);
  devRef1.disable();
  BOOST_CHECK_EQUAL(cmdDisable, proxy.getLastCommand());
}

BOOST_AUTO_TEST_CASE(testEnable) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOff);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.enable();
  BOOST_CHECK_EQUAL(cmdEnable, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOff);
  DeviceReference devRef1(dev1, &apt);
  devRef1.enable();
  BOOST_CHECK_EQUAL(cmdEnable, proxy.getLastCommand());
}

BOOST_AUTO_TEST_CASE(testIncreaseValue) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOff);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.increaseValue();
  BOOST_CHECK_EQUAL(cmdIncreaseValue, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOff);
  DeviceReference devRef1(dev1, &apt);
  devRef1.increaseValue();
  BOOST_CHECK_EQUAL(cmdIncreaseValue, proxy.getLastCommand());
}

BOOST_AUTO_TEST_CASE(testDecreaseValue) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOff);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.decreaseValue();
  BOOST_CHECK_EQUAL(cmdDecreaseValue, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOff);
  DeviceReference devRef1(dev1, &apt);
  devRef1.decreaseValue();
  BOOST_CHECK_EQUAL(cmdDecreaseValue, proxy.getLastCommand());
}

BOOST_AUTO_TEST_CASE(testStartDimUp) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOff);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.startDim(true);
  BOOST_CHECK_EQUAL(cmdStartDimUp, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOff);
  DeviceReference devRef1(dev1, &apt);
  devRef1.startDim(true);
  BOOST_CHECK_EQUAL(cmdStartDimUp, proxy.getLastCommand());
}

BOOST_AUTO_TEST_CASE(testStartDimDown) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOff);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.startDim(false);
  BOOST_CHECK_EQUAL(cmdStartDimDown, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOff);
  DeviceReference devRef1(dev1, &apt);
  devRef1.startDim(false);
  BOOST_CHECK_EQUAL(cmdStartDimDown, proxy.getLastCommand());
}

BOOST_AUTO_TEST_CASE(testEndDim) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOff);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.endDim();
  BOOST_CHECK_EQUAL(cmdStopDim, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOff);
  DeviceReference devRef1(dev1, &apt);
  devRef1.endDim();
  BOOST_CHECK_EQUAL(cmdStopDim, proxy.getLastCommand());
}

BOOST_AUTO_TEST_CASE(testCallScene) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOff);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.callScene(Scene1);
  BOOST_CHECK_EQUAL(cmdCallScene, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOff);
  DeviceReference devRef1(dev1, &apt);
  devRef1.callScene(Scene1);
  BOOST_CHECK_EQUAL(cmdCallScene, proxy.getLastCommand());
}

BOOST_AUTO_TEST_CASE(testSaveScene) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOff);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.saveScene(Scene1);
  BOOST_CHECK_EQUAL(cmdSaveScene, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOff);
  DeviceReference devRef1(dev1, &apt);
  devRef1.saveScene(Scene1);
  BOOST_CHECK_EQUAL(cmdSaveScene, proxy.getLastCommand());
}

BOOST_AUTO_TEST_CASE(testUndoScene) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOff);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.undoScene(Scene1);
  BOOST_CHECK_EQUAL(cmdUndoScene, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOff);
  DeviceReference devRef1(dev1, &apt);
  devRef1.undoScene(Scene1);
  BOOST_CHECK_EQUAL(cmdUndoScene, proxy.getLastCommand());
}

BOOST_AUTO_TEST_CASE(testNextScene) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOff);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.nextScene();
  BOOST_CHECK_EQUAL(cmdCallScene, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOff);
  DeviceReference devRef1(dev1, &apt);
  devRef1.nextScene();
  BOOST_CHECK_EQUAL(cmdCallScene, proxy.getLastCommand());
}

BOOST_AUTO_TEST_CASE(testPreviousScene) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  apt.setDS485Interface(&proxy);

  proxy.setLastCommand(cmdTurnOff);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.previousScene();
  BOOST_CHECK_EQUAL(cmdCallScene, proxy.getLastCommand());
  proxy.setLastCommand(cmdTurnOff);
  DeviceReference devRef1(dev1, &apt);
  devRef1.previousScene();
  BOOST_CHECK_EQUAL(cmdCallScene, proxy.getLastCommand());
}

BOOST_AUTO_TEST_SUITE_END()
