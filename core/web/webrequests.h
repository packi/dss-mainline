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

#include "core/web/restful.h"

#include <string>
#include <boost/shared_ptr.hpp>

#include "core/logger.h"
#include "core/dss.h"

namespace dss {

  class IDeviceInterface;
  class JSONObject;
  class JSONElement;
  
  class WebServerRequestHandler : public RestfulRequestHandler {
  protected:
    DSS& getDSS() {
      return *DSS::getInstance();
    }

    void log(const std::string& _line, aLogSeverity _severity = lsDebug) {
      Logger::getInstance()->log("RequestHandler: " + _line, _severity);
    }
  }; // WebServerRequestHandler

  class WebServerRequestHandlerJSON : public WebServerRequestHandler {
  protected:
    boost::shared_ptr<JSONObject> success();
    boost::shared_ptr<JSONObject> success(boost::shared_ptr<JSONElement> _innerResult);
    boost::shared_ptr<JSONObject> success(const std::string& _message);
    boost::shared_ptr<JSONObject> failure(const std::string& _message = "");
  public:
    virtual std::string handleRequest(const RestfulRequest& _request, Session* _session);
    virtual boost::shared_ptr<JSONObject> jsonHandleRequest(const RestfulRequest& _request, Session* _session) = 0;
  }; // WebServerRequestHandlerJSON

  class DeviceInterfaceRequestHandler : public WebServerRequestHandlerJSON {
  public:
    boost::shared_ptr<JSONObject> handleDeviceInterfaceRequest(const RestfulRequest& _request, IDeviceInterface* _interface);

  protected:
    bool isDeviceInterfaceCall(const RestfulRequest& _request);
  };

  class ApartmentRequestHandler : public DeviceInterfaceRequestHandler {
  public:
    virtual boost::shared_ptr<JSONObject> jsonHandleRequest(const RestfulRequest& _request, Session* _session);
  };

  class ZoneRequestHandler : public DeviceInterfaceRequestHandler {
  public:
    virtual boost::shared_ptr<JSONObject> jsonHandleRequest(const RestfulRequest& _request, Session* _session);
  }; // ZoneRequestHandler

  class DeviceRequestHandler : public DeviceInterfaceRequestHandler {
  public:
    virtual boost::shared_ptr<JSONObject> jsonHandleRequest(const RestfulRequest& _request, Session* _session);
  }; // DeviceRequestHandler

  class CircuitRequestHandler : public WebServerRequestHandlerJSON {
  public:
    virtual boost::shared_ptr<JSONObject> jsonHandleRequest(const RestfulRequest& _request, Session* _session);
  }; // CircuitRequestHandler

  class SetRequestHandler : public DeviceInterfaceRequestHandler {
  public:
    virtual boost::shared_ptr<JSONObject> jsonHandleRequest(const RestfulRequest& _request, Session* _session);
  }; // SetRequestHandler


}

#endif // WEBREQUESTS_H
