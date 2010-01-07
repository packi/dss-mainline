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

#include "eventrequesthandler.h"

#include "core/web/json.h"
#include "core/event.h"
#include "core/base.h"
#include "core/dss.h"

namespace dss {

  //=========================================== EventRequestHandler

  boost::shared_ptr<JSONObject> EventRequestHandler::jsonHandleRequest(const RestfulRequest& _request, Session* _session) {
    if(_request.getMethod() == "raise") {
      std::string name = _request.getParameter("name");
      std::string location = _request.getParameter("location");
      std::string context = _request.getParameter("context");
      std::string parameter = _request.getParameter("parameter");

      boost::shared_ptr<Event> evt(new Event(name));
      if(!context.empty()) {
        evt->setContext(context);
      }
      if(!location.empty()) {
        evt->setLocation(location);
      }
      std::vector<std::string> params = dss::splitString(parameter, ';');
      for(std::vector<std::string>::iterator iParam = params.begin(), e = params.end();
          iParam != e; ++iParam)
      {
        std::vector<std::string> nameValue = dss::splitString(*iParam, '=');
        if(nameValue.size() == 2) {
          dss::Logger::getInstance()->log("WebServer::handleEventCall: Got parameter '" + nameValue[0] + "'='" + nameValue[1] + "'");
          evt->setProperty(nameValue[0], nameValue[1]);
        } else {
          dss::Logger::getInstance()->log(std::string("Invalid parameter found WebServer::handleEventCall: ") + *iParam );
        }
      }

      getDSS().getEventQueue().pushEvent(evt);
      return success();
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest



} // namespace dss
