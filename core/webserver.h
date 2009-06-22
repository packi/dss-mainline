/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <shttpd/shttpd.h>

#include "base.h"
#include "thread.h"
#include "subsystem.h"
#include "session.h"

#include <string>

#include <boost/ptr_container/ptr_map.hpp>

namespace dss {


  class IDeviceInterface;

  typedef boost::ptr_map<const int, Session> SessionByID;

  class WebServer : public Subsystem,
                    private Thread {
  private:
    struct shttpd_ctx* m_SHttpdContext;
    int m_LastSessionID;
    SessionByID m_Sessions;
  private:
    virtual void execute();
  protected:
    bool isDeviceInterfaceCall(const std::string& _method);
    string callDeviceInterface(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, IDeviceInterface* _interface, Session* _session);

    static void jsonHandler(struct shttpd_arg* _arg);
    string handleApartmentCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleZoneCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleDeviceCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleSetCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handlePropertyCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleEventCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleCircuitCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleStructureCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleSimCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);
    string handleDebugCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session);

    static void httpBrowseProperties(struct shttpd_arg* _arg);
    static void emitHTTPHeader(int _code, struct shttpd_arg* _arg, const std::string& _contentType = "text/html");

  protected:
    virtual void doStart();
  public:
    WebServer(DSS* _pDSS);
    ~WebServer();

    virtual void initialize();

  }; // WebServer

}

#endif /*WEBSERVER_H_*/
