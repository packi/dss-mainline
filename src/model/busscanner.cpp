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
#include "data_types.h"

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "busscanner.h"

#include <vector>
#include <boost/make_shared.hpp>

#include <digitalSTROM/dsuid.h>

#include "src/businterface.h"
#include "src/foreach.h"
#include "src/model/modelconst.h"
#include "src/model/scenehelper.h"
#include "src/event.h"
#include "src/dss.h"
#include "src/ds485types.h"
#include "security/security.h"

#include "modulator.h"
#include "device.h"
#include "group.h"
#include "cluster.h"
#include "apartment.h"
#include "zone.h"
#include "state.h"
#include "modelmaintenance.h"
#include "structuremanipulator.h"
#include "src/ds485/dsdevicebusinterface.h"
#include "src/ds485/dsbusinterface.h"
#include "vdc-connection.h"


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
    if (!getMeterHash(_dsMeter, hash)) {
      if (!applyMeterSpec(_dsMeter)) {
        // could not get any info about device. We have to retry.
        return false;
      }
      // bus member type can be read. We have to retry for interesting bus devices.
      if (busMemberIsDSMeter(_dsMeter->getBusMemberType())) {
        // for known bus device types retry the readout
        return false;
      }
      // for unknown/uninteresting bus devices
      _dsMeter->setIsValid(true);
      return true;
    }

    uint32_t dsmHash, dsmModCount;
    dsmHash = _dsMeter->getDatamodelHash();
    dsmModCount = _dsMeter->getDatamodelModificationCount();

    log("Scan dS485 device " + dsuid2str(_dsMeter->getDSID()) +
        ": Initializing " + intToString(m_Maintenance.isInitializing()) +
        ", Init Model " + intToString(_dsMeter->isInitialized()) +
        ", Hash Model/dSM " + intToString(dsmHash, true) + "/" + intToString(hash.Hash, true) +
        ", ModCount Model/dSM " + intToString(dsmModCount, true) + "/" + intToString(hash.ModificationCount, true), lsInfo);

    if (applyMeterSpec(_dsMeter)) {
      if ((_dsMeter->getApiVersion() > 0) && (_dsMeter->getApiVersion() < 0x300)) {
        log("scanDSMeter: dSMeter is incompatible", lsWarning);
        _dsMeter->setDatamodelHash(hash.Hash);
        _dsMeter->setDatamodelModificationcount(hash.ModificationCount);
        _dsMeter->setIsPresent(false);
        _dsMeter->setIsValid(true);
        return true;
      }
    } else {
      log("scanDSMeter: Error getting dSMSpecs", lsWarning);
      return false;
    }

    if (m_Maintenance.isInitializing() ||
        (_dsMeter->isInitialized() == false) ||
        (hash.Hash != dsmHash) ||
        (hash.ModificationCount != dsmModCount)) {

      setMeterCapability(_dsMeter);

      if (_dsMeter->getCapability_HasDevices()) {
        std::vector<int> zoneIDs;
        try {
          zoneIDs = m_Interface.getZones(_dsMeter->getDSID());
        } catch(BusApiError& e) {
          log("scanDSMeter: Error getting ZoneIDs", lsWarning);
          return false;
        }
        foreach(int zoneID, zoneIDs) {
          log("scanDSMeter:  Found zone with id: " + intToString(zoneID));
          boost::shared_ptr<Zone> zone = m_Apartment.allocateZone(zoneID);
          zone->addToDSMeter(_dsMeter);
          zone->setIsPresent(true);
          zone->setIsConnected(true);
          if (!scanZone(_dsMeter, zone)) {
            return false;
          }
        }
        scanClusters(_dsMeter);
      }

      _dsMeter->setIsInitialized(true);
      m_Maintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));

    } else {

      log("Doing a quick scan for: " + dsuid2str(_dsMeter->getDSID()), lsInfo);
      if (_dsMeter->getCapability_HasDevices()) {
        std::vector<int> zoneIDs;
        try {
          zoneIDs = m_Interface.getZones(_dsMeter->getDSID());
        } catch(BusApiError& e) {
          log("scanDSMeter: Error getting ZoneIDs", lsWarning);
          return false;
        }
        foreach(int zoneID, zoneIDs) {
          boost::shared_ptr<Zone> zone = m_Apartment.allocateZone(zoneID);
          zone->addToDSMeter(_dsMeter);
          zone->setIsPresent(true);
          zone->setIsConnected(true);
          scanDevicesOfZoneQuick(_dsMeter, zone);
        }
      }

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

  void BusScanner::scanDevicesOfZoneQuick(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone) {
    std::vector<DeviceSpec_t> devices;
    try {
      if ((_dsMeter->getApiVersion() > 0) && (_dsMeter->getApiVersion() < 0x200)) {
        log("scanZone: dSMeter " + dsuid2str(_dsMeter->getDSID()) + " is incompatible", lsWarning);
      } else {
        devices = m_Interface.getDevicesInZone(_dsMeter->getDSID(), _zone->getID(), false);
        foreach(DeviceSpec_t& spec, devices) {
          initializeDeviceFromSpecQuick(_dsMeter, _zone, spec);
        }
      }
    } catch(BusApiError& e) {
      log("scanZone: Error getDevicesInZone: " + std::string(e.what()), lsWarning);
    }
  } // scanDevicesOfZone

  bool BusScanner::scanZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone) {
    std::vector<DeviceSpec_t> devices;
    bool result = true;
    try {
      if ((_dsMeter->getApiVersion() > 0) && (_dsMeter->getApiVersion() < 0x200)) {
        log("scanZone: dSMeter " + dsuid2str(_dsMeter->getDSID()) + " is incompatible", lsWarning);
      } else {
        devices = m_Interface.getDevicesInZone(_dsMeter->getDSID(), _zone->getID());
        foreach(DeviceSpec_t& spec, devices) {
          initializeDeviceFromSpec(_dsMeter, _zone, spec);
        }
      }
    } catch(BusApiError& e) {
      log("scanZone: Error getDevicesInZone: " + std::string(e.what()), lsWarning);
      result = false;
    }

    try {
      result = result && scanGroupsOfZone(_dsMeter, _zone);
    } catch(BusApiError& e) {
      log("scanZone: Error scanGroupsOfZone: " + std::string(e.what()), lsWarning);
    }

    try {
      result = result && scanStatusOfZone(_dsMeter, _zone);
    } catch(BusApiError& e) {
      log("scanZone: Error scanStatusOfZone: " + std::string(e.what()), lsWarning);
    }

    return result;
  } // scanZone

  bool BusScanner::scanClusters(boost::shared_ptr<DSMeter> _dsMeter) {
    std::vector<ClusterSpec_t> clusters;
    try {
      clusters = m_Interface.getClusters(_dsMeter->getDSID());
    } catch (BusApiError& e) {
      log("scanDSMeter: Error getting getClusters", lsWarning);
      return false;
    }

    foreach(ClusterSpec_t cluster, clusters) {

      if (cluster.StandardGroupID > 0) {
        log("scanDSMeter:    Found cluster with id: " + intToString(cluster.GroupID) +
            " and devices: " + intToString(cluster.NumberOfDevices));
      }

      boost::shared_ptr<Cluster> pCluster;

      try {
        assert(isAppUserGroup(cluster.GroupID));
        pCluster = m_Apartment.getCluster(cluster.GroupID);
      } catch (ItemNotFoundException&) {
        boost::shared_ptr<Zone> zoneBroadcast = m_Apartment.getZone(0);
        pCluster.reset(new Cluster(cluster.GroupID, m_Apartment));
        zoneBroadcast->addGroup(pCluster);
      }

      /*
       * if the configuration on the dSS and the dSM is different,
       * synchronize the settings back to the dSMs
       */
      if (!pCluster->equalConfig(cluster)) {
        pCluster->setIsSynchronized(false);
      }

      /*
       * the dSM is the master source for this data. The dSS will take the data
       * from the first dSM that has a non-zero configuration.
       */
      if ((pCluster->getStandardGroupID() == 0) ||
          ((pCluster->getStandardGroupID() > 0) && !pCluster->isReadFromDsm())) {
        pCluster->setStandardGroupID(cluster.StandardGroupID);
        pCluster->setLocation(static_cast<CardinalDirection_t>(cluster.location));
        pCluster->setProtectionClass(static_cast<WindProtectionClass_t>(cluster.protectionClass));
        pCluster->setConfigurationLocked(cluster.configurationLocked);
        pCluster->setLockedScenes(cluster.lockedScenes);
        pCluster->setReadFromDsm(true);
      }

      try {
        ActionRequestInterface &actionRequest(*m_Apartment.getActionRequestInterface());
        if (actionRequest.isOperationLock(_dsMeter->getDSID(), cluster.GroupID)) {
          // only set operation lock if lock is really active,
          // otherwise a state will be created, which is not wanted
          pCluster->setOperationLock(true, coSystemStartup);
        }
      } catch (BusApiError &e) {
        // leave State_Unknown or take lock value from other dsm's
        log(std::string("OperationLock: ") + e.what(), lsWarning);
      }

      pCluster->setIsPresent(true);
      pCluster->setIsConnected(true);
      pCluster->setLastCalledScene(SceneOff);
      pCluster->setIsValid(true);
    }
    return true;
  } // scanGroupsOfZone

  bool BusScanner::scanDeviceOnBus(boost::shared_ptr<DSMeter> _dsMeter, devid_t _shortAddress) {
    if ((_dsMeter->getApiVersion() > 0) && (_dsMeter->getApiVersion() < 0x200)) {
      log("scanDeviceOnBus: dSMeter " + dsuid2str(_dsMeter->getDSID()) + " is incompatible", lsWarning);
      return false;
    }
    DeviceSpec_t spec = m_Interface.deviceGetSpec(_shortAddress, _dsMeter->getDSID());
    boost::shared_ptr<Zone> pZone = m_Apartment.allocateZone(spec.ZoneID);
    return initializeDeviceFromSpec(_dsMeter, pZone, spec);
  } // scanDeviceOnBus

  bool BusScanner::initializeDeviceFromSpec(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone, DeviceSpec_t& _spec) {
    log("InitializeDevice: DSID:        " + dsuid2str(_spec.DSID));
    log("InitializeDevice: DSM-DSID:    " + dsuid2str(_dsMeter->getDSID()));
    log("InitializeDevice: Address:     " + intToString(_spec.ShortAddress) +
        ", Active: " + intToString(_spec.ActiveState));
    log("InitializeDevice: Function-ID: " + unsignedLongIntToHexString(_spec.FunctionID) +
        ", Product-ID: " + unsignedLongIntToHexString(_spec.ProductID) +
        ", Revision-ID: " + unsignedLongIntToHexString(_spec.Version) +
        ", Vendor-ID: " + unsignedLongIntToHexString(_spec.VendorID)
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

    if (_dsMeter && _dsMeter->getBusMemberType() == BusMember_vDC) {
      dev->setVdcDevice(true);
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
    dev->setVendorID(_spec.VendorID);
    dev->setRevisionID(_spec.Version);
    dev->setIsLockedInDSM(_spec.Locked);
    dev->setOutputMode(_spec.OutputMode);

    dev->setButtonActiveGroup(_spec.ActiveGroup);
    dev->setButtonGroupMembership(_spec.GroupMembership);
    dev->setButtonSetsLocalPriority(_spec.SetsLocalPriority);
    dev->setButtonCallsPresent(_spec.CallsPresent);
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
        uint32_t serial;
        if (dsuid_get_serial_number(&_spec.DSID, &serial) == 0) {
          inputIndex = (uint8_t)(serial & 0xff) % inputCount;
        }
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
    if (_spec.binaryInputsValid) {
      dev->setBinaryInputs(dev, _spec.binaryInputs);
    }

    if ((dev->getDeviceType() == DEVICE_TYPE_AKM) &&
        (dev->getBinaryInputCount() > 0) &&
        (dev->getBinaryInput(0)->m_targetGroupType == 0) &&
        (!dev->getGroupBitmask().test(dev->getBinaryInput(0)->m_targetGroupId - 1))) {
      /* group is only added in dSS datamodel, not on the dSM and device */
      dev->addToGroup(dev->getBinaryInput(0)->m_targetGroupId);
    }

    // synchronize sensor configuration
    if (_spec.sensorInputsValid) {
      dev->setSensors(dev, _spec.sensorInputs);
    }

    // synchronize output channel configuration
    if (_spec.outputChannelsValid) {
      dev->setOutputChannels(dev, _spec.outputChannels);
    }

    _zone->addToDSMeter(_dsMeter);
    _zone->addDevice(devRef);
    _dsMeter->addDevice(devRef);
    dev->setIsPresent(_spec.ActiveState == 1);
    dev->setIsConnected(true);
    dev->setIsValid(true);

    if ((dev->getRevisionID() >= TBVersion_OemConfigLock) &&
        (dev->getOemInfoState() == DEVICE_OEM_UNKOWN)){
        // will be reset in OEM Readout
        dev->setConfigLock(true);
    }

    {
      boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(devRef);
      boost::shared_ptr<Event> readyEvent = boost::make_shared<Event>(EventName::DeviceEvent, pDevRef);
      readyEvent->setProperty("action", "ready");
      if(DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(readyEvent);
      }
    }

    scheduleDeviceReadout(dev);

    m_Maintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return true;
  } // scanDeviceOnBus

  bool BusScanner::initializeDeviceFromSpecQuick(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone, DeviceSpec_t& _spec) {
    log("UpdateDevice: DSUID:" + dsuid2str(_spec.DSID) + "active:" + intToString(_spec.ActiveState), lsInfo);

    boost::shared_ptr<Device> dev;
    try {
      dev = m_Apartment.getDeviceByDSID(_spec.DSID);
      if ((_spec.ActiveState == 0) && dev->isPresent()) {
        // ignore this device if there is already an active one with this dSID
        return false;
      }
    } catch(ItemNotFoundException&) {
      return false;
    }

    boost::shared_ptr<DSMeter> oldDSMeter;
    try {
      oldDSMeter = m_Apartment.getDSMeterByDSID(dev->getLastKnownDSMeterDSID());
    } catch(ItemNotFoundException&) {
      return false;
    }

    // compare ds meter. if attached on wrong dsmeter
    // -> reattach to correct one given by _dsMeter
    // reference temporary
    if (oldDSMeter->getDSID() != _dsMeter->getDSID()) {
      return initializeDeviceFromSpec(_dsMeter, _zone, _spec);
    }

    dev->setIsPresent(_spec.ActiveState == 1);
    dev->setIsConnected(true);
    dev->setIsValid(true);

    m_Maintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return true;
  } // initializeDeviceFromSpecQuick

  void BusScanner::scheduleDeviceReadout(const boost::shared_ptr<Device> _pDevice) {
    if (_pDevice->isPresent() && (_pDevice->getOemInfoState() == DEVICE_OEM_UNKOWN)) {
      if (_pDevice->getRevisionID() >= TBVersion_OemEanConfig) {
        log("scheduleOEMReadout: schedule EAN readout for: " +
            dsuid2str(_pDevice->getDSID()));
        boost::shared_ptr<DSDeviceBusInterface::OEMDataReader> task;
        std::string connURI = m_Apartment.getBusInterface()->getConnectionURI();
        task = boost::make_shared<DSDeviceBusInterface::OEMDataReader>(connURI);
        task->setup(_pDevice);
        boost::shared_ptr<TaskProcessor> pTP = m_Apartment.getModelMaintenance()->getTaskProcessor();
        pTP->addEvent(task);
        _pDevice->setOemInfoState(DEVICE_OEM_LOADING);
      } else {
        _pDevice->setOemInfoState(DEVICE_OEM_NONE);
        _pDevice->setConfigLock(false);
      }
    } else if (_pDevice->isPresent() &&
                (_pDevice->getOemInfoState() == DEVICE_OEM_VALID) &&
                ((_pDevice->getOemProductInfoState() != DEVICE_OEM_VALID) &&
                 (_pDevice->getOemProductInfoState() != DEVICE_OEM_LOADING))) {
      boost::shared_ptr<ModelMaintenance::OEMWebQuery> task = boost::make_shared<ModelMaintenance::OEMWebQuery>(_pDevice);
      boost::shared_ptr<TaskProcessor> pTP = m_Apartment.getModelMaintenance()->getTaskProcessor();
      pTP->addEvent(task);
      _pDevice->setOemProductInfoState(DEVICE_OEM_LOADING);
    }

    // read properties of virtual devices connected to a VDC
    boost::shared_ptr<DSMeter> pMeter;
    try {
      pMeter = m_Apartment.getDSMeterByDSID(_pDevice->getDSMeterDSID());
    } catch (ItemNotFoundException& e) {
    }
    if (pMeter && pMeter->getBusMemberType() == BusMember_vDC) {
      boost::shared_ptr<ModelMaintenance::VdcDataQuery> task = boost::make_shared<ModelMaintenance::VdcDataQuery>(_pDevice);
      boost::shared_ptr<TaskProcessor> pTP = m_Apartment.getModelMaintenance()->getTaskProcessor();
      pTP->addEvent(task);
    }
  }

  void BusScanner::setMeterCapability(boost::shared_ptr<DSMeter> _dsMeter) {
    switch (_dsMeter->getBusMemberType()) {
      case BusMember_dSM11:
        {
          _dsMeter->setCapability_HasDevices(true);
          _dsMeter->setCapability_HasMetering(true);
          _dsMeter->setCapability_HasTemperatureControl(false);
        }
        break;
      case BusMember_dSM12:
        {
          _dsMeter->setCapability_HasDevices(true);
          _dsMeter->setCapability_HasMetering(true);
          _dsMeter->setCapability_HasTemperatureControl(true);
        }
        break;
      case BusMember_vDSM:
        {
          _dsMeter->setCapability_HasDevices(false);
          _dsMeter->setCapability_HasMetering(false);
          _dsMeter->setCapability_HasTemperatureControl(true);
        }
        break;
      case BusMember_vDC:
        {
          boost::shared_ptr<VdcSpec_t> props;
          try {
            props = VdcHelper::getCapabilities(_dsMeter->getDSID());
            if (props) {
              _dsMeter->setCapability_HasMetering(props->hasMetering);
              _dsMeter->setHardwareVersion(props->hardwareVersion);
              _dsMeter->setHardwareName(props->model);
              _dsMeter->setSoftwareVersion(props->modelVersion);
            }
          } catch(std::runtime_error& e) {
          }
          _dsMeter->setCapability_HasDevices(true);
          _dsMeter->setCapability_HasTemperatureControl(true); // transparently trapped by the vdSM

          boost::shared_ptr<VdsdSpec_t> spec;
          try {
            spec = VdcHelper::getSpec(_dsMeter->getDSID(), _dsMeter->getDSID());
            if (spec) {
              if (_dsMeter->getName().empty()) {
                _dsMeter->setName(spec->name);
              }
              _dsMeter->setVdcConfigURL(spec->configURL);
            }
          } catch(std::runtime_error& e) {}
        }
        break;
      default:
        {
          _dsMeter->setCapability_HasDevices(false);
          _dsMeter->setCapability_HasMetering(false);
          _dsMeter->setCapability_HasTemperatureControl(false);
        }
        break;
    }
  }

  bool BusScanner::applyMeterSpec(boost::shared_ptr<DSMeter> _dsMeter) {
    try {
      DSMeterSpec_t spec;
      spec = m_Interface.getDSMeterSpec(_dsMeter->getDSID());
      synchronizeDSMeterData(_dsMeter, spec);
    } catch(BusApiError& e) {
      log("applyMeterSpec: Error getting dSMSpecs", lsWarning);
      return false;
    }
    return true;
  }

  bool BusScanner::getMeterHash(boost::shared_ptr<DSMeter> _dsMeter, DSMeterHash_t& _hash) {
    try {
      _hash = m_Interface.getDSMeterHash(_dsMeter->getDSID());
    } catch(BusApiError& e) {
      log("getMeterHash " + dsuid2str(_dsMeter->getDSID()) +
          ": getDSMeterHash Error: " + e.what(), lsWarning);
      return false;
    }
    return true;
  }

  bool BusScanner::scanGroupsOfZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone) {
    std::vector<GroupSpec_t> groups;
    try {
      groups = m_Interface.getGroups(_dsMeter->getDSID(), _zone->getID());
    } catch (BusApiError& e) {
      log("scanDSMeter: Error getting getGroups", lsWarning);
      return false;
    }

    foreach(GroupSpec_t group, groups) {
      if ((group.GroupID == 0) || isAppUserGroup(group.GroupID)) {
        // ignore broadcast group and clusters
        continue;
      }

      log("scanDSMeter:    Found group with id: " + intToString(group.GroupID) +
          " and devices: " + intToString(group.NumberOfDevices));

      // apartment-wide unique standard- and user-groups published in zone<0>
      boost::shared_ptr<Group> pGroup;
      boost::shared_ptr<Group> groupOnZone;

      if (isDefaultGroup(group.GroupID)) {
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
          switch (gid) {
            case GroupIDControlTemperature:
              if (isUndo) {
                pGroup->setLastButOneCalledScene(it->second);
              } else {
                pGroup->setLastCalledScene(it->second);
                _zone->setHeatingOperationMode(it->second);
              }
              break;
            default:
              if (isUndo) {
                pGroup->setLastButOneCalledScene(it->second);
              } else {
                pGroup->setLastCalledScene(it->second);
              }
              break;
          }
        }
      }
    }

    std::bitset<7> states = m_Interface.getZoneStates(_dsMeter->getDSID(), _zone->getID());
    // only for light
    boost::shared_ptr<Group> pGroup = _zone->getGroup(GroupIDYellow);
    // light group will automatically generate the appropriate state
    if ((pGroup) && (pGroup->getState() == State_Unknown)) {
      pGroup->setOnState(coUnknown, states.test(GroupIDYellow-1));
    }

    uint8_t idList[] = { SensorIDTemperatureIndoors,
        SensorIDHumidityIndoors,
        SensorIDCO2Concentration,
        SensorIDBrightnessIndoors };

    for (uint8_t i=0; i < sizeof(idList)/sizeof(uint8_t); i++) {
      dsuid_t sensorDevice;
      try {
        sensorDevice = m_Interface.getZoneSensor(_dsMeter->getDSID(), _zone->getID(), idList[i]);
        DeviceReference devRef = _zone->getDevices().getByDSID(sensorDevice);
        boost::shared_ptr<Device> pDev = devRef.getDevice();

        // check if zone sensor already assigned
        boost::shared_ptr<Device> oldDev = _zone->getAssignedSensorDevice(idList[i]);
        if (oldDev && (oldDev->getDSID() != sensorDevice)) {
          log("Duplicate sensor type " + intToString(idList[i]) +
              " registration on zone " + intToString(_zone->getID()) +
              ": dsuid " + dsuid2str(oldDev->getDSID()) +
              " is zone reference, and dsuid " + dsuid2str(sensorDevice) +
              " is additionally registered on dSM " + dsuid2str(_dsMeter->getDSID()), lsWarning);
        } else {
          _zone->setSensor(pDev, idList[i]);
        }
      } catch (ItemNotFoundException& e) {
        log("Sensor on zone " + intToString(_zone->getID()) +
            " with id " + dsuid2str(sensorDevice) +
            " is not present but assigned as zone reference on dSM " +
            dsuid2str(_dsMeter->getDSID()), lsWarning);
      } catch (BusApiError& e) {
        // not fatal, catch exception here and avoid endless readout
        break;
      } catch (std::runtime_error& e) {
        log("Sensor with id " + dsuid2str(sensorDevice) +
            " cannot be assigned as zone reference: " + e.what(), lsWarning);
      }
    }

    if (_dsMeter->getCapability_HasTemperatureControl()) {
      uint16_t sensorValue;
      uint32_t sensorAge;
      DateTime now, age;
      ZoneHeatingConfigSpec_t hConfig;
      ZoneHeatingStateSpec_t hState;
      ZoneHeatingProperties_t hProp = _zone->getHeatingProperties();

      try {
        memset(&hConfig, 0, sizeof(ZoneHeatingConfigSpec_t));
        hConfig = m_Interface.getZoneHeatingConfig(_dsMeter->getDSID(), _zone->getID());
        hState = m_Interface.getZoneHeatingState(_dsMeter->getDSID(), _zone->getID());

        log(std::string("Heating properties") +
            " for zone " + intToString(_zone->getID()) +
            ": control dsm  " + dsuid2str(hProp.m_HeatingControlDSUID) +
            ", mode " + intToString(hProp.m_HeatingControlMode) +
            ", state " + intToString(hProp.m_HeatingControlState)
            , lsInfo);
        log(std::string("Heating configuration") +
            " for zone " + intToString(_zone->getID()) +
            " on dsm " + dsuid2str(_dsMeter->getDSID()) +
            ": mode " + intToString(hConfig.ControllerMode) +
            ", state " + intToString(hState.State)
            , lsInfo);

      } catch (std::runtime_error& e) {
        log("Error getting heating config from dsm " +
            dsuid2str(_dsMeter->getDSID()) + ": " + e.what(), lsWarning);
        return false;
      }

      // is there a controller running for this zone?
      if ((hState.State == HeatingControlStateIDInternal) ||
          (hState.State == HeatingControlStateIDEmergency)) {
        if (hConfig.ControllerMode > 0) {
            if ((hProp.m_HeatingControlDSUID == DSUID_NULL) ||
                (hProp.m_HeatingControlDSUID == _dsMeter->getDSID())) {
            _zone->setHeatingControlMode(hConfig.ControllerMode,
                hConfig.Offset, hConfig.SourceZoneId, hConfig.ManualValue,
                _dsMeter->getDSID());
              hProp = _zone->getHeatingProperties();
          } else {
            log("Heating controller conflict for zone " + _zone->getName() + "/" + intToString(_zone->getID()) +
                ": active on dsm " + dsuid2str(hProp.m_HeatingControlDSUID) +
                " and a second one this dsm " + dsuid2str(_dsMeter->getDSID()), lsError);
            StructureManipulator manip(
                *(m_Apartment.getBusInterface()->getStructureModifyingBusInterface()),
                m_Interface,
                m_Apartment);
            ZoneHeatingConfigSpec_t disableConfig = hConfig;
            disableConfig.ControllerMode = 0;
            manip.setZoneHeatingConfig(_zone,
                (const dsuid_t) _dsMeter->getDSID(),
                (const ZoneHeatingConfigSpec_t) disableConfig);
          }
        }
      }

      // sync zone settings from the controlling dsm only
      if (hProp.m_HeatingControlDSUID == _dsMeter->getDSID()) {
        ZoneHeatingStatus_t zValues = _zone->getHeatingStatus();
        ZoneSensorStatus_t zSensors = _zone->getSensorStatus();
        try {
          m_Interface.getZoneSensorValue(_dsMeter->getDSID(), _zone->getID(), SensorIDTemperatureIndoors,
              &sensorValue, &sensorAge);
          age = now.addSeconds(-1 * sensorAge);
          if (age > zSensors.m_TemperatureValueTS) {
            _zone->setTemperature(
                SceneHelper::sensorToFloat12(SensorIDTemperatureIndoors, sensorValue), age);
          } else {
            _zone->pushSensor(coSystem, SAC_MANUAL, DSUID_NULL, SensorIDTemperatureIndoors,
                SceneHelper::sensorToFloat12(SensorIDTemperatureIndoors, sensorValue), "");
          }
        } catch (BusApiError& e) {
          log("Error getting heating temperature value on zone " + intToString(_zone->getID()) +
              " from " + dsuid2str(_dsMeter->getDSID()) + ": " + e.what(), lsWarning);
        }
        try {
          m_Interface.getZoneSensorValue(_dsMeter->getDSID(), _zone->getID(), SensorIDRoomTemperatureSetpoint,
              &sensorValue, &sensorAge);
          age = now.addSeconds(-1 * sensorAge);
          if (age > zValues.m_NominalValueTS) {
            _zone->setNominalValue(
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, sensorValue), age);
          } else {
            _zone->pushSensor(coSystem, SAC_MANUAL, DSUID_NULL, SensorIDRoomTemperatureSetpoint,
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, sensorValue), "");
          }
        } catch (BusApiError& e) {
          log("Error reading heating nominal temperature value on zone " + intToString(_zone->getID()) +
              " from " + dsuid2str(_dsMeter->getDSID()) + ": " + e.what(), lsWarning);
        }
        try {
          m_Interface.getZoneSensorValue(_dsMeter->getDSID(), _zone->getID(), SensorIDRoomTemperatureControlVariable,
              &sensorValue, &sensorAge);
          age = now.addSeconds(-1 * sensorAge);
          if (age > zValues.m_ControlValueTS) {
            _zone->setControlValue(
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, sensorValue), age);
          } else {
            _zone->pushSensor(coSystem, SAC_MANUAL, DSUID_NULL, SensorIDRoomTemperatureControlVariable,
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, sensorValue), "");
          }
        } catch (BusApiError& e) {
          log("Error reading heating control value on zone " + intToString(_zone->getID()) +
              " from " + dsuid2str(_dsMeter->getDSID()) + ": " + e.what(), lsWarning);
        }
      }
    }

    return true;
  } // scanStatusOfZone

  void BusScanner::syncBinaryInputStates(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr <Device> _device) {
    boost::shared_ptr<BinaryInputScanner> task;
    std::string connURI = m_Apartment.getBusInterface()->getConnectionURI();
    task = boost::make_shared<BinaryInputScanner>(&m_Apartment, connURI);
    task->setup(_dsMeter, _device);
    boost::shared_ptr<TaskProcessor> pTP = m_Apartment.getModelMaintenance()->getTaskProcessor();
    pTP->addEvent(task);
  } // syncBinaryInputStates

  void BusScanner::synchronizeDSMeterData(boost::shared_ptr<DSMeter> _dsMeter, DSMeterSpec_t &_spec)
  {
    _dsMeter->setArmSoftwareVersion(_spec.SoftwareRevisionARM);
    _dsMeter->setDspSoftwareVersion(_spec.SoftwareRevisionDSP);
    _dsMeter->setHardwareVersion(_spec.HardwareVersion);
    _dsMeter->setApiVersion(_spec.APIVersion);
    _dsMeter->setPropertyFlags(_spec.flags);
    _dsMeter->setBusMemberType(_spec.DeviceType);

    // recover old dSM11. (firmware < 1.8.3.)
    if ((_spec.DeviceType == BusMember_Unknown) && DsmApiIsdSM(_dsMeter->getDSID())) {
      _dsMeter->setBusMemberType(BusMember_dSM11);
    }

    _dsMeter->setApartmentState(_spec.ApartmentState);
    if (_dsMeter->getName().empty()) {
      _dsMeter->setName(_spec.Name);
    }
  } // synchronizeDSMeterData

  void BusScanner::log(const std::string& _line, aLogSeverity _severity) {
    Logger::getInstance()->log(_line, _severity);
  } // log


  BinaryInputScanner::BinaryInputScanner(Apartment* _papartment, const std::string& _busConnection)
    : m_pApartment(_papartment),
      m_busConnection(_busConnection),
      m_dsmApiHandle(NULL)
  {
    m_device.reset();
    m_dsm.reset();
  }

  BinaryInputScanner::~BinaryInputScanner()
  {
    if (m_dsmApiHandle != NULL) {
      DsmApiClose(m_dsmApiHandle);
      DsmApiCleanup(m_dsmApiHandle);
      m_dsmApiHandle = NULL;
    }
  }

  uint16_t BinaryInputScanner::getDeviceSensorValue(const dsuid_t& _dsm,
                            dev_t _device,
                            uint8_t _sensorIndex) const
  {
    if (m_dsmApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    uint16_t retVal;
    int ret = DeviceSensor_get_value_sync(m_dsmApiHandle, _dsm,
        _device,
        _sensorIndex,
        30,
        &retVal);
    DSBusInterface::checkResultCode(ret);
    return retVal;
  }

  uint8_t BinaryInputScanner::getDeviceConfig(const dsuid_t& _dsm,
      dev_t _device,
      uint8_t _configClass,
      uint8_t _configIndex) const
  {
    if (m_dsmApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    uint8_t retVal;
    int ret = DeviceConfig_get_sync_8(m_dsmApiHandle, _dsm,
        _device,
        _configClass,
        _configIndex,
        30,
        &retVal);
    DSBusInterface::checkResultCode(ret);
    return retVal;
  }

  void BinaryInputScanner::run() {

    Logger::getInstance()->log("BinaryInputScanner run:"
        " dsm = " + (m_dsm ? dsuid2str(m_dsm->getDSID()) : "NULL") +
        ", dev = " + (m_device ? dsuid2str(m_device->getDSID()) : "NULL"), lsDebug);

    if (DSS::hasInstance()) {
      DSS::getInstance()->getSecurity().loginAsSystemUser("BinaryInputScanner needs system-rights");
    }

    m_dsmApiHandle = DsmApiInitialize();
    if (!m_dsmApiHandle) {
      throw std::runtime_error("BinaryInputScanner: Unable to init dsmapi handle");
    }

    int result = DsmApiOpen(m_dsmApiHandle, m_busConnection.c_str(), 0);
    if (result < 0) {
      throw std::runtime_error(std::string("BinaryInputScanner: Unable to open connection to: ") + m_busConnection);
    }

    std::vector<boost::shared_ptr<Device> > devices;

    if (m_device != NULL) {
      // synchronize a single device
      if (m_device->isPresent() && m_device->getBinaryInputCount() > 0) {
        devices.push_back(m_device);
      }
    } else if (m_dsm != NULL) {
      Set D = m_dsm->getDevices();
      BinaryInputDeviceFilter filter;
      D.perform(filter);
      devices = filter.getDeviceList();
    } else {
      Set D = m_pApartment->getDevices();
      BinaryInputDeviceFilter filter;
      D.perform(filter);
      devices = filter.getDeviceList();
    }

    // send a request to each binary input device to resend its current status
    foreach(boost::shared_ptr<Device> dev, devices) {
      uint16_t value = 0;

      try {

        value = getDeviceSensorValue(dev->getDSMeterDSID(),
                                     dev->getShortAddress(), 0x20);
        Logger::getInstance()->log("BinaryInputScanner: device " +
                  dsuid2str(dev->getDSID()) + ", state = " + intToString(value), lsDebug);

        for (int index = 0; index < dev->getBinaryInputCount(); index++) {
          boost::shared_ptr<State> state = dev->getBinaryInputState(index);
          assert(state != NULL);
          if ((value & (1 << index)) > 0) {
            state->setState(coSystem, State_Active);
          } else {
            state->setState(coSystem, State_Inactive);
          }
        }

        // avoid bus monopolization
        sleep(5);

      } catch (BusApiError& ex) {
        Logger::getInstance()->log("BinaryInputScanner: device " + dsuid2str(dev->getDSID())
              + " did not respond to input state query", lsWarning);
        for (int index = 0; index < dev->getBinaryInputCount(); index++) {
          boost::shared_ptr<State> state = dev->getBinaryInputState(index);
          assert(state != NULL);
          state->setState(coSystem, State_Unknown);
        }
      }
    }
  }

  void BinaryInputScanner::setup(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Device> _device)
  {
    m_dsm = _dsMeter;
    m_device = _device;
  }

} // namespace dss
