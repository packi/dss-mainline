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

#include "webrequests.h"

#include <boost/shared_ptr.hpp>

#include "json.h"

namespace dss {


  //================================================== WebServerRequestHandlerJSON

  std::string WebServerRequestHandlerJSON::handleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    boost::shared_ptr<JSONObject> response = jsonHandleRequest(_request, _session);
    if(response != NULL) {
      return response->toString();
    }
    return "";
  } // handleRequest

  boost::shared_ptr<JSONObject> WebServerRequestHandlerJSON::success() {
    return success(boost::shared_ptr<JSONObject>());
  }

  boost::shared_ptr<JSONObject> WebServerRequestHandlerJSON::success(const std::string& _message) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("ok", true);
    result->addProperty("message", _message);
    return result;
  }

  boost::shared_ptr<JSONObject> WebServerRequestHandlerJSON::success(boost::shared_ptr<JSONElement> _innerResult) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("ok", true);
    if(_innerResult != NULL) {
      result->addElement("result", _innerResult);
    }
    return result;
  }

  boost::shared_ptr<JSONObject> WebServerRequestHandlerJSON::failure(const std::string& _message) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("ok", false);
    if(!_message.empty()) {
      result->addProperty("message", _message);
    }
    Logger::getInstance()->log("JSON call failed: '" + _message + "'");
    return result;
  }

} // namespace dss
