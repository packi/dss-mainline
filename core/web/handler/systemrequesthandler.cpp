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

#include "systemrequesthandler.h"
#include "core/datetools.h"

#include "core/web/json.h"
#include "core/dss.h"

#include <sstream>

namespace dss {

  //=========================================== SystemRequestHandler

  boost::shared_ptr<JSONObject> SystemRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    if(_request.getMethod() == "version") {
      return success(DSS::getInstance()->versionString());
    } else if (_request.getMethod() == "time") {
      std::stringstream s;
      s << DateTime().secondsSinceEpoch();
      return success(s.str());
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest


} // namespace dss
