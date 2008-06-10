/*
 * Copyright (c) 2006 by Patrick Stählin <me@packi.ch>
 *
 */

#include "thread.h"

#include <cassert>
#include <iostream>

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
  //pthread_exit( NULL );
  cout << "*** exiting thread" << endl;
  if( static_cast<Thread*>(_pThreadObj)->GetFreeAtTerimnation() ) {
    delete static_cast<Thread*>(_pThreadObj);
  }
  return NULL;
} // ThreadStarterHelpFunc


Thread::Thread( bool _createSuspended, char* _name )
  : m_ThreadHandle( 0 ),
    m_Name( _name ),
    m_FreeAtTermination( false ),
    m_Running( false ),
    m_Terminated( false )
{
  if( !_createSuspended ) {
    Run();
  }
}


Thread::~Thread()
{
  cout << "~Thread" << endl;
  if( m_Name != NULL  ) {
    cout << "  Thread: " << m_Name << endl;
  }
  if( !m_Terminated && (m_ThreadHandle != 0) ) {
    m_Terminated = true;
#ifndef WIN32
    pthread_join( m_ThreadHandle, NULL );
#else
    WaitForSingleObject( m_ThreadHandle, INFINITE );
#endif
  }
}

bool Thread::Run() {
  assert( !m_Running );
  m_Running = true;
  if( m_Name != NULL ) {
    cout << "creating thread for \"" << m_Name << "\" @" << this <<endl;
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
  cout << "Thread::Terminate()" << endl;
  if( m_Name != NULL ) {
    cout << " Thread: " << m_Name << endl;
  }
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
