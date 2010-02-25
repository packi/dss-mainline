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

#include "datetools.h"

#include <cstring>

namespace dss {

  //================================================== DateTime

  DateTime::DateTime() {
    time_t now;
    time( &now );
    localtime_r(&now, &m_DateTime);
    mktime(&m_DateTime);
  } // ctor

  DateTime::DateTime(time_t _time) {
    localtime_r(&_time, &m_DateTime);
    mktime(&m_DateTime);
  } // ctor(time_t)

  DateTime::DateTime(const DateTime& _copy)
  : m_DateTime(_copy.m_DateTime)
  { } // ctor(copy)

  DateTime::DateTime(const struct tm& _tm) {
    m_DateTime = _tm;
    validate();
  }

  void DateTime::validate() {
    mktime(&m_DateTime);
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
  } // setDay

  void DateTime::setMonth(const int _value) {
    m_DateTime.tm_mon = _value;
  } // setMonth

  void DateTime::setYear(const int _value) {
    m_DateTime.tm_year = _value - 1900;
  } // setYear

  void DateTime::setHour(const int _value) {
    m_DateTime.tm_hour = _value;
  } // setHour

  void DateTime::setMinute(const int _value) {
    m_DateTime.tm_min = _value;
  } // setMinute

  void DateTime::setSecond(const int _value) {
    m_DateTime.tm_sec = _value;
  } // setSecond

  void DateTime::setDate(int _day, int _month, int _year) {
    m_DateTime.tm_mday = _day;
    m_DateTime.tm_mon = _month;
    m_DateTime.tm_year = _year - 1900;
    m_DateTime.tm_gmtoff = 0;
    m_DateTime.tm_isdst = -1;
    mktime(&m_DateTime);
  } // setDate

  void DateTime::setTime(int _hour, int _minute, int _second) {
    m_DateTime.tm_hour = _hour;
    m_DateTime.tm_min = _minute;
    m_DateTime.tm_sec = _second;
    mktime(&m_DateTime);
  } // setTime

  void DateTime::clearDate() {
    setDate(1, 0, 1900);
  } // clearDate

  void DateTime::clearTime() {
    setTime(0,0,0);
  } // clearTime

  void DateTime::clear() {
    clearDate();
    clearTime();
  } // clear

  bool DateTime::before(const DateTime& _other) const {
    struct tm self = m_DateTime;
    struct tm other = _other.m_DateTime;
    return difftime(mktime(&self), mktime(&other)) < 0;
  } // before

  bool DateTime::after(const DateTime& _other) const {
    struct tm self = m_DateTime;
    struct tm other = _other.m_DateTime;
    return difftime(mktime(&self), mktime(&other)) > 0;
  } // after

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

  std::ostream& DateTime::operator<<(std::ostream& out) const {
    std::string bla = dateToISOString<std::string>(&m_DateTime);
    out << bla;
    return out;
  } // operator<<

  DateTime::operator std::string() const {
    return toString();
  } // operator std::string()

  std::string DateTime::toString() const {
    return dateToISOString<std::string>(&m_DateTime);
  } // toString

  std::ostream& operator<<(std::ostream& out, const DateTime& _dt) {
    _dt << out;
    return out;
  } // operator<<

  std::string EraseLeadingZeros(const std::string& _string) {
    std::string result = _string;
    while((result.size() > 1) && (result.find('0') == 0)) {
      result.erase(0,1);
    }
    return result;
  }

  DateTime DateTime::fromISO(const std::string& _isoStr) {
    DateTime result;

    if(_isoStr.size() < 8 /*date*/ + 6 /*time*/ + 2 /* 'T', 'Z' */) {
      throw std::invalid_argument("_isoStr is shorter than expected");
    }

    int year = strToInt(EraseLeadingZeros(_isoStr.substr(0, 4)));
    int month = strToInt(EraseLeadingZeros(_isoStr.substr(4, 2)));
    if(month > 12 || month == 0) {
      throw std::invalid_argument("month should be between 1 and 12");
    }
    int day = strToInt(EraseLeadingZeros(_isoStr.substr(6, 2)));

    if(_isoStr.at(8) != 'T') {
      throw std::invalid_argument("_isoStr should have a 'T' at position 8");
    }

    int hour = strToInt(EraseLeadingZeros(_isoStr.substr(9,2)));
    if(hour > 23) {
      throw std::invalid_argument("hour should be between 0 and 24");
    }
    int min = strToInt(EraseLeadingZeros(_isoStr.substr(11,2)));
    if(min > 59) {
      throw std::invalid_argument("minute should be between 0 and 59");
    }
    int sec = strToInt(EraseLeadingZeros(_isoStr.substr(13, 2)));
    if(sec > 59) {
      throw std::invalid_argument("second should be between 0 and 59");
    }

    struct tm tm;
    memset(&tm, '\0', sizeof(tm));
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1; // month is zero based "*ç"*!
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_sec = sec;

    return DateTime::fromUTC(mktime(&tm));
  } // fromISO

