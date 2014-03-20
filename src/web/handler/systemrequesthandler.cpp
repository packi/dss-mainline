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

#include "systemrequesthandler.h"

#include <locale>

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "src/datetools.h"

#include "src/web/json.h"
#include "src/dss.h"
#include "src/sessionmanager.h"
#include "src/session.h"

#include "src/security/user.h"
#include "src/security/security.h"
#include "src/stringconverter.h"

#include "src/propertysystem.h"
#include "util.h"
#include <sstream>

namespace dss {

  //=========================================== SystemRequestHandler

  WebServerResponse SystemRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    StringConverter st("UTF-8", "UTF-8");
    if(_request.getMethod() == "version") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("version", DSS::getInstance()->versionString());
      return success(resultObj);
    } else if (_request.getMethod() == "getDSID") {
      std::string dsid;
      DSS::getInstance()->getSecurity().loginAsSystemUser("dSID call needs system rights");

      PropertyNodePtr dsidNode =
          DSS::getInstance()->getPropertySystem().getProperty(
                  "/system/dSID");
      if (dsidNode != NULL) {
        dsid = dsidNode->getAsString();
      }

      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("dSID", dsid);
      return success(resultObj);
    } else if (_request.getMethod() == "time") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("time",
              static_cast<long unsigned int>(DateTime().secondsSinceEpoch()));
      return success(resultObj);
    } else if(_request.getMethod() == "login") {
      if(_session != NULL) {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("token", _session->getID());

        WebServerResponse response(success(resultObj));
        response.setCookie("path", "/");
        response.setCookie("token", _session->getID());
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
          return failure("Missing parameter 'user'");
        }
        if(password.empty()) {
          return failure("Missing parameter 'password'");
        }

        m_pSessionManager->getSecurity()->signOff();
        if(m_pSessionManager->getSecurity()->authenticate(user, password)) {
          std::string token = m_pSessionManager->registerSession();
          if (token.empty()) {
            log("Session limit reached", lsError);
            return failure("Authentication failed");
          }
          m_pSessionManager->getSession(token)->inheritUserFromSecurity();
          log("Registered new JSON session for user: " + user +
              " (" + token + ")");

          boost::shared_ptr<JSONObject> resultObj(new JSONObject());
          resultObj->addProperty("token", token);

          WebServerResponse response(success(resultObj));
          response.setCookie("path", "/");
          response.setCookie("token", token);
          return response;
        } else {
          log("Authentication failed for user '" + user + "'", lsError);
          return failure("Authentication failed");
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
        return failure("Missing parameter 'loginToken'");
      }

      m_pSessionManager->getSecurity()->signOff();
      if(m_pSessionManager->getSecurity()->authenticateApplication(loginToken)) {
        std::string token = m_pSessionManager->registerApplicationSession();
        if (token.empty()) {
          log("Session limit reached", lsError);
          return failure("Application-Authentication failed");
        }
        m_pSessionManager->getSession(token)->inheritUserFromSecurity();
        log("Registered new JSON-Application session for " +
            m_pSessionManager->getSecurity()->getApplicationName(loginToken) +
            " (" + token + ")");

        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("token", token);

        WebServerResponse response(success(resultObj));
        response.setCookie("path", "/");
        response.setCookie("token", token);
        return response;
      } else {
        log("Application-Authentication failed", lsError);
        return failure("Application-Authentication failed");
      }
    } else if(_request.getMethod() == "logout") {
      m_pSessionManager->getSecurity()->signOff();
      if(_session != NULL) {
        m_pSessionManager->removeSession(_session->getID());
      }
      WebServerResponse response(success());
      response.setCookie("path", "/");
      response.setCookie("token", "");
      return response;
    } else if(_request.getMethod() == "loggedInUser") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      User* pUser = m_pSessionManager->getSecurity()->getCurrentlyLoggedInUser();
      if(pUser != NULL) {
        resultObj->addProperty("name", pUser->getName());
      }
      return success(resultObj);
    } else if(_request.getMethod() == "setPassword") {
      if(_session == NULL) {
        return failure("Need to be logged in to change password");
      }

      std::string password = _request.getParameter("password");
      if(password.empty()) {
        return failure("Missing parameter 'password'");
      }

      User* pUser = m_pSessionManager->getSecurity()->getCurrentlyLoggedInUser();
      if(pUser != NULL) {
        pUser->setPassword(password);
        return success("Password changed, have a nice day!");
      } else {
        return failure("You've got a session but no user, strange... not doing anything for you sir.");
      }
    } else if(_request.getMethod() == "requestApplicationToken") {
      if(_session != NULL) {
        return failure("Must not be logged-in to request a token");
      }

      // TODO: filter characters in applicationName
      std::string applicationName = st.convert(_request.getParameter("applicationName"));
      if(applicationName.empty()) {
        return failure("Need parameter 'applicationName'");
      }
      applicationName = escapeHTML(applicationName);

      std::string applicationToken =  SessionTokenGenerator::generate();
      m_pSessionManager->getSecurity()->loginAsSystemUser("Temporary access to create token");
      m_pSessionManager->getSecurity()->createApplicationToken(applicationName,
                                                               applicationToken);

      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("applicationToken", applicationToken);
      return success(resultObj);
    } else if(_request.getMethod() == "enableToken") {
      if(_session == NULL) {
        return failure("Must be logged-in");
      }

      if(_session->isApplicationSession()) {
        return failure("Please let the user validate your tokens");
      }

      User* pUser = m_pSessionManager->getSecurity()->getCurrentlyLoggedInUser();
      if(pUser == NULL) {
        return failure("You've got a session but no user, strange... not doing anything for you sir.");
      }

      std::string applicationToken = _request.getParameter("applicationToken");
      if(applicationToken.empty()) {
        return failure("Need parameter 'applicationToken'");
      }

      // create a copy since logging in as system will invalidate our pointer
      User userCopy(*pUser);
      pUser = NULL;
      m_pSessionManager->getSecurity()->loginAsSystemUser("Temporary access to grant token");
      if(m_pSessionManager->getSecurity()->enableToken(applicationToken, &userCopy)) {
        return success();
      } else {
        return failure("Unknown token");
      }
    } else if(_request.getMethod() == "revokeToken") {
      if(_session == NULL) {
        return failure("Must be logged-in");
      }

      std::string applicationToken = _request.getParameter("applicationToken");
      if(applicationToken.empty()) {
        return failure("Need parameter 'applicationToken'");
      }

      m_pSessionManager->getSecurity()->loginAsSystemUser("Temporary access to revoke token");
      if(m_pSessionManager->getSecurity()->revokeToken(applicationToken)) {
        return success();
      } else {
        return failure("Unknown token");
      }
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest


} // namespace dss
