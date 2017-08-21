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

BOOST_AUTO_TEST_CASE(test_mktime_dst_modifies_gmtoff) {
  struct tm tmw, tms;
  TZSwitcher s("Europe/Zurich");

  strptime("2014-01-02T12:00:00+0300", "%FT%T%z", &tmw);
  strptime("2014-07-02T12:00:00+0300", "%FT%T%z", &tms);

  tmw.tm_isdst = tms.tm_isdst = -1; // valgrind
  mktime(&tmw);
  mktime(&tms);

  BOOST_CHECK_EQUAL(tmw.tm_isdst, 0);
  BOOST_CHECK_EQUAL(tmw.tm_gmtoff, 3600);
  BOOST_CHECK_EQUAL(tms.tm_isdst, 1);
  BOOST_CHECK_EQUAL(tms.tm_gmtoff, 7200);
}

BOOST_AUTO_TEST_CASE(testSimpleDates) {
  DateTime dt = DateTime::parseISO8601("2008-01-01T12:00:00Z");

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

BOOST_AUTO_TEST_CASE(testAddingMilliseconds) {
  DateTime dt;
  DateTime dt2 = dt.addMilliSeconds(2000);

  BOOST_CHECK(dt2 > dt);
  BOOST_CHECK(!(dt2 < dt));
  BOOST_CHECK(!(dt2 <= dt));
  BOOST_CHECK(dt < dt2);
  BOOST_CHECK(!(dt > dt2));
  BOOST_CHECK(!(dt >= dt2));

  dt2 = dt.addMilliSeconds(-5000);
  BOOST_CHECK(dt2 < dt);
  BOOST_CHECK(dt2 <= dt);
  BOOST_CHECK(!(dt2 > dt));

  dt2 = dt.addMilliSeconds(-2000);
  BOOST_CHECK(dt2.difference(dt) == -2);
}

BOOST_AUTO_TEST_CASE(testDifference) {
  DateTime dt, dt2;

  dt2 = dt.addMinute(1);
  BOOST_CHECK(dt2.difference(dt) == 60);
  BOOST_CHECK(dt.difference(dt2) == -60);

  dt2 = dt.addMinute(-dt.getHour() * 60);
  BOOST_CHECK(dt2.getHour() == 0);

  dt = dt.addMinute(-dt.getMinute());
  BOOST_CHECK(dt.getMinute() == 0);
  dt = dt.addSeconds(-(dt.getSecond() - 1));
  BOOST_CHECK(dt.getSecond() == 1);
  dt2 = dt.addSeconds(-2);
  BOOST_CHECK(dt.difference(dt2) == 2);
}

BOOST_AUTO_TEST_CASE(testRFC2445) {
  DateTime dt = DateTime::parseRFC2445("20080506T080102");
  BOOST_CHECK_EQUAL(2008, dt.getYear());
  BOOST_CHECK_EQUAL(4, dt.getMonth()); // zero based
  BOOST_CHECK_EQUAL(6, dt.getDay());
  BOOST_CHECK_EQUAL(8, dt.getHour());
  BOOST_CHECK_EQUAL(1, dt.getMinute());
  BOOST_CHECK_EQUAL(2, dt.getSecond());
  BOOST_CHECK_EQUAL(dt.toRFC2445IcalDataTime(), "20080506T080102");
}
BOOST_AUTO_TEST_CASE(testRFC2445UTC) {
  TZSwitcher s("Europe/Zurich");
  DateTime dt = DateTime::parseRFC2445("20080101T120102Z");
  BOOST_CHECK_EQUAL(dt.toRFC2445IcalDataTime(), "20080101T130102");
  dt = DateTime::parseRFC2445("20080721T120102Z");
  BOOST_CHECK_EQUAL(dt.toRFC2445IcalDataTime(), "20080721T140102");
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
  DateTime foo = DateTime::parseISO8601("2009-01-02T13:00:00+01:00");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-01-02T12:00:00Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_local(), "2009-01-02T13:00:00+01:00");

  foo = DateTime::parseISO8601("2009-07-21T14:00:00+02:00");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-07-21T12:00:00Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_local(), "2009-07-21T14:00:00+02:00");

  // convert +0300
  foo = DateTime::parseISO8601("2009-01-02T15:00:00+03:00");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-01-02T12:00:00Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_local(), "2009-01-02T13:00:00+01:00");
  foo = DateTime::parseISO8601("2009-07-21T15:00:00+03:00");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-07-21T12:00:00Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_local(), "2009-07-21T14:00:00+02:00");

  // convert +0230
  foo = DateTime::parseISO8601("2009-01-02T14:30:00+02:30");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-01-02T12:00:00Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_local(), "2009-01-02T13:00:00+01:00");

  // half working: +01:00 format
  foo = DateTime::parseISO8601("2009-01-02T11:00:00+01");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-01-02T10:00:00Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_local(), "2009-01-02T11:00:00+01:00");
  // the minutes are not parsed
  foo = DateTime::parseISO8601("2009-01-02T11:00:00+01:00");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-01-02T10:00:00Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_local(), "2009-01-02T11:00:00+01:00");

  // utc
  foo = DateTime::parseISO8601("2009-01-02T12:00:00Z");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-01-02T12:00:00Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_local(), "2009-01-02T13:00:00+01:00");
}

