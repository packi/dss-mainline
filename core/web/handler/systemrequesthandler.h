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

#include "core/web/webrequests.h"

#ifndef SYSTEMREQUESTHANDLER_H_
#define SYSTEMREQUESTHANDLER_H_

namespace dss {

  class SystemRequestHandler : public WebServerRequestHandlerJSON {
  public:
    virtual boost::shared_ptr<JSONObject> jsonHandleRequest(const RestfulRequest& _request, Session* _session);
  }; // SystemRequestHandler

} // namespace dss

#endif /* SYSTEMREQUESTHANDLER_H_ */