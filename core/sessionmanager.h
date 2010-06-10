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

#ifndef __SESSION_MANAGER_H__
#define __SESSION_MANAGER_H__

#include "core/mutex.h"
#include "core/event.h"
#include "core/session.h"

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_map.hpp>

namespace dss {
  class SessionManager {
  public:
    /// \param Specifies how long a session can remain idle before it gets
    /// removed, value of 0 means - never remove.
    SessionManager(EventQueue& _EventQueue, EventInterpreter& _eventInterpreter, const int _timeout = 0);
    /// \brief Registers a new session, creates a new Session object
    /// \return session identifier
    int registerSession();

    /// \brief Returns the session object with the given id
    boost::shared_ptr<Session>& getSession(const int _id);

    /// \brief Removes session with the given id
    void removeSession(const int _id);

    /// \brief Updates idle status of a session
    void touchSession(const int _id);

    void cleanupSessions(Event& _event, const EventSubscription& _subscription);

  protected:
    int m_NextSessionID;
    EventQueue& m_EventQueue;
    EventInterpreter& m_EventInterpreter;
    int m_timeout;

    boost::ptr_map<const int, boost::shared_ptr<Session> > m_Sessions;
    Mutex m_MapMutex;

    bool m_eventRunning;
  };
}

#endif//__SESSION_MANAGER_H__

