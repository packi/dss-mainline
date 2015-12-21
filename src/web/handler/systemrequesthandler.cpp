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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "systemrequesthandler.h"

#include <locale>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include <digitalSTROM/dsuid.h>

#include "src/datetools.h"
#include "src/ds485types.h"
#include "src/dss.h"
#include "src/propertysystem.h"
#include "src/security/user.h"
#include "src/security/security.h"
#include "src/session.h"
#include "src/sessionmanager.h"
#include "src/stringconverter.h"
#include "util.h"
#include "unix/systeminfo.h"

namespace dss {

  //=========================================== SystemRequestHandler

  WebServerResponse SystemRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session, const struct mg_connection* _connection) {
    StringConverter st("UTF-8", "UTF-8");
    if(_request.getMethod() == "version") {
      JSONWriter json;
      json.add("version", DSS::getInstance()->versionString());
      json.add("distroVersion", DSS::getInstance()->readDistroVersion());
      SystemInfo info;
      std::vector<std::string> hwv = info.sysinfo();
      for (std::vector<std::string>::iterator it = hwv.begin(); it != hwv.end(); it++) {
        std::string key = it->substr(0, it->find(':'));
        std::string value = it->substr(it->find(':') + 1);
        json.add(key, value);
      }
      return json.successJSON();
    } else if ((_request.getMethod() == "getDSID") ||
               (_request.getMethod() == "getDSUID")) {
      DSS::getInstance()->getSecurity().loginAsSystemUser("dSUID call needs system rights");

      std::string dsuidStr;
      std::string dsidStr;

      PropertyNodePtr dsidNode = DSS::getInstance()->getPropertySystem().getProperty(pp_sysinfo_dsid);
      if (dsidNode != NULL) {
        dsidStr = dsidNode->getAsString();
      }
      PropertyNodePtr dsuidNode = DSS::getInstance()->getPropertySystem().getProperty(pp_sysinfo_dsuid);
      if (dsuidNode != NULL) {
        dsuidStr = dsuidNode->getAsString();
      }

      JSONWriter json;
      json.add("dSID", dsidStr);
      json.add("dSUID", dsuidStr);
      return json.successJSON();
    } else if (_request.getMethod() == "time") {
      JSONWriter json;
      time_t t0 = DateTime().secondsSinceEpoch();
      struct tm* tnow = localtime(&t0);
      json.add("time", static_cast<long long int> (t0));
      json.add("offset", static_cast<long long int> (tnow->tm_gmtoff));
      json.add("daylight", tnow->tm_isdst > 0);
      json.add("timezone", tnow->tm_zone);
      return json.successJSON();
    } else if(_request.getMethod() == "login") {
      if(_session != NULL) {
        JSONWriter json;
        json.add("token", _session->getID());

        WebServerResponse response(json.successJSON());
        response.setPublishSessionToken(_session->getID());
        return response;
      } else {
        std::string userRaw = _request.getParameter("user");
        std::string user;
        std::locale locl;
        std::remove_copy_if(userRaw.begin(), userRaw.end(), std::back_inserter(user),
          !boost::bind(&std::isalnum<char>, _1, locl)
        );

        std::string password = _request.getParameter("password");

        if(user.empty()) {
          return JSONWriter::failure("Missing parameter 'user'");
        }
        if(password.empty()) {
          return JSONWriter::failure("Missing parameter 'password'");
        }

        m_pSessionManager->getSecurity()->signOff();
        if(m_pSessionManager->getSecurity()->authenticate(user, password)) {
          std::string token = m_pSessionManager->registerSession();
          if (token.empty()) {
            log("Session limit reached", lsError);
            return JSONWriter::failure("Authentication failed");
          }
          m_pSessionManager->getSession(token)->inheritUserFromSecurity();
          log("Registered new JSON session for user: " + user +
              " (" + token + ")");

          JSONWriter json;
          json.add("token", token);

          WebServerResponse response(json.successJSON());
          response.setPublishSessionToken(token);
          return response;
        } else {
          log("Authentication failed for user '" + user + "'", lsError);
          return JSONWriter::failure("Authentication failed");
        }
      }
    } else if(_request.getMethod() == "loginApplication") {
      std::string loginTokenRaw = _request.getParameter("loginToken");
      std::string loginToken;
      std::locale locl;
      std::remove_copy_if(loginTokenRaw.begin(),
                          loginTokenRaw.end(),
                          std::back_inserter(loginToken),
        !boost::bind(&std::isalnum<char>, _1, locl)
       );

      if(loginToken.empty()) {
        return JSONWriter::failure("Missing parameter 'loginToken'");
      }

      m_pSessionManager->getSecurity()->signOff();
      if(m_pSessionManager->getSecurity()->authenticateApplication(loginToken)) {
        std::string token = m_pSessionManager->registerApplicationSession();
        if (token.empty()) {
          log("Session limit reached", lsError);
          return JSONWriter::failure("Application-Authentication failed");
        }
        m_pSessionManager->getSession(token)->inheritUserFromSecurity();
        log("Registered new JSON-Application session for " +
            m_pSessionManager->getSecurity()->getApplicationName(loginToken) +
            " (" + token + ")");

        JSONWriter json;
        json.add("token", token);

        WebServerResponse response(json.successJSON());
        response.setPublishSessionToken(token);
        return response;
      } else {
        log("Application-Authentication failed", lsError);
        return JSONWriter::failure("Application-Authentication failed");
      }
    } else if(_request.getMethod() == "logout") {
      m_pSessionManager->getSecurity()->signOff();
      if(_session != NULL) {
        m_pSessionManager->removeSession(_session->getID());
      }
      WebServerResponse response(JSONWriter::success());
      response.setRevokeSessionToken();
      return response;
    } else if(_request.getMethod() == "loggedInUser") {
      JSONWriter json;
      User* pUser = m_pSessionManager->getSecurity()->getCurrentlyLoggedInUser();
      if(pUser != NULL) {
        json.add("name", pUser->getName());
      }
      return json.successJSON();
    } else if(_request.getMethod() == "setPassword") {
      if(_session == NULL) {
        return JSONWriter::failure("Need to be logged in to change password");
      }

      std::string password = _request.getParameter("password");
      if(password.empty()) {
        return JSONWriter::failure("Missing parameter 'password'");
      }

      User* pUser = m_pSessionManager->getSecurity()->getCurrentlyLoggedInUser();
      if(pUser != NULL) {
        pUser->setPassword(password);
        return JSONWriter::success("Password changed, have a nice day!");
      } else {
        return JSONWriter::failure("You've got a session but no user, strange... not doing anything for you sir.");
      }
    } else if(_request.getMethod() == "requestApplicationToken") {
      if(_session != NULL) {
        return JSONWriter::failure("Must not be logged-in to request a token");
      }

      // TODO: filter characters in applicationName
      std::string applicationName = st.convert(_request.getParameter("applicationName"));
      if(applicationName.empty()) {
        return JSONWriter::failure("Need parameter 'applicationName'");
      }
      applicationName = escapeHTML(applicationName);

      std::string applicationToken =  SessionTokenGenerator::generate();
      m_pSessionManager->getSecurity()->loginAsSystemUser("Temporary access to create token");
      m_pSessionManager->getSecurity()->createApplicationToken(applicationName,
                                                               applicationToken);

      JSONWriter json;
      json.add("applicationToken", applicationToken);
      return json.successJSON();
    } else if(_request.getMethod() == "enableToken") {
      if(_session == NULL) {
        return JSONWriter::failure("Must be logged-in");
      }

      if(_session->isApplicationSession()) {
        return JSONWriter::failure("Please let the user validate your tokens");
      }

      User* pUser = m_pSessionManager->getSecurity()->getCurrentlyLoggedInUser();
      if(pUser == NULL) {
        return JSONWriter::failure("You've got a session but no user, strange... not doing anything for you sir.");
      }

      std::string applicationToken = _request.getParameter("applicationToken");
      if(applicationToken.empty()) {
        return JSONWriter::failure("Need parameter 'applicationToken'");
      }

      // create a copy since logging in as system will invalidate our pointer
      User userCopy(*pUser);
      pUser = NULL;
      m_pSessionManager->getSecurity()->loginAsSystemUser("Temporary access to grant token");
      if(m_pSessionManager->getSecurity()->enableToken(applicationToken, &userCopy)) {
        return JSONWriter::success();
      } else {
        return JSONWriter::failure("Unknown token");
      }
    } else if(_request.getMethod() == "revokeToken") {
      if(_session == NULL) {
        return JSONWriter::failure("Must be logged-in");
      }

      std::string applicationToken = _request.getParameter("applicationToken");
      if(applicationToken.empty()) {
        return JSONWriter::failure("Need parameter 'applicationToken'");
      }

      m_pSessionManager->getSecurity()->loginAsSystemUser("Temporary access to revoke token");
      if(m_pSessionManager->getSecurity()->revokeToken(applicationToken)) {
        return JSONWriter::success();
      } else {
        return JSONWriter::failure("Unknown token");
      }
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest


} // namespace dss
