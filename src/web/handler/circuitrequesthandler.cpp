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

#include <digitalSTROM/dsuid/dsuid.h>
#include "circuitrequesthandler.h"

#include "src/model/modulator.h"
#include "src/model/apartment.h"
#include "src/ds485types.h"

#include "src/web/json.h"
#include "src/model/modelmaintenance.h"
#include "src/structuremanipulator.h"
#include "src/stringconverter.h"
#include "src/util.h"

namespace dss {


  //=========================================== CircuitRequestHandler

  CircuitRequestHandler::CircuitRequestHandler(Apartment& _apartment,
                                               StructureModifyingBusInterface* _pStructureBusInterface,
                                               StructureQueryBusInterface* _pStructureQueryBusInterface)
  : m_Apartment(_apartment),
    m_pStructureBusInterface(_pStructureBusInterface),
    m_pStructureQueryBusInterface(_pStructureQueryBusInterface)
  { }

  WebServerResponse CircuitRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    std::string idString = _request.getParameter("id");
    std::string dsuidStr = _request.getParameter("dsuid");
    if (idString.empty() && dsuidStr.empty()) {
      return failure("Missing parameter dsuid");
    }

    dsuid_t dsuid;
    try {
      if (dsuidStr.empty()) {
          dsid_t dsid = str2dsid(idString);
          dsuid = dsuid_from_dsid(&dsid);
      } else {
        dsuid = str2dsuid(dsuidStr);
      }
    } catch(std::runtime_error& e) {
      return failure(e.what());
    }

    try {
      boost::shared_ptr<DSMeter> dsMeter = m_Apartment.getDSMeterByDSID(dsuid);
      if(_request.getMethod() == "getName") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("name", dsMeter->getName());
        return success(resultObj);
      } else if(_request.getMethod() == "setName") {
        if(_request.hasParameter("newName")) {
          StringConverter st("UTF-8", "UTF-8");
          std::string nameStr = _request.getParameter("newName");
          nameStr = st.convert(nameStr);
          nameStr = escapeHTML(nameStr);
          dsMeter->setName(nameStr);
          if (m_pStructureBusInterface != NULL) {
            StructureManipulator manipulator(*m_pStructureBusInterface,
                                             *m_pStructureQueryBusInterface,
                                             m_Apartment);
            manipulator.meterSetName(dsMeter, nameStr);
          }
          return success();
        } else {
          return failure("missing parameter newName");
        }
      } else if(_request.getMethod() == "getConsumption") {
        if (!dsMeter->getCapability_HasMetering()) {
          return failure("Metering not supported on this device");
        }
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("consumption", dsMeter->getPowerConsumption());
        return success(resultObj);
      } else if(_request.getMethod() == "getEnergyMeterValue") {
        if (!dsMeter->getCapability_HasMetering()) {
          return failure("Metering not supported on this device");
        }
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
      return failure("Could not find dSMeter with given dSUID");
    }
  } // handleRequest

} // namespace dss
