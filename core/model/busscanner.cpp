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

#include "core/DS485Interface.h"
#include "core/foreach.h"
#include "core/ds485const.h"
#include "core/model/modelconst.h"
#include "core/event.h"
#include "core/dss.h"

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

    int dsMeterID = _dsMeter->getBusID();

    log("scanDSMeter: Start " + intToString(dsMeterID) , lsInfo);
    std::vector<int> zoneIDs;
    try {
      zoneIDs = m_Interface.getZones(dsMeterID);
    } catch(DS485ApiError& e) {
      log("scanDSMeter: Error getting ZoneIDs", lsFatal);
      return false;
    }

    int levelOrange, levelRed;
    try {
      if(m_Interface.getEnergyBorder(dsMeterID, levelOrange, levelRed)) {
        _dsMeter->setEnergyLevelOrange(levelOrange);
        _dsMeter->setEnergyLevelRed(levelRed);
      }
    } catch(DS485ApiError& e) {
      log("scanDSMeter: Error getting EnergyLevels", lsFatal);
      return false;
    }

    try {
      DSMeterSpec_t spec = m_Interface.getDSMeterSpec(dsMeterID);
      _dsMeter->setSoftwareVersion(spec.get<1>());
      _dsMeter->setHardwareVersion(spec.get<2>());
      _dsMeter->setHardwareName(spec.get<3>());
      _dsMeter->setDeviceType(spec.get<4>());
    } catch(DS485ApiError& e) {
      log("scanDSMeter: Error getting dSMSpecs", lsFatal);
      return false;
    }

    bool firstZone = true;
    foreach(int zoneID, zoneIDs) {
      log("scanDSMeter:  Found zone with id: " + intToString(zoneID));
      Zone& zone = m_Apartment.allocateZone(zoneID);
      zone.addToDSMeter(_dsMeter);
      zone.setIsPresent(true);
      if(firstZone) {
        zone.setFirstZoneOnDSMeter(dsMeterID);
        firstZone = false;
      }
      if(!scanZone(_dsMeter, zone)) {
        return false;
      }
    }
    _dsMeter->setIsValid(true);
    return true;
  } // scanDSMeter

  bool BusScanner::scanZone(boost::shared_ptr<DSMeter> _dsMeter, Zone& _zone) {
    std::vector<int> devices;
    try {
      devices = m_Interface.getDevicesInZone(_dsMeter->getBusID(), _zone.getID());
    } catch(DS485ApiError& e) {
      log("scanDSMeter: Error getting getDevicesInZone", lsFatal);
      return false;
    }
    foreach(int devID, devices) {
      scanDeviceOnBus(_dsMeter, _zone, devID);
    }

    return scanGroupsOfZone(_dsMeter, _zone);
  } // scanZone

  bool BusScanner::scanDeviceOnBus(boost::shared_ptr<DSMeter> _dsMeter, Zone& _zone, devid_t _shortAddress) {
    dsid_t dsid;
    try {
      dsid = m_Interface.getDSIDOfDevice(_dsMeter->getBusID(), _shortAddress);
    } catch(DS485ApiError& e) {
        log("scanDeviceOnBus: Error getting getDSIDOfDevice", lsFatal);
        return false;
    }

    int functionID = 0;
    int productID = 0;
    int revisionID = 0;
    try {
      DeviceSpec_t spec = m_Interface.deviceGetSpec(_shortAddress, _dsMeter->getBusID());
      functionID = spec.get<0>();
      productID = spec.get<1>();
      revisionID = spec.get<2>();
    } catch(DS485ApiError& e) {
      log(std::string("scanDSMeter: Error getting device spec ") + e.what(), lsFatal);
      return false;
    }
    log("scanDeviceOnBus:    Found device with address: " + intToString(_shortAddress));
    log("scanDeviceOnBus:    DSID:        " + dsid.toString());
    log("scanDeviceOnBus:    Function-ID: " + unsignedLongIntToHexString(functionID) +
        ", Product-ID: " + unsignedLongIntToHexString(productID) +
        ", Revision-ID: " + unsignedLongIntToHexString(revisionID)
        );

    Device& dev = m_Apartment.allocateDevice(dsid);
    DeviceReference devRef(dev, &m_Apartment);

    // remove from old dsMeter
    try {
      boost::shared_ptr<DSMeter> oldDSMeter = m_Apartment.getDSMeterByDSID(dev.getLastKnownDSMeterDSID());
      oldDSMeter->removeDevice(devRef);
    } catch(std::runtime_error&) {
    }

    // remove from old zone
    if(dev.getZoneID() != 0) {
      try {
        Zone& oldZone = m_Apartment.getZone(dev.getZoneID());
        oldZone.removeDevice(devRef);
        // TODO: check if the zone is empty on the dsMeter and remove it in that case
      } catch(std::runtime_error&) {
      }
    }

    dev.setShortAddress(_shortAddress);
    dev.setDSMeter(_dsMeter);
    dev.setZoneID(_zone.getID());
    dev.setFunctionID(functionID);
    dev.setProductID(productID);
    dev.setRevisionID(revisionID);

    try {
      bool locked = m_Interface.isLocked(dev);
      dev.setIsLockedInDSM(locked);

      log(std::string("scanDeviceOnBus:   Device is ") + (locked ? "locked" : "unlocked"));
    } catch(DS485ApiError& e) {
      log(std::string("scanDeviceOnBus: Error getting devices lock state, not aborting scan (") + e.what() + ")", lsWarning);
    }

    dev.resetGroups();
    std::vector<int> groupIDsPerDevice = m_Interface.getGroupsOfDevice(_dsMeter->getBusID(), _shortAddress);
    foreach(int groupID, groupIDsPerDevice) {
      log(std::string("scanDeviceOnBus: adding device ") + intToString(_shortAddress) + " to group " + intToString(groupID));
      dev.addToGroup(groupID);
    }

    _zone.addToDSMeter(_dsMeter);
    _zone.addDevice(devRef);
    _dsMeter->addDevice(devRef);
    dev.setIsPresent(true);

    {
      boost::shared_ptr<Event> readyEvent(new Event("new_device"));
      readyEvent->setProperty("device", dsid.toString());
      readyEvent->setProperty("zone", intToString(_zone.getID()));
      if(DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(readyEvent);
      }
    }

    return true;
  } // scanDeviceOnBus

  bool BusScanner::scanGroupsOfZone(boost::shared_ptr<DSMeter> _dsMeter, Zone& _zone) {
    std::vector<int> groupIDs;
    try {
      groupIDs = m_Interface.getGroups(_dsMeter->getBusID(), _zone.getID());
    } catch(DS485ApiError& e) {
      log("scanDSMeter: Error getting getGroups", lsFatal);
      return false;
    }

    foreach(int groupID, groupIDs) {
      if(groupID == 0) {
        log("scanDSMeter:    Group ID is zero, bailing out... (dsMeterID: "
            + intToString(_dsMeter->getBusID()) +
            "zoneID: " + intToString(_zone.getID()) + ")",
            lsError);
        continue;
      }
      log("scanDSMeter:    Found group with id: " + intToString(groupID));
      if(_zone.getGroup(groupID) == NULL) {
        log(" scanDSMeter:    Adding new group to zone");
        _zone.addGroup(new Group(groupID, _zone.getID(), m_Apartment));
      }
      try {
        Group& group = m_Apartment.getGroup(groupID);
        group.setIsPresent(true);
      } catch(ItemNotFoundException&) {
        Group* pGroup = new Group(groupID, 0, m_Apartment);
        m_Apartment.getZone(0).addGroup(pGroup);
        pGroup->setIsPresent(true);
        log("scanDSMeter:     Adding new group to zone 0");
      }

      // get last called scene for zone, group
      try {
        int lastCalledScene = m_Interface.getLastCalledScene(_dsMeter->getBusID(), _zone.getID(), groupID);
        Group* pGroup = _zone.getGroup(groupID);
        assert(pGroup != NULL);
        log("scanDSMeter: zoneID: " + intToString(_zone.getID()) + " groupID: " + intToString(groupID) + " lastScene: " + intToString(lastCalledScene));
        if(lastCalledScene < 0 || lastCalledScene > MaxSceneNumber) {
          log("scanDSMeter: _sceneID is out of bounds. zoneID: " + intToString(_zone.getID()) + " groupID: " + intToString(groupID) + " scene: " + intToString(lastCalledScene), lsError);
        } else {
          m_Maintenance.onGroupCallScene(_zone.getID(), groupID, lastCalledScene);
        }
      } catch(DS485ApiError& error) {
        log(std::string("scanDSMeter: Error getting last called scene '") + error.what() + "'", lsError);
      }
    }
    return true;
  } // scanGroupsOfZone

  void BusScanner::log(const std::string& _line, aLogSeverity _severity) {
    Logger::getInstance()->log(_line, _severity);
  } // log

} // namespace dss
