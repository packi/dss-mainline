#ifndef NEUROSYNCEVENT_H
#define NEUROSYNCEVENT_H

#ifndef WIN32
#include <pthread.h>
#include "mutex.h"
#else
#include <windows.h>
#endif

namespace dss {

/**
  @author Patrick Staehlin <me@packi.ch>
*/
class SyncEvent{
  private:
#ifndef WIN32
	Mutex m_ConditionMutex;
    pthread_cond_t m_Condition;
#else
    HANDLE m_EventHandle;
#endif
  public:
    SyncEvent();
    virtual ~SyncEvent();

    void signal();
    void broadcast();
    int waitFor();
    bool waitFor( int _timeoutMS );
}; //  SyncEvent

}

#endif
