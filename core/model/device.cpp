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

#include "core/ds485const.h"
#include "core/model/modelconst.h"
#include "core/model/busrequest.h"
#include "core/propertysystem.h"
#include "core/model/scenehelper.h"
#include "core/model/modelevent.h"
#include "core/model/apartment.h"
#include "core/model/modelmaintenance.h"
#include "core/model/modulator.h"

namespace dss {

  //================================================== Device

  const devid_t ShortAddressStaleDevice = 0xFFFF;

  Device::Device(dsid_t _dsid, Apartment* _pApartment)
  : AddressableModelItem(_pApartment),
    m_DSID(_dsid),
    m_ShortAddress(ShortAddressStaleDevice),
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
    m_IsLockedInDSM(false)
  { } // ctor

  void Device::publishToPropertyTree() {
    if(m_pPropertyNode == NULL) {
      if(m_pApartment->getPropertyNode() != NULL) {
        m_pPropertyNode = m_pApartment->getPropertyNode()->createProperty("zones/zone0/" + m_DSID.toString());
        m_pPropertyNode->createProperty("name")->linkToProxy(PropertyProxyMemberFunction<Device, std::string>(*this, &Device::getName, &Device::setName));
        m_pPropertyNode->createProperty("DSMeterID")->linkToProxy(PropertyProxyMemberFunction<Device,int>(*this, &Device::getDSMeterID));
        m_pPropertyNode->createProperty("ZoneID")->linkToProxy(PropertyProxyReference<int>(m_ZoneID, false));
        if(m_pPropertyNode->getProperty("interrupt/mode") == NULL) {
          PropertyNodePtr interruptNode = m_pPropertyNode->createProperty("interrupt");
          interruptNode->setFlag(PropertyNode::Archive, true);
          PropertyNodePtr interruptModeNode = interruptNode->createProperty("mode");
          interruptModeNode->setStringValue("ignore");
          interruptModeNode->setFlag(PropertyNode::Archive, true);
        }
        m_TagsNode = m_pPropertyNode->createProperty("tags");
        m_TagsNode->setFlag(PropertyNode::Archive, true);
      }
    }
  } // publishToPropertyTree

  void Device::enable() {
    boost::shared_ptr<EnableDeviceCommandBusRequest> request(new EnableDeviceCommandBusRequest());
    boost::shared_ptr<AddressableModelItem> modelItem = shared_from_this();
    request->setTarget(boost::dynamic_pointer_cast<Device>(modelItem));
    m_pApartment->dispatchRequest(request);
  } // enable

  void Device::disable() {
    boost::shared_ptr<DisableDeviceCommandBusRequest> request(new DisableDeviceCommandBusRequest());
    boost::shared_ptr<AddressableModelItem> modelItem = shared_from_this();
    request->setTarget(boost::dynamic_pointer_cast<Device>(modelItem));
    m_pApartment->dispatchRequest(request);
  } // disable

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

  bool Device::hasSwitch() const {
    return getFunctionID() == FunctionIDSwitch;
  } // hasSwitch

  void Device::setRawValue(const uint16_t _value, const int _parameterNr, const int _size) {
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setValueDevice(*this, _value, _parameterNr, _size);
    }
  } // setRawValue

  double Device::getValue(const int _parameterNr) {
    return m_pApartment->getDeviceBusInterface()->deviceGetParameterValue(m_ShortAddress, getDSMeterID(),  _parameterNr);
  } // getValue

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
    publishToPropertyTree();
    m_LastDiscovered = DateTime();
  } // setShortAddress

  dsid_t Device::getDSID() const {
    return m_DSID;
  } // getDSID;

  int Device::getDSMeterID() const {
    if(m_DSMeterDSID != NullDSID) {
      return m_pApartment->getDSMeterByDSID(m_DSMeterDSID)->getBusID();
    } else {
      return -1;
    }
  } // getDSMeterID

  void Device::setLastKnownDSMeterDSID(const dsid_t& _value) {
    m_LastKnownMeterDSID = _value;
  } // setLastKnownDSMeterDSID

  const dsid_t& Device::getLastKnownDSMeterDSID() const {
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
        std::string basePath = "zones/zone" + intToString(m_ZoneID);
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
    m_GroupBitmask.set(_groupID-1);
    if(find(m_Groups.begin(), m_Groups.end(), _groupID) == m_Groups.end()) {
      m_Groups.push_back(_groupID);
    } else {
      Logger::getInstance()->log("Device " + m_DSID.toString() + " (bus: " + intToString(m_ShortAddress) + ", zone: " + intToString(m_ZoneID) + ") is already in group " + intToString(_groupID));
    }
  } // addToGroup

  void Device::removeFromGroup(const int _groupID) {
    m_GroupBitmask.reset(_groupID-1);
    std::vector<int>::iterator it = find(m_Groups.begin(), m_Groups.end(), _groupID);
    if(it != m_Groups.end()) {
      m_Groups.erase(it);
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
    return (_groupID == 0) || m_GroupBitmask.test(_groupID - 1);
  } // isInGroup

  Apartment& Device::getApartment() const {
    return *m_pApartment;
  } // getApartment

  unsigned long Device::getPowerConsumption() {
    return m_Consumption;
  } // getPowerConsumption

  uint8_t Device::dsLinkSend(uint8_t _value, bool _lastByte, bool _writeOnly) {
    uint8_t flags = 0;
    if(_lastByte) {
      flags |= DSLinkSendLastByte;
    }
    if(_writeOnly) {
      flags |= DSLinkSendWriteOnly;
    }
    return m_pApartment->getDeviceBusInterface()->dSLinkSend(getDSMeterID(), m_ShortAddress, _value, flags);
  } // dsLinkSend

  int Device::getSensorValue(const int _sensorID) {
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
