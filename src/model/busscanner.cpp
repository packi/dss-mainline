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
#include "state.h"
#include "modelmaintenance.h"
#include "src/ds485/dsdevicebusinterface.h"


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
        if ((_dsMeter->getApiVersion() > 0) && (_dsMeter->getApiVersion() < 0x200)) {
          log("scanDSMeter: dSMeter is incompatible", lsWarning);
          _dsMeter->setDatamodelHash(hash.Hash);
          _dsMeter->setDatamodelModificationcount(hash.ModificationCount);
          _dsMeter->setIsPresent(false);
          _dsMeter->setIsValid(true);
          return true;
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
        zone->setIsConnected(true);
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

    // event counter support first implemented in v2.6
    if ((_dsMeter->getApiVersion() > 0) && (_dsMeter->getApiVersion() < 0x206)) {
      hash.EventCount = 0;
    }
    _dsMeter->setHasPendingEvents(((hash.EventCount == 0) ||
        (_dsMeter->getBinaryInputEventCount() != hash.EventCount)));
    _dsMeter->setBinaryInputEventCount(hash.EventCount);
    return true;
  } // scanDSMeter

  bool BusScanner::scanZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone) {
    std::vector<DeviceSpec_t> devices;
    try {
      if ((_dsMeter->getApiVersion() > 0) && (_dsMeter->getApiVersion() < 0x200)) {
        log("scanZone: dSMeter " + _dsMeter->getDSID().toString() + " is incompatible", lsWarning);
      } else {
        devices = m_Interface.getDevicesInZone(_dsMeter->getDSID(), _zone->getID());
        foreach(DeviceSpec_t& spec, devices) {
          initializeDeviceFromSpec(_dsMeter, _zone, spec);
        }
      }
    } catch(BusApiError& e) {
      log("scanZone: Error getDevicesInZone: " + std::string(e.what()), lsFatal);
    }

    return (scanGroupsOfZone(_dsMeter, _zone) && scanStatusOfZone(_dsMeter, _zone));
  } // scanZone

  bool BusScanner::scanDeviceOnBus(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone, devid_t _shortAddress) {
    if ((_dsMeter->getApiVersion() > 0) && (_dsMeter->getApiVersion() < 0x200)) {
      log("scanDeviceOnBus: dSMeter " + _dsMeter->getDSID().toString() + " is incompatible", lsWarning);
      return false;
    }
    DeviceSpec_t spec = m_Interface.deviceGetSpec(_shortAddress, _dsMeter->getDSID());
    return initializeDeviceFromSpec(_dsMeter, _zone, spec);
  } // scanDeviceOnBus

  bool BusScanner::scanDeviceOnBus(boost::shared_ptr<DSMeter> _dsMeter, devid_t _shortAddress) {
    if ((_dsMeter->getApiVersion() > 0) && (_dsMeter->getApiVersion() < 0x200)) {
      log("scanDeviceOnBus: dSMeter " + _dsMeter->getDSID().toString() + " is incompatible", lsWarning);
      return false;
    }
    DeviceSpec_t spec = m_Interface.deviceGetSpec(_shortAddress, _dsMeter->getDSID());
    boost::shared_ptr<Zone> pZone = m_Apartment.allocateZone(spec.ZoneID);
    return initializeDeviceFromSpec(_dsMeter, pZone, spec);
  } // scanDeviceOnBus

  bool BusScanner::initializeDeviceFromSpec(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone, DeviceSpec_t& _spec) {
    log("InitializeDevice: DSID:        " + _spec.DSID.toString());
    log("InitializeDevice: DSM-DSID:    " + _dsMeter->getDSID().toString());
    log("InitializeDevice: Address:     " + intToString(_spec.ShortAddress) +
        ", Active: " + intToString(_spec.ActiveState));
    log("InitializeDevice: Function-ID: " + unsignedLongIntToHexString(_spec.FunctionID) +
        ", Product-ID: " + unsignedLongIntToHexString(_spec.ProductID) +
        ", Revision-ID: " + unsignedLongIntToHexString(_spec.Version)
        );

    boost::shared_ptr<Device> dev;
    try {
      dev = m_Apartment.getDeviceByDSID(_spec.DSID);
      if ((_spec.ActiveState == 0) && dev->isPresent()) {
        // ignore this device if there is already an active one with this dSID
        return false;
      }
    } catch(ItemNotFoundException&) {
      dev = m_Apartment.allocateDevice(_spec.DSID);
    }

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
      if (groupID == 0) {
        continue;
      }
      dev->addToGroup(groupID);
    }

    // synchronize binary input configuration
    dev->setBinaryInputs(dev, _spec.binaryInputs);

    _zone->addToDSMeter(_dsMeter);
    _zone->addDevice(devRef);
    _dsMeter->addDevice(devRef);
    dev->setIsPresent(_spec.ActiveState == 1);
    dev->setIsConnected(true);
    dev->setIsValid(true);

    {
      boost::shared_ptr<Event> readyEvent(new Event("new_device"));
      readyEvent->setProperty("device", _spec.DSID.toString());
      readyEvent->setProperty("zone", intToString(_zone->getID()));
      if(DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(readyEvent);
      }
    }

    scheduleOEMReadout(dev);

    m_Maintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return true;
  } // scanDeviceOnBus

  void BusScanner::scheduleOEMReadout(const boost::shared_ptr<Device> _pDevice) {
    if (_pDevice->isPresent() && (_pDevice->getOemInfoState() == DEVICE_OEM_UNKOWN)) {
      if (_pDevice->getRevisionID() >= 0x0350) {
        log("scheduleOEMReadout: schedule EAN readout for: " + _pDevice->getDSID().toString());
        boost::shared_ptr<DSDeviceBusInterface::OEMDataReader> task;
        std::string connURI = m_Apartment.getBusInterface()->getConnectionURI();
        task = boost::shared_ptr<DSDeviceBusInterface::OEMDataReader>(new DSDeviceBusInterface::OEMDataReader(connURI));
        task->setup(_pDevice);
        boost::shared_ptr<TaskProcessor> pTP = m_Apartment.getModelMaintenance()->getTaskProcessor();
        pTP->addEvent(task);
        _pDevice->setOemInfoState(DEVICE_OEM_LOADING);
      } else {
        _pDevice->setOemInfoState(DEVICE_OEM_NONE);
      }
    } else if (_pDevice->isPresent() &&
                (_pDevice->getOemInfoState() == DEVICE_OEM_VALID) &&
                ((_pDevice->getOemProductInfoState() != DEVICE_OEM_VALID) &&
                 (_pDevice->getOemProductInfoState() != DEVICE_OEM_LOADING))) {
      boost::shared_ptr<ModelMaintenance::OEMWebQuery> task(new ModelMaintenance::OEMWebQuery(_pDevice));
      boost::shared_ptr<TaskProcessor> pTP = m_Apartment.getModelMaintenance()->getTaskProcessor();
      pTP->addEvent(task);
      _pDevice->setOemProductInfoState(DEVICE_OEM_LOADING);
    }
  }

  bool BusScanner::scanGroupsOfZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone) {
    std::vector<GroupSpec_t> groups;
    try {
      groups = m_Interface.getGroups(_dsMeter->getDSID(), _zone->getID());
    } catch (BusApiError& e) {
      log("scanDSMeter: Error getting getGroups", lsFatal);
      return false;
    }

    foreach(GroupSpec_t group, groups) {
      if (group.GroupID == 0) {
        // ignore broadcast group
        continue;
      }

      log("scanDSMeter:    Found group with id: " + intToString(group.GroupID) +
          " and devices: " + intToString(group.NumberOfDevices));

      // apartment-wide unique standard- and user-groups published in zone<0>
      boost::shared_ptr<Group> pGroup;
      boost::shared_ptr<Group> groupOnZone;

      if (group.GroupID <= 15) {

        groupOnZone = _zone->getGroup(group.GroupID);
        if (groupOnZone == NULL) {
          log(" scanDSMeter:    Adding new group to zone");
          groupOnZone.reset(new Group(group.GroupID, _zone, m_Apartment));
          groupOnZone->setName(group.Name);
          groupOnZone->setStandardGroupID(group.StandardGroupID);
          _zone->addGroup(groupOnZone);
        }
        groupOnZone->setIsPresent(true);
        groupOnZone->setIsConnected(true);
        groupOnZone->setLastCalledScene(SceneOff);
        groupOnZone->setIsValid(true);

        try {
          pGroup = m_Apartment.getGroup(group.GroupID);
        } catch (ItemNotFoundException&) {
          boost::shared_ptr<Zone> zoneBroadcast = m_Apartment.getZone(0);
          pGroup.reset(new Group(group.GroupID, zoneBroadcast, m_Apartment));
          pGroup->setName(group.Name);
          pGroup->setStandardGroupID(group.StandardGroupID);
          zoneBroadcast->addGroup(pGroup);
        }
        pGroup->setIsPresent(true);
        pGroup->setIsConnected(true);
        pGroup->setLastCalledScene(SceneOff);
        pGroup->setIsValid(true);

      } else if (group.GroupID <= 23) {

        groupOnZone = _zone->getGroup(group.GroupID);
        if (groupOnZone == NULL) {
          log(" scanDSMeter:    Adding new group to zone");
          groupOnZone.reset(new Group(group.GroupID, _zone, m_Apartment));
          groupOnZone->setName(group.Name);
          groupOnZone->setStandardGroupID(group.StandardGroupID);
          _zone->addGroup(groupOnZone);
        } else {
          if (groupOnZone->getName() != group.Name ||
              groupOnZone->getStandardGroupID() != group.StandardGroupID) {
            groupOnZone->setIsSynchronized(false);
          }
        }
        groupOnZone->setIsPresent(true);
        groupOnZone->setIsConnected(true);
        groupOnZone->setLastCalledScene(SceneOff);
        groupOnZone->setIsValid(true);

        try {
          pGroup = m_Apartment.getGroup(group.GroupID);
          // apartment-user-group has no color unassigned
          if (pGroup->getStandardGroupID() == 0 && group.StandardGroupID > 0) {
            pGroup->setName(group.Name);
            pGroup->setStandardGroupID(group.StandardGroupID);
          }
        } catch (ItemNotFoundException&) {
          boost::shared_ptr<Zone> zoneBroadcast = m_Apartment.getZone(0);
          pGroup.reset(new Group(group.GroupID, zoneBroadcast, m_Apartment));
          pGroup->setName(group.Name);
          pGroup->setStandardGroupID(group.StandardGroupID);
          zoneBroadcast->addGroup(pGroup);
        }
        pGroup->setIsPresent(true);
        pGroup->setIsConnected(true);
        pGroup->setLastCalledScene(SceneOff);
        pGroup->setIsValid(true);

        // this zone's apartment-user-group settings do not match the global group (zone 0)
        if (groupOnZone->getName() != pGroup->getName() ||
            groupOnZone->getStandardGroupID() != pGroup->getStandardGroupID()) {
          groupOnZone->setIsSynchronized(false);
        }

      } else {

        groupOnZone = _zone->getGroup(group.GroupID);
        if (groupOnZone == NULL) {
          log(" scanDSMeter:    Adding new group to zone");
          groupOnZone.reset(new Group(group.GroupID, _zone, m_Apartment));
          groupOnZone->setName(group.Name);
          groupOnZone->setStandardGroupID(group.StandardGroupID);
          _zone->addGroup(groupOnZone);
        } else {
          if (groupOnZone->getName() != group.Name ||
              groupOnZone->getStandardGroupID() != group.StandardGroupID) {
            groupOnZone->setIsSynchronized(false);
          }
        }
        groupOnZone->setIsPresent(true);
        groupOnZone->setIsConnected(true);
        groupOnZone->setLastCalledScene(SceneOff);
        groupOnZone->setIsValid(true);

      }
    }
    return true;
  } // scanGroupsOfZone

  bool BusScanner::scanStatusOfZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone) {
    std::vector<std::pair<int, int> > sceneHistory = m_Interface.getLastCalledScenes(_dsMeter->getDSID(), _zone->getID());
    std::vector<std::pair<int, int> >::iterator it;
    for (it = sceneHistory.begin(); it < sceneHistory.end(); it++) {
      int gid = it->first & 0x3f;
      bool isUndo = (it->first & 0x80) > 0;
      bool isForced = (it->first & 0x40) > 0;
      log("Scene History in Zone " + intToString(_zone->getID()) +
          ": group " + intToString(gid) + " - " +
          (isUndo? "undo-scene" : (isForced ? "forced-scene" : "scene")) + " " + intToString(it->second));
      if (gid == 0) {
        std::vector<boost::shared_ptr<Group> > pGroups = _zone->getGroups();
        foreach(boost::shared_ptr<Group> pGroup, pGroups) {
          if (isUndo) {
            pGroup->setLastButOneCalledScene(it->second);
          } else {
            pGroup->setLastCalledScene(it->second);
          }
        }
      } else {
        boost::shared_ptr<Group> pGroup = _zone->getGroup(gid);
        if (pGroup) {
          if (isUndo) {
            pGroup->setLastButOneCalledScene(it->second);
          } else {
            pGroup->setLastCalledScene(it->second);
          }
        }
      }
    }
    return true;
  } // scanStatusOfZone

  void BusScanner::syncBinaryInputStates(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr <Device> _device) {
    std::vector<boost::shared_ptr<Device> > devices;

    if ((_device != NULL) && (_device->getBinaryInputCount() > 0)) {
      // synchronize a single device
      devices.push_back(_device);
    } else {
      // create list of device state providers
      std::vector<boost::shared_ptr<State> > states = m_Apartment.getStates();
      foreach(boost::shared_ptr<State> s, states) {
        if (s->getType() == StateType_Device) {
          boost::shared_ptr<Device> dev = s->getProviderDevice();
          if ((_dsMeter == NULL) || (dev->getDSMeterDSID() == _dsMeter->getDSID())) {
            devices.push_back(s->getProviderDevice());
          }
        }
      }
    }
    sort(devices.begin(), devices.end());
    devices.erase(unique(devices.begin(), devices.end()), devices.end());

    // send a request to each binary input device to resend its current status
    foreach(boost::shared_ptr<Device> dev, devices) {
      if (dev->getBinaryInputCount() == 0) {
        continue;
      }

      uint32_t value = 0;
      int index;
      try {
        value = m_Apartment.getDeviceBusInterface()->getSensorValue(*dev, 0x20);
        for (index = 0; index < dev->getBinaryInputCount(); index++) {
          boost::shared_ptr<State> state = dev->getBinaryInputState(index);
          assert(state != NULL);
          if ((value & (1 << index)) > 0) {
            state->setState(State_Active);
          } else {
            state->setState(State_Inactive);
          }
        }
      } catch (BusApiError& ex) {
        log("Device " + dev->getDSID().toString() + " did not respond to input state query", lsError);
        for (index = 0; index < dev->getBinaryInputCount(); index++) {
          boost::shared_ptr<State> state = dev->getBinaryInputState(index);
          assert(state != NULL);
          state->setState(State_Unkown);
        }
      }
    }
  } // syncBinaryInputStates

  void BusScanner::log(const std::string& _line, aLogSeverity _severity) {
    Logger::getInstance()->log(_line, _severity);
  } // log

} // namespace dss
