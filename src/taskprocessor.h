/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Authors: Sergey 'Jin' Bostandznyan <jin@dev.digitalstrom.org>
             Christian Hitz <christian.hitz@aizo.com>

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

#ifndef _TASK_PROCESSOR_H
#define _TASK_PROCESSOR_H

#include <boost/shared_ptr.hpp>
#include <pthread.h>
#include <list>

namespace dss {

  class Task {
  public:
    Task() {};
    virtual ~Task() {};
    virtual void run() = 0;
  };

  class TaskProcessor {
  public:
    TaskProcessor();
    virtual ~TaskProcessor();
    void addEvent(boost::shared_ptr<Task> event);
  private:
    std::list<boost::shared_ptr<Task> > m_eventList;
    mutable pthread_mutex_t m_eventListMutex;
    pthread_cond_t m_eventWakeupCondition;
    pthread_t m_eventProcessorThread;
    bool m_shutdownFlag;

    void eventProcessorThread();
    static void *staticThreadProc(void *arg);

    void lockList();
    void unlockList();
  protected:
    bool startThread();
  };
}

#endif //_TASK_PROCESSOR_H
