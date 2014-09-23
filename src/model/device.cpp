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
#include <digitalSTROM/dsm-api-v2/dsm-api.h>
#include <digitalSTROM/dsuid/dsuid.h>

#include "src/businterface.h"
#include "src/propertysystem.h"
#include "src/ds485types.h"

#include "src/model/modelconst.h"
#include "src/model/scenehelper.h"
#include "src/model/modelevent.h"
#include "src/model/apartment.h"
#include "src/model/modelmaintenance.h"
#include "src/model/modulator.h"
#include "src/model/devicereference.h"
#include "src/model/state.h"

#include <boost/shared_ptr.hpp>

namespace dss {

  //================================================== Device

  Device::Device(dsuid_t _dsid, Apartment* _pApartment)
  : AddressableModelItem(_pApartment),
    m_DSID(_dsid),
    m_ShortAddress(ShortAddressStaleDevice),
    m_LastKnownShortAddress(ShortAddressStaleDevice),
    m_ZoneID(0),
    m_LastKnownZoneID(0),
    m_DSMeterDSID(),
    m_LastKnownMeterDSID(),
    m_DSMeterDSIDstr(),
    m_LastKnownMeterDSIDstr(),
    m_FunctionID(0),
    m_ProductID(0),
    m_VendorID(0),
    m_RevisionID(0),
    m_LastCalledScene(SceneOff),
    m_LastButOneCalledScene(SceneOff),
    m_Consumption(0),
    m_Energymeter(0),
    m_LastDiscovered(DateTime::NullDate),
    m_FirstSeen(DateTime::NullDate),
    m_IsValid(false),
    m_IsLockedInDSM(false),
    m_OutputMode(0),
    m_ButtonInputMode(0),
    m_ButtonInputIndex(0),
    m_ButtonInputCount(0),
    m_ButtonSetsLocalPriority(false),
    m_ButtonCallsPresent(true),
    m_ButtonGroupMembership(0),
    m_ButtonActiveGroup(0),
    m_ButtonID(0),
    m_OemEanNumber(0),
    m_OemSerialNumber(0),
    m_OemPartNumber(0),
    m_OemInetState(DEVICE_OEM_EAN_NO_EAN_CONFIGURED),
    m_OemState(DEVICE_OEM_UNKOWN),
    m_OemIsIndependent(true),
    m_HWInfo(),
    m_iconPath("unknown.png"),
    m_OemProductInfoState(DEVICE_OEM_UNKOWN),
    m_OemProductName(),
    m_OemProductIcon(),
    m_OemProductURL(),
    m_IsConfigLocked(false),
    m_binaryInputCount(0),
    m_sensorInputCount(0),
    m_outputChannelCount(0),
    m_AKMInputProperty(),
    m_isVdcDevice(false)
    {
      SetNullDsuid(m_DSMeterDSID);
      SetNullDsuid(m_LastKnownMeterDSID);
      m_DSMeterDSUIDstr = dsuid2str(m_DSMeterDSID);
      m_LastKnownMeterDSUIDstr = dsuid2str(m_LastKnownMeterDSID);

      dsid_t dsid;
      if (dsuid_to_dsid(m_DSMeterDSID, &dsid)) {
        m_DSMeterDSIDstr = dsid2str(dsid);
      }
      if (dsuid_to_dsid(m_LastKnownMeterDSID, &dsid)) {
        m_LastKnownMeterDSIDstr = dsid2str(dsid);
      }
    } // ctor

  Device::~Device() {
    removeFromPropertyTree();
  }

  void Device::removeFromPropertyTree() {
    if(m_pPropertyNode != NULL) {
      if ((!IsNullDsuid(m_DSMeterDSID)) && (!IsNullDsuid(m_DSID))) {
        std::string devicePath = "devices/" + dsuid2str(m_DSID);
        PropertyNodePtr dev = m_pApartment->getDSMeterByDSID(m_DSMeterDSID)->getPropertyNode()->getProperty(devicePath);
        dev->alias(PropertyNodePtr());
        dev->getParentNode()->removeChild(dev);
      }

      if (m_pApartment->getPropertyNode() != NULL) {
        for (int g = 1; g <= 63; g++) {
          if (m_GroupBitmask.test(g-1)) {
            int zid = m_ZoneID > 0 ? m_ZoneID : m_LastKnownZoneID;
            std::string gPath = "zones/zone" + intToString(zid) +
                "/groups/group" + intToString(g) + "/devices/" +
                dsuid2str(m_DSID);
            PropertyNodePtr gnode = m_pApartment->getPropertyNode()->getProperty(gPath);
            if (gnode != NULL) {
              gnode->alias(PropertyNodePtr());
              gnode->getParentNode()->removeChild(gnode);
            }
            PropertyNodePtr gsubnode = m_pPropertyNode->getProperty("groups/group" + intToString(g));
            if (gsubnode != NULL) {
              gsubnode->getParentNode()->removeChild(gsubnode);
            }
          }
        }
      }

      m_pPropertyNode->unlinkProxy(true);
      PropertyNode *parent = m_pPropertyNode->getParentNode();
      if (parent != NULL) {
        parent->removeChild(m_pPropertyNode);
      }
      m_pPropertyNode.reset();
    }
    if(m_pAliasNode != NULL) {
      m_pAliasNode->alias(PropertyNodePtr());
      PropertyNode *parent = m_pAliasNode->getParentNode();
      if (parent != NULL) {
        parent->removeChild(m_pAliasNode);
      }
      m_pAliasNode.reset();
    }
  }

