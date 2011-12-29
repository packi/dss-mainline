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

#include "src/businterface.h"
#include "src/foreach.h"
#include "src/model/modelconst.h"
#include "src/event.h"
#include "src/dss.h"
#include "src/dsidhelper.h"

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
    DSMeterHash_t hash;

    log("scanDSMeter: Start " + _dsMeter->getDSID().toString() , lsInfo);
    // TODO: implement energy border handling

    try {
      hash = m_Interface.getDSMeterHash(_dsMeter->getDSID());
    } catch(BusApiError& e) {
      log("scanDSMeter: Error getting dSMHash", lsFatal);
      return false;
    }

    if (m_Maintenance.isInitializing() || (_dsMeter->isInitialized() == false) ||
        (hash.Hash != _dsMeter->getDatamodelHash()) ||
        (hash.ModificationCount != _dsMeter->getDatamodelModificationCount())) {

      try {
        DSMeterSpec_t spec = m_Interface.getDSMeterSpec(_dsMeter->getDSID());
        _dsMeter->setArmSoftwareVersion(spec.SoftwareRevisionARM);
        _dsMeter->setDspSoftwareVersion(spec.SoftwareRevisionDSP);
        _dsMeter->setHardwareVersion(spec.HardwareVersion);
        _dsMeter->setApiVersion(spec.APIVersion);
        if (_dsMeter->getName().empty()) {
          _dsMeter->setName(spec.Name);
        }
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

      foreach(int zoneID, zoneIDs) {
        log("scanDSMeter:  Found zone with id: " + intToString(zoneID));
        boost::shared_ptr<Zone> zone = m_Apartment.allocateZone(zoneID);
        zone->addToDSMeter(_dsMeter);
        zone->setIsPresent(true);
        if(!scanZone(_dsMeter, zone)) {
          return false;
        }
      }
      _dsMeter->setIsInitialized(true);
      m_Maintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    }

    _dsMeter->setDatamodelHash(hash.Hash);
    _dsMeter->setDatamodelModificationcount(hash.ModificationCount);
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

  bool BusScanner::scanDeviceOnBus(boost::shared_ptr<DSMeter> _dsMeter, devid_t _shortAddress) {
    DeviceSpec_t spec = m_Interface.deviceGetSpec(_shortAddress, _dsMeter->getDSID());
    boost::shared_ptr<Zone> pZone = m_Apartment.allocateZone(spec.ZoneID);
    return initializeDeviceFromSpec(_dsMeter, pZone, spec);
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

    // remove from groups
    dev->resetGroups();

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
    dev->setOutputMode(_spec.OutputMode);

    dev->setButtonActiveGroup(_spec.ActiveGroup);
    dev->setButtonGroupMembership(_spec.GroupMembership);
    dev->setButtonSetsLocalPriority(_spec.SetsLocalPriority);
    dev->setButtonID(_spec.ButtonID);

    uint8_t inputCount = 0;
    uint8_t inputIndex = 0;
    if((_spec.FunctionID & 0xffc0) == 0x1000) {
      switch(_spec.FunctionID & 0x7) {
      case 0: inputCount = 1; break;
      case 1: inputCount = 2; break;
      case 2: inputCount = 4; break;
      case 7: inputCount = 0; break;
      }
    } else if((_spec.FunctionID & 0x0fc0) == 0x0100) {
      switch(_spec.FunctionID & 0x3) {
      case 0: inputCount = 0; break;
      case 1: inputCount = 1; break;
      case 2: inputCount = 2; break;
      case 3: inputCount = 4; break;
      }
    }
    if(inputCount > 1) {
      inputIndex = (uint8_t)(_spec.DSID.lower & 0xff) % inputCount;
    }
    dev->setButtonInputMode(_spec.LTMode);
    dev->setButtonInputCount(inputCount);
    dev->setButtonInputIndex(inputIndex);

    if(dev->getName().empty()) {
      dev->setName(_spec.Name);
    }

    foreach(int groupID, _spec.Groups) {
      log(std::string("scanDeviceOnBus: adding device ") + intToString(_spec.ShortAddress) + " to group " + intToString(groupID));
      dev->addToGroup(groupID);
    }

    _zone->addToDSMeter(_dsMeter);
    _zone->addDevice(devRef);
    _dsMeter->addDevice(devRef);
    dev->setIsPresent(true);
    dev->setIsConnected(true);

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
      // TODO: get last called scene
      groupOnZone->setLastCalledScene(SceneOff);
      boost::shared_ptr<Group> pGroup;
      try {
        pGroup = m_Apartment.getGroup(groupID);
      } catch(ItemNotFoundException&) {
        boost::shared_ptr<Zone> zoneBroadcast = m_Apartment.getZone(0);
        pGroup.reset(new Group(groupID, zoneBroadcast, m_Apartment));
        zoneBroadcast->addGroup(pGroup);
        log("scanDSMeter:     Adding new group to zone 0");
      }
      pGroup->setIsPresent(true);
    }
    return true;
  } // scanGroupsOfZone

  void BusScanner::log(const std::string& _line, aLogSeverity _severity) {
    Logger::getInstance()->log(_line, _severity);
  } // log

} // namespace dss
