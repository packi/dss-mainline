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
#include "core/ds485/ds485busrequestdispatcher.h"

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
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  proxy.setInitializeDS485Controller(false);
  proxy.initialize();
  apt.setBusRequestDispatcher(&dispatcher);

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

  dev1.callScene(Scene1);
  sleepMS(500);
  BOOST_CHECK_EQUAL(Scene1, dev1.getLastCalledScene());
  apt.getZone(0).callScene(Scene2);
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

    //------------------------------------------------ Device    
    virtual uint16_t deviceGetParameterValue(devid_t _id, uint8_t _modulatorID, int _paramID) { return 0; }
    virtual uint16_t deviceGetFunctionID(devid_t _id, uint8_t _modulatorID) { return 0; }

    //------------------------------------------------ Device manipulation
    virtual void setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size) {}
    virtual int getSensorValue(const Device& _device, const int _sensorID) { return 0; }
  private:
    int m_LastFunctionID;
    uint16_t m_Parameter1;
    uint16_t m_Parameter2;
    uint16_t m_Parameter3;
  };


BOOST_AUTO_TEST_CASE(testTurnOnDevice) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.turnOn();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  BOOST_CHECK_EQUAL(SceneMax, proxy.getParameter2());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.turnOn();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  BOOST_CHECK_EQUAL(SceneMax, proxy.getParameter2());
}

BOOST_AUTO_TEST_CASE(testTurnOnGroup) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Group& group = apt.getGroup(1);
  group.turnOn();
  BOOST_CHECK_EQUAL(FunctionGroupCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x0, proxy.getParameter1()); // zone 0
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter2()); // group 1
  BOOST_CHECK_EQUAL(SceneMax, proxy.getParameter3());
  proxy.setLastFunctionID(-1);
}

BOOST_AUTO_TEST_CASE(testTurnOffDevice) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.turnOff();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  BOOST_CHECK_EQUAL(SceneMin, proxy.getParameter2());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.turnOff();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  BOOST_CHECK_EQUAL(SceneMin, proxy.getParameter2());
}

BOOST_AUTO_TEST_CASE(testTurnOffGroup) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Group& group = apt.getGroup(1);
  group.turnOff();
  BOOST_CHECK_EQUAL(FunctionGroupCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x0, proxy.getParameter1()); // zone 0
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter2()); // group 1
  BOOST_CHECK_EQUAL(SceneMin, proxy.getParameter3());
}

BOOST_AUTO_TEST_CASE(testEnableDevice) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.enable();
  BOOST_CHECK_EQUAL(FunctionDeviceEnable, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.enable();
  BOOST_CHECK_EQUAL(FunctionDeviceEnable, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
}

BOOST_AUTO_TEST_CASE(testDisableDevice) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.disable();
  BOOST_CHECK_EQUAL(FunctionDeviceDisable, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.disable();
  BOOST_CHECK_EQUAL(FunctionDeviceDisable, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
}

BOOST_AUTO_TEST_CASE(testIncreaseValueDevice) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.increaseValue();
  BOOST_CHECK_EQUAL(FunctionDeviceIncreaseValue, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.increaseValue();
  BOOST_CHECK_EQUAL(FunctionDeviceIncreaseValue, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
}

BOOST_AUTO_TEST_CASE(testDecreaseValueDevice) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.decreaseValue();
  BOOST_CHECK_EQUAL(FunctionDeviceDecreaseValue, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.decreaseValue();
  BOOST_CHECK_EQUAL(FunctionDeviceDecreaseValue, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
}

BOOST_AUTO_TEST_CASE(testDecreaseValueZone) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  Zone& zone = apt.allocateZone(1);

  proxy.setLastFunctionID(-1);
  zone.decreaseValue();
  BOOST_CHECK_EQUAL(FunctionGroupDecreaseValue, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1()); // zone id
  BOOST_CHECK_EQUAL(0x0, proxy.getParameter2()); // group id (broadcast)
}

BOOST_AUTO_TEST_CASE(testIncreaseValueZone) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  Zone& zone = apt.allocateZone(1);

  proxy.setLastFunctionID(-1);
  zone.increaseValue();
  BOOST_CHECK_EQUAL(FunctionGroupIncreaseValue, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1()); // zone id
  BOOST_CHECK_EQUAL(0x0, proxy.getParameter2()); // group id (broadcast)
}

BOOST_AUTO_TEST_CASE(testStartDimUp) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.startDim(true);
  BOOST_CHECK_EQUAL(FunctionDeviceStartDimInc, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.startDim(true);
  BOOST_CHECK_EQUAL(FunctionDeviceStartDimInc, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
}

BOOST_AUTO_TEST_CASE(testStartDimDown) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.startDim(false);
  BOOST_CHECK_EQUAL(FunctionDeviceStartDimDec, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.startDim(false);
  BOOST_CHECK_EQUAL(FunctionDeviceStartDimDec, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
}

BOOST_AUTO_TEST_CASE(testEndDim) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.endDim();
  BOOST_CHECK_EQUAL(FunctionDeviceEndDim, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.endDim();
  BOOST_CHECK_EQUAL(FunctionDeviceEndDim, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
}

BOOST_AUTO_TEST_CASE(testCallSceneDevice) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.callScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter2());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.callScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter2());
}

BOOST_AUTO_TEST_CASE(testSaveSceneDevice) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.saveScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceSaveScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter2());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.saveScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceSaveScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter2());
}