  void Device::publishToPropertyTree() {
    if(m_pPropertyNode == NULL) {
      if(m_pApartment->getPropertyNode() != NULL) {
        m_pPropertyNode = m_pApartment->getPropertyNode()->createProperty("zones/zone0/devices/" + dsuid2str(m_DSID));

        dsid_t dsid;
        if (dsuid_to_dsid(m_DSID, &dsid)) {
          m_pPropertyNode->createProperty("dSID")->setStringValue(dsid2str(dsid));
        } else {
          m_pPropertyNode->createProperty("dSID")->setStringValue(dsuid2str(m_DSID));
        }

        m_pPropertyNode->createProperty("dSUID")->setStringValue(dsuid2str(m_DSID));
        m_pPropertyNode->createProperty("present")
          ->linkToProxy(PropertyProxyMemberFunction<Device, bool>(*this, &Device::isPresent));
        m_pPropertyNode->createProperty("name")
          ->linkToProxy(PropertyProxyMemberFunction<Device, std::string>(*this, &Device::getName, &Device::setName));
        if (!m_DSMeterDSIDstr.empty()) {
          m_pPropertyNode->createProperty("DSMeterDSID")
            ->linkToProxy(PropertyProxyReference<std::string>(m_DSMeterDSIDstr, false));
        } else {
          m_pPropertyNode->createProperty("DSMeterDSID")
            ->linkToProxy(PropertyProxyReference<std::string>(m_DSMeterDSUIDstr, false));
        }
        m_pPropertyNode->createProperty("DSMeterDSUID")
          ->linkToProxy(PropertyProxyReference<std::string>(m_DSMeterDSUIDstr, false));

        m_pPropertyNode->createProperty("ZoneID")->linkToProxy(PropertyProxyReference<int>(m_ZoneID, false));
        m_pPropertyNode->createProperty("functionID")
          ->linkToProxy(PropertyProxyReference<int>(m_FunctionID, false));
        m_pPropertyNode->createProperty("revisionID")
          ->linkToProxy(PropertyProxyReference<int>(m_RevisionID, false));
        m_pPropertyNode->createProperty("productID")
          ->linkToProxy(PropertyProxyReference<int>(m_ProductID, false));
        m_pPropertyNode->createProperty("vendorID")
          ->linkToProxy(PropertyProxyReference<int>(m_VendorID, false));
        m_pPropertyNode->createProperty("HWInfo")
          ->linkToProxy(PropertyProxyReference<std::string>(m_HWInfo, false));
        m_pPropertyNode->createProperty("GTIN")
          ->linkToProxy(PropertyProxyReference<std::string>(m_GTIN, false));
        PropertyNodePtr oemNode = m_pPropertyNode->createProperty("productInfo");
        oemNode->createProperty("ProductState")
          ->linkToProxy(PropertyProxyMemberFunction<Device, std::string, false>(*this, &Device::getOemProductInfoStateAsString));
        oemNode->createProperty("ProductName")
          ->linkToProxy(PropertyProxyReference<std::string>(m_OemProductName, false));
        oemNode->createProperty("ProductIcon")
          ->linkToProxy(PropertyProxyReference<std::string>(m_OemProductIcon, false));
        oemNode->createProperty("ProductURL")
          ->linkToProxy(PropertyProxyReference<std::string>(m_OemProductURL, false));
        oemNode->createProperty("State")
          ->linkToProxy(PropertyProxyMemberFunction<Device, std::string, false>(*this, &Device::getOemStateAsString));
        oemNode->createProperty("EAN")
          ->linkToProxy(PropertyProxyMemberFunction<Device, std::string, false>(*this, &Device::getOemEanAsString));
        oemNode->createProperty("SerialNumber")
          ->linkToProxy(PropertyProxyReference<int, uint16_t>(m_OemSerialNumber, false));
        oemNode->createProperty("PartNumber")
          ->linkToProxy(PropertyProxyReference<int, uint8_t>(m_OemPartNumber, false));
        oemNode->createProperty("isIndependent")
          ->linkToProxy(PropertyProxyReference<bool>(m_OemIsIndependent, false));
        oemNode->createProperty("InternetState")
          ->linkToProxy(PropertyProxyMemberFunction<Device, std::string, false>(*this, &Device::getOemInetStateAsString));
        oemNode->createProperty("configurationLocked")
          ->linkToProxy(PropertyProxyReference<bool>(m_IsConfigLocked, false));

        m_pPropertyNode->createProperty("isVdcDevice")
          ->linkToProxy(PropertyProxyReference<bool>(m_isVdcDevice, false));
        PropertyNodePtr propNode = m_pPropertyNode->createProperty("properties");
        propNode->createProperty("ModelGuid")
          ->linkToProxy(PropertyProxyReference<std::string>(m_VdcModelGuid, false));
        propNode->createProperty("VendorGuid")
          ->linkToProxy(PropertyProxyReference<std::string>(m_VdcVendorGuid, false));
        propNode->createProperty("OemGuid")
          ->linkToProxy(PropertyProxyReference<std::string>(m_VdcOemGuid, false));
        propNode->createProperty("ConfigURL")
          ->linkToProxy(PropertyProxyReference<std::string>(m_VdcConfigURL, false));
        propNode->createProperty("HardwareGuid")
          ->linkToProxy(PropertyProxyReference<std::string>(m_VdcHardwareGuid, false));
        propNode->createProperty("HardwareInfo")
          ->linkToProxy(PropertyProxyReference<std::string>(m_VdcHardwareInfo, false));
        propNode->createProperty("HardwareVersion")
          ->linkToProxy(PropertyProxyReference<std::string>(m_VdcHardwareVersion, false));

        m_pPropertyNode->createProperty("lastKnownZoneID")
          ->linkToProxy(PropertyProxyReference<int>(m_LastKnownZoneID, false));
        m_pPropertyNode->createProperty("shortAddress")
          ->linkToProxy(PropertyProxyReference<int, uint16_t>(m_ShortAddress, false));
        m_pPropertyNode->createProperty("lastKnownShortAddress")
          ->linkToProxy(PropertyProxyReference<int, uint16_t>(m_LastKnownShortAddress, false));
        if (!m_LastKnownMeterDSIDstr.empty()) {
          m_pPropertyNode->createProperty("lastKnownMeterDSID")
            ->linkToProxy(PropertyProxyReference<std::string>(m_LastKnownMeterDSIDstr, false));
        } else {
          m_pPropertyNode->createProperty("lastKnownMeterDSID")
            ->linkToProxy(PropertyProxyReference<std::string>(m_LastKnownMeterDSUIDstr, false));
        }
        m_pPropertyNode->createProperty("lastKnownMeterDSUID")
          ->linkToProxy(PropertyProxyReference<std::string>(m_LastKnownMeterDSUIDstr, false));
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
        m_pPropertyNode->createProperty("button/callsPresent")
          ->linkToProxy(PropertyProxyReference<bool>(m_ButtonCallsPresent));
        if (!m_pPropertyNode->getProperty("sensorEvents")) {
          PropertyNodePtr sensorNode = m_pPropertyNode->createProperty("sensorEvents");
        }
        PropertyNodePtr binaryInputNode = m_pPropertyNode->createProperty("binaryInputs");
        PropertyNodePtr sensorInputNode = m_pPropertyNode->createProperty("sensorInputs");
        PropertyNodePtr outputChannelNode = m_pPropertyNode->createProperty("outputChannels");

        m_TagsNode = m_pPropertyNode->createProperty("tags");
        m_TagsNode->setFlag(PropertyNode::Archive, true);

        if (m_ZoneID != 0) {
          std::string basePath = "zones/zone" + intToString(m_ZoneID) +
                                 "/devices";
          if(m_pAliasNode == NULL) {
            PropertyNodePtr node = m_pApartment->getPropertyNode()->getProperty(basePath + "/" + dsuid2str(m_DSID));
            if ((node == NULL) || ((node != NULL) && (node->size() == 0))) {
              m_pAliasNode = m_pApartment->getPropertyNode()->createProperty(basePath + "/" + dsuid2str(m_DSID));
              m_pAliasNode->alias(m_pPropertyNode);
            }
          } else {
            PropertyNodePtr base = m_pApartment->getPropertyNode()->getProperty(basePath);
            if (base != NULL) {
              base->addChild(m_pAliasNode);
            }
          }
        }

        for (int g = 1; g <= 63; g++) {
          if (m_GroupBitmask.test(g-1)) {
            std::string gPath = "zones/zone" + intToString(m_ZoneID) +
                                "/groups/group" + intToString(g) + "/devices/" +
                                dsuid2str(m_DSID);
            PropertyNodePtr gnode = m_pApartment->getPropertyNode()->createProperty(gPath);
            if (gnode) {
              gnode->alias(m_pPropertyNode);
            }
            PropertyNodePtr gsubnode = m_pPropertyNode->createProperty("groups/group" + intToString(g));
            gsubnode->createProperty("id")->setIntegerValue(g);
          }
        }

        if (!IsNullDsuid(m_DSMeterDSID)) {
          setDSMeter(m_pApartment->getDSMeterByDSID(m_DSMeterDSID));
        }
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
    if ((m_FunctionID != 0) && (m_ProductID != 0) && (m_VendorID != 0)) {
      calculateHWInfo();
    }
    updateIconPath();
  } // setFunctionID

  int Device::getProductID() const {
    return m_ProductID;
  } // getProductID

  void Device::setProductID(const int _value) {
    m_ProductID = _value;
    if ((m_FunctionID != 0) && (m_ProductID != 0) && (m_VendorID != 0)) {
      calculateHWInfo();
    }
    updateIconPath();

    if ((getDeviceType() == DEVICE_TYPE_AKM) && (m_pPropertyNode != NULL)) {
        m_pPropertyNode->createProperty("AKMInputProperty")
          ->linkToProxy(PropertyProxyReference<std::string>(m_AKMInputProperty, false));
    }
  } // setProductID

  int Device::getRevisionID() const {
    return m_RevisionID;
  } // getRevisionID

  void Device::setRevisionID(const int _value) {
    m_RevisionID = _value;
  } // setRevisionID

  int Device::getVendorID() const {
    return m_VendorID;
  } // getVendrID

  void Device::setVendorID(const int _value) {
    m_VendorID = _value;
    if ((m_FunctionID != 0) && (m_ProductID != 0) && (m_VendorID != 0)) {
      calculateHWInfo();
    }
    updateIconPath();
  } // setVendorID

  void Device::fillSensorTable(std::vector<DeviceSensorSpec_t>& _slist) {
    DeviceSensorSpec_t sensorInputReserved1 = { 0x3d, 0, 0, 0 };
    DeviceSensorSpec_t sensorInputReserved2 = { 0x3e, 0, 0, 0 };
    DeviceSensorSpec_t sensorInput04 = { SensorIDActivePower, 0, 0, 0 };
    DeviceSensorSpec_t sensorInput05 = { SensorIDOutputCurrent, 0, 0, 0 };
    DeviceSensorSpec_t sensorInput06 = { SensorIDElectricMeter, 0, 0, 0 };
    DeviceSensorSpec_t sensorInput64 = { SensorIDOutputCurrent16A, 0, 0, 0 };
    DeviceSensorSpec_t sensorInput65 = { SensorIDActivePowerVA, 0, 0, 0 };
    DeviceClasses_t deviceClass = getDeviceClass();
    int devType = (deviceClass << 16) | m_ProductID;

    /* common */
    _slist.push_back(sensorInputReserved1);
    _slist.push_back(sensorInputReserved2);
    _slist.push_back(sensorInput04);
    _slist.push_back(sensorInput05);
    _slist.push_back(sensorInput06);

    switch (devType) {
      /* KM, SDM, TKM type with output */
      case 0x100c8:
      case 0x104d2:
      case 0x108c8:
      case 0x300c8:
      case 0x600c8:
      case 0x608c8:
      case 0x700c8:
      case 0x704c8:
      case 0x704d2:
        break;
      /* SDS200 with add. PowerVA sensor */
      case 0x118c8:
      case 0x118c9:
      case 0x118ca:
        _slist.push_back(sensorInput65);
        break;
      /* KL types */
      case 0x10cc8:
      case 0x20cc8:
      case 0x20cd2:
      case 0x20cdc:
      case 0x80cc8:
      case 0x814c8:
      case 0x814c9:
      case 0x814ca:
        _slist.push_back(sensorInput64);
        _slist.push_back(sensorInput65);
        break;
      /* all others - like TKM without output */
      default:
        _slist.clear();
        break;
    }
  } // fillSensorTable

  void Device::setDeviceConfig(uint8_t _configClass, uint8_t _configIndex, uint8_t _value) {
    if (m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (m_pApartment->getDeviceBusInterface() == NULL) {
      throw std::runtime_error("DeviceBusInterface missing");
    }
    if (m_pApartment->getModelMaintenance() == NULL) {
      throw std::runtime_error("ModelMaintenance missing");
    }

    m_pApartment->getDeviceBusInterface()->setDeviceConfig(
        *this, _configClass, _configIndex, _value);

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceConfigChanged,
                                                m_DSMeterDSID);
    pEvent->addParameter(m_ShortAddress);
    pEvent->addParameter(_configClass);
    pEvent->addParameter(_configIndex);
    pEvent->addParameter(_value);
    m_pApartment->getModelMaintenance()->addModelEvent(pEvent);
  } // setDeviceConfig

  void Device::setDeviceConfig16(uint8_t _configClass, uint8_t _configIndex, uint16_t _value)
  {
    /*
     * webroot/js/dss/dss-setup-interface/dSS/util/Util.js: dSS.util.decode16
     * -- used to configure lamella time(uint16_t) for KLEMME-GR using
     *  2x setDeviceConfig
     */
    if (m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (m_pApartment->getDeviceBusInterface() == NULL) {
      throw std::runtime_error("DeviceBusInterface missing");
    }
    if (m_pApartment->getModelMaintenance() == NULL) {
      throw std::runtime_error("ModelMaintenance missing");
    }
    DeviceBusInterface *shorty = m_pApartment->getDeviceBusInterface();

    uint8_t low = (_value & 0x000000ff);
    uint8_t high = (_value & 0x0000ff00) >> 8;

    shorty->setDeviceConfig(*this, _configClass, _configIndex, low);
    shorty->setDeviceConfig(*this, _configClass, _configIndex + 1, high);

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceConfigChanged,
                                                m_DSMeterDSID);
    pEvent->addParameter(m_ShortAddress);
    pEvent->addParameter(_configClass);
    pEvent->addParameter(_configIndex);
    pEvent->addParameter(_value);
    m_pApartment->getModelMaintenance()->addModelEvent(pEvent);
  }

  void Device::setDeviceButtonID(uint8_t _buttonId) {
    setButtonID(_buttonId);
    setDeviceConfig(CfgClassFunction, CfgFunction_ButtonMode,
        ((m_ButtonGroupMembership & 0xf) << 4) | (_buttonId & 0xf));
  } // setDeviceButtonId

  void Device::setDeviceButtonActiveGroup(uint8_t _buttonActiveGroup) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setDeviceButtonActiveGroup(*this,
                                                                        _buttonActiveGroup);
      if (_buttonActiveGroup >= GroupIDAppUserMin &&
          _buttonActiveGroup <= GroupIDAppUserMax &&
          ((m_ButtonID < ButtonId_Zone) || (m_ButtonID >= ButtonId_Area1_Extended))) {
        setDeviceButtonID(ButtonId_Zone);
      }
      /* refresh device information for correct active group */
      if((m_pApartment != NULL) && (m_pApartment->getModelMaintenance() != NULL)) {
        ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceChanged,
                                                    m_DSMeterDSID);
        pEvent->addParameter(m_ShortAddress);
        m_pApartment->getModelMaintenance()->addModelEvent(pEvent);
      }
    }
  } // setDeviceActiveGroup

  void Device::setDeviceJokerGroup(uint8_t _groupId) {
    if ((_groupId < GroupIDYellow) || (_groupId > GroupIDStandardMax)) {
      throw std::runtime_error("Invalid joker group value");
    }
    // for standard groups force that only one group is active
    for (int g = GroupIDYellow; g <= GroupIDStandardMax; g++) {
      if (m_GroupBitmask.test(g-1)) {
        removeFromGroup(g);
      }
    }
    addToGroup(_groupId);
    // propagate target group value to device
    setButtonGroupMembership(_groupId);
    setDeviceConfig(CfgClassFunction, CfgFunction_ButtonMode,
        ((_groupId & 0xf) << 4) | (m_ButtonID & 0xf));

    updateIconPath();
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

 void Device::increaseDeviceOutputChannelValue(uint8_t _channel) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->increaseDeviceOutputChannelValue(*this, _channel);
    }
  } // increaseDeviceOutputChannelValue

  void Device::decreaseDeviceOutputChannelValue(uint8_t _channel) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->decreaseDeviceOutputChannelValue(*this, _channel);
    }
  } // decreaseDeviceOutputChannelValue

  void Device::stopDeviceOutputChannelValue(uint8_t _channel) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->stopDeviceOutputChannelValue(*this, _channel);
    }
  } // decreaseDeviceOutputChannelValue

  uint16_t Device::getDeviceOutputChannelValue(uint8_t _channel) {
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getDeviceOutputChannelValue(*this, _channel);
    }
    throw std::runtime_error("Bus interface not available");
  } // getDeviceOutputChannelValue

  void Device::setDeviceOutputChannelValue(uint8_t _channel, uint8_t _size,
                                   uint16_t _value, bool _applyNow) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setDeviceOutputChannelValue(*this, _channel, _size, _value, _applyNow);
    }
  } // setDeviceOutputChannelValue

  uint16_t Device::getDeviceOutputChannelSceneValue(uint8_t _channel,
                                                    uint8_t _scene) {
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getDeviceOutputChannelSceneValue(*this, _channel, _scene);
    }
    throw std::runtime_error("Bus interface not available");
  } // getDeviceOutputChannelSceneValue

  void Device::setDeviceOutputChannelSceneValue(uint8_t _channel,
                                                uint8_t _size,
                                                uint8_t _scene,
                                                uint16_t _value) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setDeviceOutputChannelSceneValue(*this, _channel, _size, _scene, _value);
    }
  }

  void Device::getDeviceOutputChannelSceneConfig(uint8_t _scene,
                                                 DeviceSceneSpec_t& _config) {
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      uint16_t mode = m_pApartment->getDeviceBusInterface()->getDeviceOutputChannelSceneConfig(*this, _scene);
      _config.dontcare = (mode & 1) > 0;
      _config.localprio = (mode & 2) > 0;
      _config.specialmode = (mode & 4) > 0;
      _config.ledconIndex = (mode >> 4) & 7;
      _config.dimtimeIndex = 0; // no dim time index
      _config.flashmode = (mode & (1 << 8)) > 0; // effect 1
    }
    throw std::runtime_error("Bus interface not available");
  } // getDeviceOutputChannelSceneConfig

  void Device::setDeviceOutputChannelSceneConfig(uint8_t _scene,
                                                 DeviceSceneSpec_t _config) {
    if(m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      uint16_t mode = _config.dontcare ? 1 : 0;
      mode |= _config.localprio ? 2 : 0;
      mode |= _config.specialmode ? 4 : 0;
      mode |= ((_config.ledconIndex & 7) << 4);
      mode |= _config.flashmode ? (1 << 8) : 0;

      m_pApartment->getDeviceBusInterface()->setDeviceOutputChannelSceneConfig(*this, _scene, mode);
    }
  } // setDeviceOutputChannelSceneConfig

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

  void Device::setDeviceOutputChannelDontCareFlags(uint8_t _scene,
                                                   uint16_t _value) {
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setDeviceOutputChannelDontCareFlags(*this, _scene, _value);
    }
  }

  uint16_t Device::getDeviceOutputChannelDontCareFlags(uint8_t _scene) {
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getDeviceOutputChannelDontCareFlags(*this, _scene);
    }
    throw std::runtime_error("Bus interface not available");
  }

  int Device::getSceneAngle(const int _scene) {
    DeviceFeatures_t features = getFeatures();
    if (features.hasOutputAngle) {
      return getDeviceConfig(CfgClassSceneAngle, _scene);
    } else {
      throw std::runtime_error("Device does not support output angle setting");
    }
  }

  void Device::setSceneAngle(const int _scene, const int _angle) {
    DeviceFeatures_t features = getFeatures();
    if (!features.hasOutputAngle) {
      throw std::runtime_error("Device does not support output angle setting");
    }
    if ((_angle < 0) || (_angle > 255)) {
      throw std::runtime_error("Device angle value out of bounds");
    }

    setDeviceConfig(CfgClassSceneAngle, _scene, _angle);
  }

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

  /** Configure climate actuator */
  void Device::setDeviceValveTimer(DeviceValveTimerSpec_t _config) {
    if (getDeviceClass() != DEVICE_CLASS_BL) {
      throw DSSException("Not a climate device");
    }
    setDeviceConfig(CfgClassFunction, CfgFunction_Valve_EmergencyValue, _config.emergencyControlValue);
    setDeviceConfig16(CfgClassFunction, CfgFunction_Valve_EmergencyTimer, _config.emergencyTimer);
    setDeviceConfig16(CfgClassFunction, CfgFunction_Valve_ProtectionTimer, _config.protectionTimer);
  } // setDeviceValveTimer

  void Device::getDeviceValveTimer(DeviceValveTimerSpec_t& _config) {
    if (getDeviceClass() != DEVICE_CLASS_BL) {
      throw DSSException("Not a climate device");
    }
    _config.emergencyControlValue = getDeviceConfig(CfgClassFunction, CfgFunction_Valve_EmergencyValue);
    _config.emergencyTimer = getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_EmergencyTimer);
    _config.protectionTimer = getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_ProtectionTimer);
  } // getDeviceValveTimer

  void Device::setDeviceValvePwm(DeviceValvePwmSpec_t _config) {
    if (getDeviceClass() != DEVICE_CLASS_BL) {
      throw DSSException("Not a climate device");
    }
    setDeviceConfig16(CfgClassFunction, CfgFunction_Valve_PwmPeriod, _config.pwmPeriod);
    setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMinValue, _config.pwmMinValue);
    setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMaxValue, _config.pwmMaxValue);
    setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMinY, _config.pwmMinX);
    setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMaxY, _config.pwmMaxY);
    setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmOffset, _config.pwmOffset);
  } // setDeviceValvePwm

  void Device::getDeviceValvePwm(DeviceValvePwmSpec_t& _config) {
    if (getDeviceClass() != DEVICE_CLASS_BL) {
      throw DSSException("Not a climate device");
    }
    _config.pwmPeriod = getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_PwmPeriod);
    uint16_t value = getDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMinValue);
    _config.pwmMinValue = value & 0xff; // CfgFunction_Valve_PwmMinValue
    _config.pwmMaxValue = (value >> 8) & 0xff; // CfgFunction_Valve_PwmMaxValue
    value = getDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMinY);
    _config.pwmMinX = value & 0xff; // CfgFunction_Valve_PwmMinY
    _config.pwmMaxY = (value >> 8) & 0xff; // CfgFunction_Valve_PwmMaxY
    _config.pwmOffset = getDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmOffset);
  } // getDeviceValvePwm

  void Device::setDeviceValveControl(DeviceValveControlSpec_t _config) {
    if (getDeviceClass() != DEVICE_CLASS_BL) {
      throw DSSException("Not a climate device");
    }
    uint8_t value = _config.ctrlRawValue;
    value &= 0xf0; // clear bits 0..3, reserved bits 4..7
    value |= _config.ctrlClipMinZero ? 1 : 0;
    value |= _config.ctrlClipMinLower? 2 : 0;
    value |= _config.ctrlClipMaxHigher ? 4 : 0;
    value |= _config.ctrlNONC ? 8 : 0;
    setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmConfig, value);
  }

  void Device::getDeviceValveControl(DeviceValveControlSpec_t& _config) {
    if (getDeviceClass() != DEVICE_CLASS_BL) {
      throw DSSException("Not a climate device");
    }
    uint8_t value = getDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmConfig);
    _config.ctrlClipMinZero = ((value & 0x01) == 0x01);
    _config.ctrlClipMinLower = ((value & 0x02) == 0x02);
    _config.ctrlClipMaxHigher = ((value & 0x04) == 0x04);
    _config.ctrlNONC = ((value & 0x08) == 0x08);
    _config.ctrlRawValue = value;
  } // getDeviceValveControl

  uint8_t Device::getDeviceConfig(uint8_t _configClass, uint8_t _configIndex) {
    if(m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getDeviceConfig(*this,
                                                                  _configClass,
                                                                  _configIndex);
    }
    throw std::runtime_error("Bus interface not available");
  } // getDeviceConfig

  uint16_t Device::getDeviceConfigWord(uint8_t _configClass, uint8_t _configIndex) {
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
    uint16_t result = getDeviceConfigWord(CfgClassRuntime, _offset);
    if(_offset == 0) result &= 0xff;   // fix offset 0 value which is 8-bit actually
    return result;
  } // getDeviceOutputValue (offset)

  uint32_t Device::getDeviceSensorValue(const int _sensorIndex) {
    return m_pApartment->getDeviceBusInterface()->getSensorValue(*this, _sensorIndex);
  } // getDeviceSensorValue

  unsigned long Device::getPowerConsumption() {
    return m_Consumption;
  } // getPowerConsumption

  unsigned long Device::getEnergymeterValue() {
    return m_Energymeter;
  } // getEnergymeterValue

  void Device::nextScene(const callOrigin_t _origin, const SceneAccessCategory _category) {
    callScene(_origin, _category, SceneHelper::getNextScene(m_LastCalledScene),  "", false);
  } // nextScene

  void Device::previousScene(const callOrigin_t _origin, const SceneAccessCategory _category) {
    callScene(_origin, _category, SceneHelper::getNextScene(m_LastCalledScene), "", false);
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
    return IsEqualDsuid(_other.m_DSID, m_DSID);
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

  dsuid_t Device::getDSID() const {
    return m_DSID;
  } // getDSID;

  dsuid_t Device::getDSMeterDSID() const {
    return m_DSMeterDSID;
  } // getDSMeterID

  void Device::setLastKnownDSMeterDSID(const dsuid_t& _value) {
    m_LastKnownMeterDSID = _value;
  } // setLastKnownDSMeterDSID

  const dsuid_t& Device::getLastKnownDSMeterDSID() const {
    return m_LastKnownMeterDSID;
  } // getLastKnownDSMeterDSID

  void Device::setDSMeter(boost::shared_ptr<DSMeter> _dsMeter) {
    PropertyNodePtr alias;
    std::string devicePath = "devices/" + dsuid2str(m_DSID);
    if((m_pPropertyNode != NULL) && (!IsNullDsuid(m_DSMeterDSID))) {
      alias = m_pApartment->getDSMeterByDSID(m_DSMeterDSID)->getPropertyNode()->getProperty(devicePath);
    }
    m_DSMeterDSID = _dsMeter->getDSID();
    m_LastKnownMeterDSID = _dsMeter->getDSID();
    m_DSMeterDSUIDstr = dsuid2str(_dsMeter->getDSID());
    m_LastKnownMeterDSUIDstr = dsuid2str(_dsMeter->getDSID());

    dsid_t dsid;
    if (dsuid_to_dsid(m_DSMeterDSID, &dsid)) {
      m_DSMeterDSIDstr = dsid2str(dsid);
    } else {
      m_DSMeterDSIDstr = dsuid2str(m_DSMeterDSID);
    }
    if (dsuid_to_dsid(m_LastKnownMeterDSID, &dsid)) {
      m_LastKnownMeterDSIDstr = dsid2str(dsid);
    } else {
      m_LastKnownMeterDSIDstr = dsuid2str(m_LastKnownMeterDSID);
    }

    if(m_pPropertyNode != NULL) {
      PropertyNodePtr target = _dsMeter->getPropertyNode()->createProperty("devices");
      if(alias != NULL) {
        target->addChild(alias);
      } else {
        alias = target->createProperty(dsuid2str(m_DSID));
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
          PropertyNodePtr node = m_pApartment->getPropertyNode()->getProperty(basePath + "/" + dsuid2str(m_DSID));
          if(node != NULL) {
            Logger::getInstance()->log("Device::setZoneID: Target node for device " + dsuid2str(m_DSID) + " already exists", lsError);
            if(node->size() > 0) {
              Logger::getInstance()->log("Device::setZoneID: Target node for device " + dsuid2str(m_DSID) + " has children", lsFatal);
              return;
            }
          }
          m_pAliasNode = m_pApartment->getPropertyNode()->createProperty(basePath + "/" + dsuid2str(m_DSID));

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
      updateIconPath();
      if(find(m_Groups.begin(), m_Groups.end(), _groupID) == m_Groups.end()) {
        m_Groups.push_back(_groupID);
        if((m_pPropertyNode != NULL) && (m_pApartment->getPropertyNode() != NULL)) {
          // create alias in group list
          std::string gPath = "zones/zone" + intToString(m_ZoneID) + "/groups/group" + intToString(_groupID) + "/devices/"  +  dsuid2str(m_DSID);
          PropertyNodePtr gnode = m_pApartment->getPropertyNode()->createProperty(gPath);
          if (gnode) {
            gnode->alias(m_pPropertyNode);
          }
          // create group-n property
          PropertyNodePtr gsubnode = m_pPropertyNode->createProperty("groups/group" + intToString(_groupID));
          gsubnode->createProperty("id")->setIntegerValue(_groupID);
        }
      } else {
        Logger::getInstance()->log("Device " + dsuid2str(m_DSID) + " (bus: " + intToString(m_ShortAddress) + ", zone: " + intToString(m_ZoneID) + ") is already in group " + intToString(_groupID));
      }
    } else {
      Logger::getInstance()->log("Device::addToGroup: Group ID out of bounds: " + intToString(_groupID), lsInfo);
    }
  } // addToGroup

  void Device::removeFromGroup(const int _groupID) {
    if((_groupID > 0) && (_groupID <= GroupIDMax)) {
      m_GroupBitmask.reset(_groupID-1);
      updateIconPath();
      std::vector<int>::iterator it = find(m_Groups.begin(), m_Groups.end(), _groupID);
      if(it != m_Groups.end()) {
        m_Groups.erase(it);
        if((m_pPropertyNode != NULL) && (m_pApartment->getPropertyNode() != NULL)) {
          // remove alias in group list
          int zid = m_ZoneID > 0 ? m_ZoneID : m_LastKnownZoneID;
          std::string gPath = "zones/zone" + intToString(zid) + "/groups/group" + intToString(_groupID) + "/devices/"  +  dsuid2str(m_DSID);
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
    value = ((_entry.sceneDeviceMode & 0x03) << 2) | (_entry.validity & 0x03);
    setDeviceConfig(CfgClassSensorEvent, CfgFSensorEvent_TableSize * _eventIndex + 4, value);
    if (_entry.action == 2) {
      value = (_entry.buttonNumber << 4) | (_entry.clickType);
      setDeviceConfig(CfgClassSensorEvent, CfgFSensorEvent_TableSize * _eventIndex + 5, value);
    }
  }

  bool Device::hasExtendendSceneTable() {
    if ((((getFunctionID() & 0xf000) >> 12) == GroupIDGray) &&
        (((getProductID() >> 10) & 0x3f) == DEVICE_TYPE_KL)) {
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
      if ((_scene % 2) == 1) {
        extValue &= 0xF0; // high bits for odd scene numbers
        extValue |= (extValue >> 4);
      } else {
        extValue &= 0x0F; // lower bits for even scene numbers
        extValue |= (extValue << 4);
      }
      value |= extValue;
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

    int areaStopScene = areaOnScene - SceneA11 + SceneStopA1;
    getDeviceSceneMode(areaStopScene, sceneSpec);
    sceneSpec.dontcare = !_addToArea;
    setDeviceSceneMode(areaStopScene, sceneSpec);
  }

  void Device::setButtonInputMode(const uint8_t _value) {
    bool wasSlave = is2WaySlave();

    m_ButtonInputMode = _value;
    m_AKMInputProperty = getAKMButtonInputString(_value);
    if (is2WaySlave() && !wasSlave) {
      removeFromPropertyTree();
    } else if (wasSlave && !is2WaySlave()) {
      publishToPropertyTree();
    }
  }

  std::string Device::getAKMInputProperty() const {
    return m_AKMInputProperty;
  }

  bool Device::is2WayMaster() const {
    bool ret = false;

    try {
      ret = (getFeatures().pairing &&
            ((m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT2) ||
             (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT4) ||
             (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT2) ||
             (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT4) ||
             (((m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY) ||
               (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_1WAY)) &&
              IsEvenDsuid(m_DSID)))); // even dSID
    } catch (std::runtime_error &err) {
      Logger::getInstance()->log(err.what());
    }

    return ret;
  }

  bool Device::is2WaySlave() const {
    bool ret = false;

    if (!hasInput()) {
      return ret;
    }

    try {
      ret = ((m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT1) ||
             (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT3) ||
             (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT1) ||
             (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT3) ||
             ((m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_SDS_SLAVE_M1_M2) &&
             !IsEvenDsuid(m_DSID))); // odd dSID
    } catch (std::runtime_error &err) {
      Logger::getInstance()->log(err.what());
    }

    return ret;
  }

  bool Device::hasMultibuttons() const {
    int subclass = (m_FunctionID & 0xfc0) >> 6;
    int funcmodule = (m_FunctionID & 0x3f);

    if ((subclass == 0x04) && ((funcmodule & 0x3) > 1)) {
      return true;
    }

    return false;
  }

  bool Device::hasInput() const {
    int subclass = (m_FunctionID & 0xfc0) >> 6;
    int funcmodule = (m_FunctionID & 0x3f);

    if ((subclass == 0x04) && ((funcmodule & 0x3) > 0)) {
      return true;
    }

    return false;
  }

  bool Device::hasOutput() const {
    int funcmodule = (m_FunctionID & 0x3f);
    return (((funcmodule & 0x10) == 0x10) || (m_FunctionID == 0x6001));
  }

  DeviceTypes_t Device::getDeviceType() const {
    if (m_ProductID == 0) {
      return DEVICE_TYPE_INVALID;
    }
    int t = (m_ProductID >> 10) & 0x3f;
    switch (t) {
      case 0:
        return DEVICE_TYPE_KM;
      case 1:
        return DEVICE_TYPE_TKM;
      case 2:
        return DEVICE_TYPE_SDM;
      case 3:
        return DEVICE_TYPE_KL;
      case 4:
        return DEVICE_TYPE_TUP;
      case 5:
        return DEVICE_TYPE_ZWS;
      case 6:
        return DEVICE_TYPE_SDS;
      case 8:
        return DEVICE_TYPE_AKM;
      case 10:
        return DEVICE_TYPE_TNY;
      case 11:
        return DEVICE_TYPE_UMV;
      case 12:
        return DEVICE_TYPE_UMR;
      default:
        return DEVICE_TYPE_INVALID;
    }
  }

  const std::string Device::getDeviceTypeString(const DeviceTypes_t _type) {
    switch (_type) {
      case DEVICE_TYPE_KM:
        return "KM";
      case DEVICE_TYPE_TKM:
        return "TKM";
      case DEVICE_TYPE_SDM:
        return "SDM";
      case DEVICE_TYPE_KL:
        return "KL";
      case DEVICE_TYPE_TUP:
        return "TUP";
      case DEVICE_TYPE_ZWS:
        return "ZWS";
      case DEVICE_TYPE_SDS:
        return "SDS";
      case DEVICE_TYPE_AKM:
        return "AKM";
      case DEVICE_TYPE_TNY:
        return "TNY";
      case DEVICE_TYPE_UMV:
        return "UMV";
      case DEVICE_TYPE_UMR:
        return "UMR";
      default:
        return "";
    }
  }

  int Device::getDeviceNumber() const {
    return m_ProductID & 0x3ff;
  }

  DeviceClasses_t Device::getDeviceClass() const {
    if (m_FunctionID == 0) {
      return DEVICE_CLASS_INVALID;
    }

    int c = (m_FunctionID & 0xf000) >> 12;
    switch (c) {
      case 1:
        return DEVICE_CLASS_GE;
      case 2:
        return DEVICE_CLASS_GR;
      case 3:
        return DEVICE_CLASS_BL;
      case 4:
        return DEVICE_CLASS_TK;
      case 5:
        return DEVICE_CLASS_MG;
      case 6:
        return DEVICE_CLASS_RT;
      case 7:
        return DEVICE_CLASS_GN;
      case 8:
        return DEVICE_CLASS_SW;
      case 9:
        return DEVICE_CLASS_WE;
      default:
        return DEVICE_CLASS_INVALID;
    }
  }

  void Device::calculateHWInfo()
  {
    if (!m_HWInfo.empty()) {
      m_HWInfo.clear();
    }

    char* displayName = NULL;
    char* hwInfo = NULL;
    char* devGTIN = NULL;

    DsmApiGetDeviceDescription(m_VendorID, m_ProductID,
        m_FunctionID >> 12, m_RevisionID,
        &displayName, &hwInfo, &devGTIN);

    // HWInfo - Priorities: 1. OEM Data, 2. Device Product Data, 3. Device EEPROM Data (Vendor independent)
    if ((m_OemProductInfoState == DEVICE_OEM_VALID) && !m_OemProductName.empty()) {
      m_HWInfo = m_OemProductName;
    } else if (!m_VdcHardwareInfo.empty()) {
      m_HWInfo = m_VdcHardwareInfo;
    } else if (displayName != NULL) {
      m_HWInfo = displayName;
    } else {
      DeviceClasses_t deviceClass = getDeviceClass();
      m_HWInfo += getDeviceClassString(deviceClass);
      m_HWInfo += "-";

      DeviceTypes_t deviceType = getDeviceType();
      m_HWInfo += getDeviceTypeString(deviceType);

      int deviceNumber = getDeviceNumber();
      m_HWInfo += intToString(deviceNumber);
    }

    // if GTIN is known ...
    if (devGTIN) {
      m_GTIN = devGTIN;
    }
  }

  void Device::updateIconPath() {
    if (!m_iconPath.empty()) {
      m_iconPath.clear();
    }

    if ((m_OemProductInfoState == DEVICE_OEM_VALID) && !m_OemProductIcon.empty()) {
      m_iconPath = m_OemProductIcon;
    } else if (!m_VdcIconPath.empty()) {
      m_iconPath = m_VdcIconPath;
    } else {
      DeviceClasses_t deviceClass = getDeviceClass();
      DeviceTypes_t deviceType = getDeviceType();
      if (deviceClass == DEVICE_CLASS_INVALID) {
        m_iconPath = "unknown.png";
        return;
      }

      m_iconPath = getDeviceTypeString(deviceType);
      std::transform(m_iconPath.begin(), m_iconPath.end(), m_iconPath.begin(), ::tolower);
      if (m_iconPath.empty()) {
        m_iconPath = "star";
      }

      m_iconPath += "_" + getColorString(deviceClass);

      int jockerGroup = getJokerGroup();
      if (jockerGroup > 0) {
        m_iconPath += "_" + getColorString(jockerGroup);
      }

      m_iconPath += ".png";
    }
  }

  const std::string Device::getColorString(const int _class) {
    switch (_class) {
      case DEVICE_CLASS_GE:
        return "yellow";
      case DEVICE_CLASS_GR:
        return "grey";
      case DEVICE_CLASS_BL:
        return "blue";
      case DEVICE_CLASS_TK:
        return "cyan";
      case DEVICE_CLASS_MG:
        return "magenta";
      case DEVICE_CLASS_RT:
        return "red";
      case DEVICE_CLASS_GN:
        return "green";
      case DEVICE_CLASS_SW:
        return "black";
      case DEVICE_CLASS_WE:
        return "white";
      default:
        return "";
    }
  }

  const std::string Device::getDeviceClassString(const DeviceClasses_t _class) {
    switch (_class) {
      case DEVICE_CLASS_GE:
        return "GE";
      case DEVICE_CLASS_GR:
        return "GR";
      case DEVICE_CLASS_BL:
        return "BL";
      case DEVICE_CLASS_TK:
        return "TK";
      case DEVICE_CLASS_MG:
        return "MG";
      case DEVICE_CLASS_RT:
        return "RT";
      case DEVICE_CLASS_GN:
        return "GN";
      case DEVICE_CLASS_SW:
        return "SW";
      case DEVICE_CLASS_WE:
        return "WE";
      default:
        return "";
    }
  }

  const DeviceFeatures_t Device::getFeatures() const {
    DeviceFeatures_t features;
    features.pairing = false;
    features.syncButtonID = false;
    features.hasOutputAngle = false;

    if ((m_ProductID == 0) || (m_FunctionID == 0)) {
        return features;
    }

    DeviceTypes_t devType = this->getDeviceType();
    int devNumber = this->getDeviceNumber();
    DeviceClasses_t devCls = this->getDeviceClass();

    if ((devCls == DEVICE_CLASS_SW) && (devType == DEVICE_TYPE_TKM) &&
       ((devNumber == 200) || (devNumber == 210)) &&
       this->hasMultibuttons() &&
       ((m_ButtonInputIndex == 0) || (m_ButtonInputIndex == 2))) {
      features.pairing = true;
      features.syncButtonID = true;
    } else if ((devCls == DEVICE_CLASS_GE) && // GE-SDS20x, GE-SDS22x
               (devType == DEVICE_TYPE_SDS) &&
               ((devNumber / 10 == 20) || (devNumber / 10 == 22)) &&
               this->hasMultibuttons() &&
               (m_ButtonInputIndex == 0)) {
      features.pairing = true;
    }

    if ((devCls == DEVICE_CLASS_GR) && (devType == DEVICE_TYPE_KL) &&
        (devNumber == 220)) {
      features.hasOutputAngle = true;
    }

    return features;
  }

  int Device::getJokerGroup() const {
    bool joker = false;

    DeviceClasses_t devCls = this->getDeviceClass();
    if (devCls != DEVICE_CLASS_SW) {
      return -1;
    }

    for (int g = 1; g <= (int)DEVICE_CLASS_SW; g++) {
      if (m_GroupBitmask.test(g-1)) {
        if (g < (int)DEVICE_CLASS_SW) {
          return g;
        } else if (g == (int)DEVICE_CLASS_SW) {
          joker = true;
        }
      }
    }

    if (joker == true) {
      return (int)DEVICE_CLASS_SW;
    }

    return -1;
  }

  void Device::setOemInfo(const unsigned long long _eanNumber,
          const uint16_t _serialNumber, const uint8_t _partNumber,
          const DeviceOEMInetState_t _iNetState,
          bool _isIndependent)
  {
    m_OemEanNumber = _eanNumber;
    m_OemSerialNumber = _serialNumber;
    m_OemPartNumber = _partNumber;
    m_OemInetState = _iNetState;
    m_OemIsIndependent = _isIndependent;

    dirty();
  }

  void Device::setOemInfoState(const DeviceOEMState_t _state)
  {
    m_OemState = _state;

    if ((m_OemState == DEVICE_OEM_NONE) || (m_OemState == DEVICE_OEM_VALID)) {
      dirty();
    }
  }

  std::string Device::oemStateToString(const DeviceOEMState_t _state)
  {
    switch (_state) {
    case DEVICE_OEM_UNKOWN:
      return "Unknown";
    case DEVICE_OEM_NONE:
      return "None";
    case DEVICE_OEM_LOADING:
      return "Loading";
    case DEVICE_OEM_VALID:
      return "Valid";
    }
    return "";
  }

  DeviceOEMInetState_t Device::getOemInetStateFromString(const char* _string) const
  {
    if (strcmp(_string, "No EAN") == 0) {
      return DEVICE_OEM_EAN_NO_EAN_CONFIGURED;
    } else if (strcmp(_string, "No Internet") == 0) {
      return DEVICE_OEM_EAN_NO_INTERNET_ACCESS;
    } else if (strcmp(_string, "Internet optional") == 0) {
      return DEVICE_OEM_EAN_INTERNET_ACCESS_OPTIONAL;
    } else if (strcmp(_string, "Internet mandatory") == 0) {
      return DEVICE_OEM_EAN_INTERNET_ACCESS_MANDATORY;
    } else {
      return DEVICE_OEM_EAN_NO_EAN_CONFIGURED;
    }
  }

  std::string Device::oemInetStateToString(const DeviceOEMInetState_t _state)
  {
    switch (_state) {
    case DEVICE_OEM_EAN_NO_EAN_CONFIGURED:
      return "No EAN";
    case DEVICE_OEM_EAN_NO_INTERNET_ACCESS:
      return "No Internet";
    case DEVICE_OEM_EAN_INTERNET_ACCESS_OPTIONAL:
      return "Internet optional";
    case DEVICE_OEM_EAN_INTERNET_ACCESS_MANDATORY:
      return "Internet mandatory";
    }
    return "";
  }

  DeviceOEMState_t Device::getOemStateFromString(const char* _string) const
  {
    if (strcmp(_string, "None") == 0) {
      return DEVICE_OEM_NONE;
    } else if (strcmp(_string, "Valid") == 0) {
      return DEVICE_OEM_VALID;
    } else {
      return DEVICE_OEM_UNKOWN;
    }
  }

  void Device::setOemProductInfo(const std::string& _productName, const std::string& _iconPath, const std::string& _productURL)
  {
    m_OemProductName = _productName;
    m_OemProductIcon = _iconPath;
    m_OemProductURL = _productURL;
  }

  void Device::setOemProductInfoState(const DeviceOEMState_t _state)
  {
    m_OemProductInfoState = _state;

    calculateHWInfo();
    updateIconPath();

    if ((m_OemProductInfoState == DEVICE_OEM_NONE) || (m_OemProductInfoState == DEVICE_OEM_VALID)) {
      dirty();
    }
  }

  void Device::setDeviceBinaryInputType(uint8_t _inputIndex, uint8_t _inputType) {
    boost::mutex::scoped_lock lock(m_deviceMutex);
    if (_inputIndex > m_binaryInputs.size()) {
      throw ItemNotFoundException("Invalid binary input index");
    }
    lock.unlock();
    setDeviceConfig(CfgClassDevice, 0x40 + 3 * _inputIndex + 1, _inputType);
  }

  void Device::setDeviceBinaryInputTarget(uint8_t _inputIndex, uint8_t _targetType, uint8_t _targetGroup)
  {
    boost::mutex::scoped_lock lock(m_deviceMutex);
    if (_inputIndex > m_binaryInputs.size()) {
      throw ItemNotFoundException("Invalid binary input index");
    }
    uint8_t val = (_targetType & 0x3) << 6;
    val |= (_targetGroup & 0x3f);
    setDeviceConfig(CfgClassDevice, 0x40 + 3 * _inputIndex + 0, val);
  }

  uint8_t Device::getDeviceBinaryInputType(uint8_t _inputIndex) {
    boost::mutex::scoped_lock lock(m_deviceMutex);
    if (_inputIndex > m_binaryInputs.size()) {
      throw ItemNotFoundException("Invalid binary input index");
    }
    return m_binaryInputs[_inputIndex]->m_inputType;
  }

  void Device::setDeviceBinaryInputId(uint8_t _inputIndex, uint8_t _targetId) {
    boost::mutex::scoped_lock lock(m_deviceMutex);
    if (_inputIndex > m_binaryInputs.size()) {
      throw ItemNotFoundException("Invalid binary input index");
    }
    uint8_t val = (_targetId & 0xf) << 4;
    if (_inputIndex == m_binaryInputs.size()) {
      val |= 0x80;
    }
    setDeviceConfig(CfgClassDevice, 0x40 + 3 * _inputIndex + 2, val);
  }

  void Device::setDeviceAKMInputTimeouts(int _onDelay, int _offDelay) {
    if (_onDelay >= 0) {
      uint16_t onDelay = _onDelay / 100;
      setDeviceConfig(CfgClassFunction, CfgFunction_LTTimeoutOn, onDelay & 0xff);
      setDeviceConfig(CfgClassFunction, CfgFunction_LTTimeoutOn + 1, onDelay >> 8);
    }
    if (_offDelay >= 0) {
      uint16_t offDelay = _offDelay / 100;
      setDeviceConfig(CfgClassFunction, CfgFunction_LTTimeoutOff, offDelay & 0xff);
      setDeviceConfig(CfgClassFunction, CfgFunction_LTTimeoutOff + 1, offDelay >> 8);
    }
  }

  void Device::getDeviceAKMInputTimeouts(int& _onDelay, int& _offDelay) {
    _onDelay = getDeviceConfigWord(CfgClassFunction, CfgFunction_LTTimeoutOn) * 100;
    _offDelay = getDeviceConfigWord(CfgClassFunction, CfgFunction_LTTimeoutOff) * 100;
  }

  std::string Device::getAKMButtonInputString(const int _mode) {
    switch (_mode) {
      case DEV_PARAM_BUTTONINPUT_AKM_STANDARD:
        return BUTTONINPUT_AKM_STANDARD;
      case DEV_PARAM_BUTTONINPUT_AKM_INVERTED:
        return BUTTONINPUT_AKM_INVERTED;
      case DEV_PARAM_BUTTONINPUT_AKM_ON_RISING_EDGE:
        return BUTTONINPUT_AKM_ON_RISING_EDGE;
      case DEV_PARAM_BUTTONINPUT_AKM_ON_FALLING_EDGE:
        return BUTTONINPUT_AKM_ON_FALLING_EDGE;
      case DEV_PARAM_BUTTONINPUT_AKM_OFF_RISING_EDGE:
        return BUTTONINPUT_AKM_OFF_RISING_EDGE;
      case DEV_PARAM_BUTTONINPUT_AKM_OFF_FALLING_EDGE:
        return BUTTONINPUT_AKM_OFF_FALLING_EDGE;
      case DEV_PARAM_BUTTONINPUT_AKM_RISING_EDGE:
        return BUTTONINPUT_AKM_RISING_EDGE;
      case DEV_PARAM_BUTTONINPUT_AKM_FALLING_EDGE:
        return BUTTONINPUT_AKM_FALLING_EDGE;
      default:
        break;
    }
    return "";
  }

  const uint8_t Device::getBinaryInputCount() const {
    return (uint8_t) m_binaryInputCount;
  }

  void Device::setBinaryInputTarget(uint8_t _index, uint8_t targetGroupType, uint8_t targetGroup) {
    boost::mutex::scoped_lock lock(m_deviceMutex);
    if (_index >= m_binaryInputs.size()) {
      throw ItemNotFoundException("Invalid binary input index");
    }
    m_binaryInputs[_index]->m_targetGroupId = targetGroup;
    m_binaryInputs[_index]->m_targetGroupType = targetGroupType;
  }

  void Device::setBinaryInputId(uint8_t _index, uint8_t _inputId) {
    boost::mutex::scoped_lock lock(m_deviceMutex);
    if (_index >= m_binaryInputs.size()) {
      throw ItemNotFoundException("Invalid binary input index");
    }
    m_binaryInputs[_index]->m_inputId = _inputId;
  }

  void Device::setBinaryInputType(uint8_t _index, uint8_t _inputType) {
    boost::mutex::scoped_lock lock(m_deviceMutex);
    if (_index >= m_binaryInputs.size()) {
      throw ItemNotFoundException("Invalid binary input index");
    }
    m_binaryInputs[_index]->m_inputType = _inputType;
  }

  void Device::clearBinaryInputStates() {
    boost::mutex::scoped_lock lock(m_deviceMutex);
    m_binaryInputStates.clear();
  }

  void Device::setBinaryInputs(boost::shared_ptr<Device> me, const std::vector<DeviceBinaryInputSpec_t>& _binaryInputs) {
    boost::mutex::scoped_lock lock(m_deviceMutex);
    m_binaryInputCount = 0;
    m_binaryInputs.clear();
    m_binaryInputStates.clear();

    for (std::vector<DeviceBinaryInputSpec_t>::const_iterator it = _binaryInputs.begin();
        it != _binaryInputs.end();
        ++it) {
      boost::shared_ptr<DeviceBinaryInput_t> binput(new DeviceBinaryInput_t());
      binput->m_inputIndex = m_binaryInputCount;
      binput->m_inputId = it->InputID;
      binput->m_inputType = it->InputType;
      binput->m_targetGroupType = it->TargetGroupType;
      binput->m_targetGroupId = it->TargetGroup;
      m_binaryInputs.push_back(binput);

      boost::shared_ptr<State> state(new State(me, m_binaryInputCount));
      try {
        getApartment().allocateState(state);
      } catch (ItemDuplicateException& ex) {
        state = getApartment().getNonScriptState(state->getName());
      }
      m_binaryInputStates.push_back(state);

      if (m_pPropertyNode != NULL) {
        PropertyNodePtr binaryInputNode = m_pPropertyNode->getPropertyByName("binaryInputs");
        std::string bpath = std::string("binaryInput") + intToString(m_binaryInputCount);
        PropertyNodePtr entry = binaryInputNode->getPropertyByName(bpath);
        if (entry != NULL) {
          entry->getParentNode()->removeChild(entry);
        }
        entry = binaryInputNode->createProperty(bpath);
        entry->createProperty("targetGroupType")
                ->linkToProxy(PropertyProxyReference<int>(m_binaryInputs[m_binaryInputCount]->m_targetGroupType));
        entry->createProperty("targetGroupId")
                ->linkToProxy(PropertyProxyReference<int>(m_binaryInputs[m_binaryInputCount]->m_targetGroupId));
        entry->createProperty("inputType")
                ->linkToProxy(PropertyProxyReference<int>(m_binaryInputs[m_binaryInputCount]->m_inputType));
        entry->createProperty("inputId")
                ->linkToProxy(PropertyProxyReference<int>(m_binaryInputs[m_binaryInputCount]->m_inputId));
        entry->createProperty("inputIndex")
                ->linkToProxy(PropertyProxyReference<int>(m_binaryInputs[m_binaryInputCount]->m_inputIndex));
        PropertyNodePtr stateNode = m_binaryInputStates[m_binaryInputCount]
                ->getPropertyNode();
        PropertyNodePtr stateValueNode = stateNode->getProperty("value");
        if (stateValueNode != NULL) {
          PropertyNodePtr stateValueAlias = entry->createProperty("stateValue");
          stateValueAlias->alias(stateValueNode);
        }
      }

      m_binaryInputCount ++;
    }
  }

  const std::vector<boost::shared_ptr<DeviceBinaryInput_t> >& Device::getBinaryInputs() const {
    return m_binaryInputs;
  }

  const boost::shared_ptr<DeviceBinaryInput_t> Device::getBinaryInput(uint8_t _inputIndex) const {
    if (_inputIndex >= getBinaryInputCount()) {
      throw ItemNotFoundException(std::string("Device::getBinaryInput: index out of bounds"));
    }
    return m_binaryInputs[_inputIndex];
  }

  const uint8_t Device::getSensorCount() const {
    return (uint8_t) m_sensorInputCount;
  }

  void Device::setSensors(boost::shared_ptr<Device> me, const std::vector<DeviceSensorSpec_t>& _sensorInputs) {
    boost::mutex::scoped_lock lock(m_deviceMutex);
    m_sensorInputCount = 0;
    m_sensorInputs.clear();

    // fill in "standard" sensors if sensor table has not been read from device or is empty
    std::vector<DeviceSensorSpec_t> _slist = _sensorInputs;
    if (_slist.empty()) {
      fillSensorTable(_slist);
    }

    for (std::vector<DeviceSensorSpec_t>::const_iterator it = _slist.begin();
        it != _slist.end();
        ++it) {

      boost::shared_ptr<DeviceSensor_t> binput(new DeviceSensor_t());
      binput->m_sensorIndex = m_sensorInputCount;
      binput->m_sensorType = it->SensorType;
      binput->m_sensorPollInterval = it->SensorPollInterval;
      binput->m_sensorBroadcastFlag = it->SensorBroadcastFlag;
      binput->m_sensorPushConversionFlag = it->SensorConversionFlag;
      binput->m_sensorValue = 0;
      binput->m_sensorValueFloat = 0;
      binput->m_sensorValueTS = DateTime::NullDate;
      binput->m_sensorValueValidity = true;
      m_sensorInputs.push_back(binput);

      if (m_pPropertyNode != NULL) {
        PropertyNodePtr sensorInputNode = m_pPropertyNode->getPropertyByName("sensorInputs");
        std::string bpath = std::string("sensorInput") + intToString(m_sensorInputCount);
        PropertyNodePtr entry = sensorInputNode->getPropertyByName(bpath);
        if (entry != NULL) {
          entry->getParentNode()->removeChild(entry);
        }
        entry = sensorInputNode->createProperty(bpath);
        entry->createProperty("type")
                ->linkToProxy(PropertyProxyReference<int>(m_sensorInputs[m_sensorInputCount]->m_sensorType));
        entry->createProperty("index")
                ->linkToProxy(PropertyProxyReference<int>(m_sensorInputs[m_sensorInputCount]->m_sensorIndex));
        entry->createProperty("value")
                ->linkToProxy(PropertyProxyReference<double>(m_sensorInputs[m_sensorInputCount]->m_sensorValueFloat));
        entry->createProperty("valueDS")
                ->linkToProxy(PropertyProxyReference<int, unsigned int>(m_sensorInputs[m_sensorInputCount]->m_sensorValue));
        entry->createProperty("timestamp")
                ->linkToProxy(PropertyProxyMemberFunction<DateTime, std::string, false>(m_sensorInputs[m_sensorInputCount]->m_sensorValueTS, &DateTime::toString));
        entry->createProperty("pollinterval")
                ->linkToProxy(PropertyProxyReference<uint32_t>(m_sensorInputs[m_sensorInputCount]->m_sensorPollInterval));
        entry->createProperty("conversion")
                ->linkToProxy(PropertyProxyReference<bool>(m_sensorInputs[m_sensorInputCount]->m_sensorPushConversionFlag));
        entry->createProperty("broadcast")
                ->linkToProxy(PropertyProxyReference<bool>(m_sensorInputs[m_sensorInputCount]->m_sensorBroadcastFlag));
        entry->createProperty("valid")
                ->linkToProxy(PropertyProxyReference<bool>(m_sensorInputs[m_sensorInputCount]->m_sensorValueValidity));

      }

      m_sensorInputCount ++;
    }
  }

  void Device::setOutputChannels(boost::shared_ptr<Device> me, const std::vector<int>& _outputChannels) {
    boost::mutex::scoped_lock lock(m_deviceMutex);
    m_outputChannelCount = 0;
    m_outputChannels.clear();

    // The first entry of the table describes the default channel. Do not add to channel list.
    if (_outputChannels.size() >= 1) {
      PropertyNodePtr outputChannelNode = m_pPropertyNode->getPropertyByName("outputChannels");
      std::string bpath = std::string("outputChannelDefault");
      PropertyNodePtr entry = outputChannelNode->getPropertyByName(bpath);
      if (entry != NULL) {
        entry->getParentNode()->removeChild(entry);
      }
      entry = outputChannelNode->createProperty(bpath);
      entry->createProperty("channelID")->setIntegerValue(_outputChannels[0]);
    }

    // Only continue if there are 2 or more entries in the table
    if (_outputChannels.size() < 2) {
      return;
    }

    for (std::vector<int>::const_iterator it = ++_outputChannels.begin();
        it != _outputChannels.end();
        ++it) {
      m_outputChannels.push_back(*it);

      if (m_pPropertyNode != NULL) {
        PropertyNodePtr outputChannelNode = m_pPropertyNode->getPropertyByName("outputChannels");
        std::string bpath = std::string("outputChannel") + intToString(m_outputChannelCount);
        PropertyNodePtr entry = outputChannelNode->getPropertyByName(bpath);
        if (entry != NULL) {
          entry->getParentNode()->removeChild(entry);
        }
        entry = outputChannelNode->createProperty(bpath);
        entry->createProperty("channelID")
                ->linkToProxy(PropertyProxyReference<int>(m_outputChannels[m_outputChannelCount]));
      }

      m_outputChannelCount ++;
    }
  }

  const int Device::getOutputChannelIndex(int _channelId) const {
    int index = 0;
    for (std::vector<int>::const_iterator it = m_outputChannels.begin();
            it != m_outputChannels.end();
            ++it) {
      if (*it == _channelId) {
        return index;
      }
      index++;
    }
    return -1;
  }

  const int Device::getOutputChannel(int _index) const {
    if (_index > m_outputChannelCount) {
      return -1;
    }
    return m_outputChannels[_index];
  }

  const int Device::getOutputChannelCount() const {
    return m_outputChannelCount;
  }

  const std::vector<boost::shared_ptr<DeviceSensor_t> >& Device::getSensors() const {
    return m_sensorInputs;
  }

  const boost::shared_ptr<DeviceSensor_t> Device::getSensor(uint8_t _sensorIndex) const {
    if (_sensorIndex >= getSensorCount()) {
      throw ItemNotFoundException(std::string("Device::getSensor: index out of bounds"));
    }
    return m_sensorInputs[_sensorIndex];
  }

  const boost::shared_ptr<DeviceSensor_t> Device::getSensorByType(uint8_t _sensorType) const {
    for (size_t i = 0; i < getSensorCount(); i++) {
      boost::shared_ptr<DeviceSensor_t> sensor = m_sensorInputs.at(i);
      if (sensor->m_sensorType == _sensorType) {
        return sensor;
      }
    }
      
    throw ItemNotFoundException(std::string("Device::getSensor: no sensor with given type found"));
  }

  const void Device::setSensorValue(int _sensorIndex, unsigned int _sensorValue) const {
    if (_sensorIndex >= getSensorCount()) {
      throw ItemNotFoundException(std::string("Device::setSensorValue: index out of bounds"));
    }
    DateTime now;
    m_sensorInputs[_sensorIndex]->m_sensorValue = _sensorValue;
    m_sensorInputs[_sensorIndex]->m_sensorValueFloat =
        SceneHelper::sensorToFloat12(m_sensorInputs[_sensorIndex]->m_sensorType, _sensorValue);
    m_sensorInputs[_sensorIndex]->m_sensorValueTS = now;
    m_sensorInputs[_sensorIndex]->m_sensorValueValidity = true;
  }

  const void Device::setSensorValue(int _sensorIndex, double _sensorValue) const {
    if (_sensorIndex >= getSensorCount()) {
      throw ItemNotFoundException(std::string("Device::setSensorValue: index out of bounds"));
    }
    DateTime now;
    m_sensorInputs[_sensorIndex]->m_sensorValueFloat = _sensorValue;
    m_sensorInputs[_sensorIndex]->m_sensorValue =
            SceneHelper::sensorToSystem(m_sensorInputs[_sensorIndex]->m_sensorType, _sensorValue);
    m_sensorInputs[_sensorIndex]->m_sensorValueTS = now;
    m_sensorInputs[_sensorIndex]->m_sensorValueValidity = true;
  }

  const void Device::setSensorDataValidity(int _sensorIndex, bool _valid) const {
    if (_sensorIndex >= getSensorCount()) {
      throw ItemNotFoundException(std::string("Device::setSensorValue: index out of bounds"));
    }
    m_sensorInputs[_sensorIndex]->m_sensorValueValidity = _valid;
  }

  bool Device::isSensorDataValid(int _sensorIndex) const {
    if (_sensorIndex >= getSensorCount()) {
      throw ItemNotFoundException(std::string("Device::setSensorValue: index out of bounds"));
    }

    if (!this->isPresent() || !this->isValid()) {
      return true;
    }

    if (m_sensorInputs[_sensorIndex]->m_sensorPollInterval == 0) {
      return true;
    }

    return m_sensorInputs[_sensorIndex]->m_sensorValueValidity;
  };

  bool Device::isOemCoupledWith(boost::shared_ptr<Device> _otherDev)
  {
    dsuid_t tmp_dev = _otherDev->getDSID();
    dsuid_t tmp_meter = _otherDev->getDSMeterDSID();

    return ((m_OemState == DEVICE_OEM_VALID) &&
            !m_OemIsIndependent &&
            (m_OemSerialNumber > 0) &&
            isPresent() &&
            _otherDev->isPresent() &&
            !_otherDev->getOemIsIndependent() &&
            (_otherDev->getOemInfoState() == DEVICE_OEM_VALID) &&
            (!IsEqualDsuid(tmp_dev, m_DSID)) &&
            (!IsEqualDsuid(tmp_meter, m_DSMeterDSID)) &&
            (_otherDev->getOemEan() == m_OemEanNumber) &&
            (_otherDev->getOemSerialNumber() == m_OemSerialNumber));
  }

  boost::shared_ptr<State> Device::getBinaryInputState(uint8_t _inputIndex) const {
    boost::mutex::scoped_lock lock(m_deviceMutex);
    if (_inputIndex >= m_binaryInputStates.size()) {
      throw ItemNotFoundException(std::string("Device::getBinaryInputState: index out of bounds"));
    }
    return m_binaryInputStates[_inputIndex];
  }

  void Device::setConfigLock(bool _lockConfig) {
    boost::mutex::scoped_lock lock(m_deviceMutex);
    m_IsConfigLocked = _lockConfig;
  }

  DeviceBank3_BL::DeviceBank3_BL(boost::shared_ptr<Device> device)
    : m_device(device) {
      if (m_device->getDeviceClass() != DEVICE_CLASS_BL) {
        throw std::runtime_error("Device is not heating device");
      }
  }

  void DeviceBank3_BL::setValveProtectionTimer(uint16_t valveProtectionTimer) {
    m_device->setDeviceConfig16(CfgClassFunction, CfgFunction_Valve_ProtectionTimer,
                                valveProtectionTimer);
  }
  uint16_t DeviceBank3_BL::getValveProtectionTimer() {
    return m_device->getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_ProtectionTimer);
  }
  void DeviceBank3_BL::setEmergencySetPoint(int8_t emergency_sp) {
    // implicit conversion int8_t to uint8_t
    m_device->setDeviceConfig(CfgClassFunction, CfgFunction_Valve_EmergencyValue,
                              emergency_sp);
  }
  int8_t DeviceBank3_BL::getEmergencySetPoint() {
    // implicit conversion uint8_t to int8_t
    return m_device->getDeviceConfig(CfgClassFunction, CfgFunction_Valve_EmergencyValue);
  }
  void DeviceBank3_BL::setEmergencyTimer(int16_t emergency_tmr) {
    m_device->setDeviceConfig16(CfgClassFunction, CfgFunction_Valve_EmergencyTimer,
                                emergency_tmr);
  }
  uint16_t DeviceBank3_BL::getEmergencyTimer() {
    return m_device->getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_EmergencyTimer);
  }

  void DeviceBank3_BL::setPwmPeriod(uint16_t pwmPeriod) {
    m_device->setDeviceConfig16(CfgClassFunction, CfgFunction_Valve_PwmPeriod,
                                pwmPeriod);
  }
  uint16_t DeviceBank3_BL::getPwmPeriod() {
    return m_device->getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_PwmPeriod);
  }
  void DeviceBank3_BL::setPwmMinX(int8_t set_point) {
    m_device->setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMinValue,
                              set_point);
  }
  int8_t DeviceBank3_BL::getPwmMinX() {
    return m_device->getDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMinValue);
  }
  void DeviceBank3_BL::setPwmMaxX(int8_t set_point) {
    m_device->setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMaxValue,
                              set_point);
  }
  int8_t DeviceBank3_BL::getPwmMaxX() {
    return m_device->getDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMaxValue);
  }
  void DeviceBank3_BL::setPwmMinY(int8_t set_point) {
    m_device->setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMinY,
                              set_point);
  }
  int8_t DeviceBank3_BL::getPwmMinY() {
    return m_device->getDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMinY);
  }
  void DeviceBank3_BL::setPwmMaxY(int8_t set_point) {
    m_device->setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMaxY,
                              set_point);
  }
  int8_t DeviceBank3_BL::getPwmMaxY() {
    return m_device->getDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMaxY);
  }
  void DeviceBank3_BL::setPwmConfig(uint8_t config) {
    m_device->setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmConfig,
                              config);
  }
  uint8_t DeviceBank3_BL::getPwmConfig() {
    return m_device->getDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmConfig);
  }
  void DeviceBank3_BL::setPwmOffset(int8_t config) {
    m_device->setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmOffset,
                              config);
  }
  int8_t DeviceBank3_BL::getPwmOffset() {
    return m_device->getDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmOffset);
  }
} // namespace dss
