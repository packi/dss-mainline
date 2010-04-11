/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Authos: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#include "logger.h"
#include "sessionmanager.h"

namespace dss {
  SessionManager::SessionManager() : m_NextSessionID(0) {}

  int SessionManager::registerSession(const int _timeout) {
    m_MapMutex.lock();
    boost::shared_ptr<Session> s(new Session(m_NextSessionID++));
    int id = s->getID();
    m_Sessions[id] = s;
    m_MapMutex.unlock();
    return id;
  }

  boost::shared_ptr<Session>& SessionManager::getSession(const int _id) {
    m_MapMutex.lock();
    boost::shared_ptr<Session>& rv = m_Sessions[_id];
    m_MapMutex.unlock();
    return rv;
  }

  void SessionManager::removeSession(const int _id) {
    m_MapMutex.lock();
    boost::ptr_map<const int, boost::shared_ptr<Session> >::iterator i = m_Sessions.find(_id);
    if(i != m_Sessions.end()) {
      m_Sessions.erase(i);
    } else {
      Logger::getInstance()->log("Tried to remove nonexistent session!", lsWarning);
    }
    m_MapMutex.unlock();
  }

  void SessionManager::touchSession(const int _id) {
    m_MapMutex.lock();
    boost::ptr_map<const int, boost::shared_ptr<Session> >::iterator i = m_Sessions.find(_id);
    if(i != m_Sessions.end()) {
      m_Sessions[_id]->touch();
    }
    m_MapMutex.unlock();
  }

}

