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
    m_DSMeterID(-1),
    m_LastKnownMeterDSID(NullDSID),
    m_FunctionID(0),
    m_LastCalledScene(SceneOff),
    m_Consumption(0),
    m_LastDiscovered(DateTime::NullDate),
    m_FirstSeen(DateTime::NullDate)
  { } // ctor

  void Device::publishToPropertyTree() {
    if(m_pPropertyNode == NULL) {
      if(m_pApartment->getPropertyNode() != NULL) {
        m_pPropertyNode = m_pApartment->getPropertyNode()->createProperty("zones/zone0/" + m_DSID.toString());
//        m_pPropertyNode->createProperty("name")->linkToProxy(PropertyProxyMemberFunction<Device, std::string>(*this, &Device::getName, &Device::setName));
        m_pPropertyNode->createProperty("name")->linkToProxy(PropertyProxyReference<std::string>(m_Name));
        m_pPropertyNode->createProperty("DSMeterID")->linkToProxy(PropertyProxyReference<int>(m_DSMeterID, false));
        m_pPropertyNode->createProperty("ZoneID")->linkToProxy(PropertyProxyReference<int>(m_ZoneID, false));
        if(m_pPropertyNode->getProperty("interrupt/mode") == NULL) {
          PropertyNodePtr interruptNode = m_pPropertyNode->createProperty("interrupt");
          interruptNode->setFlag(PropertyNode::Archive, true);
          PropertyNodePtr interruptModeNode = interruptNode->createProperty("mode");
          interruptModeNode->setStringValue("ignore");
          interruptModeNode->setFlag(PropertyNode::Archive, true);
        }
      }
    }
  } // publishToPropertyTree

  void Device::enable() {
    boost::shared_ptr<EnableDeviceCommandBusRequest> request(new EnableDeviceCommandBusRequest());
    request->setTarget(this);
    m_pApartment->dispatchRequest(request);
  } // enable

  void Device::disable() {
    boost::shared_ptr<DisableDeviceCommandBusRequest> request(new DisableDeviceCommandBusRequest());
    request->setTarget(this);
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

  bool Device::hasSwitch() const {
    return getFunctionID() == FunctionIDSwitch;
  } // hasSwitch

  void Device::setRawValue(const uint16_t _value, const int _parameterNr, const int _size) {
    m_pApartment->getDeviceBusInterface()->setValueDevice(*this, _value, _parameterNr, _size);
  } // setRawValue

  double Device::getValue(const int _parameterNr) {
    return m_pApartment->getDeviceBusInterface()->deviceGetParameterValue(m_ShortAddress, m_DSMeterID,  _parameterNr);
  } // getValue

  void Device::nextScene() {
    callScene(SceneHelper::getNextScene(m_LastCalledScene));
  } // nextScene

  void Device::previousScene() {
    callScene(SceneHelper::getNextScene(m_LastCalledScene));
  } // previousScene

  std::string Device::getName() const {
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
    return m_DSMeterID;
  } // getDSMeterID

  const dsid_t& Device::getLastKnownDSMeterDSID() const {
    return m_LastKnownMeterDSID;
  } // getLastKnownDSMeterDSID

  void Device::setDSMeter(const DSMeter& _dsMeter) {
    m_DSMeterID = _dsMeter.getBusID();
    m_LastKnownMeterDSID = _dsMeter.getDSID();
  } // setDSMeterID

  int Device::getZoneID() const {
    return m_ZoneID;
  } // getZoneID

  void Device::setZoneID(const int _value) {
    if(_value != m_ZoneID) {
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

  int Device:: getGroupIdByIndex(const int _index) const {
    return m_Groups[_index];
  } // getGroupIdByIndex

  Group& Device::getGroupByIndex(const int _index) {
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
    return m_pApartment->getDeviceBusInterface()->dSLinkSend(m_DSMeterID, m_ShortAddress, _value, flags);
  } // dsLinkSend

  int Device::getSensorValue(const int _sensorID) {
    return m_pApartment->getDeviceBusInterface()->getSensorValue(*this,_sensorID);
  } // getSensorValue

  
} // namespace dss
