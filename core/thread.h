/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland
    Copyright (c) 2006,2008 Patrick Stählin <me@packi.ch>

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

/**
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
    Thread(const char* _name = NULL);

    virtual ~Thread();

    virtual void execute() = 0;

    bool run();
    bool stop();
    bool terminate();
    void setFreeAtTermination( bool _value ) { m_FreeAtTermination = _value; }
    bool getFreeAtTerimnation() { return m_FreeAtTermination; }

    bool isRunning() const { return m_Running; }

    const char* getThreadIdentifier() { return m_Name; }
}; //  Thread

}

#endif
