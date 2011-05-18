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

#include "device.h"

#include "core/businterface.h"
#include "core/propertysystem.h"
#include "core/ds485types.h"

#include "core/model/modelconst.h"
#include "core/model/scenehelper.h"
#include "core/model/modelevent.h"
#include "core/model/apartment.h"
#include "core/model/modelmaintenance.h"
#include "core/model/modulator.h"

namespace dss {

  //================================================== Device

  Device::Device(dss_dsid_t _dsid, Apartment* _pApartment)
  : AddressableModelItem(_pApartment),
    m_DSID(_dsid),
    m_ShortAddress(ShortAddressStaleDevice),
    m_LastKnownShortAddress(ShortAddressStaleDevice),
    m_ZoneID(0),
    m_LastKnownZoneID(0),
    m_DSMeterDSID(NullDSID),
    m_LastKnownMeterDSID(NullDSID),
    m_FunctionID(0),
    m_ProductID(0),
    m_RevisionID(0),
    m_LastCalledScene(SceneOff),
    m_Consumption(0),
    m_LastDiscovered(DateTime::NullDate),
    m_FirstSeen(DateTime::NullDate),
    m_IsLockedInDSM(false),
    m_OutputMode(0),
    m_ButtonSetsLocalPriority(false),
    m_ButtonGroupMembership(0),
    m_ButtonActiveGroup(0),
    m_ButtonID(0)
  { } // ctor

  void Device::publishToPropertyTree() {
    if(m_pPropertyNode == NULL) {
      if(m_pApartment->getPropertyNode() != NULL) {
        m_pPropertyNode = m_pApartment->getPropertyNode()->createProperty("zones/zone0/devices/" + m_DSID.toString());
        m_pPropertyNode->createProperty("dSID")->setStringValue(m_DSID.toString());
        m_pPropertyNode->createProperty("present")
          ->linkToProxy(PropertyProxyMemberFunction<Device, bool>(*this, &Device::isPresent));
        m_pPropertyNode->createProperty("name")
          ->linkToProxy(PropertyProxyMemberFunction<Device, std::string>(*this, &Device::getName, &Device::setName));
        m_pPropertyNode->createProperty("DSMeterDSID")
          ->linkToProxy(PropertyProxyMemberFunction<dss_dsid_t, std::string, false>(m_DSMeterDSID, &dss_dsid_t::toString));
        m_pPropertyNode->createProperty("ZoneID")->linkToProxy(PropertyProxyReference<int>(m_ZoneID, false));
        m_pPropertyNode->createProperty("functionID")
          ->linkToProxy(PropertyProxyReference<int>(m_FunctionID, false));
        m_pPropertyNode->createProperty("revisionID")
          ->linkToProxy(PropertyProxyReference<int>(m_RevisionID, false));
        m_pPropertyNode->createProperty("productID")
          ->linkToProxy(PropertyProxyReference<int>(m_ProductID, false));
        m_pPropertyNode->createProperty("lastKnownZoneID")
          ->linkToProxy(PropertyProxyReference<int>(m_LastKnownZoneID, false));
        m_pPropertyNode->createProperty("shortAddress")
          ->linkToProxy(PropertyProxyReference<int, uint16_t>(m_ShortAddress, false));
        m_pPropertyNode->createProperty("lastKnownShortAddress")
          ->linkToProxy(PropertyProxyReference<int, uint16_t>(m_LastKnownShortAddress, false));
        m_pPropertyNode->createProperty("lastKnownMeterDSID")
          ->linkToProxy(PropertyProxyMemberFunction<dss_dsid_t, std::string, false>(m_LastKnownMeterDSID, &dss_dsid_t::toString));
        m_pPropertyNode->createProperty("firstSeen")
          ->linkToProxy(PropertyProxyMemberFunction<DateTime, std::string, false>(m_FirstSeen, &DateTime::toString));
        m_pPropertyNode->createProperty("lastDiscovered")
          ->linkToProxy(PropertyProxyMemberFunction<DateTime, std::string, false>(m_LastDiscovered, &DateTime::toString));
        m_pPropertyNode->createProperty("locked")
          ->linkToProxy(PropertyProxyReference<bool>(m_IsLockedInDSM, false));
        m_pPropertyNode->createProperty("outputMode")
          ->linkToProxy(PropertyProxyReference<int, uint8_t>(m_OutputMode, false));
        m_pPropertyNode->createProperty("button/id")
          ->linkToProxy(PropertyProxyReference<int>(m_ButtonID, false));
        m_pPropertyNode->createProperty("button/activeGroup")
          ->linkToProxy(PropertyProxyReference<int>(m_ButtonActiveGroup, false));
        m_pPropertyNode->createProperty("button/groupMembership")
          ->linkToProxy(PropertyProxyReference<int>(m_ButtonGroupMembership, false));
        m_pPropertyNode->createProperty("button/setsLocalPriority")
          ->linkToProxy(PropertyProxyReference<bool>(m_ButtonSetsLocalPriority));
        m_TagsNode = m_pPropertyNode->createProperty("tags");
        m_TagsNode->setFlag(PropertyNode::Archive, true);
      }
    }
  } // publishToPropertyTree

