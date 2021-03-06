/*
    Copyright (c) 2009,2011 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
            Michael Tross, aizo GmbH <michael.tross@aizo.com>

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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "datetools.h"

#include <cstring>
#include <errno.h>
#include <math.h>
#include <sys/time.h>

namespace dss {

  /*
   * TODO can DateTools bre replaced with this:
   * http://www.boost.org/doc/libs/1_55_0/doc/html/date_time/details.html#date_time.tradeoffs
   */

  /**
   * Converts \tm to local timezone of this machine
   * @tm contains '2009-01-02T15:00:00+0300'
   * @tz_offset seconds east of utc
   * @ret struct tm
   */
  static time_t convertTZ(struct tm tm, time_t tz_offset) {
    time_t t0;

    /*
     * mktime will override tm_gmtoff with offset of local
     * timezone. tm_isdst can be 1, 0, -1 aka enabled,
     * disabled and figure it out
     * http://caml.inria.fr/mantis/print_bug_page.php?bug_id=1882
     */
    tm.tm_isdst = -1; /* rely on mktime for daylight saving time */
    t0 = mktime(&tm);
    if (t0 == -1) {
      throw std::invalid_argument("mktime");
    }

    /*
     * mktime did not modify year, month, day, hour, minutes
     * and second. It has only overwritten the timezone and
     * daylight saving time, which is of course incorrect
     * without also correcting the hours.
     * NOTE: the input tz_offset includes daylight saving
     * time correction, the tm_gmtoff computed by mktime as well
     * By subracting the old offset we get UTC by adding
     * the local offset we get localtime
     */
    t0 -= (tz_offset - tm.tm_gmtoff);
    return t0;
  }

  /**
   * TODO icaltime_as_timet sufficient?
   */
  static time_t ical_to_tm(const icaltimetype& icalTime) {
    time_t t0;
    struct tm tm;
    memset(&tm, '\0', sizeof(tm));
    if (icalTime.is_utc) {
      t0 = icaltime_as_timet(icalTime);
    } else {
      if(!icalTime.is_date) {
        tm.tm_sec = icalTime.second;
        tm.tm_min = icalTime.minute;
        tm.tm_hour = icalTime.hour;
      }
      tm.tm_mday = icalTime.day;
      tm.tm_mon = icalTime.month - 1;
      tm.tm_year = icalTime.year - 1900;
      tm.tm_isdst = -1;
      t0 = mktime(&tm);
      if (t0 == -1) {
        throw std::invalid_argument("mktime");
      }
    }
    return t0;
  } // ical_to_tm

  //================================================== DateTime

  DateTime::DateTime() {
    if (gettimeofday(&m_timeval, NULL)) {
      throw std::runtime_error(strerror(errno));
    }
  } // ctor

  DateTime::DateTime(time_t secs, suseconds_t usec) {
    m_timeval.tv_sec = secs;
    m_timeval.tv_usec = usec;
  } // ctor(time_t)

  DateTime::DateTime(const DateTime& _copy)
  : m_timeval(_copy.m_timeval)
  { } // ctor(copy)

  DateTime DateTime::addHour(const int _hours) const {
    struct timeval tv = m_timeval;
    tv.tv_sec += _hours * 60 * 60;
    return DateTime(tv);
  } // addHour

  DateTime DateTime::addMinute(const int _minutes) const {
    struct timeval tv = m_timeval;
    tv.tv_sec += _minutes * 60;
    return DateTime(tv);
  } // addMinute

  DateTime DateTime::addSeconds(const int _seconds) const {
    struct timeval tv = m_timeval;
    tv.tv_sec += _seconds;
    return DateTime(tv);
  } // addSeconds

  DateTime DateTime::addMilliSeconds(const int _milliseconds) const {
    struct timeval tv = m_timeval;
    struct timeval add;
    struct timeval result;
    add.tv_sec = abs(_milliseconds) / 1000;
    add.tv_usec = (abs(_milliseconds) - add.tv_sec * 1000) * 1000;
    if (_milliseconds < 0) {
      timersub(&tv, &add, &result);
    } else {
      timeradd(&tv, &add, &result);
    }
    return DateTime(result);
  } // addMilliSeconds

  DateTime DateTime::addMonth(const int _month) const {
    struct timeval tv = m_timeval;
    struct tm tm;

    localtime_r(&m_timeval.tv_sec, &tm);
    tm.tm_mon += _month;
    tv.tv_sec = mktime(&tm);
    return DateTime(tv);
  } // addMonth

  DateTime DateTime::addYear(const int _years) const {
    struct timeval tv = m_timeval;
    struct tm tm;

    localtime_r(&m_timeval.tv_sec, &tm);
    tm.tm_year += _years;
    tv.tv_sec = mktime(&tm);
    return DateTime(tv);
  } // addYear

  DateTime DateTime::addDay(const int _days) const {
    struct timeval tv = m_timeval;
    tv.tv_sec += _days * 24 * 60 * 60;
    return DateTime(tv);
  } // addDay

  int DateTime::getDay() const {
    struct tm tm;
    localtime_r(&m_timeval.tv_sec, &tm);
    return tm.tm_mday;
  } // getDay

  int DateTime::getMonth() const {
    struct tm tm;
    localtime_r(&m_timeval.tv_sec, &tm);
    return tm.tm_mon;
  } // getMonth

  int DateTime::getYear() const {
    struct tm tm;
    localtime_r(&m_timeval.tv_sec, &tm);
    return tm.tm_year + 1900;
  } // getYear

  int DateTime::getHour() const {
    struct tm tm;
    localtime_r(&m_timeval.tv_sec, &tm);
    return tm.tm_hour;
  } // getHour

  int DateTime::getMinute() const {
    struct tm tm;
    localtime_r(&m_timeval.tv_sec, &tm);
    return tm.tm_min;
  } // getMinute

  int DateTime::getSecond() const {
    struct tm tm;
    localtime_r(&m_timeval.tv_sec, &tm);
    return tm.tm_sec;
  } // getSecond

  int DateTime::getDayOfYear() const {
    struct tm tm;
    localtime_r(&m_timeval.tv_sec, &tm);
    return tm.tm_yday;
  } // getDayOfYear

  Weekday DateTime::getWeekday() const {
    struct tm tm;
    localtime_r(&m_timeval.tv_sec, &tm);
    return (Weekday)tm.tm_wday;
  } // getWeekday

  bool DateTime::operator==(const DateTime& _other) const {
    return difference(_other) == 0;
  } // operator==

  bool DateTime::operator!=(const DateTime& _other) const {
    return difference(_other) != 0;
  } // operator!=

  bool DateTime::operator<(const DateTime& _other) const {
    return difference(_other) < 0;
  } // operator<

  bool DateTime::operator>(const DateTime& _other) const {
    return difference(_other) > 0;
  } // operator>

  bool DateTime::operator<=(const DateTime& _other) const {
    return difference(_other) <= 0;
  } // operator<=

  bool DateTime::operator>=(const DateTime& _other) const {
    return difference(_other) >= 0;
  } // operator>=

  int DateTime::difference(const DateTime& _other) const {
    // TODO useconds are ignored
    // TODO use timercmp, timersub
    return static_cast<int>(difftime(m_timeval.tv_sec,
                            _other.m_timeval.tv_sec));
  } // difference

  time_t DateTime::secondsSinceEpoch() const {
    return m_timeval.tv_sec;
  }

  long int DateTime::getTimezoneOffset() const {
    struct tm tm;
    localtime_r(&m_timeval.tv_sec, &tm);
    return tm.tm_gmtoff;
  }

  DateTime::operator std::string() const {
    return toString();
  } // operator std::string()

  std::string DateTime::toString() const {
    return toPrettyString();
  }

  std::string DateTime::toRFC2822String() const {
    static const char* theRFC2822FormatString = "%a, %d %b %Y %T %z";
    struct tm tm;
    char buf[32];

    localtime_r(&m_timeval.tv_sec, &tm);
    strftime(buf, sizeof buf, theRFC2822FormatString, &tm);
    return std::string(buf);
  } // toRFC2822String

  std::string DateTime::toRFC2445IcalDataTime() const {
    struct tm tm;
    char buf[20];

    /*
     * http://www.ietf.org/rfc/rfc2445.txt
     * http://www.kanzaki.com/docs/ical/dateTime.html
     */
    localtime_r(&m_timeval.tv_sec, &tm);
    strftime(buf, sizeof buf, "%Y%m%dT%H%M%S", &tm);
    return std::string(buf);
  }

  DateTime DateTime::parseRFC2445(const std::string& timeString) {
    struct tm tm;
    time_t t0;
    bool utc = timeString[timeString.size() - 1] == 'Z';

    if (timeString.length() != strlen("19930202T220202") + utc ? 1 : 0) {
      throw std::invalid_argument("RFC2445: too short " + timeString);
    }

    memset(&tm, 0, sizeof(tm));
    char *end = strptime(timeString.c_str(), "%Y%m%dT%H%M%S", &tm);
    if (!end || (end - timeString.c_str() != strlen("19930202T226202"))) {
      // strptime %M matches 0-59, so ..T226202 above results in 22:06:20,
      // the trailing '2' is silently dropped
      throw std::invalid_argument("RFC2445: invalid " + timeString);
    }

    tm.tm_isdst = -1;
    t0 = mktime(&tm);
    if (t0 == -1) {
      throw std::invalid_argument("RFC2445: mktime failure" + timeString);
    }
    if (utc) {
      t0 += tm.tm_gmtoff;
    }
    return DateTime(t0);
  }

  /*
   * ISO 8061
   * http://www.cl.cam.ac.uk/~mgk25/iso-time.html
   * http://www.cs.tut.fi/~jkorpela/iso8601.html
   * http://www.w3.org/TR/NOTE-datetime
   * http://www.ietf.org/rfc/rfc3339.txt
   */
  DateTime DateTime::parseISO8601(std::string in) {
    bool utc = in[in.size() - 1]  == 'Z';
    struct tm tm;

    if (in.length() != strlen("2011-10-08T07:07:09+02:30") &&
        in.length() != strlen("2011-10-08T07:07:09+0200") &&
        in.length() != strlen("2011-10-08T07:07:09+02:") &&
        in.length() != strlen("2011-10-08T07:07:09+02") &&
        in.length() != strlen("2011-10-08T07:07:09Z")) {
      throw std::invalid_argument("ISO8601: invalid length " + in);
    }

    char *end;
    memset(&tm, 0, sizeof tm);
    if (utc) {
      end = strptime(in.c_str(), "%FT%TZ", &tm);
    } else {
      end = strptime(in.c_str(), "%FT%T%z", &tm);
    }
    if (!end || (*end != '\0' && *end != ':')) {
        throw std::invalid_argument("ISO8601: invalid format " + in);
    }

    if (*end == ':' && *(end + 1) != '\0') {
        // strptime can't parse timezone with colon
        char *tz_minutes = end + 1;
        int ret;

        errno = 0;
        ret = strtod(tz_minutes, &end);
        if (!end || end == tz_minutes || *end != '\0' ||
            (ret == HUGE_VAL || ret == -HUGE_VAL) ||
            (ret == 0 && errno == ERANGE)) {
            throw std::invalid_argument("ISO8601: invalid tz minutes " + in);
        }

        tm.tm_gmtoff += ret * 60; // gmtoff in seconds
    }

    try {
      return DateTime(convertTZ(tm, tm.tm_gmtoff));
    } catch (std::invalid_argument &e) {
      throw std::invalid_argument(std::string("ISO8601: ") + e.what() + " " + in);
    }
  }

  std::string DateTime::toISO8601() const {
    struct tm tm;
    char buf[sizeof "2011-10-08T07:07:09Z"];

    gmtime_r(&m_timeval.tv_sec, &tm);
    strftime(buf, sizeof buf, "%FT%TZ", &tm);
    return std::string(buf);
  }

  std::string DateTime::toISO8601_local() const {
    struct tm tm;
    char buf[sizeof "2011-10-08T07:07:09.000+02:00"];

    localtime_r(&m_timeval.tv_sec, &tm);
    strftime(buf, sizeof buf, "%FT%T%z", &tm);

    /* ensure there is a colon in time zone */
    if (buf[22] != ':') {
        buf[25] = '\0';
        buf[24] = buf[23];
        buf[23] = buf[22];
        buf[22] = ':';
    }
    return std::string(buf);
  }

  /*
   * 100 Hz machine has less than 10ms accuracy
   * http://stackoverflow.com/questions/361363/how-to-measure-time-in-milliseconds-using-ansi-c
   * TODO: is ms really part of 8601
   */
  std::string DateTime::toISO8601_ms_local() const {
    struct tm tm;
    char buf[sizeof "2011-10-08T07:07:09.000+02:00"];
    char fmt[sizeof "2011-10-08T07:07:09.000+02:00"];

    // http://stackoverflow.com/questions/1551597/using-strftime-in-c-how-can-i-format-time-exactly-like-a-unix-timestamp
    localtime_r(&m_timeval.tv_sec, &tm);
    strftime(fmt , sizeof fmt, "%FT%T.%%03u%z", &tm);
    snprintf(buf, sizeof buf, fmt, m_timeval.tv_usec / 1000);

    /* ensure there is a colon in time zone */
    if (buf[26] != ':') {
        buf[29] = '\0';
        buf[28] = buf[27];
        buf[27] = buf[26];
        buf[26] = ':';
    }
    return std::string(buf);
  }

  /*
   * 100 Hz machine has less than 10ms accuracy
   * http://stackoverflow.com/questions/361363/how-to-measure-time-in-milliseconds-using-ansi-c
   * TODO: is ms really part of 8601
   */
  std::string DateTime::toISO8601_ms() const {
    struct tm tm;
    char buf[40], fmt[40];

    // http://stackoverflow.com/questions/1551597/using-strftime-in-c-how-can-i-format-time-exactly-like-a-unix-timestamp
    gmtime_r(&m_timeval.tv_sec, &tm);
    strftime(fmt , sizeof fmt, "%FT%T.%%03uZ", &tm);
    snprintf(buf, sizeof buf, fmt, m_timeval.tv_usec / 1000);
    return std::string(buf);
  }

  /*
   * This is no ISO standard
   * http://www.iso.org/iso/home/search.htm?qt=+date+and+time
   * To be ISO 8601 suggests to separate date and time with the letter 'T'
   * also timezone is missing
   */
  DateTime DateTime::parsePrettyString(const std::string& timeString) {
    const char* theISOFormatString = "%Y-%m-%d %H:%M:%S"; // 19
    struct tm tm;
    time_t t0;

    memset(&tm, 0, sizeof(tm));
    if (strptime(timeString.c_str(), theISOFormatString, &tm) == NULL) {
      throw std::invalid_argument("unknown time format: " + timeString);
    }
    tm.tm_isdst = -1;
    t0 = mktime(&tm);
    if (t0 == -1) {
      throw std::invalid_argument("mktime");
    }
    return DateTime(t0);
  }

  std::string DateTime::toPrettyString() const {
    struct tm tm;
    char buf[30];
    localtime_r(&m_timeval.tv_sec, &tm);
    strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
  }

  std::ostream& operator<<(std::ostream& out, const DateTime& _dt) {
    out << _dt.toString();
    return out;
  } // operator<<

  DateTime DateTime::NullDate(0);

  //================================================== StaticSchedule

  DateTime StaticSchedule::getNextOccurence(const DateTime& _from) {
    if (_from <= m_When) {
      return m_When;
    } else {
      return _from;
    }
  } // getNextOccurence

  //================================================== ICalSchedule

