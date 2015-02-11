/*
    Copyright (c) 2013 digitalSTROM.org, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#include <unistd.h>
#include <stdlib.h>

#include "src/watchdog.h"
#include "src/datetools.h"
#include "src/dss.h"
#include "src/logger.h"
#include "src/event.h"

#define WATCHDOG_SLEEP_TIME 180 // in seconds, 3 minutes
namespace dss {

Watchdog::Watchdog(DSS* _pDSS) : ThreadedSubsystem(_pDSS, "Watchdog") {
}

void Watchdog::initialize() {
  Subsystem::initialize();
}

void Watchdog::doStart() {
  run();
}

void Watchdog::execute() {
  while (!m_Terminated) {
    int sleepTimeSec = WATCHDOG_SLEEP_TIME;
    int eventCount = getDSS().getEventInterpreter().getEventsProcessed();
    while (!m_Terminated && (sleepTimeSec > 0)) {
       const int kMinSleepTimeSec = 10; // 10 seconds
       sleepSeconds(std::min(sleepTimeSec, kMinSleepTimeSec));
       sleepTimeSec -= kMinSleepTimeSec;
    }
    if (getDSS().getEventInterpreter().getEventsProcessed() == eventCount) {
      log("Watchdog noticed event interpreter lockup, event count stuck at " +
              intToString(eventCount), lsFatal);
      abort();
    }
  }
}

} // namespace dss