  bool Device::isOn() const {
    return (m_LastCalledScene != SceneOff) &&
           (m_LastCalledScene != SceneMin) &&
           (m_LastCalledScene != SceneDeepOff) &&
           (m_LastCalledScene != SceneStandBy);
  } // isOn

  int Device::getFunctionID() const {
    return m_FunctionID;
  } // getFunctionID

  void Device::setFunctionID(const int _value) {
    m_FunctionID = _value;
  } // setFunctionID

  int Device::getProductID() const {
    return m_ProductID;
  } // getProductID

  void Device::setProductID(const int _value) {
    m_ProductID = _value;
  } // setProductID

  int Device::getRevisionID() const {
    return m_RevisionID;
  } // getRevisionID

  void Device::setRevisionID(const int _value) {
    m_RevisionID = _value;
  } // setRevisionID

  void Device::setConfig(uint8_t _configClass, uint8_t _configIndex,
                         uint8_t _value) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setDeviceConfig(*this,
                                                             _configClass,
                                                             _configIndex,
                                                             _value);
      
      if((m_pApartment != NULL) && (m_pApartment->getModelMaintenance() != NULL)) {
        ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceConfigChanged,
                                                    m_DSMeterDSID);
        pEvent->addParameter(m_ShortAddress);
        pEvent->addParameter(_configClass);
        pEvent->addParameter(_configIndex);
        pEvent->addParameter(_value);    
        m_pApartment->getModelMaintenance()->addModelEvent(pEvent);
      }      
    }
  } // setConfig

  uint8_t Device::getConfig(uint8_t _configClass, uint8_t _configIndex) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkReadAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getDeviceConfig(*this,
                                                                  _configClass,
                                                                  _configIndex);
    }
    throw std::runtime_error("Bus interface not available");
  } // getValue

  uint16_t Device::getConfigWord(uint8_t _configClass, uint8_t _configIndex) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkReadAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getDeviceConfigWord(*this,
                                                                  _configClass,
                                                                  _configIndex);
    }
    throw std::runtime_error("Bus interface not available");
  } // getValue

  std::pair<uint8_t, uint16_t> Device::getTransmissionQuality() {
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getTransmissionQuality(*this);
    }
    throw std::runtime_error("Bus interface not available");
  }

  void Device::setValue(uint8_t _value) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setValue(*this, _value);
    }
  } // setValue

  void Device::nextScene() {
    callScene(SceneHelper::getNextScene(m_LastCalledScene));
  } // nextScene

  void Device::previousScene() {
    callScene(SceneHelper::getNextScene(m_LastCalledScene));
  } // previousScene

  const std::string& Device::getName() const {
    return m_Name;
  } // getName

  void Device::setName(const std::string& _name) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if(m_Name != _name) {
      m_Name = _name;
      dirty();
    }
  } // setName

  void Device::dirty() {
    if((m_pApartment != NULL) && (m_pApartment->getModelMaintenance() != NULL)) {
      m_pApartment->getModelMaintenance()->addModelEvent(
          new ModelEvent(ModelEvent::etModelDirty)
      );
    }
  } // dirty

  bool Device::operator==(const Device& _other) const {
    return _other.m_DSID == m_DSID;
  } // operator==

  devid_t Device::getShortAddress() const {
    return m_ShortAddress;
  } // getShortAddress

  void Device::setShortAddress(const devid_t _shortAddress) {
    m_ShortAddress = _shortAddress;
    m_LastKnownShortAddress = _shortAddress;
    publishToPropertyTree();
    m_LastDiscovered = DateTime();
  } // setShortAddress

  devid_t Device::getLastKnownShortAddress() const {
    return m_LastKnownShortAddress;
  } // getLastKnownShortAddress

  void Device::setLastKnownShortAddress(const devid_t _shortAddress) {
    m_LastKnownShortAddress = _shortAddress;
  } // setLastKnownShortAddress

  dss_dsid_t Device::getDSID() const {
    return m_DSID;
  } // getDSID;

  dss_dsid_t Device::getDSMeterDSID() const {
    return m_DSMeterDSID;
  } // getDSMeterID

  void Device::setLastKnownDSMeterDSID(const dss_dsid_t& _value) {
    m_LastKnownMeterDSID = _value;
  } // setLastKnownDSMeterDSID

  const dss_dsid_t& Device::getLastKnownDSMeterDSID() const {
    return m_LastKnownMeterDSID;
  } // getLastKnownDSMeterDSID

  void Device::setDSMeter(boost::shared_ptr<DSMeter> _dsMeter) {
    PropertyNodePtr alias;
    std::string devicePath = "devices/" + m_DSID.toString();
    if((m_pPropertyNode != NULL) && (m_DSMeterDSID != NullDSID)) {
      alias = m_pApartment->getDSMeterByDSID(m_DSMeterDSID)->getPropertyNode()->getProperty(devicePath);
    }
    m_DSMeterDSID = _dsMeter->getDSID();
    m_LastKnownMeterDSID = _dsMeter->getDSID();
    if(m_pPropertyNode != NULL) {
      PropertyNodePtr target = _dsMeter->getPropertyNode()->createProperty("devices");
      if(alias != NULL) {
        target->addChild(alias);
      } else {
        alias = target->createProperty(m_DSID.toString());
        alias->alias(m_pPropertyNode);
      }
    }
  } // setDSMeter

  int Device::getZoneID() const {
    return m_ZoneID;
  } // getZoneID

  void Device::setZoneID(const int _value) {
    if(_value != m_ZoneID) {
      if(_value != 0) {
        m_LastKnownZoneID = _value;
      }
      m_ZoneID = _value;
      if((m_pPropertyNode != NULL) && (m_pApartment->getPropertyNode() != NULL)) {
        std::string basePath = "zones/zone" + intToString(m_ZoneID) +
                               "/devices";
        if(m_pAliasNode == NULL) {
          PropertyNodePtr node = m_pApartment->getPropertyNode()->getProperty(basePath + "/" + m_DSID.toString());
          if(node != NULL) {
            Logger::getInstance()->log("Device::setZoneID: Target node for device " + m_DSID.toString() + " already exists", lsError);
            if(node->size() > 0) {
              Logger::getInstance()->log("Device::setZoneID: Target node for device " + m_DSID.toString() + " has children", lsFatal);
              return;
            }
          }
          m_pAliasNode = m_pApartment->getPropertyNode()->createProperty(basePath + "/" + m_DSID.toString());

          m_pAliasNode->alias(m_pPropertyNode);
        } else {
          PropertyNodePtr base = m_pApartment->getPropertyNode()->getProperty(basePath);
          if(base == NULL) {
            throw std::runtime_error("PropertyNode of the new zone does not exist");
          }
          base->addChild(m_pAliasNode);
        }
      }
    }
  } // setZoneID

  int Device::getLastKnownZoneID() const {
    return m_LastKnownZoneID;
  } // getLastKnownZoneID

  void Device::setLastKnownZoneID(const int _value) {
    m_LastKnownZoneID = _value;
  } // setLastKnownZoneID

  int Device:: getGroupIdByIndex(const int _index) const {
    return m_Groups[_index];
  } // getGroupIdByIndex

  boost::shared_ptr<Group> Device::getGroupByIndex(const int _index) {
    return m_pApartment->getGroup(getGroupIdByIndex(_index));
  } // getGroupByIndex

  void Device::addToGroup(const int _groupID) {
    if((_groupID > 0) && (_groupID <= GroupIDMax)) {
      m_GroupBitmask.set(_groupID-1);
      if(find(m_Groups.begin(), m_Groups.end(), _groupID) == m_Groups.end()) {
        m_Groups.push_back(_groupID);
      } else {
        Logger::getInstance()->log("Device " + m_DSID.toString() + " (bus: " + intToString(m_ShortAddress) + ", zone: " + intToString(m_ZoneID) + ") is already in group " + intToString(_groupID));
      }
    } else {
      Logger::getInstance()->log("Group ID out of bounds: " + intToString(_groupID), lsError);
    }
  } // addToGroup

  void Device::removeFromGroup(const int _groupID) {
    if((_groupID > 0) && (_groupID <= GroupIDMax)) {
      m_GroupBitmask.reset(_groupID-1);
      std::vector<int>::iterator it = find(m_Groups.begin(), m_Groups.end(), _groupID);
      if(it != m_Groups.end()) {
        m_Groups.erase(it);
      }
    } else {
      Logger::getInstance()->log("Group ID out of bounds: " + intToString(_groupID), lsError);
    }
  } // removeFromGroup

  void Device::resetGroups() {
    m_GroupBitmask.reset();
    m_Groups.clear();
  } // resetGroups

  int Device::getGroupsCount() const {
    return m_Groups.size();
  } // getGroupsCount

  std::bitset<63>& Device::getGroupBitmask() {
    return m_GroupBitmask;
  } // getGroupBitmask

  const std::bitset<63>& Device::getGroupBitmask() const {
    return m_GroupBitmask;
  } // getGroupBitmask

  bool Device::isInGroup(const int _groupID) const {
    bool result = false;
    if(_groupID == 0) {
      result = true;
    } else if((_groupID < 0) || (_groupID > GroupIDMax)) {
      result = false;
      Logger::getInstance()->log("Group ID out of bounds: " + intToString(_groupID), lsError);
    } else {
      result = m_GroupBitmask.test(_groupID - 1);
    }
    return result;
  } // isInGroup

  Apartment& Device::getApartment() const {
    return *m_pApartment;
  } // getApartment

  unsigned long Device::getPowerConsumption() {
    return m_Consumption;
  } // getPowerConsumption

  int Device::getSensorValue(const int _sensorID) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkReadAccess();
    }
    return m_pApartment->getDeviceBusInterface()->getSensorValue(*this,_sensorID);
  } // getSensorValue

  void Device::setIsLockedInDSM(const bool _value) {
    m_IsLockedInDSM = _value;
  } // setIsLockedInDSM

  void Device::lock() {
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->lockOrUnlockDevice(*this, true);
      m_IsLockedInDSM = true;
    }
  } // lock

  void Device::unlock() {
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->lockOrUnlockDevice(*this, false);
      m_IsLockedInDSM = false;
    }
  } // unlock

  bool Device::hasTag(const std::string& _tagName) const {
    if(m_TagsNode != NULL) {
      return m_TagsNode->getPropertyByName(_tagName) != NULL;
    }
    return false;
  } // hasTag

  void Device::addTag(const std::string& _tagName) {
    if(!hasTag(_tagName)) {
      if(m_TagsNode != NULL) {
        PropertyNodePtr pNode = m_TagsNode->createProperty(_tagName);
        pNode->setFlag(PropertyNode::Archive, true);
        dirty();
      }
    }
  } // addTag

  void Device::removeTag(const std::string& _tagName) {
    if(hasTag(_tagName)) {
      if(m_TagsNode != NULL) {
        m_TagsNode->removeChild(m_TagsNode->getPropertyByName(_tagName));
        dirty();
      }
    }
  } // removeTag

  std::vector<std::string> Device::getTags() {
    std::vector<std::string> result;
    if(m_TagsNode != NULL) {
      int count = m_TagsNode->getChildCount();
      for(int iNode = 0; iNode < count; iNode++) {
        result.push_back(m_TagsNode->getChild(iNode)->getName());
      }
    }
    return result;
  } // getTags

} // namespace dss
