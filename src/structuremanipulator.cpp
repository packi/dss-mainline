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

#include "structuremanipulator.h"

#include <boost/ref.hpp>

#include "src/foreach.h"
#include "src/businterface.h"
#include "src/mutex.h"
#include "src/propertysystem.h"
#include "src/model/device.h"
#include "src/model/devicereference.h"
#include "src/model/apartment.h"
#include "src/model/modulator.h"
#include "src/model/zone.h"
#include "src/model/set.h"
#include "src/model/group.h"
#include "src/model/modelconst.h"
#include "src/model/scenehelper.h"
#include "src/dsidhelper.h"
#include "src/util.h"
#include "src/event.h"
#include "src/dss.h"

#include <stdexcept>
#include <digitalSTROM/ds.h>

namespace dss {

  void StructureManipulator::createZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone) {
    if(m_Apartment.getPropertyNode() != NULL) {
      m_Apartment.getPropertyNode()->checkWriteAccess();
    }
    AssertLocked apartmentLocked(&m_Apartment);
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

  void StructureManipulator::checkSensorsOnDeviceRemoval(
          boost::shared_ptr<Zone> _zone, boost::shared_ptr<Device> _device) {
    // if the device that is being moved out of the zone was a zone sensor:
    // clear the previous sensor assignment and also check if we can reassign
    // another sensor to the zone
    boost::shared_ptr<std::vector<int> > types_to_clear = _zone->getAssignedSensorTypes(_device);
    for (size_t q = 0; q < types_to_clear->size(); ++q) {
      int sensorType = types_to_clear->at(q);
      resetZoneSensor(_zone, sensorType);
    }
    autoAssignZoneSensors(_zone);
  }

  void StructureManipulator::addDeviceToZone(boost::shared_ptr<Device> _device, boost::shared_ptr<Zone> _zone) {
    if(_zone->getPropertyNode() != NULL) {
      _zone->getPropertyNode()->checkWriteAccess();
    }
    if(_device->getPropertyNode() != NULL) {
      _device->getPropertyNode()->checkWriteAccess();
    }
    AssertLocked apartmentLocked(&m_Apartment);
    if(!_device->isPresent()) {
      throw std::runtime_error("Need device to be present");
    }

    int oldZoneID = _device->getZoneID();
    boost::shared_ptr<DSMeter> targetDSMeter = m_Apartment.getDSMeterByDSID(_device->getDSMeterDSID());
    if(!_zone->registeredOnDSMeter(targetDSMeter)) {
      createZone(targetDSMeter, _zone);
    }

    m_Interface.setZoneID(targetDSMeter->getDSID(), _device->getShortAddress(), _zone->getID());
    _device->setZoneID(_zone->getID());
    DeviceReference ref(_device, &m_Apartment);
    _zone->addDevice(ref);

    {
      boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(ref);
      boost::shared_ptr<Event> mEvent = boost::make_shared<Event>("DeviceEvent", pDevRef);
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
      checkSensorsOnDeviceRemoval(oldZone, _device);

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
    autoAssignZoneSensors(_zone);
  } // addDeviceToZone

  void StructureManipulator::removeZoneOnDSMeter(boost::shared_ptr<Zone> _zone, boost::shared_ptr<DSMeter> _dsMeter) {
    AssertLocked apartmentLocked(&m_Apartment);
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
      dsuid_t tmp_last = dev->getLastKnownDSMeterDSID();
      dsuid_t tmp_dsm = _dsMeter->getDSID();
      if (!IsEqualDsuid(tmp_last, tmp_dsm)) {
        m_Interface.removeDeviceFromDSMeter(_dsMeter->getDSID(), inactiveDevice.ShortAddress);
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

  void StructureManipulator::removeZoneOnDSMeters(boost::shared_ptr<Zone> _zone) {
    AssertLocked apartmentLocked(&m_Apartment);
    try {
      dsuid_t broadcastDSID;
      SetBroadcastDsuid(broadcastDSID);

      std::vector<boost::shared_ptr<const DSMeter> > meters = _zone->getDSMeters();
      for (size_t s = 0; s < meters.size(); s++) {
        boost::shared_ptr<dss::DSMeter> meter =
            m_Apartment.getDSMeterByDSID(meters.at(s)->getDSID());
        if (meter != NULL) {
          _zone->removeFromDSMeter(meter);
        }
      }
      m_Interface.removeZone(broadcastDSID, _zone->getID());
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

    if (IsNullDsuid(dsmDsid)) {
      dsmDsid = _device->getLastKnownDSMeterDSID();
    }
    if (shortAddr == ShortAddressStaleDevice) {
      shortAddr = _device->getLastKnownShortAddress();
    }

    if (IsNullDsuid(dsmDsid) || (shortAddr == ShortAddressStaleDevice)) {
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
    dsuid_t tmp_dsid = _device->getDSID();
    if (!IsEqualDsuid(spec.DSID, tmp_dsid)) {
      throw std::runtime_error("Not deleting device - dSID mismatch between dSS model and dSM");
    }
    checkSensorsOnDeviceRemoval(m_Apartment.getZone(_device->getZoneID()),
                                _device);
    m_Interface.removeDeviceFromDSMeters(devDsid);
  } // removeDevice

  void StructureManipulator::deviceSetName(boost::shared_ptr<Device> _pDevice,
                                           const std::string& _name) {
    m_Interface.deviceSetName(_pDevice->getDSMeterDSID(),
                              _pDevice->getShortAddress(), _name);

    {
      DeviceReference ref(_pDevice, &m_Apartment);
      boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(ref);
      boost::shared_ptr<Event> mEvent = boost::make_shared<Event>("DeviceEvent", pDevRef);
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
    int idFound = -1;
    for(int groupID = GroupIDUserGroupStart; groupID <= GroupIDMax; groupID++) {
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
    if(m_Apartment.getPropertyNode() != NULL) {
      m_Apartment.getPropertyNode()->checkWriteAccess();
    }
    boost::shared_ptr<Group> pGroup;
    try {
      pGroup = m_Apartment.getGroup(_groupNumber);
    } catch(ItemNotFoundException&) {
      pGroup = boost::shared_ptr<Group>(
        new Group(_groupNumber, m_Apartment.getZone(0), m_Apartment));
      m_Apartment.getZone(0)->addGroup(pGroup);
      m_Interface.createGroup(0, _groupNumber, 0, "");
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
    if(m_Apartment.getPropertyNode() != NULL) {
      m_Apartment.getPropertyNode()->checkWriteAccess();
    }
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

  void StructureManipulator::createGroup(boost::shared_ptr<Zone> _zone, int _groupNumber, const int _standardGroupNumber, const std::string& _name) {
    if(m_Apartment.getPropertyNode() != NULL) {
      m_Apartment.getPropertyNode()->checkWriteAccess();
    }
    boost::shared_ptr<Group> pGroup;

    if (isDefaultGroup(_groupNumber)) {
      throw DSSException("Group with id " + intToString(_groupNumber) + " is reserved");
    }

    if (isAppUserGroup(_groupNumber)) {
      if (_zone->getID() != 0) {
        throw DSSException("Group with id " + intToString(_groupNumber) + " only allowed in Zone 0");
      }
      try {
        Logger::getInstance()->log("Configure user group " + intToString(_groupNumber) +
            " with standard-id " + intToString(_standardGroupNumber), lsInfo);
        pGroup = m_Apartment.getGroup(_groupNumber);
        pGroup->setName(_name);
        pGroup->setStandardGroupID(_standardGroupNumber);
        std::vector<boost::shared_ptr<Zone> > zones = m_Apartment.getZones();
        foreach(boost::shared_ptr<Zone> pZone, zones) {
          if (pZone->getID() == 0 || !pZone->isConnected()) {
            continue;
          }
          pGroup = pZone->getGroup(_groupNumber);
          if (pGroup == NULL) {
            pGroup = boost::make_shared<Group>(_groupNumber, m_Apartment.getZone(0), boost::ref<Apartment>(m_Apartment));
            pGroup->setIsValid(true);
            m_Apartment.getZone(0)->addGroup(pGroup);
            m_Interface.createGroup(pZone->getID(), _groupNumber, _standardGroupNumber, _name);
          } else {
            pGroup->setIsValid(true);
            m_Interface.groupSetName(pZone->getID(), _groupNumber, _name);
            m_Interface.groupSetStandardID(pZone->getID(), _groupNumber, _standardGroupNumber);
          }
          pGroup->setName(_name);
          pGroup->setStandardGroupID(_standardGroupNumber);
        }
      } catch (ItemNotFoundException& e) {
        Logger::getInstance()->log("Datamodel-Error creating user group " + intToString(_groupNumber) +
            ": " + e.what(), lsWarning);
      } catch (BusApiError& e) {
        Logger::getInstance()->log("Bus-Error creating user group " + intToString(_groupNumber) +
            ": " + e.what(), lsWarning);
      }
      return;
    }

    if (_groupNumber <= 31) {
      try {
        pGroup = _zone->getGroup(_groupNumber);
        throw DSSException("Group id " + intToString(_groupNumber) + " already exists");
      } catch (ItemNotFoundException& e) {
        Logger::getInstance()->log("Creating user group " + intToString(_groupNumber) +
            " in zone " + intToString(_zone->getID()), lsInfo);
        pGroup = boost::make_shared<Group>(_groupNumber, _zone, boost::ref<Apartment>(m_Apartment));
        _zone->addGroup(pGroup);
        m_Interface.createGroup(_zone->getID(), _groupNumber, _standardGroupNumber, _name);
      }
      return;
    }

    throw DSSException("Create group: id " + intToString(_groupNumber) + " too large");
  } // createGroup

  void StructureManipulator::removeGroup(boost::shared_ptr<Zone> _zone, int _groupNumber) {
    if(m_Apartment.getPropertyNode() != NULL) {
      m_Apartment.getPropertyNode()->checkWriteAccess();
    }
    boost::shared_ptr<Group> pGroup;

    if (isDefaultGroup(_groupNumber)) {
      throw DSSException("Group with id " + intToString(_groupNumber) + " is reserved");
    }

    if (isAppUserGroup(_groupNumber)) {
      if (_zone->getID() != 0) {
        throw DSSException("Group with id " + intToString(_groupNumber) + " only allowed in Zone 0");
      }
      try {
        Logger::getInstance()->log("Remove user group " + intToString(_groupNumber), lsInfo);
        pGroup = m_Apartment.getGroup(_groupNumber);
      } catch (ItemNotFoundException& e) {
        throw DSSException("Remove group: id " + intToString(_groupNumber) + " does not exist");
      }
      try {
        pGroup->setName("");
        pGroup->setStandardGroupID(0);
        std::vector<boost::shared_ptr<Zone> > zones = m_Apartment.getZones();
        foreach(boost::shared_ptr<Zone> pZone, zones) {
          if (pZone->getID() == 0 || !pZone->isConnected()) {
            continue;
          }
          pGroup = pZone->getGroup(_groupNumber);
          if (pGroup == NULL) {
            continue;
          }
          pGroup->setName("");
          pGroup->setStandardGroupID(0);
          m_Interface.groupSetName(pZone->getID(), _groupNumber, "");
          m_Interface.groupSetStandardID(pZone->getID(), _groupNumber, 0);
        }
      } catch (ItemNotFoundException& e) {
        Logger::getInstance()->log("Datamodel-Error removing user group " + intToString(_groupNumber) +
            ": " + e.what(), lsWarning);
      } catch (BusApiError& e) {
        Logger::getInstance()->log("Bus-Error removing user group " + intToString(_groupNumber) +
            ": " + e.what(), lsWarning);
      }
      return;
    }

    if (_groupNumber <= 31) {
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

    throw DSSException("Remove group: id " + intToString(_groupNumber) + " too large");
  } // removeGroup

  void StructureManipulator::groupSetName(boost::shared_ptr<Group> _group,
                                          const std::string& _name) {
    if (isDefaultGroup(_group->getID())) {
      throw DSSException("Group with id " + intToString(_group->getID()) + " is reserved");
    }

    if (isAppUserGroup(_group->getID())) {
      _group->setName(_name);
      try {
        boost::shared_ptr<Group> pGroup;
        std::vector<boost::shared_ptr<Zone> > zones = m_Apartment.getZones();
        foreach(boost::shared_ptr<Zone> pZone, zones) {
          if (pZone->getID() == 0 || !pZone->isConnected()) {
            continue;
          }
          pGroup = pZone->getGroup(_group->getID());
          if (pGroup == NULL) {
            continue;
          }
          pGroup->setName(_name);
          m_Interface.groupSetName(pZone->getID(), _group->getID(), _name);
        }
      } catch (ItemNotFoundException& e) {
        Logger::getInstance()->log("Datamodel-Error setting user group " + intToString(_group->getID()) +
            " name: " + e.what(), lsWarning);
      } catch (BusApiError& e) {
        Logger::getInstance()->log("Bus-Error removing user group " + intToString(_group->getID()) +
            " name: " + e.what(), lsWarning);
      }
      return;
    }

    if (_group->getID() <= 31) {
      _group->setName(_name);
      m_Interface.groupSetName(_group->getZoneID(), _group->getID(), _name);
    }

    throw DSSException("Remove group: id " + intToString(_group->getID()) + " too large");
  } // groupSetName

  void StructureManipulator::groupSetStandardID(boost::shared_ptr<Group> _group,
                                                const int _standardGroupNumber) {
    if (isDefaultGroup(_group->getID())) {
      throw DSSException("Group with id " + intToString(_group->getID()) + " is reserved");
    }

    if (isAppUserGroup(_group->getID())) {
      _group->setStandardGroupID(_standardGroupNumber);
      try {
        boost::shared_ptr<Group> pGroup;
        std::vector<boost::shared_ptr<Zone> > zones = m_Apartment.getZones();
        foreach(boost::shared_ptr<Zone> pZone, zones) {
          if (pZone->getID() == 0 || !pZone->isConnected()) {
            continue;
          }
          pGroup = pZone->getGroup(_group->getID());
          if (pGroup == NULL) {
            continue;
          }
          pGroup->setStandardGroupID(_standardGroupNumber);
          m_Interface.groupSetStandardID(pZone->getID(), _group->getID(), _standardGroupNumber);
        }
      } catch (ItemNotFoundException& e) {
        Logger::getInstance()->log("Datamodel-Error setting user group " + intToString(_group->getID()) +
            " name: " + e.what(), lsWarning);
      } catch (BusApiError& e) {
        Logger::getInstance()->log("Bus-Error removing user group " + intToString(_group->getID()) +
            " name: " + e.what(), lsWarning);
      }
      return;
    }

    if (_group->getID() <= 31) {
      _group->setStandardGroupID(_standardGroupNumber);
      m_Interface.groupSetStandardID(_group->getZoneID(), _group->getID(), _standardGroupNumber);
    }

    throw DSSException("Remove group: id " + intToString(_group->getID()) + " too large");
  } // groupSetStandardID

  void StructureManipulator::sceneSetName(boost::shared_ptr<Group> _group,
                                          int _sceneNumber,
                                          const std::string& _name) {
    _group->setSceneName(_sceneNumber, _name);
    m_Interface.sceneSetName(_group->getZoneID(), _group->getID(), _sceneNumber, _name);
  } // sceneSetName

  void StructureManipulator::deviceAddToGroup(boost::shared_ptr<Device> _device, boost::shared_ptr<Group> _group) {
    if(m_Apartment.getPropertyNode() != NULL) {
      m_Apartment.getPropertyNode()->checkWriteAccess();
    }

    m_Interface.addToGroup(_device->getDSMeterDSID(), _group->getID(), _device->getShortAddress());

    // Devices that have a configurable standard group, group bitmask is adjusted by the dSM,
    if (isDefaultGroup(_group->getID()) && ((_device->getFunctionID() & Fid_105_Mask_VariableStandardGroup) > 0)) {
      _device->resetGroups();
    }
    _device->addToGroup(_group->getID());

    if (isAppUserGroup(_group->getID())) {
      if ((_device->getDeviceType() == DEVICE_TYPE_AKM) && (_device->getBinaryInputCount() == 1)) {
        /* AKM with single input, set active group to last group */
        _device->setDeviceBinaryInputTarget(0, 0, _group->getID());
        _device->setBinaryInputTarget(0, 0, _group->getID());
      } else if (_device->getOutputMode() == 0) {
        /* device has no output, button is active on group */
        _device->setDeviceButtonActiveGroup(_group->getID());
      }
    }
    {
      DeviceReference ref(_device, &m_Apartment);
      boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(ref);
      boost::shared_ptr<Event> mEvent = boost::make_shared<Event>("DeviceEvent", pDevRef);
      mEvent->setProperty("action", "groupAdd");
      mEvent->setProperty("id", intToString(_group->getID()));
      if(DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(mEvent);
      }
    }
  } // deviceAddToGroup

  void StructureManipulator::deviceRemoveFromGroup(boost::shared_ptr<Device> _device, boost::shared_ptr<Group> _group) {
    if(m_Apartment.getPropertyNode() != NULL) {
      m_Apartment.getPropertyNode()->checkWriteAccess();
    }
    m_Interface.removeFromGroup(_device->getDSMeterDSID(), _group->getID(), _device->getShortAddress());
    _device->removeFromGroup(_group->getID());

    if ((_device->getDeviceType() == DEVICE_TYPE_AKM) &&
        (_device->getBinaryInputCount() == 1) &&
        (_group->getID() == _device->getBinaryInputs()[0]->m_targetGroupId)) {
      /* AKM with single input, active on removed group */
      _device->setDeviceBinaryInputTarget(0, 0, GroupIDBlack);
      _device->setBinaryInputTarget(0, 0, GroupIDBlack);
    } else if ((_device->getOutputMode() == 0) && (_group->getID() == _device->getButtonActiveGroup())) {
      /* device has no output, button is active on removed group */
      _device->setDeviceButtonActiveGroup(BUTTON_ACTIVE_GROUP_RESET);
    }
    {
      DeviceReference ref(_device, &m_Apartment);
      boost::shared_ptr<DeviceReference> pDevRef = boost::make_shared<DeviceReference>(ref);
      boost::shared_ptr<Event> mEvent = boost::make_shared<Event>("DeviceEvent", pDevRef);
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
      if (!device->getGroupBitmask().test(g - 1)) {
        continue;
      }
      boost::shared_ptr<Group> pGroup = pZone->getGroup(g);
      deviceRemoveFromGroup(device, pGroup);
    }
  } // deviceRemoveFromGroups

  bool StructureManipulator::setJokerGroup(boost::shared_ptr<Device> device,
                                           boost::shared_ptr<Group> newGroup) {
    bool modified = false;
    int oldGroupId = device->getJokerGroup();
    if (oldGroupId != newGroup->getID()) {
      device->setDeviceJokerGroup(newGroup->getID());
      modified = true;
    }

    /* check if device is also in a colored user group */
    boost::shared_ptr<Zone> pZone = m_Apartment.getZone(newGroup->getZoneID());
    for (int g = GroupIDAppUserMin; g <= GroupIDAppUserMax; g++) {
      if (!device->getGroupBitmask().test(g - 1)) {
        continue;
      }
      boost::shared_ptr<Group> itGroup = pZone->getGroup(g);
      if (itGroup->getStandardGroupID() == newGroup->getID()) {
        continue;
      }
      deviceRemoveFromGroup(device, newGroup);
      modified = true;
    }

    return modified;
  }

  void StructureManipulator::setZoneHeatingConfig(boost::shared_ptr<Zone> _zone,
                                                  const dsuid_t& _ctrlDSUID,
                                                  const ZoneHeatingConfigSpec_t _spec) {
    // TODO: synchronize different and disconnected dSM's

    _zone->setHeatingControlMode(_spec.ControllerMode,_spec.Offset, _spec.SourceZoneId, _spec.ManualValue, _ctrlDSUID);
    m_Interface.setZoneHeatingConfig(_ctrlDSUID, _zone->getID(), _spec);
  } // setZoneHeatingConfig

  void StructureManipulator::setZoneSensor(boost::shared_ptr<Zone> _zone,
                                           const uint8_t _sensorType,
                                           boost::shared_ptr<Device> _dev) {
    _zone->setSensor(_dev, _sensorType);
    m_Interface.setZoneSensor(_zone->getID(), _sensorType, _dev->getDSID());
  }

  void StructureManipulator::resetZoneSensor(boost::shared_ptr<Zone> _zone,
                                             const uint8_t _sensorType) {
    _zone->resetSensor(_sensorType);
    m_Interface.resetZoneSensor(_zone->getID(), _sensorType);
  }

  void StructureManipulator::autoAssignZoneSensors(boost::shared_ptr<Zone> _zone) {
    // don't assign sensor to zone zero
    if (_zone->getID() == 0) {
      return;
    }

    Set devices = _zone->getDevices();
    if (devices.isEmpty()) {
      return;
    }

    boost::shared_ptr<std::vector<int> > unassigned_sensors =
       _zone->getUnassignedSensorTypes();

    // check if our set contains devices that with the matching sensor type
    // and assign the first device that we find automatically: UC 8.1
    for (size_t q = 0; q < unassigned_sensors->size(); ++q) {
      Set devicesBySensor =
          devices.getBySensorType(unassigned_sensors->at(q));
      if (devicesBySensor.length() > 0) {
        // select an active sensor
        bool assigned = false;
        for (int i = 0; i < devicesBySensor.length(); ++i) {
          if (devicesBySensor.get(i).getDevice()->isPresent()) {
            setZoneSensor(_zone, unassigned_sensors->at(q),
                          devicesBySensor.get(i).getDevice());
            assigned = true;
            break;
          }
        }
        if (!assigned) {
          // all sensor inactive. take first one
          setZoneSensor(_zone, unassigned_sensors->at(q),
                        devicesBySensor.get(0).getDevice());
        }
      }
    }
  }

  void StructureManipulator::synchronizeZoneSensorAssignment(std::vector<boost::shared_ptr<Zone> > _zones)
  {
    for (size_t i = 0; i < _zones.size(); ++i) {
      boost::shared_ptr<Zone> zone = _zones.at(i);
      if (!zone) {
        continue;
      }

      // remove sensors that do not belong to the zone
      zone->removeInvalidZoneSensors();

      // erase all unassigned sensors in zone
      boost::shared_ptr<std::vector<int> > sUnasList =  zone->getUnassignedSensorTypes();
      try {
        for (size_t index = 0; index < sUnasList->size(); ++index) {
            m_Interface.resetZoneSensor(zone->getID(), sUnasList->at(index));
        }

        // reassign all assigned sensors in zone
        std::vector<boost::shared_ptr<MainZoneSensor_t> >sAsList = zone->getAssignedSensors();
        for (std::vector<boost::shared_ptr<MainZoneSensor_t> >::iterator it = sAsList.begin();
            it != sAsList.end();
            ++it) {
            m_Interface.setZoneSensor(zone->getID(), (*it)->m_sensorType, (*it)->m_DSUID);
        }
      } catch (std::runtime_error &err) {
        Logger::getInstance()->log(std::string("StructureManipulator::synchronizeZoneSensorAssignment: can't synchronize zone sensors: ") + err.what(), lsWarning);
      }
    }
  }
} // namespace dss
