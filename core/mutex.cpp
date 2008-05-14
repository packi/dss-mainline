#include "mutex.h"

namespace dss {

Mutex::Mutex()
{
#ifndef WIN32
  pthread_mutex_init( &m_Mutex, NULL );
#else
  m_Mutex = CreateMutex( NULL, false, NULL );
#endif
}


Mutex::~Mutex()
{
#ifndef WIN32
  pthread_mutex_destroy( &m_Mutex );
#else
  CloseHandle( m_Mutex );
#endif
}


bool Mutex::Lock() {
#ifndef WIN32
  return pthread_mutex_lock( &m_Mutex ) == 0;
#else
	
  return WaitForSingleObject( m_Mutex, INFINITE ) == WAIT_OBJECT_0;
#endif
} // Lock


bool Mutex::TryLock() {
#ifndef WIN32
  return pthread_mutex_trylock( &m_Mutex ) == 0;
#else
  return WaitForSingleObject( m_Mutex, 0 ) == WAIT_OBJECT_0;
#endif
} // TryLock


bool Mutex::Unlock() {
#ifndef WIN32
  return pthread_mutex_unlock( &m_Mutex ) == 0;
#else
  return (bool)ReleaseMutex( m_Mutex );
#endif
} // Unlock

//==================================================== LockableObject


LockableObject::LockableObject() :
  m_Locked( false )
{

} // ctor


LockableObject::~LockableObject() {
} // dtor


bool LockableObject::IsLocked() {
  return m_Locked;
} // IsLocked


bool LockableObject::Lock() {
  m_LockMutex.Lock();
#ifdef WIN32
  m_LockedBy = GetCurrentThreadId();
#else
  m_LockedBy = pthread_self();
#endif
  m_Locked = true;
  return m_Locked;
} // Lock


bool LockableObject::Unlock() {
  m_Locked = false;
  m_LockedBy = 0;
  m_LockMutex.Unlock();
  return true;
} // Unlock


bool LockableObject::IsLockedCurrentThread() {
  if( !m_Locked || m_LockedBy == 0 ) {
    return false;
  } else {
#ifdef WIN32    
	  return GetCurrentThreadId() == m_LockedBy;
#else
    return pthread_equal( pthread_self(), m_LockedBy ) != 0;
#endif
  }
} // IsLockedByCurrentThread


//==================================================== AssertLocked

AssertLocked::AssertLocked( LockableObject* objToLock )
  : m_ObjRef( objToLock )
{
  if( objToLock->IsLocked() && objToLock->IsLockedCurrentThread() ) {
    m_OwnLock = false;
  } else {
    m_ObjRef->Lock();
    m_OwnLock = true;
  }
} // ctor


AssertLocked::~AssertLocked() {
  if( m_OwnLock ) {
    m_ObjRef->Unlock();
  }
} // dtor


}
