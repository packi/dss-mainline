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

#ifndef _DATE_TOOLS_H_INCLUDED
#define _DATE_TOOLS_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base.h"

#ifdef HAVE_LIBICAL_ICAL_H
#include <libical/ical.h>
#elif defined(HAVE_ICAL_H)
#include <ical.h>
#endif
#include <vector>
#include <ostream>
#include <sys/time.h>

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
      * @param _time Time as seconds since epoch
      */
    DateTime(time_t _time);
    /** Copy constuctor */
    DateTime(const DateTime& _copy);
    DateTime(const struct tm& _tm);
    DateTime(const struct icaltimetype& _icaltime);

    /** Adds \a _hours hours to the time and normalizes the DateTime */
    DateTime addHour(const int _hours) const;
    /** Adds \a _minutes minutes to the time and normalizes the DateTime */
    DateTime addMinute(const int _minutes) const;
    /** Adds \a _seconds seconds to the time and normalizes the DateTime */
    DateTime addSeconds(const int _seconds) const;
    /** Adds \a _month months to the date and normalizes the DateTime */
    DateTime addMonth(const int _month) const;
    /** Adds \a _years years to the date and normalizes the DateTime */
    DateTime addYear(const int _years) const;
    /** Adds \a _days days to the date and normalizes the DateTime */
    DateTime addDay(const int _days) const;

    /** Returns the day of month */
    int getDay() const;
    /** Returns the month */
    int getMonth() const;
    /** Returns the year */
    int getYear() const;

    /** Returns the hour */
    int getHour() const;
    /** Returns the minute */
    int getMinute() const;
    /** Returns the second */
    int getSecond() const;

    /** Sets the day of month */
    void setDay(const int _value);
    /** Sets the month */
    void setMonth(const int _value);
    /** Sets the year */
    void setYear(const int _value);
    /** Sets the hour */
    void setHour(const int _value);
    /** Sets the minute */
    void setMinute(const int _value);
    /** Sets the second */
    void setSecond(const int _value);

    /** Sets the date part without touching the time */
    void setDate(int _day, int _month, int _year);
    /** Sets the time part without touching the date */
    void setTime(int _hour, int _minute, int _second);

    /** Clears the date part as in setting it to zero */
    void clearDate();
    /** Clears the time part as in settin it to zero */
    void clearTime();
    /** Clears the date and time part
      * @see clearDate
      * @see clearTime
      */
    void clear();

    /** Normalizes the date/time information */
    void validate();

    /** Returns the day of year */
    int getDayOfYear() const;
    /** Returns the weekday */
    Weekday getWeekday() const;

    /** Returns true if the instance is before \a _other */
    bool before(const DateTime& _other) const;
    /** Returns true if the instance is after \a _other */
    bool after(const DateTime& _other) const;
    /** Returns true if the instance and \a _other represent the same date and time */
    bool operator==(const DateTime& _other) const;
    /** Returns true if the instance and \a _other do not represent the same time and date */
    bool operator!=(const DateTime& _other) const;
    /** Returns true if the instance is before _other.
      * @see before */
    bool operator<(const DateTime& _other) const;
    /** Returns true if the instance is after _other.
      * @see after */
    bool operator>(const DateTime& _other) const;
    /** Returns true if the instance is before or equal _other.
      * @see before */
    bool operator<=(const DateTime& _other) const;
    /** Returns true if the instance is after or equal _other.
      * @see after */
    bool operator>=(const DateTime& _other) const;

    /** Returns the difference in days */
    int difference(const DateTime& _other) const;

    /** Returns the seconds since epoch */
    time_t secondsSinceEpoch() const;

    /** Returns the offset in seconds from GMT */
    long int getTimezoneOffset() const;

    std::ostream& operator<<(std::ostream& out) const;
    operator std::string() const;
    std::string toString() const;
    std::string toRFC2822String() const;

    /** The NullDate has it's date and time parts set to 0. It should
      * be used for default values. */
    static DateTime NullDate;

   /** Creates an instance from an ISO date.
     * @param _isoStr DateTime string formatted as "yyyymmddThhmmssZ"
     * @throw invalid_argument if a malformatted \a _isoStr is provided
     */
    static DateTime fromISO(const std::string& _isoStr);
  }; // DateTime

  std::ostream& operator<<(std::ostream& out, const DateTime& _dt);

  /** A Schedule is an abstract construct that's able to determine the
    * next occurence or even the next occurences. */
  class Schedule {
  public:
     virtual ~Schedule() {}
    /** Returns the date of the next occurence.
      * @return \a DateTime::NullDate or a \a DateTime value after \a _from
      */
    virtual DateTime getNextOccurence(const DateTime& _from) = 0;
    /** Lists all ocurrences between \a _from and \a _to.
      * @return A list containing dates between \a _from and \a _to or an empty std::vector
      */
    virtual std::vector<DateTime> getOccurencesBetween(const DateTime& _from, const DateTime& _to) = 0;
  };

  /** Schedule that's scheduled on a specific DateTime */
  class StaticSchedule : public Schedule {
  private:
    DateTime m_When;
  public:
    StaticSchedule(const DateTime& _when) : m_When(_when) {}
    virtual ~StaticSchedule() {}

    virtual DateTime getNextOccurence(const DateTime& _from);
    virtual std::vector<DateTime> getOccurencesBetween(const DateTime& _from, const DateTime& _to);
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
    int getIntervalInSeconds();
  public:
    RepeatingSchedule(RepetitionMode _mode, int _interval, DateTime _beginingAt);
    RepeatingSchedule(RepetitionMode _mode, int _interval, DateTime _beginingAt, DateTime _endingAt);
    virtual ~RepeatingSchedule() {}

    virtual DateTime getNextOccurence(const DateTime& _from) ;
    virtual std::vector<DateTime> getOccurencesBetween(const DateTime& _from, const DateTime& _to);
  }; // RepeatingSchedule

#if defined(HAVE_LIBICAL_ICAL_H) || defined(HAVE_ICAL_H)
  /** Schedule that gets it's schedule from an iCal's RRULE */
  class ICalSchedule : public Schedule {
  private:
    struct icalrecurrencetype m_Recurrence;
    struct icaltimetype m_StartDate;
    struct icaltimetype m_NextSchedule;
    icalrecur_iterator* m_ICalIterator;
  public:
    ICalSchedule(const std::string& _rrule, const std::string _startDateISO);
    virtual ~ICalSchedule();

    virtual bool hasNextOccurence(const DateTime& _from);
    virtual DateTime getNextOccurence(const DateTime& _from) ;
    virtual std::vector<DateTime> getOccurencesBetween(const DateTime& _from, const DateTime& _to);
  }; // ICalSchedule
#endif

  //================================================== Timestamp

  /** Class that can store and compute the difference to another timestamp. */
  class Timestamp {
  private:
    struct timeval m_Value;
  public:
    Timestamp() {
      gettimeofday(&m_Value, NULL);
    }

    /** Calculates the difference to \a _previous in miliseconds */
    double getDifference(const Timestamp& _previous) {
      double diffMS = ((m_Value.tv_sec*1000.0 + m_Value.tv_usec/1000.0) -
                       (_previous.m_Value.tv_sec*1000.0 + _previous.m_Value.tv_usec/1000.0));
      return diffMS;
    }
  }; // Timestamp

}

#endif
