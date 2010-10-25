/*
    Copyright (c) 2009, 2010 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include <boost/filesystem.hpp>

#include "core/logger.h"
#include "core/dss.h"
#include "core/propertysystem.h"
#include "core/web/restful.h"
#include "core/web/restfulapiwriter.h"
#include "core/web/webrequests.h"

#include "core/web/handler/debugrequesthandler.h"
#include "core/web/handler/systemrequesthandler.h"
#include "core/web/handler/simrequesthandler.h"
#include "core/web/handler/meteringrequesthandler.h"
#include "core/web/handler/structurerequesthandler.h"
#include "core/web/handler/eventrequesthandler.h"
#include "core/web/handler/propertyrequesthandler.h"
#include "core/web/handler/apartmentrequesthandler.h"
#include "core/web/handler/circuitrequesthandler.h"
#include "core/web/handler/devicerequesthandler.h"
#include "core/web/handler/setrequesthandler.h"
#include "core/web/handler/zonerequesthandler.h"
#include "core/web/handler/subscriptionrequesthandler.h"

#include "core/businterface.h"

#include "webserverapi.h"
#include "json.h"

namespace fs = boost::filesystem;

namespace dss {
  //============================================= WebServer

  WebServer::WebServer(DSS* _pDSS)
    : Subsystem(_pDSS, "WebServer"), m_mgContext(0), m_LastSessionID(0)
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
    if (m_mgContext == NULL) {
      throw std::runtime_error("Could not start mongoose webserver!");
    }

    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "webroot", getDSS().getWebrootDirectory(), true, false);
    getDSS().getPropertySystem().setIntValue(getConfigPropertyBasePath() + "ports", 8080, true, false);
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "files/apartment.xml", getDSS().getDataDirectory() + "apartment.xml", true, false);
    getDSS().getPropertySystem().setIntValue(getConfigPropertyBasePath() + "sessionTimeoutMinutes", WEB_SESSION_TIMEOUT_MINUTES, true, false);
    m_SessionManager = boost::shared_ptr<SessionManager>(new SessionManager(getDSS().getEventQueue(), getDSS().getEventInterpreter(), getDSS().getPropertySystem().getIntValue(getConfigPropertyBasePath() + "sessionTimeoutMinutes")*60));

    publishJSLogfiles();
    setupAPI();
    instantiateHandlers();
  } // initialize

  void WebServer::publishJSLogfiles() {
    // pre-create the property as the gui accesses it on startup and gets upset if the node's not there
    getDSS().getPropertySystem().createProperty("/system/js/logsfiles/");

    std::string logDir = getDSS().getJSLogDirectory();
    try {
      fs::directory_iterator end_iter;
      for(fs::directory_iterator dir_itr(logDir);
          dir_itr != end_iter;
          ++dir_itr)  {
        if(fs::is_regular(dir_itr->status())) {
          std::string fileName = dir_itr->filename();
          if(endsWith(dir_itr->filename(), ".log") ||
             endsWith(dir_itr->filename(), ".LOG")) {
            std::string abspath = dir_itr->string();
            getDSS().getPropertySystem().setStringValue("/config/subsystems/WebServer/files/" + std::string(fileName), abspath, true, false);
            getDSS().getPropertySystem().setStringValue("/system/js/logsfiles/" + std::string(fileName), abspath, true, false);
          }
        }
      }
    } catch(std::exception& e) {
      throw std::runtime_error("Directory for js logfiles does not exist: '" + logDir + "'");
    }
  } // publishJSLogfiles

  void WebServer::setupAPI() {
    m_pAPI = WebServerAPI::createRestfulAPI();
    RestfulAPIWriter::writeToXML(*m_pAPI, "doc/json_api.xml");
  } // setupAPI

  void WebServer::doStart() {
    std::string ports = intToString(DSS::getInstance()->getPropertySystem().getIntValue(getConfigPropertyBasePath() + "ports"));
    log("Webserver: Listening on port(s) " + ports);
    mg_set_option(m_mgContext, "ports", ports.c_str());

    std::string aliases = std::string("/=") + DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "webroot");
    log("Webserver: Configured aliases: " + aliases);
    mg_set_option(m_mgContext, "aliases", aliases.c_str());

    mg_set_callback(m_mgContext, MG_EVENT_NEW_REQUEST, &httpRequestCallback);

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

  void WebServer::emitHTTPHeader(int _code, struct mg_connection* _connection, const std::string& _contentType, const std::string& _setCookie) {
    std::ostringstream sstream;
    sstream << "HTTP/1.1 " << _code << ' ' << httpCodeToMessage(_code) << "\r\n";
    sstream << "Content-Type: " << _contentType << "; charset=utf-8\r\n";
    if(!_setCookie.empty()) {
      sstream << "Set-Cookie: " << _setCookie << "\r\n";
    }
    sstream << "\r\n";
    std::string tmp = sstream.str();
    mg_write(_connection, tmp.c_str(), tmp.length());
  } // emitHTTPHeader

  HashMapConstStringString parseParameter(const char* _params) {
    HashMapConstStringString result;
    if(_params != NULL) {
      std::vector<std::string> paramList = splitString(_params, '&');
      for(std::vector<std::string>::iterator iParam = paramList.begin(); iParam != paramList.end(); ++iParam) {
        std::string key;
        std::string value;
        boost::tie(key, value) = splitIntoKeyValue(*iParam);
        if(key.empty() || value.empty()) {
          result[*iParam] = "";
        } else {
          result[urlDecode(key)] = urlDecode(value);
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
  const char* kHandlerSubscription = "subscription";

  void WebServer::instantiateHandlers() {
    m_Handlers[kHandlerApartment] = new ApartmentRequestHandler(getDSS().getApartment(), getDSS().getModelMaintenance());
    m_Handlers[kHandlerZone] = new ZoneRequestHandler(getDSS().getApartment());
    m_Handlers[kHandlerDevice] = new DeviceRequestHandler(getDSS().getApartment());
    m_Handlers[kHandlerCircuit] = new CircuitRequestHandler(
            getDSS().getApartment(),
            getDSS().getModelMaintenance());
    m_Handlers[kHandlerSet] = new SetRequestHandler(getDSS().getApartment());
    m_Handlers[kHandlerProperty] = new PropertyRequestHandler(getDSS().getPropertySystem());
    m_Handlers[kHandlerEvent] = new EventRequestHandler(getDSS().getEventInterpreter());
    m_Handlers[kHandlerSystem] = new SystemRequestHandler();
    m_Handlers[kHandlerStructure] =
      new StructureRequestHandler(
        getDSS().getApartment(),
        getDSS().getModelMaintenance(),
        *getDSS().getBusInterface().getStructureModifyingBusInterface()
      );
    m_Handlers[kHandlerSim] = new SimRequestHandler(getDSS().getApartment());
    m_Handlers[kHandlerDebug] = new DebugRequestHandler(getDSS());
    m_Handlers[kHandlerMetering] = new MeteringRequestHandler(getDSS().getApartment(), getDSS().getMetering());
    m_Handlers[kHandlerSubscription] = new SubscriptionRequestHandler(getDSS().getEventInterpreter());
  } // instantiateHandlers

  boost::ptr_map<std::string, std::string> WebServer::parseCookies(const char *_cookies) {
      boost::ptr_map<std::string, std::string> result;

      if ((_cookies == NULL) || (strlen(_cookies) == 0)) {
        return result;
      }

      std::string c = _cookies;
      std::string pairStr;
      size_t semi = std::string::npos;

      do {
        semi = c.find(';');
        if (semi != std::string::npos) {
          pairStr = c.substr(0, semi);
          c = c.substr(semi+1);
        } else {
          pairStr = c;
        }

        size_t eq = pairStr.find('=');
        if (eq == std::string::npos) {
          continue;
        }

        std::string key = pairStr.substr(0, eq);
        std::string value = pairStr.substr(eq+1);

        if (key.empty()) {
          continue;
        }

        result[key] = value;


      } while (semi != std::string::npos);


      return result;
  }

  std::string WebServer::generateCookieString(boost::ptr_map<std::string, std::string> _cookies) {
    std::string result = "";

    boost::ptr_map<std::string, std::string>::iterator i;
    std::string path;

    for (i = _cookies.begin(); i != _cookies.end(); i++) {
      if (i->first == "path") {
        path = *i->second;
        continue;
      }

      if (!result.empty()) {
        result = result + "; ";
      }

      result = result + i->first + "=" + (*i->second);
    }

    if (!path.empty()) {
      if (!result.empty()) {
        result = result + "; ";
      }

      result = result + "path=" + path;
    }

    return result;
  }

  mg_error_t WebServer::jsonHandler(struct mg_connection* _connection,
                              const struct mg_request_info* _info) {
    const std::string urlid = "/json/";
    const char  *cookie;
    std::string setCookie;
    int token = -1;
    boost::shared_ptr<Session> session;

    std::string uri = _info->uri;
    HashMapConstStringString paramMap = parseParameter(_info->query_string);

    std::string method = uri.substr(uri.find(urlid) + urlid.size());

    RestfulRequest request(method, paramMap);

    WebServer& self = DSS::getInstance()->getWebServer();
    self.log("Processing call to " + method);

    cookie = mg_get_header(_connection, "Cookie");

    if (cookie != NULL) {
      boost::ptr_map<std::string, std::string> cookies = self.parseCookies(cookie);
      std::string& tokenStr = cookies["token"];
      token = strToIntDef(tokenStr, -1);
      if (token != -1) {
        session = self.m_SessionManager->getSession(token);
      }
    }

    if ((cookie == NULL) || (token == -1) || (session == NULL)){

      token = self.m_SessionManager->registerSession();
      session = self.m_SessionManager->getSession(token);
      self.log("Registered new JSON session");

      boost::ptr_map<std::string, std::string> cmap;
      cmap["token"] = intToString(token);
      cmap["path"] = "/";
      setCookie = self.generateCookieString(cmap);
    }

    session->touch();

    std::string result;
    if(self.m_Handlers[request.getClass()] != NULL) {
      try {
        result = self.m_Handlers[request.getClass()]->handleRequest(request, session);
        emitHTTPHeader(200, _connection, "application/json", setCookie);
      } catch(std::runtime_error& e) {
        emitHTTPHeader(500, _connection, "application/json", setCookie);
        JSONObject resultObj;
        resultObj.addProperty("ok", false);
        resultObj.addProperty("message", e.what());
        result = resultObj.toString();
      }
    } else {
      emitHTTPHeader(404, _connection, "application/json", setCookie);
      std::ostringstream sstream;
      sstream << "{" << "\"ok\"" << ":" << "false" << ",";
      sstream << "\"message\"" << ":" << "\"Call to unknown function\"";
      sstream << "}";
      result = sstream.str();
      self.log("Unknown function '" + method + "'", lsError);
    }
    mg_write(_connection, result.c_str(), result.length());
    return MG_SUCCESS;
  } // jsonHandler

  mg_error_t WebServer::downloadHandler(struct mg_connection* _connection,
                                  const struct mg_request_info* _info) {
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
    return MG_SUCCESS;
  } // downloadHandler

  mg_error_t WebServer::httpBrowseProperties(struct mg_connection* _connection,
                                       const struct mg_request_info* _info) {
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
    return MG_SUCCESS;
  } // httpBrowseProperties

  mg_error_t WebServer::httpRequestCallback(struct mg_connection* _connection,
                                      const struct mg_request_info* _info) {

    std::string uri = _info->uri;

    if (uri.find("/browse/") == 0) {
      return httpBrowseProperties(_connection, _info);
    } else if (uri.find("/json/") == 0) {
      return jsonHandler(_connection, _info);
    } else if (uri.find("/download/") == 0) {
      return downloadHandler(_connection, _info);
    }

    return MG_NOT_FOUND;
  }
}
