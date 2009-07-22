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

    Last change $Date: 2009-06-30 13:56:46 +0200 (Tue, 30 Jun 2009) $
    by $Author: pstaehlin $
*/


#include "webserverplugin.h"
#include "plugin/webserver_plugin.h"
#include "core/logger.h"

#include <cassert>

#include <dlfcn.h>

namespace dss {

  WebServerPlugin::WebServerPlugin(const std::string& _uri, const std::string& _file)
  : m_URI(_uri), m_File(_file), m_Handle(NULL), m_pHandleRequest(NULL)
  { } // ctor

  WebServerPlugin::~WebServerPlugin() {
    m_pHandleRequest = NULL;
    if(m_Handle != NULL) {
      dlclose(m_Handle);
    }
  } // dtor

  void WebServerPlugin::load() {
    assert(m_Handle == NULL);
    Logger::getInstance()->log("WebServerPlugin::load(): Trying to load \"" + m_File + "\"", lsInfo);
    if(!fileExists(m_File)) {
      throw std::runtime_error(std::string("Plugin '") + m_File + "' does not exist.");
    }
    m_Handle = dlopen(m_File.c_str(), RTLD_LAZY);
    if(m_Handle == NULL) {
      Logger::getInstance()->log("WebServerPlugin::load(): Could not load plugin \"" + m_File + "\" message: " + dlerror(), lsError);
      return;
    }

    dlerror();
    int (*version)();
    *(void**) (&version) = dlsym(m_Handle, "plugin_getversion");
    char* error;
    if((error = dlerror()) != NULL) {
      Logger::getInstance()->log("WebServerPlugin::load(): Could not get symbol 'plugin_getversion' from \"" + m_File + "\":" + error, lsError);
      return;
    }

    int ver = (*version)();
    if(ver != WEBSERVER_PLUGIN_API_VERSION) {
      Logger::getInstance()->log("WebServerPlugin::load(): Version mismatch (plugin: " + intToString(ver) + " api: " + intToString(WEBSERVER_PLUGIN_API_VERSION) + ")", lsError);
      return;
    }

    *(void**) (&m_pHandleRequest) = dlsym(m_Handle, "plugin_handlerequest");
    if((error = dlerror()) != NULL) {
       Logger::getInstance()->log("WebServerPlugin::load(): Could not get symbol 'plugin_handlerequest' from plugin: \"" + m_File + "\":" + error, lsError);
       return;
    }
  } // load

  bool WebServerPlugin::handleRequest(const std::string& _uri, HashMapConstStringString& _parameter, std::string& result) {
    if(m_Handle != NULL && m_pHandleRequest != NULL) {
      return (*m_pHandleRequest)(_uri, _parameter, result);
    }
    return false;
  } // handleRequest


} // namespace dss
