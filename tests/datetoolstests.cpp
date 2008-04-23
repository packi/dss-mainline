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
  CPPUNIT_TEST(testStaticSchedule);
  CPPUNIT_TEST(testDynamicSchedule);
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
    
    DateTime dt3 = dt2.AddDay(-2);
    
    CPPUNIT_ASSERT_EQUAL(dt3.GetWeekday(), Monday);
    CPPUNIT_ASSERT_EQUAL(dt3.GetYear(), 2007);
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
  
  void testStaticSchedule() {
    DateTime when;
    when.SetDate(15, April, 2008);
    when.SetTime(10, 00, 00);
    
    StaticSchedule schedule(when);
    
    CPPUNIT_ASSERT_EQUAL(when, schedule.GetNextOccurence(when.AddMinute(-1)));
    CPPUNIT_ASSERT_EQUAL(DateTime::NullDate, schedule.GetNextOccurence(when.AddMinute(1)));
    
    vector<DateTime> schedList = schedule.GetOccurencesBetween(when.AddMinute(-1), when.AddMinute(1));
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), schedList.size());
    
    schedList = schedule.GetOccurencesBetween(when.AddMinute(1), when.AddMinute(2));
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), schedList.size());
  }
  
  void testDynamicSchedule() {
    DateTime when;
    when.SetDate(15, April, 2008);
    when.SetTime(10, 00, 00);
    
    RepeatingSchedule schedule(Minutely, 5, when);
    
    CPPUNIT_ASSERT_EQUAL(when,              schedule.GetNextOccurence(when.AddSeconds(-1)));
    CPPUNIT_ASSERT_EQUAL(when.AddMinute(5), schedule.GetNextOccurence(when.AddSeconds(1)));
    
    vector<DateTime> v = schedule.GetOccurencesBetween(when, when.AddMinute(10));
    
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), v.size());
    CPPUNIT_ASSERT_EQUAL(when, v[0]);
    CPPUNIT_ASSERT_EQUAL(when.AddMinute(5), v[1]);
    CPPUNIT_ASSERT_EQUAL(when.AddMinute(10), v[2]);
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(DateToolsTest);

