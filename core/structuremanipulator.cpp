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

#include "businterface.h"
#include "mutex.h"
#include "core/model/device.h"
#include "core/model/apartment.h"
#include "core/model/modulator.h"
#include "core/model/zone.h"
#include "core/model/set.h"
#include "core/model/group.h"
#include "core/model/modelconst.h"

#include <stdexcept>

namespace dss {

  void StructureManipulator::createZone(boost::shared_ptr<DSMeter> _dsMeter, boost::shared_ptr<Zone> _zone) {
    AssertLocked apartmentLocked(&m_Apartment);
    if(!_dsMeter->isPresent()) {
      throw std::runtime_error("Need dsMeter to be present");
    }
    m_Interface.createZone(_dsMeter->getDSID(), _zone->getID());
    _zone->addToDSMeter(_dsMeter);
    _zone->setIsPresent(true);
  } // createZone

  void StructureManipulator::addDeviceToZone(boost::shared_ptr<Device> _device, boost::shared_ptr<Zone> _zone) {
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
    if(_zone->getFirstZoneOnDSMeter() != _dsMeter->getDSID()) {

      m_Interface.removeZone(_dsMeter->getDSID(), _zone->getID());
      _zone->removeFromDSMeter(_dsMeter);
      if(_zone->isRegisteredOnAnyMeter()) {
        _zone->setIsPresent(false);
      }
    } else {
      Logger::getInstance()->log("Not removing zone as it's a primary zone on this meter", lsInfo);
    }
  } // removeZoneOnDSMeter

  void StructureManipulator::sceneSetName(boost::shared_ptr<Group> _group, int _sceneNumber, const std::string& _name) {
    m_Interface.sceneSetName(_group->getZoneID(), _group->getID(), _sceneNumber, _name);
  } // sceneSetName

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

} // namespace dss
