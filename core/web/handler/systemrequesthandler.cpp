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

#include "core/datetools.h"

#include "core/web/json.h"
#include "core/dss.h"
#include "core/sessionmanager.h"
#include "core/session.h"

#include "core/security/user.h"
#include "core/security/security.h"

#include <sstream>

namespace dss {

  //=========================================== SystemRequestHandler

  WebServerResponse SystemRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    if(_request.getMethod() == "version") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("version", DSS::getInstance()->versionString());
      return success(resultObj);
    } else if (_request.getMethod() == "time") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("time",
              static_cast<long unsigned int>(DateTime().secondsSinceEpoch()));
      return success(resultObj);
    } else if(_request.getMethod() == "login") {
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
        m_pSessionManager->getSession(token)->inheritUserFromSecurity();
        log("Registered new JSON session");

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
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest


} // namespace dss