BOOST_AUTO_TEST_CASE(testISO8601_parse_tz) {
  DateTime foo = DateTime::parseISO8601("2009-01-02T19:30:00+1030");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-01-02T09:00:00Z");

  foo = DateTime::parseISO8601("2009-01-02T19:30:00+10:30");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-01-02T09:00:00Z");

  foo = DateTime::parseISO8601("2009-01-02T19:30:00+10");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-01-02T09:30:00Z");

  foo = DateTime::parseISO8601("2009-01-02T19:30:00+10:");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-01-02T09:30:00Z");
}

BOOST_AUTO_TEST_CASE(testISO8601_australia) {
  TZSwitcher s("Australia/Adelaide");
  // convert australia local time, in australia timezone
  DateTime foo = DateTime::parseISO8601("2009-01-02T19:30:00+1030");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-01-02T09:00:00Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_local(), "2009-01-02T19:30:00+10:30");

  // convert non-australian local time, still australia timezone
  foo = DateTime::parseISO8601("2009-01-02T12:00:00+0300");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-01-02T09:00:00Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_local(), "2009-01-02T19:30:00+10:30");

  // same but with daylight saving
  foo = DateTime::parseISO8601("2009-07-21T12:00:00+0300");
  BOOST_CHECK_EQUAL(foo.toISO8601(), "2009-07-21T09:00:00Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_local(), "2009-07-21T18:30:00+09:30");
}