#if defined(HAVE_LIBICAL_ICAL_H) || defined(HAVE_ICAL_H)
  __DEFINE_LOG_CHANNEL__(ICalSchedule, lsInfo);

  ICalSchedule::ICalSchedule(const std::string& _rrule, const std::string _startDateISO) {
    m_Recurrence = icalrecurrencetype_from_string(_rrule.c_str());
    m_StartDate = icaltime_from_string(_startDateISO.c_str());
    m_ICalIterator = icalrecur_iterator_new(m_Recurrence, m_StartDate);
    m_NextSchedule = icalrecur_iterator_next(m_ICalIterator);

    // setup iterator to point it to the next occurence
    DateTime now;
    time_t t0 = now.secondsSinceEpoch();
    if(!m_StartDate.is_utc) {
      t0 += now.getTimezoneOffset();
    }

    leapAdjust(t0);
  } // ctor

  ICalSchedule::~ICalSchedule() {
    icalrecurrencetype_clear(&m_Recurrence);
    icalrecur_iterator_free(m_ICalIterator);
  } // dtor

  size_t ICalSchedule::leapAdjust(time_t endTime) {
    size_t skippedSchedules = 0;
    struct icaltimetype istart = icaltime_from_timet(endTime, 0);
    while(!icaltime_is_null_time(m_NextSchedule) &&
        icaltime_compare(m_NextSchedule, istart) < 0)
    {
      m_NextSchedule = icalrecur_iterator_next(m_ICalIterator);
      skippedSchedules ++;
    }
    return skippedSchedules;
  }

  DateTime ICalSchedule::getNextOccurence(const DateTime& _from) {
    DateTime result;
    DateTime current;

    time_t t0 = _from.secondsSinceEpoch() + _from.getTimezoneOffset();
    struct icaltimetype ifrom = icaltime_from_timet(t0, 0);

    if(icaltime_is_null_time(m_NextSchedule)) {
      result = DateTime::NullDate;
    }
    else if(icaltime_compare(ifrom, m_NextSchedule) < 0) {
      result = DateTime(ical_to_tm(m_NextSchedule));
    }
    else if(icaltime_compare(ifrom, m_NextSchedule) >= 0) {
      result = DateTime(ical_to_tm(m_NextSchedule));
      m_NextSchedule = icalrecur_iterator_next(m_ICalIterator);
    }
    else {
      result = DateTime::NullDate;
    }
    log(std::string(__func__) + " " + result.toString(), lsDebug);
    return result;
  } // getNextOccurence
