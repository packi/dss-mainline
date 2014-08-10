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

  DateTime::DateTime(const struct icaltimetype& _icaltime) {
    memset(&m_DateTime, 0, sizeof(m_DateTime));
    time_t t0 = icaltime_as_timet(_icaltime);
    if(_icaltime.is_utc) {
      localtime_r(&t0, &m_DateTime);
    } else {
      //gmtime_r(&t0, &m_DateTime);
      if(!_icaltime.is_date) {
        m_DateTime.tm_sec = _icaltime.second;
        m_DateTime.tm_min = _icaltime.minute;
        m_DateTime.tm_hour = _icaltime.hour;
      }
      m_DateTime.tm_mday = _icaltime.day;
      m_DateTime.tm_mon = _icaltime.month - 1;
      m_DateTime.tm_year = _icaltime.year - 1900;
      m_DateTime.tm_isdst = -1;
      (void) mktime(&m_DateTime);
    }
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

  std::vector<DateTime> StaticSchedule::getOccurencesBetween(const DateTime& _from, const DateTime& _to) {
    std::vector<DateTime> result;
    if (_from < m_When && _to > m_When) {
      result.push_back(m_When);
    }
    return result;
  } // getOccurencesBetween

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

  void ical_to_tm(const icaltimetype& icalTime, struct tm& tm) {
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
      (void) mktime(&tm);
    }
  } // ical_to_tm

  bool ICalSchedule::hasNextOccurence(const DateTime& _from) {
    if(icaltime_is_null_time(m_NextSchedule)) {
      return false;
    }
    return true;
  } // hasNextOccurence

  DateTime ICalSchedule::getNextOccurence(const DateTime& _from) {
    DateTime result;
    DateTime current;

    time_t t0 = _from.secondsSinceEpoch() + _from.getTimezoneOffset();
    struct icaltimetype ifrom = icaltime_from_timet(t0, 0);

    if(icaltime_is_null_time(m_NextSchedule)) {
      result = DateTime::NullDate;
    }
    else if(icaltime_compare(ifrom, m_NextSchedule) < 0) {
      result = DateTime(m_NextSchedule);
    }
    else if(icaltime_compare(ifrom, m_NextSchedule) >= 0) {
      result = DateTime(m_NextSchedule);
      m_NextSchedule = icalrecur_iterator_next(m_ICalIterator);
    }
    else {
      result = DateTime::NullDate;
    }
    log(std::string(__func__) + " " + result.toString(), lsDebug);
    return result;
  } // getNextOccurence

  std::vector<DateTime> ICalSchedule::getOccurencesBetween(const DateTime& _from, const DateTime& _to) {
    std::vector<DateTime> result;

    DateTime current;
    DateTime last;
    icalrecur_iterator* it = icalrecur_iterator_new(m_Recurrence, m_StartDate);

    // skip instances before "_from"
    do {
      last = current;
      struct icaltimetype icalTime = icalrecur_iterator_next(it);
      if (!icaltime_is_null_time(icalTime)) {
        current = DateTime(icaltime_as_timet(icalTime));
      } else {
        break;
      }
    } while (current < _from);

    if (last < _to) {
      do {
        result.push_back(last);
        struct icaltimetype icalTime = icalrecur_iterator_next(it);
        if (!icaltime_is_null_time(icalTime)) {
          last = DateTime(icaltime_as_timet(icalTime));
        } else {
          break;
        }
      } while (last < _to);
    }

    icalrecur_iterator_free(it);
    it = NULL;

    return result;
  } // getOccurencesBetween

#endif
  //================================================== RepeatingSchedule

  RepeatingSchedule::RepeatingSchedule(RepetitionMode _mode, int _interval, DateTime _beginingAt)
  : m_RepetitionMode(_mode),
    m_RepeatingInterval(_interval),
    m_BeginingAt(_beginingAt),
    m_EndingAt(DateTime::NullDate)
  {
  } // ctor

  RepeatingSchedule::RepeatingSchedule(RepetitionMode _mode, int _interval, DateTime _beginingAt, DateTime _endingAt)
  : m_RepetitionMode(_mode),
    m_RepeatingInterval(_interval),
    m_BeginingAt(_beginingAt),
    m_EndingAt(_endingAt)
  {
  } // ctor

  DateTime RepeatingSchedule::getNextOccurence(const DateTime& _from)  {
    bool hasEndDate = m_EndingAt != DateTime::NullDate;

    if (_from < m_BeginingAt) {
      return m_BeginingAt;
    } else if (hasEndDate && _from > m_EndingAt) {
      return DateTime::NullDate;
    }
    int intervalInSeconds = getIntervalInSeconds();
    int diffInSeconds = _from.difference(m_BeginingAt);
    int numIntervals = diffInSeconds / intervalInSeconds;
    if (diffInSeconds % intervalInSeconds != 0) {
      numIntervals++;
    }

    return m_BeginingAt.addSeconds(numIntervals * intervalInSeconds);
  } // getNextOccurence

  int RepeatingSchedule::getIntervalInSeconds() {
    switch(m_RepetitionMode) {
      case Weekly:
        return 60 /*sec*/ * 60 /* minutes */ * 24 /*hours*/ * 7 /*days*/ * m_RepeatingInterval;
      case Daily:
        return 60 * 60 * 24 * m_RepeatingInterval;
      case Minutely:
        return 60 * m_RepeatingInterval;
      case Monthly:
        // TODO: a month has 30 days for now... other repetition schemes would allow for "every 1st monday" etc
        return 30 * 60 /*sec*/ * 60 /* minutes */ * 24 /*hours*/ * 7 /*days*/ * m_RepeatingInterval;
      default:
        throw std::invalid_argument("m_RepetitionMode must be one of the given enum values");
    }
  } // getIntervalInSeconds

  std::vector<DateTime> RepeatingSchedule::getOccurencesBetween(const DateTime& _from, const DateTime& _to) {
    std::vector<DateTime> result;

    int intervalInSeconds = getIntervalInSeconds();
    DateTime currentDate = getNextOccurence(_from);
    while (currentDate != DateTime::NullDate && (currentDate <= _to)) {
      result.push_back(currentDate);
      currentDate = currentDate.addSeconds(intervalInSeconds);
    }
    return result;
  } // getOccurencesBetween

}