BOOST_AUTO_TEST_CASE(testISO8601_ms_australia) {
  TZSwitcher s("Australia/Adelaide");
  // convert australia local time, in australia timezone
  DateTime foo = DateTime::parseISO8601("2009-01-02T19:30:00+1030");
  BOOST_CHECK_EQUAL(foo.toISO8601_ms(), "2009-01-02T09:00:00.000Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_ms_local(), "2009-01-02T19:30:00.000+10:30");

  // convert non-australian local time, still australia timezone
  foo = DateTime::parseISO8601("2009-01-02T12:00:00+0300");
  BOOST_CHECK_EQUAL(foo.toISO8601_ms(), "2009-01-02T09:00:00.000Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_ms_local(), "2009-01-02T19:30:00.000+10:30");

  // same but with daylight saving
  foo = DateTime::parseISO8601("2009-07-21T12:00:00+0300");
  BOOST_CHECK_EQUAL(foo.toISO8601_ms(), "2009-07-21T09:00:00.000Z");
  BOOST_CHECK_EQUAL(foo.toISO8601_ms_local(), "2009-07-21T18:30:00.000+09:30");
}

BOOST_AUTO_TEST_CASE(testISO8601_timet) {
  // the epoch start is in UTC, converted seconds are in UTC too
  DateTime foo = DateTime::parseISO8601("2009-01-02T12:00:00+0200");
  DateTime bar = DateTime::parseISO8601("2009-01-02T15:00:00+0500");
  BOOST_CHECK_EQUAL(foo.secondsSinceEpoch(), bar.secondsSinceEpoch());

  foo = DateTime::parseISO8601("2009-01-02T12:00:00+0200");
  bar = DateTime::parseISO8601("2009-01-02T10:00:00Z");
  BOOST_CHECK_EQUAL(foo.secondsSinceEpoch(), bar.secondsSinceEpoch());
}

BOOST_AUTO_TEST_CASE(testISO8601_ms) {
  TZSwitcher s("Europe/Zurich");
  DateTime foo = DateTime::parseISO8601("2009-01-02T12:00:00+0100");
  BOOST_CHECK_EQUAL(foo.toISO8601_ms(), "2009-01-02T11:00:00.000Z");

  DateTime bar(foo.secondsSinceEpoch(), 777777);
  BOOST_CHECK_EQUAL(bar.toISO8601_ms(), "2009-01-02T11:00:00.777Z");
}

BOOST_AUTO_TEST_CASE(testISO8601_ms_local) {
  TZSwitcher s("Europe/Zurich");
  DateTime foo = DateTime::parseISO8601("2009-01-02T12:00:00+0100");
  BOOST_CHECK_EQUAL(foo.toISO8601_ms_local(), "2009-01-02T12:00:00.000+01:00");

  DateTime bar(foo.secondsSinceEpoch(), 777777);
  BOOST_CHECK_EQUAL(bar.toISO8601_ms_local(), "2009-01-02T12:00:00.777+01:00");
}

BOOST_AUTO_TEST_CASE(testStaticSchedule) {
  DateTime when = DateTime::parseISO8601("2008-04-15T10:00:00+0200");

  StaticSchedule schedule(when);
  BOOST_CHECK_EQUAL(when, schedule.getNextOccurence(when.addMinute(-1)));
  BOOST_CHECK_EQUAL(when.addMinute(1), schedule.getNextOccurence(when.addMinute(1)));
} // testStaticSchedule

#if defined(HAVE_LIBICAL_ICAL_H) || defined(HAVE_ICAL_H)
BOOST_AUTO_TEST_CASE(testDynamicScheduleICal) {
  ICalSchedule sched("FREQ=MINUTELY;INTERVAL=2", "20080505T080000Z");

  DateTime now;
  DateTime firstRecurr = sched.getNextOccurence(now);
  BOOST_CHECK(firstRecurr.getMinute() <= now.getMinute() + 2);
  BOOST_CHECK_EQUAL(firstRecurr.getMinute() % 2, 0);
  BOOST_CHECK(firstRecurr.secondsSinceEpoch() - now.secondsSinceEpoch() <= 120);
} // testDynamicScheduleICal

BOOST_AUTO_TEST_CASE(testScheduleICalTimeLeap) {
  ICalSchedule sched("FREQ=SECONDLY;INTERVAL=2", "20170815T120000Z");

  DateTime now;
  DateTime firstRecurr = sched.getNextOccurence(now);
  BOOST_TEST_MESSAGE("now " + now.toISO8601_ms());
  BOOST_TEST_MESSAGE("1st " + firstRecurr.toISO8601_ms());
  BOOST_CHECK(firstRecurr.secondsSinceEpoch() <= now.secondsSinceEpoch() + 2);

  sleep(6);
  DateTime secondRecurr = sched.getNextOccurence(now);
  BOOST_TEST_MESSAGE("2nd " + secondRecurr.toISO8601_ms());
  BOOST_CHECK(secondRecurr.secondsSinceEpoch() <= now.secondsSinceEpoch() + 4);

  DateTime now2;
  sched.leapAdjust(now2.secondsSinceEpoch());
  DateTime thirdRecurr = sched.getNextOccurence(now);

  // at least one interval must have been skipped by leapAdjust()
  BOOST_TEST_MESSAGE("now2 " + now2.toISO8601_ms());
  BOOST_TEST_MESSAGE("3rd " + thirdRecurr.toISO8601_ms());
  BOOST_CHECK(thirdRecurr.secondsSinceEpoch() >= now2.secondsSinceEpoch());
} // testScheduleICalTimeLeap

BOOST_AUTO_TEST_CASE(testICalEvent) {
  ICalEvent event("FREQ=DAILY;BYMONTH=7;UNTIL=20160801T000000Z", "20150707T150000", "20150707T170000");

  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T130000")), false);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T150000")), true);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T160000")), true);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T170000")), true);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T180000")), false);
  // next day
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150708T130000")), false);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150708T160000")), true);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150708T180000")), false);
  // next month
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150808T130000")), false);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150808T160000")), false);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150808T180000")), false);
  // next following year
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20160707T130000")), false);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20160707T160000")), true);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20160707T180000")), false);
  // the year after
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20170707T130000")), false);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20170707T160000")), false);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20170707T180000")), false);
} // testICalEvent

BOOST_AUTO_TEST_CASE(testICalEventNight) {
  ICalEvent event("FREQ=DAILY;BYMONTH=7;UNTIL=20160801T000000Z", "20150707T213000", "20150708T053000");

  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T130000")), false);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T220000")), true);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150708T010000")), true);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150708T060000")), false);
  // next night
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150708T130000")), false);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150708T220000")), true);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150709T010000")), true);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150709T060000")), false);
} // testICalEventNight

BOOST_AUTO_TEST_CASE(testICalEventDST) {
  ICalEvent event("FREQ=DAILY", "20150707T150000", "20150707T170000");

  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T145900")), false);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T160000")), true);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T170001")), false);

  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20151025T145900")), false);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20151025T160000")), true);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20151025T170001")), false);
} // testICalEvent

BOOST_AUTO_TEST_CASE(testICalEventTZ) {
  ICalEvent event("FREQ=DAILY", "20150707T150000", "20150707T170000");

  TZSwitcher s("Australia/Adelaide");
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T145900")), false);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T160000")), true);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T170001")), false);

  TZSwitcher t("America/Los_Angeles");
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T145900")), false);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T160000")), true);
  BOOST_CHECK_EQUAL(event.isDateInside(DateTime::parseRFC2445("20150707T170001")), false);
} // testICalEvent
#endif

BOOST_AUTO_TEST_SUITE_END()
