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


protected:

  void testSet() {
    Apartment apt(NULL);
    apt.Initialize();

    Device& dev1 = apt.AllocateDevice(dsid_t(0,1));
    dev1.SetShortAddress(1);
    Device& dev2 = apt.AllocateDevice(dsid_t(0,2));
    dev2.SetShortAddress(2);
    Device& dev3 = apt.AllocateDevice(dsid_t(0,3));
    dev3.SetShortAddress(3);
    Device& dev4 = apt.AllocateDevice(dsid_t(0,4));
    dev4.SetShortAddress(4);

    Set allDevices = apt.GetDevices();

    CPPUNIT_ASSERT_EQUAL(dev1, allDevices.GetByBusID(1).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev2, allDevices.GetByBusID(2).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allDevices.GetByBusID(3).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allDevices.GetByBusID(4).GetDevice());

    Set setdev1 = Set(dev1);

    Set allMinusDev1 = allDevices.Remove(setdev1);

    CPPUNIT_ASSERT_EQUAL(3, allMinusDev1.Length());

    CPPUNIT_ASSERT_EQUAL(false, allMinusDev1.Contains(dev1));

    /*
    try {
      allMinusDev1.GetByBusID(1);
      CPPUNIT_ASSERT(false);
    } catch(ItemNotFoundException& e) {
      CPPUNIT_ASSERT(true);
    }*/

    CPPUNIT_ASSERT_EQUAL(dev2, allMinusDev1.GetByBusID(2).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allMinusDev1.GetByBusID(3).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allMinusDev1.GetByBusID(4).GetDevice());

    // check that the other sets are not afected by our operation
    CPPUNIT_ASSERT_EQUAL(1, setdev1.Length());
    CPPUNIT_ASSERT_EQUAL(dev1, setdev1.GetByBusID(1).GetDevice());

    CPPUNIT_ASSERT_EQUAL(4, allDevices.Length());
    CPPUNIT_ASSERT_EQUAL(dev1, allDevices.GetByBusID(1).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev2, allDevices.GetByBusID(2).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allDevices.GetByBusID(3).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allDevices.GetByBusID(4).GetDevice());

    Set allRecombined = allMinusDev1.Combine(setdev1);

    CPPUNIT_ASSERT_EQUAL(4, allRecombined.Length());
    CPPUNIT_ASSERT_EQUAL(dev1, allRecombined.GetByBusID(1).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev2, allRecombined.GetByBusID(2).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allRecombined.GetByBusID(3).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allRecombined.GetByBusID(4).GetDevice());

    CPPUNIT_ASSERT_EQUAL(1, setdev1.Length());
    CPPUNIT_ASSERT_EQUAL(dev1, setdev1.GetByBusID(1).GetDevice());

    allRecombined = allRecombined.Combine(setdev1);
    CPPUNIT_ASSERT_EQUAL(4, allRecombined.Length());

    allRecombined = allRecombined.Combine(setdev1);
    CPPUNIT_ASSERT_EQUAL(4, allRecombined.Length());

    allRecombined = allRecombined.Combine(allRecombined);
    CPPUNIT_ASSERT_EQUAL(4, allRecombined.Length());
    CPPUNIT_ASSERT_EQUAL(dev1, allRecombined.GetByBusID(1).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev2, allRecombined.GetByBusID(2).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allRecombined.GetByBusID(3).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allRecombined.GetByBusID(4).GetDevice());

    allMinusDev1 = allRecombined;
    allMinusDev1.RemoveDevice(dev1);
    CPPUNIT_ASSERT_EQUAL(3, allMinusDev1.Length());
    CPPUNIT_ASSERT_EQUAL(dev2, allMinusDev1.GetByBusID(2).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allMinusDev1.GetByBusID(3).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allMinusDev1.GetByBusID(4).GetDevice());
  } // testSet

  void testSetBuilder() {
    Apartment apt(NULL);
    apt.Initialize();

    Device& dev1 = apt.AllocateDevice(dsid_t(0,1));
    dev1.SetShortAddress(1);
    dev1.GetGroupBitmask().set(GroupIDYellow - 1);
    Device& dev2 = apt.AllocateDevice(dsid_t(0,2));
    dev2.SetShortAddress(2);
    dev2.GetGroupBitmask().set(GroupIDCyan - 1);
    Device& dev3 = apt.AllocateDevice(dsid_t(0,3));
    dev3.SetShortAddress(3);
    Device& dev4 = apt.AllocateDevice(dsid_t(0,4));
    dev4.SetShortAddress(4);

    SetBuilder builder;
    Set builderTest = builder.BuildSet("yellow", &apt.GetZone(0));

    CPPUNIT_ASSERT_EQUAL(1, builderTest.Length());
    CPPUNIT_ASSERT_EQUAL(dev1, builderTest.Get(0).GetDevice());

    builderTest = builder.BuildSet("cyan", &apt.GetZone(0));

    CPPUNIT_ASSERT_EQUAL(1, builderTest.Length());
    CPPUNIT_ASSERT_EQUAL(dev2, builderTest.Get(0).GetDevice());

    builderTest = builder.BuildSet("dsid(1)", &apt.GetZone(0));
    CPPUNIT_ASSERT_EQUAL(1, builderTest.Length());
    CPPUNIT_ASSERT_EQUAL(dev1, builderTest.Get(0).GetDevice());

    builderTest = builder.BuildSet("yellow.dsid(1)", &apt.GetZone(0));
    CPPUNIT_ASSERT_EQUAL(1, builderTest.Length());
    CPPUNIT_ASSERT_EQUAL(dev1, builderTest.Get(0).GetDevice());

  } // testSetBuilder

};

CPPUNIT_TEST_SUITE_REGISTRATION(ModelTest);



