/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#include "webserver.h"

#include <iostream>
#include <sstream>

#include "core/logger.h"
#include "core/dss.h"
#include "core/propertysystem.h"
#include "core/web/restful.h"
#include "core/web/restfulapiwriter.h"
#include "core/web/webrequests.h"
#include "core/web/webserverplugin.h"

#include "webserverapi.h"

namespace dss {
  //============================================= WebServer

  WebServer::WebServer(DSS* _pDSS)
    : Subsystem(_pDSS, "WebServer"), m_mgContext(0)
  {
  } // ctor

  WebServer::~WebServer() {
    if (m_mgContext) {
      mg_stop(m_mgContext);
    }
  } // dtor

  void WebServer::initialize() {
    Subsystem::initialize();

    log("Starting Webserver...");
    m_mgContext = mg_start();

    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "webroot", getDSS().getWebrootDirectory(), true, false);
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "ports", "8080", true, false);

    setupAPI();
    instantiateHandlers();
  } // initialize

  void WebServer::loadPlugins() {
    PropertyNodePtr pluginsNode = getDSS().getPropertySystem().getProperty(getConfigPropertyBasePath() + "plugins");
    if(pluginsNode != NULL) {
      log("Found plugins node, trying to loading plugins", lsInfo);
      pluginsNode->foreachChildOf(*this, &WebServer::loadPlugin);
    }
  } // loadPlugins

  void WebServer::loadPlugin(PropertyNode& _node) {
    PropertyNodePtr pFileNode = _node.getProperty("file");
    PropertyNodePtr pURINode = _node.getProperty("uri");

    if(pFileNode == NULL) {
      log("loadPlugin: Missing subnode name 'file' on node " + _node.getDisplayName(), lsError);
      return;
    }
    if(pURINode == NULL) {
      log("loadPlugin: Missing subnode 'uri on node " + _node.getDisplayName(), lsError);
    }
    WebServerPlugin* plugin = new WebServerPlugin(pURINode->getStringValue(), pFileNode->getStringValue());
    try {
      plugin->load();
    } catch(std::runtime_error& e) {
      delete plugin;
      plugin = NULL;
      log(std::string("Caught exception while loading: ") + e.what(), lsError);
      return;
    }
    m_Plugins.push_back(plugin);

    log("Registering " + pFileNode->getStringValue() + " for URI '" + pURINode->getStringValue() + "'");
    mg_set_uri_callback(m_mgContext, pURINode->getStringValue().c_str(), &httpPluginCallback, plugin);
  } // loadPlugin

  void WebServer::setupAPI() {
    RestfulAPI api = WebServerAPI::createRestfulAPI();
    RestfulAPIWriter::writeToXML(api, "doc/json_api.xml");
  } // setupAPI

  void WebServer::doStart() {
    std::string ports = DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "ports");
    log("Webserver: Listening on port(s) " + ports);
    mg_set_option(m_mgContext, "ports", ports.c_str());

    std::string aliases = std::string("/=") + DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "webroot");
    log("Webserver: Configured aliases: " + aliases);
    mg_set_option(m_mgContext, "aliases", aliases.c_str());

    mg_set_uri_callback(m_mgContext, "/browse/*", &httpBrowseProperties, NULL);
    mg_set_uri_callback(m_mgContext, "/json/*", &jsonHandler, NULL);
    mg_set_uri_callback(m_mgContext, "/download/*", &downloadHandler, NULL);

    loadPlugins();

    log("Webserver started", lsInfo);
  } // start

  const char* httpCodeToMessage(const int _code) {
    if(_code == 400) {
      return "Bad Request";
    } else if(_code == 401) {
      return "Unauthorized\r\nWWW-Authenticate: Basic realm=\"dSS\"";
    } else if(_code == 403) {
      return "Forbidden";
    } else if(_code == 500) {
      return "Internal Server Error";
    } else {
      return "OK";
    }
  }

  void WebServer::emitHTTPHeader(int _code, struct mg_connection* _connection, const std::string& _contentType) {
    std::ostringstream sstream;
    sstream << "HTTP/1.1 " << _code << ' ' << httpCodeToMessage(_code) << "\r\n";
    sstream << "Content-Type: " << _contentType << "; charset=utf-8\r\n\r\n";
    std::string tmp = sstream.str();
    mg_write(_connection, tmp.c_str(), tmp.length());
  } // emitHTTPHeader

  HashMapConstStringString parseParameter(const char* _params) {
    HashMapConstStringString result;
    if(_params != NULL) {
      std::vector<std::string> paramList = splitString(_params, '&');
      for(std::vector<std::string>::iterator iParam = paramList.begin(); iParam != paramList.end(); ++iParam) {
        std::vector<std::string> nameValue = splitString(*iParam, '=');
        if(nameValue.size() != 2) {
          result[*iParam] = "";
        } else {
          result[urlDecode(nameValue.at(0))] = urlDecode(nameValue.at(1));
        }
      }
    }
    return result;
  } // parseParameter

  const char* kHandlerApartment = "apartment";
  const char* kHandlerZone = "zone";
  const char* kHandlerDevice = "device";
  const char* kHandlerCircuit = "circuit";
  const char* kHandlerSet = "set";
  const char* kHandlerProperty = "property";
  const char* kHandlerEvent = "event";
  const char* kHandlerSystem = "system";
  const char* kHandlerStructure = "structure";
  const char* kHandlerSim = "sim";
  const char* kHandlerDebug = "debug";
  const char* kHandlerMetering = "metering";

  void WebServer::instantiateHandlers() {
    m_Handlers[kHandlerApartment] = new ApartmentRequestHandler();
    m_Handlers[kHandlerZone] = new ZoneRequestHandler();
    m_Handlers[kHandlerDevice] = new DeviceRequestHandler();
    m_Handlers[kHandlerCircuit] = new CircuitRequestHandler();
    m_Handlers[kHandlerSet] = new SetRequestHandler();
    m_Handlers[kHandlerProperty] = new PropertyRequestHandler();
    m_Handlers[kHandlerEvent] = new EventRequestHandler();
    m_Handlers[kHandlerSystem] = new SystemRequestHandler();
    m_Handlers[kHandlerStructure] = new StructureRequestHandler();
    m_Handlers[kHandlerSim] = new SimRequestHandler();
    m_Handlers[kHandlerDebug] = new DebugRequestHandler();
    m_Handlers[kHandlerMetering] = new MeteringRequestHandler();
  } // instantiateHandlers
  
  void WebServer::httpPluginCallback(struct mg_connection* _connection,
                                     const struct mg_request_info* _info,
                                     void* _userData) {
    if(_userData != NULL) {
      WebServerPlugin* plugin = static_cast<WebServerPlugin*>(_userData);
      WebServer& self = DSS::getInstance()->getWebServer();

      std::string uri = _info->uri;
      self.log("Plugin: Processing call to " + uri);

      self.pluginCalled(_connection, _info, *plugin, uri);
    }
  } // httpPluginCallback

  void WebServer::pluginCalled(struct mg_connection* _connection,
                               const struct mg_request_info* _info,
                               WebServerPlugin& plugin,
                               const std::string& _uri) {
    HashMapConstStringString paramMap = parseParameter(_info->query_string);

    std::string result;
    if(plugin.handleRequest(_uri, paramMap, getDSS(), result)) {
      emitHTTPHeader(200, _connection, "text/plain");
      mg_write(_connection, result.c_str(), result.length());
    } else {
      emitHTTPHeader(500, _connection, "text/plain");
      mg_printf(_connection, "error");
    }
  } // pluginCalled

  void WebServer::jsonHandler(struct mg_connection* _connection,
                              const struct mg_request_info* _info,
                              void* _userData) {
    const std::string urlid = "/json/";
    std::string uri = _info->uri;

    HashMapConstStringString paramMap = parseParameter(_info->query_string);

    std::string method = uri.substr(uri.find(urlid) + urlid.size());

    RestfulRequest request(method, paramMap);

    WebServer& self = DSS::getInstance()->getWebServer();
    self.log("Processing call to " + method);

    Session* session = NULL;
    std::string tokenStr = paramMap["token"];
    if(!tokenStr.empty()) {
      int token = strToIntDef(tokenStr, -1);
      if(token != -1) {
        SessionByID::iterator iEntry = self.m_Sessions.find(token);
        if(iEntry != self.m_Sessions.end()) {
          if(iEntry->second->isStillValid()) {
            Session& s = *iEntry->second;
            session = &s;
          }
        }
      }
    }

    std::string result;
    if(self.m_Handlers[request.getClass()] != NULL) {
      result = self.m_Handlers[request.getClass()]->handleRequest(request, session);
      emitHTTPHeader(200, _connection, "application/json");
    } else {
      emitHTTPHeader(404, _connection, "application/json");
      std::ostringstream sstream;
      sstream << "{" << "\"ok\"" << ":" << "false" << ",";
      sstream << "\"message\"" << ":" << "\"Call to unknown function\"";
      sstream << "}";
      result = sstream.str();
      self.log("Unknown function '" + method + "'", lsError);
    }
    mg_write(_connection, result.c_str(), result.length());
  } // jsonHandler

  void WebServer::downloadHandler(struct mg_connection* _connection,
                                  const struct mg_request_info* _info, 
                                  void* _userData) {
    const std::string kURLID = "/download/";
    std::string uri = _info->uri;

    std::string givenFileName = uri.substr(uri.find(kURLID) + kURLID.size());

    WebServer& self = DSS::getInstance()->getWebServer();
    self.log("Processing call to download/" + givenFileName);

    // TODO: make the files-node readonly as this might pose a security threat
    //       (you could download any file on the disk if you add it as a subnode
    //        of files)
    PropertyNodePtr filesNode = self.getDSS().getPropertySystem().getProperty(
                                    self.getConfigPropertyBasePath() + "files"
                                );
    std::string fileName;
    if(filesNode != NULL) {
      PropertyNodePtr fileNode = filesNode->getProperty(givenFileName);
      if(fileNode != NULL) {
        fileName = fileNode->getStringValue();
      }
    }
    self.log("Using local file: " + fileName);
    struct mgstat st;
    if(mg_stat(fileName.c_str(), &st) != 0) {
      self.log("Not found");
      memset(&st, '\0', sizeof(st));
    }
    mg_send_file(_connection, fileName.c_str(), &st);
  } // downloadHandler

  void WebServer::httpBrowseProperties(struct mg_connection* _connection,
                                       const struct mg_request_info* _info,
                                       void* _userData) {
    emitHTTPHeader(200, _connection);

    const std::string urlid = "/browse";
    std::string uri = _info->uri;
    HashMapConstStringString paramMap = parseParameter(_info->query_string);

    std::string path = uri.substr(uri.find(urlid) + urlid.size());
    if(path.empty()) {
      path = "/";
    }
    mg_printf(_connection, "<html><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><body>");

    std::ostringstream stream;
    stream << "<h1>" << path << "</h1>";
    stream << "<ul>";
    PropertyNodePtr node = DSS::getInstance()->getPropertySystem().getProperty(path);
    if(node != NULL) {
      for(int iNode = 0; iNode < node->getChildCount(); iNode++) {
        PropertyNodePtr cnode = node->getChild(iNode);

        stream << "<li>";
        if(cnode->getChildCount() != 0) {
          stream << "<a href=\"/browse" << path << "/" << cnode->getDisplayName() << "\">" << cnode->getDisplayName() << "</a>";
        } else {
          stream << cnode->getDisplayName();
        }
        stream << " : ";
        stream << getValueTypeAsString(cnode->getValueType()) << " : ";
        switch(cnode->getValueType()) {
        case vTypeBoolean:
          stream << cnode->getBoolValue();
          break;
        case vTypeInteger:
          stream << cnode->getIntegerValue();
          break;
        case vTypeNone:
          break;
        case vTypeString:
          stream << cnode->getStringValue();
          break;
        default:
          stream << "unknown value";
        }
      }
    }

    stream << "</ul></body></html>";
    std::string tmp = stream.str();
    mg_write(_connection, tmp.c_str(), tmp.length());
  } // httpBrowseProperties

}