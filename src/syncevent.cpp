/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2008 Patrick Staehlin <me@packi.ch>

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
#include <boost/thread.hpp>

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "syncevent.h"

#include <cassert>
#ifndef WIN32
#include <sys/time.h>
#endif
#include <cerrno>
#include <cstdio>

namespace dss {


SyncEvent::SyncEvent()
: m_State(false)
{
}

void SyncEvent::signal() {
  boost::mutex::scoped_lock lock(m_ConditionMutex);
  m_State = true;
  m_Condition.notify_one();
} // signal

void SyncEvent::broadcast() {
  boost::mutex::scoped_lock lock(m_ConditionMutex);
  m_State = true;
  m_Condition.notify_all();
} // broadcast

int SyncEvent::waitFor() {
  boost::mutex::scoped_lock lock(m_ConditionMutex);

  m_State = false;
  while(!m_State) {
    m_Condition.wait(lock);
  }
  return m_State;
} // waitFor

bool SyncEvent::waitFor(int _timeoutMS) {
  boost::mutex::scoped_lock lock(m_ConditionMutex);
  boost::system_time const timeout = boost::get_system_time() +
                                     boost::posix_time::milliseconds(_timeoutMS);

  m_State = false;
  while(!m_State) {
    if (!m_Condition.timed_wait(lock, timeout)) {
      return false;
    }
  }

  return m_State;
} // waitFor

} // namespace
