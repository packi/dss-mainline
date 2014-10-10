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

#include <pthread.h>
#include <string>

namespace dss {

/** Wrapper for pthread-threads
@author Patrick Staehlin
*/
class Thread{
private:
  pthread_t m_ThreadHandle;
  std::string m_Name;
  bool m_Running;

  static void *ThreadStarterHelperFunc( void* _pThreadObj );
protected:
  bool m_Terminated;
public:
    /** Constructs a thread with the given name */
    Thread(const std::string& _name = "");

    virtual ~Thread();

    /** This function will be called in a separate thread. */
    virtual void execute() = 0;

    /** Starts the thread. This will call execute. */
    bool run();
    /** Terminates the thread */
    bool terminate();

    /** Returns whether the Thread is running or not*/
    bool isRunning() const { return m_Running; }

    /** Returns the name of the thread that got passed to the constructor */
    const std::string& getThreadIdentifier() { return m_Name; }
}; //  Thread

}

#endif
