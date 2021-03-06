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

#include "structurerequesthandler.h"

#include <unistd.h>
#include "foreach.h"
#include <digitalSTROM/dsuid.h>
#include <ds/str.h>

#include "src/businterface.h"
#include "src/ds485types.h"
#include "jsonhelper.h"
#include "src/model/apartment.h"
#include "src/model/zone.h"
#include "src/model/group.h"
#include "src/model/cluster.h"
#include "src/model/set.h"
#include "src/model/device.h"
#include "src/model/modulator.h"
#include "src/model/devicereference.h"
#include "src/model/modelmaintenance.h"
#include "src/model/modelconst.h"
#include "src/setbuilder.h"
#include "src/stringconverter.h"
#include "src/structuremanipulator.h"
#include "util.h"
#include "event.h"
#include "event/event_create.h"

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

  std::string StructureRequestHandler::zoneAddDevice(const RestfulRequest& _request) {
    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    std::string deviceIDStr = _request.getParameter("deviceID");
    std::string dsuidStr = _request.getParameter("dsuid");

    if (dsuidStr.empty() && deviceIDStr.empty()) {
      return JSONWriter::failure("Missing parameter 'dsuid'");
    }

    dsuid_t dsuid = dsidOrDsuid2dsuid(deviceIDStr, dsuidStr);

    boost::shared_ptr<Device> dev = DSS::getInstance()->getApartment().getDeviceByDSID(dsuid);
    if(!dev->isPresent()) {
      return JSONWriter::failure("cannot add nonexisting device to a zone");
    }

    std::string zoneIDStr = _request.getParameter("zone");
    JSONWriter json;

    if(!zoneIDStr.empty()) {
      try {
        int zoneID = strToInt(zoneIDStr);
        if (zoneID < 1) {
          return JSONWriter::failure("Could not move device: invalid zone id given!");
        }
        DeviceReference devRef(dev, &DSS::getInstance()->getApartment());
        if (dev->getZoneID() == zoneID) {
          return JSONWriter::failure("Device is already in zone " + zoneIDStr);
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
              return JSONWriter::failure("Could not find partner device with dsuid '" + dsuid2str(next) + "'");
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
          } else if (dev->isMainDevice() && (dev->getPairedDevices() > 0)) {
            bool doSleep = false;
            dsuid_t next;
            dsuid_t current = dev->getDSID();
            for (int p = 0; p < dev->getPairedDevices(); p++) {
              dsuid_get_next_dsuid(current, &next);
              current = next;
              try {
                boost::shared_ptr<Device> pPartnerDevice;
                pPartnerDevice = m_Apartment.getDeviceByDSID(next);
                if (!pPartnerDevice->isVisible()) {
                  if (doSleep) {
                    usleep(500 * 1000); // 500ms
                  }
                  manipulator.addDeviceToZone(pPartnerDevice, zone);
                }
              } catch(std::runtime_error& e) {
                Logger::getInstance()->log("Could not find partner device with dsuid '" + dsuid2str(next) + "'");
              }
            }
          }

          json.startArray("movedDevices");
          foreach (const boost::shared_ptr<Device>& device, movedDevices) {
            const DeviceReference d(device, &m_Apartment);
            toJSON(d, json);
          }
          json.endArray();
        } catch(ItemNotFoundException&) {
          return JSONWriter::failure("Could not find zone");
        }
      } catch(std::runtime_error& err) {
        return JSONWriter::failure(err.what());
      }
    } else {
      return JSONWriter::failure("Need parameter 'zone'");
    }
    return json.successJSON();
  }

  std::string StructureRequestHandler::addZone(const RestfulRequest& _request) {
    int zoneID = -1;

    std::string zoneIDStr = _request.getParameter("zoneID");
    if(!zoneIDStr.empty()) {
      zoneID = strToIntDef(zoneIDStr, -1);
    }
    if(zoneID != -1) {
      m_Apartment.allocateZone(zoneID);
    } else {
      return JSONWriter::failure("No or invalid zone id specified");
    }
    return JSONWriter::success("");
  }

  std::string StructureRequestHandler::removeZone(const RestfulRequest& _request) {
    int zoneID = -1;

    std::string zoneIDStr = _request.getParameter("zoneID");
    if(!zoneIDStr.empty()) {
      zoneID = strToIntDef(zoneIDStr, -1);
    }
    if(zoneID != -1) {
      try {
        StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
        boost::shared_ptr<Zone> zone = m_Apartment.getZone(zoneID);

        // #14823 don't remove zones that have connected devices
        std::vector<boost::shared_ptr<Group> > groups = zone->getGroups();
        for (size_t i = 0; i < groups.size(); i++) {
          boost::shared_ptr<Group> group = groups.at(i);
          if (group->hasConnectedDevices()) {
            return JSONWriter::failure("Cannot delete a non-empty zone");
          }
        }

        Set set = zone->getDevices();
        if(set.length() > 0) {
          Set toMove;
          Set toErase;
          int i;

          // check if the zone "appears" empty in the UI
          for (i = 0; i < set.length(); i++) {
            const DeviceReference& d = set.get(i);

            if (!d.getDevice()->isVisible()) {
              if (d.getDevice()->isPresent()) {
                // invisible and not present devices are not rendered,
                // the only possibility here are multi-dsid devices that are
                // hidden
                // this also means, that if we get a "main" device, then
                // something is really wrong
                if (d.getDevice()->isMainDevice()) {
                  return JSONWriter::failure("Cannot delete a non-empty zone");
                }
                toMove.addDevice(d);
              } else {
                // invisible and not present devices can be erased
                toErase.addDevice(d);
              }
            } else {
              // if there are visible devices in the zone, removal is anyway
              // forbidden
              return JSONWriter::failure("Cannot delete a non-empty zone");
            }
          }

          // not sure if this can ever happen since we actually try to sync
          // the slaves when moving devices around
          for (i = 0; i < toMove.length(); i++) {
            const DeviceReference& d = set.get(i);
            dsuid_t current = d.getDevice()->getDSID();
            dsuid_t master = d.getDevice()->getMainDeviceDSUID();
            if (dsuid_equal(&current, &master)) {
              // this should never happen
              return JSONWriter::failure("Cannot delete a non-empty zone");
            }

            boost::shared_ptr<Device> md = m_Apartment.getDeviceByDSID(master);
            boost::shared_ptr<Zone> z = m_Apartment.getZone(md->getZoneID());
            boost::shared_ptr<Device> dev = m_Apartment.getDeviceByDSID(current);
            manipulator.addDeviceToZone(dev, zone);
          }

          for (i = 0; i < toErase.length(); i++) {
            const DeviceReference& d = set.get(i);
            dsuid_t id = d.getDevice()->getDSID();
            boost::shared_ptr<Device> dev = m_Apartment.getDeviceByDSID(id);
            manipulator.removeDevice(dev);
          }
          if (toErase.length() > 0) {
            m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etClusterCleanup));
          }

          // safeguard: if there is somehow still something left in the zone,
          // we can't remove it
          if (zone->getDevices().length() > 0) {
            return JSONWriter::failure("Cannot delete a non-empty zone");
          }
        }
        manipulator.removeZoneOnDSMeters(zone);

        m_Apartment.removeZone(zoneID);
        m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
        return JSONWriter::success("");
      } catch(ItemNotFoundException&) {
        return JSONWriter::failure("Could not find zone");
      }
    }

    return JSONWriter::failure("Missing parameter zoneID");
  }

  std::string StructureRequestHandler::removeDevice(const RestfulRequest& _request) {
    std::string deviceIDStr = _request.getParameter("deviceID");
    std::string dsuidStr = _request.getParameter("dsuid");

    if (dsuidStr.empty() && deviceIDStr.empty()) {
      return JSONWriter::failure("Missing parameter 'dsuid'");
    }

    dsuid_t dsuid = dsidOrDsuid2dsuid(deviceIDStr, dsuidStr);

    boost::shared_ptr<Device> dev;
    
    try {
      dev = DSS::getInstance()->getApartment().getDeviceByDSID(dsuid);
    } catch (ItemNotFoundException &infe) {
      Logger::getInstance()->log(std::string("Device not removed: ") + infe.what(), lsWarning);
      JSONWriter json;
      json.startArray("devices");
      json.startObject();
      json.add("id", dsuid);
      json.add("dSUID", dsuid);
      json.add("DisplayID", dsuid2str(dsuid));
      json.endObject();
      json.endArray();
      json.add("action", "remove");
      return json.successJSON();
    }

    if (dev->isPresent()) {
      // ATTENTION: this is string is translated by the web UI
      return JSONWriter::failure("Cannot remove present device");
    }

    std::vector<boost::shared_ptr<Device> > result;
    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    try {
      result = manipulator.removeDevice(dev);
    } catch (std::runtime_error& e) {
      Logger::getInstance()->log(std::string("Could not remove device: ") + e.what(), lsError);
      return JSONWriter::failure("Could not remove device");
    }

    // model dirty is called implicitly
    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etClusterCleanup));

    // the devices in the returned list are no longer present in the apartment!
    JSONWriter json;
    json.startArray("devices");
    foreach(boost::shared_ptr<Device> pDev, result) {
      json.startObject();
      dsid_t dsid;
      if (dsuid_to_dsid(pDev->getDSID(), &dsid)) {
        json.add("id", dsid2str(dsid));
      } else {
        json.add("id", "");
      }
      json.add("dSUID", pDev->getDSID());
      json.add("DisplayID", pDev->getDisplayID());
      json.endObject();
    }
    json.endArray();
    json.add("action", "remove");
    return json.successJSON();
  }

  std::string StructureRequestHandler::persistSet(const RestfulRequest& _request) {
    std::string setStr = _request.getParameter("set");
    bool hasGroupID = _request.hasParameter("groupID");
    int groupID = -1;
    if(hasGroupID) {
      std::string groupIDStr = _request.getParameter("groupID");
      groupID = strToIntDef(groupIDStr, -1);
      if(groupID == -1) {
        return JSONWriter::failure("Invalid value for parameter groupID : '" + groupIDStr + "'");
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

    JSONWriter json;
    json.add("groupID", groupID);
    return json.successJSON();
  } // persistSet

  std::string StructureRequestHandler::unpersistSet(const RestfulRequest& _request) {
    if(_request.hasParameter("set")) {
      std::string setStr = _request.getParameter("set");
      if(!setStr.empty()) {
        StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
        manipulator.unpersistSet(setStr);
        m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
        return JSONWriter::success();
      } else {
        return JSONWriter::failure("Need non-empty parameter 'set'");
      }
    } else {
      return JSONWriter::failure("Need parameter 'set'");
    }
  } // persistSet

  std::string StructureRequestHandler::removeGroup(const RestfulRequest& _request) {
    boost::shared_ptr<Zone> zone;
    int groupID = -1;
    int zoneID = -1;

    if (_request.hasParameter("zoneID")) {
      std::string zoneIDStr = _request.getParameter("zoneID");
      zoneID = strToIntDef(zoneIDStr, -1);
      zone = m_Apartment.getZone(zoneID);
    }
    if (zoneID < 0 || !zone) {
      return JSONWriter::failure("Invalid value for parameter zoneID : '" + _request.getParameter("zoneID") + "'");
    }

    if (_request.hasParameter("groupID")) {
      std::string groupIDStr = _request.getParameter("groupID");
      groupID = strToIntDef(groupIDStr, -1);
    }
    if (groupID == -1) {
      return JSONWriter::failure("Invalid groupID : '" + _request.getParameter("groupID") + "'");
    }

    if (zone->getGroup(groupID) == NULL) {
      return JSONWriter::failure("Group with groupID : '" + _request.getParameter("groupID") + "' does not exist");
    }

    if (!zone->getGroup(groupID)->getDevices().isEmpty()) {
      return JSONWriter::failure("Group with groupID : '" + _request.getParameter("groupID") + "' is not empty.");
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.removeGroup(zone, groupID);

    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return JSONWriter::success();
  } // removeGroup

  std::string StructureRequestHandler::removeCluster(const RestfulRequest& _request) {
    boost::shared_ptr<Zone> zone;
    int clusterID = -1;

    if (!_request.getParameter<int>("clusterID", clusterID)) {
      return JSONWriter::failure("Invalid clusterID : '" + _request.getParameter("clusterID") + "'");
    }

    if (m_Apartment.getCluster(clusterID) == NULL) {
      return JSONWriter::failure("Group with clusterID : '" + _request.getParameter("clusterID") + "' does not exist");
    }

    if (!m_Apartment.getCluster(clusterID)->getDevices().isEmpty()) {
      return JSONWriter::failure("Group with clusterID : '" + _request.getParameter("clusterID") + "' is not empty.");
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.removeGroup(m_Apartment.getZone(0), clusterID);

    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return JSONWriter::success();
  } // removeCluster

  std::string StructureRequestHandler::addCluster(const RestfulRequest& _request) {
    boost::shared_ptr<Cluster> pCluster;
    int applicationType = 0;
    std::string clusterName;

    // find a group slot with unassigned state machine id
    pCluster = m_Apartment.getEmptyCluster();

    if (pCluster == NULL) {
      return JSONWriter::failure("No free user groups");
    }

    if (_request.hasParameter("name")) {
      StringConverter st("UTF-8", "UTF-8");
      clusterName = st.convert(_request.getParameter("name"));
      clusterName = escapeHTML(clusterName);
    }

    if (_request.hasParameter("applicationType")) {
      applicationType  = strToIntDef(_request.getParameter("applicationType"), 0);
    } else if (_request.hasParameter("color")) {
      Logger::getInstance()->log("addCluster parameter \"color\" is deprecated, use \"applicationType\"", lsWarning);
      applicationType = strToIntDef(_request.getParameter("color"), 0);
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.createGroup(m_Apartment.getZone(0), pCluster->getID(), static_cast<ApplicationType>(applicationType),
        0, clusterName);

    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));

    JSONWriter json;
    json.add("clusterID", pCluster->getID());
    json.add("name", clusterName);
    json.add("color", pCluster->getColor());
    json.add("applicationType", applicationType);
    return json.successJSON();
  } // addCluster

  std::string StructureRequestHandler::addGroup(const RestfulRequest& _request) {
    boost::shared_ptr<Zone> zone;
    boost::shared_ptr<Group> pGroup;
    int groupID = -1;
    int zoneID = -1;
    int groupColor = 0;
    std::string groupName;

    if (_request.hasParameter("zoneID")) {
      std::string zoneIDStr = _request.getParameter("zoneID");
      zoneID = strToIntDef(zoneIDStr, -1);
      zone = m_Apartment.getZone(zoneID);
    }
    if (zoneID < 0 || !zone) {
      return JSONWriter::failure("Parameter zoneID missing or empty: '" + _request.getParameter("zoneID") + "'");
    }

    if (_request.hasParameter("groupID")) {
      std::string groupIDStr = _request.getParameter("groupID");
      groupID = strToIntDef(groupIDStr, -1);
      if (groupID < 0) {
        return JSONWriter::failure("Parameter GroupID '" + _request.getParameter("groupID") + "' is not valid");
      }
      if (zone->getGroup(groupID) != NULL) {
        return JSONWriter::failure("Group with ID " + _request.getParameter("groupID") + " already exists");
      }
    }
    else if (_request.hasParameter("groupAutoSelect")) {
      std::string grType = _request.getParameter("groupAutoSelect");
      if (grType == "user") {
        groupID = -1;
      } else if (grType == "global") {
        groupID = -2;
      } else if (grType == "globalApp") {
        groupID = -3;
      }
    } else {
      return JSONWriter::failure("Parameter groupID or groupAutoSelect missing");
    }

    if (groupID == -1) {
      // find any free group slot
      for (groupID = GroupIDUserGroupStart; pGroup == NULL && groupID < GroupIDUserGroupEnd; groupID ++) {
        pGroup = zone->getGroup(groupID);
      }
    } else if (groupID == -2) {
      if (zoneID != 0) {
        return JSONWriter::failure("Apartment user groups only allowed in Zone 0");
      }
      // find a group slot with unassigned state machine id
      for (groupID = GroupIDAppUserMin; groupID <= GroupIDAppUserMax; groupID++) {
        pGroup = zone->getGroup(groupID);
        if (pGroup->getApplicationType() == ApplicationType::None) {
          break;
        }
      }
      if (pGroup->getApplicationType() != ApplicationType::None) {
        pGroup.reset();
      }
    } else if (groupID == -3) {
      if (zoneID != 0) {
        return JSONWriter::failure("Global application groups only allowed in Zone 0");
      }
      // find any free user global application slot
      for (groupID = GroupIDGlobalAppUserMin; (pGroup == NULL) && (groupID < GroupIDGlobalAppUserMax); groupID ++) {
        pGroup = zone->getGroup(groupID);
      }
    }

    if (pGroup == NULL) {
      return JSONWriter::failure("No free user groups");
    }

    if (_request.hasParameter("groupName")) {
      StringConverter st("UTF-8", "UTF-8");
      groupName = st.convert(_request.getParameter("groupName"));
      groupName = escapeHTML(groupName);
    }
    if (_request.hasParameter("groupColor")) {
      groupColor = strToIntDef(_request.getParameter("groupColor"), 0);
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.createGroup(zone, groupID, static_cast<ApplicationType>(groupColor), 0, groupName);

    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));

    JSONWriter json;
    json.add("groupID", groupID);
    json.add("zoneID", zoneID);
    json.add("groupName", groupName);
    json.add("groupColor", groupColor);
    return json.successJSON();
  } // addGroup

  std::string StructureRequestHandler::groupAddDevice(const RestfulRequest& _request) {
    boost::shared_ptr<Device> dev;
    boost::shared_ptr<Group> gr;

    std::string deviceIDStr = _request.getParameter("deviceID");
    std::string dsuidStr = _request.getParameter("dsuid");

    if (dsuidStr.empty() && deviceIDStr.empty()) {
      return JSONWriter::failure("Missing parameter 'dsuid'");
    }

    dsuid_t dsuid = dsidOrDsuid2dsuid(deviceIDStr, dsuidStr);

    dev = m_Apartment.getDeviceByDSID(dsuid);
    if(!dev->isPresent()) {
      return JSONWriter::failure("Cannot modify inactive device");
    }

    int id;
    if (!_request.getParameter("clusterID", id) && !_request.getParameter("groupID", id)) {
      return JSONWriter::failure("Could not parse groupID or clusterID");
    }
    if (isAppUserGroup(id)) {
      try {
        gr = m_Apartment.getCluster(id);
      } catch (ItemNotFoundException& e) {
        return JSONWriter::failure("Invalid value for parameter clusterID : '" + intToString(id) + "'");
      }
    } else if (isGlobalAppGroup(id)) {
      try {
        gr = m_Apartment.getGroup(id);
      } catch (ItemNotFoundException& e) {
        return JSONWriter::failure("Invalid value for parameter apartment groupID : '" + intToString(id) + "'");
      }
    } else {
      try {
        boost::shared_ptr<Zone> zone = m_Apartment.getZone(dev->getZoneID());
        gr = zone->getGroup(id);
        if (gr == NULL) {
          return JSONWriter::failure("Invalid value for parameter groupID : '" + intToString(id) + "'");
        }
      } catch (ItemNotFoundException& e) {
        return JSONWriter::failure("Could not get relevant zone.");
      }
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    try {
      if (((dev->getDeviceType() == DEVICE_TYPE_AKM) ||
          ((dev->getDeviceType() == DEVICE_TYPE_TKM) && !dev->hasOutput()) ||
          ((dev->getDeviceType() == DEVICE_TYPE_UMR) && dev->hasInput()))
          && (dev->getGroupsCount() > 1)) {
        for (int g = 0; g < dev->getGroupsCount(); g++) {
          boost::shared_ptr<Group> oldGroup = dev->getGroupByIndex(g);
          if ((oldGroup != NULL) && (oldGroup->getID() >= GroupIDAppUserMin) && (oldGroup->getID() <= GroupIDAppUserMax)) {
            manipulator.deviceRemoveFromGroup(dev, oldGroup);
          }
        }
      }
      manipulator.deviceAddToGroup(dev, gr);
    } catch (DSSException& e) {
      return JSONWriter::failure(e.what());
    }

    std::vector<boost::shared_ptr<Device> > modifiedDevices;
    modifiedDevices.push_back(dev);

    if (dev->is2WayMaster()) {
      dsuid_t next;
      dsuid_get_next_dsuid(dev->getDSID(), &next);
      try {
        boost::shared_ptr<Device> pPartnerDevice;

        // Remove slave/partner device from all other user groups
        try {
          pPartnerDevice = m_Apartment.getDeviceByDSID(next);
          for (int g = 0; g < pPartnerDevice->getGroupsCount(); g++) {
            boost::shared_ptr<Group> oldGroup = pPartnerDevice->getGroupByIndex(g);
            if ((oldGroup != NULL) && (oldGroup->getID() >= GroupIDAppUserMin) && (oldGroup->getID() <= GroupIDAppUserMax)) {
              manipulator.deviceRemoveFromGroup(pPartnerDevice, oldGroup);
            }
          }
          manipulator.deviceAddToGroup(pPartnerDevice, gr);
        } catch (DSSException& e) {
          return JSONWriter::failure(e.what());
        }

      } catch(std::runtime_error& e) {
        return JSONWriter::failure("Could not find partner device with dSUID '" + dsuid2str(next) + "'");
      }
    }
    if (dev->getOemInfoState() == DEVICE_OEM_VALID) {
      uint16_t serialNr = dev->getOemSerialNumber();
      if ((serialNr > 0) & !dev->getOemIsIndependent()) {
        std::vector<boost::shared_ptr<Device> > devices = m_Apartment.getDevicesVector();
        foreach (const boost::shared_ptr<Device>& device, devices) {
          if (dev->isOemCoupledWith(device)) {
            // Remove coupled OEM devices from all other user groups as well
            try {
              for (int g = 0; g < device->getGroupsCount(); g++) {
                boost::shared_ptr<Group> oldGroup = device->getGroupByIndex(g);
                if ((oldGroup != NULL) && (oldGroup->getID() >= GroupIDAppUserMin) && (oldGroup->getID() <= GroupIDAppUserMax)) {
                  manipulator.deviceRemoveFromGroup(device, oldGroup);
                }
              }
              manipulator.deviceAddToGroup(device, gr);
              modifiedDevices.push_back(device);
            } catch (DSSException& e) {
              return JSONWriter::failure(e.what());
            }
          }
        }
      }
    }

    JSONWriter json;
    if (!modifiedDevices.empty()) {
      sleep(2);
      json.startArray("devices");
      foreach (const boost::shared_ptr<Device>& device, modifiedDevices) {
        const DeviceReference d(device, &m_Apartment);
        toJSON(d, json);
      }
      json.endArray();
      json.add("action", "update");
    } else {
      json.add("action", "none");
    }
    return json.successJSON();
  } // groupAddDevice

  std::string StructureRequestHandler::groupRemoveDevice(const RestfulRequest& _request) {
    boost::shared_ptr<Device> dev;
    boost::shared_ptr<Group> gr;

    std::string deviceIDStr = _request.getParameter("deviceID");
    std::string dsuidStr = _request.getParameter("dsuid");

    if (dsuidStr.empty() && deviceIDStr.empty()) {
      return JSONWriter::failure("Missing parameter 'dsuid'");
    }

    dsuid_t dsuid = dsidOrDsuid2dsuid(deviceIDStr, dsuidStr);

    dev = m_Apartment.getDeviceByDSID(dsuid);
    if(!dev->isPresent()) {
      return JSONWriter::failure("Cannot modify inactive device");
    }

    int id;
    if (!_request.getParameter("clusterID", id) && !_request.getParameter("groupID", id)) {
      return JSONWriter::failure("Could not parse groupID or clusterID");
    }
    if (isAppUserGroup(id)) {
      try {
        gr = m_Apartment.getCluster(id);
      } catch (ItemNotFoundException& e) {
        return JSONWriter::failure("Invalid value for parameter clusterID : '" + intToString(id) + "'");
      }
    } else {
      try {
        boost::shared_ptr<Zone> zone = m_Apartment.getZone(dev->getZoneID());
        gr = zone->getGroup(id);
        if (gr == NULL) {
          return JSONWriter::failure("Invalid value for parameter groupID : '" + intToString(id) + "'");
        }
      } catch (ItemNotFoundException& e) {
        return JSONWriter::failure("Could not get relevant zone.");
      }
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    try {
      manipulator.deviceRemoveFromGroup(dev, gr);
    } catch (DSSException& e) {
      return JSONWriter::failure(e.what());
    }

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
        return JSONWriter::failure("Could not find partner device with dSUID '" + dsuid2str(next) + "'");
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

    JSONWriter json;
    if (!modifiedDevices.empty()) {
      sleep(2);
      json.startArray("devices");
      foreach (const boost::shared_ptr<Device>& device, modifiedDevices) {
        const DeviceReference d(device, &m_Apartment);
        toJSON(d, json);
      }
      json.endArray();
      json.add("action", "update");
    } else {
      json.add("action", "none");
    }
    return json.successJSON();
  } // groupRemoveDevice


  std::string StructureRequestHandler::groupSetName(const RestfulRequest& _request) {
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
      return JSONWriter::failure("Invalid value for parameter zoneID : '" + _request.getParameter("zoneID") + "'");
    }

    if(_request.hasParameter("groupID")) {
      std::string groupIDStr = _request.getParameter("groupID");
      groupID = strToIntDef(groupIDStr, -1);
    }

    group = zone->getGroup(groupID);
    if ((groupID < 1) || !group) {
      return JSONWriter::failure("Could not find group with id : '" + _request.getParameter("groupID") + "'");
    }

    if (!_request.hasParameter("newName")) {
      return JSONWriter::failure("missing parameter 'newName'");
    }

    std::string newName = st.convert(_request.getParameter("newName"));
    newName = escapeHTML(newName);

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.groupSetName(group, newName);

    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return JSONWriter::success();
  }

  std::string StructureRequestHandler::groupSetColor(const RestfulRequest& _request) {
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
      return JSONWriter::failure("Invalid value for parameter zoneID : '" + _request.getParameter("zoneID") + "'");
    }

    if(_request.hasParameter("groupID")) {
      std::string groupIDStr = _request.getParameter("groupID");
      groupID = strToIntDef(groupIDStr, -1);
    }

    group = zone->getGroup(groupID);
    if ((groupID < 1) || !group) {
      return JSONWriter::failure("Could not find group with id : '" + _request.getParameter("groupID") + "'");
    }

    if (!_request.hasParameter("newColor")) {
      return JSONWriter::failure("missing parameter 'newColor'");
    }

    int newColor = strToIntDef(_request.getParameter("newColor"), -1);
    if (newColor < 0) {
      return JSONWriter::failure("Invalid value for parameter newColor: '" + _request.getParameter("newColor") + "'");
    }

    std::string configuration = "{}";
    if (_request.hasParameter("configuration")) {
      configuration = _request.getParameter("configuration");
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.groupSetApplication(
        group, static_cast<ApplicationType>(newColor), group->deserializeApplicationConfiguration(configuration));

    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return JSONWriter::success();
  }

  std::string StructureRequestHandler::groupGetConfiguration(const RestfulRequest& _request) {
    boost::shared_ptr<Zone> zone;
    boost::shared_ptr<Group> group;
    int groupID = -1;
    int zoneID = 0;

    if (_request.hasParameter("zoneID")) {
      std::string zoneIDStr = _request.getParameter("zoneID");
      zoneID = strToIntDef(zoneIDStr, -1);
    }

    zone = m_Apartment.getZone(zoneID);

    if (zoneID < 0 || !zone) {
      return JSONWriter::failure("Invalid value for parameter zoneID : '" + _request.getParameter("zoneID") + "'");
    }

    if (_request.hasParameter("groupID")) {
      std::string groupIDStr = _request.getParameter("groupID");
      groupID = strToIntDef(groupIDStr, -1);
    }

    group = zone->getGroup(groupID);
    if ((groupID < 1) || !group) {
      return JSONWriter::failure("Could not find group with id : '" + _request.getParameter("groupID") + "'");
    }

    // Currently configuration for zone related groups in zone 0 are undefined
    if ((zone->getID() == 0) && (isDefaultGroup(group->getID()))) {
      return JSONWriter::failure(
          "Configuration for group : '" + _request.getParameter("groupID") + "' in zone 0 is not defined");
    }

    JSONWriter json;
    group->serializeApplicationConfiguration(group->getApplicationConfiguration(), json);
    return json.successJSON();
  }

  std::string StructureRequestHandler::groupSetConfiguration(const RestfulRequest& _request) {
    boost::shared_ptr<Zone> zone;
    boost::shared_ptr<Group> group;
    int groupID = -1;
    int zoneID = 0;
    int applicationType = -1;

    if (_request.hasParameter("zoneID")) {
      std::string zoneIDStr = _request.getParameter("zoneID");
      zoneID = strToIntDef(zoneIDStr, -1);
    }

    zone = m_Apartment.getZone(zoneID);

    if (zoneID < 0 || !zone) {
      return JSONWriter::failure("Invalid value for parameter zoneID : '" + _request.getParameter("zoneID") + "'");
    }

    if (_request.hasParameter("groupID")) {
      std::string groupIDStr = _request.getParameter("groupID");
      groupID = strToIntDef(groupIDStr, -1);
    }

    group = zone->getGroup(groupID);
    if ((groupID < 1) || !group) {
      return JSONWriter::failure("Could not find group with id : '" + _request.getParameter("groupID") + "'");
    }

    // Currently configuration for zone related groups in zone 0 are undefined
    if ((zone->getID() == 0) && (isDefaultGroup(group->getID()))) {
      return JSONWriter::failure(
          "Configuration for group : '" + _request.getParameter("groupID") + "' in zone 0 is not defined");
    }

    if (_request.hasParameter("applicationType")) {
      std::string applicationTypeStr = _request.getParameter("applicationType");
      applicationType = strToIntDef(applicationTypeStr, -1);
    }

    // if the applicationType was not provided we assume current group application type, but only for dS defined groups
    if (applicationType < 0) {
      if (isDefaultGroup(group->getID()) || isGlobalAppDsGroup(group->getID())) {
        applicationType = static_cast<int>(group->getApplicationType());
      } else {
        return JSONWriter::failure("Invalid application Type: '" + _request.getParameter("applicationType") + "'");
      }
    }

    // if not present the configuration is empty - default
    std::string configuration = "{}";

    if (_request.hasParameter("configuration")) {
      configuration = _request.getParameter("configuration");
    }

    // set the configuration in the DSM
    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.groupSetApplication(group, static_cast<ApplicationType>(applicationType),
        group->deserializeApplicationConfiguration(configuration));

    // raise ModelDirty to force rewrite of model data to apartment.xml
    DSS::getInstance()->getModelMaintenance().addModelEvent(new ModelEvent(ModelEvent::etModelDirty));

    return JSONWriter::success();
  }

  std::string StructureRequestHandler::clusterSetName(const RestfulRequest& _request) {
    StringConverter st("UTF-8", "UTF-8");
    boost::shared_ptr<Cluster> cluster;
    int clusterID = -1;

    if(!_request.getParameter<int>("clusterID", clusterID)) {
      return JSONWriter::failure("Required parameter clusterID missing");
    }

    cluster = m_Apartment.getCluster(clusterID);
    if (!cluster) {
      return JSONWriter::failure("Could not find group with id : '" + _request.getParameter("clusterID") + "'");
    }

    std::string newName;
    if (!_request.getParameter<std::string>("newName", newName)) {
      return JSONWriter::failure("missing parameter 'newName'");
    }
    newName = escapeHTML(newName);

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.clusterSetName(cluster, newName);

    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return JSONWriter::success();
  }

  std::string StructureRequestHandler::clusterSetColor(const RestfulRequest& _request) {
    boost::shared_ptr<Cluster> pCluster;
    int clusterID = -1;

    if(!_request.getParameter<int>("clusterID", clusterID)) {
      return JSONWriter::failure("Required parameter clusterID missing");
    }

    pCluster = m_Apartment.getCluster(clusterID);
    if ((clusterID < 1) || !pCluster) {
      return JSONWriter::failure("Could not find pCluster with id : '" + _request.getParameter("clusterID") + "'");
    }

    int newColor;
    if (!_request.getParameter<int>("newColor", newColor)) {
      return JSONWriter::failure("missing parameter 'newColor'");
    }

    if (newColor < 0) {
      return JSONWriter::failure("Invalid value for parameter newColor: '" + _request.getParameter("newColor") + "'");
    }

    std::string configuration = "{}";
    if (_request.hasParameter("configuration")) {
      configuration = _request.getParameter("configuration");
    }

    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.clusterSetApplication(
        pCluster, static_cast<ApplicationType>(newColor), pCluster->deserializeApplicationConfiguration(configuration));

    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return JSONWriter::success();
  }

  std::string StructureRequestHandler::clusterSetConfigLock(const RestfulRequest& _request) {
    boost::shared_ptr<Cluster> pCluster;
    int clusterID = -1;

    if(!_request.getParameter<int>("clusterID", clusterID)) {
      return JSONWriter::failure("Required parameter clusterID missing");
    }

    pCluster = m_Apartment.getCluster(clusterID);
    if ((clusterID < 1) || !pCluster) {
      return JSONWriter::failure("Could not find pCluster with id : '" + _request.getParameter("clusterID") + "'");
    }

    if (!isAppUserGroup(clusterID)) {
      return JSONWriter::failure(" Id : '" + _request.getParameter("clusterID") + "' is not a cluster");
    }

    int lock;
    if (!_request.getParameter<int>("lock", lock)) {
      return JSONWriter::failure("missing parameter 'lock'");
    }

    if ((lock < 0) || (lock > 1)) {
      return JSONWriter::failure("Invalid value for parameter lock: '" + _request.getParameter("lock") + "'");
    }

    boost::shared_ptr<Cluster> cluster = m_Apartment.getCluster(clusterID);
    StructureManipulator manipulator(m_Interface, m_QueryInterface, m_Apartment);
    manipulator.clusterSetConfigurationLock(cluster, lock);
    m_ModelMaintenance.getDSS().getEventQueue().pushEvent(createClusterConfigLockEvent(pCluster, 0, clusterID, lock, coJSON));
    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return JSONWriter::success();
  }


  WebServerResponse StructureRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session, const struct mg_connection* _connection) {
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
    } else if(_request.getMethod() == "addCluster") {
      return addCluster(_request);
    } else if(_request.getMethod() == "removeCluster") {
      return removeCluster(_request);
    } else if(_request.getMethod() == "groupAddDevice" || _request.getMethod() == "clusterAddDevice") {
      return groupAddDevice(_request);
    } else if(_request.getMethod() == "groupRemoveDevice" || _request.getMethod() == "clusterRemoveDevice") {
      return groupRemoveDevice(_request);
    } else if(_request.getMethod() == "groupSetName") {
      return groupSetName(_request);
    } else if(_request.getMethod() == "groupSetColor") {
      return groupSetColor(_request);
    } else if (_request.getMethod() == "groupGetConfiguration") {
      return groupGetConfiguration(_request);
    } else if (_request.getMethod() == "groupSetConfiguration") {
      return groupSetConfiguration(_request);
    } else if(_request.getMethod() == "clusterSetName") {
      return clusterSetName(_request);
    } else if(_request.getMethod() == "clusterSetColor") {
      return clusterSetColor(_request);
    } else if(_request.getMethod() == "clusterSetConfigLock") {
      return clusterSetConfigLock(_request);
    } else {
      throw std::runtime_error("Unhandled function");
    }
  } // handleRequest

} // namespace dss
