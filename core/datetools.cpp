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
    struct tm self = m_DateTime;
    struct tm other = _other.m_DateTime;
    return difftime(mktime(&self), mktime(&other)) == 0;    
  } // operator==
  
  
}