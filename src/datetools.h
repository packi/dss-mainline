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
#include "logger.h"

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
    struct timeval m_timeval;
  public:
    /** Current date-time */
    DateTime();
    /** Construct from sec.usec since epoch (UTC) */
    DateTime(time_t secs, suseconds_t usecs = 0);
    DateTime(struct timeval tv) : m_timeval(tv) {}
    /** Copy constuctor */
    DateTime(const DateTime& _copy);

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

    /** Returns the day of year */
    int getDayOfYear() const;
    /** Returns the weekday */
    Weekday getWeekday() const;

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

    /** Returns the difference in seconds */
    int difference(const DateTime& _other) const;

    /** Returns the seconds since epoch */
    time_t secondsSinceEpoch() const;

    /** Returns the offset in seconds from GMT */
    long int getTimezoneOffset() const;

    operator std::string() const;
    std::string toString() const;

    std::string toRFC2822String() const;

    /**
     * Parses RFC2445 date-time
     * @param _isoStr DateTime string formatted as "yyyymmddThhmmss[Z]"
     * @throw invalid_argument if a malformatted \a _isoStr is provided
     */
    static DateTime parseRFC2445(const std::string& _isoStr);

    /**
     * Emit RFC2445 date-time format
     */
    std::string toRFC2445IcalDataTime() const;

    /**
     * parseISO8601 -- ISO8601 or similar RFC3339
     * http://www.cl.cam.ac.uk/~mgk25/iso-time.html
     * http://www.cs.tut.fi/~jkorpela/iso8601.html
     * http://www.ietf.org/rfc/rfc3339.txt
     */
    static DateTime parseISO8601(std::string in);

    /**
     * Emit ISO8601 or RFC3339 format
     * http://www.cl.cam.ac.uk/~mgk25/iso-time.html
     * http://www.cs.tut.fi/~jkorpela/iso8601.html
     * http://www.ietf.org/rfc/rfc3339.txt
     */
    std::string toISO8601() const;

    /**
     * Will emit ISO8601 format with timezone appended
     *
     * Various versions are floating around for the timezone
     * format: e.g. 02:00, 0200, 02 and not all entities
     * accept all formats
     * We emit it as '+hh:mm' since that is the format
     * mentioned in the public rfc3339 specification[2]
     * It's recomended to emit the time in UTC where no
     * timezone needs to be appended
     *
     * @see toISO8601
     * [1] http://www.cs.tut.fi/~jkorpela/iso8601.html
     * [2] http://www.ietf.org/rfc/rfc3339.txt
     */
    std::string toISO8601_local() const;

    /**
     * Emit ISO8601 or RFC3339 with ms precision
     * http://www.cl.cam.ac.uk/~mgk25/iso-time.html
     * http://www.cs.tut.fi/~jkorpela/iso8601.html
     * http://www.ietf.org/rfc/rfc3339.txt
     * TODO probably non-standard:
     * - rfc3339 has 100 ms digit precision
     * - ISO8601 seems seconds only precision(spec is 140$)
     *
     * Various versions are floating around for the timezone
     * format: e.g. 02:00, 0200, 02 and not all entities
     * accept all formats
     * We emit it as '+hh:mm' since that is the format
     * mentioned in the public rfc3339 specification[2]
     * It's recomended to emit the time in UTC where no
     * timezone needs to be appended
     */
    std::string toISO8601_ms_local() const;

    /**
     * Emit ISO8601 or RFC3339 with ms precision
     * http://www.cl.cam.ac.uk/~mgk25/iso-time.html
     * http://www.cs.tut.fi/~jkorpela/iso8601.html
     * http://www.ietf.org/rfc/rfc3339.txt
     * TODO probably non-standard:
     * - rfc3339 has 100 ms digit precision
     * - ISO8601 seems seconds only precision(spec is 140$)
     */
    std::string toISO8601_ms() const;

    /**
     * Parses human readable "2014-08-07 23:33:30" format
     * @throw invalid_argument if parsing fails
     */
    static DateTime parsePrettyString(const std::string& strTime);

    /**
     * Emits human readable "2014-08-07 23:33:30" format
     */
    std::string toPrettyString() const;

    /** The NullDate has it's date and time parts set to 0. It should
      * be used for default values. */
    static DateTime NullDate;
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
  };

  /** Schedule that's scheduled on a specific DateTime */
  class StaticSchedule : public Schedule {
  private:
    DateTime m_When;
  public:
    StaticSchedule(const DateTime& _when) : m_When(_when) {}
    virtual ~StaticSchedule() {}
    virtual DateTime getNextOccurence(const DateTime& _from);
  };

#if defined(HAVE_LIBICAL_ICAL_H) || defined(HAVE_ICAL_H)
  /** Schedule that gets it's schedule from an iCal's RRULE */
  class ICalSchedule : public Schedule {
    __DECL_LOG_CHANNEL__
  private:
    struct icalrecurrencetype m_Recurrence;
    struct icaltimetype m_StartDate;
    struct icaltimetype m_NextSchedule;
    icalrecur_iterator* m_ICalIterator;
  public:
    ICalSchedule(const std::string& _rrule, const std::string _startDateISO);
    virtual ~ICalSchedule();

    virtual DateTime getNextOccurence(const DateTime& _from) ;
  }; // ICalSchedule
#endif

  class TimeStamp {
  public:
    TimeStamp() {
      m_stamp.tv_sec = 0;
      m_stamp.tv_nsec = 0;
    };

    void timestamp();
    //unsigned toNanoSec() const; /* mind the range is only 32bit */
    unsigned toMicroSec() const;

    TimeStamp operator+(const TimeStamp &o) const;
    TimeStamp operator-(const TimeStamp &o) const;
    TimeStamp operator+=(const TimeStamp &o);
    TimeStamp operator-=(const TimeStamp &o);
    bool operator<(const TimeStamp &other) const;

  private:
#ifndef __APPLE__
    struct timespec m_stamp;
#else
#error NOT IMPLEMENTED
#endif
  };

#ifndef __APPLE__
  /* inline to minimize overhead */
  inline void TimeStamp::timestamp() {
    (void)clock_gettime(CLOCK_THREAD_CPUTIME_ID, &m_stamp);
  }
#else
#error NOT IMPLEMENTED
#endif
}
#endif
