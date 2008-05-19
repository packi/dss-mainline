/*
 *  datetools.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/17/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _DATE_TOOLS_H_INCLUDED
#define _DATE_TOOLS_H_INCLUDED

#include <bitset>

#include "base.h"
/*
#include <icalrecur.h>
#include <icaltime.h>
*/

#include <ical.h>
#include <vector>
#include <ostream>

using namespace std;

namespace dss {

  typedef enum {
    Sunday = 0, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday
  } Weekday;
  
  typedef enum {
    January = 0, February, March, April, May, June, July, August, September, October, November, December
  } Month;
  
  class DateTime {
  private:
    struct tm m_DateTime;
  public:
    DateTime();
    DateTime(time_t _time);
    DateTime(const DateTime& _copy);
    
    DateTime AddHour(const int _hours) const;
    DateTime AddMinute(const int _minutes) const;
    DateTime AddSeconds(const int _seconds) const;
    DateTime AddMonth(const int _month) const;
    DateTime AddYear(const int _years) const;    
    DateTime AddDay(const int _days) const; 
    
    int GetDay() const;
    int GetWeek() const;
    int GetMonth() const;
    int GetYear() const;
    
    int GetHour() const;
    int GetMinute() const;
    int GetSecond() const;
    
    void SetDay(const int _value);
    void SetMonth(const int _value);
    void SetYear(const int _value);
    void SetHour(const int _value);
    void SetMinute(const int _value);
    void SetSecond(const int _value);
    
    void SetDate(int _day, int _month, int _year);
    void SetTime(int _hour, int _minute, int _second);
    
    void ClearDate();
    void ClearTime();
    void Clear();
    
    void Validate();
    
    int GetDayOfYear() const;
    Weekday GetWeekday() const;
    
    bool Before(const DateTime& _other) const;
    bool After(const DateTime& _other) const;
    bool operator==(const DateTime& _other) const;
    bool operator!=(const DateTime& _other) const;
    bool operator<(const DateTime& _other) const;
    
    int Difference(const DateTime& _other) const;
    
    ostream& operator<<(ostream& out) const;
    
    static DateTime NullDate;
    
    static DateTime FromISO(const string& _isoStr);
  }; // DateTime
  
  ostream& operator<<(ostream& out, const DateTime& _dt);
  
  class Schedule {
  public:
    virtual DateTime GetNextOccurence(const DateTime& _from) = 0;
    virtual vector<DateTime> GetOccurencesBetween(const DateTime& _from, const DateTime& _to) = 0;
  };
  
  class StaticSchedule : public Schedule {
  private:
    DateTime m_When;
  public:
    StaticSchedule(const DateTime& _when) : m_When(_when) {}
    
    virtual DateTime GetNextOccurence(const DateTime& _from) ;
    virtual vector<DateTime> GetOccurencesBetween(const DateTime& _from, const DateTime& _to);
  };
  
  typedef enum {
    Secondly = 0, Minutely, Hourly, Daily, Weekly, Monthly, Yearly
  } RepetitionMode;
  
  class RepeatingSchedule : public Schedule {
  private:
    /** Repetition mode */
    RepetitionMode m_RepetitionMode;
    /** Interval in which to repeat. 
     * If the Mode is Minutely and m_RepeatingInterval is 5, we're scheduled every 5 minutes from m_BeginningAt through m_EndingAt.
     * Equaly, if the Mode is Hourly and the interval is 12, we're scheduled every 12 hours.
     */
    int m_RepeatingInterval;
    /** If the Mode is Weekly, the m_Weekdays represents a bit-set of the days, when we're being scheduled */
    int m_Weekdays;
    DateTime m_BeginingAt;
    DateTime m_EndingAt;
  private:
    int GetIntervalInSeconds();
  public:
    RepeatingSchedule(RepetitionMode _mode, int _interval, DateTime _beginingAt);
    RepeatingSchedule(RepetitionMode _mode, int _interval, DateTime _beginingAt, DateTime _endingAt);

    virtual DateTime GetNextOccurence(const DateTime& _from) ;
    virtual vector<DateTime> GetOccurencesBetween(const DateTime& _from, const DateTime& _to);
  }; // RepeatingSchedule
  
  class ICalSchedule : public Schedule {
  private:
    struct icalrecurrencetype m_Recurrence;
    struct icaltimetype m_StartDate;
  public:
    ICalSchedule(const string& _rrule, const string _startDateISO);
    
    virtual DateTime GetNextOccurence(const DateTime& _from) ;
    virtual vector<DateTime> GetOccurencesBetween(const DateTime& _from, const DateTime& _to);
  }; // ICalSchedule
  
}

#endif