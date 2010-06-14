/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#ifndef EVENTREQUESTHANDLER_H_
#define EVENTREQUESTHANDLER_H_

#include "core/web/webrequests.h"
#include "core/session.h"
#include "core/event.h"
#include "core/eventcollector.h"
#include "core/eventinterpreterplugins.h"
#include "core/mutex.h"

#include <deque>
#include <map>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace dss {

  class EventQueue;

  class EventSubscriptionSession : public Session {
  public:
      EventSubscriptionSession(boost::shared_ptr<Session> _parentSession);
      EventSubscriptionSession(const int _tokenID, boost::shared_ptr<Session> _parentSession);
//    virtual bool isStillValid();
    int getTokenID();
    void subscribe(const std::string& _eventName);
    void unsubscribe(const std::string& _eventName);
    // blocks if no events are available
    boost::shared_ptr<JSONObject> getEvents(const int _timeoutMS = 0);
  private:
    boost::shared_ptr<Session> m_parentSession;
    std::deque<Event> m_events;
    boost::shared_ptr<EventCollector> m_pEventCollector;
    void createCollector();
    // name, sid
    std::map <std::string, std::string> m_subscriptionMap;
  };

  typedef boost::ptr_map<const int, boost::shared_ptr<EventSubscriptionSession> > EventSubscriptionSessionByTokenID;

  class EventRequestHandler : public WebServerRequestHandlerJSON {
  public:
    EventRequestHandler(EventQueue& _queue);
    virtual boost::shared_ptr<JSONObject> jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
  private:
    EventQueue& m_Queue;
    Mutex m_eventsMutex;

    // make use of the dataMap in the new session object (cookie stuff)

    EventSubscriptionSessionByTokenID m_SessionByTokenID;

    boost::shared_ptr<JSONObject> raise(const RestfulRequest& _request);
    boost::shared_ptr<JSONObject> subscribe(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
    boost::shared_ptr<JSONObject> unsubscribe(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
    boost::shared_ptr<JSONObject> get(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
  }; // StructureRequestHandler

} // namespace dss

#endif /* EVENTREQUESTHANDLER_H_ */