#endif

  //================================================== TimeStamp

  /*
   * TODO cleanup this mess
   * - move macros into operator functions
   * - use Templates: typedef TimeStampImpl<timerspec> TimeStamp
   *   so we can switch between __APPLE__ and/or float
   */

  /*
   * taken from sys/time.h addapted for struct timespec
   */
  /* Convenience macros for operations on timevals.
   * NOTE: `timercmp' does not work for >= or <=.  */
# define timespec_cmp(a, b, CMP) 						      \
  (((a)->tv_sec == (b)->tv_sec) ? 					      \
   ((a)->tv_nsec CMP (b)->tv_nsec) : 					      \
   ((a)->tv_sec CMP (b)->tv_sec))
# define timespec_add(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;			      \
    (result)->tv_nsec = (a)->tv_nsec + (b)->tv_nsec;			      \
    if ((result)->tv_nsec >= 1000000000)					      \
    {									      \
      ++(result)->tv_sec;						      \
      (result)->tv_nsec -= 1000000000;					      \
    }									      \
  } while (0)
# define timespec_sub(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;			      \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;			      \
    if ((result)->tv_nsec < 0) {					      \
      --(result)->tv_sec;						      \
      (result)->tv_nsec += 1000000000;					      \
    }									      \
  } while (0)

