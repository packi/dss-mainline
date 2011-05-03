/*
    Copyright (c) 2010,2011 digitalSTROM.org, Zurich, Switzerland

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

#include "busscanner.h"

#include <vector>

#include "core/businterface.h"
#include "core/foreach.h"
#include "core/model/modelconst.h"
#include "core/event.h"
#include "core/dss.h"
#include "core/dsidhelper.h"

#include "modulator.h"
#include "device.h"
#include "group.h"
#include "apartment.h"
#include "zone.h"
#include "modelmaintenance.h"


namespace dss {

  BusScanner::BusScanner(StructureQueryBusInterface& _interface, Apartment& _apartment, ModelMaintenance& _maintenance)
  : m_Apartment(_apartment),
    m_Interface(_interface),
    m_Maintenance(_maintenance)
  {
  }

  bool BusScanner::scanDSMeter(boost::shared_ptr<DSMeter> _dsMeter) {
    _dsMeter->setIsPresent(true);
    _dsMeter->setIsValid(false);

    log("scanDSMeter: Start " + _dsMeter->getDSID().toString() , lsInfo);
    // TODO: implement energy border handling

    try {
      DSMeterSpec_t spec = m_Interface.getDSMeterSpec(_dsMeter->getDSID());
      _dsMeter->setArmSoftwareVersion(spec.SoftwareRevisionARM);
      _dsMeter->setDspSoftwareVersion(spec.SoftwareRevisionDSP);
      _dsMeter->setHardwareVersion(spec.HardwareVersion);
      _dsMeter->setApiVersion(spec.APIVersion);
      _dsMeter->setName(spec.Name);
    } catch(BusApiError& e) {
      log("scanDSMeter: Error getting dSMSpecs", lsFatal);
      return false;
    }

    std::vector<int> zoneIDs;
    try {
      zoneIDs = m_Interface.getZones(_dsMeter->getDSID());
    } catch(BusApiError& e) {
      log("scanDSMeter: Error getting ZoneIDs", lsFatal);
      return false;
    }

    bool firstZone = true;
    foreach(int zoneID, zoneIDs) {
      log("scanDSMeter:  Found zone with id: " + intToString(zoneID));
      boost::shared_ptr<Zone> zone = m_Apartment.allocateZone(zoneID);
      zone->addToDSMeter(_dsMeter);
      zone->setIsPresent(true);
      if(firstZone) {
        zone->setFirstZoneOnDSMeter(_dsMeter->getDSID());
        firstZone = false;
      }
      if(!scanZone(_dsMeter, zone)) {
        return false;
      }
    }
    _dsMeter->setIsValid(true);
    return true;
  } // scanDSMeter

  bool BusScanner::scanZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone) {
    std::vector<DeviceSpec_t> devices;
    try {
      devices = m_Interface.getDevicesInZone(_dsMeter->getDSID(), _zone->getID());
    } catch(BusApiError& e) {
      log("scanDSMeter: Error getting getDevicesInZone", lsFatal);
      return false;
    }
    foreach(DeviceSpec_t& spec, devices) {
      initializeDeviceFromSpec(_dsMeter, _zone, spec);
    }

    return scanGroupsOfZone(_dsMeter, _zone);
  } // scanZone

  bool BusScanner::scanDeviceOnBus(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone, devid_t _shortAddress) {
    DeviceSpec_t spec = m_Interface.deviceGetSpec(_shortAddress, _dsMeter->getDSID());
    return initializeDeviceFromSpec(_dsMeter, _zone, spec);
  } // scanDeviceOnBus

  bool BusScanner::initializeDeviceFromSpec(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone, DeviceSpec_t& _spec) {
    log("scanDeviceOnBus:    Found device with address: " + intToString(_spec.ShortAddress));
    log("scanDeviceOnBus:    DSID:        " + _spec.DSID.toString());
    log("scanDeviceOnBus:    Function-ID: " + unsignedLongIntToHexString(_spec.FunctionID) +
        ", Product-ID: " + unsignedLongIntToHexString(_spec.ProductID) +
        ", Revision-ID: " + unsignedLongIntToHexString(_spec.Version)
        );

    boost::shared_ptr<Device> dev = m_Apartment.allocateDevice(_spec.DSID);
    DeviceReference devRef(dev, &m_Apartment);

    // remove from old dsMeter
    try {
      boost::shared_ptr<DSMeter> oldDSMeter = m_Apartment.getDSMeterByDSID(dev->getLastKnownDSMeterDSID());
      oldDSMeter->removeDevice(devRef);
    } catch(std::runtime_error&) {
    }

    // remove from old zone
    if(dev->getZoneID() != 0) {
      try {
        boost::shared_ptr<Zone> oldZone = m_Apartment.getZone(dev->getZoneID());
        oldZone->removeDevice(devRef);
        // TODO: check if the zone is empty on the dsMeter and remove it in that case
      } catch(std::runtime_error&) {
      }
    }

    dev->setShortAddress(_spec.ShortAddress);
    dev->setDSMeter(_dsMeter);
    dev->setZoneID(_zone->getID());
    dev->setFunctionID(_spec.FunctionID);
    dev->setProductID(_spec.ProductID);
    dev->setRevisionID(_spec.Version);
    dev->setIsLockedInDSM(_spec.Locked);
    dev->setHasOutputLoad(_spec.OutputHasLoad);
    dev->setButtonActiveGroup(_spec.ActiveGroup);
    dev->setButtonGroupMembership(_spec.GroupMembership);
    dev->setButtonSetsLocalPriority(_spec.SetsLocalPriority);
    dev->setButtonID(_spec.ButtonID);
    if(dev->getName().empty()) {
      dev->setName(_spec.Name);
    }
    dev->resetGroups();

    foreach(int groupID, _spec.Groups) {
      log(std::string("scanDeviceOnBus: adding device ") + intToString(_spec.ShortAddress) + " to group " + intToString(groupID));
      dev->addToGroup(groupID);
    }

    _zone->addToDSMeter(_dsMeter);
    _zone->addDevice(devRef);
    _dsMeter->addDevice(devRef);
    dev->setIsPresent(true);

    {
      boost::shared_ptr<Event> readyEvent(new Event("new_device"));
      readyEvent->setProperty("device", _spec.DSID.toString());
      readyEvent->setProperty("zone", intToString(_zone->getID()));
      if(DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(readyEvent);
      }
    }

    return true;
  } // scanDeviceOnBus

  bool BusScanner::scanGroupsOfZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone) {
    std::vector<int> groupIDs;
    try {
      groupIDs = m_Interface.getGroups(_dsMeter->getDSID(), _zone->getID());
    } catch(BusApiError& e) {
      log("scanDSMeter: Error getting getGroups", lsFatal);
      return false;
    }

    foreach(int groupID, groupIDs) {
      if(groupID == 0) {
        log("scanDSMeter:    Group ID is zero, bailing out... (dsMeterID: "
            + _dsMeter->getDSID().toString() +
            "zoneID: " + intToString(_zone->getID()) + ")",
            lsError);
        continue;
      }
      log("scanDSMeter:    Found group with id: " + intToString(groupID));
      boost::shared_ptr<Group> groupOnZone = _zone->getGroup(groupID);
      if(groupOnZone == NULL) {
        log(" scanDSMeter:    Adding new group to zone");
        groupOnZone.reset(new Group(groupID, _zone, m_Apartment));
        _zone->addGroup(groupOnZone);
      }
      groupOnZone->setIsPresent(true);
      try {
        boost::shared_ptr<Group> group = m_Apartment.getGroup(groupID);
        group->setIsPresent(true);
      } catch(ItemNotFoundException&) {
        boost::shared_ptr<Zone> zoneBroadcast = m_Apartment.getZone(0);
        boost::shared_ptr<Group> pGroup(new Group(groupID, zoneBroadcast, m_Apartment));
        zoneBroadcast->addGroup(pGroup);
        pGroup->setIsPresent(true);
        log("scanDSMeter:     Adding new group to zone 0");
      }

      // TODO: get last called scene
      m_Maintenance.onGroupCallScene(_zone->getID(), groupID, SceneOff);
    }
    return true;
  } // scanGroupsOfZone

  void BusScanner::log(const std::string& _line, aLogSeverity _severity) {
    Logger::getInstance()->log(_line, _severity);
  } // log

} // namespace dss
