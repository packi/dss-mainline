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

#include "src/web/json.h"

#include "src/businterface.h"
#include "src/structuremanipulator.h"
#include "src/setbuilder.h"

#include "src/model/apartment.h"
#include "src/model/zone.h"
#include "src/model/group.h"
#include "src/model/set.h"
#include "src/model/device.h"
#include "src/model/modulator.h"
#include "src/model/devicereference.h"
#include "src/model/modelmaintenance.h"

#include "foreach.h"
#include "jsonhelper.h"

namespace dss {


  //=========================================== StructureRequestHandler

  StructureRequestHandler::StructureRequestHandler(Apartment& _apartment, ModelMaintenance& _modelMaintenance,
                                                   StructureModifyingBusInterface& _interface,
                                                   StructureQueryBusInterface& _queryInterface)
  : m_Apartment(_apartment),
    m_ModelMaintenance(_modelMaintenance),
    m_Interface(_interface),
    m_QueryInterface(_queryInterface)
  { }

  boost::shared_ptr<JSONObject> StructureRequestHandler::zoneAddDevice(const RestfulRequest& _request) {
    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    std::string deviceIDStr = _request.getParameter("deviceID");
    if(!deviceIDStr.empty()) {
      dss_dsid_t deviceID = dsid::fromString(deviceIDStr);

      boost::shared_ptr<Device> dev = DSS::getInstance()->getApartment().getDeviceByDSID(deviceID);
      if(!dev->isPresent()) {
        return failure("cannot add nonexisting device to a zone");
      }

      std::string zoneIDStr = _request.getParameter("zone");
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());

      if(!zoneIDStr.empty()) {
        try {
          int zoneID = strToInt(zoneIDStr);
          if (zoneID < 1) {
            return failure("Could not move device: invalid zone id given!");
          }
          DeviceReference devRef(dev, &DSS::getInstance()->getApartment());
          if (dev->getZoneID() == zoneID) {
            return failure("Device is already in zone " + zoneIDStr);
          }
          try {
            std::vector<boost::shared_ptr<Device> > movedDevices;
            boost::shared_ptr<Zone> zone = m_Apartment.getZone(zoneID);
            manipulator.addDeviceToZone(dev, zone);
            movedDevices.push_back(dev);
            if (dev->is2WayMaster()) {
              dss_dsid_t next = dev->getDSID();
              next.lower++;
              try {
                boost::shared_ptr<Device> pPartnerDevice;

                pPartnerDevice = m_Apartment.getDeviceByDSID(next);
                manipulator.addDeviceToZone(pPartnerDevice, zone);
              } catch(std::runtime_error& e) {
                return failure("Could not find partner device with dsid '" + next.toString() + "'");
              }
            } else if (dev->getOemInfoState() == DEVICE_OEM_VALID) {
              uint16_t serialNr = dev->getOemSerialNumber();
              if (serialNr > 0) {
                unsigned long long ean = dev->getOemEan();
                dss_dsid_t dsmId = dev->getDSMeterDSID();
                std::vector<boost::shared_ptr<Device> > devices = DSS::getInstance()->getApartment().getDevicesVector();
                foreach (const boost::shared_ptr<Device>& device, devices) {
                  if ((device->getDSID() != deviceID) &&
                      device->isPresent() &&
                      (device->getDSMeterDSID() == dsmId) &&
                      (device->getOemInfoState() == DEVICE_OEM_VALID) &&
                      (device->getOemEan() == ean) &&
                      (device->getOemSerialNumber() == serialNr)) {
                    manipulator.addDeviceToZone(device, zone);
                    movedDevices.push_back(device);
                  }
                }
              }
            }

            boost::shared_ptr<JSONArrayBase> moved(new JSONArrayBase());
            foreach (const boost::shared_ptr<Device>& device, movedDevices) {
              const DeviceReference d(device, &m_Apartment);
              moved->addElement("", toJSON(d));
            }
            resultObj->addElement("movedDevices", moved);
          } catch(ItemNotFoundException&) {
            return failure("Could not find zone");
          }
        } catch(std::runtime_error& err) {
          return failure(err.what());
        }
      } else {
        return failure("Need parameter 'zone'");
      }
      return success(resultObj);
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
        boost::shared_ptr<Zone> zone = m_Apartment.getZone(zoneID);
        if(zone->getDevices().length() > 0) {
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
      dss_dsid_t deviceID = dsid::fromString(deviceIDStr);

      boost::shared_ptr<Device> dev = DSS::getInstance()->getApartment().getDeviceByDSID(deviceID);
      if(dev->isPresent()) {
        return failure("Cannot remove present device");
      }

      boost::shared_ptr<Device> pPartnerDevice;
      if (dev->is2WayMaster()) {
        dss_dsid_t next = dev->getDSID();
        next.lower++;
        try {
          pPartnerDevice = m_Apartment.getDeviceByDSID(next);
        } catch(std::runtime_error& e) {
          Logger::getInstance()->log("Could not find partner device with dsid '" + next.toString() + "'");
        }
      }
      StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
      try {
        manipulator.removeDeviceFromDSMeter(dev);
        if (pPartnerDevice != NULL) {
         manipulator.removeDeviceFromDSMeter(pPartnerDevice);
        }
      } catch (std::runtime_error& e) {
        Logger::getInstance()->log(std::string("Could not remove device from "
                                   "dSM: ") + e.what(), lsError);
      }
      m_Apartment.removeDevice(deviceID);
      if (pPartnerDevice != NULL) {
        Logger::getInstance()->log("Also removing partner device " + pPartnerDevice->getDSID().toString() + "'");
        m_Apartment.removeDevice(pPartnerDevice->getDSID());
      }
      m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
      return success();
    }

    return failure("Missing deviceID");
  }

