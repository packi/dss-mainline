/*
 *  datetoolstests.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/17/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <cassert>

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
  CPPUNIT_TEST(testDynamicScheduleICal);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {}

protected:
  void testSimpleDates(void) {
    DateTime dt;
    dt.clear();
    CPPUNIT_ASSERT_EQUAL(dt.getDay(), 0);
    CPPUNIT_ASSERT_EQUAL(dt.getMonth(), 0);
    CPPUNIT_ASSERT_EQUAL(dt.getYear(), 1900);
    CPPUNIT_ASSERT_EQUAL(dt.getSecond(), 0);
    CPPUNIT_ASSERT_EQUAL(dt.getMinute(), 0);
    CPPUNIT_ASSERT_EQUAL(dt.getHour(), 0);

    dt.setDate(1, January, 2008);
    dt.validate();

    CPPUNIT_ASSERT_EQUAL(dt.getWeekday(), Tuesday);

    DateTime dt2 = dt.addDay(1);

    CPPUNIT_ASSERT_EQUAL(dt2.getWeekday(), Wednesday);

    DateTime dt3 = dt2.addDay(-2);

    CPPUNIT_ASSERT_EQUAL(dt3.getWeekday(), Monday);
    CPPUNIT_ASSERT_EQUAL(dt3.getYear(), 2007);
  }

  void testAddingComparing(void) {
    DateTime dt;
    DateTime dt2 = dt.addHour(1);

    assert(dt2.after(dt));
    assert(!dt2.before(dt));
    assert(dt.before(dt2));
    assert(!dt.after(dt2));

    CPPUNIT_ASSERT(dt2.after(dt));
    CPPUNIT_ASSERT(!dt2.before(dt));
    CPPUNIT_ASSERT(dt.before(dt2));
    CPPUNIT_ASSERT(!dt.after(dt2));
  }

  void testStaticSchedule() {
    DateTime when;
    when.setDate(15, April, 2008);
    when.setTime(10, 00, 00);

    StaticSchedule schedule(when);

    CPPUNIT_ASSERT_EQUAL(when, schedule.getNextOccurence(when.addMinute(-1)));
    CPPUNIT_ASSERT_EQUAL(DateTime::NullDate, schedule.getNextOccurence(when.addMinute(1)));

    vector<DateTime> schedList = schedule.getOccurencesBetween(when.addMinute(-1), when.addMinute(1));
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), schedList.size());

    schedList = schedule.getOccurencesBetween(when.addMinute(1), when.addMinute(2));
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), schedList.size());
  }

  void testDynamicSchedule() {
    DateTime when;
    when.setDate(15, April, 2008);
    when.setTime(10, 00, 00);

    RepeatingSchedule schedule(Minutely, 5, when);

    CPPUNIT_ASSERT_EQUAL(when,              schedule.getNextOccurence(when.addSeconds(-1)));
    CPPUNIT_ASSERT_EQUAL(when.addMinute(5), schedule.getNextOccurence(when.addSeconds(1)));

    vector<DateTime> v = schedule.getOccurencesBetween(when, when.addMinute(10));

    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), v.size());
    CPPUNIT_ASSERT_EQUAL(when, v[0]);
    CPPUNIT_ASSERT_EQUAL(when.addMinute(5), v[1]);
    CPPUNIT_ASSERT_EQUAL(when.addMinute(10), v[2]);
  }

  void testDynamicScheduleICal() {
    ICalSchedule sched("FREQ=MINUTELY;INTERVAL=2", "20080505T080000Z");

    DateTime startTime = DateTime::fromISO("20080505T080000Z");

    DateTime firstRecurr = sched.getNextOccurence(startTime);
    CPPUNIT_ASSERT_EQUAL(startTime, firstRecurr);

    DateTime startPlusOneSec = startTime.addSeconds(1);
    DateTime nextRecurr = sched.getNextOccurence(startPlusOneSec);
    CPPUNIT_ASSERT_EQUAL(startTime.addMinute(2), nextRecurr);
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION(DateToolsTest);

