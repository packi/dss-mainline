/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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

#include "src/thread.h"
#include "src/logger.h"
#include "src/base.h"

#include <cassert>
#include <cstring>
#include <signal.h>

namespace dss {

void *Thread::ThreadStarterHelperFunc( void* _pThreadObj ) {
  Thread* thObj = static_cast<Thread*>(_pThreadObj);

  std::string message = thObj->getThreadIdentifier();
  if (!message.empty()) {
    message = std::string("Destroying thread: ") + message;
  } else {
    message = "Destroying thread: (no name)";
  }

  thObj->execute();
  thObj->m_Running = false;
  pthread_detach(thObj->m_ThreadHandle);
  thObj->m_ThreadHandle = 0;
  
  Logger::getInstance()->log(message);

  return NULL;
}

Thread::Thread(const std::string& _name )
  : m_ThreadHandle( 0 ),
    m_Name( _name ),
    m_Running( false ),
    m_Terminated( false )
{
}

Thread::~Thread() {
  if (m_ThreadHandle != 0) {
    m_Terminated = true;
    pthread_join(m_ThreadHandle, NULL);
  }
}

bool Thread::run() {
  assert( !m_Running );
  m_Running = true;
  if (!m_Name.empty()) {
    Logger::getInstance()->log(std::string("creating thread for \"") + m_Name + "\"");
  }
  int ret = pthread_create(&m_ThreadHandle, NULL, ThreadStarterHelperFunc, this);
  if (ret) {
      Logger::getInstance()->log(std::string("Thread: pthread_create: ") +
                                 strerror(ret), lsWarning);
  }
  return true;
}

bool Thread::terminate() {
  if (!m_Terminated && (m_ThreadHandle != 0)) {
    m_Terminated = true;
    int ret = pthread_join( m_ThreadHandle, NULL );
    if (ret != 0) {
      Logger::getInstance()->log("Error stopping thread: " + intToString(ret), lsError);
    }
  }
  m_ThreadHandle = 0;
  m_Running = false;
  return true;
}

}
