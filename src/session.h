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

#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

#include <string>
#include <map>
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "eventsubscriptionsession.h"
#include "datetools.h"

namespace dss {

  class User;

  class Session {
  public:
    Session(const std::string& _id);
    ~Session();

    void setTimeout(const int _timeout);
    const std::string& getID() const;
    virtual bool isStillValid();
    bool isUsed();
    void use();
    void unuse();
    void touch();
    void markAsApplicationSession();
    boost::shared_ptr<EventSubscriptionSession> getEventSubscription(int _token);
    boost::shared_ptr<EventSubscriptionSession>
      createEventSubscription(EventInterpreter &interp, int _token);
    void deleteEventSubscription(boost::shared_ptr<EventSubscriptionSession> subs);
    void inheritUserFromSecurity();
    User* getUser() { return m_pUser; }
    bool isApplicationSession() { return m_IsApplicationSession; }
    unsigned int getIdleTime() { return m_LastTouched.secondsSinceEpoch(); }
  protected:
    const std::string m_Token;
    boost::mutex m_UseCountMutex;
    int m_UsageCount;
    DateTime m_LastTouched;
    int m_SessionTimeoutSec;
    User* m_pUser;
    bool m_IsApplicationSession;

    typedef std::vector<boost::shared_ptr<EventSubscriptionSession> > subscriptions_t;
    std::vector<boost::shared_ptr<EventSubscriptionSession> > m_subscriptions;
  }; // Session


}

#endif // defined SESSION_H_INCLUDED
