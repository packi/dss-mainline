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

#ifndef NEUROMUTEX_H
#define NEUROMUTEX_H

#ifndef WIN32
#include <pthread.h>
#else
#include <windows.h>
#endif

namespace dss {

/**
@author Patrick Staehlin
*/
class Mutex{
private:
#ifndef WIN32
	pthread_mutex_t m_Mutex;
#else
	HANDLE m_Mutex;
#endif
public:
    Mutex();
    virtual ~Mutex();

    virtual bool lock();
    virtual bool tryLock();
    virtual bool unlock();

#ifndef WIN32
	pthread_mutex_t* getMutex() { return &m_Mutex; }
#endif
}; //  Mutex


/** Provides an object with a lockable interface
 */
class LockableObject {
  private:
    Mutex m_LockMutex;
    bool m_Locked;
#ifdef WIN32
	DWORD m_LockedBy;
#else
    pthread_t m_LockedBy;
#endif
  public:
    LockableObject();
    virtual ~LockableObject();
    bool isLocked();
    bool isLockedCurrentThread();
    bool lock();
    bool unlock();
}; // LockableObject;

/** Asserts that an object can be accessed from the current thread.
 * If another thread instanciates assertLocked( obj ), it requires
 * the other thread to wait for a proper lock.
 */
class AssertLocked {
  private:
    bool m_OwnLock;
    LockableObject* m_ObjRef;
  public:
    AssertLocked( LockableObject* objToLock );
    ~AssertLocked();
}; // LockAsserter

}

#endif
