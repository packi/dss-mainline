/*
    Copyright (c) 2009,2010 digitalSTROM.org, Zurich, Switzerland

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

#include "session.h"

#include "src/security/security.h"
#include "src/security/user.h"

namespace dss {

  Session::Session(const std::string& _tokenID)
  : m_Token(_tokenID),
    m_pUser(NULL),
    m_IsApplicationSession(false)
  {
    m_LastTouched = DateTime();
    m_UsageCount = 0;
    m_SessionTimeoutSec = 30;
  } // ctor

  Session::~Session() {
    delete m_pUser;
    m_pUser = NULL;
  }

  void Session::setTimeout(const int _timeout) {
    m_SessionTimeoutSec = _timeout;
  }

  bool Session::isStillValid() {
    return isUsed() || m_LastTouched.addSeconds(m_SessionTimeoutSec).after(DateTime());
  } // isStillValid

  bool Session::isUsed() {
    m_UseCountMutex.lock();
    bool result = (m_UsageCount != 0);
    m_UseCountMutex.unlock();
    return result;
  }

  void Session::use() {
    m_UseCountMutex.lock();
    m_UsageCount++;
    m_UseCountMutex.unlock();
  }

  void Session::unuse() {
    m_UseCountMutex.lock();
    m_UsageCount--;
    if(m_UsageCount < 0) {
      m_UsageCount = 0;
    }
    m_UseCountMutex.unlock();
    touch();
  }

  void Session::markAsApplicationSession() {
    m_IsApplicationSession = true;
  }

  const std::string& Session::getID() const {
    return m_Token;
  }

  void Session::touch() {
    m_LastTouched = DateTime();
  }

  void Session::inheritUserFromSecurity() {
    if(m_pUser != NULL) {
      delete m_pUser;
      m_pUser = NULL;
    }
    User* loggedInUser = Security::getCurrentlyLoggedInUser();
    if(loggedInUser != NULL) {
      m_pUser = new User(*loggedInUser);
    }
  }

  /**
   * getEventSubscription
   * @subscription_id
   * @ret subscription std::runtime_error if not found
   */
  boost::shared_ptr<EventSubscriptionSession>
    Session::getEventSubscription(int _token)
  {
    subscriptions_t::iterator item = std::find_if(m_subscriptions.begin(), m_subscriptions.end(),
                                                  EventSubscriptionSessionSelectById(_token));
    if (item == m_subscriptions.end()) {
        throw std::runtime_error(" Token " + intToString(_token) + " not found!");
    }
    return *item;
  }

  boost::shared_ptr<EventSubscriptionSession>
    Session::createEventSubscription(EventInterpreter &interp, int _token)
  {
    subscriptions_t::iterator item = std::find_if(m_subscriptions.begin(), m_subscriptions.end(),
                                                  EventSubscriptionSessionSelectById(_token));
    if (item != m_subscriptions.end()) {
      return *item;
    } else {
      boost::shared_ptr<EventSubscriptionSession> elt;
      elt.reset(new EventSubscriptionSession(interp, _token));
      m_subscriptions.push_back(elt);
      return elt;
    }
  }

  void Session::deleteEventSubscription(boost::shared_ptr<EventSubscriptionSession> subs) {
    m_subscriptions.erase(std::remove(m_subscriptions.begin(),
                                      m_subscriptions.end(), subs),
                          m_subscriptions.end());
  }
} // namespace dss
