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

#include "circuitrequesthandler.h"

#include "core/model/modulator.h"
#include "core/model/apartment.h"

#include "core/web/json.h"
#include "core/model/modelmaintenance.h"

namespace dss {


  //=========================================== CircuitRequestHandler

  CircuitRequestHandler::CircuitRequestHandler(Apartment& _apartment, ModelMaintenance& _modelMaintenance)
  : m_Apartment(_apartment), m_ModelMaintenance(_modelMaintenance)
  { }

  boost::shared_ptr<JSONObject> CircuitRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    std::string idString = _request.getParameter("id");
    if(idString.empty()) {
      return failure("Missing parameter id");
    }
    dsid_t dsid = NullDSID;
    try {
      dsid = dsid_t::fromString(idString);
    } catch(std::invalid_argument&) {
      return failure("Could not parse dsid");
    }
    try {
      boost::shared_ptr<DSMeter> dsMeter = m_Apartment.getDSMeterByDSID(dsid);
      if(_request.getMethod() == "getName") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("name", dsMeter->getName());
        return success(resultObj);
      } else if(_request.getMethod() == "setName") {
        if(_request.hasParameter("newName")) {
          dsMeter->setName(_request.getParameter("newName"));
          m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
          return success();
        } else {
          return failure("missing parameter newName");
        }
      } else if(_request.getMethod() == "getEnergyBorder") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("orange", dsMeter->getEnergyLevelOrange());
        resultObj->addProperty("red", dsMeter->getEnergyLevelRed());
        return success(resultObj);
      } else if(_request.getMethod() == "getConsumption") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("consumption", dsMeter->getPowerConsumption());
        return success(resultObj);
      } else if(_request.getMethod() == "getEnergyMeterValue") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("meterValue", dsMeter->getEnergyMeterValue());
        return success(resultObj);
      } else if(_request.getMethod() == "rescan") {
        dsMeter->setIsValid(false);
        return success();
      } else {
        throw std::runtime_error("Unhandled function");
      }
    } catch(ItemNotFoundException&) {
      return failure("Could not find dSMeter with given dsid");
    }
  } // handleRequest

} // namespace dss
