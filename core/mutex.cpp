#include "mutex.h"

namespace dss {

Mutex::Mutex() {
#ifndef WIN32
  pthread_mutex_init( &m_Mutex, NULL );
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

//==================================================== LockableObject


LockableObject::LockableObject() :
  m_Locked( false )
{

} // ctor


LockableObject::~LockableObject() {
} // dtor


bool LockableObject::isLocked() {
  return m_Locked;
} // isLocked


bool LockableObject::lock() {
  m_LockMutex.lock();
#ifdef WIN32
  m_LockedBy = GetCurrentThreadId();
#else
  m_LockedBy = pthread_self();
#endif
  m_Locked = true;
  return m_Locked;
} // lock


bool LockableObject::unlock() {
  m_Locked = false;
  m_LockedBy = 0;
  m_LockMutex.unlock();
  return true;
} // unlock


bool LockableObject::isLockedCurrentThread() {
  if( !m_Locked || m_LockedBy == 0 ) {
    return false;
  } else {
#ifdef WIN32
	  return GetCurrentThreadId() == m_LockedBy;
#else
    return pthread_equal( pthread_self(), m_LockedBy ) != 0;
#endif
  }
} // isLockedByCurrentThread


//==================================================== AssertLocked

AssertLocked::AssertLocked( LockableObject* objToLock )
  : m_ObjRef( objToLock )
{
  if( objToLock->isLocked() && objToLock->isLockedCurrentThread() ) {
    m_OwnLock = false;
  } else {
    m_ObjRef->lock();
    m_OwnLock = true;
  }
} // ctor


AssertLocked::~AssertLocked() {
  if( m_OwnLock ) {
    m_ObjRef->unlock();
  }
} // dtor


}
