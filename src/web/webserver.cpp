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
#include "src/ds485types.h"
#include "src/propertysystem.h"
#include "src/foreach.h"

#include "src/security/security.h"
#include "src/session.h"
#include "src/sessionmanager.h"

#include "src/web/restful.h"
#include "src/web/webrequests.h"

#include "src/web/handler/databaserequesthandler.h"
#include "src/web/handler/systemrequesthandler.h"
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

#include "src/model/device.h"
#include "src/model/apartment.h"

#include <netinet/in.h>
#include <arpa/inet.h>

namespace fs = boost::filesystem;

namespace dss {

  static const char* httpCodeToMessage(const int _code) {
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

  static std::string strprintf_HTTPHeader(int _code, int length,
                                          const std::string& _contentType,
                                          const std::string& _setCookie,
                                          const std::string& _contentName = "") {
    std::ostringstream sstream;
    sstream << "HTTP/1.1 " << _code << ' ' << httpCodeToMessage(_code) << "\r\n";
    sstream << "Content-Type: " << _contentType << "; charset=utf-8\r\n";
    if(!_setCookie.empty()) {
      sstream << "Set-Cookie: " << _setCookie << "; HttpOnly";
#ifndef WITH_INSECURE_COOKIE
      sstream << "; Secure";
#endif
      sstream << "\r\n";
    }
    sstream << "Content-Length: " << intToString(length) << "\r\n";
    if (!_contentName.empty()) {
      sstream << "Content-Disposition: attachment; filename=\"" << _contentName << "\"" << "\r\n";
    }
    sstream << "\r\n";
    std::string header = sstream.str();
    return header;
  }

  static void emitHTTPHeader(struct mg_connection* _connection, int _code, int length,
                             const std::string& _contentType,
                             const std::string& _setCookie = "",
                             const std::string& _contentName = "") {
    std::string tmp = strprintf_HTTPHeader(_code, length, _contentType,
                                           _setCookie, _contentName);
    mg_write(_connection, tmp.c_str(), tmp.length());
  }

  static void emitHTTPPacket(struct mg_connection* _connection, int _code,
                             const std::string& _contentType,
                             const std::string& _setCookie,
                             const std::string& content) {
    std::string packet = strprintf_HTTPHeader(_code, content.length(),
                                              _contentType, _setCookie);
    packet += content;
    mg_write(_connection, packet.c_str(), packet.length());
  }

  static void emitHTTPJsonPacket(struct mg_connection* _connection, int _code,
                                 const std::string& _setCookie,
                                 const std::string& content) {
    return emitHTTPPacket(_connection, _code, "application/json", _setCookie,
                          content);
  }

  static void emitHTTPTextPacket(struct mg_connection* _connection, int _code,
                                 const std::string& _setCookie,
                                 const std::string& content) {
    return emitHTTPPacket(_connection, _code, "text/html", _setCookie,
                          content);
  }

  //============================================= WebServer

  WebServer::WebServer(DSS* _pDSS)
    : Subsystem(_pDSS, "WebServer"), m_mgContext(0),
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
    } else if (boost::filesystem::is_empty(sslCert)) {
      throw std::runtime_error("SSL certificate file is empty: " + sslCert);
    }

    log("Webserver: Configured SSL certificate: " + sslCert, lsInfo);

    getDSS().getPropertySystem().setIntValue(getConfigPropertyBasePath() + "sessionTimeoutMinutes", WEB_SESSION_TIMEOUT_MINUTES, true, false);
    m_SessionManager->setTimeout(getDSS().getPropertySystem().getIntValue(getConfigPropertyBasePath() + "sessionTimeoutMinutes")*60);

    getDSS().getPropertySystem().setIntValue(getConfigPropertyBasePath() + "sessionLimit", WEB_SESSION_LIMIT, true, false);
    m_SessionManager->setMaxSessionCount(getDSS().getPropertySystem().getIntValue(getConfigPropertyBasePath() + "sessionLimit"));

    publishJSLogfiles();
    instantiateHandlers();

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

  void WebServer::doStart() { } // start

