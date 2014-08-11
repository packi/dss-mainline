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

#include "../src/datetools.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(DateTools)

class TZSwitcher {
public:
  TZSwitcher(const char *tz) {
    m_old_tz = getenv("TZ");
    (void)setenv("TZ", tz, true);
    tzset();
  }
  ~TZSwitcher() {
    if (m_old_tz) {
      setenv("TZ", m_old_tz, true);
    } else {
      unsetenv("TZ");
    }
    tzset();
  }
private:
  const char *m_old_tz;
};

BOOST_AUTO_TEST_CASE(testSimpleDates) {
  DateTime dt(0);
  dt.setDate(1, January, 2008);

  BOOST_CHECK_EQUAL(dt.getYear(), 2008);
  BOOST_CHECK_EQUAL(dt.getWeekday(), Tuesday);
  DateTime dt2 = dt.addDay(1);
  BOOST_CHECK_EQUAL(dt2.getWeekday(), Wednesday);
  DateTime dt3 = dt2.addDay(-2);
  BOOST_CHECK_EQUAL(dt3.getWeekday(), Monday);
} // testSimpleDates

BOOST_AUTO_TEST_CASE(testAddingMonth) {
  DateTime dt;
  DateTime dt2 = dt.addMonth(1);

  BOOST_CHECK(dt2 > dt);
  BOOST_CHECK(!(dt2 < dt));
  BOOST_CHECK(!(dt2 <= dt));
  BOOST_CHECK(dt < dt2);
  BOOST_CHECK(!(dt > dt2));
  BOOST_CHECK(!(dt >= dt2));
}

BOOST_AUTO_TEST_CASE(testAddingHour) {
  DateTime dt;
  DateTime dt2 = dt.addHour(1);

  BOOST_CHECK(dt2 > dt);
  BOOST_CHECK(!(dt2 < dt));
  BOOST_CHECK(!(dt2 <= dt));
  BOOST_CHECK(dt < dt2);
  BOOST_CHECK(!(dt > dt2));
  BOOST_CHECK(!(dt >= dt2));
}

