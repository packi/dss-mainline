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
  
  StructureRequestHandler::StructureRequestHandler(Apartment& _apartment, ModelMaintenance& _modelMaintenance, StructureModifyingBusInterface& _interface)
  : m_Apartment(_apartment),
    m_ModelMaintenance(_modelMaintenance),
    m_Interface(_interface)
  { }

  boost::shared_ptr<JSONObject> StructureRequestHandler::zoneAddDevice(const RestfulRequest& _request) {
    StructureManipulator manipulator(m_Interface, m_Apartment);
    std::string deviceIDStr = _request.getParameter("deviceID");
    if(!deviceIDStr.empty()) {
      dsid_t deviceID = dsid::fromString(deviceIDStr);

      Device& dev = DSS::getInstance()->getApartment().getDeviceByDSID(deviceID);
      if(!dev.isPresent()) {
        return failure("cannot add nonexisting device to a zone");
      }

      std::string zoneIDStr = _request.getParameter("zone");
      if(!zoneIDStr.empty()) {
        try {
          int zoneID = strToInt(zoneIDStr);
          DeviceReference devRef(dev, &DSS::getInstance()->getApartment());
          try {
            Zone& zone = m_Apartment.getZone(zoneID);
            manipulator.addDeviceToZone(dev, zone);
          } catch(ItemNotFoundException&) {
            return failure("Could not find zone");
          }
        } catch(std::runtime_error& err) {
          return failure(err.what());
        }
      }
      return success();
    }

    return failure("Need parameter deviceID");
  }

  boost::shared_ptr<JSONObject> StructureRequestHandler::addZone(const RestfulRequest& _request) {
    int zoneID = -1;

    std::string zoneIDStr = _request.getParameter("zoneID");
    if(!zoneIDStr.empty()) {
      zoneID = strToIntDef(zoneIDStr, -1);
    }
    if(zoneID != -1) {
      m_Apartment.allocateZone(zoneID);
    } else {
      return failure("No or invalid zone id specified");
    }
    return success();
  }

  boost::shared_ptr<JSONObject> StructureRequestHandler::removeZone(const RestfulRequest& _request) {
    int zoneID = -1;

    std::string zoneIDStr = _request.getParameter("zoneID");
    if(!zoneIDStr.empty()) {
      zoneID = strToIntDef(zoneIDStr, -1);
    }
    if(zoneID != -1) {
      try {
        Zone& zone = m_Apartment.getZone(zoneID);
        if(zone.getFirstZoneOnDSMeter() != -1) {
          return failure("Cannot delete a primary zone");
        }
        if(zone.getDevices().length() > 0) {
          return failure("Cannot delete a non-empty zone");
        }
        m_Apartment.removeZone(zoneID);
        m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
        return success();
      } catch(ItemNotFoundException&) {
        return failure("Could not find zone");
      }
    }
  
    return failure("Missing parameter zoneID");
  }

  boost::shared_ptr<JSONObject> StructureRequestHandler::removeDevice(const RestfulRequest& _request) {
    std::string deviceIDStr = _request.getParameter("deviceID");
    if(!deviceIDStr.empty()) {
      dsid_t deviceID = dsid::fromString(deviceIDStr);

      Device& dev = DSS::getInstance()->getApartment().getDeviceByDSID(deviceID);
      if(dev.isPresent()) {
        return failure("Cannot remove present device");
      }
     
      m_Apartment.removeDevice(deviceID);
      m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
      return success();
    }
    
    return failure("Missing deviceID");
  }

  boost::shared_ptr<JSONObject> StructureRequestHandler::removeInactiveDevices(const RestfulRequest& _request) {
    StructureManipulator manipulator(m_Interface, m_Apartment);
    std::string id = _request.getParameter("id");
    if(id.empty()) {
      return failure("Missing parameter id");
    }

    dsid_t dID = dsid::fromString(id);

    DSMeter& dsMeter = m_Apartment.getDSMeterByDSID(dID);
    
    manipulator.removeInactiveDevices(dsMeter);

    Set all = m_Apartment.getDevices();
    
    if(all.length() <= 0) {
      return success();
    }

    Set byMeter = all.getByDSMeter(dsMeter);
    if (byMeter.length() <= 0) {
      return success();
    }

    Set notPresent = byMeter.getByPresence(false);
    if(notPresent.length() <= 0) {
      return success();
    }

    for(int i = 0; i < notPresent.length(); i++) {
      const DeviceReference& d = notPresent.get(i);
      m_Apartment.removeDevice(d.getDSID());
    }

    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));

    return success();
  }

  boost::shared_ptr<JSONObject> StructureRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    if(_request.getMethod() == "zoneAddDevice") {
      return zoneAddDevice(_request);
    } else if(_request.getMethod() == "removeDevice") {
      return removeDevice(_request);
    } else if(_request.getMethod() == "addZone") {
      return addZone(_request);
    } else if(_request.getMethod() == "removeZone") {
      return removeZone(_request);
    } else if (_request.getMethod() == "removeInactiveDevices") {
      return removeInactiveDevices(_request);
    } else {
      throw std::runtime_error("Unhandled function");
    }
  } // handleRequest

} // namespace dss
