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

#include "src/foreach.h"
#include "src/businterface.h"
#include "src/mutex.h"
#include "src/propertysystem.h"
#include "src/model/device.h"
#include "src/model/apartment.h"
#include "src/model/modulator.h"
#include "src/model/zone.h"
#include "src/model/set.h"
#include "src/model/group.h"
#include "src/model/modelconst.h"

#include <stdexcept>

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
  } // createZone

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

    // check if we can remove the zone from the dsMeter
    if(oldZoneID != 0) {
      Logger::getInstance()->log("StructureManipulator::addDeviceToZone: Removing device from old zone " + intToString(oldZoneID), lsInfo);
      boost::shared_ptr<Zone> oldZone = m_Apartment.getZone(oldZoneID);
      oldZone->removeDevice(ref);

      Set presentDevicesInZoneOfDSMeter = oldZone->getDevices().getByDSMeter(targetDSMeter).getByPresence(true);
      if(presentDevicesInZoneOfDSMeter.length() == 0) {
        Logger::getInstance()->log("StructureManipulator::addDeviceToZone: Removing zone from meter " + targetDSMeter->getDSID().toString(), lsInfo);
        removeZoneOnDSMeter(oldZone, targetDSMeter);
      }
    } else {
      Logger::getInstance()->log("StructureManipulator::addDeviceToZone: No previous zone...", lsWarning);
    }
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
        throw std::runtime_error("Inactive device '" + inactiveDevice.DSID.toString() + "' not known here.");
      }
      if(dev->getLastKnownDSMeterDSID() != _dsMeter->getDSID()) {
        m_Interface.removeDeviceFromDSMeter(_dsMeter->getDSID(), inactiveDevice.ShortAddress);
      } else {
        throw std::runtime_error("Inactive device '" + inactiveDevice.DSID.toString() + "' only known on this meter, can't remove it.");
      }
    }

    try {
      m_Interface.removeZone(_dsMeter->getDSID(), _zone->getID());
      _zone->removeFromDSMeter(_dsMeter);
      if(_zone->isRegisteredOnAnyMeter()) {
        _zone->setIsPresent(false);
      }
    } catch(BusApiError& e) {
      Logger::getInstance()->log("Can't remove zone " +
                                 intToString(_zone->getID()) + " from meter: "
                                 + _dsMeter->getDSID().toString() + ". '" +
                                 e.what() + "'", lsWarning);
    }
  } // removeZoneOnDSMeter

  void StructureManipulator::removeDeviceFromDSMeter(boost::shared_ptr<Device> _device) {

    if (_device->getDSID().isSimulated()) {
      return;
    }

    dss_dsid_t dsmDsid = _device->getDSMeterDSID();
    devid_t shortAddr = _device->getShortAddress();

    if (dsmDsid == NullDSID) {
      dsmDsid = _device->getLastKnownDSMeterDSID();
    }
    if (shortAddr == ShortAddressStaleDevice) {
      shortAddr = _device->getLastKnownShortAddress();
    }

    if ((dsmDsid == NullDSID) || (shortAddr == ShortAddressStaleDevice)) {
      throw std::runtime_error("Not enough data to delete device on dSM");
    }

    m_Interface.removeDeviceFromDSMeter(dsmDsid, shortAddr);
  } // removeDevice

  void StructureManipulator::sceneSetName(boost::shared_ptr<Group> _group,
                                          int _sceneNumber,
                                          const std::string& _name) {
    m_Interface.sceneSetName(_group->getZoneID(), _group->getID(), _sceneNumber,
                             _name);
  } // sceneSetName

  void StructureManipulator::deviceSetName(boost::shared_ptr<Device> _pDevice,
                                           const std::string& _name) {
    m_Interface.deviceSetName(_pDevice->getDSMeterDSID(),
                              _pDevice->getShortAddress(), _name);
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
      m_Interface.createGroup(0, _groupNumber);
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

  void StructureManipulator::createGroup(boost::shared_ptr<Zone> _zone, int _groupNumber) {
    if(m_Apartment.getPropertyNode() != NULL) {
      m_Apartment.getPropertyNode()->checkWriteAccess();
    }
    boost::shared_ptr<Group> pGroup = _zone->getGroup(_groupNumber);
    if (!pGroup) {
      Logger::getInstance()->log("creating new group " + intToString(_groupNumber) +
          " in zone " + intToString(_zone->getID()));
      pGroup = boost::shared_ptr<Group>(
        new Group(_groupNumber, _zone, m_Apartment));
      // TODO: allow new groups across all zone, e.g. with ZoneID == 0
      _zone->addGroup(pGroup);
      m_Interface.createGroup(_zone->getID(), _groupNumber);
    }
  } // createGroup

  void StructureManipulator::deviceAddToGroup(boost::shared_ptr<Device> _device, boost::shared_ptr<Group> _group) {
    if(m_Apartment.getPropertyNode() != NULL) {
      m_Apartment.getPropertyNode()->checkWriteAccess();
    }
    m_Interface.addToGroup(_device->getDSMeterDSID(), _group->getID(), _device->getShortAddress());
    _device->addToGroup(_group->getID());
  } // deviceAddToGroup

  void StructureManipulator::deviceRemoveFromGroup(boost::shared_ptr<Device> _device, boost::shared_ptr<Group> _group) {
    if(m_Apartment.getPropertyNode() != NULL) {
      m_Apartment.getPropertyNode()->checkWriteAccess();
    }
    m_Interface.removeFromGroup(_device->getDSMeterDSID(), _group->getID(), _device->getShortAddress());
    _device->removeFromGroup(_group->getID());
  } // deviceRemoveFromGroup

  void StructureManipulator::sensorPush(boost::shared_ptr<Zone> _zone, dss_dsid_t _sourceID, int _sensorType, int _sensorValue) {
    if(m_Apartment.getPropertyNode() != NULL) {
      m_Apartment.getPropertyNode()->checkWriteAccess();
    }
    m_Interface.sensorPush(_zone->getID(), _sourceID, _sensorType, _sensorValue);
    _zone->sensorPush(_sourceID, _sensorType, _sensorValue);
  } // sensorPush

} // namespace dss