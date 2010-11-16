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
#include <openssl/sha.h>

#include "core/logger.h"
#include "core/event.h"
#include "core/session.h"
#include "core/eventinterpreterplugins.h"
#include "core/internaleventrelaytarget.h"


namespace dss {
  SessionManager::SessionManager(EventQueue& _EventQueue, EventInterpreter& _eventInterpreter, const std::string _salt)
  : m_EventQueue(_EventQueue), m_EventInterpreter(_eventInterpreter), m_timeoutSecs(60), m_Salt(_salt)
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

  std::string SessionManager::registerSession() {
    m_MapMutex.lock();
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

    boost::shared_ptr<Session> s(new Session(generateToken()));
    s->setTimeout(m_timeoutSecs);
    std::string id = s->getID();
    m_Sessions[id] = s;
    m_MapMutex.unlock();
    return id;
  }

  std::string SessionManager::generateToken() {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    if(!m_Salt.empty()) {
      SHA256_Update(&ctx, m_Salt.c_str(), m_Salt.length());
    } else {
      Logger::getInstance()->log("SessionManager: No salt specified, sessions ids might not be secure", lsWarning);
    }
    SHA256_Update(&ctx, m_VersionInfo.c_str(), m_VersionInfo.length());
    SHA256_Update(&ctx, &m_NextSessionID, sizeof(m_NextSessionID));
    m_NextSessionID++;
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_Final(digest, &ctx);
    std::ostringstream sstr;
    sstr << std::hex;
    sstr.fill('0');
    for(int iByte = 0; iByte < SHA256_DIGEST_LENGTH; iByte++) {
      sstr.width(2);
      sstr << static_cast<unsigned int>(digest[iByte] & 0x0ff);
    }
    return sstr.str();
  }

  boost::shared_ptr<Session>& SessionManager::getSession(const std::string& _id) {
    m_MapMutex.lock();
    boost::shared_ptr<Session>& rv = m_Sessions[_id];
    m_MapMutex.unlock();
    return rv;
  }

  void SessionManager::removeSession(const std::string& _id) {
    m_MapMutex.lock();
    boost::ptr_map<const std::string, boost::shared_ptr<Session> >::iterator i = m_Sessions.find(_id);
    if(i != m_Sessions.end()) {
      m_Sessions.erase(i);
    } else {
      Logger::getInstance()->log("Tried to remove nonexistent session!", lsWarning);
    }
    m_MapMutex.unlock();
  }

  void SessionManager::touchSession(const std::string& _id) {
    m_MapMutex.lock();
    boost::ptr_map<const std::string, boost::shared_ptr<Session> >::iterator i = m_Sessions.find(_id);
    if(i != m_Sessions.end()) {
      m_Sessions[_id]->touch();
    }
    m_MapMutex.unlock();
  }

  void SessionManager::cleanupSessions(Event& _event, const EventSubscription& _subscription) {
    m_MapMutex.lock();
    boost::ptr_map<const std::string, boost::shared_ptr<Session> >::iterator i;
    for (i = m_Sessions.begin(); i != m_Sessions.end(); i++) {
      if (i != m_Sessions.end()) {
        boost::shared_ptr<Session>& s = m_Sessions[i->first];
        if (s != NULL) {
          if (!s->isStillValid()) {
            m_Sessions.erase(i->first);
          }
        } else {
          // when the session, for some reason, is NULL, we can get rid of
          // the entry in the map anyway
          m_Sessions.erase(i->first);
        }
      }
    }

    m_MapMutex.unlock();
    this->sendCleanupEvent();
  }
}

