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
#include <digitalSTROM/dsuid/dsuid.h>
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
#include "src/model/modelconst.h"
#include "src/stringconverter.h"
#include "src/ds485types.h"

#include "foreach.h"
#include "jsonhelper.h"
#include "foreach.h"
#include "util.h"

/* TODO use const in src/model/modelconst */
#define MIN_USER_GROUP_ID       24  // minimum allowed user group id
#define MAX_USER_GROUP_ID       31

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
    std::string dsuidStr = _request.getParameter("dsuid");

    if (dsuidStr.empty() && deviceIDStr.empty()) {
      return failure("Missing parameter 'dsuid'");
    }

    dsuid_t dsuid = dsidOrDsuid2dsuid(deviceIDStr, dsuidStr);
    
    boost::shared_ptr<Device> dev = DSS::getInstance()->getApartment().getDeviceByDSID(dsuid);
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
            dsuid_t next;
            dsuid_get_next_dsuid(dev->getDSID(), &next);
            try {
              boost::shared_ptr<Device> pPartnerDevice;
              pPartnerDevice = m_Apartment.getDeviceByDSID(next);
              manipulator.addDeviceToZone(pPartnerDevice, zone);
            } catch(std::runtime_error& e) {
              return failure("Could not find partner device with dsuid '" + dsuid2str(next) + "'");
            }
          } else if (dev->getOemInfoState() == DEVICE_OEM_VALID) {
            uint16_t serialNr = dev->getOemSerialNumber();
            if ((serialNr > 0) & !dev->getOemIsIndependent()) {
              std::vector<boost::shared_ptr<Device> > devices = m_Apartment.getDevicesVector();
              foreach (const boost::shared_ptr<Device>& device, devices) {
                if (dev->isOemCoupledWith(device)) {
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
        StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
        manipulator.removeZoneOnDSMeters(zone);

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
    std::string dsuidStr = _request.getParameter("dsuid");

    if (dsuidStr.empty() && deviceIDStr.empty()) {
      return failure("Missing parameter 'dsuid'");
    }

    dsuid_t dsuid = dsidOrDsuid2dsuid(deviceIDStr, dsuidStr);

    boost::shared_ptr<Device> dev = DSS::getInstance()->getApartment().getDeviceByDSID(dsuid);
    if(dev->isPresent()) {
      return failure("Cannot remove present device");
    }

    boost::shared_ptr<Device> pPartnerDevice;
    if (dev->is2WayMaster()) {
      dsuid_t next;
      dsuid_get_next_dsuid(dev->getDSID(), &next);
      try {
        pPartnerDevice = m_Apartment.getDeviceByDSID(next);
      } catch(ItemNotFoundException& e) {
        Logger::getInstance()->log("Could not find partner device with dsuid '" + dsuid2str(next) + "'");
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
    m_Apartment.removeDevice(dsuid);
    if (pPartnerDevice != NULL) {
      Logger::getInstance()->log("Also removing partner device " + dsuid2str(pPartnerDevice->getDSID()) + "'");
      m_Apartment.removeDevice(pPartnerDevice->getDSID());
    }
    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return success();
    
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

  boost::shared_ptr<JSONObject> StructureRequestHandler::removeGroup(const RestfulRequest& _request) {
    boost::shared_ptr<Zone> zone;
    int groupID = -1;
    int zoneID = -1;

    if (_request.hasParameter("zoneID")) {
      std::string zoneIDStr = _request.getParameter("zoneID");
      zoneID = strToIntDef(zoneIDStr, -1);
      zone = m_Apartment.getZone(zoneID);
    }
    if (zoneID < 0 || !zone) {
      return failure("Invalid value for parameter zoneID : '" + _request.getParameter("zoneID") + "'");
    }

    if (_request.hasParameter("groupID")) {
      std::string groupIDStr = _request.getParameter("groupID");
      groupID = strToIntDef(groupIDStr, -1);
    }
    if (groupID == -1) {
      return failure("Invalid groupID : '" + _request.getParameter("groupID") + "'");
    }

    if (zone->getGroup(groupID) == NULL) {
      return failure("Group with groupID : '" + _request.getParameter("groupID") + "' does not exist");
    }

    if (!zone->getGroup(groupID)->getDevices().isEmpty()) {
      return failure("Group with groupID : '" + _request.getParameter("groupID") + "' is not empty.");
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.removeGroup(zone, groupID);

    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return success();
  } // removeGroup

  boost::shared_ptr<JSONObject> StructureRequestHandler::addGroup(const RestfulRequest& _request) {
    boost::shared_ptr<Zone> zone;
    boost::shared_ptr<Group> pGroup;
    int groupID = -1;
    int zoneID = -1;
    int standardGroupID = 0;
    std::string groupName;

    if (_request.hasParameter("zoneID")) {
      std::string zoneIDStr = _request.getParameter("zoneID");
      zoneID = strToIntDef(zoneIDStr, -1);
      zone = m_Apartment.getZone(zoneID);
    }
    if (zoneID < 0 || !zone) {
      return failure("Parameter zoneID missing or empty: '" + _request.getParameter("zoneID") + "'");
    }

    if (_request.hasParameter("groupID")) {
      std::string groupIDStr = _request.getParameter("groupID");
      groupID = strToIntDef(groupIDStr, -1);
      if (groupID < 0) {
        return failure("Parameter GroupID '" + _request.getParameter("groupID") + "' is not valid");
      }
      if (zone->getGroup(groupID) != NULL) {
        return failure("Group with ID " + _request.getParameter("groupID") + " already exists");
      }
    }
    else if (_request.hasParameter("groupAutoSelect")) {
      std::string grType = _request.getParameter("groupAutoSelect");
      if (grType == "user") {
        groupID = -1;
      } else if (grType == "global") {
        groupID = -2;
      }
    } else {
      return failure("Parameter groupID or groupAutoSelect missing");
    }

    if (groupID == -1) {
      // find any free group slot
      for (groupID = MIN_USER_GROUP_ID; pGroup == NULL && groupID < MAX_USER_GROUP_ID; groupID ++) {
        pGroup = zone->getGroup(groupID);
      }
    } else if (groupID == -2) {
      if (zoneID != 0) {
        return failure("Apartment user groups only allowed in Zone 0");
      }
      // find a group slot with unassigned state machine id
      for (groupID = GroupIDAppUserMin; groupID <= GroupIDAppUserMax; groupID++) {
        pGroup = zone->getGroup(groupID);
        if (pGroup->getStandardGroupID() == 0) {
          break;
        }
      }
      if (pGroup->getStandardGroupID() > 0) {
        pGroup.reset();
      }
    }

    if (pGroup == NULL) {
      return failure("No free user groups");
    }

    if (_request.hasParameter("groupName")) {
      StringConverter st("UTF-8", "UTF-8");
      groupName = st.convert(_request.getParameter("groupName"));
      groupName = escapeHTML(groupName);
    }
    if (_request.hasParameter("groupColor")) {
      standardGroupID = strToIntDef(_request.getParameter("groupColor"), 0);
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.createGroup(zone, groupID, standardGroupID, groupName);

    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));

    boost::shared_ptr<JSONObject> resultObj(new JSONObject());
    resultObj->addProperty("groupID", groupID);
    resultObj->addProperty("zoneID", zoneID);
    resultObj->addProperty("groupName", groupName);
    resultObj->addProperty("groupColor", standardGroupID);
    return success(resultObj);
  } // addGroup

  boost::shared_ptr<JSONObject> StructureRequestHandler::groupAddDevice(const RestfulRequest& _request) {
    boost::shared_ptr<Device> dev;
    boost::shared_ptr<Group> gr;

    std::string deviceIDStr = _request.getParameter("deviceID");
    std::string dsuidStr = _request.getParameter("dsuid");

    if (dsuidStr.empty() && deviceIDStr.empty()) {
      return failure("Missing parameter 'dsuid'");
    }

    dsuid_t dsuid = dsidOrDsuid2dsuid(deviceIDStr, dsuidStr);
    
    dev = m_Apartment.getDeviceByDSID(dsuid);
    if(!dev->isPresent()) {
      return failure("Cannot modify inactive device");
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
    if(!gr || !gr->isValid()) {
      return failure("Invalid value for parameter groupID : '" + _request.getParameter("groupID") + "'");
    }

    if (!(dev->getGroupBitmask().test(gr->getStandardGroupID()-1) ||
          (dev->getDeviceType() == DEVICE_TYPE_AKM))) {
      return failure("Devices does not match color of group (" + intToString(gr->getStandardGroupID()) + ")");
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    if (((dev->getDeviceType() == DEVICE_TYPE_AKM) ||
         ((dev->getDeviceType() == DEVICE_TYPE_TKM) && !dev->hasOutput()))
        && (dev->getGroupsCount() > 1)) {
      for (int g = 0; g < dev->getGroupsCount(); g++) {
        boost::shared_ptr<Group> oldGroup = dev->getGroupByIndex(g);
        if ((oldGroup != NULL) && (oldGroup->getID() >= GroupIDAppUserMin) && (oldGroup->getID() <= GroupIDAppUserMax)) {
          manipulator.deviceRemoveFromGroup(dev, oldGroup);
        }
      }
    }
    manipulator.deviceAddToGroup(dev, gr);

    std::vector<boost::shared_ptr<Device> > modifiedDevices;
    modifiedDevices.push_back(dev);

    if (dev->is2WayMaster()) {
      dsuid_t next;
      dsuid_get_next_dsuid(dev->getDSID(), &next);
      try {
        boost::shared_ptr<Device> pPartnerDevice;

        // Remove slave/partner device from all other user groups
        pPartnerDevice = m_Apartment.getDeviceByDSID(next);
        for (int g = 0; g < pPartnerDevice->getGroupsCount(); g++) {
          boost::shared_ptr<Group> oldGroup = pPartnerDevice->getGroupByIndex(g);
          if ((oldGroup != NULL) && (oldGroup->getID() >= GroupIDAppUserMin) && (oldGroup->getID() <= GroupIDAppUserMax)) {
            manipulator.deviceRemoveFromGroup(pPartnerDevice, oldGroup);
          }
        }
        manipulator.deviceAddToGroup(pPartnerDevice, gr);

      } catch(std::runtime_error& e) {
        return failure("Could not find partner device with dSUID '" + dsuid2str(next) + "'");
      }
    }
    if (dev->getOemInfoState() == DEVICE_OEM_VALID) {
      uint16_t serialNr = dev->getOemSerialNumber();
      if ((serialNr > 0) & !dev->getOemIsIndependent()) {
        std::vector<boost::shared_ptr<Device> > devices = m_Apartment.getDevicesVector();
        foreach (const boost::shared_ptr<Device>& device, devices) {
          if (dev->isOemCoupledWith(device)) {
            // Remove coupled OEM devices from all other user groups as well
            for (int g = 0; g < device->getGroupsCount(); g++) {
              boost::shared_ptr<Group> oldGroup = device->getGroupByIndex(g);
              if ((oldGroup != NULL) && (oldGroup->getID() >= GroupIDAppUserMin) && (oldGroup->getID() <= GroupIDAppUserMax)) {
                manipulator.deviceRemoveFromGroup(device, oldGroup);
              }
            }
            manipulator.deviceAddToGroup(device, gr);
            modifiedDevices.push_back(device);
          }
        }
      }
    }

    boost::shared_ptr<JSONObject> resultObj(new JSONObject());
    if (!modifiedDevices.empty()) {
      boost::shared_ptr<JSONArrayBase> modified(new JSONArrayBase());
      foreach (const boost::shared_ptr<Device>& device, modifiedDevices) {
        const DeviceReference d(device, &m_Apartment);
        modified->addElement("", toJSON(d));
      }
      resultObj->addProperty("action", "update");
      resultObj->addElement("devices", modified);
    } else {
      resultObj->addProperty("action", "none");
    }

    return success(resultObj);
  } // groupAddDevice

  boost::shared_ptr<JSONObject> StructureRequestHandler::groupRemoveDevice(const RestfulRequest& _request) {
    boost::shared_ptr<Device> dev;
    boost::shared_ptr<Group> gr;

    std::string deviceIDStr = _request.getParameter("deviceID");
    std::string dsuidStr = _request.getParameter("dsuid");

    if (dsuidStr.empty() && deviceIDStr.empty()) {
      return failure("Missing parameter 'dsuid'");
    }

    dsuid_t dsuid = dsidOrDsuid2dsuid(deviceIDStr, dsuidStr);

    dev = m_Apartment.getDeviceByDSID(dsuid);
    if(!dev->isPresent()) {
      return failure("Cannot modify inactive device");
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
    if(!gr || !gr->isValid()) {
      return failure("Invalid value for parameter groupID : '" + _request.getParameter("groupID") + "'");
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.deviceRemoveFromGroup(dev, gr);

    std::vector<boost::shared_ptr<Device> > modifiedDevices;
    modifiedDevices.push_back(dev);

    if (dev->is2WayMaster()) {
      dsuid_t next;
      dsuid_get_next_dsuid(dev->getDSID(), &next);
      try {
        boost::shared_ptr<Device> pPartnerDevice;

        pPartnerDevice = m_Apartment.getDeviceByDSID(next);
        manipulator.deviceRemoveFromGroup(pPartnerDevice, gr);
      } catch(std::runtime_error& e) {
        return failure("Could not find partner device with dSUID '" + dsuid2str(next) + "'");
      }
    }
    if (dev->getOemInfoState() == DEVICE_OEM_VALID) {
      uint16_t serialNr = dev->getOemSerialNumber();
      if ((serialNr > 0) & !dev->getOemIsIndependent()) {
        std::vector<boost::shared_ptr<Device> > devices = m_Apartment.getDevicesVector();
        foreach (const boost::shared_ptr<Device>& device, devices) {
          if (dev->isOemCoupledWith(device)) {
            manipulator.deviceRemoveFromGroup(device, gr);
            modifiedDevices.push_back(device);
          }
        }
      }
    }

    boost::shared_ptr<JSONObject> resultObj(new JSONObject());
    if (!modifiedDevices.empty()) {
      boost::shared_ptr<JSONArrayBase> modified(new JSONArrayBase());
      foreach (const boost::shared_ptr<Device>& device, modifiedDevices) {
        const DeviceReference d(device, &m_Apartment);
        modified->addElement("", toJSON(d));
      }
      resultObj->addProperty("action", "update");
      resultObj->addElement("devices", modified);
    } else {
      resultObj->addProperty("action", "none");
    }

    return success(resultObj);
  } // groupRemoveDevice


  boost::shared_ptr<JSONObject> StructureRequestHandler::groupSetName(const RestfulRequest& _request) {
    StringConverter st("UTF-8", "UTF-8");
    boost::shared_ptr<Zone> zone;
    boost::shared_ptr<Group> group;
    int groupID = -1;
    int zoneID = -1;

    if(_request.hasParameter("zoneID")) {
      std::string zoneIDStr = _request.getParameter("zoneID");
      zoneID = strToIntDef(zoneIDStr, -1);
      zone = m_Apartment.getZone(zoneID);
    }
    if(zoneID < 0 || !zone) {
      return failure("Invalid value for parameter zoneID : '" + _request.getParameter("zoneID") + "'");
    }

    if(_request.hasParameter("groupID")) {
      std::string groupIDStr = _request.getParameter("groupID");
      groupID = strToIntDef(groupIDStr, -1);
    }

    group = zone->getGroup(groupID);
    if ((groupID < 1) || !group) {
      return failure("Could not find group with id : '" + _request.getParameter("groupID") + "'");
    }

    if (!_request.hasParameter("newName")) {
      return failure("missing parameter 'newName'");
    }

    std::string newName = st.convert(_request.getParameter("newName"));
    newName = escapeHTML(newName);

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.groupSetName(group, newName);

    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return success();
  }

  boost::shared_ptr<JSONObject> StructureRequestHandler::groupSetColor(const RestfulRequest& _request) {
    boost::shared_ptr<Zone> zone;
    boost::shared_ptr<Group> group;
    int groupID = -1;
    int zoneID = -1;

    if(_request.hasParameter("zoneID")) {
      std::string zoneIDStr = _request.getParameter("zoneID");
      zoneID = strToIntDef(zoneIDStr, -1);
      zone = m_Apartment.getZone(zoneID);
    }
    if(zoneID < 0 || !zone) {
      return failure("Invalid value for parameter zoneID : '" + _request.getParameter("zoneID") + "'");
    }

    if(_request.hasParameter("groupID")) {
      std::string groupIDStr = _request.getParameter("groupID");
      groupID = strToIntDef(groupIDStr, -1);
    }

    group = zone->getGroup(groupID);
    if ((groupID < 1) || !group) {
      return failure("Could not find group with id : '" + _request.getParameter("groupID") + "'");
    }

    if (!_request.hasParameter("newColor")) {
      return failure("missing parameter 'newColor'");
    }

    int newColor = strToIntDef(_request.getParameter("newColor"), -1);
    if (newColor < 0) {
      return failure("Invalid value for parameter newColor: '" + _request.getParameter("newColor") + "'");
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.groupSetStandardID(group, newColor);

    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return success();
  }

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
    } else if(_request.getMethod() == "removeGroup") {
      return removeGroup(_request);
    } else if(_request.getMethod() == "groupAddDevice") {
      return groupAddDevice(_request);
    } else if(_request.getMethod() == "groupRemoveDevice") {
      return groupRemoveDevice(_request);
    } else if(_request.getMethod() == "groupSetName") {
      return groupSetName(_request);
    } else if(_request.getMethod() == "groupSetColor") {
      return groupSetColor(_request);
    } else {
      throw std::runtime_error("Unhandled function");
    }
  } // handleRequest

} // namespace dss
