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

#include "datetools.h"

#include <cstring>

namespace dss {

  /**
   * Converts \tm to local timezone of this machine
   * @tm contains '2009-01-02T15:00:00+0300'
   * @tz_offset seconds east of utc
   * @ret struct tm
   */
  static struct tm convertTZ(struct tm tm, time_t tz_offset) {
    time_t t0;

    tm.tm_isdst = -1; /* rely on mktime for daylight saving time */
    t0 = mktime(&tm);
    if (t0 == -1) {
      throw std::invalid_argument("mktime");
    }

    /* timezone: seconds west of utc (man tzset) */
    t0 -= (tz_offset + timezone);
    t0 += tm.tm_isdst * 3600; // TODO, probably okay
    localtime_r(&t0, &tm);
    return tm;
  }

  /**
   * TODO icaltime_as_timet sufficient?
   */
  static struct tm ical_to_tm(const icaltimetype& icalTime) {
    struct tm tm;
    memset(&tm, '\0', sizeof(tm));
    if (icalTime.is_utc) {
      time_t t0 = icaltime_as_timet(icalTime);
      localtime_r(&t0, &tm);
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
      if (mktime(&tm) == -1) {
        throw std::invalid_argument("mktime");
      }
    }
    return tm;
  } // ical_to_tm

  //================================================== DateTime

  DateTime::DateTime() {
    time_t now;
    time( &now );
    localtime_r(&now, &m_DateTime);
  } // ctor

  DateTime::DateTime(time_t _time) {
    localtime_r(&_time, &m_DateTime);
  } // ctor(time_t)

  DateTime::DateTime(const DateTime& _copy)
  : m_DateTime(_copy.m_DateTime)
  { } // ctor(copy)

  DateTime::DateTime(const struct tm& _tm) {
    m_DateTime = _tm;
  }

  void DateTime::validate() {
    time_t t0 = mktime(&m_DateTime);
    if (t0 == -1) {
      throw DSSException("Invalid time");
    }
  } // validate

  DateTime DateTime::addHour(const int _hours) const {
    DateTime result(*this);
    result.m_DateTime.tm_hour += _hours;
    mktime(&result.m_DateTime);
    return result;
  } // addHour

  DateTime DateTime::addMinute(const int _minutes) const {
    DateTime result(*this);
    result.m_DateTime.tm_min += _minutes;
    mktime(&result.m_DateTime);
    return result;
  } // addMinute

  DateTime DateTime::addSeconds(const int _seconds) const {
    DateTime result(*this);
    result.m_DateTime.tm_sec += _seconds;
    mktime(&result.m_DateTime);
    return result;
  } // addSeconds

  DateTime DateTime::addMonth(const int _month) const {
    DateTime result(*this);
    result.m_DateTime.tm_mon += _month;
    mktime(&result.m_DateTime);
    return result;

  } // addMonth

  DateTime DateTime::addYear(const int _years) const {
    DateTime result(*this);
    result.m_DateTime.tm_year += _years;
    mktime(&result.m_DateTime);
    return result;
  } // addYear

  DateTime DateTime::addDay(const int _days) const {
    DateTime result(*this);
    result.m_DateTime.tm_mday += _days;
    mktime(&result.m_DateTime);
    return result;
  } // addDay

  int DateTime::getDay() const {
    return m_DateTime.tm_mday;
  } // getDay

  int DateTime::getMonth() const {
    return m_DateTime.tm_mon;
  } // getMonth

  int DateTime::getYear() const {
    return m_DateTime.tm_year + 1900;
  } // getYear

  int DateTime::getHour() const {
    return m_DateTime.tm_hour;
  } // getHour

  int DateTime::getMinute() const {
    return m_DateTime.tm_min;
  } // getMinute

  int DateTime::getSecond() const {
    return m_DateTime.tm_sec;
  } // getSecond

  int DateTime::getDayOfYear() const {
    return m_DateTime.tm_yday;
  } // getDayOfYear

  Weekday DateTime::getWeekday() const {
    return (Weekday)m_DateTime.tm_wday;
  } // getWeekday

  void DateTime::setDay(const int _value) {
    m_DateTime.tm_mday = _value;
    validate();
  } // setDay

  void DateTime::setMonth(const int _value) {
    m_DateTime.tm_mon = _value;
    validate();
  } // setMonth

