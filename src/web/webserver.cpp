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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "webserver.h"

#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include <boost/filesystem.hpp>
#include <boost/tuple/tuple.hpp>

#include "src/logger.h"
#include "src/dss.h"
#include "src/propertysystem.h"
#include "src/foreach.h"

#include "src/security/security.h"

#include "src/web/restful.h"
#include "src/web/restfulapiwriter.h"
#include "src/web/webrequests.h"

#include "src/web/handler/systemrequesthandler.h"
#include "src/web/handler/simrequesthandler.h"
#include "src/web/handler/meteringrequesthandler.h"
#include "src/web/handler/structurerequesthandler.h"
#include "src/web/handler/eventrequesthandler.h"
#include "src/web/handler/propertyrequesthandler.h"
#include "src/web/handler/apartmentrequesthandler.h"
#include "src/web/handler/circuitrequesthandler.h"
#include "src/web/handler/devicerequesthandler.h"
#include "src/web/handler/setrequesthandler.h"
#include "src/web/handler/zonerequesthandler.h"
#include "src/web/handler/subscriptionrequesthandler.h"

#include "src/businterface.h"

#include "webserverapi.h"
#include "json.h"

namespace fs = boost::filesystem;

namespace dss {

  //============================================= WebServer

  WebServer::WebServer(DSS* _pDSS)
    : Subsystem(_pDSS, "WebServer"), m_mgContext(0), m_LastSessionID(0),
      m_TrustedPort(0)
  {
  } // ctor

  WebServer::~WebServer() {
    if (m_mgContext) {
      mg_stop(m_mgContext);
    }
  } // dtor

