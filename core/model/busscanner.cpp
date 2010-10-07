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

#include "busscanner.h"

#include <vector>

#include "core/businterface.h"
#include "core/foreach.h"
#include "core/ds485const.h"
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

    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_dsMeter->getDSID(), dsmDSID);

    log("scanDSMeter: Start " + _dsMeter->getDSID().toString() , lsInfo);
    std::vector<int> zoneIDs;
    try {
      zoneIDs = m_Interface.getZones(dsmDSID);
    } catch(BusApiError& e) {
      log("scanDSMeter: Error getting ZoneIDs", lsFatal);
      return false;
    }

    // TODO: implement energy border handling

    try {
      DSMeterSpec_t spec = m_Interface.getDSMeterSpec(dsmDSID);
      _dsMeter->setSoftwareVersion(spec.get<1>());
      _dsMeter->setHardwareVersion(spec.get<2>());
      _dsMeter->setApiVersion(spec.get<3>());
      _dsMeter->setHardwareName(spec.get<4>());
    } catch(BusApiError& e) {
      log("scanDSMeter: Error getting dSMSpecs", lsFatal);
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
    std::vector<int> devices;
    try {
      dsid_t dsmDSID;
      dsid_helper::toDsmapiDsid(_dsMeter->getDSID(), dsmDSID);
      devices = m_Interface.getDevicesInZone(dsmDSID, _zone->getID());
    } catch(BusApiError& e) {
      log("scanDSMeter: Error getting getDevicesInZone", lsFatal);
      return false;
    }
    foreach(int devID, devices) {
      scanDeviceOnBus(_dsMeter, _zone, devID);
    }

    return scanGroupsOfZone(_dsMeter, _zone);
  } // scanZone

  bool BusScanner::scanDeviceOnBus(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone, devid_t _shortAddress) {
    dss_dsid_t dsid;
    dsid_t dsmDSID;
    try {
      dsid_helper::toDsmapiDsid(_dsMeter->getDSID(), dsmDSID);
      dsid = m_Interface.getDSIDOfDevice(dsmDSID, _shortAddress);
    } catch(BusApiError& e) {
      log("scanDeviceOnBus: Error getting getDSIDOfDevice:" + std::string(e.what()) + " " + 
          dsid.toString() + " " + intToString(_shortAddress), lsFatal);
      return false;
    }

    int functionID = 0;
    int productID = 0;
    int revisionID = 0;
    try {
      DeviceSpec_t spec = m_Interface.deviceGetSpec(_shortAddress, _dsMeter->getDSID());
      functionID = spec.get<0>();
      productID = spec.get<1>();
      revisionID = spec.get<2>();
    } catch(BusApiError& e) {
      log(std::string("scanDSMeter: Error getting device spec ") + e.what(), lsFatal);
      return false;
    }
    log("scanDeviceOnBus:    Found device with address: " + intToString(_shortAddress));
    log("scanDeviceOnBus:    DSID:        " + dsid.toString());
    log("scanDeviceOnBus:    Function-ID: " + unsignedLongIntToHexString(functionID) +
        ", Product-ID: " + unsignedLongIntToHexString(productID) +
        ", Revision-ID: " + unsignedLongIntToHexString(revisionID)
        );

    boost::shared_ptr<Device> dev = m_Apartment.allocateDevice(dsid);
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

    dev->setShortAddress(_shortAddress);
    dev->setDSMeter(_dsMeter);
    dev->setZoneID(_zone->getID());
    dev->setFunctionID(functionID);
    dev->setProductID(productID);
    dev->setRevisionID(revisionID);

    try {
      bool locked = m_Interface.isLocked(dev);
      dev->setIsLockedInDSM(locked);

      log(std::string("scanDeviceOnBus:   Device is ") + (locked ? "locked" : "unlocked"));
    } catch(BusApiError& e) {
      log(std::string("scanDeviceOnBus: Error getting devices lock state, not aborting scan (") + e.what() + ")", lsWarning);
    }

    dev->resetGroups();
    dsid_helper::toDsmapiDsid(_dsMeter->getDSID(), dsmDSID);

    std::vector<int> groupIDsPerDevice = m_Interface.getGroupsOfDevice(dsmDSID, _shortAddress);
    foreach(int groupID, groupIDsPerDevice) {
      log(std::string("scanDeviceOnBus: adding device ") + intToString(_shortAddress) + " to group " + intToString(groupID));
      dev->addToGroup(groupID);
    }

    _zone->addToDSMeter(_dsMeter);
    _zone->addDevice(devRef);
    _dsMeter->addDevice(devRef);
    dev->setIsPresent(true);

    {
      boost::shared_ptr<Event> readyEvent(new Event("new_device"));
      readyEvent->setProperty("device", dsid.toString());
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
      dsid_t dsid;
      dsid_helper::toDsmapiDsid(_dsMeter->getDSID(), dsid);

      groupIDs = m_Interface.getGroups(dsid, _zone->getID());
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
        groupOnZone.reset(new Group(groupID, _zone->getID(), m_Apartment));
        _zone->addGroup(groupOnZone);
      }
      groupOnZone->setIsPresent(true);
      try {
        boost::shared_ptr<Group> group = m_Apartment.getGroup(groupID);
        group->setIsPresent(true);
      } catch(ItemNotFoundException&) {
        boost::shared_ptr<Group> pGroup(new Group(groupID, 0, m_Apartment));
        m_Apartment.getZone(0)->addGroup(pGroup);
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
