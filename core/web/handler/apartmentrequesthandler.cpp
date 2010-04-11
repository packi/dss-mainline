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

#include "apartmentrequesthandler.h"

#include "core/foreach.h"
#include "core/ds485const.h"
#include "core/model/modelconst.h"

#include "core/web/json.h"

#include "core/model/apartment.h"
#include "core/model/zone.h"
#include "core/model/modulator.h"
#include "core/model/device.h"
#include "core/model/deviceinterface.h"
#include "core/model/group.h"
#include "core/model/set.h"

#include "jsonhelper.h"

namespace dss {

  //=========================================== ApartmentRequestHandler

  ApartmentRequestHandler::ApartmentRequestHandler(Apartment& _apartment)
  : m_Apartment(_apartment)
  { }

  boost::shared_ptr<JSONObject> ApartmentRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session>& _session) {
    std::string errorMessage;
    if(_request.getMethod() == "getConsumption") {
      int accumulatedConsumption = 0;
      foreach(DSMeter* pDSMeter, m_Apartment.getDSMeters()) {
        accumulatedConsumption += pDSMeter->getPowerConsumption();
      }
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("consumption", accumulatedConsumption);
      return success(resultObj);
    } else if(isDeviceInterfaceCall(_request)) {
      IDeviceInterface* interface = NULL;
      std::string groupName = _request.getParameter("groupName");
      std::string groupIDString = _request.getParameter("groupID");
      if(!groupName.empty()) {
        try {
          Group& grp = m_Apartment.getGroup(groupName);
          interface = &grp;
        } catch(std::runtime_error& e) {
          return failure("Could not find group with name '" + groupName + "'");
        }
      } else if(!groupIDString.empty()) {
        try {
          int groupID = strToIntDef(groupIDString, -1);
          if(groupID != -1) {
            Group& grp = m_Apartment.getGroup(groupID);
            interface = &grp;
          } else {
            return failure("Could not parse group id '" + groupIDString + "'");
          }
        } catch(std::runtime_error& e) {
          return failure("Could not find group with ID '" + groupIDString + "'");
        }
      }
      if(interface == NULL) {
        interface = &m_Apartment.getGroup(GroupIDBroadcast);
      }
    
      return handleDeviceInterfaceRequest(_request, interface);

    } else {
      if(_request.getMethod() == "getStructure") {
        return success(toJSON(m_Apartment));
      } else if(_request.getMethod() == "getDevices") {
        Set devices;
        if(_request.getParameter("unassigned").empty()) {
          devices = m_Apartment.getDevices();
        } else {
          devices = getUnassignedDevices();
        }

        return success(toJSON(devices));
/* TODO: re-enable
      } else if(_request.getMethod() == "login") {
        int token = m_LastSessionID;
        m_Sessions[token] = Session(token);
        return "{" + toJSON("token") + ": " + toJSON(token) + "}";
*/
      } else if(_request.getMethod() == "getCircuits") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        boost::shared_ptr<JSONArrayBase> circuits(new JSONArrayBase());

        resultObj->addElement("circuits", circuits);
        std::vector<DSMeter*>& dsMeters = m_Apartment.getDSMeters();
        foreach(DSMeter* dsMeter, dsMeters) {
          boost::shared_ptr<JSONObject> circuit(new JSONObject());
          circuits->addElement("", circuit);
          circuit->addProperty("name", dsMeter->getName());
          circuit->addProperty("dsid", dsMeter->getDSID().toString());
          circuit->addProperty("busid", dsMeter->getBusID());
          circuit->addProperty("hwVersion", dsMeter->getHardwareVersion());
          circuit->addProperty("swVersion", dsMeter->getSoftwareVersion());
          circuit->addProperty("hwName", dsMeter->getHardwareName());
          circuit->addProperty("deviceType", dsMeter->getDeviceType());
          circuit->addProperty("isPresent", dsMeter->isPresent());
        }
        return success(resultObj);
      } else if(_request.getMethod() == "getName") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("name", m_Apartment.getName());
        return success(resultObj);
      } else if(_request.getMethod() == "setName") {
        m_Apartment.setName(_request.getParameter("newName"));
        return success();
      } else if(_request.getMethod() == "rescan") {
        std::vector<DSMeter*> mods = m_Apartment.getDSMeters();
        foreach(DSMeter* pDSMeter, mods) {
          pDSMeter->setIsValid(false);
        }
        return success();
      } else {
        throw std::runtime_error("Unhandled function");
      }
    }
  } // handleApartmentCall

  Set ApartmentRequestHandler::getUnassignedDevices() {
    Apartment& apt = DSS::getInstance()->getApartment();
    Set devices = apt.getZone(0).getDevices();

    std::vector<Zone*>& zones = apt.getZones();
    for(std::vector<Zone*>::iterator ipZone = zones.begin(), e = zones.end();
        ipZone != e; ++ipZone)
    {
      Zone* pZone = *ipZone;
      if(pZone->getID() == 0) {
        // zone 0 holds all devices, so we're going to skip it
        continue;
      }

      devices = devices.remove(pZone->getDevices());
    }

    return devices;
  } // getUnassignedDevices



} // namespace dss