  const char* kHandlerApartment = "apartment";
  const char* kHandlerZone = "zone";
  const char* kHandlerDevice = "device";
  const char* kHandlerCircuit = "circuit";
  const char* kHandlerSet = "set";
  const char* kHandlerProperty = "property";
  const char* kHandlerEvent = "event";
  const char* kHandlerSystem = "system";
  const char* kHandlerStructure = "structure";
  const char* kHandlerMetering = "metering";
  const char* kHandlerSubscription = "subscription";
  const char* kHandlerDatabase = "database";

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
    m_Handlers[kHandlerMetering] = new MeteringRequestHandler(getDSS().getApartment(), getDSS().getMetering());
    m_Handlers[kHandlerSubscription] = new SubscriptionRequestHandler(getDSS().getEventInterpreter());
    m_Handlers[kHandlerDatabase] = new DatabaseRequestHandler();
  } // instantiateHandlers

  /**
   * extractToken
   * http://tools.ietf.org/html/rfc6265#section-4.1.1
   */
  std::string extractToken(const char *_cookie) {
    if (!_cookie) {
      return "";
    }

    // according spec there has to be a space
    std::string cookie_s = "; " + std::string(_cookie);
    size_t start = cookie_s.find("; token=");
    if (std::string::npos == start) {
      return "";
    }
    start += strlen("; token=");

    size_t end = cookie_s.find(';', start);
    return (end == std::string::npos) ?
      cookie_s.substr(start) :
      cookie_s.substr(start, end - start);
  }

  std::string generateCookieString(const std::string& token) {
    return "token=" + token + "; path=/";
  }

  /**
   * emit 'Set-Cookie' header with same name but 'expires' in the past
   * http://tools.ietf.org/html/rfc6265#section-4.1.2
   */
  std::string generateRevokeCookieString() {
    return "token=; path=/; expires=Wed, 29 April 1970 12:00:00 GMT";
  }

  /**
   * extractAuthenticatedUser
   * http://tools.ietf.org/html/rfc2617
   */
  std::string extractAuthenticatedUser(const char *_header) {
    if (!_header) {
      return "";
    }

    // header:= Digest SP [user | qopc | etc]+ comma separated
    // user:= "username" "=" quoted-string
    static const char key[] = " username=\"";

    std::string header_s(_header);
    size_t start = header_s.find(key);
    if (std::string::npos == start) {
      return "";
    }
    start += strlen(key);
    size_t end = header_s.find("\"", start);
    if (std::string::npos == end) {
      return "";
    }
    return header_s.substr(start, end - start);
  }

  void *WebServer::jsonHandler(struct mg_connection* _connection,
                               RestfulRequest &request,
                               const std::string &trustedSetCookie,
                               boost::shared_ptr<Session> _session) {
    std::string result, setCookieHeader = trustedSetCookie;

    request.setActiveCallback(boost::bind(&mg_connection_active, _connection));
    if(m_Handlers[request.getClass()] != NULL) {
      try {
        if ((_session == NULL) && (request.getClass() != kHandlerSystem)) {
          throw SecurityException("not logged in");
        }
        WebServerResponse response =
          m_Handlers[request.getClass()]->jsonHandleRequest(request, _session, _connection);
        std::string callback = request.getParameter("callback");
        if (callback.empty()) {
          result = response.getResponse();
        } else {
          result = callback + "(" + response.getResponse() + ")";
        }
        if (response.isRevokeSessionToken()) {
          setCookieHeader = generateRevokeCookieString();
        } else if (response.isPublishSessionToken()) {
          // CAUTION: the session is new, the _session argument
          // of this function is NULL
          setCookieHeader = generateCookieString(response.getNewSessionToken());
          // NOTE, this way we might override the trusted login session
          // IGNORE, it doesn't happen and if so, we don't care either
        }
        log("JSON request returned with 200: " + result.substr(0, 50), lsInfo);
        emitHTTPJsonPacket(_connection, 200, setCookieHeader, result);
      } catch(SecurityException& e) {
        result = JSONWriter::failure(e.what());
        log("JSON request returned with 403: " + result.substr(0, 50), lsInfo);
        emitHTTPJsonPacket(_connection, 403, setCookieHeader, result);;
      } catch(std::runtime_error& e) {
        result = JSONWriter::failure(e.what());
        log("JSON request returned with 500: " + result.substr(0, 50), lsInfo);
        emitHTTPJsonPacket(_connection, 500, setCookieHeader, result);
      } catch(std::invalid_argument& e) {
        result = JSONWriter::failure(e.what());
        log("JSON request returned with 500: " + result.substr(0, 50), lsInfo);
        emitHTTPJsonPacket(_connection, 500, setCookieHeader, result);
      }
    } else {
      log("Unknown function '" + request.getUrlPath() + "'", lsError);
      result = JSONWriter::failure("Call to unknown function");
      emitHTTPJsonPacket(_connection, 404, setCookieHeader, result);
    }
    return _connection;
  } // jsonHandler

  void *WebServer::logDownloadHandler(struct mg_connection* _connection,
                                      RestfulRequest &request,
                                      const std::string &trustedSetCookie,
                                      boost::shared_ptr<Session> _session) {
    std::string result, setCookieHeader = trustedSetCookie;
    static std::string logFileDirectory = "/var/log/collection";

    try {
      if (_session == NULL) {
        throw SecurityException("not logged in");
      }

      std::string logCollectionBasePath =
          DSS::getInstance()->getPropertySystem().getStringValue(
                            "/config/subsystems/Apartment/logCollectionBasePath");

      if (!fs::exists(logCollectionBasePath) || !fs::is_directory(logCollectionBasePath)) {
        throw std::runtime_error("Log File directory does not exist.");
      }

      fs::directory_iterator path_it(logCollectionBasePath);
      fs::directory_iterator end_it;
      bool fileFound = false;
      for (; path_it != end_it; ++path_it) {
        const fs::path &file_path = path_it->path();
        if (file_path.extension() != ".gz") {
          continue;
        }
        fileFound = true;
        log("Sending File: " + file_path.string());
        FILE *fp = fopen(file_path.c_str(), "r");
        if (fp != NULL)
        {
          emitHTTPHeader(_connection, 200, fs::file_size(file_path), "application/x-gzip",
                         setCookieHeader, fs::basename(file_path));
          mg_send_file(_connection, fp, fs::file_size(file_path));
          fclose(fp);
          break;
        }
      }
      if (!fileFound) {
        emitHTTPTextPacket(_connection, 404, setCookieHeader, "File not found");
      }
    } catch(SecurityException& e) {
      emitHTTPTextPacket(_connection, 500, setCookieHeader, e.what());
    } catch(std::runtime_error& e) {
      emitHTTPTextPacket(_connection, 500, setCookieHeader, e.what());
    } catch(std::invalid_argument& e) {
      emitHTTPTextPacket(_connection, 500, setCookieHeader, e.what());
    }

    return _connection;
  } // logDownloadHandler

  void *WebServer::iconHandler(struct mg_connection* _connection,
                               RestfulRequest &request,
                               const std::string &setCookieHeader,
                               boost::shared_ptr<Session> _session) {
    std::string result;
    try {
      if (_session == NULL) {
        throw SecurityException("not logged in");
      }

      if (request.getClass() != "getDeviceIcon") {
        throw std::runtime_error("unhandled function");
      }

      std::string dsuidStr = request.getParameter("dsuid");
      std::string dsidStr = request.getParameter("dsid");
      if (dsuidStr.empty() && dsidStr.empty()) {
        throw std::invalid_argument("missing device dsuid parameter");
      }

      dsuid_t deviceDSUID = dsidOrDsuid2dsuid(dsidStr, dsuidStr);
      boost::shared_ptr<Device> result;
      result = getDSS().getApartment().getDeviceByDSID(deviceDSUID);
      if (result == NULL) {
        throw std::runtime_error("device with id " + dsuid2str(deviceDSUID) + " not found");
      }

      std::string icon = result->getIconPath();
      if (icon.empty()) {
        throw std::runtime_error("icon for device " + dsuid2str(deviceDSUID) + " not found");
      }

      std::string iconBasePath =
          DSS::getInstance()->getPropertySystem().getStringValue(
                            "/config/subsystems/Apartment/iconBasePath");
      if (!endsWith(iconBasePath, "/")) {
        iconBasePath += "/";
      }

      icon = iconBasePath + icon;
      log("Using icon file: " + icon);
      if (boost::filesystem::exists(icon)) {
        FILE *fp = fopen(icon.c_str(), "r");
        if (fp != NULL) {
          emitHTTPHeader(_connection, 200, fs::file_size(icon), "image/png",
                         setCookieHeader);
          mg_send_file(_connection, fp, fs::file_size(icon));
          fclose(fp);
        } else {
          throw std::runtime_error("icon file " + icon + " for device " + dsuid2str(deviceDSUID) + " not found");
        }
      } else {
        throw std::runtime_error("icon file " + icon + " for device " + dsuid2str(deviceDSUID) + " not found");
      }
    } catch(SecurityException& e) {
      result = JSONWriter::failure(e.what());
      emitHTTPJsonPacket(_connection, 500, setCookieHeader, result);
    } catch(std::runtime_error& e) {
      result = JSONWriter::failure(e.what());
      emitHTTPJsonPacket(_connection, 500, setCookieHeader, result);
    } catch(std::invalid_argument& e) {
      result = JSONWriter::failure(e.what());
      emitHTTPJsonPacket(_connection, 500, setCookieHeader, result);
    }

    return _connection;
  } // iconHandler

  void *WebServer::httpBrowseProperties(struct mg_connection* _connection,
                                        RestfulRequest &request,
                                        const std::string &trustedSetCookie) {
    std::string path = request.getUrlPath();
    if (path.empty()) {
      path = "/";
    }

    std::ostringstream stream;
    stream << "<html><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><body>";
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
    emitHTTPTextPacket(_connection, 200, trustedSetCookie, tmp);
    return _connection;
  } // httpBrowseProperties

  void *WebServer::httpRequestCallback(enum mg_event event,
                                       struct mg_connection* _connection,
                                       const struct mg_request_info* _info) {
    std::string trustedLoginCookie;

    if (event != MG_NEW_REQUEST || !_info->uri || !_info->remote_ip) {
      return NULL;
    }

    std::string uri_path(_info->uri);
    size_t offset = uri_path.find('/', 1);
    if (std::string::npos == offset) {
        return NULL;
    }
    std::string toplevel = uri_path.substr(0, offset);
    std::string sublevel = uri_path.substr(offset);

    if (toplevel != "/browse" && toplevel != "/json" && toplevel != "/icons" && toplevel != "/getLatestLogs") {
      // quit early, not our request
      return NULL;
    }

    WebServer& self = DSS::getInstance()->getWebServer();
    self.m_SessionManager->getSecurity()->signOff(); // protect log call below

    {
        struct in_addr remote;
        remote.s_addr = htonl(_info->remote_ip);
        std::string remote_s(std::string(inet_ntoa(remote)));
        std::string uri_path(_info->uri);
        if (_info->query_string) {
            uri_path += "?" + std::string(_info->query_string);
        }
        self.log("REST call from " + remote_s + ": " + uri_path, lsInfo);
    }

    RestfulRequest request(sublevel, _info->query_string ?: "");

    boost::shared_ptr<Session> session;
    std::string token = extractToken(mg_get_header(_connection, "Cookie"));
    if (token.empty()) {
      // ... used when not going through lighttpd
      token = request.getParameter("token");
    }
    if (!token.empty()) {
      session = self.m_SessionManager->getSession(token);
    }

    // lighttpd is proxying to our trusted port. We trust that it
    // properly authenticated the user and just filter the HTTP
    // Authorization header for the username
    if (((session == NULL) || (session->getUser() == NULL)) &&
        (_info->local_port == self.m_TrustedPort)) {
      const char *auth_header = mg_get_header(_connection, "Authorization");
      std::string userName = extractAuthenticatedUser(auth_header);

      if (!userName.empty()) {
        self.log("Logging-in from a trusted port as '" + userName + "'");
        if (!self.m_SessionManager->getSecurity()->impersonate(userName)) {
          self.log("Unknown user", lsInfo);
          return NULL;
        }
        if (session == NULL) {
          std::string newToken = self.m_SessionManager->registerSession();
          if (newToken.empty()) {
            self.log("Session limit reached", lsError);
            return NULL;
          }
          self.log("Registered new JSON session for trusted port (" + newToken + ")");
          session = self.m_SessionManager->getSession(newToken);
          session->inheritUserFromSecurity();

          // this is the first call, piggy back the set-cookie header
          trustedLoginCookie = generateCookieString(session->getID());
        }
      }
    }

    if(session != NULL) {
      session->touch();
      self.m_SessionManager->getSecurity()->authenticate(session);
    }

    if (toplevel == "/browse") {
      return self.httpBrowseProperties(_connection, request, trustedLoginCookie);
    } else if (toplevel == "/json") {
      return self.jsonHandler(_connection, request, trustedLoginCookie, session);
    } else if (toplevel == "/icons") {
      return self.iconHandler(_connection, request, trustedLoginCookie, session);
    } else if (toplevel == "/getLatestLogs") {
      return self.logDownloadHandler(_connection, request, trustedLoginCookie, session);
    }

    return NULL;
  }
}
