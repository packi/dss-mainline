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

#include "src/businterface.h"
#include "src/propertysystem.h"
#include "src/ds485types.h"

#include "src/model/modelconst.h"
#include "src/model/scenehelper.h"
#include "src/model/modelevent.h"
#include "src/model/apartment.h"
#include "src/model/modelmaintenance.h"
#include "src/model/modulator.h"

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
    m_Energymeter(0),
    m_LastDiscovered(DateTime::NullDate),
    m_FirstSeen(DateTime::NullDate),
    m_IsLockedInDSM(false),
    m_OutputMode(0),
    m_ButtonSetsLocalPriority(false),
    m_ButtonGroupMembership(0),
    m_ButtonActiveGroup(0),
    m_ButtonID(0)
  { } // ctor

  Device::~Device() {
    if(m_pPropertyNode != NULL) {
      m_pPropertyNode->unlinkProxy(true);
      m_pPropertyNode->getParentNode()->removeChild(m_pPropertyNode);
    }
    if(m_pAliasNode != NULL) {
      m_pAliasNode->alias(PropertyNodePtr());
      m_pAliasNode->getParentNode()->removeChild(m_pAliasNode);
    }
  }

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
        m_pPropertyNode->createProperty("inactiveSince")
          ->linkToProxy(PropertyProxyMemberFunction<Device, std::string, false>(*this, &Device::getInactiveSinceStr));
        m_pPropertyNode->createProperty("locked")
          ->linkToProxy(PropertyProxyReference<bool>(m_IsLockedInDSM, false));
        m_pPropertyNode->createProperty("outputMode")
          ->linkToProxy(PropertyProxyReference<int, uint8_t>(m_OutputMode, false));
        m_pPropertyNode->createProperty("button/id")
          ->linkToProxy(PropertyProxyReference<int>(m_ButtonID, false));
        m_pPropertyNode->createProperty("button/inputMode")
          ->linkToProxy(PropertyProxyReference<int, uint8_t>(m_ButtonInputMode, false));
        m_pPropertyNode->createProperty("button/inputIndex")
          ->linkToProxy(PropertyProxyReference<int, uint8_t>(m_ButtonInputIndex, false));
        m_pPropertyNode->createProperty("button/inputCount")
          ->linkToProxy(PropertyProxyReference<int, uint8_t>(m_ButtonInputCount, false));
        m_pPropertyNode->createProperty("button/activeGroup")
          ->linkToProxy(PropertyProxyReference<int>(m_ButtonActiveGroup, false));
        m_pPropertyNode->createProperty("button/groupMembership")
          ->linkToProxy(PropertyProxyReference<int>(m_ButtonGroupMembership, false));
        m_pPropertyNode->createProperty("button/setsLocalPriority")
          ->linkToProxy(PropertyProxyReference<bool>(m_ButtonSetsLocalPriority));
        if (!m_pPropertyNode->getProperty("sensorEvents")) {
          PropertyNodePtr sensorNode = m_pPropertyNode->createProperty("sensorEvents");
        }
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
    fillSensorTable(_value);
  } // setProductID

  int Device::getRevisionID() const {
    return m_RevisionID;
  } // getRevisionID

  void Device::setRevisionID(const int _value) {
    m_RevisionID = _value;
  } // setRevisionID

  void Device::fillSensorTable(const int _productId) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();

      if(m_pApartment->getDeviceBusInterface() != NULL) {
        if (!m_pPropertyNode->getProperty("sensorTable")) {
          if ((_productId == ProductID_KL_200) ||
              (_productId == ProductID_KL_201) ||
              (_productId == ProductID_KL_210)) {
            m_pPropertyNode->createProperty("sensorTable/sensor0")->setIntegerValue(0x3d);
            m_pPropertyNode->createProperty("sensorTable/sensor1")->setIntegerValue(0x3e);
            m_pPropertyNode->createProperty("sensorTable/sensor2")->setIntegerValue(0x04);
            m_pPropertyNode->createProperty("sensorTable/sensor3")->setIntegerValue(0x05);
            m_pPropertyNode->createProperty("sensorTable/sensor4")->setIntegerValue(0x06);
            m_pPropertyNode->createProperty("sensorTable/sensor5")->setIntegerValue(0x40);
          }
        }
      }
    }
  }

  void Device::setDeviceConfig(uint8_t _configClass, uint8_t _configIndex,
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
  } // setDeviceConfig

  void Device::setDeviceButtonID(uint8_t _buttonId) {
    setDeviceConfig(CfgClassFunction, CfgFunction_ButtonMode,
        ((m_ButtonActiveGroup & 0xf) << 4) | (_buttonId & 0xf));
  } // setDeviceButtonId

  void Device::setDeviceJokerGroup(uint8_t _groupId) {
    if((_groupId < 1) && (_groupId > 7)) {
      throw std::runtime_error("Invalid joker group value");
    }
    // for standard groups force that only one group is active
    for (int g = 1; g <= 7; g++) {
      if (m_GroupBitmask.test(g-1)) {
        removeFromGroup(g);
      }
    }
    addToGroup(_groupId);
    // propagate target group value to device
    setDeviceConfig(CfgClassFunction, CfgFunction_ButtonMode,
        ((_groupId & 0xf) << 4) | (m_ButtonID & 0xf));
  } // setDeviceJokerGroup

  void Device::setDeviceOutputMode(uint8_t _modeId) {
    setDeviceConfig(CfgClassFunction, CfgFunction_Mode, _modeId);
  } // setDeviceOutputMode

  void Device::setDeviceButtonInputMode(uint8_t _modeId) {
    setDeviceConfig(CfgClassFunction, CfgFunction_LTMode, _modeId);
  } // setDeviceButtonInputMode

  void Device::setProgMode(uint8_t _modeId) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setDeviceProgMode(*this, _modeId);
    }
  } // setProgMode

  void Device::setDeviceSceneMode(uint8_t _sceneId, DeviceSceneSpec_t _config) {
    uint8_t mode = _config.dontcare ? 1 : 0;
    mode |= _config.localprio ? 2 : 0;
    mode |= _config.specialmode ? 4 : 0;
    mode |= _config.flashmode ? 8 : 0;
    mode |= ((_config.ledconIndex & 3) << 4);
    mode |= ((_config.dimtimeIndex & 3) << 6);
    setDeviceConfig(CfgClassScene, 128 + _sceneId, mode);
  } // setDeviceSceneMode

  void Device::getDeviceSceneMode(uint8_t _sceneId, DeviceSceneSpec_t& _config) {
    uint8_t mode = getDeviceConfig(CfgClassScene, 128 + _sceneId);
    _config.dontcare = (mode & 1) > 0;
    _config.localprio = (mode & 2) > 0;
    _config.specialmode = (mode & 4) > 0;
    _config.flashmode = (mode & 8) > 0;
    _config.ledconIndex = (mode >> 4) & 3;
    _config.dimtimeIndex = (mode >> 6) & 3;
  } // getDeviceSceneMode

  void Device::setDeviceLedMode(uint8_t _ledconIndex, DeviceLedSpec_t _config) {
    if(_ledconIndex > 2) {
      throw DSSException("Device::setDeviceLedMode: index out of range");
    }
    uint8_t mode = _config.colorSelect & 7;
    mode |= ((_config.modeSelect & 3) << 3);
    mode |= _config.dimMode ? 32 : 0;
    mode |= _config.rgbMode ? 64 : 0;
    mode |= _config.groupColorMode ? 128 : 0;
    setDeviceConfig(CfgClassFunction, CfgFunction_LedConfig0 + _ledconIndex, mode);
  }  // setDeviceLedMode

  void Device::getDeviceLedMode(uint8_t _ledconIndex, DeviceLedSpec_t& _config) {
    if(_ledconIndex > 2) {
      throw DSSException("Device::getDeviceLedMode: index out of range");
    }
    uint8_t mode = getDeviceConfig(CfgClassFunction, CfgFunction_LedConfig0 + _ledconIndex);
    _config.colorSelect = mode & 7;
    _config.modeSelect = (mode >> 3) & 3;
    _config.dimMode = (mode & 32) > 0;
    _config.rgbMode = (mode & 64) > 0;
    _config.groupColorMode = (mode & 128) > 0;
  } // getDeviceLedMode

  int Device::transitionVal2Time(uint8_t _value) {
    int hval = _value / 16;
    int lval = _value % 16;
    return 100 * (1 << hval) - (100 * (1 << hval) / 32) * (15 - lval);
  }

  uint8_t Device::transitionTimeEval(int timems) {
    uint8_t val;
    int thigh, tlow;
    for (val = 0, tlow = thigh = 0; val < 255 && thigh < timems; ) {
      val ++;
      tlow = thigh;
      thigh = transitionVal2Time(val);
    }
    if((thigh - timems) > (timems - tlow)) val --;
    return val;
  }

  void Device::setDeviceTransitionTime(uint8_t _dimtimeIndex, int up, int down) {
    if(_dimtimeIndex > 2) {
      throw DSSException("Device::setDeviceTransitionTime: index out of range");
    }
    uint8_t vup = transitionTimeEval(up);
    uint8_t vdown  = transitionTimeEval(down);
    setDeviceConfig(CfgClassFunction, CfgFunction_DimTime0 + _dimtimeIndex*2, vup);
    setDeviceConfig(CfgClassFunction, CfgFunction_DimTime0 + _dimtimeIndex*2 + 1, vdown);
  } // setDeviceTransitionTime

  void Device::getDeviceTransitionTime(uint8_t _dimtimeIndex, int& up, int& down) {
    if(_dimtimeIndex > 2) {
      throw DSSException("Device::getDeviceTransitionTime: index out of range");
    }
    uint16_t value = getDeviceConfigWord(CfgClassFunction, CfgFunction_DimTime0 + _dimtimeIndex*2);
    uint8_t vup = value & 0xff;
    uint8_t vdown = (value >> 8) & 0xff;
    up = transitionVal2Time(vup);
    down = transitionVal2Time(vdown);
  } // getDeviceTransitionTime

  uint8_t Device::getDeviceConfig(uint8_t _configClass, uint8_t _configIndex) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkReadAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getDeviceConfig(*this,
                                                                  _configClass,
                                                                  _configIndex);
    }
    throw std::runtime_error("Bus interface not available");
  } // getDeviceConfig

  uint16_t Device::getDeviceConfigWord(uint8_t _configClass, uint8_t _configIndex) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkReadAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getDeviceConfigWord(*this,
                                                                  _configClass,
                                                                  _configIndex);
    }
    throw std::runtime_error("Bus interface not available");
  } // getDeviceConfigWord

  std::pair<uint8_t, uint16_t> Device::getDeviceTransmissionQuality() {
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getTransmissionQuality(*this);
    }
    throw std::runtime_error("Bus interface not available");
  } // getDeviceTransmissionQuality

  void Device::setDeviceValue(uint8_t _value) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setValue(*this, _value);
    }
  } // setDeviceValue (8)

  void Device::setDeviceOutputValue(uint8_t _offset, uint16_t _value) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if(_offset & 1) {
      setDeviceConfig(CfgClassRuntime, _offset, _value & 0xff);
    } else {
      setDeviceConfig(CfgClassRuntime, _offset+1, (_value >> 8) & 0xff);
      setDeviceConfig(CfgClassRuntime, _offset, _value & 0xff);
    }
  } // setDeviceOutputValue (offset)

  uint16_t Device::getDeviceOutputValue(uint8_t _offset) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkReadAccess();
    }
    uint16_t result = getDeviceConfigWord(CfgClassRuntime, _offset);
    if(_offset == 0) result &= 0xff;   // fix offset 0 value which is 8-bit actually
    return result;
  } // getDeviceOutputValue (offset)

  uint32_t Device::getDeviceSensorValue(const int _sensorIndex) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkReadAccess();
    }
    return m_pApartment->getDeviceBusInterface()->getSensorValue(*this, _sensorIndex);
  } // getDeviceSensorValue

  uint8_t Device::getDeviceSensorType(const int _sensorIndex) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkReadAccess();
    }
    if (getRevisionID() > 0x0321) {
      uint16_t value = getDeviceConfigWord(CfgClassDevice, CfgDevice_SensorParameter + _sensorIndex * 2);
      return (value & 0xFF00) >> 8;
    } else {
      return m_pApartment->getDeviceBusInterface()->getSensorType(*this, _sensorIndex);
    }
  } // getDeviceSensorType

  unsigned long Device::getPowerConsumption() {
    return m_Consumption;
  } // getPowerConsumption

  unsigned long Device::getEnergymeterValue() {
    return m_Energymeter;
  } // getEnergymeterValue

  void Device::nextScene(const callOrigin_t _origin) {
    callScene(_origin, SceneHelper::getNextScene(m_LastCalledScene), false);
  } // nextScene

  void Device::previousScene(const callOrigin_t _origin) {
    callScene(_origin, SceneHelper::getNextScene(m_LastCalledScene), false);
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
        if((m_pPropertyNode != NULL) && (m_pApartment->getPropertyNode() != NULL)) {
          // create alias in group list
          std::string gPath = "zones/zone" + intToString(m_ZoneID) + "/groups/group" + intToString(_groupID) + "/devices/"  +  m_DSID.toString();
          PropertyNodePtr gnode = m_pApartment->getPropertyNode()->createProperty(gPath);
          if (gnode) {
            gnode->alias(m_pPropertyNode);
          }
          // create group-n property
          PropertyNodePtr gsubnode = m_pPropertyNode->createProperty("groups/group" + intToString(_groupID));
          gsubnode->createProperty("id")->setIntegerValue(_groupID);
        }
      } else {
        Logger::getInstance()->log("Device " + m_DSID.toString() + " (bus: " + intToString(m_ShortAddress) + ", zone: " + intToString(m_ZoneID) + ") is already in group " + intToString(_groupID));
      }
    } else {
      Logger::getInstance()->log("Device::addToGroup: Group ID out of bounds: " + intToString(_groupID), lsInfo);
    }
  } // addToGroup

  void Device::removeFromGroup(const int _groupID) {
    if((_groupID > 0) && (_groupID <= GroupIDMax)) {
      m_GroupBitmask.reset(_groupID-1);
      std::vector<int>::iterator it = find(m_Groups.begin(), m_Groups.end(), _groupID);
      if(it != m_Groups.end()) {
        m_Groups.erase(it);
        if((m_pPropertyNode != NULL) && (m_pApartment->getPropertyNode() != NULL)) {
          // remove alias in group list
          int zid = m_ZoneID > 0 ? m_ZoneID : m_LastKnownZoneID;
          std::string gPath = "zones/zone" + intToString(zid) + "/groups/group" + intToString(_groupID) + "/devices/"  +  m_DSID.toString();
          PropertyNodePtr gnode = m_pApartment->getPropertyNode()->getProperty(gPath);
          if (gnode) {
            gnode->getParentNode()->removeChild(gnode);
          }
          // remove property in device group list
          PropertyNodePtr gsubnode = m_pPropertyNode->getProperty("groups/group" + intToString(_groupID));
          if (gsubnode) {
            gsubnode->getParentNode()->removeChild(gsubnode);
          }
        }
      }
    } else {
      Logger::getInstance()->log("Device::removeFromGroup: Group ID out of bounds: " + intToString(_groupID), lsInfo);
    }
  } // removeFromGroup

  void Device::resetGroups() {
    m_GroupBitmask.reset();
    std::vector<int>::iterator it;
    while (m_Groups.size() > 0) {
      int g = m_Groups.front();
      removeFromGroup(g);
    }
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
    } else {
      result = m_GroupBitmask.test(_groupID - 1);
    }
    return result;
  } // isInGroup

  Apartment& Device::getApartment() const {
    return *m_pApartment;
  } // getApartment

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

  std::string Device::getSensorEventName(const int _eventIndex) const {
    if (m_pPropertyNode != NULL) {
      PropertyNodePtr nameNode = m_pPropertyNode->getProperty("sensorEvents/event" + intToString(_eventIndex));
      if (nameNode) {
        return nameNode->getStringValue();
      }
    }
    return "";
  }

  void Device::setSensorEventName(const int _eventIndex, std::string& _name) {
    if (m_pPropertyNode != NULL) {
      PropertyNodePtr nameNode = m_pPropertyNode->createProperty("sensorEvents/event" + intToString(_eventIndex));
      if (_name.empty()) {
        m_pPropertyNode->getProperty("sensorEvents")->removeChild(nameNode);
      } else {
        nameNode->setStringValue(_name);
        nameNode->setFlag(PropertyNode::Archive, true);
      }
      dirty();
    }
  }

  void Device::getSensorEventEntry(const int _eventIndex, DeviceSensorEventSpec_t& _entry) {
    if(_eventIndex > 15) {
      throw DSSException("Device::getSensorEventEntry: index out of range");
    }
    _entry.name = getSensorEventName(_eventIndex);
    uint16_t value = getDeviceConfigWord(CfgClassSensorEvent, CfgFSensorEvent_TableSize * _eventIndex + 0);
    _entry.sensorIndex = (value & 0x00F0) >> 4;
    _entry.test = (value & 0x000C) >> 2;
    _entry.action = (value & 0x0003);
    _entry.value = (value & 0xFF00) >> 4;
    value = getDeviceConfigWord(CfgClassSensorEvent, CfgFSensorEvent_TableSize * _eventIndex + 2);
    _entry.value |= (value & 0x00F0) >> 4;
    _entry.hysteresis = ((value & 0x000F) << 8) | ((value & 0xFF00) >> 8);
    value = getDeviceConfigWord(CfgClassSensorEvent, CfgFSensorEvent_TableSize * _eventIndex + 4);
    _entry.sceneDeviceMode = (value & 0x000C) >> 2;
    _entry.validity = (value & 0x0003);
    _entry.buttonNumber = (value & 0xF000) >> 12;
    _entry.clickType = (value & 0x0F00) >> 8;
    _entry.sceneID = (value & 0x7F00) >> 8;
  }

  void Device::setSensorEventEntry(const int _eventIndex, DeviceSensorEventSpec_t _entry) {
    if(_eventIndex > 15) {
      throw DSSException("Device::setSensorEventEntry: index out of range");
    }
    if (getRevisionID() < 0x0328) {
      /* older devices have a bug, where the hysteresis setting leads to strange behavior */
      _entry.hysteresis = 0;
    }
    setSensorEventName(_eventIndex, _entry.name);
    uint8_t value;
    value = (_entry.sensorIndex << 4) | (_entry.test << 2) | (_entry.action);
    setDeviceConfig(CfgClassSensorEvent, CfgFSensorEvent_TableSize * _eventIndex + 0, value);
    value = (_entry.value & 0xFF0) >> 4;
    setDeviceConfig(CfgClassSensorEvent, CfgFSensorEvent_TableSize * _eventIndex + 1, value);
    value = ((_entry.value & 0x00F) << 4) | ((_entry.hysteresis & 0xF00) >> 8);
    setDeviceConfig(CfgClassSensorEvent, CfgFSensorEvent_TableSize * _eventIndex + 2, value);
    value = (_entry.hysteresis & 0x0FF);
    setDeviceConfig(CfgClassSensorEvent, CfgFSensorEvent_TableSize * _eventIndex + 3, value);
    value = (_entry.sceneDeviceMode << 2) | (_entry.validity);
    setDeviceConfig(CfgClassSensorEvent, CfgFSensorEvent_TableSize * _eventIndex + 4, value);
    if (_entry.action == 2) {
      if (!isSceneDevice()) {
        value = (_entry.buttonNumber << 4) | (_entry.clickType);
      } else {
        value = _entry.sceneID;
      }
      setDeviceConfig(CfgClassSensorEvent, CfgFSensorEvent_TableSize * _eventIndex + 5, value);
    }
  }

  bool Device::hasExtendendSceneTable() {
    if ((((getFunctionID() & 0xf000) >> 12) == GroupIDGray) &&
        (((getProductID() >> 10) & 0x3f) == DeviceTypeKL)) {
      return true;
    }
    return false;
  }

  void Device::setSceneValue(const int _scene, const int _value) {
    if (hasExtendendSceneTable()) {
      int extValue = getDeviceConfig(CfgClassSceneExtention, _scene / 2);
      if ((_scene % 2) == 0) {
        extValue &= 0xF0;
        extValue |= (_value >> 4) & 0x0F;
      } else {
        extValue &= 0x0F;
        extValue |= _value & 0xF0;
      }
      setDeviceConfig(CfgClassScene, _scene, (_value >> 8) & 0xFF);
      setDeviceConfig(CfgClassSceneExtention, _scene / 2, extValue);
    } else {
      setDeviceConfig(CfgClassScene, _scene, _value);
    }
  }

  int Device::getSceneValue(const int _scene) {
    int value = getDeviceConfig(CfgClassScene, _scene);
    if (hasExtendendSceneTable()) {
      value <<= 8;
      int extValue = getDeviceConfig(CfgClassSceneExtention, _scene / 2);
      if ((_scene % 2) == 0) {
        extValue &= 0xF0;
        value |= extValue & (extValue >> 4);
      } else {
        extValue &= 0x0F;
        value |= (extValue << 4) & extValue;
      }
    }
    return value;
  }

  void Device::configureAreaMembership(const int _areaScene, const bool _addToArea) {
    if ((_areaScene < SceneOffA1) || (_areaScene == Scene1) || (_areaScene > SceneA41)) {
      throw DSSException("Device::configureAreaMembership: scene number does not indicate an area");
    }
    DeviceSceneSpec_t sceneSpec;

    int areaOnScene = (_areaScene >= SceneA11) ? _areaScene : (_areaScene - SceneOffA1 + SceneA11);
    getDeviceSceneMode(areaOnScene, sceneSpec);
    sceneSpec.dontcare = !_addToArea;
    setDeviceSceneMode(areaOnScene, sceneSpec);
    if (_addToArea) {
      setSceneValue(areaOnScene, 0xFFFF);
    }

    int areaOffScene = areaOnScene - SceneA11 + SceneOffA1;
    getDeviceSceneMode(areaOffScene, sceneSpec);
    sceneSpec.dontcare = !_addToArea;
    setDeviceSceneMode(areaOffScene, sceneSpec);
    if (_addToArea) {
      setSceneValue(areaOffScene, 0);
    }
  }

} // namespace dss
