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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "sessionmanager.h"

#ifdef HAVE_BUILD_INFO_H
  #include "build_info.h"
#endif

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <vector>
#include <sstream>

#include "dss.h"
#include "src/logger.h"
#include "src/session.h"
#include "src/eventinterpreterplugins.h"
#include "src/internaleventrelaytarget.h"
#include "src/hasher.h"
#include "src/security/security.h"


namespace dss {

  //========================================== SessionTokenGenerator

  __DEFINE_LOG_CHANNEL__(SessionTokenGenerator, lsInfo);

  std::string SessionTokenGenerator::generate() {
    static SessionTokenGenerator instance;
    return instance._generate();
  }

  SessionTokenGenerator::SessionTokenGenerator() {
    m_NextSessionID = rand();
    m_VersionInfo = DSS::versionString();
    m_Salt = hexEncodeByteArray(DSS::getRandomSalt(8)); /* magic number */
  }

  std::string SessionTokenGenerator::_generate() {
    Hasher hasher;
    if(!m_Salt.empty()) {
      hasher.add(m_Salt);
    } else {
      log("No salt specified, sessions ids might not be secure", lsWarning);
    }

    hasher.add(m_VersionInfo);
    hasher.add(m_NextSessionID);
    hasher.add(DateTime().toString());
    m_NextSessionID++;
    Hasher stage2;
    stage2.add(hasher.str());
    return stage2.str();
  }

  //================================================== SessionManager

  SessionManager::SessionManager(EventQueue& _EventQueue,
                                 EventInterpreter& _eventInterpreter,
                                 boost::shared_ptr<Security> _pSecurity)
  : m_EventQueue(_EventQueue),
    m_EventInterpreter(_eventInterpreter),
    m_pSecurity(_pSecurity),
    m_timeoutSecs(60),
    m_maxSessionCount(100)
  {
  }

  void SessionManager::sendCleanupEvent() {
    DateTime now;
    boost::shared_ptr<Event> pEvent = boost::make_shared<Event>("webSessionCleanup");
    pEvent->setProperty(EventProperty::ICalStartTime, now.toRFC2445IcalDataTime());
    pEvent->setProperty(EventProperty::ICalRRule, "FREQ=SECONDLY;INTERVAL=" + intToString(kSessionCleanupInterval));
    m_EventQueue.pushEvent(pEvent);
  }

  void SessionManager::setupCleanupEventRelayTarget() {
    if (m_pRelayTarget == NULL) {
      EventInterpreterInternalRelay* pRelay =
        dynamic_cast<EventInterpreterInternalRelay*>(m_EventInterpreter.getPluginByName(EventInterpreterInternalRelay::getPluginName()));
      m_pRelayTarget = boost::make_shared<InternalEventRelayTarget>(boost::ref<EventInterpreterInternalRelay>(*pRelay));

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

    boost::shared_ptr<Session> result = boost::make_shared<Session>(SessionTokenGenerator::generate());
    result->setTimeout(m_timeoutSecs);
    return result;
  }

  std::string SessionManager::registerSession() {
    m_MapMutex.lock();
    if (m_Sessions.size() >= m_maxSessionCount) {
      Logger::getInstance()->log("SessionManager: session limit reached!", lsWarning);
      m_MapMutex.unlock();
      return std::string();
    }
    boost::shared_ptr<Session> session = createSession();
    std::string id = session->getID();
    m_Sessions[id] = session;
    m_MapMutex.unlock();
    Logger::getInstance()->log("SessionManager: register session " + id, lsDebug);
    return id;
  }

  std::string SessionManager::registerApplicationSession() {
    m_MapMutex.lock();
    if (m_Sessions.size() >= m_maxSessionCount) {
      Logger::getInstance()->log("SessionManager: session limit reached!", lsWarning);
      m_MapMutex.unlock();
      return std::string();
    }
    boost::shared_ptr<Session> session = createSession();
    session->markAsApplicationSession();
    std::string id = session->getID();
    m_Sessions[id] = session;
    m_MapMutex.unlock();
    Logger::getInstance()->log("SessionManager: register application session " + id, lsDebug);
    return id;
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

  void SessionManager::cleanupOldestSession() {
    m_MapMutex.lock();
    unsigned int age = 0;
    std::map<const std::string, boost::shared_ptr<Session> >::iterator i, oldest;
    for (oldest = m_Sessions.end(), i = m_Sessions.begin(); i != m_Sessions.end(); ) {
      boost::shared_ptr<Session> s = i->second;
      if (s == NULL) {
        m_Sessions.erase(i++);
      } else {
        if (s->getIdleTime() > age) {
          oldest = i;
        }
        i++;
      }
    }
    if (oldest != m_Sessions.end()) {
      boost::shared_ptr<Session> oSession = oldest->second;
      Logger::getInstance()->log("SessionManager: auto-cleanup session: " + oSession->getID(), lsWarning);
      m_Sessions.erase(oldest);
    }
    m_MapMutex.unlock();
  }

  void SessionManager::cleanupSessions(Event& _event, const EventSubscription& _subscription) {
    m_MapMutex.lock();
    std::map<const std::string, boost::shared_ptr<Session> >::iterator i;
    for(i = m_Sessions.begin(); i != m_Sessions.end(); ) {
      boost::shared_ptr<Session> s = i->second;
      if((s == NULL) || (!s->isStillValid())) {
        if (s) {
          Logger::getInstance()->log("SessionManager: cleanup session " + s->getID() + " (" + intToString(m_Sessions.size()) + ")", lsDebug);
        }
        m_Sessions.erase(i++);
      } else {
        i++;
      }
    }
    m_MapMutex.unlock();
  }
}

