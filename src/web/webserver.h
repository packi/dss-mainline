/*
    Copyright (c) 2009, 2010 digitalSTROM.org, Zurich, Switzerland

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

#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <string>
#include <boost/shared_ptr.hpp>

#include <external/mongoose/mongoose.h>

#include "src/base.h"
#include "src/subsystem.h"

#define WEB_SESSION_TIMEOUT_MINUTES 3
#define WEB_SESSION_LIMIT 30

namespace dss {

  class IDeviceInterface;
  class PropertyNode;
  class RestfulAPI;
  class RestfulRequest;
  class WebServerRequestHandlerJSON;
  class Session;
  class SessionManager;

  std::string extractToken(const char *_cookie);
  std::string extractAuthenticatedUser(const char *_header);
  std::string generateCookieString(const std::string &token);
  std::string generateRevokeCookieString();

  class WebServer : public Subsystem {
  private:
    struct mg_context* m_mgContext;
    int m_TrustedPort;
    HASH_MAP<std::string, WebServerRequestHandlerJSON*> m_Handlers;
    boost::shared_ptr<RestfulAPI> m_pAPI;
    boost::shared_ptr<SessionManager> m_SessionManager;
  private:
    void setupAPI();
    void instantiateHandlers();
    void publishJSLogfiles();
  protected:
    void *httpBrowseProperties(struct mg_connection* _connection,
                               RestfulRequest &request,
                               const std::string &trustedSetCookie);
    void *jsonHandler(struct mg_connection* _connection,
                      RestfulRequest &request,
                      const std::string &trustedSetCookie,
                      boost::shared_ptr<Session> _session);
    void *iconHandler(struct mg_connection* _connection,
                      RestfulRequest &request,
                      const std::string &trustedSetCookie,
                      boost::shared_ptr<Session> _session);
    static void *httpRequestCallback(enum mg_event event, 
                                     struct mg_connection* _connection,
                                     const struct mg_request_info* _info);

  protected:
    virtual void doStart();
  public:
    WebServer(DSS* _pDSS);
    ~WebServer();

    virtual void initialize();
    void setSessionManager(boost::shared_ptr<SessionManager> _pSessionManager);
  }; // WebServer

}

#endif /*WEBSERVER_H_*/
