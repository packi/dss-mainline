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

#include "structurerequesthandler.h"

#include "core/web/json.h"

#include "core/dss.h"
#include "core/DS485Interface.h"
#include "core/structuremanipulator.h"

#include "core/model/apartment.h"
#include "core/model/zone.h"
#include "core/model/set.h"
#include "core/model/device.h"
#include "core/model/devicereference.h"
#include "core/model/modelmaintenance.h"

namespace dss {


  //=========================================== StructureRequestHandler

  boost::shared_ptr<JSONObject> StructureRequestHandler::jsonHandleRequest(const RestfulRequest& _request, Session* _session) {
    StructureManipulator manipulator(*getDSS().getDS485Interface().getStructureModifyingBusInterface(), getDSS().getApartment());
    if(_request.getMethod() == "zoneAddDevice") {
      std::string devidStr = _request.getParameter("devid");
      if(!devidStr.empty()) {
        dsid_t devid = dsid::fromString(devidStr);

        Device& dev = DSS::getInstance()->getApartment().getDeviceByDSID(devid);
        if(!dev.isPresent()) {
          return failure("cannot add nonexisting device to a zone");
        }

        std::string zoneIDStr = _request.getParameter("zone");
        if(!zoneIDStr.empty()) {
          try {
            int zoneID = strToInt(zoneIDStr);
            DeviceReference devRef(dev, &DSS::getInstance()->getApartment());
            try {
              Zone& zone = getDSS().getApartment().getZone(zoneID);
              manipulator.addDeviceToZone(dev, zone);
            } catch(ItemNotFoundException&) {
              return failure("Could not find zone");
            }
          } catch(std::runtime_error& err) {
            return failure(err.what());
          }
        }
        return success();
      } else {
        return failure("Need parameter devid");
      }
    } else if(_request.getMethod() == "addZone") {
      int zoneID = -1;

      std::string zoneIDStr = _request.getParameter("zoneID");
      if(!zoneIDStr.empty()) {
        zoneID = strToIntDef(zoneIDStr, -1);
      }
      if(zoneID != -1) {
        getDSS().getApartment().allocateZone(zoneID);
      } else {
        return failure("could not find zone");
      }
      return success();
    } else if(_request.getMethod() == "removeZone") {
      int zoneID = -1;

      std::string zoneIDStr = _request.getParameter("zoneID");
      if(!zoneIDStr.empty()) {
        zoneID = strToIntDef(zoneIDStr, -1);
      }
      if(zoneID != -1) {
        try {
          Zone& zone = getDSS().getApartment().getZone(zoneID);
          if(zone.getFirstZoneOnDSMeter() != -1) {
            return failure("Cannot delete a primary zone");
          }
          if(zone.getDevices().length() > 0) {
            return failure("Cannot delete a non-empty zone");
          }
          getDSS().getApartment().removeZone(zoneID);
          getDSS().getModelMaintenance().addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
          return success();
        } catch(ItemNotFoundException&) {
          return failure("Could not find zone");
        }
      } else {
        return failure("Missing parameter zoneID");
      }
    } else {
      throw std::runtime_error("Unhandled function");
    }
  } // handleRequest


} // namespace dss
