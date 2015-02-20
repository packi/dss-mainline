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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "mutex.h"

namespace dss {

Mutex::Mutex() {
#ifndef WIN32
  (void) pthread_mutex_init( &m_Mutex, NULL );
#else
  m_Mutex = CreateMutex( NULL, false, NULL );
#endif
} // ctor


Mutex::~Mutex() {
#ifndef WIN32
  pthread_mutex_destroy( &m_Mutex );
#else
  CloseHandle( m_Mutex );
#endif
} // dtor


bool Mutex::lock() {
#ifndef WIN32
  return pthread_mutex_lock( &m_Mutex ) == 0;
#else

  return WaitForSingleObject( m_Mutex, INFINITE ) == WAIT_OBJECT_0;
#endif
} // lock


bool Mutex::tryLock() {
#ifndef WIN32
  return pthread_mutex_trylock( &m_Mutex ) == 0;
#else
  return WaitForSingleObject( m_Mutex, 0 ) == WAIT_OBJECT_0;
#endif
} // tryLock


bool Mutex::unlock() {
#ifndef WIN32
  return pthread_mutex_unlock( &m_Mutex ) == 0;
#else
  return (bool)ReleaseMutex( m_Mutex );
#endif
} // unlock

}
