/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Authors: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>
             Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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

#include "sessionmanager.h"

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifdef HAVE_BUILD_INFO_H
  #include "build_info.h"
#endif

#include <boost/bind.hpp>
#include <vector>
#include <sstream>

#include "core/logger.h"
#include "core/event.h"
#include "core/session.h"
#include "core/eventinterpreterplugins.h"
#include "core/internaleventrelaytarget.h"
#include "core/hasher.h"


namespace dss {
  SessionManager::SessionManager(EventQueue& _EventQueue,
                                 EventInterpreter& _eventInterpreter,
                                 boost::shared_ptr<Security> _pSecurity,
                                 const std::string _salt)
  : m_EventQueue(_EventQueue),
    m_EventInterpreter(_eventInterpreter),
    m_pSecurity(_pSecurity),
    m_Salt(_salt),
    m_timeoutSecs(60)
  {
    m_NextSessionID = rand();
    std::ostringstream ostr;
    ostr << "DSS";
#ifdef HAVE_CONFIG_H
    ostr << " v" << DSS_VERSION;
#endif
#ifdef HAVE_BUILD_INFO_H
    ostr << " (" << DSS_RCS_REVISION << ")"
         << " (" << DSS_BUILD_USER << "@" << DSS_BUILD_HOST << ")";
#endif
    m_VersionInfo = ostr.str();
  }

  void SessionManager::sendCleanupEvent() {
    boost::shared_ptr<Event> pEvent(new Event("webSessionCleanup"));
    pEvent->setProperty("time", "+" + intToString(m_timeoutSecs));
    m_EventQueue.pushEvent(pEvent);
  }

  void SessionManager::setupCleanupEventRelayTarget() {
    if (m_pRelayTarget == NULL) {
      EventInterpreterInternalRelay* pRelay =
        dynamic_cast<EventInterpreterInternalRelay*>(m_EventInterpreter.getPluginByName(EventInterpreterInternalRelay::getPluginName()));
      m_pRelayTarget = boost::shared_ptr<InternalEventRelayTarget>(new InternalEventRelayTarget(*pRelay));

      if (m_pRelayTarget != NULL) {
        boost::shared_ptr<EventSubscription> cleanupEventSubscription(
                new dss::EventSubscription(
                    "webSessionCleanup",
                    EventInterpreterInternalRelay::getPluginName(),
                    m_EventInterpreter,
                    boost::shared_ptr<SubscriptionOptions>())
        );
        m_pRelayTarget->subscribeTo(cleanupEventSubscription);
        m_pRelayTarget->setCallback(boost::bind(&SessionManager::cleanupSessions, this, _1, _2));
        this->sendCleanupEvent();
      }
    }
  }

  boost::shared_ptr<Session> SessionManager::createSession() {
    setupCleanupEventRelayTarget();

    boost::shared_ptr<Session> result(new Session(generateToken()));
    result->setTimeout(m_timeoutSecs);
    return result;
  }

  std::string SessionManager::registerSession() {
    m_MapMutex.lock();

    boost::shared_ptr<Session> session = createSession();
    std::string id = session->getID();
    m_Sessions[id] = session;
    m_MapMutex.unlock();
    return id;
  }

  std::string SessionManager::registerApplicationSession() {
    m_MapMutex.lock();

    boost::shared_ptr<Session> session = createSession();
    session->markAsApplicationSession();
    std::string id = session->getID();
    m_Sessions[id] = session;
    m_MapMutex.unlock();
    return id;
  }

  std::string SessionManager::generateToken() {
    Hasher hasher;
    if(!m_Salt.empty()) {
      hasher.add(m_Salt);
    } else {
      Logger::getInstance()->log("SessionManager: No salt specified, sessions ids might not be secure", lsWarning);
    }
    hasher.add(m_VersionInfo);
    hasher.add(m_NextSessionID);
    hasher.add(DateTime().toString());
    m_NextSessionID++;
    std::string tmp = hasher.str();
    Hasher stage2;
    stage2.add(tmp);
    return stage2.str();
  }

  boost::shared_ptr<Session> SessionManager::getSession(const std::string& _id) {
    m_MapMutex.lock();
    boost::shared_ptr<Session> rv = m_Sessions[_id];
    m_MapMutex.unlock();
    return rv;
  }

  void SessionManager::removeSession(const std::string& _id) {
    m_MapMutex.lock();
    std::map<const std::string, boost::shared_ptr<Session> >::iterator i = m_Sessions.find(_id);
    if(i != m_Sessions.end()) {
      m_Sessions.erase(i);
    } else {
      Logger::getInstance()->log("Tried to remove nonexistent session!", lsWarning);
    }
    m_MapMutex.unlock();
  }

  void SessionManager::touchSession(const std::string& _id) {
    m_MapMutex.lock();
    std::map<const std::string, boost::shared_ptr<Session> >::iterator i = m_Sessions.find(_id);
    if(i != m_Sessions.end()) {
      m_Sessions[_id]->touch();
    }
    m_MapMutex.unlock();
  }

  void SessionManager::cleanupSessions(Event& _event, const EventSubscription& _subscription) {
    m_MapMutex.lock();
    std::map<const std::string, boost::shared_ptr<Session> >::iterator i;
    for(i = m_Sessions.begin(); i != m_Sessions.end(); ) {
      boost::shared_ptr<Session> s = i->second;
      if((s == NULL) || (!s->isStillValid())) {
        m_Sessions.erase(i++);
      } else {
        ++i;
      }
    }

    m_MapMutex.unlock();
    this->sendCleanupEvent();
  }
}