#define SEC_TO_NSEC(s) ((s) * 1000 * 1000 * 1000)
# define timespec_2_ns(a) \
  (SEC_TO_NSEC(a.tv_sec) + a.tv_nsec) \

#define SEC_TO_MSEC(s) ((s) * 1000 * 1000)
# define timespec_2_us(a) \
  (SEC_TO_MSEC(a.tv_sec) + a.tv_nsec / 1000)

  unsigned TimeStamp::toMicroSec() const {
    return timespec_2_us(m_stamp);
  }

  TimeStamp TimeStamp::operator+(const TimeStamp &o) const {
    TimeStamp tmp;
    timespec_add(&m_stamp, &o.m_stamp, &tmp.m_stamp);
    return tmp;
  }

  TimeStamp TimeStamp::operator-(const TimeStamp &o) const {
    TimeStamp tmp;
    timespec_sub(&m_stamp, &o.m_stamp, &tmp.m_stamp);
    return tmp;
  }

  TimeStamp TimeStamp::operator+=(const TimeStamp &o) {
    timespec_add(&m_stamp, &o.m_stamp, &m_stamp);
    return *this;
  }

  TimeStamp TimeStamp::operator-=(const TimeStamp &o) {
    timespec_sub(&m_stamp, &o.m_stamp, &m_stamp);
    return *this;
  }

  bool TimeStamp::operator<(const TimeStamp &o) const {
    return timespec_cmp(&m_stamp, &o.m_stamp, <);
  }

  ICalEvent::ICalEvent(const std::string& _rrule, const std::string& _startDate, const std::string& _endDate) {
    m_Recurrence = icalrecurrencetype_from_string(_rrule.c_str());
    {
      DateTime start = DateTime::parseRFC2445(_startDate);
      time_t t0 = start.secondsSinceEpoch() + start.getTimezoneOffset();
      m_StartDate = icaltime_from_timet(t0, 0);
    }
    {
      DateTime end = DateTime::parseRFC2445(_endDate);
      time_t t0 = end.secondsSinceEpoch() + end.getTimezoneOffset();
      struct icaltimetype iEnd = icaltime_from_timet(t0, 0);
      m_Duration = icaltime_subtract(iEnd, m_StartDate);
    }
  }

  ICalEvent::~ICalEvent() {
    icalrecurrencetype_clear(&m_Recurrence);
  }

  bool ICalEvent::isDateInside(const DateTime& _date) {
    bool ret = false;
    time_t t0 = _date.secondsSinceEpoch() + _date.getTimezoneOffset();
    struct icaltimetype iDate = icaltime_from_timet(t0, 0);

    icalrecur_iterator* iCalIterator = icalrecur_iterator_new(m_Recurrence, m_StartDate);
    if (!iCalIterator) {
      return false;
    }
    struct icaltimetype rule_time;

    while (!icaltime_is_null_time(rule_time = icalrecur_iterator_next(iCalIterator))) {
      if (icaltime_compare(iDate, rule_time) < 0) {
        break;
      }
      if (icaltime_compare(iDate, rule_time) >= 0 && icaltime_compare(iDate, icaltime_add(rule_time, m_Duration)) <= 0) {
        ret = true;
        break;
      }
    }

    icalrecur_iterator_free(iCalIterator);
    return ret;
  }
}
