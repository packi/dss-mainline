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

#include "src/web/webrequests.h"
#include "src/session.h"
#include "src/mutex.h"

#include <deque>
#include <map>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace dss {

  class Event;
  class EventInterpreter;
  class EventCollector;
  class EventSubscriptionSession;

  typedef std::map<const int, boost::shared_ptr<EventSubscriptionSession> > EventSubscriptionSessionByTokenID;

  class EventRequestHandler : public WebServerRequestHandlerJSON {
  public:
    EventRequestHandler(EventInterpreter& _queue);
    virtual WebServerResponse jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
  private:
    EventInterpreter& m_EventInterpreter;
    Mutex m_eventsMutex;

    // make use of the dataMap in the new session object (cookie stuff)

    EventSubscriptionSessionByTokenID m_SessionByTokenID;

    int validateArgs(boost::shared_ptr<Session> _session, const std::string &name,
                     const std::string &tokenStr);
    boost::shared_ptr<JSONObject> raise(const RestfulRequest& _request);
    boost::shared_ptr<JSONObject> subscribe(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
    boost::shared_ptr<JSONObject> unsubscribe(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
    boost::shared_ptr<JSONObject> get(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
    boost::shared_ptr<JSONObject> buildEventResponse(boost::shared_ptr<EventSubscriptionSession> _subscriptionSession);
    boost::shared_ptr<EventSubscriptionSession> getSubscriptionSession(int _token, boost::shared_ptr<Session> _session);
  }; // StructureRequestHandler

} // namespace dss

#endif /* EVENTREQUESTHANDLER_H_ */
