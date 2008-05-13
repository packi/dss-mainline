/*
 *  datetools.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/17/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "datetools.h"

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
  
  void DateTime::Validate() {
    mktime(&m_DateTime);
  } // Validate
    
  DateTime DateTime::AddHour(const int _hours) const {
    DateTime result(*this);
    result.m_DateTime.tm_hour += _hours;
    mktime(&result.m_DateTime);
    return result;
  } // AddHour
  
  DateTime DateTime::AddMinute(const int _minutes) const {
    DateTime result(*this);
    result.m_DateTime.tm_min += _minutes;
    mktime(&result.m_DateTime);
    return result;
  } // AddMinute
  
  DateTime DateTime::AddSeconds(const int _seconds) const {
    DateTime result(*this);
    result.m_DateTime.tm_sec += _seconds;
    mktime(&result.m_DateTime);
    return result;    
  } // AddSeconds
  
  DateTime DateTime::AddMonth(const int _month) const {
    DateTime result(*this);
    result.m_DateTime.tm_mon += _month;
    mktime(&result.m_DateTime);
    return result;
    
  } // AddMonth
  
  DateTime DateTime::AddYear(const int _years) const {
    DateTime result(*this);
    result.m_DateTime.tm_year += _years;
    mktime(&result.m_DateTime);
    return result;    
  } // AddYear
  
  DateTime DateTime::AddDay(const int _days) const {
    DateTime result(*this);
    result.m_DateTime.tm_mday += _days;
    mktime(&result.m_DateTime);
    return result;
  } // AddDay
  
  
  int DateTime::GetDay() const {
    return m_DateTime.tm_mday;
  } // GetDay
  
  int DateTime::GetWeek() const {
    return -1;
  } // GetWeek
  
  int DateTime::GetMonth() const {
    return m_DateTime.tm_mon;
  } // GetMonth
  
  int DateTime::GetYear() const {
    return m_DateTime.tm_year + 1900;
  } // GetYear
  
  int DateTime::GetHour() const {
    return m_DateTime.tm_hour;
  } // GetHour
  
  int DateTime::GetMinute() const {
    return m_DateTime.tm_min;
  } // GetMinute
  
  int DateTime::GetSecond() const {
    return m_DateTime.tm_sec;
  } // GetSecond
  
  int DateTime::GetDayOfYear() const {
    return m_DateTime.tm_yday;
  } // GetDayOfYear
  
  Weekday DateTime::GetWeekday() const {
    return (Weekday)m_DateTime.tm_wday;
  } // GetWeekday
  
  void DateTime::SetDay(const int _value) {
    m_DateTime.tm_mday = _value;
  } // SetDay
  
  void DateTime::SetMonth(const int _value) {
    m_DateTime.tm_mon = _value;
  } // SetMonth
  
  void DateTime::SetYear(const int _value) {
    m_DateTime.tm_year = _value - 1900;
  } // SetYear
  
  void DateTime::SetHour(const int _value) {
    m_DateTime.tm_hour = _value;
  } // SetHour
  
  void DateTime::SetMinute(const int _value) {
    m_DateTime.tm_min = _value;
  } // SetMinute
  
  void DateTime::SetSecond(const int _value) {
    m_DateTime.tm_sec = _value;
  } // SetSecond
  
  
  void DateTime::SetDate(int _day, int _month, int _year) {
    m_DateTime.tm_mday = _day;
    m_DateTime.tm_mon = _month;
    m_DateTime.tm_year = _year - 1900;
    m_DateTime.tm_gmtoff = 0;
    m_DateTime.tm_isdst = -1;
    mktime(&m_DateTime);
  } // SetDate
  
  void DateTime::SetTime(int _hour, int _minute, int _second) {
    m_DateTime.tm_hour = _hour;
    m_DateTime.tm_min = _minute;
    m_DateTime.tm_sec = _second;
    mktime(&m_DateTime);
  } // SetTime
  
  void DateTime::ClearDate() {
    SetDate(0,0,1900);
  } // ClearDate
  
  void DateTime::ClearTime() {
    SetTime(0,0,0);
  } // ClearTime
  
  void DateTime::Clear() {
    ClearDate();
    ClearTime();
  } // Clear
    
  bool DateTime::Before(const DateTime& _other) const {
    struct tm self = m_DateTime;
    struct tm other = _other.m_DateTime;
    return difftime(mktime(&self), mktime(&other)) < 0;
  } // Before
  
  bool DateTime::After(const DateTime& _other) const {
    struct tm self = m_DateTime;
    struct tm other = _other.m_DateTime;
    return difftime(mktime(&self), mktime(&other)) > 0;
  } // After
  
  bool DateTime::operator==(const DateTime& _other) const {
    return Difference(_other) == 0;
  } // operator==
  
  bool DateTime::operator!=(const DateTime& _other) const {
    return Difference(_other) != 0;
  }
  
  bool DateTime::operator<(const DateTime& _other) const {
    return Difference(_other) < 0;
  }
  
  int DateTime::Difference(const DateTime& _other) const {
    struct tm self = m_DateTime;
    struct tm other = _other.m_DateTime;
    return difftime(mktime(&self), mktime(&other));    
  }
  
  ostream& DateTime::operator<<(ostream& out) const {
    string bla = DateToISOString<string>(&m_DateTime);
    out << bla;
    return out;
  }

  ostream& operator<<(ostream& out, const DateTime& _dt) {
    out << _dt;
    return out;
  }
    
  DateTime DateTime::NullDate(0);
  
  //================================================== StaticSchedule
  
  DateTime StaticSchedule::GetNextOccurence(const DateTime& _from) {
    if(_from.Before(m_When)) {
      return m_When;
    }
    return DateTime::NullDate;
  } // GetNextOccurence
  
  vector<DateTime> StaticSchedule::GetOccurencesBetween(const DateTime& _from, const DateTime& _to) {
    vector<DateTime> result;
    if(_from.Before(m_When) && _to.After(m_When)) {
      result.push_back(m_When);
    }
    return result;
  } // GetOccurencesBetween
  
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

  DateTime RepeatingSchedule::GetNextOccurence(const DateTime& _from)  {
    bool hasEndDate = m_EndingAt != DateTime::NullDate;
    
    if(_from.Before(m_BeginingAt)) {
      return m_BeginingAt;
    } else if(hasEndDate && _from.After(m_EndingAt)) {
      return DateTime::NullDate;
    }
    int intervalInSeconds = GetIntervalInSeconds();
    int diffInSeconds = _from.Difference(m_BeginingAt);
    int numIntervals = diffInSeconds / intervalInSeconds;
    if(diffInSeconds % intervalInSeconds != 0) {
      numIntervals++;
    }
    
    return m_BeginingAt.AddSeconds(numIntervals * intervalInSeconds);
  } // GetNextOccurence
           
  int RepeatingSchedule::GetIntervalInSeconds() {
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
        throw new invalid_argument("m_RepetitionMode must be one of the given enum values");
    }             
  }
  
  vector<DateTime> RepeatingSchedule::GetOccurencesBetween(const DateTime& _from, const DateTime& _to) {
    vector<DateTime> result;
    
    int intervalInSeconds = GetIntervalInSeconds();
    DateTime currentDate = GetNextOccurence(_from);
    while(currentDate != DateTime::NullDate && (currentDate.Before(_to) || currentDate == _to)) {
      result.push_back(currentDate);
      currentDate = currentDate.AddSeconds(intervalInSeconds);
    }
    return result;
  } // GetOccurencesBetween

  
}