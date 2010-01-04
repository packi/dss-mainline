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

#include <mongoose/mongoose.h>

#include "core/base.h"
#include "core/subsystem.h"
#include "core/session.h"

#include <string>

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace dss {

  class IDeviceInterface;
  class PropertyNode;
  class WebServerPlugin;
  class RestfulRequestHandler;

  typedef boost::ptr_map<const int, Session> SessionByID;

  class WebServer : public Subsystem {
  private:
    struct mg_context* m_mgContext;
    int m_LastSessionID;
    SessionByID m_Sessions;
    boost::ptr_vector<WebServerPlugin> m_Plugins;
    __gnu_cxx::hash_map<const std::string, RestfulRequestHandler*> m_Handlers;
  private:
    void setupAPI();
    void loadPlugin(PropertyNode& _node);
    void loadPlugins();
    void instantiateHandlers();
  protected:
    void pluginCalled(struct mg_connection* _connection,
                      const struct mg_request_info* _info,
                      WebServerPlugin& plugin, 
                      const std::string& _uri);

    static void httpPluginCallback(struct mg_connection* _connection,
                                   const struct mg_request_info* _info, 
                                   void* _userData);
    static void httpBrowseProperties(struct mg_connection* _connection,
                                   const struct mg_request_info* _info, 
                                   void* _userData);
    static void jsonHandler(struct mg_connection* _connection,
                            const struct mg_request_info* _info, 
                            void* _userData);
    static void downloadHandler(struct mg_connection* _connection,
                            const struct mg_request_info* _info, 
                            void* _userData);

      static void emitHTTPHeader(int _code, struct mg_connection* _connection, const std::string& _contentType = "text/html");
  protected:
    virtual void doStart();
  public:
    WebServer(DSS* _pDSS);
    ~WebServer();

    virtual void initialize();

  }; // WebServer

}

#endif /*WEBSERVER_H_*/