  boost::shared_ptr<JSONObject> StructureRequestHandler::persistSet(const RestfulRequest& _request) {
    std::string setStr = _request.getParameter("set");
    bool hasGroupID = _request.hasParameter("groupID");
    int groupID = -1;
    if(hasGroupID) {
      std::string groupIDStr = _request.getParameter("groupID");
      groupID = strToIntDef(groupIDStr, -1);
      if(groupID == -1) {
        return failure("Invalid value for parameter groupID : '" + groupIDStr + "'");
      }
    }
    SetBuilder builder(m_Apartment);
    Set set = builder.buildSet(setStr, boost::shared_ptr<Zone>());
    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    if(hasGroupID) {
      manipulator.persistSet(set, setStr, groupID);
    } else {
      groupID = manipulator.persistSet(set, setStr);
    }
    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));

    boost::shared_ptr<JSONObject> resultObj(new JSONObject());
    resultObj->addProperty("groupID", groupID);
    return success(resultObj);
  } // persistSet

  boost::shared_ptr<JSONObject> StructureRequestHandler::unpersistSet(const RestfulRequest& _request) {
    if(_request.hasParameter("set")) {
      std::string setStr = _request.getParameter("set");
      if(!setStr.empty()) {
        StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
        manipulator.unpersistSet(setStr);
        m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
        return success();
      } else {
        return failure("Need non-empty parameter 'set'");
      }
    } else {
      return failure("Need parameter 'set'");
    }
  } // persistSet

  boost::shared_ptr<JSONObject> StructureRequestHandler::addGroup(const RestfulRequest& _request) {
    boost::shared_ptr<Zone> zone;
    int groupID = -1;
    int zoneID = -1;

    if(_request.hasParameter("zoneID")) {
      std::string zoneIDStr = _request.getParameter("zoneID");
      zoneID = strToIntDef(zoneIDStr, -1);
      zone = m_Apartment.getZone(zoneID);
    }
    if(zoneID < 1 || !zone) {
      return failure("Invalid value for parameter zoneID : '" + _request.getParameter("zoneID") + "'");
    }

    if(_request.hasParameter("groupID")) {
      std::string groupIDStr = _request.getParameter("groupID");
      groupID = strToIntDef(groupIDStr, -1);
      if(zone->getGroup(groupID)) {
        return failure("Group with groupID : '" + _request.getParameter("groupID") + "' already exists");
      }
    }
    if(groupID == -1) {
      return failure("Invalid value for parameter groupID : '" + _request.getParameter("groupID") + "'");
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.createGroup(zone, groupID);

    return success();
  } // addGroup

  boost::shared_ptr<JSONObject> StructureRequestHandler::groupAddDevice(const RestfulRequest& _request) {
    boost::shared_ptr<Device> dev;
    boost::shared_ptr<Group> gr;

    if(_request.hasParameter("deviceID")) {
      std::string deviceIDStr = _request.getParameter("deviceID");
      dss_dsid_t deviceID = dsid::fromString(deviceIDStr);
      dev = m_Apartment.getDeviceByDSID(deviceID);
      if(!dev->isPresent()) {
        return failure("Cannot modify inactive device");
      }
    }
    if(!dev) {
      return failure("Invalid value for parameter deviceID : '" + _request.getParameter("deviceID") + "'");
    }

    if(_request.hasParameter("groupID")) {
      std::string groupIDStr = _request.getParameter("groupID");
      int groupID = strToIntDef(groupIDStr, -1);
      try {
        boost::shared_ptr<Zone> zone = m_Apartment.getZone(dev->getZoneID());
        gr = zone->getGroup(groupID);
      } catch(ItemNotFoundException& e) {
        gr = boost::shared_ptr<Group> ();
      }
    }
    if(!gr) {
      return failure("Invalid value for parameter groupID : '" + _request.getParameter("groupID") + "'");
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.deviceAddToGroup(dev, gr);

    return success();
  } // groupAddDevice

  boost::shared_ptr<JSONObject> StructureRequestHandler::groupRemoveDevice(const RestfulRequest& _request) {
    boost::shared_ptr<Device> dev;
    boost::shared_ptr<Group> gr;

    if(_request.hasParameter("deviceID")) {
      std::string deviceIDStr = _request.getParameter("deviceID");
      dss_dsid_t deviceID = dsid::fromString(deviceIDStr);
      dev = m_Apartment.getDeviceByDSID(deviceID);
      if(!dev->isPresent()) {
        return failure("Cannot modify inactive device");
      }
    }
    if(!dev) {
      return failure("Invalid value for parameter deviceID : '" + _request.getParameter("deviceID") + "'");
    }

    if(_request.hasParameter("groupID")) {
      std::string groupIDStr = _request.getParameter("groupID");
      int groupID = strToIntDef(groupIDStr, -1);
      try {
        boost::shared_ptr<Zone> zone = m_Apartment.getZone(dev->getZoneID());
        gr = zone->getGroup(groupID);
      } catch(ItemNotFoundException& e) {
        gr = boost::shared_ptr<Group> ();
      }
    }
    if(!gr) {
      return failure("Invalid value for parameter groupID : '" + _request.getParameter("groupID") + "'");
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.deviceRemoveFromGroup(dev, gr);

    return success();
  } // groupRemoveDevice

  WebServerResponse StructureRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    if(_request.getMethod() == "zoneAddDevice") {
      return zoneAddDevice(_request);
    } else if(_request.getMethod() == "removeDevice") {
      return removeDevice(_request);
    } else if(_request.getMethod() == "addZone") {
      return addZone(_request);
    } else if(_request.getMethod() == "removeZone") {
      return removeZone(_request);
    } else if(_request.getMethod() == "persistSet") {
      return persistSet(_request);
    } else if(_request.getMethod() == "unpersistSet") {
      return unpersistSet(_request);
    } else if(_request.getMethod() == "addGroup") {
      return addGroup(_request);
    } else if(_request.getMethod() == "groupAddDevice") {
      return groupAddDevice(_request);
    } else if(_request.getMethod() == "groupRemoveDevice") {
      return groupRemoveDevice(_request);
    } else {
      throw std::runtime_error("Unhandled function");
    }
  } // handleRequest

} // namespace dss
