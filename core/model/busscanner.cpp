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

#include "modulator.h"
#include "device.h"
#include "group.h"
#include "apartment.h"
#include "zone.h"

namespace dss {
  
  BusScanner::BusScanner(StructureQueryBusInterface& _interface, Apartment& _apartment)
  : m_Apartment(_apartment),
    m_Interface(_interface)
  {
  }

  bool BusScanner::scanDSMeter(DSMeter& _dsMeter) {
    _dsMeter.setIsPresent(true);
    _dsMeter.setIsValid(false);

    int dsMeterID = _dsMeter.getBusID();

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
        _dsMeter.setEnergyLevelOrange(levelOrange);
        _dsMeter.setEnergyLevelRed(levelRed);
      }
    } catch(DS485ApiError& e) {
      log("scanDSMeter: Error getting EnergyLevels", lsFatal);
      return false;
    }

    try {
      DSMeterSpec_t spec = m_Interface.getDSMeterSpec(dsMeterID);
      _dsMeter.setSoftwareVersion(spec.get<1>());
      _dsMeter.setHardwareVersion(spec.get<2>());
      _dsMeter.setHardwareName(spec.get<3>());
      _dsMeter.setDeviceType(spec.get<4>());
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
      std::vector<int> devices;
      try {
        devices = m_Interface.getDevicesInZone(dsMeterID, zoneID);
      } catch(DS485ApiError& e) {
        log("scanDSMeter: Error getting getDevicesInZone", lsFatal);
        return false;
      }
      foreach(int devID, devices) {
        dsid_t dsid;
        try {
          dsid = m_Interface.getDSIDOfDevice(dsMeterID, devID);
        } catch(DS485ApiError& e) {
            log("scanDSMeter: Error getting getDSIDOfDevice", lsFatal);
            return false;
        }

        int functionID = 0;
        try {
          functionID = m_Interface.deviceGetFunctionID(devID, dsMeterID);
        } catch(DS485ApiError& e) {
          log("scanDSMeter: Error getting cmdGetFunctionID", lsFatal);
          return false;
        }
        log("scanDSMeter:    Found device with id: " + intToString(devID));
        log("scanDSMeter:    DSID:        " + dsid.toString());
        log("scanDSMeter:    Function ID: " + unsignedLongIntToHexString(functionID));
        Device& dev = m_Apartment.allocateDevice(dsid);
        dev.setShortAddress(devID);
        dev.setDSMeterID(dsMeterID);
        dev.setZoneID(zoneID);
        dev.setFunctionID(functionID);

        std::vector<int> groupIdperDevices = m_Interface.getGroupsOfDevice(dsMeterID, devID);
        std::vector<int> groupIDsPerDevice = m_Interface.getGroupsOfDevice(dsMeterID,devID);
        foreach(int groupID, groupIDsPerDevice) {
          log(std::string("scanDSMeter: adding device ") + intToString(devID) + " to group " + intToString(groupID));
          dev.addToGroup(groupID);
        }

        DeviceReference devRef(dev, &m_Apartment);
        zone.addDevice(devRef);
        _dsMeter.addDevice(devRef);
        dev.setIsPresent(true);
      }
      std::vector<int> groupIDs;
      try {
        groupIDs = m_Interface.getGroups(dsMeterID, zoneID);
      } catch(DS485ApiError& e) {
        log("scanDSMeter: Error getting getGroups", lsFatal);
        return false;
      }

      foreach(int groupID, groupIDs) {
        if(groupID == 0) {
          log("scanDSMeter:    Group ID is zero, bailing out... (dsMeterID: "
              + intToString(dsMeterID) +
              "zoneID: " + intToString(zoneID) + ")",
              lsError);
          continue;
        }
        log("scanDSMeter:    Found group with id: " + intToString(groupID));
        if(zone.getGroup(groupID) == NULL) {
          log(" scanDSMeter:    Adding new group to zone");
          zone.addGroup(new Group(groupID, zone.getID(), m_Apartment));
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
          int lastCalledScene = m_Interface.getLastCalledScene(dsMeterID, zoneID, groupID);
          Group* pGroup = zone.getGroup(groupID);
          assert(pGroup != NULL);
          log("scanDSMeter: zoneID: " + intToString(zoneID) + " groupID: " + intToString(groupID) + " lastScene: " + intToString(lastCalledScene));
          if(lastCalledScene < 0 || lastCalledScene > MaxSceneNumber) {
            log("scanDSMeter: _sceneID is out of bounds. zoneID: " + intToString(zoneID) + " groupID: " + intToString(groupID) + " scene: " + intToString(lastCalledScene), lsError);
          } else {
            m_Apartment.onGroupCallScene(zoneID, groupID, lastCalledScene);
          }
        } catch(DS485ApiError& error) {
          log(std::string("scanDSMeter: Error getting last called scene '") + error.what() + "'", lsError);
        }
      }
    }
    _dsMeter.setIsValid(true);
    return true;

  } // scanDSMeter

  void BusScanner::log(const std::string& _line, aLogSeverity _severity) {
    Logger::getInstance()->log(_line, _severity);
  }
}