  void WebServer::initialize() {
    Subsystem::initialize();

    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "webroot", getDSS().getWebrootDirectory(), true, false);
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "bindip", "0.0.0.0", true, false);
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "listen", "8080", true, false);
    getDSS().getPropertySystem().setIntValue(getConfigPropertyBasePath() + "trustedPort", 0, true, false);
    getDSS().getPropertySystem().setIntValue(getConfigPropertyBasePath() + "announcedport", 8080, true, false);
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "files/apartment.xml", getDSS().getDataDirectory() + "apartment.xml", true, false);
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "sslcert", getDSS().getPropertySystem().getStringValue("/config/configdirectory") + "dsscert.pem" , true, false);

    std::vector<std::string> portList = splitString(DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "listen"), ',', true);
    std::string configPorts, protocol;
    std::string bindIP = DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "bindip");
    foreach(std::string port, portList) {
      protocol = "s";   // defaults to https
#ifdef WITH_HTTP
      if(port.at(port.length() - 1) == 'h') {
        protocol = "";  // use plain http on this port
        port.erase(port.length() - 1);
      }
#endif
      if(port.find(":") == std::string::npos) {
        configPorts += bindIP + ":" + port;
      } else {
        configPorts += port;
      }
      configPorts += protocol + ",";
    }
    configPorts.erase(configPorts.length() - 1);
    log("Webserver: Listening on " + configPorts, lsInfo);
    m_TrustedPort = getDSS().getPropertySystem().getIntValue(getConfigPropertyBasePath() + "trustedPort");

    std::string configAliases = DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "webroot");
    log("Webserver: Configured webroot: " + configAliases, lsInfo);

    std::string sslCert = DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "sslcert");
    if (!boost::filesystem::exists(sslCert)) {
      throw std::runtime_error("Could not find SSL certificate file: " + sslCert);
    }

    log("Webserver: Configured SSL certificate: " + sslCert, lsInfo);

    const char *mgOptions[] = {
      "document_root", configAliases.c_str(),
      "listening_ports", configPorts.c_str(),
      "ssl_certificate", sslCert.c_str(),
      NULL
    };

    m_mgContext = mg_start(&httpRequestCallback, mgOptions);
    if (m_mgContext == NULL) {
      throw std::runtime_error("Could not start webserver");
    }
    log("Webserver started", lsInfo);

    getDSS().getPropertySystem().setIntValue(getConfigPropertyBasePath() + "sessionTimeoutMinutes", WEB_SESSION_TIMEOUT_MINUTES, true, false);
    m_SessionManager->setTimeout(getDSS().getPropertySystem().getIntValue(getConfigPropertyBasePath() + "sessionTimeoutMinutes")*60);

    publishJSLogfiles();
    setupAPI();
    instantiateHandlers();
  } // initialize

  void WebServer::setSessionManager(boost::shared_ptr<SessionManager> _pSessionManager) {
    m_SessionManager = _pSessionManager;
  } // setSessionManager

  void WebServer::publishJSLogfiles() {
    // pre-create the property as the gui accesses it on startup and gets upset if the node's not there
    getDSS().getPropertySystem().createProperty("/system/js/logfiles/");

    std::string logDir = getDSS().getJSLogDirectory();
    try {
      fs::directory_iterator end_iter;
      for(fs::directory_iterator dir_itr(logDir);
          dir_itr != end_iter;
          ++dir_itr)  {
        if(fs::is_regular(dir_itr->status())) {
#if defined(BOOST_VERSION_135)
          std::string fileName = dir_itr->filename();
          if(endsWith(dir_itr->filename(), ".log") ||
             endsWith(dir_itr->filename(), ".LOG")) {
            std::string abspath = dir_itr->string();
#else
          std::string fileName = dir_itr->path().filename().string();
          if(endsWith(dir_itr->path().filename().string(), ".log") ||
             endsWith(dir_itr->path().filename().string(), ".LOG")) {
            std::string abspath = dir_itr->path().string();
#endif
            getDSS().getPropertySystem().setStringValue("/config/subsystems/WebServer/files/" + std::string(fileName), abspath, true, false);
            getDSS().getPropertySystem().setStringValue("/system/js/logfiles/" + std::string(fileName), abspath, true, false);
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

  void WebServer::doStart() { } // start

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
  const char* kHandlerMetering = "metering";
  const char* kHandlerSubscription = "subscription";

  void WebServer::instantiateHandlers() {
    m_Handlers[kHandlerApartment] = new ApartmentRequestHandler(getDSS().getApartment(), getDSS().getModelMaintenance());
    m_Handlers[kHandlerZone] =
      new ZoneRequestHandler(
        getDSS().getApartment(),
        getDSS().getBusInterface().getStructureModifyingBusInterface(),
        getDSS().getBusInterface().getStructureQueryBusInterface());
    m_Handlers[kHandlerDevice] =
      new DeviceRequestHandler(
        getDSS().getApartment(),
        getDSS().getBusInterface().getStructureModifyingBusInterface(),
        getDSS().getBusInterface().getStructureQueryBusInterface());
    m_Handlers[kHandlerCircuit] = new CircuitRequestHandler(
            getDSS().getApartment(),
            getDSS().getModelMaintenance(),
            getDSS().getBusInterface().getStructureModifyingBusInterface(),
            getDSS().getBusInterface().getStructureQueryBusInterface());
    m_Handlers[kHandlerSet] = new SetRequestHandler(getDSS().getApartment());
    m_Handlers[kHandlerProperty] = new PropertyRequestHandler(getDSS().getPropertySystem());
    m_Handlers[kHandlerEvent] = new EventRequestHandler(getDSS().getEventInterpreter());
    m_Handlers[kHandlerSystem] = new SystemRequestHandler(m_SessionManager);
    m_Handlers[kHandlerStructure] =
      new StructureRequestHandler(
        getDSS().getApartment(),
        getDSS().getModelMaintenance(),
        *getDSS().getBusInterface().getStructureModifyingBusInterface(),
        *getDSS().getBusInterface().getStructureQueryBusInterface()
      );
    m_Handlers[kHandlerSim] = new SimRequestHandler(getDSS().getApartment());
    m_Handlers[kHandlerMetering] = new MeteringRequestHandler(getDSS().getApartment(), getDSS().getMetering());
    m_Handlers[kHandlerSubscription] = new SubscriptionRequestHandler(getDSS().getEventInterpreter());
  } // instantiateHandlers

  HashMapConstStringString WebServer::parseCookies(const char* _cookies) {
      HashMapConstStringString result;

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

  std::string WebServer::generateCookieString(HashMapConstStringString _cookies) {
    std::string result = "";

    HashMapConstStringString::iterator i;
    std::string path;

    for (i = _cookies.begin(); i != _cookies.end(); i++) {
      if (i->first == "path") {
        path = i->second;
        continue;
      }

      if (!result.empty()) {
        result = result + "; ";
      }

      result = result + i->first + "=" + (i->second);
    }

    if (!path.empty()) {
      if (!result.empty()) {
        result = result + "; ";
      }

      result = result + "path=" + path;
    }

    return result;
  }

  void *WebServer::jsonHandler(struct mg_connection* _connection,
                               const struct mg_request_info* _info,
                               HashMapConstStringString _parameter,
                               HashMapConstStringString _cookies,
                               HashMapConstStringString _injectedCookies,
                               boost::shared_ptr<Session> _session) {
    const std::string urlid = "/json/";

    std::string uri = _info->uri;
    std::string method = uri.substr(uri.find(urlid) + urlid.size());

    RestfulRequest request(method, _parameter, _cookies);
    request.setActiveCallback(boost::bind(&mg_connection_active, _connection));

    log("Processing call to " + method);

    std::string result;
    if(m_Handlers[request.getClass()] != NULL) {
      try {
        WebServerResponse response =
          m_Handlers[request.getClass()]->jsonHandleRequest(request, _session);
        if(response.getResponse() != NULL) {
          result = response.getResponse()->toString();
        }
        std::string cookies;
        if(response.getCookies().empty()) {
          cookies = generateCookieString(_injectedCookies);
        } else {
          cookies = generateCookieString(response.getCookies());
        }
        emitHTTPHeader(200, _connection, "application/json", cookies);
      } catch(SecurityException& e) {
        emitHTTPHeader(403, _connection, "application/json");
        JSONObject resultObj;
        resultObj.addProperty("ok", false);
        resultObj.addProperty("message", e.what());
        result = resultObj.toString();
      } catch(std::runtime_error& e) {
        emitHTTPHeader(500, _connection, "application/json");
        JSONObject resultObj;
        resultObj.addProperty("ok", false);
        resultObj.addProperty("message", e.what());
        result = resultObj.toString();
      } catch(std::invalid_argument& e) {
        emitHTTPHeader(500, _connection, "application/json");
        JSONObject resultObj;
        resultObj.addProperty("ok", false);
        resultObj.addProperty("message", e.what());
        result = resultObj.toString();
      }
    } else {
      emitHTTPHeader(404, _connection, "application/json");
      std::ostringstream sstream;
      sstream << "{" << "\"ok\"" << ":" << "false" << ",";
      sstream << "\"message\"" << ":" << "\"Call to unknown function\"";
      sstream << "}";
      result = sstream.str();
      log("Unknown function '" + method + "'", lsError);
    }
    mg_write(_connection, result.c_str(), result.length());
    return _connection;
  } // jsonHandler

  void *WebServer::downloadHandler(struct mg_connection* _connection,
                                  const struct mg_request_info* _info) {
    const std::string kURLID = "/download/";
    std::string uri = _info->uri;

    std::string givenFileName = uri.substr(uri.find(kURLID) + kURLID.size());

    log("Processing call to download/" + givenFileName);

    // TODO: make the files-node readonly as this might pose a security threat
    //       (you could download any file on the disk if you add it as a subnode
    //        of files)
    PropertyNodePtr filesNode = getDSS().getPropertySystem().getProperty(
                                  getConfigPropertyBasePath() + "files"
                                );
    std::string fileName;
    if(filesNode != NULL) {
      PropertyNodePtr fileNode = filesNode->getProperty(givenFileName);
      if(fileNode != NULL) {
        fileName = fileNode->getStringValue();
      }
    }
    log("Using local file: " + fileName);
    if (boost::filesystem::exists(fileName)) {
        FILE *fp = fopen(fileName.c_str(), "r");
        if (fp != NULL)
        {
            mg_send_file(_connection, fp, fs::file_size(fileName));
            fclose(fp);
        }
    }
    return _connection;
  } // downloadHandler

  void *WebServer::httpBrowseProperties(struct mg_connection* _connection,
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
    PropertyNodePtr node;
    try {
      node = DSS::getInstance()->getPropertySystem().getProperty(path);
    } catch(SecurityException&) {
      stream << "Access denied";
    }
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
        try {
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
        } catch(SecurityException& ex) {
          stream << "access denied";
        }
      }
    }

    stream << "</ul></body></html>";
    std::string tmp = stream.str();
    mg_write(_connection, tmp.c_str(), tmp.length());
    return _connection;
  } // httpBrowseProperties

  void *WebServer::httpRequestCallback(enum mg_event event,
                                       struct mg_connection* _connection,
                                       const struct mg_request_info* _info) {
    if (event != MG_NEW_REQUEST) {
      return NULL;
    }
    std::string uri = _info->uri;

    WebServer& self = DSS::getInstance()->getWebServer();
    self.m_SessionManager->getSecurity()->signOff();

    HashMapConstStringString paramMap = parseParameter(_info->query_string);
    const char* cookie = mg_get_header(_connection, "Cookie");
    HashMapConstStringString cookies = self.parseCookies(cookie);

    boost::shared_ptr<Session> session;
    std::string token = cookies["token"];
    if(token.empty()) {
      token = paramMap["token"];
    }
    if(!token.empty()) {
      session = self.m_SessionManager->getSession(token);
    }
    HashMapConstStringString injectedCookies;

    // if we're coming from a trusted port, impersonate that user and start
    // a new session on his behalf
    if(((session == NULL) || (session->getUser() == NULL)) &&
            (_info->local_port == self.m_TrustedPort)) {
      const char* digestCString = mg_get_header(_connection, "Authorization");
      if(digestCString > 0) {
        std::string digest = digestCString;
        const std::string userNamePrefix = "username=\"";
        std::string::size_type userNamePos = digest.find(userNamePrefix);
        std::string::size_type userNameEnd =
          digest.find("\"", userNamePos + userNamePrefix.size() - 1);
        if((userNamePos != std::string::npos) &&
           (userNameEnd != std::string::npos)) {
          std::string userName = digest.substr(userNamePos + userNamePrefix.size(),
                                               userNameEnd - userNamePos - 1);
          self.log("Logging-in from a trusted port as '" + userName + "'");
          self.m_SessionManager->getSecurity()->impersonate(userName);
          if (session == NULL) {
            std::string newToken = self.m_SessionManager->registerSession();
            session = self.m_SessionManager->getSession(newToken);
            injectedCookies["path"] = "/";
            injectedCookies["token"] = newToken;
          }
          session->inheritUserFromSecurity();
        }
      }
    }

    if(session != NULL) {
      session->touch();
      self.m_SessionManager->getSecurity()->authenticate(session);
    }

    if (uri.find("/browse/") == 0) {
      return self.httpBrowseProperties(_connection, _info);
    } else if (uri.find("/json/") == 0) {
      return self.jsonHandler(_connection, _info, paramMap, cookies,
                              injectedCookies, session);
    } else if (uri.find("/download/") == 0) {
      return self.downloadHandler(_connection, _info);
    }

    return NULL;
  }
}