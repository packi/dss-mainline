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
