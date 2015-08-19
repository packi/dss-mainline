/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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

#ifndef SUBSCRIPTIONREQUESTHANDLER_H_
#define SUBSCRIPTIONREQUESTHANDLER_H_

#include "src/web/webrequests.h"

namespace dss {

  class EventInterpreter;

  class SubscriptionRequestHandler : public WebServerRequestHandlerJSON {
  public:
    SubscriptionRequestHandler(EventInterpreter& _interpreter);
    virtual WebServerResponse jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session, const struct mg_connection* _connection);
  private:
    std::string list(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
    std::string remove(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
    std::string add(const RestfulRequest& _request, boost::shared_ptr<Session> _session);
  private:
    EventInterpreter& m_EventInterpreter;
  }; // StructureRequestHandler

} // namespace dss

#endif /* SUBSCRIPTIONREQUESTHANDLER_H_ */
