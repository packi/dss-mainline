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

#include "base.h"

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

  /** Class that supports various date and time calculations */
  class DateTime {
  private:
    struct tm m_DateTime;
  public:
    /** Initializes the instance to be equal to \a DateTime::NullDate */
    DateTime();
    /** Initializes the instance to be equal to \a _time.
      * @param _time Time as localtime
      */
    DateTime(time_t _time);
    /** Copy constuctor */
    DateTime(const DateTime& _copy);

    /** Adds \a _hours hours to the time and normalizes the DateTime */
    DateTime AddHour(const int _hours) const;
    /** Adds \a _minutes minutes to the time and normalizes the DateTime */
    DateTime AddMinute(const int _minutes) const;
    /** Adds \a _seconds seconds to the time and normalizes the DateTime */
    DateTime AddSeconds(const int _seconds) const;
    /** Adds \a _month months to the date and normalizes the DateTime */
    DateTime AddMonth(const int _month) const;
    /** Adds \a _years years to the date and normalizes the DateTime */
    DateTime AddYear(const int _years) const;
    /** Adds \a _days days to the date and normalizes the DateTime */
    DateTime AddDay(const int _days) const;

    /** Returns the day of month */
    int GetDay() const;
    /** Returns the week */
    int GetWeek() const;
    /** Returns the month */
    int GetMonth() const;
    /** Returns the year */
    int GetYear() const;

    /** Returns the hour */
    int GetHour() const;
    /** Returns the minute */
    int GetMinute() const;
    /** Returns the second */
    int GetSecond() const;

    /** Sets the day of month */
    void SetDay(const int _value);
    /** Sets the month */
    void SetMonth(const int _value);
    /** Sets the year */
    void SetYear(const int _value);
    /** Sets the hour */
    void SetHour(const int _value);
    /** Sets the minute */
    void SetMinute(const int _value);
    /** Sets the second */
    void SetSecond(const int _value);

    /** Sets the date part without touching the time */
    void SetDate(int _day, int _month, int _year);
    /** Sets the time part without touching the date */
    void SetTime(int _hour, int _minute, int _second);

    /** Clears the date part as in setting it to zero */
    void ClearDate();
    /** Clears the time part as in settin it to zero */
    void ClearTime();
    /** Clears the date and time part
      * @see ClearDate
      * @see ClearTime
      */
    void Clear();

    /** Normalizes the date/time information */
    void Validate();

    /** Returns the day of year */
    int GetDayOfYear() const;
    /** Returns the weekday */
    Weekday GetWeekday() const;

    /** Returns true if the instance is before _other */
    bool Before(const DateTime& _other) const;
    /** Returns true if the instance is after _other */
    bool After(const DateTime& _other) const;
    /** Returns true if the instance and _other represent the same date and time */
    bool operator==(const DateTime& _other) const;
    /** Returns true if the instance and _other do not represent the same time and date */
    bool operator!=(const DateTime& _other) const;
    /** Returns true if the instance is before _other.
      * @see Before */
    bool operator<(const DateTime& _other) const;

    /** Returns the difference in days */
    int Difference(const DateTime& _other) const;

    ostream& operator<<(ostream& out) const;
    operator string() const;

    /** The NullDate has it's date and time parts set to 0. It should
      * be used for default values. */
    static DateTime NullDate;

   /** Creates an instance from an ISO date.
     * @param _isoStr DateTime string formatted as "yyyymmddThhmmssZ"
     * @throw invalid_argument if a malformatted \a _isoStr is provided
     */
    static DateTime FromISO(const string& _isoStr);
    /** Creates an instance from a time_t struct that is in UTC */
    static DateTime FromUTC(const time_t& _time);
    /** Creates an instance from a time_t struct and converts the internal
      * time to UTC */
    static DateTime ToUTC(const time_t& _time);
  }; // DateTime

  ostream& operator<<(ostream& out, const DateTime& _dt);

  /** A Schedule is an abstract construct that's able to determine the
    * next occurence or even the next occurences. */
  class Schedule {
  public:
     virtual ~Schedule() {}
    /** Returns the date of the next occurence.
      * @return \a DateTime::NullDate or a \a DateTime value after \a _from
      */
    virtual DateTime GetNextOccurence(const DateTime& _from) = 0;
    /** Lists all ocurrences between \a _from and \a _to.
      * @return A list containing dates between \a _from and \a _to or an empty vector
      */
    virtual vector<DateTime> GetOccurencesBetween(const DateTime& _from, const DateTime& _to) = 0;
  };

  /** Schedule that's scheduled on a specific DateTime */
  class StaticSchedule : public Schedule {
  private:
    DateTime m_When;
  public:
    StaticSchedule(const DateTime& _when) : m_When(_when) {}
    virtual ~StaticSchedule() {}

    virtual DateTime GetNextOccurence(const DateTime& _from) ;
    virtual vector<DateTime> GetOccurencesBetween(const DateTime& _from, const DateTime& _to);
  };

  typedef enum {
    Secondly = 0, Minutely, Hourly, Daily, Weekly, Monthly, Yearly
  } RepetitionMode;


  /** RepeatingSchedule */
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
    virtual ~RepeatingSchedule() {}

    virtual DateTime GetNextOccurence(const DateTime& _from) ;
    virtual vector<DateTime> GetOccurencesBetween(const DateTime& _from, const DateTime& _to);
  }; // RepeatingSchedule

  /** Schedule that gets it's schedule from an iCal's RRULE */
  class ICalSchedule : public Schedule {
  private:
    struct icalrecurrencetype m_Recurrence;
    struct icaltimetype m_StartDate;
  public:
    ICalSchedule(const string& _rrule, const string _startDateISO);
    virtual ~ICalSchedule() {}

    virtual DateTime GetNextOccurence(const DateTime& _from) ;
    virtual vector<DateTime> GetOccurencesBetween(const DateTime& _from, const DateTime& _to);
  }; // ICalSchedule

}

#endif
