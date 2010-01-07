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

#include "../core/datetools.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(DateTools)

BOOST_AUTO_TEST_CASE(testSimpleDates) {
  DateTime dt;
  dt.clear();
  BOOST_CHECK_EQUAL(dt.getDay(), 1);
  BOOST_CHECK_EQUAL(dt.getMonth(), 0);
  BOOST_CHECK_EQUAL(dt.getYear(), 1900);
  BOOST_CHECK_EQUAL(dt.getSecond(), 0);
  BOOST_CHECK_EQUAL(dt.getMinute(), 0);
  BOOST_CHECK_EQUAL(dt.getHour(), 0);

  dt.setDate(1, January, 2008);
  dt.validate();

  BOOST_CHECK_EQUAL(dt.getWeekday(), Tuesday);

  DateTime dt2 = dt.addDay(1);

  BOOST_CHECK_EQUAL(dt2.getWeekday(), Wednesday);

  DateTime dt3 = dt2.addDay(-2);

  BOOST_CHECK_EQUAL(dt3.getWeekday(), Monday);
  BOOST_CHECK_EQUAL(dt3.getYear(), 2007);
} // testSimpleDates

BOOST_AUTO_TEST_CASE(testAddingMonth) {
  DateTime dt;
  DateTime dt2 = dt.addMonth(1);

  assert(dt2.after(dt));
  assert(!dt2.before(dt));
  assert(dt.before(dt2));
  assert(!dt.after(dt2));

  BOOST_CHECK(dt2.after(dt));
  BOOST_CHECK(!dt2.before(dt));
  BOOST_CHECK(dt.before(dt2));
  BOOST_CHECK(!dt.after(dt2));
} // testAddingMonth

BOOST_AUTO_TEST_CASE(testISODate) {
  DateTime dt = DateTime::fromISO("20080506T080102Z");
  BOOST_CHECK_EQUAL(2008, dt.getYear());
  BOOST_CHECK_EQUAL(4, dt.getMonth()); // zero based
  BOOST_CHECK_EQUAL(6, dt.getDay());
  BOOST_CHECK_EQUAL(8, dt.getHour());
  BOOST_CHECK_EQUAL(1, dt.getMinute());
  BOOST_CHECK_EQUAL(2, dt.getSecond());
} // testISODate

BOOST_AUTO_TEST_CASE(testTooshortISODate) {
  BOOST_CHECK_THROW(DateTime::fromISO("20080506T080102"), std::invalid_argument);
} // testISODate

BOOST_AUTO_TEST_CASE(testMonthOutOfRange) {
  BOOST_CHECK_THROW(DateTime::fromISO("20083006T080102Z"), std::invalid_argument);
} // testMonthOutOfRange

BOOST_AUTO_TEST_CASE(testMissingTISODate) {
  BOOST_CHECK_THROW(DateTime::fromISO("20080506X080102Z"), std::invalid_argument);
} // testMissingTISODate

BOOST_AUTO_TEST_CASE(testHourOutOfRangeISODate) {
  BOOST_CHECK_THROW(DateTime::fromISO("20080506T250102Z"), std::invalid_argument);
} // testHourOutOfRangeISODate

BOOST_AUTO_TEST_CASE(testMinuteOutOfRangeISODate) {
  BOOST_CHECK_THROW(DateTime::fromISO("20080506T086202Z"), std::invalid_argument);
} // testMinuteOutOfRangeISODate

BOOST_AUTO_TEST_CASE(testSecondsOutOfRangeISODate) {
  BOOST_CHECK_THROW(DateTime::fromISO("20080506T080162Z"), std::invalid_argument);
} // testSecondOutOfRangeISODate

BOOST_AUTO_TEST_CASE(testGetDayOfYear) {
  BOOST_CHECK_EQUAL(1, DateTime::fromISO("20090102T100000Z").getDayOfYear()); // zero based
} // testGetDayOfYear

BOOST_AUTO_TEST_CASE(testSetters) {
  DateTime dt;
  dt.setDay(1);
  dt.setMonth(2);
  dt.setYear(2009);
  dt.setHour(3);
  dt.setMinute(4);
  dt.setSecond(5);
  dt.validate();
  DateTime other;
  other.setDate(1, 2, 2009);
  other.setTime(3,4,5);
  BOOST_CHECK_EQUAL(dt, other);
} // testSetters

BOOST_AUTO_TEST_CASE(testAddingComparing) {
  DateTime dt;
  DateTime dt2 = dt.addHour(1);

  assert(dt2.after(dt));
  assert(!dt2.before(dt));
  assert(dt.before(dt2));
  assert(!dt.after(dt2));

  BOOST_CHECK(dt2.after(dt));
  BOOST_CHECK(!dt2.before(dt));
  BOOST_CHECK(dt.before(dt2));
  BOOST_CHECK(!dt.after(dt2));
} // testAddingComparing

BOOST_AUTO_TEST_CASE(testStaticSchedule) {
  DateTime when;
  when.setDate(15, April, 2008);
  when.setTime(10, 00, 00);

  StaticSchedule schedule(when);

  BOOST_CHECK_EQUAL(when, schedule.getNextOccurence(when.addMinute(-1)));
  BOOST_CHECK_EQUAL(DateTime::NullDate, schedule.getNextOccurence(when.addMinute(1)));

  std::vector<DateTime> schedList = schedule.getOccurencesBetween(when.addMinute(-1), when.addMinute(1));
  BOOST_CHECK_EQUAL(static_cast<size_t>(1), schedList.size());

  schedList = schedule.getOccurencesBetween(when.addMinute(1), when.addMinute(2));
  BOOST_CHECK_EQUAL(static_cast<size_t>(0), schedList.size());
} // testStaticSchedule

BOOST_AUTO_TEST_CASE(testDynamicSchedule) {
  DateTime when;
  when.setDate(15, April, 2008);
  when.setTime(10, 00, 00);

  RepeatingSchedule schedule(Minutely, 5, when);

  BOOST_CHECK_EQUAL(when,              schedule.getNextOccurence(when.addSeconds(-1)));
  BOOST_CHECK_EQUAL(when.addMinute(5), schedule.getNextOccurence(when.addSeconds(1)));

  std::vector<DateTime> v = schedule.getOccurencesBetween(when, when.addMinute(10));

  BOOST_CHECK_EQUAL(static_cast<size_t>(3), v.size());
  BOOST_CHECK_EQUAL(when, v[0]);
  BOOST_CHECK_EQUAL(when.addMinute(5), v[1]);
  BOOST_CHECK_EQUAL(when.addMinute(10), v[2]);
} // testDynamicSchedule

#if defined(HAVE_LIBICAL_ICAL_H) || defined(HAVE_ICAL_H)
BOOST_AUTO_TEST_CASE(testDynamicScheduleICal) {
  ICalSchedule sched("FREQ=MINUTELY;INTERVAL=2", "20080505T080000Z");

  DateTime startTime = DateTime::fromISO("20080505T080000Z");

  DateTime firstRecurr = sched.getNextOccurence(startTime);
  BOOST_CHECK_EQUAL(startTime, firstRecurr);

  DateTime startPlusOneSec = startTime.addSeconds(1);
  DateTime nextRecurr = sched.getNextOccurence(startPlusOneSec);
  BOOST_CHECK_EQUAL(startTime.addMinute(2), nextRecurr);
} // testDynamicScheduleICal
#endif

BOOST_AUTO_TEST_SUITE_END()