  void DateTime::setYear(const int _value) {
    m_DateTime.tm_year = _value - 1900;
    validate();
  } // setYear

  void DateTime::setHour(const int _value) {
    m_DateTime.tm_hour = _value;
    validate();
  } // setHour

  void DateTime::setMinute(const int _value) {
    m_DateTime.tm_min = _value;
    validate();
  } // setMinute

  void DateTime::setSecond(const int _value) {
    m_DateTime.tm_sec = _value;
    validate();
  } // setSecond

  void DateTime::setDate(int _day, int _month, int _year) {
    m_DateTime.tm_mday = _day;
    m_DateTime.tm_mon = _month;
    m_DateTime.tm_year = _year - 1900;
    validate();
  } // setDate

  void DateTime::setTime(int _hour, int _minute, int _second) {
    m_DateTime.tm_hour = _hour;
    m_DateTime.tm_min = _minute;
    m_DateTime.tm_sec = _second;
    validate();
  } // setTime

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
    struct tm self = m_DateTime;
    struct tm other = _other.m_DateTime;
    return static_cast<int>(difftime(mktime(&self), mktime(&other)));
  } // difference

  time_t DateTime::secondsSinceEpoch() const {
    struct tm self = m_DateTime;
    return mktime(&self);
  }

  long int DateTime::getTimezoneOffset() const {
    return m_DateTime.tm_gmtoff;
  }

  DateTime::operator std::string() const {
    return toString();
  } // operator std::string()

  std::string DateTime::toString() const {
    return toPrettyString();
  }

  std::string DateTime::toRFC2822String() const {
    static const char* theRFC2822FormatString = "%a, %d %b %Y %T %z";
    char buf[32];
    strftime(buf, 32, theRFC2822FormatString, &m_DateTime);
    return std::string(buf);
  } // toRFC2822String

  std::string DateTime::toRFC2445IcalDataTime() const {
    /*
     * http://www.ietf.org/rfc/rfc2445.txt
     * http://www.kanzaki.com/docs/ical/dateTime.html
     */
    char buf[20];
    strftime(buf, sizeof buf, "%Y%m%dT%H%M%S", &m_DateTime);
    return std::string(buf);
  }

  DateTime DateTime::parseRFC2445(const std::string& timeString) {
    struct tm tm;
    time_t t0;
    bool utc = *timeString.rbegin() == 'Z';

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
#if defined(__CYGWIN__)
      t0 += -_timezone;
#else
      t0 += tm.tm_gmtoff;
#endif
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
    if (!end || (*end != '\0' && std::string(end) != ":00")) {
      throw std::invalid_argument("ISO8601: invalid format " + in);
    }

    try {
      // TODO __CYGWIN__ probably fails here
      return DateTime(convertTZ(tm, tm.tm_gmtoff));
    } catch (std::invalid_argument &e) {
      throw std::invalid_argument(std::string("ISO8601: ") + e.what() + " " + in);
    }
  }

  std::string DateTime::toISO8601() const {
    /*
     * C++11: http://en.cppreference.com/w/cpp/chrono/c/strftime
     * http://stackoverflow.com/questions/9527960/how-do-i-construct-an-iso-8601-datetime-in-c
     * TODO: is ms really part of 8601
     */
    char buf[sizeof "2011-10-08T07:07:09.000+02:00"];
    strftime(buf, sizeof buf, "%FT%T%z", &m_DateTime);
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

    memset(&tm, 0, sizeof(tm));
    if (strptime(timeString.c_str(), theISOFormatString, &tm) == NULL) {
      throw std::invalid_argument("unknown time format: " + timeString);
    }
    tm.tm_isdst = -1;
    if (mktime(&tm) == -1) {
      throw std::invalid_argument("mktime");
    }
    return DateTime(tm);
  }

  std::string DateTime::toPrettyString() const {
    char buf[20];
    strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &m_DateTime);
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
    }
    return DateTime::NullDate;
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
    struct icaltimetype istart = icaltime_from_timet(t0, 0);

    while(!icaltime_is_null_time(m_NextSchedule) &&
        icaltime_compare(m_NextSchedule, istart) < 0)
    {
      m_NextSchedule = icalrecur_iterator_next(m_ICalIterator);
    }
  } // ctor

  ICalSchedule::~ICalSchedule() {
    icalrecurrencetype_clear(&m_Recurrence);
    icalrecur_iterator_free(m_ICalIterator);
  } // dtor

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
}
