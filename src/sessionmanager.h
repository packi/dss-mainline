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

#ifndef __SESSION_MANAGER_H__
#define __SESSION_MANAGER_H__

#include "src/mutex.h"

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_map.hpp>

#include "logger.h"

namespace dss {

  const int kSessionCleanupInterval = 30;

  class EventQueue;
  class EventInterpreter;
  class InternalEventRelayTarget;
  class Session;
  class Security;
  class Event;
  class EventSubscription;

  class SessionTokenGenerator {
    __DECL_LOG_CHANNEL__
  public:
    static std::string generate();
  private:
    std::string _generate();
    SessionTokenGenerator();
    std::string m_Salt;
    std::string m_VersionInfo;
    unsigned int m_NextSessionID;
  };

  class SessionManager {
  public:
    /// \param Specifies how long a session can remain idle before it gets
    /// removed, value of 0 means - never remove.
    SessionManager(EventQueue& _EventQueue, EventInterpreter& _eventInterpreter,
                   boost::shared_ptr<Security> _pSecurity);
    /// \brief Registers a new session, creates a new Session object
    /// \return session identifier
    std::string registerSession();

    std::string registerApplicationSession();

    /// \brief Returns the session object with the given id
    boost::shared_ptr<Session> getSession(const std::string& _id);

    /// \brief Removes session with the given id
    void removeSession(const std::string& _id);

    /// \brief Updates idle status of a session
    void touchSession(const std::string& _id);

    void cleanupSessions(Event& _event, const EventSubscription& _subscription);

    void setTimeout(const int _timeoutSeconds) {
      m_timeoutSecs = _timeoutSeconds;
    }

    void setMaxSessionCount(const int _max) {
      m_maxSessionCount = _max;
    }

    boost::shared_ptr<Security> getSecurity() {
      return m_pSecurity;
    }
  private:
    boost::shared_ptr<Session> createSession();
    void setupCleanupEventRelayTarget();
  private:
    EventQueue& m_EventQueue;
    EventInterpreter& m_EventInterpreter;
    boost::shared_ptr<Security> m_pSecurity;
    int m_timeoutSecs;
    unsigned int m_maxSessionCount;
    std::string m_VersionInfo;

    std::map<const std::string, boost::shared_ptr<Session> > m_Sessions;
    Mutex m_MapMutex;

    boost::shared_ptr<InternalEventRelayTarget> m_pRelayTarget;
    void sendCleanupEvent();
    void cleanupOldestSession();
  };
}

#endif//__SESSION_MANAGER_H__

