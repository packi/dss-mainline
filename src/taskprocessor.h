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

#include <boost/weak_ptr.hpp>
#include <pthread.h>
#include <list>
#include <atomic>

namespace dss {

  class Task {
  public:
    Task() {};
    virtual ~Task() {};
    virtual void run() = 0;
    virtual void cancel() {};
  };

  /// Guard canceling the task in its descructor.
  class TaskScope {
  public:
    explicit TaskScope(const boost::shared_ptr<Task>& task = boost::shared_ptr<Task>()) : m_task(task) {}
    TaskScope(const TaskScope&) = delete;
    TaskScope & operator= (const TaskScope&) = delete;
    TaskScope(TaskScope&& x) {
      m_task = std::move(x.m_task);
      x.m_task.reset();
    }
    TaskScope & operator= (TaskScope&& x) {
      m_task = std::move(x.m_task);
      x.m_task.reset();
      return *this;
    }
    ~TaskScope() {
      auto task = m_task.lock();
      if (task) {
        task->cancel();
      }
    }
  private:
    boost::weak_ptr<Task> m_task;
  };

  // Task calling functor passed in the constructor
  template< class F >
  class FunctorTask: public Task {
  public:
    static_assert(std::is_void<decltype(std::declval<F>()())>::value, "F must be of type `void f();`");
    FunctorTask(F &&f) : m_cancelled(false), m_f(std::move(f)) {}

    virtual void run() /* override */ {
      if (m_cancelled.load()) {
        return;
      }
      m_f();
    }
    void cancel() /* override */ { m_cancelled.store(true); }

  private:
    std::atomic_bool m_cancelled;
    F m_f;
  };
  template< class F >
  boost::shared_ptr<Task> makeTask(F &&f) { return boost::shared_ptr<Task>(new FunctorTask<F>(std::move(f))); }

  class TaskProcessor {
  public:
    TaskProcessor();
    virtual ~TaskProcessor();
    void addEvent(boost::shared_ptr<Task> event);

  private:
    TaskProcessor(const TaskProcessor& that);

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