BOOST_AUTO_TEST_CASE(testRFC2445) {
  DateTime dt = DateTime::parseRFC2445("20080506T080102");
  BOOST_CHECK_EQUAL(2008, dt.getYear());
  BOOST_CHECK_EQUAL(4, dt.getMonth()); // zero based
  BOOST_CHECK_EQUAL(6, dt.getDay());
  BOOST_CHECK_EQUAL(8, dt.getHour());
  BOOST_CHECK_EQUAL(1, dt.getMinute());
  BOOST_CHECK_EQUAL(2, dt.getSecond());
  BOOST_CHECK(dt.toRFC2445IcalDataTime() == "20080506T080102");
}
BOOST_AUTO_TEST_CASE(testRFC2445UTC) {
  TZSwitcher s("Europe/Zurich");
  DateTime dt = DateTime::parseRFC2445("20080101T120102Z");
  BOOST_CHECK(dt.toRFC2445IcalDataTime() == "20080101T130102");
  dt = DateTime::parseRFC2445("20080721T120102Z");
  BOOST_CHECK(dt.toRFC2445IcalDataTime() == "20080721T140102");
}
BOOST_AUTO_TEST_CASE(testGetDayOfYearZeroBased) {
  BOOST_CHECK_EQUAL(1, DateTime::parseRFC2445("20090102T100000Z").getDayOfYear());
}
BOOST_AUTO_TEST_CASE(testMandate_T_Separator) {
  BOOST_CHECK_THROW(DateTime::parseRFC2445("20080506 080102"), std::invalid_argument);
  BOOST_CHECK_THROW(DateTime::parseRFC2445("20080506080102"), std::invalid_argument);
}
BOOST_AUTO_TEST_CASE(testTooshortRFC2445) {
  BOOST_CHECK_THROW(DateTime::parseRFC2445("20080506T08010"), std::invalid_argument);
}
BOOST_AUTO_TEST_CASE(testMonthOutOfRange) {
  BOOST_CHECK_THROW(DateTime::parseRFC2445("20083006T080102Z"), std::invalid_argument);
}
BOOST_AUTO_TEST_CASE(testMissingTRFC2445) {
  BOOST_CHECK_THROW(DateTime::parseRFC2445("20080506X080102Z"), std::invalid_argument);
}
BOOST_AUTO_TEST_CASE(testHourOutOfRangeRFC2445) {
  BOOST_CHECK_THROW(DateTime::parseRFC2445("20080506T250102Z"), std::invalid_argument);
}
BOOST_AUTO_TEST_CASE(testMinuteOutOfRangeRFC2445) {
  BOOST_CHECK_THROW(DateTime::parseRFC2445("20080506T086202Z"), std::invalid_argument);
}
BOOST_AUTO_TEST_CASE(testSecondsOutOfRangeRFC2445) {
  BOOST_CHECK_THROW(DateTime::parseRFC2445("20080506T080162Z"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(testPrettyDate) {
  std::string asString = "2014-08-05 23:23:32";
  DateTime parsedObj = DateTime::parsePrettyString(asString);
  BOOST_CHECK_EQUAL(asString, parsedObj.toPrettyString());
}

BOOST_AUTO_TEST_CASE(testISO8601) {
  TZSwitcher s("Europe/Zurich");

  // localtime
  DateTime foo = DateTime::parseISO8601("2009-01-02T13:00:00+0100");
  BOOST_CHECK(foo.toISO8601() == "2009-01-02T13:00:00+0100");
  foo = DateTime::parseISO8601("2009-07-21T14:00:00+0200");
  BOOST_CHECK(foo.toISO8601() == "2009-07-21T14:00:00+0200");

  // convert +0300
  foo = DateTime::parseISO8601("2009-01-02T15:00:00+0300");
  BOOST_CHECK(foo.toISO8601() == "2009-01-02T13:00:00+0100");
  foo = DateTime::parseISO8601("2009-07-21T15:00:00+0300");
  BOOST_CHECK(foo.toISO8601() == "2009-07-21T14:00:00+0200");

  // convert +0230
  foo = DateTime::parseISO8601("2009-01-02T14:30:00+0230");
  BOOST_CHECK(foo.toISO8601() == "2009-01-02T13:00:00+0100");

  // half working: +01:00 format
  foo = DateTime::parseISO8601("2009-01-02T11:00:00+01");
  BOOST_CHECK(foo.toISO8601() == "2009-01-02T11:00:00+0100");
  // the minutes are not parsed
  foo = DateTime::parseISO8601("2009-01-02T11:00:00+01:00");
  BOOST_CHECK(foo.toISO8601() == "2009-01-02T11:00:00+0100");

  // utc
  foo = DateTime::parseISO8601("2009-01-02T12:00:00Z");
  BOOST_CHECK(foo.toISO8601() == "2009-01-02T13:00:00+0100");
}

BOOST_AUTO_TEST_CASE(testISO8601_australia) {
  TZSwitcher s("Australia/Adelaide");
  DateTime foo = DateTime::parseISO8601("2009-01-02T12:00:00+0300");
  BOOST_CHECK(foo.toISO8601() == "2009-01-02T19:30:00+1030"); // dst
  foo = DateTime::parseISO8601("2009-07-21T12:00:00+0300");
  BOOST_CHECK(foo.toISO8601() == "2009-07-21T18:30:00+0930");
}

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
  DateTime startTime = DateTime::parseRFC2445("20080505T080000Z");

  DateTime currentTime;
  DateTime nextSchedule = currentTime.addMinute(2);
  nextSchedule.setMinute(nextSchedule.getMinute() & ~1);
  nextSchedule.setSecond(0);

  DateTime firstRecurr = sched.getNextOccurence(startTime);
  BOOST_CHECK_EQUAL(nextSchedule, firstRecurr);

} // testDynamicScheduleICal
#endif

BOOST_AUTO_TEST_SUITE_END()

