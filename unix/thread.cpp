/*
 * Copyright (c) 2006 by Patrick Stählin <me@packi.ch>
 *
 */

#include "../core/thread.h"
#include "../core/logger.h"

#include <cassert>

#ifndef WIN32
#include <signal.h>
#endif

using namespace std;

namespace dss {

#ifndef WIN32
void*
#else
DWORD WINAPI
#endif
ThreadStarterHelperFunc( void* _pThreadObj ) {
	Thread* thObj = static_cast<Thread*>(_pThreadObj);
	thObj->Execute();
  if( static_cast<Thread*>(_pThreadObj)->GetFreeAtTerimnation() ) {
    delete static_cast<Thread*>(_pThreadObj);
  }
  return NULL;
} // ThreadStarterHelpFunc


Thread::Thread(const char* _name )
  : m_ThreadHandle( 0 ),
    m_Name( _name ),
    m_FreeAtTermination( false ),
    m_Running( false ),
    m_Terminated( false )
{ } // ctor

Thread::~Thread() {
  if( m_Name != NULL  ) {
    Logger::GetInstance()->Log(string("Destroying thread: ") + m_Name);
  } else {
    Logger::GetInstance()->Log("Destroying thread: (no name)");
  }
  if( !m_Terminated && (m_ThreadHandle != 0) ) {
    m_Terminated = true;
#ifndef WIN32
    pthread_join( m_ThreadHandle, NULL );
#else
    WaitForSingleObject( m_ThreadHandle, INFINITE );
#endif
  }
} // dtor

bool Thread::Run() {
  assert( !m_Running );
  m_Running = true;
  if( m_Name != NULL ) {
    Logger::GetInstance()->Log(string("creating thread for \"") + m_Name + "\"");
  }
#ifndef WIN32
  pthread_create( &m_ThreadHandle, NULL, ThreadStarterHelperFunc, this );
#else
  m_ThreadHandle = CreateThread( NULL, 0, &ThreadStarterHelperFunc, this, NULL, NULL );
#endif
  return true;
} // Run

bool Thread::Stop() {
#ifdef WIN32
  TerminateThread( m_ThreadHandle, 0 );
#else
    // FIXME: Hmm... Quit rude ;)
  pthread_kill( m_ThreadHandle, -9 );
#endif
  return true;
} // Stop


bool Thread::Terminate() {
  if( !m_Terminated && (m_ThreadHandle != 0) ) {
    m_Terminated = true;
#ifndef WIN32
    pthread_join( m_ThreadHandle, NULL );
#else
    WaitForSingleObject( m_ThreadHandle, INFINITE );
#endif
  }
  m_ThreadHandle = 0;
  return true;
} // Terminate


}
