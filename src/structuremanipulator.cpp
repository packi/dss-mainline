/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#include "structuremanipulator.h"

#include <stdexcept>
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
#include <ds/str.h>

#include "src/foreach.h"
#include "src/businterface.h"
#include "src/propertysystem.h"
#include "src/model/device.h"
#include "src/model/devicereference.h"
#include "src/model/apartment.h"
#include "src/model/modulator.h"
#include "src/model/zone.h"
#include "src/model/set.h"
#include "src/model/group.h"
#include "src/model/cluster.h"
#include "src/model/modelconst.h"
#include "src/util.h"
#include "src/event.h"
#include "src/dss.h"
#include "src/base.h"
#include "src/messages/vdc-messages.pb.h"

namespace dss {

  void StructureManipulator::createZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_Apartment.getMutex());
    if(!_dsMeter->isPresent()) {
      throw std::runtime_error("Need dsMeter to be present");
    }
    m_Interface.createZone(_dsMeter->getDSID(), _zone->getID());
    _zone->addToDSMeter(_dsMeter);
    _zone->setIsPresent(true);
    _zone->setIsConnected(true);
    std::vector<boost::shared_ptr<Group> > groupList = _zone->getGroups();
    foreach(boost::shared_ptr<Group> group, groupList) {
      group->setIsConnected(true);
    }
    synchronizeGroups(&m_Apartment, &m_Interface);
  } // createZone

  void StructureManipulator::checkSensorsOnDeviceRemoval(Zone &_zone, Device &_device)
  {
    // if the device that is being moved out of the zone was a zone sensor:
    // clear the previous sensor assignment and also check if we can reassign
    // another sensor to the zone
    foreach (auto&& sensorType, _zone.getAssignedSensorTypes(_device)) {
      resetZoneSensor(_zone, sensorType);
    }
    autoAssignZoneSensors(_zone);
  }

  void StructureManipulator::addDeviceToZone(boost::shared_ptr<Device> _device, boost::shared_ptr<Zone> _zone) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_Apartment.getMutex());
    if(!_device->isPresent()) {
      throw std::runtime_error("Need device to be present");
    }

    int oldZoneID = _device->getZoneID();
    boost::shared_ptr<DSMeter> targetDSMeter = m_Apartment.getDSMeterByDSID(_device->getDSMeterDSID());
    if(!_zone->registeredOnDSMeter(targetDSMeter)) {
      try {
        createZone(targetDSMeter, _zone);
      } catch (BusApiError &baer) {
        // this is not fatal, just sync up the model in the dSS
        if (baer.error == ERROR_ZONE_ALREADY_EXISTS) {
          _zone->addToDSMeter(targetDSMeter);
          _zone->setIsPresent(true);
          _zone->setIsConnected(true);
          std::vector<boost::shared_ptr<Group> > groupList = _zone->getGroups();
          foreach(boost::shared_ptr<Group> group, groupList) {
            group->setIsConnected(true);
          }
          synchronizeGroups(&m_Apartment, &m_Interface);
        } else if (baer.error == ERROR_NO_FURTHER_ZONES) {
          // WARNING: do not change this string, it is used and translated in
          // the UI
          throw std::runtime_error("The dSM of this device can not handle more "
                                   "rooms");
        } else {
          // do not ignore other errors that might happen
          throw std::runtime_error(baer.what());
        }
      }
    }

    m_Interface.setZoneID(targetDSMeter->getDSID(), _device->getShortAddress(), _zone->getID());
    _device->setZoneID(_zone->getID());
    DeviceReference ref(_device, &m_Apartment);
    _zone->addDevice(ref);

    {
      boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(ref);
      boost::shared_ptr<Event> mEvent = boost::make_shared<Event>(EventName::DeviceEvent, pDevRef);
      mEvent->setProperty("action", "moved");
      mEvent->setProperty("id", intToString(_zone->getID())); //TODO: remove property in next release
      mEvent->setProperty("oldZoneID", intToString(oldZoneID));
      mEvent->setProperty("newZoneID", intToString(_zone->getID()));
      if(DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(mEvent);
      }
    }

    // update group presence
    for (int g = 0; g < GroupIDStandardMax; g++ ) {
      if (_device->isInGroup(g)) {
        boost::shared_ptr<Group> group = _zone->getGroup(g);
        if (group) {
          group->setIsPresent(true);
        }
      }
    }

    // check if we can remove the zone from the dsMeter
    if(oldZoneID != 0) {
      Logger::getInstance()->log("StructureManipulator::addDeviceToZone: Removing device from old zone " + intToString(oldZoneID), lsInfo);
      boost::shared_ptr<Zone> oldZone = m_Apartment.getZone(oldZoneID);
      oldZone->removeDevice(ref);
      checkSensorsOnDeviceRemoval(*oldZone, *_device);

      Set presentDevicesInZoneOfDSMeter = oldZone->getDevices().getByDSMeter(targetDSMeter).getByPresence(true);
      if(presentDevicesInZoneOfDSMeter.length() == 0) {
        Logger::getInstance()->log("StructureManipulator::addDeviceToZone: Removing zone from meter " + dsuid2str(targetDSMeter->getDSID()), lsInfo);
        try {
          removeZoneOnDSMeter(oldZone, targetDSMeter);
        } catch (std::runtime_error &err) {
          Logger::getInstance()->log(std::string("StructureManipulator::addDeviceToZone: can't remove zone: ") + err.what(), lsInfo);
        }
      }

      // cleanup group presence
      for (int g = 0; g < GroupIDStandardMax; g++ ) {
        if (_device->isInGroup(g)) {
          boost::shared_ptr<Group> group = oldZone->getGroup(g);
          Set presentDevicesInGroup = oldZone->getDevices().getByGroup(group);
          if(presentDevicesInGroup.length() == 0) {
            group->setIsPresent(false);
          }
        }
      }

    } else {
      Logger::getInstance()->log("StructureManipulator::addDeviceToZone: No previous zone...", lsWarning);
    }
    // check if newly added device might need to be assigned as sensor
    autoAssignZoneSensors(*_zone);
  } // addDeviceToZone

  void StructureManipulator::removeZoneOnDSMeter(boost::shared_ptr<Zone> _zone, boost::shared_ptr<DSMeter> _dsMeter) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_Apartment.getMutex());
    Set presentDevicesInZoneOfDSMeter = _zone->getDevices().getByDSMeter(_dsMeter).getByPresence(true);
    if(presentDevicesInZoneOfDSMeter.length() != 0) {
      throw std::runtime_error("cannot delete zone if there are still devices present");
    }

    std::vector<DeviceSpec_t> inactiveDevices =
      m_QueryInterface.getInactiveDevicesInZone(_dsMeter->getDSID(), _zone->getID());

    foreach(const DeviceSpec_t& inactiveDevice, inactiveDevices) {
      boost::shared_ptr<Device> dev;
      try {
        dev = m_Apartment.getDeviceByDSID(inactiveDevice.DSID);
      } catch(ItemNotFoundException&) {
        throw std::runtime_error("Inactive device '" + dsuid2str(inactiveDevice.DSID) + "' not known here.");
      }
      if (dev->getLastKnownDSMeterDSID() != _dsMeter->getDSID()) {
        m_Interface.removeDeviceFromDSMeter(_dsMeter->getDSID(),
                                            inactiveDevice.ShortAddress);
      } else {
        throw std::runtime_error("Inactive device '" + dsuid2str(inactiveDevice.DSID) + "' only known on this meter, can't remove it.");
      }
    }

    try {
      m_Interface.removeZone(_dsMeter->getDSID(), _zone->getID());
      _zone->removeFromDSMeter(_dsMeter);
      if(!_zone->isRegisteredOnAnyMeter()) {
        _zone->setIsConnected(false);
      }
    } catch(BusApiError& e) {
      Logger::getInstance()->log("Can't remove zone " +
                                 intToString(_zone->getID()) + " from meter: "
                                 + dsuid2str(_dsMeter->getDSID()) + ". '" +
                                 e.what() + "'", lsWarning);
    }
  } // removeZoneOnDSMeter

  void StructureManipulator::wipeZoneOnMeter(dsuid_t _meterdSUID, int _zoneID) {
    // check if the zone consists only of inactive devices and remove them
    // if this is indeed the case, then retry to remove the zone
    if (m_QueryInterface.getDevicesCountInZone(_meterdSUID, _zoneID, true) > 0) {
      throw std::runtime_error("Can not remove zone " + intToString(_zoneID) +
                               ": zone contains active devices");
    }

    std::vector<DeviceSpec_t> devices = m_QueryInterface.getDevicesInZone(
            _meterdSUID, _zoneID, false);
    for (size_t i = 0; i < devices.size(); i++) {
      m_Interface.removeDeviceFromDSMeter(_meterdSUID, devices.at(i).ShortAddress);
    }
    m_Interface.removeZone(_meterdSUID, _zoneID);
  } // wipeZoneOnMeter

  void StructureManipulator::removeZoneOnDSMeters(boost::shared_ptr<Zone> _zone) {
    boost::recursive_mutex::scoped_lock scoped_lock(m_Apartment.getMutex());
    try {
      std::vector<boost::shared_ptr<const DSMeter> > meters = _zone->getDSMeters();
      for (size_t s = 0; s < meters.size(); s++) {
        boost::shared_ptr<dss::DSMeter> meter =
            m_Apartment.getDSMeterByDSID(meters.at(s)->getDSID());
        if (meter != NULL) {
          _zone->removeFromDSMeter(meter);
        }
      }

      // the dSM removes the zone only, if the controller is disabled.
      m_Interface.disableZoneHeatingConfig(DSUID_BROADCAST, _zone->getID());

      for (size_t s = 0; s < meters.size(); s++) {
        try {
          m_Interface.removeZone(meters.at(s)->getDSID(), _zone->getID());
        } catch (BusApiError &be) {
          if (be.error == ERROR_ZONE_NOT_EMPTY) {
            wipeZoneOnMeter(meters.at(s)->getDSID(), _zone->getID());
          }
        }
      }

      if(!_zone->isRegisteredOnAnyMeter()) {
        _zone->setIsConnected(false);
      }
    } catch(BusApiError& e) {
      Logger::getInstance()->log("Can't broadcast-remove zone " +
                                 intToString(_zone->getID()) + ": " +
                                 e.what() + "'", lsWarning);
    }
  } // removeZoneOnDSMeters

  void StructureManipulator::removeDeviceFromDSMeter(boost::shared_ptr<Device> _device) {
    dsuid_t dsmDsid = _device->getDSMeterDSID();
    dsuid_t devDsid = _device->getDSID();
    devid_t shortAddr = _device->getShortAddress();

    if (dsmDsid == DSUID_NULL) {
      dsmDsid = _device->getLastKnownDSMeterDSID();
      // #10453 - unknown last dsmid
      if (dsmDsid == DSUID_NULL) {
        Logger::getInstance()->log("StructureManipulator::removeDevicefromDSMeter: device " +
            dsuid2str(_device->getDSID()) + " has invalid dSM ID " + dsuid2str(dsmDsid), lsWarning);
        return;
      }
    }
    if (shortAddr == ShortAddressStaleDevice) {
      shortAddr = _device->getLastKnownShortAddress();
    }

    if ((dsmDsid == DSUID_NULL) || (shortAddr == ShortAddressStaleDevice)) {
      throw std::runtime_error("Not enough data to delete device on dSM");
    }

    // if the dSM is not connected, then there is no point in trying to remove
    // the device from it, this would only cause a timeout and thus introduce
    // an unnecessary delay in the UI/for the JSON API users
    boost::shared_ptr<DSMeter> dsm = m_Apartment.getDSMeterByDSID(_device->getDSMeterDSID());
    if (dsm->isConnected() == false) {
      Logger::getInstance()->log("StructureManipulator::"
        "removeDevicefromDSMeter: not removing device " +
        dsuid2str(_device->getDSID()) + " from dSM " +
        dsuid2str(dsmDsid) +
        ": this dSM is not connected", lsWarning);
      return;
    }

    DeviceSpec_t spec = m_QueryInterface.deviceGetSpec(shortAddr, dsmDsid);
    if (spec.DSID != _device->getDSID()) {
      throw std::runtime_error("Not deleting device - dSID mismatch between dSS model and dSM");
    }

    m_Interface.removeDeviceFromDSMeters(devDsid);
  } // removeDevice

  std::vector<boost::shared_ptr<Device> > StructureManipulator::removeDevice(
      boost::shared_ptr<Device> _pDevice) {
    std::vector<boost::shared_ptr<Device> > result;
    boost::shared_ptr<Device> pPartnerDevice;

    if (_pDevice->is2WayMaster()) {
      dsuid_t next;
      dsuid_get_next_dsuid(_pDevice->getDSID(), &next);
      try {
        pPartnerDevice = m_Apartment.getDeviceByDSID(next);
      } catch(ItemNotFoundException& e) {
        Logger::getInstance()->log("Could not find partner device with dsuid '" + dsuid2str(next) + "'");
      }
    }

    if (_pDevice->isMainDevice() && (_pDevice->getPairedDevices() > 0)) {
      bool doSleep = false;
      dsuid_t next;
      dsuid_t current = _pDevice->getDSID();
      for (int p = 0; p < _pDevice->getPairedDevices(); p++) {
        dsuid_get_next_dsuid(current, &next);
        current = next;
        try {
          boost::shared_ptr<Device> pPartnerDev;
          pPartnerDev = m_Apartment.getDeviceByDSID(next);
          if (doSleep) {
            usleep(500 * 1000); // 500ms
          }
          removeDeviceFromDSMeter(pPartnerDev);
          checkSensorsOnDeviceRemoval(*m_Apartment.getZone(pPartnerDev->getZoneID()), *pPartnerDev);
          m_Apartment.removeDevice(pPartnerDev->getDSID());
          doSleep = true;

          if (pPartnerDev->isVisible()) {
            result.push_back(pPartnerDev);
          }
        } catch(std::runtime_error& e) {
            Logger::getInstance()->log(std::string("Could not find partner device with dsuid '") + dsuid2str(next) + "'");
        }
      }
    }

    try {
      removeDeviceFromDSMeter(_pDevice);
      if (pPartnerDevice != NULL) {
        removeDeviceFromDSMeter(pPartnerDevice);
      }
    } catch (std::runtime_error& e) {
      Logger::getInstance()->log(std::string("Could not remove device from "
                                 "dSM: ") + e.what(), lsError);
    }

    checkSensorsOnDeviceRemoval(*m_Apartment.getZone(_pDevice->getZoneID()), *_pDevice);
    m_Apartment.removeDevice(_pDevice->getDSID());
    result.push_back(_pDevice);

    if (pPartnerDevice != NULL) {
      Logger::getInstance()->log("Also removing partner device " + dsuid2str(pPartnerDevice->getDSID()) + "'");
      checkSensorsOnDeviceRemoval(*m_Apartment.getZone(pPartnerDevice->getZoneID()), *pPartnerDevice);
      m_Apartment.removeDevice(pPartnerDevice->getDSID());
      result.push_back(pPartnerDevice);
    }

    return result;
  }

  void StructureManipulator::deviceSetName(boost::shared_ptr<Device> _pDevice,
                                           const std::string& _name) {
    m_Interface.deviceSetName(_pDevice->getDSMeterDSID(),
                              _pDevice->getShortAddress(), _name);

    {
      DeviceReference ref(_pDevice, &m_Apartment);
      boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(ref);
      boost::shared_ptr<Event> mEvent = boost::make_shared<Event>(EventName::DeviceEvent, pDevRef);
      mEvent->setProperty("action", "name");
      mEvent->setProperty("name", _name);
      if(DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(mEvent);
      }
    }
  } // deviceSetName

  void StructureManipulator::meterSetName(boost::shared_ptr<DSMeter> _pMeter, const std::string& _name) {
    m_Interface.meterSetName(_pMeter->getDSID(), _name);
  } // meterSetName

  int StructureManipulator::persistSet(Set& _set, const std::string& _originalSet) {
    // find next empty user-group
    // TODO: As I understand this persist/create group with devices is only for UserGroups? or also ControlGroups?
    int idFound = -1;
    for(int groupID = GroupIDUserGroupStart; groupID <= GroupIDUserGroupEnd; groupID++) {
      try {
        m_Apartment.getGroup(groupID);
      } catch(ItemNotFoundException&) {
        idFound = groupID;
        break;
      }
    }
    if(idFound != -1) {
      return persistSet(_set, _originalSet, idFound);
    }
    return -1;
  } // persistSet

  int StructureManipulator::persistSet(Set& _set, const std::string& _originalSet, int _groupNumber) {
    boost::shared_ptr<Group> pGroup;
    try {
      pGroup = m_Apartment.getGroup(_groupNumber);
    } catch(ItemNotFoundException&) {
      pGroup = boost::shared_ptr<Group>(
        new Group(_groupNumber, m_Apartment.getZone(0)));
      m_Apartment.getZone(0)->addGroup(pGroup);
      m_Interface.createGroup(0, _groupNumber, ApplicationType::None, 0, "");
      pGroup->setAssociatedSet(_originalSet);
    }
    Logger::getInstance()->log("creating new group " + intToString(_groupNumber));
    assert(pGroup != NULL);
    for(int iDevice = 0; iDevice < _set.length(); iDevice++) {
      boost::shared_ptr<Device> pDevice = _set.get(iDevice).getDevice();
      pDevice->addToGroup(_groupNumber);
      m_Interface.addToGroup(pDevice->getDSMeterDSID(), _groupNumber, pDevice->getShortAddress());
    }
    return _groupNumber;
  } // persistSet

  void StructureManipulator::unpersistSet(std::string _setDescription) {
    std::vector<boost::shared_ptr<Group> > groups = m_Apartment.getZone(0)->getGroups();
    boost::shared_ptr<Group> pGroup;
    for(std::size_t iGroup = 0; iGroup < groups.size(); iGroup++) {
      if(groups[iGroup]->getAssociatedSet() == _setDescription) {
        pGroup = groups[iGroup];
        break;
      }
    }
    if(pGroup != NULL) {
      Set devs = pGroup->getDevices();
      int groupID = pGroup->getID();
      for(int iDevice = 0; iDevice < devs.length(); iDevice++) {
        boost::shared_ptr<Device> pDevice = devs.get(iDevice).getDevice();
        pDevice->removeFromGroup(groupID);
        m_Interface.removeFromGroup(pDevice->getDSMeterDSID(), groupID, pDevice->getShortAddress());
      }
      m_Interface.removeGroup(pGroup->getZoneID(), groupID);
      m_Apartment.getZone(0)->removeGroup(pGroup);
    }
  } // unpersistSet

  void StructureManipulator::createGroup(boost::shared_ptr<Zone> _zone, int _groupNumber, ApplicationType applicationType,
      const int applicationConfiguration, const std::string& _name) {
    if (isDefaultGroup(_groupNumber) || isGlobalAppDsGroup(_groupNumber)) {
      throw DSSException("Group with id " + intToString(_groupNumber) + " is reserved");
    }

    if (isAppUserGroup(_groupNumber)) {
      if (_zone->getID() != 0) {
        throw DSSException("Group with id " + intToString(_groupNumber) + " only allowed in Zone 0");
      }
      try {
        Logger::getInstance()->log(ds::str("Configure user group ", _groupNumber,
            " with standard-id ", applicationType), lsInfo);
        boost::shared_ptr<Cluster> pCluster = m_Apartment.getCluster(_groupNumber);
        pCluster->setName(_name);
        pCluster->setApplicationType(applicationType);
        m_Interface.createCluster( _groupNumber, applicationType, applicationConfiguration, _name);
      } catch (ItemNotFoundException& e) {
        Logger::getInstance()->log("Datamodel-Error creating user group " + intToString(_groupNumber) +
            ": " + e.what(), lsWarning);
      } catch (BusApiError& e) {
        Logger::getInstance()->log("Bus-Error creating user group " + intToString(_groupNumber) +
            ": " + e.what(), lsWarning);
      }
      return;
    }

    if (isZoneUserGroup(_groupNumber)) {
      boost::shared_ptr<Group> pGroup;
      try {
        pGroup = _zone->getGroup(_groupNumber);
        throw DSSException("Group id " + intToString(_groupNumber) + " already exists");
      } catch (ItemNotFoundException& e) {
        Logger::getInstance()->log("Creating user group " + intToString(_groupNumber) +
            " in zone " + intToString(_zone->getID()), lsInfo);
        pGroup = boost::make_shared<Group>(_groupNumber, _zone);
        _zone->addGroup(pGroup);
        m_Interface.createGroup(_zone->getID(), _groupNumber, applicationType, applicationConfiguration, _name);
      }
      return;
    }

    // for groups in User Global Application range, we are locked to zone 0, but we create Groups not clusters
    if (isGlobalAppUserGroup(_groupNumber)) {
      boost::shared_ptr<Group> pGroup;
      if (_zone->getID() != 0) {
        throw DSSException("Group with id " + intToString(_groupNumber) + " only allowed in Zone 0");
      }
      try {
        pGroup = _zone->getGroup(_groupNumber);
        throw DSSException("Group id " + intToString(_groupNumber) + " already exists");
      } catch (ItemNotFoundException& e) {
        Logger::getInstance()->log("Creating user global application group " + intToString(_groupNumber), lsInfo);
        pGroup = boost::make_shared<Group>(_groupNumber, _zone);
        pGroup->setName(_name);
        pGroup->setApplicationType(applicationType);
        _zone->addGroup(pGroup);
        m_Interface.createGroup(_zone->getID(), _groupNumber, applicationType, applicationConfiguration, _name);
      }
      return;
    }

    throw DSSException("Create group: id " + intToString(_groupNumber) + " too large");
  } // createGroup

  void StructureManipulator::removeGroup(boost::shared_ptr<Zone> _zone, int _groupNumber) {
    if (isDefaultGroup(_groupNumber) || isGlobalAppDsGroup(_groupNumber)) {
      throw DSSException("Group with id " + intToString(_groupNumber) + " is reserved");
    }

    if (isAppUserGroup(_groupNumber)) {
      boost::shared_ptr<Cluster> pCluster;
      if (_zone->getID() != 0) {
        throw DSSException("Group with id " + intToString(_groupNumber) + " only allowed in Zone 0");
      }
      try {
        Logger::getInstance()->log("Remove user group " + intToString(_groupNumber), lsInfo);
        pCluster = m_Apartment.getCluster(_groupNumber);
      } catch (ItemNotFoundException& e) {
        throw DSSException("Remove group: id " + intToString(_groupNumber) + " does not exist");
      }
      if (pCluster->isConfigurationLocked()) {
        throw DSSException("The group is locked and cannot be modified");
      }
      try {
        pCluster->reset();
        m_Interface.removeCluster(_groupNumber);
      } catch (ItemNotFoundException& e) {
        Logger::getInstance()->log("Datamodel-Error removing user group " + intToString(_groupNumber) +
            ": " + e.what(), lsWarning);
      } catch (BusApiError& e) {
        Logger::getInstance()->log("Bus-Error removing user group " + intToString(_groupNumber) +
            ": " + e.what(), lsWarning);
      }
      return;
    }

    if (isZoneUserGroup(_groupNumber)) {
      boost::shared_ptr<Group> pGroup;
      try {
        pGroup = _zone->getGroup(_groupNumber);
      } catch (ItemNotFoundException& e) {
        throw DSSException("Group id " + intToString(_groupNumber) + " does not exist");
      }
      try {
        Logger::getInstance()->log("Removing user group " + intToString(_groupNumber) +
          " in zone " + intToString(_zone->getID()), lsInfo);
        _zone->removeGroup(pGroup);
        m_Interface.removeGroup(_zone->getID(), _groupNumber);
      } catch (BusApiError& e) {
        Logger::getInstance()->log("Bus-Error removing user group " + intToString(_groupNumber) +
            ": " + e.what(), lsWarning);
      }
      return;
    }

    // for groups in User Global Application range, we are locked to zone 0, but we remove Groups not clusters
    if (isGlobalAppUserGroup(_groupNumber)) {
      boost::shared_ptr<Group> pGroup;
      if (_zone->getID() != 0) {
        throw DSSException("Group with id " + intToString(_groupNumber) + " only allowed in Zone 0");
      }
      try {
        pGroup = _zone->getGroup(_groupNumber);
      } catch (ItemNotFoundException& e) {
        throw DSSException("Group id " + intToString(_groupNumber) + " does not exist");
      }
      try {
        Logger::getInstance()->log("Removing user global application group " + intToString(_groupNumber), lsInfo);
        _zone->removeGroup(pGroup);
        m_Interface.removeGroup(_zone->getID(), _groupNumber);
      } catch (BusApiError& e) {
        Logger::getInstance()->log("Bus-Error removing user group " + intToString(_groupNumber) +
            ": " + e.what(), lsWarning);
      }
      return;
    }

    throw DSSException("Remove group: id " + intToString(_groupNumber) + " not found");
  } // removeGroup

  void StructureManipulator::groupSetName(boost::shared_ptr<Group> _group,
                                          const std::string& _name) {
    if (isDefaultGroup(_group->getID()) || isGlobalAppDsGroup(_group->getID())) {
      throw DSSException("Group with id " + intToString(_group->getID()) + " is reserved");
    }

    if (isAppUserGroup(_group->getID()) || isZoneUserGroup(_group->getID()) || isGlobalAppUserGroup(_group->getID())) {
      _group->setName(_name);
      m_Interface.groupSetName(_group->getZoneID(), _group->getID(), _name);
      return;
    }

    throw DSSException("Rename group: id " + intToString(_group->getID()) + " too large");
  } // groupSetName

  void StructureManipulator::groupSetApplication(boost::shared_ptr<Group> _group,
      ApplicationType applicationType, const int applicationConfiguration) {
    if (isDefaultGroup(_group->getID()) || isGlobalAppDsGroup(_group->getID())) {
      // for default groups we allow to change the configuration, but we do not allow changing of application type
      if (_group->getApplicationType() != applicationType) {
        throw DSSException("Group with id " + intToString(_group->getID()) + " cannot be modified");
      }
    }

    if (isAppUserGroup(_group->getID())) {
      boost::shared_ptr<Cluster> pCluster = m_Apartment.getCluster(_group->getID());
      clusterSetApplication(pCluster, applicationType, applicationConfiguration);
      return;
    }

    if (isValidGroup(_group->getID())){
      _group->setApplicationType(applicationType);
      _group->setApplicationConfiguration(applicationConfiguration);
      m_Interface.groupSetApplication(_group->getZoneID(), _group->getID(), applicationType, applicationConfiguration);
      return;
    }

    throw DSSException("groupSetApplication group: id " + intToString(_group->getID()) + " not valid");
  } // groupSetApplication

  void StructureManipulator::sceneSetName(boost::shared_ptr<Group> _group,
                                          int _sceneNumber,
                                          const std::string& _name) {
    _group->setSceneName(_sceneNumber, _name);
    m_Interface.sceneSetName(_group->getZoneID(), _group->getID(), _sceneNumber, _name);
    if (DSS::hasInstance()) {
      boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(EventName::SceneNameChanged);
      pEvent->setProperty("zone", intToString(_group->getZoneID()));
      pEvent->setProperty("group", intToString(_group->getID()));
      pEvent->setProperty("sceneId", intToString(_sceneNumber));
      pEvent->setProperty("newSceneName", _name);
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
  } // sceneSetName

  void StructureManipulator::deviceAddToGroup(boost::shared_ptr<Device> _device, boost::shared_ptr<Group> _group) {
    assert(_device.get() != NULL);
    assert(_group.get() != NULL);

    boost::shared_ptr<Cluster> cluster = boost::dynamic_pointer_cast<Cluster> (_group);
    if (cluster && cluster->isConfigurationLocked()) {
      throw DSSException("The group is locked and cannot be modified");  // note: translated in dss-websrc
    }

    m_Interface.addToGroup(_device->getDSMeterDSID(), _group->getID(), _device->getShortAddress());

    // Devices that have a configurable standard group, group bitmask is adjusted by the dSM,
    if ((isDefaultGroup(_group->getID()) || isGlobalAppDsGroup(_group->getID()))
        && ((_device->getFunctionID() & Fid_105_Mask_VariableStandardGroup) > 0)) {
      _device->resetGroups();
    }
    _device->addToGroup(_group->getID());

    // inputs of "simple" devices automatically follow into the new group
    {
      if ((_device->getDeviceType() == DEVICE_TYPE_AKM || _device->getDeviceType() == DEVICE_TYPE_UMR) && (_device->getBinaryInputCount() == 1)) {
        /* AKM with single input, set active group to last group */
        _device->setDeviceBinaryInputTargetId(0, _group->getID());
        _device->setBinaryInputTargetId(0, _group->getID());
      } else if (_device->getOutputMode() == 0) {
        /* device has no output, button is active on group */
        _device->setDeviceButtonActiveGroup(_group->getID());
      }

      if ((_device->getDeviceType() == DEVICE_TYPE_KM) && (_device->getDeviceClass() == DEVICE_CLASS_BL) && (_device->getDeviceNumber() == 200)) {
        // #18333
        // when moved into temperature control group, set ltmode to 0x41
        // when moved into heating group, reset ltmode to 0x0
        if (_group->getID() == GroupIDControlTemperature) {
          _device->setDeviceButtonInputMode(ButtonInputMode::HEATING_PUSHBUTTON);
        } else {
          _device->setDeviceButtonInputMode(ButtonInputMode::STANDARD);
        }
      }
    }
    {
      DeviceReference ref(_device, &m_Apartment);
      boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(ref);
      boost::shared_ptr<Event> mEvent = boost::make_shared<Event>(EventName::DeviceEvent, pDevRef);
      mEvent->setProperty("action", "groupAdd");
      mEvent->setProperty("id", intToString(_group->getID()));
      if(DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(mEvent);
      }
    }
  } // deviceAddToGroup

  void StructureManipulator::deviceRemoveFromGroup(boost::shared_ptr<Device> _device, boost::shared_ptr<Group> _group, bool _adaptInputs) {
    assert(_device.get() != NULL);
    assert(_group.get() != NULL);

    boost::shared_ptr<Cluster> cluster = boost::dynamic_pointer_cast<Cluster> (_group);
    if (cluster && cluster->isConfigurationLocked()) {
      throw DSSException("The group is locked and cannot be modified");
    }

    m_Interface.removeFromGroup(_device->getDSMeterDSID(), _group->getID(), _device->getShortAddress());
    _device->removeFromGroup(_group->getID());

    if (_adaptInputs) {
      if ((_device->getDeviceType() == DEVICE_TYPE_AKM || _device->getDeviceType() == DEVICE_TYPE_UMR) &&
          (_device->getBinaryInputCount() == 1) &&
          (_group->getID() == _device->getBinaryInputs()[0]->m_targetGroupId)) {
        /* AKM with single input, active on removed group */
        _device->setDeviceBinaryInputTargetId(0, GroupIDBlack);
        _device->setBinaryInputTargetId(0, GroupIDBlack);
      }

      if (_group->getID() == _device->getButtonActiveGroup()) {
        /* device has button that is active on removed group */
        _device->setDeviceButtonActiveGroup(_device->getActiveGroup());
      }
    }

    {
      DeviceReference ref(_device, &m_Apartment);
      boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(ref);
      boost::shared_ptr<Event> mEvent = boost::make_shared<Event>(EventName::DeviceEvent, pDevRef);
      mEvent->setProperty("action", "groupRemove");
      mEvent->setProperty("id", intToString(_group->getID()));
      if(DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(mEvent);
      }
    }
  } // deviceRemoveFromGroup

  void StructureManipulator::deviceRemoveFromGroups(boost::shared_ptr<Device> device) {
    boost::shared_ptr<Zone> pZone = m_Apartment.getZone(0);
    for (int g = GroupIDAppUserMin; g <= GroupIDAppUserMax; g++) {
      if (!device->isInGroup(g)) {
        continue;
      }
      boost::shared_ptr<Group> pGroup = pZone->getGroup(g);
      deviceRemoveFromGroup(device, pGroup);
    }
  } // deviceRemoveFromGroups

  bool StructureManipulator::setJokerGroup(boost::shared_ptr<Device> device, int groupId) {
    bool modified = false;

    int oldGroupId = device->getActiveGroup();
    if (oldGroupId == GroupIDNotApplicable) {
      oldGroupId = GroupIDBlack;
    }

    if (oldGroupId != groupId) {
      modified = true;

      // for standard groups force that only one group is active
      for (int g = GroupIDYellow; g <= GroupIDStandardMax; g++) {
        if (g == groupId || GroupIDBlack == g) {
          continue;
        }
        if (device->isInGroup(g)) {
          deviceRemoveFromGroup(device, m_Apartment.getZone(device->getZoneID())->getGroup(g), false);
        }
      }
      // remove from control groups
      for (int g = GroupIDControlGroupMin; g <= GroupIDControlGroupMax; g++) {
        if (device->isInGroup(g)) {
          deviceRemoveFromGroup(device, m_Apartment.getZone(device->getZoneID())->getGroup(g), false);
        }
      }
      // remove also from GA groups
      for (int g = GroupIDGlobalAppMin; g <= GroupIDGlobalAppMax; g++) {
        if (device->isInGroup(g)) {
          deviceRemoveFromGroup(device, m_Apartment.getZone(0)->getGroup(g), false);
        }
      }
      // assign to new "standard" group
      if (groupId >= GroupIDGlobalAppMin) {
        deviceAddToGroup(device, m_Apartment.getZone(0)->getGroup(groupId));
      } else {
        deviceAddToGroup(device, m_Apartment.getZone(device->getZoneID())->getGroup(groupId));
      }
      device->setDeviceJokerGroup(groupId);
    }

    /* check if device is also in a cluster of different application type */
    boost::shared_ptr<Zone> pZone = m_Apartment.getZone(0);
    for (int g = GroupIDAppUserMin; g <= GroupIDAppUserMax; g++) {
      if (!device->isInGroup(g)) {
        continue;
      }
      boost::shared_ptr<Group> itGroup = pZone->getGroup(g);
      if (static_cast<int>(itGroup->getApplicationType()) == groupId) {
        continue;
      }
      deviceRemoveFromGroup(device, itGroup);
      modified = true;
    }

    return modified;
  }

  void StructureManipulator::setZoneHeatingConfigImpl(
      const boost::shared_ptr<const DSMeter>& meter, const boost::shared_ptr<Zone>& zone) {
    Logger::getInstance()->log(ds::str("setZoneHeatingConfigImpl meter:", meter->getDSID(), " zone:", zone->getID(),
                                   " mode:", zone->getHeatingConfig().mode),
        lsNotice);
    // disable temperature controller on all meters
    m_Interface.disableZoneHeatingConfig(DSUID_BROADCAST, zone->getID());

    // enable temperature controller on selected meter
    if (zone->getHeatingProperties().m_mode == HeatingControlMode::PID) {
      m_Interface.setZoneHeatingOperationModes(
          meter->getDSID(), zone->getID(), zone->getHeatingControlOperationModeValues());
    } else if (zone->getHeatingProperties().m_mode == HeatingControlMode::FIXED) {
      m_Interface.setZoneHeatingOperationModes(
          meter->getDSID(), zone->getID(), zone->getHeatingFixedOperationModeValues());
    }
    m_Interface.setZoneHeatingConfig(meter->getDSID(), zone->getID(), zone->getHeatingConfig());
  }

  void StructureManipulator::setZoneHeatingConfig(boost::shared_ptr<Zone> zone, const ZoneHeatingConfigSpec_t& spec) {
    zone->setHeatingConfig(spec);

    zone->updateTemperatureControlMeter();
    if (auto&& meter = zone->tryGetPresentTemperatureControlMeter()) {
      setZoneHeatingConfigImpl(meter, zone);
    } else {
      Logger::getInstance()->log(ds::str("setZoneHeatingConfig No meter present. zone:", zone->getID()), lsWarning);
    }

    if (auto&& modelMaintenance = zone->tryGetModelMaintenance()) {
      ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etControllerConfig, DSUID_BROADCAST);
      pEvent->addParameter(zone->getID());
      pEvent->setSingleObjectParameter(boost::make_shared<ZoneHeatingConfigSpec_t>(spec));
      modelMaintenance->addModelEvent(pEvent);
    }
  }

  void StructureManipulator::updateZoneTemperatureControlMeters() {
    foreach (auto&& zone, m_Apartment.getZones()) {
      if (zone->getID() == 0) {
        continue;
      }

      if (auto meter = zone->tryGetPresentTemperatureControlMeter()) {
        Logger::getInstance()->log(
            ds::str("updateZoneTemperatureControlMeters zone->tryGetPresentTemperatureControlMeter() zone:",
                zone->getID(), " meter:", meter->getDSID()),
            lsDebug);
        continue;
      }

      zone->updateTemperatureControlMeter();
      if (auto&& meter = zone->tryGetPresentTemperatureControlMeter()) {
        setZoneHeatingConfigImpl(meter, zone);
      } else {
        Logger::getInstance()->log(
            ds::str("updateZoneTemperatureControlMeters !meter after update zone:", zone->getID()), lsDebug);
      }
    }
  }

  void StructureManipulator::setZoneSensor(Zone &_zone,
                                           SensorType _sensorType,
                                           boost::shared_ptr<Device> _dev) {
    Logger::getInstance()->log("SensorAssignment:" + std::string(__func__) + " zone:" + intToString(_zone.getID()) + " type: " +
                               sensorTypeName(_sensorType) + " => " + dsuid2str(_dev->getDSID()), lsInfo);

    _zone.setSensor(*_dev, _sensorType);
    m_Interface.setZoneSensor(_zone.getID(), _sensorType, _dev->getDSID());
  }

  void StructureManipulator::resetZoneSensor(Zone &_zone,
                                             SensorType _sensorType) {
    Logger::getInstance()->log("SensorAssignment:" + std::string(__func__) + "  zone:" + intToString(_zone.getID()) + " type: " +
                               sensorTypeName(_sensorType) + " => none", lsInfo);

    _zone.resetSensor(_sensorType);
    m_Interface.resetZoneSensor(_zone.getID(), _sensorType);
  }

  void StructureManipulator::autoAssignZoneSensors(Zone &_zone) {
    Set devices = _zone.getDevices();
    if (devices.isEmpty()) {
      return;
    }

    auto&& unassigned_sensors = _zone.getUnassignedSensorTypes();

    Logger::getInstance()->log("SensorAssignment:" + std::string(__func__) + " zone:" + intToString(_zone.getID()) + " " +
        intToString(unassigned_sensors.size()) + " unassigned sensor types, trying to assign some more... ", lsInfo);


    int assigned = 0;
    // check if our set contains devices that with the matching sensor type
    // and assign the first device that we find automatically: UC 8.1
    foreach (auto&& sensorType, unassigned_sensors) {
      Set devicesBySensor = devices.getBySensorType(sensorType);
      if (devicesBySensor.length() > 0) {
        // select an active sensor
        for (int i = 0; i < devicesBySensor.length(); ++i) {
          if (devicesBySensor.get(i).getDevice()->isPresent()) {
            setZoneSensor(_zone, sensorType, devicesBySensor.get(i).getDevice());
            assigned++;
            break;
          }
        }
        // #13433: removed code that assigned inactive sensors to a zone
      }
    }

    Logger::getInstance()->log("SensorAssignment:" + std::string(__func__) + " zone:" + intToString(_zone.getID()) +
        " employed " + intToString(assigned) + " additional sensor as room sensors", lsInfo);
  }

  void StructureManipulator::synchronizeZoneSensorAssignment(Zone &zone)
  {
    Logger::getInstance()->log("SensorAssignment:" + std::string(__func__) + " zone:" + intToString(zone.getID()) +
                               " roll out sensor configuration to dsms", lsInfo);

    {
      // remove sensors that do not belong to the zone
      zone.removeInvalidZoneSensors();

      // erase all unassigned sensors in zone
      try {
        foreach (auto&& sensorType, zone.getUnassignedSensorTypes()) {
          Logger::getInstance()->log("SensorAssignment:" + std::string(__func__) + " zone:" + intToString(zone.getID()) +
                                     " clear type: " + sensorTypeName(sensorType) + " => none (no available sensor)",
                                     lsInfo);

          m_Interface.resetZoneSensor(zone.getID(), sensorType);
        }

        // reassign all assigned sensors in zone
        foreach (auto&& s, zone.getAssignedSensors()) {
          Logger::getInstance()->log("SensorAssignment:" + std::string(__func__) + " zone:" + intToString(zone.getID()) + " set type: "
                                     + sensorTypeName(s.m_sensorType) + " => " + dsuid2str(s.m_DSUID), lsInfo);

          m_Interface.setZoneSensor(zone.getID(), s.m_sensorType, s.m_DSUID);
        }
      } catch (std::runtime_error &err) {
        Logger::getInstance()->log(std::string("StructureManipulator::") + std::string(__func__) + " " + intToString(zone.getID())
                                   + " failed with exception:" + err.what(), lsWarning);
      }
    }
  }

  void StructureManipulator::clusterSetName(boost::shared_ptr<Cluster> _cluster,
                                            const std::string& _name) {
    if (isAppUserGroup(_cluster->getID())) {
      _cluster->setName(_name);
      m_Interface.clusterSetName(_cluster->getID(), _name);
      return;
    }

    throw DSSException("Rename cluster: id " + intToString(_cluster->getID()) + " not a cluster");
  } // clusterSetName

  void StructureManipulator::clusterSetApplication(boost::shared_ptr<Cluster> _cluster,
      ApplicationType applicationType, const int applicationConfiguration) {
    if (isAppUserGroup(_cluster->getID())) {
      if (_cluster->isConfigurationLocked()) {
        throw DSSException("The group is locked and cannot be modified");
      }
      _cluster->setApplicationType(applicationType);
      _cluster->setApplicationConfiguration(applicationConfiguration);
      m_Interface.clusterSetApplication(_cluster->getID(), applicationType, applicationConfiguration);
      return;
    }

    throw DSSException("clusterSetApplication cluster: id " + intToString(_cluster->getID()) + " not a cluster");
  } // clusterSetStandardID

  void StructureManipulator::clusterSetConfigurationLock(boost::shared_ptr<Cluster> _cluster,
                                                         bool _locked) {
    if (isAppUserGroup(_cluster->getID())) {
      _cluster->setConfigurationLocked(_locked);
      m_Interface.clusterSetConfigurationLock(_cluster->getID(), _locked);
      return;
    }

    throw DSSException("set cluster lock: id " + intToString(_cluster->getID()) + " not a cluster");
  } // clusterSetLockedState

  void StructureManipulator::setProperty(boost::shared_ptr<DSMeter> _dsMeter,
        const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& properties) {
    return m_Interface.setProperty(_dsMeter->getDSID(), properties);
  }

  vdcapi::Message StructureManipulator::getProperty(boost::shared_ptr<DSMeter> _dsMeter,
        const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& query) {
    return m_Interface.getProperty(_dsMeter->getDSID(), query);
  }

} // namespace dss
