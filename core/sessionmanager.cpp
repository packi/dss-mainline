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
#include "eventinterpreterplugins.h"
#include "internaleventrelaytarget.h"

#include <boost/bind.hpp>

namespace dss {
  SessionManager::SessionManager(EventQueue& _EventQueue, EventInterpreter& _eventInterpreter, const int _timeout) : m_NextSessionID(0), m_EventQueue(_EventQueue), m_EventInterpreter(_eventInterpreter), m_timeout(_timeout), m_eventRunning(false)
  {
  } 

  int SessionManager::registerSession() {
    m_MapMutex.lock();
    if (!m_eventRunning) {
      EventInterpreterInternalRelay* relay = new EventInterpreterInternalRelay(&m_EventInterpreter);
      m_EventInterpreter.addPlugin(relay);
      InternalEventRelayTarget target(*relay);
      boost::shared_ptr<EventSubscription> cleanupEventSubscription(
              new dss::EventSubscription(
                  "webSessionCleanup",
                  EventInterpreterInternalRelay::getPluginName(),
                  m_EventInterpreter,
                  boost::shared_ptr<SubscriptionOptions>())
      );
      target.subscribeTo(cleanupEventSubscription);
      target.setCallback(boost::bind(&SessionManager::cleanupSessions, this, _1, _2));
      boost::shared_ptr<Event> pEvent(new Event("webSessionCleanup"));
      pEvent->setProperty("time", "+" + intToString(m_timeout));
      //pEvent->setProperty("time", "+30");
      m_EventQueue.pushEvent(pEvent);
      m_eventRunning = true;
    }

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

  void SessionManager::cleanupSessions(Event& _event, const EventSubscription& _subscription) {
    printf("UUUUUUUuuuuuuuuuuuuuuuuuuU\n");
  }
}