  DateTime DateTime::fromUTC(const time_t& _time) {
    return DateTime(_time - timezone);
  } // fromUTC

  DateTime DateTime::toUTC(const time_t& _time) {
    return DateTime(_time + timezone);
  } // toUTC

  DateTime DateTime::NullDate(0);

  //================================================== StaticSchedule

  DateTime StaticSchedule::getNextOccurence(const DateTime& _from) {
    if(_from.before(m_When)) {
      return m_When;
    }
    return DateTime::NullDate;
  } // getNextOccurence

  std::vector<DateTime> StaticSchedule::getOccurencesBetween(const DateTime& _from, const DateTime& _to) {
    std::vector<DateTime> result;
    if(_from.before(m_When) && _to.after(m_When)) {
      result.push_back(m_When);
    }
    return result;
  } // getOccurencesBetween


  //================================================== ICalSchedule

#if defined(HAVE_LIBICAL_ICAL_H) || defined(HAVE_ICAL_H)
  ICalSchedule::ICalSchedule(const std::string& _rrule, const std::string _startDateISO) {
    m_Recurrence = icalrecurrencetype_from_string(_rrule.c_str());
    m_StartDate = icaltime_from_string(_startDateISO.c_str());
  } // ctor

  ICalSchedule::~ICalSchedule() {
    icalrecurrencetype_clear(&m_Recurrence);
  } // dtor

  void ical_to_tm(const icaltimetype& icalTime, struct tm& tm) {
    memset(&tm, '\0', sizeof(tm));
    if(!icalTime.is_date) {
      tm.tm_sec = icalTime.second;
      tm.tm_min = icalTime.minute;
      tm.tm_hour = icalTime.hour;
    }
    tm.tm_mday = icalTime.day;
    tm.tm_mon = icalTime.month - 1;
    tm.tm_year = icalTime.year - 1900;
  } // ical_to_tm

  DateTime ICalSchedule::getNextOccurence(const DateTime& _from) {
    DateTime result;
    DateTime current;

    icalrecur_iterator* it = icalrecur_iterator_new(m_Recurrence, m_StartDate);
    do {
      struct icaltimetype icalTime = icalrecur_iterator_next(it);
      if(icaltime_is_null_time(icalTime)) {
        break;
      }

      struct tm tm;
      ical_to_tm(icalTime, tm);
      current = DateTime::fromUTC(mktime(&tm));
    } while(current.before(_from));
    icalrecur_iterator_free(it);
    it = NULL;

    if(current.after(_from) || current == _from) {
      result = current;
    } else {
      result = DateTime::NullDate;
    }

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
      if(!icaltime_is_null_time(icalTime)) {
        current = DateTime(icaltime_as_timet(icalTime));
      } else {
        break;
      }
    } while(current.before(_from));

    if(last.before(_to)) {

      do {
        result.push_back(last);
        struct icaltimetype icalTime = icalrecur_iterator_next(it);
        if(!icaltime_is_null_time(icalTime)) {
          last = DateTime(icaltime_as_timet(icalTime));
        } else {
          break;
        }
      } while(last.before(_to));
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

    if(_from.before(m_BeginingAt)) {
      return m_BeginingAt;
    } else if(hasEndDate && _from.after(m_EndingAt)) {
      return DateTime::NullDate;
    }
    int intervalInSeconds = getIntervalInSeconds();
    int diffInSeconds = _from.difference(m_BeginingAt);
    int numIntervals = diffInSeconds / intervalInSeconds;
    if(diffInSeconds % intervalInSeconds != 0) {
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
    while(currentDate != DateTime::NullDate && (currentDate.before(_to) || currentDate == _to)) {
      result.push_back(currentDate);
      currentDate = currentDate.addSeconds(intervalInSeconds);
    }
    return result;
  } // getOccurencesBetween

}
