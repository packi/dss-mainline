/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2010 futureLAB AG, Winterthur, Switzerland

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

  bool BusScanner::scanModulator(Modulator& _modulator) {
    _modulator.setIsPresent(true);
    _modulator.setIsValid(false);

    int modulatorID = _modulator.getBusID();

    log("scanModulator: Start " + intToString(modulatorID) , lsInfo);
    std::vector<int> zoneIDs;
    try {
      zoneIDs = m_Interface.getZones(modulatorID);
    } catch(DS485ApiError& e) {
      log("scanModulator: Error getting ZoneIDs", lsFatal);
      return false;
    }

    int levelOrange, levelRed;
    try {
      if(m_Interface.getEnergyBorder(modulatorID, levelOrange, levelRed)) {
        _modulator.setEnergyLevelOrange(levelOrange);
        _modulator.setEnergyLevelRed(levelRed);
      }
    } catch(DS485ApiError& e) {
      log("scanModulator: Error getting EnergyLevels", lsFatal);
      return false;
    }

    try {
      ModulatorSpec_t spec = m_Interface.getModulatorSpec(modulatorID);
      _modulator.setSoftwareVersion(spec.get<1>());
      _modulator.setHardwareVersion(spec.get<2>());
      _modulator.setHardwareName(spec.get<3>());
      _modulator.setDeviceType(spec.get<4>());
    } catch(DS485ApiError& e) {
      log("scanModulator: Error getting dSMSpecs", lsFatal);
      return false;
    }

    bool firstZone = true;
    foreach(int zoneID, zoneIDs) {
      log("scanModulator:  Found zone with id: " + intToString(zoneID));
      Zone& zone = m_Apartment.allocateZone(zoneID);
      zone.addToModulator(_modulator);
      zone.setIsPresent(true);
      if(firstZone) {
        zone.setFirstZoneOnModulator(modulatorID);
        firstZone = false;
      }
      std::vector<int> devices;
      try {
        devices = m_Interface.getDevicesInZone(modulatorID, zoneID);
      } catch(DS485ApiError& e) {
        log("scanModulator: Error getting getDevicesInZone", lsFatal);
        return false;
      }
      foreach(int devID, devices) {
        dsid_t dsid;
        try {
          dsid = m_Interface.getDSIDOfDevice(modulatorID, devID);
        } catch(DS485ApiError& e) {
            log("scanModulator: Error getting getDSIDOfDevice", lsFatal);
            return false;
        }

        int functionID = 0;
        try {
          functionID = m_Interface.deviceGetFunctionID(devID, modulatorID);
        } catch(DS485ApiError& e) {
          log("scanModulator: Error getting cmdGetFunctionID", lsFatal);
          return false;
        }
        log("scanModulator:    Found device with id: " + intToString(devID));
        log("scanModulator:    DSID:        " + dsid.toString());
        log("scanModulator:    Function ID: " + unsignedLongIntToHexString(functionID));
        Device& dev = m_Apartment.allocateDevice(dsid);
        dev.setShortAddress(devID);
        dev.setModulatorID(modulatorID);
        dev.setZoneID(zoneID);
        dev.setFunctionID(functionID);

        std::vector<int> groupIdperDevices = m_Interface.getGroupsOfDevice(modulatorID, devID);
        std::vector<int> groupIDsPerDevice = m_Interface.getGroupsOfDevice(modulatorID,devID);
        foreach(int groupID, groupIDsPerDevice) {
          log(std::string("scanModulator: adding device ") + intToString(devID) + " to group " + intToString(groupID));
          dev.addToGroup(groupID);
        }

        DeviceReference devRef(dev, &m_Apartment);
        zone.addDevice(devRef);
        _modulator.addDevice(devRef);
        dev.setIsPresent(true);
      }
      std::vector<int> groupIDs;
      try {
        groupIDs = m_Interface.getGroups(modulatorID, zoneID);
      } catch(DS485ApiError& e) {
        log("scanModulator: Error getting getGroups", lsFatal);
        return false;
      }

      foreach(int groupID, groupIDs) {
        if(groupID == 0) {
          log("scanModulator:    Group ID is zero, bailing out... (modulatorID: "
              + intToString(modulatorID) +
              "zoneID: " + intToString(zoneID) + ")",
              lsError);
          continue;
        }
        log("scanModulator:    Found group with id: " + intToString(groupID));
        if(zone.getGroup(groupID) == NULL) {
          log(" scanModulator:    Adding new group to zone");
          zone.addGroup(new Group(groupID, zone.getID(), m_Apartment));
        }
        try {
          Group& group = m_Apartment.getGroup(groupID);
          group.setIsPresent(true);
        } catch(ItemNotFoundException&) {
          Group* pGroup = new Group(groupID, 0, m_Apartment);
          m_Apartment.getZone(0).addGroup(pGroup);
          pGroup->setIsPresent(true);
          log("scanModulator:     Adding new group to zone 0");
        }

        // get last called scene for zone, group
        try {
          int lastCalledScene = m_Interface.getLastCalledScene(modulatorID, zoneID, groupID);
          Group* pGroup = zone.getGroup(groupID);
          assert(pGroup != NULL);
          log("scanModulator: zoneID: " + intToString(zoneID) + " groupID: " + intToString(groupID) + " lastScene: " + intToString(lastCalledScene));
          if(lastCalledScene < 0 || lastCalledScene > MaxSceneNumber) {
            log("scanModulator: _sceneID is out of bounds. zoneID: " + intToString(zoneID) + " groupID: " + intToString(groupID) + " scene: " + intToString(lastCalledScene), lsError);
          } else {
            m_Apartment.onGroupCallScene(zoneID, groupID, lastCalledScene);
          }
        } catch(DS485ApiError& error) {
          log(std::string("scanModulator: Error getting last called scene '") + error.what() + "'", lsError);
        }
      }
    }
    _modulator.setIsValid(true);
    return true;

  } // scanModulator

  void BusScanner::log(const std::string& _line, aLogSeverity _severity) {
    Logger::getInstance()->log(_line, _severity);
  }
}
