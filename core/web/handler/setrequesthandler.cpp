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

#include "setrequesthandler.h"

#include "core/web/json.h"
#include "core/model/apartment.h"
#include "core/model/set.h"

#include "core/setbuilder.h"

#include "jsonhelper.h"

namespace dss {

  //=========================================== SetRequestHandler
  
  SetRequestHandler::SetRequestHandler(Apartment& _apartment)
  : m_Apartment(_apartment)
  { }

  boost::shared_ptr<JSONObject> SetRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    if(_request.getMethod() == "fromApartment") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("self", ".");
      return success(resultObj);
    } else {
      std::string self = trim(_request.getParameter("self"));
      if(self.empty()) {
        return failure("Missing parameter 'self'");
      }

      if(_request.getMethod() == "byZone") {
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        if(!_request.getParameter("zoneID").empty()) {
          additionalPart += "zone(" + _request.getParameter("zoneID") + ")";
        } else if(!_request.getParameter("zoneName").empty()) {
          additionalPart += _request.getParameter("zoneName");
        } else {
          return failure("Missing either zoneID or zoneName");
        }

        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("self", self + additionalPart);
        return success(resultObj);
      } else if(_request.getMethod() == "byGroup") {
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        if(!_request.getParameter("groupID").empty()) {
          additionalPart += "group(" + _request.getParameter("groupID") + ")";
        } else if(!_request.getParameter("groupName").empty()) {
          additionalPart += _request.getParameter("groupName");
        } else {
          return failure("Missing either groupID or groupName");
        }

        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("self", self + additionalPart);
        return success(resultObj);
      } else if(_request.getMethod() == "byDSID") {
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        if(!_request.getParameter("dsid").empty()) {
          additionalPart += "dsid(" + _request.getParameter("dsid") + ")";
        } else {
          return failure("Missing parameter dsid");
        }

        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("self", self + additionalPart);
        return success(resultObj);
      } else if(_request.getMethod() == "getDevices") {
        SetBuilder builder(m_Apartment);
        Set set = builder.buildSet(self, boost::shared_ptr<Zone>());
        return success(toJSON(set));
      } else if(_request.getMethod() == "add") {
        std::string other = _request.getParameter("other");
        if(other.empty()) {
          return failure("Missing parameter other");
        }
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        additionalPart += "add(" + other + ")";

        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("self", self + additionalPart);
        return success(resultObj);
      } else if(_request.getMethod() == "subtract") {
        std::string other = _request.getParameter("other");
        if(other.empty()) {
          return failure("Missing parameter other");
        }
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        additionalPart += "subtract(" + other + ")";

        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("self", self + additionalPart);
        return success(resultObj);
      } else if(isDeviceInterfaceCall(_request)) {
        SetBuilder builder(m_Apartment);
        Set set = builder.buildSet(self, boost::shared_ptr<Zone>());
        boost::shared_ptr<Set> pSet(new Set(set));
        return handleDeviceInterfaceRequest(_request, pSet);
      } else {
        throw std::runtime_error("Unhandled function");
      }
    }
  } // handleRequest


} // namespace dss
