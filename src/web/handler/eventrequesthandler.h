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

#include <deque>
#include <map>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/thread/mutex.hpp>

#include "src/web/webrequests.h"
#include "src/session.h"

namespace dss {

  class Event;
  class EventInterpreter;
  class EventCollector;
  class EventSubscriptionSession;

  class EventRequestHandler : public WebServerRequestHandlerJSON {
  public:
    EventRequestHandler(EventInterpreter& _queue);
    virtual WebServerResponse jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
  private:
    EventInterpreter& m_EventInterpreter;
    boost::mutex m_Mutex; // locking what?

    int validateArgs(boost::shared_ptr<Session> _session, const std::string &name,
                     const std::string &tokenStr);
    std::string raise(const RestfulRequest& _request);
    std::string subscribe(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
    std::string unsubscribe(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
    std::string get(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
    std::string buildEventResponse(boost::shared_ptr<EventSubscriptionSession> _subscriptionSession);
  }; // StructureRequestHandler

} // namespace dss

#endif /* EVENTREQUESTHANDLER_H_ */
