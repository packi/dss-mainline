/*
 * Copyright (c) 2006 by Patrick Stählin <me@packi.ch>
 *
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
  char* m_Name;
  bool m_FreeAtTermination;
  bool m_Running;
protected:
  bool m_Terminated;
public:
    Thread( bool _createSuspended = false, char* _name = NULL );

    virtual ~Thread();

    virtual void Execute() = 0;

    bool Run();
    bool Stop();
    bool Terminate();
    void SetFreeAtTermination( bool _value ) { m_FreeAtTermination = _value; };
    bool GetFreeAtTerimnation() { return m_FreeAtTermination; };

    bool IsRunning() const { return m_Running; };

}; //  Thread

}

#endif
