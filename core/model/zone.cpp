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

#include "zone.h"

#include <vector>

#include "core/base.h"
#include "core/logger.h"
#include "core/ds485const.h"
#include "core/model/modelconst.h"
#include "core/propertysystem.h"
#include "set.h"
#include "device.h"
#include "apartment.h"
#include "modulator.h"

namespace dss {

  //================================================== Zone

  Zone::~Zone() {
    scrubVector(m_Groups);
    // we don't own our dsMeters
    m_DSMeters.clear();
  } // dtor

  Set Zone::getDevices() const {
    return Set(m_Devices);
  } // getDevices

  void Zone::addDevice(DeviceReference& _device) {
    const Device& dev = _device.getDevice();
    int oldZoneID = dev.getZoneID();
    if((oldZoneID != -1) && (oldZoneID != 0)) {
      try {
        Zone& oldZone = dev.getApartment().getZone(oldZoneID);
        oldZone.removeDevice(_device);
      } catch(std::runtime_error&) {
      }
    }
    if(!contains(m_Devices, _device)) {
      m_Devices.push_back(_device);
    } else {
      // don't warn about multiple additions to zone 0
      if(m_ZoneID != 0) {
        Logger::getInstance()->log("Zone::addDevice: DUPLICATE DEVICE Detected Zone: " + intToString(m_ZoneID) + " device: " + _device.getDSID().toString(), lsWarning);
      }
    }
    _device.getDevice().setZoneID(m_ZoneID);
  } // addDevice

  void Zone::addGroup(Group* _group) {
    if(_group->getZoneID() != m_ZoneID) {
      throw std::runtime_error("Zone::addGroup: ZoneID of _group does not match own");
    }
    m_Groups.push_back(_group);
    if(m_pPropertyNode != NULL) {
      PropertyNodePtr groupNode = m_pPropertyNode->createProperty("groups/" + intToString(_group->getID()));
      groupNode->createProperty("lastCalledScene")
        ->linkToProxy(PropertyProxyMemberFunction<Group, int>(*_group, &Group::getLastCalledScene));
    }
  } // addGroup

  void Zone::removeGroup(UserGroup* _group) {
    std::vector<Group*>::iterator it = find(m_Groups.begin(), m_Groups.end(), _group);
    if(it != m_Groups.end()) {
      m_Groups.erase(it);
    }
    if(m_pPropertyNode != NULL) {
      PropertyNodePtr groupNode = m_pPropertyNode->getProperty("groups/" + intToString(_group->getID()));
      if(groupNode != NULL) {
        groupNode->getParentNode()->removeChild(groupNode);
      }
    }
  } // removeGroup

  void Zone::removeDevice(const DeviceReference& _device) {
    DeviceIterator pos = find(m_Devices.begin(), m_Devices.end(), _device);
    if(pos != m_Devices.end()) {
      m_Devices.erase(pos);
    }
  } // removeDevice

  Group* Zone::getGroup(const std::string& _name) const {
    for(std::vector<Group*>::const_iterator ipGroup = m_Groups.begin(), e = m_Groups.end();
        ipGroup != e; ++ipGroup)
    {
        if((*ipGroup)->getName() == _name) {
          return *ipGroup;
        }
    }
    return NULL;
  } // getGroup

  Group* Zone::getGroup(const int _id) const {
    for(std::vector<Group*>::const_iterator ipGroup = m_Groups.begin(), e = m_Groups.end();
        ipGroup != e; ++ipGroup)
    {
        if((*ipGroup)->getID() == _id) {
          return *ipGroup;
        }
    }
    return NULL;
  } // getGroup

  int Zone::getID() const {
    return m_ZoneID;
  } // getID

  void Zone::setZoneID(const int _value) {
    m_ZoneID = _value;
  } // setZoneID

  void Zone::addToDSMeter(DSMeter& _dsMeter) {
    // make sure the zone is connected to the dsMeter
    if(find(m_DSMeters.begin(), m_DSMeters.end(), &_dsMeter) == m_DSMeters.end()) {
      m_DSMeters.push_back(&_dsMeter);
      if(_dsMeter.getPropertyNode() != NULL) {
        PropertyNodePtr alias = _dsMeter.getPropertyNode()->createProperty("zones/" + intToString(m_ZoneID));
        alias->alias(m_pPropertyNode);
      }
    }
  } // addToDSMeter

  void Zone::removeFromDSMeter(DSMeter& _dsMeter) {
    m_DSMeters.erase(find(m_DSMeters.begin(), m_DSMeters.end(), &_dsMeter));
    if(_dsMeter.getPropertyNode() != NULL) {
      PropertyNodePtr alias = _dsMeter.getPropertyNode()->getProperty("zones/" + intToString(m_ZoneID));
      if(alias != NULL) {
        alias->getParentNode()->removeChild(alias);
      }
    }
  } // removeFromDSMeter

  bool Zone::registeredOnDSMeter(const DSMeter& _dsMeter) const {
    return find(m_DSMeters.begin(), m_DSMeters.end(), &_dsMeter) != m_DSMeters.end();
  } // registeredOnDSMeter

  bool Zone::isRegisteredOnAnyMeter() const {
    return !m_DSMeters.empty();
  } // isRegisteredOnAnyMeter

  unsigned long Zone::getPowerConsumption() {
    return getDevices().getPowerConsumption();
  } // getPowerConsumption

  void Zone::nextScene() {
    getGroup(GroupIDBroadcast)->nextScene();
  } // nextScene

  void Zone::previousScene() {
    getGroup(GroupIDBroadcast)->previousScene();
  } // previousScene

  std::vector<AddressableModelItem*> Zone::splitIntoAddressableItems() {
    std::vector<AddressableModelItem*> result;
    result.push_back(getGroup(GroupIDBroadcast));
    return result;
  } // splitIntoAddressableItems

  void Zone::publishToPropertyTree() {
    if(m_pPropertyNode == NULL) {
      if(m_pApartment->getPropertyNode() != NULL) {
        m_pPropertyNode = m_pApartment->getPropertyNode()->createProperty("zones/zone" + intToString(m_ZoneID));
        m_pPropertyNode->createProperty("name")
          ->linkToProxy(PropertyProxyMemberFunction<Zone, std::string>(*this, &Zone::getName, &Zone::setName));
      }
    }
  } // publishToPropertyTree

} // namespace dss
