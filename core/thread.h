/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#ifndef WIN32
#include <pthread.h>
#else
#include <windows.h>
#endif

namespace dss {

/** Wrapper for pthread- and Windows threads
@author Patrick Staehlin
*/
class Thread{
private:
#ifndef WIN32
  pthread_t m_ThreadHandle;
#else
  HANDLE m_ThreadHandle;
#endif
  const char* m_Name;
  bool m_FreeAtTermination;
  bool m_Running;
protected:
  bool m_Terminated;
public:
    /** Constructs a thread with the given name */
    Thread(const char* _name = NULL);

    virtual ~Thread();

    /** This function will be called in a separate thread. */
    virtual void execute() = 0;

    /** Starts the thread. This will call execute. */
    bool run();
    /** Stops the thread */
    bool stop();
    /** Terminates the thread */
    bool terminate();
    /** Sets whether the thread should be freed after it finishes running.
     * The destructor has to make sure that no dangling references are
     * left.*/
    void setFreeAtTermination( bool _value ) { m_FreeAtTermination = _value; }
    /** Returns whether the thread gets freed after finishing. */
    bool getFreeAtTerimnation() { return m_FreeAtTermination; }

    /** Returns whether the Thread is running or not*/
    bool isRunning() const { return m_Running; }

    /** Returns the name of the thread that got passed to the constructor */
    const char* getThreadIdentifier() { return m_Name; }
private:
    virtual void cleanup() {}
}; //  Thread

}

#endif
