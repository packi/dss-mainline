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

#include "core/logger.h"
#include "core/dss.h"

namespace dss {

  class IDeviceInterface;
  
  class WebServerRequestHandler : public RestfulRequestHandler {
  protected:
    DSS& getDSS() {
      return *DSS::getInstance();
    }

    void log(const std::string& _line, aLogSeverity _severity = lsDebug) {
      Logger::getInstance()->log("RequestHandler: " + _line, _severity);
    }
  }; // WebServerRequestHandler

  class DeviceInterfaceRequestHandler : public WebServerRequestHandler {
  public:
    std::string handleDeviceInterfaceRequest(const RestfulRequest& _request, IDeviceInterface* _interface);

  protected:
    bool isDeviceInterfaceCall(const RestfulRequest& _request);
  };

  class ApartmentRequestHandler : public DeviceInterfaceRequestHandler {
  public:
    virtual std::string handleRequest(const RestfulRequest& _request, Session* _session);
  };

  class ZoneRequestHandler : public DeviceInterfaceRequestHandler {
  public:
    virtual std::string handleRequest(const RestfulRequest& _request, Session* _session);
  }; // ZoneRequestHandler

  class DeviceRequestHandler : public DeviceInterfaceRequestHandler {
  public:
    virtual std::string handleRequest(const RestfulRequest& _request, Session* _session);
  }; // DeviceRequestHandler

  class CircuitRequestHandler : public WebServerRequestHandler {
  public:
    virtual std::string handleRequest(const RestfulRequest& _request, Session* _session);
  }; // CircuitRequestHandler

  class SetRequestHandler : public DeviceInterfaceRequestHandler {
  public:
    virtual std::string handleRequest(const RestfulRequest& _request, Session* _session);
  }; // SetRequestHandler

  class PropertyRequestHandler : public WebServerRequestHandler {
  public:
    virtual std::string handleRequest(const RestfulRequest& _request, Session* _session);
  }; // PropertyRequestHandler

  class EventRequestHandler : public WebServerRequestHandler {
  public:
    virtual std::string handleRequest(const RestfulRequest& _request, Session* _session);
  }; // EventRequestHandler

  class SystemRequestHandler : public WebServerRequestHandler {
  public:
    virtual std::string handleRequest(const RestfulRequest& _request, Session* _session);
  }; // SystemRequestHandler

  class StructureRequestHandler : public WebServerRequestHandler {
  public:
    virtual std::string handleRequest(const RestfulRequest& _request, Session* _session);
  }; // StructureRequestHandler

  class SimRequestHandler : public WebServerRequestHandler {
  public:
    virtual std::string handleRequest(const RestfulRequest& _request, Session* _session);
  }; // SimRequestHandler

  class DebugRequestHandler : public WebServerRequestHandler {
  public:
    virtual std::string handleRequest(const RestfulRequest& _request, Session* _session);
  }; // DebugRequestHandler

  class MeteringRequestHandler : public WebServerRequestHandler {
  public:
    virtual std::string handleRequest(const RestfulRequest& _request, Session* _session);
  }; // MeteringRequestHandler


}

#endif // WEBREQUESTS_H
