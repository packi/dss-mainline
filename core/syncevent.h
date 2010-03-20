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

#ifndef NEUROSYNCEVENT_H
#define NEUROSYNCEVENT_H

#ifndef WIN32
#include <pthread.h>
#include "mutex.h"
#else
#include <windows.h>
#endif

namespace dss {

/** Wrapper for pthread-conditions and Windows events.
 * A number of threads can wait on the same SyncEvent. Using
 * signal() or broadcast() one or all of these listeners can
 * be awaken.
 * @author Patrick Staehlin <me@packi.ch>
 */
class SyncEvent{
  private:
#ifndef WIN32
    Mutex m_ConditionMutex;
    pthread_cond_t m_Condition;
    bool m_State;
#else
    HANDLE m_EventHandle;
#endif
  public:
    SyncEvent();
    virtual ~SyncEvent();

    /** Signals one listener */
    void signal();
    /** Signals all listeners */
    void broadcast();
    /** Waits for a signal */
    int waitFor();
    /** Waits for a signal for _timeoutMS miliseconds.
     * @return \c true if a signal was received within _timeoutMS miliseconds */
    bool waitFor( int _timeoutMS );
}; //  SyncEvent

}

#endif
