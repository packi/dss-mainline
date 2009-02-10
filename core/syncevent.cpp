#include "syncevent.h"

#include <cassert>
#ifndef WIN32
#include <sys/time.h>
#endif
#include <cerrno>
#include <cstdio>

namespace dss {


SyncEvent::SyncEvent()
{
#ifndef WIN32
  pthread_cond_init( &m_Condition, NULL );
#else
  m_EventHandle = CreateEvent( NULL, false, false, NULL );
#endif
}


SyncEvent::~SyncEvent()
{
#ifndef WIN32
  pthread_cond_destroy( &m_Condition );
#else
  CloseHandle( m_EventHandle );
#endif
}


void SyncEvent::Signal() {
#ifndef WIN32
  m_ConditionMutex.Lock();

  assert( pthread_cond_signal( &m_Condition ) == 0 );

  m_ConditionMutex.Unlock();
#else
  SetEvent( m_EventHandle );
#endif
} // Signal

void SyncEvent::Broadcast() {
#ifndef WIN32
  m_ConditionMutex.Lock();

  assert(pthread_cond_broadcast(&m_Condition) == 0);

  m_ConditionMutex.Unlock();
#else
  #error SyncEvent::Broadcast is not yet implemented
#endif
}


int SyncEvent::WaitFor() {
#ifndef WIN32
  m_ConditionMutex.Lock();

  int result = pthread_cond_wait( &m_Condition, m_ConditionMutex.GetMutex() );

  m_ConditionMutex.Unlock();
  if( result != ETIMEDOUT && result != 0 ) {
    assert( false );
  }
  return result;
#else
  return WaitForSingleObject( m_EventHandle, INFINITE );
#endif
} // WaitFor


bool SyncEvent::WaitFor( int _timeoutMS ) {
#ifndef WIN32
  struct timeval now;
  struct timespec timeout;
  int timeoutSec = _timeoutMS / 1000;
  int timeoutMSec = _timeoutMS - 1000 * timeoutSec;

  m_ConditionMutex.Lock();
  gettimeofday( &now, NULL );
  timeout.tv_sec = now.tv_sec + timeoutSec;
  timeout.tv_nsec = (now.tv_usec + timeoutMSec * 1000) * 1000;
  int result = pthread_cond_timedwait( &m_Condition, m_ConditionMutex.GetMutex(), &timeout );
  m_ConditionMutex.Unlock();
  if(result != ETIMEDOUT && result != 0) {
    //perror("SyncEvent::WaitFor");
  }
  return !(result == ETIMEDOUT);
#else
  return WaitForSingleObject( m_EventHandle, _timeoutMS ) == WAIT_OBJECT_0;
#endif
} // WaitFor


} // namespace
