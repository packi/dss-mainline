/*
 *  modeltests.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/21/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "core/model.h"
#include "core/setbuilder.h"
#include "core/ds485const.h"

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

using namespace dss;

class ModelTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ModelTest);
  CPPUNIT_TEST(testSet);
  CPPUNIT_TEST(testSetBuilder);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {}

  ModelTest() {}


protected:

  void testSet() {
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

    CPPUNIT_ASSERT_EQUAL(dev1, allDevices.getByBusID(1).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev2, allDevices.getByBusID(2).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allDevices.getByBusID(3).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allDevices.getByBusID(4).getDevice());

    Set setdev1 = Set(dev1);

    Set allMinusDev1 = allDevices.remove(setdev1);

    CPPUNIT_ASSERT_EQUAL(3, allMinusDev1.length());

    CPPUNIT_ASSERT_EQUAL(false, allMinusDev1.contains(dev1));

    /*
    try {
      allMinusDev1.getByBusID(1);
      CPPUNIT_ASSERT(false);
    } catch(ItemNotFoundException& e) {
      CPPUNIT_ASSERT(true);
    }*/

    CPPUNIT_ASSERT_EQUAL(dev2, allMinusDev1.getByBusID(2).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allMinusDev1.getByBusID(3).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allMinusDev1.getByBusID(4).getDevice());

    // check that the other sets are not afected by our operation
    CPPUNIT_ASSERT_EQUAL(1, setdev1.length());
    CPPUNIT_ASSERT_EQUAL(dev1, setdev1.getByBusID(1).getDevice());

    CPPUNIT_ASSERT_EQUAL(4, allDevices.length());
    CPPUNIT_ASSERT_EQUAL(dev1, allDevices.getByBusID(1).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev2, allDevices.getByBusID(2).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allDevices.getByBusID(3).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allDevices.getByBusID(4).getDevice());

    Set allRecombined = allMinusDev1.combine(setdev1);

    CPPUNIT_ASSERT_EQUAL(4, allRecombined.length());
    CPPUNIT_ASSERT_EQUAL(dev1, allRecombined.getByBusID(1).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev2, allRecombined.getByBusID(2).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allRecombined.getByBusID(3).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allRecombined.getByBusID(4).getDevice());

    CPPUNIT_ASSERT_EQUAL(1, setdev1.length());
    CPPUNIT_ASSERT_EQUAL(dev1, setdev1.getByBusID(1).getDevice());

    allRecombined = allRecombined.combine(setdev1);
    CPPUNIT_ASSERT_EQUAL(4, allRecombined.length());

    allRecombined = allRecombined.combine(setdev1);
    CPPUNIT_ASSERT_EQUAL(4, allRecombined.length());

    allRecombined = allRecombined.combine(allRecombined);
    CPPUNIT_ASSERT_EQUAL(4, allRecombined.length());
    CPPUNIT_ASSERT_EQUAL(dev1, allRecombined.getByBusID(1).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev2, allRecombined.getByBusID(2).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allRecombined.getByBusID(3).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allRecombined.getByBusID(4).getDevice());

    allMinusDev1 = allRecombined;
    allMinusDev1.removeDevice(dev1);
    CPPUNIT_ASSERT_EQUAL(3, allMinusDev1.length());
    CPPUNIT_ASSERT_EQUAL(dev2, allMinusDev1.getByBusID(2).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allMinusDev1.getByBusID(3).getDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allMinusDev1.getByBusID(4).getDevice());
  } // testSet

  void testSetBuilder() {
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

    SetBuilder builder;
    Set builderTest = builder.buildSet("yellow", &apt.getZone(0));

    CPPUNIT_ASSERT_EQUAL(1, builderTest.length());
    CPPUNIT_ASSERT_EQUAL(dev1, builderTest.get(0).getDevice());

    builderTest = builder.buildSet("cyan", &apt.getZone(0));

    CPPUNIT_ASSERT_EQUAL(1, builderTest.length());
    CPPUNIT_ASSERT_EQUAL(dev2, builderTest.get(0).getDevice());

    builderTest = builder.buildSet("dsid(1)", &apt.getZone(0));
    CPPUNIT_ASSERT_EQUAL(1, builderTest.length());
    CPPUNIT_ASSERT_EQUAL(dev1, builderTest.get(0).getDevice());

    builderTest = builder.buildSet("yellow.dsid(1)", &apt.getZone(0));
    CPPUNIT_ASSERT_EQUAL(1, builderTest.length());
    CPPUNIT_ASSERT_EQUAL(dev1, builderTest.get(0).getDevice());

  } // testSetBuilder

};

CPPUNIT_TEST_SUITE_REGISTRATION(ModelTest);



