/*
 *  datetoolstests.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/17/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */


#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>


#include "../core/datetools.h"

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

using namespace dss;

class DateToolsTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(DateToolsTest);
  CPPUNIT_TEST(testSimpleDates);
  CPPUNIT_TEST(testAddingComparing);
  CPPUNIT_TEST_SUITE_END();
  
public:
  void setUp(void) {}
  void tearDown(void) {} 
  
protected:
  void testSimpleDates(void) { 
    DateTime dt;
    dt.Clear();
    CPPUNIT_ASSERT_EQUAL(dt.GetDay(), 0);
    CPPUNIT_ASSERT_EQUAL(dt.GetMonth(), 0);
    CPPUNIT_ASSERT_EQUAL(dt.GetYear(), 1900);
    CPPUNIT_ASSERT_EQUAL(dt.GetSecond(), 0);
    CPPUNIT_ASSERT_EQUAL(dt.GetMinute(), 0);
    CPPUNIT_ASSERT_EQUAL(dt.GetHour(), 0);
   
    dt.SetDate(1, January, 2008);
    dt.Validate();
    
    CPPUNIT_ASSERT_EQUAL(dt.GetWeekday(), Tuesday);
    
    DateTime dt2 = dt.AddDay(1);

    CPPUNIT_ASSERT_EQUAL(dt2.GetWeekday(), Wednesday);
  }
  
  void testAddingComparing(void) {
    DateTime dt;
    DateTime dt2 = dt.AddHour(1);
    
    assert(dt2.After(dt));
    assert(!dt2.Before(dt));
    assert(dt.Before(dt2));
    assert(!dt.After(dt2));

    CPPUNIT_ASSERT(dt2.After(dt));
    CPPUNIT_ASSERT(!dt2.Before(dt));
    CPPUNIT_ASSERT(dt.Before(dt2));
    CPPUNIT_ASSERT(!dt.After(dt2)); 
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(DateToolsTest);

