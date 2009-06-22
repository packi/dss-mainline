/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland
    Copyright (c) 2008 Patrick Staehlin <me@packi.ch>

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

/**
  @author Patrick Staehlin <me@packi.ch>
*/
class SyncEvent{
  private:
#ifndef WIN32
	Mutex m_ConditionMutex;
    pthread_cond_t m_Condition;
#else
    HANDLE m_EventHandle;
#endif
  public:
    SyncEvent();
    virtual ~SyncEvent();

    void signal();
    void broadcast();
    int waitFor();
    bool waitFor( int _timeoutMS );
}; //  SyncEvent

}

#endif
