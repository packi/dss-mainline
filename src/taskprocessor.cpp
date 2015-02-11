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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "taskprocessor.h"
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdexcept>
#include "logger.h"

namespace dss {

  TaskProcessor::TaskProcessor() : m_shutdownFlag(false) {
    m_eventProcessorThread = 0;

    int ret = pthread_mutex_init(&m_eventListMutex, NULL);
    if (ret != 0) {
      throw std::runtime_error("failed to initialize event list mutex: " +
          std::string(strerror(errno)));
    }

    ret = pthread_cond_init(&m_eventWakeupCondition, NULL);
    if (ret != 0) {
      pthread_mutex_destroy(&m_eventListMutex);
      throw std::runtime_error("failed to initialize event wakeup condition: " +
          std::string(strerror(errno)));
    }

    ret = pthread_create(&m_eventProcessorThread, NULL,
        TaskProcessor::staticThreadProc, this);
    if (ret != 0) {
      pthread_mutex_destroy(&m_eventListMutex);
      pthread_cond_destroy(&m_eventWakeupCondition);
      m_eventProcessorThread = 0;
      throw std::runtime_error("failed to start event processor thread" +
          std::string(strerror(errno)));
    }
  }

  TaskProcessor::~TaskProcessor() {
    m_shutdownFlag = true;
    lockList();
    pthread_cond_signal(&m_eventWakeupCondition);
    unlockList();
    if (m_eventProcessorThread) {
      pthread_join(m_eventProcessorThread, NULL);
      m_eventProcessorThread = 0;
    }

    pthread_cond_destroy(&m_eventWakeupCondition);
    pthread_mutex_destroy(&m_eventListMutex);
  }

  void TaskProcessor::lockList() {
    int ret = pthread_mutex_lock(&m_eventListMutex);
    if (ret != 0) {
      throw std::runtime_error("failed to lock event list mutex: " +
          std::string(strerror(errno)));
    }
  }

  void TaskProcessor::unlockList() {
    int ret = pthread_mutex_unlock(&m_eventListMutex);
    if (ret != 0) {
      throw std::runtime_error("failed to unlock event list mutex: " +
          std::string(strerror(errno)));
    }
  }

  void *TaskProcessor::staticThreadProc(void *arg) {
    TaskProcessor *instance = (TaskProcessor *)arg;
    instance->eventProcessorThread();
    pthread_exit(NULL);
    return NULL;
  }

  void TaskProcessor::eventProcessorThread() {
    boost::shared_ptr<Task> event;

    lockList();
    while (!m_shutdownFlag) {
      if (m_eventList.empty()) {
        pthread_cond_wait(&m_eventWakeupCondition, &m_eventListMutex);
        continue;
      }

      event = m_eventList.back();
      m_eventList.pop_back();

      unlockList();

      try {
        if (event != NULL) {
          event->run();
        }
      } catch (std::runtime_error& ex) {
        Logger::getInstance()->log("TaskProcessor::eventProcessorThread "
            "caught exception: " + std::string(ex.what()), lsError);
      } catch (...) {
        Logger::getInstance()->log("TaskProcessor::eventProcessorThread "
            "caught exception", lsError);
      }

      event.reset();

      if (!m_shutdownFlag) {
        lockList();
      }
    }
    unlockList();
  }

  void TaskProcessor::addEvent(boost::shared_ptr<Task> event) {
    if (event == NULL) {
      Logger::getInstance()->log("TaskProcessor::addEvent: "
          "will not add invalid NULL event!");
      return;
    }

    lockList();
    m_eventList.push_front(event);
    pthread_cond_signal(&m_eventWakeupCondition);
    unlockList();
  }

}