BOOST_AUTO_TEST_CASE(testUndoSceneDevice) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.undoScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceUndoScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter2());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.undoScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceUndoScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter2());
}

BOOST_AUTO_TEST_CASE(testCallSceneGroup) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Group& group = apt.getGroup(1);
  group.callScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionGroupCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x0, proxy.getParameter1()); // zone 0
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter2()); // group 1
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter3());
}

BOOST_AUTO_TEST_CASE(testSaveSceneGroup) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Group& group = apt.getGroup(1);
  group.saveScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionGroupSaveScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x0, proxy.getParameter1()); // zone 0
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter2()); // group 1
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter3());
}

BOOST_AUTO_TEST_CASE(testUndoSceneGroup) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Group& group = apt.getGroup(1);
  group.undoScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionGroupUndoScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x0, proxy.getParameter1()); // zone 0
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter2()); // group 1
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter3());
}

BOOST_AUTO_TEST_CASE(testCallSceneZone) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Zone& zone = apt.allocateZone(1);
  zone.callScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionGroupCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1()); // zone 1
  BOOST_CHECK_EQUAL(0x0, proxy.getParameter2()); // group 0
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter3());
}

BOOST_AUTO_TEST_CASE(testSaveSceneZone) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Zone& zone = apt.allocateZone(1);
  zone.saveScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionGroupSaveScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1()); // zone 1
  BOOST_CHECK_EQUAL(0x0, proxy.getParameter2()); // group 0
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter3());
}

BOOST_AUTO_TEST_CASE(testUndoSceneZone) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Zone& zone = apt.allocateZone(1);
  zone.undoScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionGroupUndoScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1()); // zone 1
  BOOST_CHECK_EQUAL(0x0, proxy.getParameter2()); // group 0
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter3());
}

BOOST_AUTO_TEST_CASE(callUndoSceneSet) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);
  proxy.setLastFunctionID(-1);

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  Set set;
  set.addDevice(dev1);
  set.callScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter2());
  proxy.setLastFunctionID(-1);
}

BOOST_AUTO_TEST_CASE(testSaveSceneSet) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);
  proxy.setLastFunctionID(-1);

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  Set set;
  set.addDevice(dev1);
  set.saveScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceSaveScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter2());
  proxy.setLastFunctionID(-1);
}

BOOST_AUTO_TEST_CASE(testUndoSceneSet) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);
  proxy.setLastFunctionID(-1);

  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  Device& dev2 = apt.allocateDevice(dsid_t(0,2));
  dev2.setShortAddress(2);
  Set set;
  set.addDevice(dev1);
  set.undoScene(Scene1);
  BOOST_CHECK_EQUAL(FunctionDeviceUndoScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  BOOST_CHECK_EQUAL(Scene1, proxy.getParameter2());
  proxy.setLastFunctionID(-1);
}

BOOST_AUTO_TEST_CASE(testNextSceneDevice) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.nextScene();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.nextScene();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
}

BOOST_AUTO_TEST_CASE(testPreviousSceneDevice) {
  Apartment apt(NULL, NULL);
  apt.initialize();

  DSModulatorSim modSim(NULL);
  DS485InterfaceTest proxy;
  DS485BusRequestDispatcher dispatcher;
  dispatcher.setProxy(&proxy);
  apt.setDS485Interface(&proxy);
  apt.setBusRequestDispatcher(&dispatcher);

  proxy.setLastFunctionID(-1);
  Device& dev1 = apt.allocateDevice(dsid_t(0,1));
  dev1.setShortAddress(1);
  dev1.previousScene();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
  proxy.setLastFunctionID(-1);
  DeviceReference devRef1(dev1, &apt);
  devRef1.previousScene();
  BOOST_CHECK_EQUAL(FunctionDeviceCallScene, proxy.getLastFunctionID());
  BOOST_CHECK_EQUAL(0x1, proxy.getParameter1());
}

BOOST_AUTO_TEST_SUITE_END()
