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

#include <external/civetweb/civetweb.h>

#include "src/base.h"
#include "src/subsystem.h"

#define WEB_SESSION_TIMEOUT_MINUTES 3
#define WEB_SESSION_LIMIT 30
#define WEB_SOCKET_TIMEOUT_S 300 // 5min
namespace dss {

  class IDeviceInterface;
  class PropertyNode;
  class RestfulAPI;
  class RestfulRequest;
  class WebServerRequestHandlerJSON;
  class Session;
  class SessionManager;

  enum websocket_state {
      ws_disconnected = 0,
      ws_connected,
      ws_ready
  };

  typedef struct {
      struct mg_connection *connection;
      uint8_t state;
  } websocket_connection_t;


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
    size_t m_max_ws_clients;
    std::list<boost::shared_ptr<websocket_connection_t> > m_websockets;
    boost::mutex m_websocket_mutex;

  private:
    void setupAPI();
    void instantiateHandlers();
    void publishJSLogfiles();
  protected:
    int httpBrowseProperties(struct mg_connection* _connection,
                             RestfulRequest &request,
                             const std::string &trustedSetCookie);
    int jsonHandler(struct mg_connection* _connection,
                    RestfulRequest &request,
                    const std::string &trustedSetCookie,
                    boost::shared_ptr<Session> _session);
    int iconHandler(struct mg_connection* _connection,
                    RestfulRequest &request,
                    const std::string &trustedSetCookie,
                    boost::shared_ptr<Session> _session);
    int logDownloadHandler(struct mg_connection* _connection,
                           RestfulRequest &request,
                           const std::string &trustedSetCookie,
                           boost::shared_ptr<Session> _session);

    int WebSocketConnectHandler(const struct mg_connection* _connection,
                                void *cbdata);
    void WebSocketReadyHandler(struct mg_connection* _connection,
                              void *cbdata);
    void WebSocketCloseHandler(const struct mg_connection* _connection,
                                       void *cbdata);

    static int httpRequestCallback(struct mg_connection* _connection,
                                   void *cbdata);
    static int WebSocketConnectCallback(const struct mg_connection* _connection,
                                       void *cbdata);
    static void WebSocketReadyCallback(struct mg_connection* _connection,
                                     void *cbdata);
    static void WebSocketCloseCallback(const struct mg_connection* _connection,
                                       void *cbdata);

  protected:
    virtual void doStart();
  public:
    WebServer(DSS* _pDSS);
    ~WebServer();

    virtual void initialize();
    void setSessionManager(boost::shared_ptr<SessionManager> _pSessionManager);
    void sendToWebSockets(std::string data);
    size_t WebSocketClientCount();
  }; // WebServer

}

#endif /*WEBSERVER_H_*/
