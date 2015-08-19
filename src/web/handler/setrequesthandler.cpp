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


#include "setrequesthandler.h"

#include "src/model/apartment.h"
#include "src/model/set.h"

#include "src/setbuilder.h"

#include "jsonhelper.h"

namespace dss {

  //=========================================== SetRequestHandler
  
  SetRequestHandler::SetRequestHandler(Apartment& _apartment)
  : m_Apartment(_apartment)
  { }

  WebServerResponse SetRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session, const struct mg_connection* _connection) {
    if(_request.getMethod() == "fromApartment") {
      JSONWriter json;
      json.add("self", ".");
      return json.successJSON();
    } else {
      std::string self = trim(_request.getParameter("self"));
      if(self.empty()) {
        return JSONWriter::failure("Missing parameter 'self'");
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
          return JSONWriter::failure("Missing either zoneID or zoneName");
        }

        JSONWriter json;
        json.add("self", self + additionalPart);
        return json.successJSON();
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
          return JSONWriter::failure("Missing either groupID or groupName");
        }

        JSONWriter json;
        json.add("self", self + additionalPart);
        return json.successJSON();
      } else if(_request.getMethod() == "byDSID") {
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        if(!_request.getParameter("dsid").empty()) {
          additionalPart += "dsud(" + _request.getParameter("dsid") + ")";
        } else {
          return JSONWriter::failure("Missing parameter dsid");
        }

        JSONWriter json;
        json.add("self", self + additionalPart);
        return json.successJSON();
      } else if(_request.getMethod() == "byDSUID") {
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        if(!_request.getParameter("dsuid").empty()) {
          additionalPart += "dsuid(" + _request.getParameter("dsuid") + ")";
        } else {
          return JSONWriter::failure("Missing parameter dsuid");
        }

        JSONWriter json;
        json.add("self", self + additionalPart);
        return json.successJSON();

      } else if(_request.getMethod() == "getDevices") {
        SetBuilder builder(m_Apartment);
        Set set = builder.buildSet(self, boost::shared_ptr<Zone>());
        JSONWriter json(JSONWriter::jsonArrayResult);
        toJSON(set, json);
        return json.successJSON();
      } else if(_request.getMethod() == "add") {
        std::string other = _request.getParameter("other");
        if(other.empty()) {
          return JSONWriter::failure("Missing parameter other");
        }
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        additionalPart += "add(" + other + ")";

        JSONWriter json;
        json.add("self", self + additionalPart);
        return json.successJSON();
      } else if(_request.getMethod() == "subtract") {
        std::string other = _request.getParameter("other");
        if(other.empty()) {
          return JSONWriter::failure("Missing parameter other");
        }
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        additionalPart += "subtract(" + other + ")";

        JSONWriter json;
        json.add("self", self + additionalPart);
        return json.successJSON();
      } else if(isDeviceInterfaceCall(_request)) {
        SetBuilder builder(m_Apartment);
        Set set = builder.buildSet(self, boost::shared_ptr<Zone>());
        boost::shared_ptr<Set> pSet = boost::make_shared<Set>(set);
        return handleDeviceInterfaceRequest(_request, pSet, _session);
      } else {
        throw std::runtime_error("Unhandled function");
      }
    }
  } // handleRequest


} // namespace dss
