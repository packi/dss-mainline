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

#ifndef WEBREQUESTS_H
#define WEBREQUESTS_H

#include "src/web/restful.h"

#include <string>
#include <boost/shared_ptr.hpp>

#include "src/logger.h"
#include "src/dss.h"

namespace dss {

  class JSONObject;
  class JSONElement;
  class RestfulRequest;
  class Session;
  
  class WebServerResponse {
  public:
    WebServerResponse(boost::shared_ptr<JSONObject> _response)
    : m_Response(_response)
    { }

    void setCookie(const std::string& _key, const std::string& _value) {
      m_Cookies[_key] = _value;
    }

    const HashMapStringString& getCookies() const {
      return m_Cookies;
    }

    boost::shared_ptr<JSONObject> getResponse() {
      return m_Response;
    }
  private:
    boost::shared_ptr<JSONObject> m_Response;
    HashMapStringString m_Cookies;
  }; // WebServerResponse

  class WebServerRequestHandlerJSON {
  protected:
    boost::shared_ptr<JSONObject> success();
    boost::shared_ptr<JSONObject> success(boost::shared_ptr<JSONElement> _innerResult);
    boost::shared_ptr<JSONObject> success(const std::string& _message);
    boost::shared_ptr<JSONObject> failure(const std::string& _message = "");
  public:
    virtual std::string handleRequest(const RestfulRequest& _request,
                                      boost::shared_ptr<Session> _session);
    virtual WebServerResponse jsonHandleRequest(const RestfulRequest& _request,
                                                boost::shared_ptr<Session> _session) = 0;
  protected:
    void log(const std::string& _line, aLogSeverity _severity = lsDebug) {
        Logger::getInstance()->log("RequestHandler: " + _line, _severity);
    }
  }; // WebServerRequestHandlerJSON



}

#endif // WEBREQUESTS_H
