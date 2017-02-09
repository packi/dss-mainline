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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "device.h"

#include <unistd.h>
#include <math.h>
#include "foreach.h"

#include <boost/shared_ptr.hpp>

#include <digitalSTROM/dsuid.h>
#include <digitalSTROM/dsm-api-v2/dsm-api.h>
#include <ds/str.h>

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
#include "src/model/group.h"
#include "src/model/cluster.h"
#include "src/event.h"
#include "src/event/event_create.h"
#include "src/messages/vdc-messages.pb.h"
#include "src/vdc-element-reader.h"
#include "src/vdc-connection.h"
#include "src/protobufjson.h"
#include "status-field.h"
#include "status.h"
#include "src/model-features.h"

#define UMR_DELAY_STEPS  33.333333 // value specced by Christian Theiss
namespace dss {

  //================================================== DeviceBinaryInput

  __DEFINE_LOG_CHANNEL__(DeviceBinaryInput, lsNotice);

  // RAII class to handling stream to StatusField
  // We capture only weak reference to StatusField to avaid crashes
  // if this object is not notified soon enough that StatusField was deleted.
  class DeviceBinaryInput::StatusFieldHandle: boost::noncopyable {
  public:
    StatusFieldHandle(DeviceBinaryInput& parent, StatusField& statusField)
        : m_parent(parent),
        m_weakPtr(boost::shared_ptr<StatusField>(statusField.getStatus().getGroup().sharedFromThis(), &statusField)) {
      statusField.addSubState(*m_parent.m_state);
    }

    ~StatusFieldHandle() {
      if (auto&& ptr = m_weakPtr.lock()) {
        ptr->removeSubState(*m_parent.m_state);
      }
    }

    StatusField* getPtr() {
      if (auto&& ptr = m_weakPtr.lock()) {
        return ptr.get();
      }
      return DS_NULLPTR;
    }

    void update() {
      log(std::string("StatusFieldHandle::update this:") + m_parent.m_name, lsDebug);
      if (auto&& ptr = m_weakPtr.lock()) {
        ptr->updateSubState(*m_parent.m_state);
      } else {
        // We should have been deleted at this point
        log(ds::str("Referenced StatusField instance was deleted"), lsError);
      }
    }
  private:
    DeviceBinaryInput& m_parent;
    boost::weak_ptr<StatusField> m_weakPtr;
  };

  DeviceBinaryInput::DeviceBinaryInput(Device& device, const DeviceBinaryInputSpec_t& spec, int index)
      : m_inputIndex(index)
      , m_inputType(spec.InputType)
      , m_inputId(spec.InputID)
      , m_targetGroupId(spec.TargetGroup)
      , m_device(device)
      , m_name(m_device.getName() + "/" + intToString(index)) {
    log(std::string("DeviceBinaryInput this:") + m_name
        + " inputType:" + intToString(static_cast<int>(m_inputType))
        + " inputId:" + intToString(static_cast<int>(m_inputId))
        + " targetGroupId:" + intToString(m_targetGroupId), lsInfo);
    m_state = boost::make_shared<State>(device.sharedFromThis(), index);

    // assignCustomBinaryInputValues
    if (m_inputType == BinaryInputType::WindowTilt) {
      State::ValueRange_t windowTiltValues;
      windowTiltValues.push_back("invalid");
      windowTiltValues.push_back("closed");
      windowTiltValues.push_back("open");
      windowTiltValues.push_back("tilted");
      windowTiltValues.push_back("unknown");
      m_state->setValueRange(windowTiltValues);
    }

    try {
      device.getApartment().allocateState(m_state);
    } catch (ItemDuplicateException& ex) {
      m_state = device.getApartment().getNonScriptState(m_state->getName());
    }

    updateStatusFieldHandle();
  }

  DeviceBinaryInput::~DeviceBinaryInput() {
    log(std::string("~DeviceBinaryInput this:") + m_name, lsInfo);
    try {
      m_device.getApartment().removeState(m_state);
    } catch (const std::exception& e) {
      log(std::string("~DeviceBinaryInput: remove state failed:") + e.what(), lsWarning);
    }
  }

  void DeviceBinaryInput::setTargetId(uint8_t targetGroupId) {
    if (m_targetGroupId == targetGroupId) {
      return;
    }
    log(std::string("setTarget this:") + m_name
        + " targetGroupId:" + intToString(targetGroupId), lsInfo);
    m_targetGroupId = targetGroupId;
    updateStatusFieldHandle();
  }

  void DeviceBinaryInput::setInputId(BinaryInputId inputId) {
    if (m_inputId == inputId) {
      return;
    }
    log(std::string("setInputId this:") + m_name
        + " inputId:" + intToString(static_cast<int>(inputId)), lsInfo);
    m_inputId = inputId;
  }

  void DeviceBinaryInput::setInputType(BinaryInputType inputType) {
    if (m_inputType == inputType) {
      return;
    }
    log(std::string("setInputType this:") + m_name
        + " inputType:" + intToString(static_cast<int>(inputType)), lsInfo);
    m_inputType = inputType;
  }

  void DeviceBinaryInput::handleEvent(BinaryInputState inputState) {
    log(std::string("handleEvent this:") + m_name
        + " m_inputType:" + intToString(static_cast<int>(m_inputType))
        + " inputState:" + intToString(static_cast<int>(inputState)), lsInfo);
    try {
      if (m_inputType == BinaryInputType::WindowTilt) {
        m_state->setState(coSystem,
            [&]() {
              switch (static_cast<BinaryInputWindowHandleState>(inputState)) {
                case BinaryInputWindowHandleState::Closed: return StateWH_Closed;
                case BinaryInputWindowHandleState::Open: return StateWH_Open;
                case BinaryInputWindowHandleState::Tilted: return StateWH_Tilted;
                case BinaryInputWindowHandleState::Unknown: return StateWH_Unknown;
              };
              return StateWH_Unknown;
            }());
      } else {
        m_state->setState(coSystem,
            [&]() {
              switch (inputState) {
                case BinaryInputState::Inactive: return State_Inactive;
                case BinaryInputState::Active: return State_Active;
                case BinaryInputState::Unknown: return State_Unknown;
              };
              return State_Unknown;
            }());
      }
    } catch (const std::exception& e) {
      log(std::string("handleEvent: what:")+ e.what(), lsWarning);
    }
    if (m_statusFieldHandle) {
      m_statusFieldHandle->update();
    }
  }

  void DeviceBinaryInput::updateStatusFieldHandle() {
    // find status object within target group
    boost::shared_ptr<Group> targetGroup;
    StatusField* statusField = DS_NULLPTR;
    if (m_targetGroupId != 0) {
      if (auto type = statusFieldTypeForBinaryInputType(m_inputType)) {
        if ((targetGroup = m_device.tryGetGroup(m_targetGroupId).lock())) {
          if (auto&& status = targetGroup->getStatus()) {
            statusField = &status->getField(*type);
          } else {
            log(ds::str("updateStatusFieldHandle Group does not support status. this:", m_name,
                " m_inputType:", m_inputType, " m_targetGroupId:", m_targetGroupId), lsWarning);
            // some groups do not support status
          }
        }
      }
    }
    auto oldStatusField = m_statusFieldHandle ? m_statusFieldHandle->getPtr() : (StatusField*) DS_NULLPTR;
    if (oldStatusField == statusField) {
      return;
    }

    m_statusFieldHandle.reset();
    if (!statusField) {
      log(ds::str("updateStatusFieldHandle this:", m_name,
          " m_inputType:", m_inputType, " m_targetGroupId:", m_targetGroupId,
          " statusField:nullptr"), lsInfo);
      return;
    }
    log(ds::str("updateStatusFieldHandle this:", m_name,
        " m_inputType:", m_inputType, " m_targetGroupId:", m_targetGroupId,
        " type:",  statusField->getType(), " name:", statusField->getName()), lsInfo);
    m_statusFieldHandle.reset(new StatusFieldHandle(*this, *statusField));
  }

  //================================================== Device
  __DEFINE_LOG_CHANNEL__(Device, lsNotice);

  Device::Device(dsuid_t _dsid, Apartment* _pApartment)
  : AddressableModelItem(_pApartment),
    m_DSID(_dsid),
    m_ShortAddress(ShortAddressStaleDevice),
    m_LastKnownShortAddress(ShortAddressStaleDevice),
    m_ZoneID(0),
    m_LastKnownZoneID(0),
    m_DSMeterDSID(DSUID_NULL),
    m_LastKnownMeterDSID(DSUID_NULL),
    m_DSMeterDSIDstr(),
    m_LastKnownMeterDSIDstr(),
    m_ActiveGroup(0),
    m_DefaultGroup(0),
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
    m_OemState(DEVICE_OEM_UNKNOWN),
    m_OemIsIndependent(true),
    m_HWInfo(),
    m_iconPath("unknown.png"),
    m_OemProductInfoState(DEVICE_OEM_UNKNOWN),
    m_OemProductName(),
    m_OemProductIcon(),
    m_OemProductURL(),
    m_OemConfigLink(),
    m_isVdcDevice(false),
    m_hasActions(false),
    m_ValveType(DEVICE_VALVE_UNKNOWN),
    m_IsConfigLocked(false),
    m_sensorInputCount(0),
    m_outputChannelCount(0),
    m_AKMInputProperty(),
    m_cardinalDirection(cd_none),
    m_windProtectionClass(wpc_none),
    m_floor(0),
    m_pairedDevices(0),
    m_visible(true)
    {
      m_DSMeterDSUIDstr = dsuid2str(m_DSMeterDSID);
      m_LastKnownMeterDSUIDstr = dsuid2str(m_LastKnownMeterDSID);

      dsid_t dsid;
      if (dsuid_to_dsid(m_DSMeterDSID, &dsid)) {
        m_DSMeterDSIDstr = dsid2str(dsid);
      }
      if (dsuid_to_dsid(m_LastKnownMeterDSID, &dsid)) {
        m_LastKnownMeterDSIDstr = dsid2str(dsid);
      }

      m_vdcSpec = std::unique_ptr<VdsdSpec_t>(new VdsdSpec_t());
    } // ctor

  Device::~Device() {
    removeFromPropertyTree();
  }

  int Device::getGroupZoneID(int groupID) const {
    return (isAppUserGroup(groupID) || isGlobalAppGroup(groupID)) ? 0 : (m_ZoneID > 0 ? m_ZoneID : m_LastKnownZoneID);
  }

  boost::weak_ptr<Group> Device::tryGetGroup(int groupId) const {
    if (auto&& zone = m_pApartment->tryGetZone(getGroupZoneID(groupId)).lock()) {
      return zone->tryGetGroup(groupId);
    }
    return boost::weak_ptr<Group>();
  }

  void Device::removeFromPropertyTree() {
    if (m_pPropertyNode != NULL) {
      try {
        if ((m_DSMeterDSID != DSUID_NULL) && (m_DSID != DSUID_NULL)) {
          std::string devicePath = "devices/" + dsuid2str(m_DSID);
          PropertyNodePtr dev = m_pApartment->getDSMeterByDSID(m_DSMeterDSID)->getPropertyNode()->getProperty(devicePath);
          dev->alias(PropertyNodePtr());
          dev->getParentNode()->removeChild(dev);
        }
      } catch (std::runtime_error &err) {
        log(err.what(), lsError);
      }

      if (m_pApartment->getPropertyNode() != NULL) {
        foreach(auto&& g, m_groupIds) {
          std::string gPath = "zones/zone" + intToString(getGroupZoneID(g)) +
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

      m_pPropertyNode->unlinkProxy(true);
      PropertyNode *parent = m_pPropertyNode->getParentNode();
      if (parent != NULL) {
        parent->removeChild(m_pPropertyNode);
      }
      m_pPropertyNode.reset();
    }
    if (m_pAliasNode != NULL) {
      m_pAliasNode->alias(PropertyNodePtr());
      PropertyNode *parent = m_pAliasNode->getParentNode();
      if (parent != NULL) {
        parent->removeChild(m_pAliasNode);
      }
      m_pAliasNode.reset();
    }
  }

  void Device::publishVdcToPropertyTree() {
    if (m_isVdcDevice && (m_pPropertyNode != NULL)) {
      PropertyNodePtr propNode = m_pPropertyNode->createProperty("properties");

      propNode->createProperty("HardwareModelGuid")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcHardwareModelGuid, false));
      propNode->createProperty("ModelUID")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcModelUID, false));
      propNode->createProperty("ModelVersion")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcModelVersion, false));
      propNode->createProperty("VendorGuid")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcVendorGuid, false));
      propNode->createProperty("OemGuid")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcOemGuid, false));
      propNode->createProperty("OemModelGuid")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcOemModelGuid, false));
      propNode->createProperty("ConfigURL")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcConfigURL, false));
      propNode->createProperty("HardwareGuid")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcHardwareGuid, false));
      propNode->createProperty("HardwareInfo")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcHardwareInfo, false));
      propNode->createProperty("HardwareVersion")
        ->linkToProxy(PropertyProxyReference<std::string>(m_VdcHardwareVersion, false));
    }
  }

  void Device::publishToPropertyTree() {
    if (m_pPropertyNode != NULL) {
      // already published
      return;
    }

    if (m_pApartment->getPropertyNode() == NULL) {
      // never happens
      return;
    }

    if (!m_visible) {
      return;
    }

    m_pPropertyNode = m_pApartment->getPropertyNode()->createProperty("zones/zone0/devices/" + dsuid2str(m_DSID));

    dsid_t dsid;
    if (dsuid_to_dsid(m_DSID, &dsid)) {
      m_pPropertyNode->createProperty("dSID")->setStringValue(dsid2str(dsid));
    } else {
      m_pPropertyNode->createProperty("dSID")->setStringValue("");
    }
    m_pPropertyNode->createProperty("DisplayID")
          ->linkToProxy(PropertyProxyMemberFunction<Device, std::string, false>(*this, &Device::getDisplayID));

    m_pPropertyNode->createProperty("dSUID")->setStringValue(dsuid2str(m_DSID));
    m_pPropertyNode->createProperty("present")
      ->linkToProxy(PropertyProxyMemberFunction<Device, bool>(*this, &Device::isPresent));
    m_pPropertyNode->createProperty("name")
      ->linkToProxy(PropertyProxyMemberFunction<Device, std::string>(*this, &Device::getName, &Device::setName));
    if (!m_DSMeterDSIDstr.empty()) {
      m_pPropertyNode->createProperty("DSMeterDSID")
        ->linkToProxy(PropertyProxyReference<std::string>(m_DSMeterDSIDstr, false));
    } else {
      m_pPropertyNode->createProperty("DSMeterDSID")->setStringValue("");
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
    oemNode->createProperty("ConfigLink")
      ->linkToProxy(PropertyProxyReference<std::string>(m_OemConfigLink, false));
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
    m_pPropertyNode->createProperty("hasActions")
      ->linkToProxy(PropertyProxyReference<bool>(m_hasActions, false));
    publishVdcToPropertyTree();

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
      m_pPropertyNode->createProperty("lastKnownMeterDSID")->setStringValue("");
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

    m_pPropertyNode->createProperty("isValveType")
      ->linkToProxy(PropertyProxyMemberFunction<Device, bool>(*this, &Device::isValveDevice));

    m_pPropertyNode->createProperty("CardinalDirection")
      ->linkToProxy(PropertyProxyToString<CardinalDirection_t>(m_cardinalDirection));
    m_pPropertyNode->createProperty("WindProtectionClass")
      ->linkToProxy(PropertyProxyReference<int,
                    WindProtectionClass_t>(m_windProtectionClass));
    m_pPropertyNode->createProperty("Floor")
      ->linkToProxy(PropertyProxyReference<int>(m_floor));

    publishValveTypeToPropertyTree();

    m_TagsNode = m_pPropertyNode->createProperty("tags");
    m_TagsNode->setFlag(PropertyNode::Archive, true);

    if (m_ZoneID != 0) {
      std::string basePath = "zones/zone" + intToString(m_ZoneID) + "/devices";
      if (m_pAliasNode == NULL) {
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

    foreach (auto&& g, m_groupIds) {
      std::string gPath = "zones/zone" + intToString(getGroupZoneID(g)) +
                          "/groups/group" + intToString(g) + "/devices/" +
                          dsuid2str(m_DSID);
      PropertyNodePtr gnode = m_pApartment->getPropertyNode()->createProperty(gPath);
      if (gnode) {
        gnode->alias(m_pPropertyNode);
      }
      PropertyNodePtr gsubnode = m_pPropertyNode->createProperty("groups/group" + intToString(g));
      gsubnode->createProperty("id")->setIntegerValue(g);
    }

    if (m_DSMeterDSID != DSUID_NULL) {
      setDSMeter(m_pApartment->getDSMeterByDSID(m_DSMeterDSID));
    }

    republishModelFeaturesToPropertyTree();
  } // publishToPropertyTree

  bool Device::isOn() const {
    return (m_LastCalledScene != SceneOff) &&
           (m_LastCalledScene != SceneMin) &&
           (m_LastCalledScene != SceneDeepOff) &&
           (m_LastCalledScene != SceneStandBy);
  } // isOn

  void Device::setPartiallyFromSpec(const DeviceSpec_t& spec) {
    m_FunctionID = spec.FunctionID;
    m_ProductID = spec.ProductID;
    m_VendorID = spec.VendorID;
    m_RevisionID = spec.revisionId;
    m_IsLockedInDSM = spec.Locked;
    m_OutputMode = spec.OutputMode;
    m_ActiveGroup = spec.activeGroup;
    m_DefaultGroup = spec.defaultGroup;

    if ((m_FunctionID != 0) && (m_ProductID != 0) && (m_VendorID != 0)) {
      calculateHWInfo();
    }
    updateModelFeatures();
    updateIconPath();
    updateAKMNode();
    publishValveTypeToPropertyTree();
  }

  int Device::getFunctionID() const {
    return m_FunctionID;
  } // getFunctionID

  int Device::getProductID() const {
    return m_ProductID;
  } // getProductID

  void Device::updateAKMNode() {
    if (((getDeviceType() == DEVICE_TYPE_AKM) || (getDeviceType() == DEVICE_TYPE_UMR)) && (m_pPropertyNode != NULL)) {
      m_pPropertyNode->createProperty("AKMInputProperty")
          ->linkToProxy(PropertyProxyReference<std::string>(m_AKMInputProperty, false));
    }

  }

  int Device::getRevisionID() const {
    if (isVdcDevice()) {
      return 0;
    }
    return m_RevisionID;
  } // getRevisionID

  int Device::getVendorID() const {
    return m_VendorID;
  } // getVendrID

  void Device::fillSensorTable(std::vector<DeviceSensorSpec_t>& _slist) {
    DeviceSensorSpec_t sensorInputReserved1 = { SensorType::Reserved1, 0, 0, 0 };
    DeviceSensorSpec_t sensorInputReserved2 = { SensorType::Reserved2, 0, 0, 0 };
    DeviceSensorSpec_t sensorInput04 = { SensorType::ActivePower, 0, 0, 0 };
    DeviceSensorSpec_t sensorInput05 = { SensorType::OutputCurrent, 0, 0, 0 };
    DeviceSensorSpec_t sensorInput06 = { SensorType::ElectricMeter, 0, 0, 0 };
    DeviceSensorSpec_t sensorInput64 = { SensorType::OutputCurrent16A, 0, 0, 0 };
    DeviceSensorSpec_t sensorInput65 = { SensorType::ActivePowerVA, 0, 0, 0 };
    DeviceClasses_t deviceClass = getDeviceClass();
    int devType = (deviceClass << 16) | m_ProductID;

    if (isVdcDevice()) {
        // vdc devices have no implicit sensors, but 1st
        // generation vdc devices mocked themselves as Klemmen,
        // so they accidently got an implicit sensor attached
        // need to exit early here
        // http://redmine.digitalstrom.org/issues/8294
        return;
    }

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
        ((m_ButtonGroupMembership & 0xf) << 4) | (m_ButtonID & 0xf));
  } // setDeviceButtonId

  void Device::setDeviceButtonActiveGroup(uint8_t _buttonActiveGroup) {
    if (m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      /* re-configure area or device button mode for groups other then lights and shades */
      bool isAreaButton =
          ((m_ButtonID >= ButtonId_Area1) && (m_ButtonID <= ButtonId_Area4)) ||
          ((m_ButtonID >= ButtonId_Area1_Extended) && (m_ButtonID <= ButtonId_Area4_Extended));
      if (isAreaButton &&
          ((_buttonActiveGroup < GroupIDYellow) || (_buttonActiveGroup > GroupIDGray))) {
        setDeviceButtonID(ButtonId_Zone);
      }
      /* tell dsm to change button active group */
      m_pApartment->getDeviceBusInterface()->setDeviceButtonActiveGroup(*this, _buttonActiveGroup);
      /* refresh device information for correct active group */
      if ((m_pApartment != NULL) && (m_pApartment->getModelMaintenance() != NULL)) {
        ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceChanged, m_DSMeterDSID);
        pEvent->addParameter(m_ShortAddress);
        sleep(3); // #8900: make sure all settings were really saved
        m_pApartment->getModelMaintenance()->addModelEvent(pEvent);
      }
    }
  } // setDeviceActiveGroup

  void Device::setDeviceJokerGroup(uint8_t _groupId) {
    if (!isDefaultGroup(_groupId) && !isGlobalAppDsGroup(_groupId)) {
      throw std::runtime_error("Invalid joker group value");
    }
    // for standard groups force that only one group is active
    for (int g = GroupIDYellow; g <= GroupIDStandardMax; g++) {
      if (isInGroup(g)) {
        removeFromGroup(g);
      }
    }

    // remove from control groups
    for (int g = GroupIDControlGroupMin; g <= GroupIDControlGroupMax; g++) {
      if (isInGroup(g)) {
        removeFromGroup(g);
      }
    }

    // remove also from GA groups
    for (int g = GroupIDGlobalAppMin; g <= GroupIDGlobalAppMax; g++) {
      if (isInGroup(g)) {
        removeFromGroup(g);
      }
    }

    // assign device to new group
    addToGroup(_groupId);
    // set button target group
    if (getButtonInputCount() > 0) {
      setButtonGroupMembership(_groupId);
      setDeviceButtonActiveGroup(_groupId);
    }
    // set binary input to target group
    if (getBinaryInputCount() == 1) {
      setDeviceBinaryInputTargetId(0, _groupId);
      setBinaryInputTargetId(0, _groupId);
    }
    updateIconPath();
  } // setDeviceJokerGroup

  void Device::setDeviceOutputMode(uint8_t _modeId) {
    setDeviceConfig(CfgClassFunction, CfgFunction_Mode, _modeId);
  } // setDeviceOutputMode

  void Device::setDeviceButtonInputMode(uint8_t _modeId) {
    setDeviceConfig(CfgClassFunction, CfgFunction_LTMode, _modeId);
  } // setDeviceButtonInputMode

  void Device::setProgMode(uint8_t _modeId) {
    if (m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setDeviceProgMode(*this, _modeId);
    }
  } // setProgMode

 void Device::increaseDeviceOutputChannelValue(uint8_t _channel) {
    if (m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->increaseDeviceOutputChannelValue(*this, _channel);
    }
  } // increaseDeviceOutputChannelValue

  void Device::decreaseDeviceOutputChannelValue(uint8_t _channel) {
    if (m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->decreaseDeviceOutputChannelValue(*this, _channel);
    }
  } // decreaseDeviceOutputChannelValue

  void Device::stopDeviceOutputChannelValue(uint8_t _channel) {
    if (m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->stopDeviceOutputChannelValue(*this, _channel);
    }
  } // decreaseDeviceOutputChannelValue

  uint16_t Device::getDeviceOutputChannelValue(uint8_t _channel) {
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getDeviceOutputChannelValue(*this, _channel);
    }
    throw std::runtime_error("Bus interface not available");
  } // getDeviceOutputChannelValue

  void Device::setDeviceOutputChannelValue(uint8_t _channel, uint8_t _size,
                                   uint16_t _value, bool _applyNow) {
    if (m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setDeviceOutputChannelValue(*this, _channel, _size, _value, _applyNow);
    }
  } // setDeviceOutputChannelValue

  uint16_t Device::getDeviceOutputChannelSceneValue(uint8_t _channel,
                                                    uint8_t _scene) {
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getDeviceOutputChannelSceneValue(*this, _channel, _scene);
    }
    throw std::runtime_error("Bus interface not available");
  } // getDeviceOutputChannelSceneValue

  void Device::setDeviceOutputChannelSceneValue(uint8_t _channel,
                                                uint8_t _size,
                                                uint8_t _scene,
                                                uint16_t _value) {
    if (m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setDeviceOutputChannelSceneValue(*this, _channel, _size, _scene, _value);
    }
  }

  void Device::getDeviceOutputChannelSceneConfig(uint8_t _scene,
                                                 DeviceSceneSpec_t& _config) {
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      uint16_t mode = m_pApartment->getDeviceBusInterface()->getDeviceOutputChannelSceneConfig(*this, _scene);
      _config.dontcare = (mode & 1) > 0;
      _config.localprio = (mode & 2) > 0;
      _config.specialmode = (mode & 4) > 0;
      _config.ledconIndex = (mode >> 4) & 7;
      _config.dimtimeIndex = 0; // no dim time index
      _config.flashmode = (mode & (1 << 8)) > 0; // effect 1
      return;
    }
    throw std::runtime_error("Bus interface not available");
  } // getDeviceOutputChannelSceneConfig

  void Device::setDeviceOutputChannelSceneConfig(uint8_t _scene,
                                                 DeviceSceneSpec_t _config) {
    if (m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (m_pApartment->getDeviceBusInterface() != NULL) {
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
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setDeviceOutputChannelDontCareFlags(*this, _scene, _value);
    }
  }

  uint16_t Device::getDeviceOutputChannelDontCareFlags(uint8_t _scene) {
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getDeviceOutputChannelDontCareFlags(*this, _scene);
    }
    throw std::runtime_error("Bus interface not available");
  }

  int Device::getSceneAngle(const int _scene) {
    DeviceFeatures_t features = getDeviceFeatures();
    if (features.hasOutputAngle) {
      return getDeviceConfig(CfgClassSceneAngle, _scene);
    } else {
      throw std::runtime_error("Device does not support output angle setting");
    }
  }

  void Device::setSceneAngle(const int _scene, const int _angle) {
    DeviceFeatures_t features = getDeviceFeatures();
    if (!features.hasOutputAngle) {
      throw std::runtime_error("Device does not support output angle setting");
    }
    if ((_angle < 0) || (_angle > 255)) {
      throw std::runtime_error("Device angle value out of bounds");
    }

    setDeviceConfig(CfgClassSceneAngle, _scene, _angle);
  }

  void Device::setDeviceLedMode(uint8_t _ledconIndex, DeviceLedSpec_t _config) {
    if (_ledconIndex > 2) {
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
    if (_ledconIndex > 2) {
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
    if ((thigh - timems) > (timems - tlow)) {
      val --;
    }
    return val;
  }

  void Device::setDeviceTransitionTime(uint8_t _dimtimeIndex, int up, int down) {
    if (_dimtimeIndex > 2) {
      throw DSSException("Device::setDeviceTransitionTime: index out of range");
    }
    uint8_t vup = transitionTimeEval(up);
    uint8_t vdown  = transitionTimeEval(down);
    setDeviceConfig(CfgClassFunction, CfgFunction_DimTime0 + _dimtimeIndex*2, vup);
    setDeviceConfig(CfgClassFunction, CfgFunction_DimTime0 + _dimtimeIndex*2 + 1, vdown);
  } // setDeviceTransitionTime

  void Device::getDeviceTransitionTime(uint8_t _dimtimeIndex, int& up, int& down) {
    if (_dimtimeIndex > 2) {
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
    if (!isValveDevice()) {
      throw DSSException("Not a climate device");
    }
    setDeviceConfig(CfgClassFunction, CfgFunction_Valve_EmergencyValue, _config.emergencyControlValue);
    setDeviceConfig16(CfgClassFunction, CfgFunction_Valve_EmergencyTimer, _config.emergencyTimer);
    setDeviceConfig16(CfgClassFunction, CfgFunction_Valve_ProtectionTimer, _config.protectionTimer);
  } // setDeviceValveTimer

  void Device::getDeviceValveTimer(DeviceValveTimerSpec_t& _config) {
    if (!isValveDevice()) {
      throw DSSException("Not a climate device");
    }
    _config.emergencyControlValue = getDeviceConfig(CfgClassFunction, CfgFunction_Valve_EmergencyValue);
    _config.emergencyTimer = getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_EmergencyTimer);
    _config.protectionTimer = getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_ProtectionTimer);
  } // getDeviceValveTimer

  void Device::setDeviceValvePwm(DeviceValvePwmSpec_t _config) {
    if (!isValveDevice()) {
      throw DSSException("Not a climate device");
    }
    setDeviceConfig16(CfgClassFunction, CfgFunction_Valve_PwmPeriod, _config.pwmPeriod);
    setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMinX, _config.pwmMinX);
    setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMaxX, _config.pwmMaxX);
    setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMinY, _config.pwmMinY);
    setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmMaxY, _config.pwmMaxY);
    setDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmOffset, static_cast<uint8_t>(_config.pwmOffset));
  } // setDeviceValvePwm

  void Device::getDeviceValvePwm(DeviceValvePwmSpec_t& _config) {
    if (!isValveDevice()) {
      throw DSSException("Not a climate device");
    }
    _config.pwmPeriod = getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_PwmPeriod);
    uint16_t value = getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_PwmMinX);
    _config.pwmMinX = value & 0xff; // CfgFunction_Valve_PwmMinX
    _config.pwmMaxX = (value >> 8) & 0xff; // CfgFunction_Valve_PwmMaxX
    value = getDeviceConfigWord(CfgClassFunction, CfgFunction_Valve_PwmMinY);
    _config.pwmMinY = value & 0xff; // CfgFunction_Valve_PwmMinY
    _config.pwmMaxY = (value >> 8) & 0xff; // CfgFunction_Valve_PwmMaxY
    _config.pwmOffset = static_cast<int8_t>(getDeviceConfig(CfgClassFunction, CfgFunction_Valve_PwmOffset));
  } // getDeviceValvePwm

  void Device::getDeviceValvePwmState(DeviceValvePwmStateSpec_t& _config) {
    if (!isValveDevice()) {
      throw DSSException("Not a climate device");
    }
    uint16_t value = getDeviceConfigWord(CfgClassRuntime, CfgRuntime_Valve_PwmValue);
    _config.pwmValue = value & 0xff;
    _config.pwmPriorityMode = (value >> 8) & 0xff;
  } // getDeviceValvePwmState

  void Device::setDeviceValveControl(DeviceValveControlSpec_t _config) {
    if (!isValveDevice()) {
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
    if (!isValveDevice()) {
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
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getDeviceConfig(*this,
                                                                  _configClass,
                                                                  _configIndex);
    }
    throw std::runtime_error("Bus interface not available");
  } // getDeviceConfig

  uint16_t Device::getDeviceConfigWord(uint8_t _configClass, uint8_t _configIndex) {
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getDeviceConfigWord(*this,
                                                                  _configClass,
                                                                  _configIndex);
    }
    throw std::runtime_error("Bus interface not available");
  } // getDeviceConfigWord

  std::pair<uint8_t, uint16_t> Device::getDeviceTransmissionQuality() {
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      return m_pApartment->getDeviceBusInterface()->getTransmissionQuality(*this);
    }
    throw std::runtime_error("Bus interface not available");
  } // getDeviceTransmissionQuality

  void Device::setDeviceValue(uint8_t _value) {
    if (m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->setValue(*this, _value);
    }
  } // setDeviceValue (8)

  void Device::setDeviceOutputValue(uint8_t _offset, uint16_t _value) {
    if (m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (_offset & 1) {
      setDeviceConfig(CfgClassRuntime, _offset, _value & 0xff);
    } else {
      setDeviceConfig(CfgClassRuntime, _offset+1, (_value >> 8) & 0xff);
      setDeviceConfig(CfgClassRuntime, _offset, _value & 0xff);
    }
  } // setDeviceOutputValue (offset)

  uint16_t Device::getDeviceOutputValue(uint8_t _offset) {
    uint16_t result = getDeviceConfigWord(CfgClassRuntime, _offset);
    if (_offset == 0) result &= 0xff;   // fix offset 0 value which is 8-bit actually
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
    if (m_pPropertyNode) {
      m_pPropertyNode->checkWriteAccess();
    }
    if (m_Name != _name) {
      m_Name = _name;
      dirty();
    }
  } // setName

  void Device::dirty() {
    if ((m_pApartment != NULL) && (m_pApartment->getModelMaintenance() != NULL)) {
      m_pApartment->getModelMaintenance()->addModelEvent(
          new ModelEvent(ModelEvent::etModelDirty)
      );
    }
  } // dirty

  void Device::checkAutoCluster() {
    if ((m_pApartment != NULL) && (m_pApartment->getModelMaintenance() != NULL)) {
      ModelEventWithDSID* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceDirty, getDSID());
      m_pApartment->getModelMaintenance()->addModelEvent(pEvent);
    }
  }

  bool Device::operator==(const Device& _other) const {
    return (_other.m_DSID == m_DSID);
  } // operator==

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
    if ((m_pPropertyNode != NULL) && (m_DSMeterDSID != DSUID_NULL)) {
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
      m_DSMeterDSIDstr = "";
    }
    if (dsuid_to_dsid(m_LastKnownMeterDSID, &dsid)) {
      m_LastKnownMeterDSIDstr = dsid2str(dsid);
    } else {
      m_LastKnownMeterDSIDstr = "";
    }

    if (m_pPropertyNode != NULL) {
      PropertyNodePtr target = _dsMeter->getPropertyNode()->createProperty("devices");
      if (alias != NULL) {
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
    if (_value == m_ZoneID) {
      // nothing to do
      return;
    }

    if (_value != 0) {
      m_LastKnownZoneID = _value;
    }

    m_ZoneID = _value;
    if ((m_pPropertyNode != NULL) && (m_pApartment->getPropertyNode() != NULL)) {
      std::string basePath = "zones/zone" + intToString(m_ZoneID) + "/devices";
      if (m_pAliasNode == NULL) {
        PropertyNodePtr node = m_pApartment->getPropertyNode()->getProperty(basePath + "/" + dsuid2str(m_DSID));
        if (node != NULL) {
          log("Device::setZoneID: Target node for device " + dsuid2str(m_DSID) + " already exists", lsError);
          if (node->size() > 0) {
            log("Device::setZoneID: Target node for device " + dsuid2str(m_DSID) + " has children", lsFatal);
            return;
          }
        }
        m_pAliasNode = m_pApartment->getPropertyNode()->createProperty(basePath + "/" + dsuid2str(m_DSID));
        m_pAliasNode->alias(m_pPropertyNode);
      } else {
        PropertyNodePtr base = m_pApartment->getPropertyNode()->getProperty(basePath);
        if (base == NULL) {
          throw std::runtime_error("PropertyNode of the new zone does not exist");
        }
        base->addChild(m_pAliasNode);
      }
    }
    // Binary input status bit handles depend on zoneId by call to `tryGetGroup`.
    // TODO(someday): refactor to some kind of observer pattern?
    foreach (auto&& binaryInput, m_binaryInputs) {
      binaryInput->updateStatusFieldHandle();
    }
  } // setZoneID

  int Device::getLastKnownZoneID() const {
    return m_LastKnownZoneID;
  } // getLastKnownZoneID

  void Device::setLastKnownZoneID(const int _value) {
    m_LastKnownZoneID = _value;
  } // setLastKnownZoneID

  int Device::getGroupIdByIndex(const int index) const {
    return m_groupIds.at(index);
  } // getGroupIdByIndex

  boost::shared_ptr<Group> Device::getGroupByIndex(const int _index) {
    return m_pApartment->getGroup(getGroupIdByIndex(_index));
  } // getGroupByIndex

  void Device::addToGroup(const int _groupID) {
    if (isValidGroup(_groupID)) {
      if (find(m_groupIds.begin(), m_groupIds.end(), _groupID) == m_groupIds.end()) {
        m_groupIds.push_back(_groupID);
        updateIconPath();
        if ((m_pPropertyNode != NULL) && (m_pApartment->getPropertyNode() != NULL)) {
          // create alias in group list
          std::string gPath = "zones/zone" + intToString(getGroupZoneID(_groupID)) + "/groups/group" + intToString(_groupID) + "/devices/"  +  dsuid2str(m_DSID);
          PropertyNodePtr gnode = m_pApartment->getPropertyNode()->createProperty(gPath);
          if (gnode) {
            gnode->alias(m_pPropertyNode);
          }
          // create group-n property
          PropertyNodePtr gsubnode = m_pPropertyNode->createProperty("groups/group" + intToString(_groupID));
          gsubnode->createProperty("id")->setIntegerValue(_groupID);
        }
      } else {
        log("Device " + dsuid2str(m_DSID) + " (bus: " + intToString(m_ShortAddress) + ", zone: " + intToString(m_ZoneID) + ") is already in group " + intToString(_groupID), lsDebug);
      }
    } else {
      log("Device::addToGroup: Group ID out of bounds: " + intToString(_groupID), lsInfo);
    }
  } // addToGroup

  void Device::removeFromGroup(const int _groupID) {
    if (isValidGroup(_groupID)) {
      std::vector<int>::iterator it = find(m_groupIds.begin(), m_groupIds.end(), _groupID);
      if (it != m_groupIds.end()) {
        m_groupIds.erase(it);
        updateIconPath();
        if ((m_pPropertyNode != NULL) && (m_pApartment->getPropertyNode() != NULL)) {
          // remove alias in group list
          std::string gPath = "zones/zone" + intToString(getGroupZoneID(_groupID)) + "/groups/group" + intToString(_groupID) + "/devices/"  +  dsuid2str(m_DSID);
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
      log("Device::removeFromGroup: Group ID out of bounds: " + intToString(_groupID), lsInfo);
    }
  } // removeFromGroup

  void Device::resetGroups() {
    std::vector<int>::iterator it;
    while (m_groupIds.size() > 0) {
      int g = m_groupIds.front();
      removeFromGroup(g);
    }
  } // resetGroups

  int Device::getGroupsCount() const {
    return m_groupIds.size();
  } // getGroupsCount

  bool Device::isInGroup(const int _groupID) const {
    bool result = false;
    if (_groupID == 0) {
      result = true;
    } else if (!isValidGroup(_groupID)) {
      result = false;
    } else {
      auto it = find(m_groupIds.begin(), m_groupIds.end(), _groupID);
      result = (it != m_groupIds.end());
    }
    return result;
  } // isInGroup

  void Device::setIsLockedInDSM(const bool _value) {
    m_IsLockedInDSM = _value;
  } // setIsLockedInDSM

  void Device::lock() {
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->lockOrUnlockDevice(*this, true);
      m_IsLockedInDSM = true;
    }
  } // lock

  void Device::unlock() {
    if (m_pApartment->getDeviceBusInterface() != NULL) {
      m_pApartment->getDeviceBusInterface()->lockOrUnlockDevice(*this, false);
      m_IsLockedInDSM = false;
    }
  } // unlock

  bool Device::hasTag(const std::string& _tagName) const {
    if (m_TagsNode != NULL) {
      return m_TagsNode->getPropertyByName(_tagName) != NULL;
    }
    return false;
  } // hasTag

  void Device::addTag(const std::string& _tagName) {
    if (!hasTag(_tagName)) {
      if (m_TagsNode != NULL) {
        PropertyNodePtr pNode = m_TagsNode->createProperty(_tagName);
        pNode->setFlag(PropertyNode::Archive, true);
        dirty();
      }
    }
  } // addTag

  void Device::removeTag(const std::string& _tagName) {
    if (hasTag(_tagName)) {
      if (m_TagsNode != NULL) {
        m_TagsNode->removeChild(m_TagsNode->getPropertyByName(_tagName));
        dirty();
      }
    }
  } // removeTag

  std::vector<std::string> Device::getTags() {
    std::vector<std::string> result;
    if (m_TagsNode != NULL) {
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
    if (_eventIndex > 15) {
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
    if (_eventIndex > 15) {
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
    int areaOnScene = (_areaScene >= SceneA11) ? _areaScene : (_areaScene - SceneOffA1 + SceneA11);
    uint8_t outValue = 0;

    if (_addToArea) {
      outValue = 0xFF;
    }
    setDeviceValue(outValue);
    saveScene(coSystem, areaOnScene, "");
  }

  void Device::setButtonInputMode(const uint8_t _value) {
    bool wasSlave = is2WaySlave();

    m_ButtonInputMode = _value;
    m_AKMInputProperty = getAKMButtonInputString(_value);
    if (is2WaySlave() && !wasSlave) {
      removeFromPropertyTree();
    } else if (wasSlave && !is2WaySlave()) {
      publishToPropertyTree();
      updateAKMNode();
    }
  }

  void Device::setOutputMode(const uint8_t _value) {
    bool wasSlave = is2WaySlave();

    m_OutputMode = _value;
    if ((getDeviceType() == DEVICE_TYPE_UMR) && (multiDeviceIndex() == 3)) {
      if (is2WaySlave()) {
        removeFromPropertyTree();
      } else if (wasSlave) {
        publishToPropertyTree();
        updateAKMNode();
      }
    }
  }

  std::string Device::getAKMInputProperty() const {
    return m_AKMInputProperty;
  }

  bool Device::is2WayMaster() const {

    if (!getDeviceFeatures().pairing) {
      return false;
    }

    bool ret = false;
    try {
      if ((multiDeviceIndex() == 2) && (getDeviceType() == DEVICE_TYPE_UMR)) {
        ret = ((m_OutputMode == OUTPUT_MODE_TWO_STAGE_SWITCH) ||
               (m_OutputMode == OUTPUT_MODE_BIPOLAR_SWITCH)   ||
               (m_OutputMode == OUTPUT_MODE_THREE_STAGE_SWITCH)) &&
               IsEvenDsuid(m_DSID); // even dSID
      } else if (hasInput()) {
        ret = ((m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT2) ||
               (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT4) ||
               (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT2) ||
               (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT4) ||
               (((m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY) ||
                 (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_1WAY)) &&
                IsEvenDsuid(m_DSID))); // even dSID
      }
    } catch (std::runtime_error &err) {
      log(err.what(), lsError);
    }
    return ret;
  }

  bool Device::is2WaySlave() const {

    if (!(multiDeviceIndex() == 3) && (getDeviceType() == DEVICE_TYPE_UMR)) {
      if (!hasInput()) {
        return false;
      }
    }

    bool ret = false;
    try {
      if ((getDeviceType() == DEVICE_TYPE_UMR) && (multiDeviceIndex() == 3)) {
        ret = ((m_OutputMode == OUTPUT_MODE_TWO_STAGE_SWITCH) ||
               (m_OutputMode == OUTPUT_MODE_BIPOLAR_SWITCH)   ||
               (m_OutputMode == OUTPUT_MODE_THREE_STAGE_SWITCH)) &&
               (!IsEvenDsuid(m_DSID)); // odd dSID
      } else if (hasInput()) {
        // Only devices of type SDS, TKM and UMR can be paired.
        if ((getDeviceType() == DEVICE_TYPE_SDS) ||
            (getDeviceType() == DEVICE_TYPE_TKM) ||
            (getDeviceType() == DEVICE_TYPE_UMR)) {
          ret = ((m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT1) ||
                 (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_DW_WITH_INPUT3) ||
                 (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT1) ||
                 (m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_2WAY_UP_WITH_INPUT3) ||
                 ((m_ButtonInputMode == DEV_PARAM_BUTTONINPUT_SDS_SLAVE_M1_M2) &&
                 !IsEvenDsuid(m_DSID))); // odd dSID
        }
      }
    } catch (std::runtime_error &err) {
      log(err.what(), lsError);
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
      case 7:
        return DEVICE_TYPE_SK;
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
      case DEVICE_TYPE_SK:
        return "SK";
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

  void Device::updateModelFeatures() {
    decltype(m_modelFeatures) modelFeatures;
    auto&& subclass = (m_FunctionID >> 6) & 0x3F;
    if (subclass == 0x07) {
      modelFeatures[ModelFeatureId::apartmentapplication] = true;
    }
    if (m_modelFeatures == modelFeatures) {
      return;
    }
    m_modelFeatures.swap(modelFeatures);
    republishModelFeaturesToPropertyTree();
  }

  void Device::republishModelFeaturesToPropertyTree() {
    if (!m_pPropertyNode) {
      return;
    }
    // remove old nodes
    {
      auto&& node = m_pPropertyNode->getProperty("modelFeatures");
      if (node) {
        m_pPropertyNode->removeChild(node);
      }
    }
    // add new nodes
    auto&& modelFeaturesNode = m_pPropertyNode->createProperty("modelFeatures");
    foreach(auto&& modelFeaturePair, m_modelFeatures) {
      auto&& id = modelFeaturePair.first;
      if (auto&& name = modelFeatureName(id)) {
        auto&& node = modelFeaturesNode->createProperty(*name);
        node->setValue(modelFeaturePair.second);
      } else {
        log(ds::str("Cannot publish model feature ", id), lsWarning);
      }
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
      if ((deviceType == DEVICE_TYPE_KL) && ((getDeviceNumber() == 213) || (getDeviceNumber() == 214))) {
        m_iconPath = "ssl";
      } else if ((deviceType == DEVICE_TYPE_SDM) && ((getDeviceNumber() == 201) || (getDeviceNumber() == 202))) {
        m_iconPath = "sdm_plug";
      } else {
        m_iconPath = getDeviceTypeString(deviceType);
        std::transform(m_iconPath.begin(), m_iconPath.end(), m_iconPath.begin(), ::tolower);
      }
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

  const DeviceFeatures_t Device::getDeviceFeatures() const {
    DeviceFeatures_t features;
    features.pairing = false;
    features.syncButtonID = false;
    features.hasOutputAngle = false;
    features.posTimeMax = false;

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
    } else if ((devCls == DEVICE_CLASS_SW) && (devType == DEVICE_TYPE_UMR) &&
               (devNumber == 200)) {
      if (multiDeviceIndex() == 0) {
        features.pairing = true;
        features.syncButtonID = true;
      } else if (multiDeviceIndex() == 2) {
        features.pairing = true;
      }
    }

    if ((devCls == DEVICE_CLASS_GR) && (devType == DEVICE_TYPE_KL)) {
      if (getRevisionID() >= 0x360) {
        features.posTimeMax = true;
      }

      if ((devNumber == 220) || (devNumber == 230)) {
        features.hasOutputAngle = true;
      }
    }

    return features;
  }

  int Device::getJokerGroup() const {
    bool joker = false;

    DeviceClasses_t devCls = this->getDeviceClass();
    if (devCls != DEVICE_CLASS_SW) {
      return -1;
    }

    // TODO: is this valid? "Joker group" of device will be first standard group <= 8?
    for (int g = 1; g <= (int)DEVICE_CLASS_SW; g++) {
      if (isInGroup(g)) {
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
    case DEVICE_OEM_UNKNOWN:
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
      return DEVICE_OEM_UNKNOWN;
    }
  }

  void Device::setOemProductInfo(const std::string& _productName, const std::string& _iconPath, const std::string& _productURL, const std::string& _configLink)
  {
    m_OemProductName = _productName;
    m_OemProductIcon = _iconPath;
    m_OemProductURL = _productURL;
    m_OemConfigLink = _configLink;
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

  std::string Device::getValveTypeAsString() const
  {
    return valveTypeToString(m_ValveType);
  }

  bool Device::setValveTypeAsString(const std::string& _value, bool _ignoreValveCheck)
  {
    if (_ignoreValveCheck || isValveDevice()) {
      DeviceValveType_t type;
      if (getValveTypeFromString(_value.c_str(), type)) {
        setValveType(type);
        return true;
      }
    }
    return false;
  }

  std::string Device::valveTypeToString(const DeviceValveType_t _type) {
    switch (_type) {
    case DEVICE_VALVE_UNKNOWN:
      return "Unknown";
    case DEVICE_VALVE_FLOOR:
      return "Floor";
    case DEVICE_VALVE_RADIATOR:
      return "Radiator";
    case DEVICE_VALVE_WALL:
      return "Wall";
    }
    return "";
  }

  bool Device::getValveTypeFromString(const char* _string, DeviceValveType_t &deviceType) {
    bool assigned = false;
    if (strcmp(_string, "Floor") == 0) {
      deviceType = DEVICE_VALVE_FLOOR;
      assigned = true;
    } else if (strcmp(_string, "Radiator") == 0) {
      deviceType = DEVICE_VALVE_RADIATOR;
      assigned = true;
    } else if (strcmp(_string, "Wall") == 0) {
      deviceType = DEVICE_VALVE_WALL;
      assigned = true;
    } else if (strcmp(_string, "Unknown") == 0) {
      deviceType = DEVICE_VALVE_UNKNOWN;
      assigned = true;
    } else {
      log(std::string("Invalid valve type: ") + _string,
                                 lsWarning);
    }

    return assigned;
  }

  void Device::publishValveTypeToPropertyTree() {
    if (isValveDevice() && (m_pPropertyNode != NULL)) {
      PropertyNodePtr valveType = m_pPropertyNode->createProperty("ValveType");
      valveType->linkToProxy(PropertyProxyMemberFunction<Device, std::string, false>(*this, &Device::getValveTypeAsString));
    }
  }

  bool Device::isValveDevice() const {
    return (hasOutput() && ((getDeviceClass() == DEVICE_CLASS_BL) ||
                               ((getDeviceType() == DEVICE_TYPE_UMR) && (getRevisionID() >= 0x0383))));
  }

  DeviceValveType_t Device::getValveType () const {
    return m_ValveType;
  }

  void Device::setValveType(DeviceValveType_t _type) {
    m_ValveType = _type;
    if (DSS::getInstance()) {
      boost::shared_ptr<Event> pEvent;
      pEvent.reset(new Event(EventName::DeviceHeatingTypeChanged));
      pEvent->setProperty("zoneID", intToString(getZoneID()));
      if (!DSS::getInstance()->getModelMaintenance().isInitializing()) {
        DSS::getInstance()->getEventQueue().pushEvent(pEvent);
      }
    }
    dirty();
  }

  void Device::setDeviceBinaryInputType(uint8_t _inputIndex, BinaryInputType inputType) {
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    if (_inputIndex >= m_binaryInputs.size()) {
      throw ItemNotFoundException("Invalid binary input index");
    }
    lock.unlock();
    setDeviceConfig(CfgClassDevice, 0x40 + 3 * _inputIndex + 1, static_cast<int>(inputType));
  }

  void Device::setDeviceBinaryInputTargetId(uint8_t _inputIndex, uint8_t _targetGroup)
  {
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    if (_inputIndex >= m_binaryInputs.size()) {
      throw ItemNotFoundException("Invalid binary input index");
    }
    setDeviceConfig(CfgClassDevice, 0x40 + 3 * _inputIndex + 0, _targetGroup);
  }

  BinaryInputType Device::getDeviceBinaryInputType(uint8_t _inputIndex) {
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    if (_inputIndex >= m_binaryInputs.size()) {
      throw ItemNotFoundException("Invalid binary input index");
    }
    return m_binaryInputs[_inputIndex]->m_inputType;
  }

  void Device::setDeviceBinaryInputId(uint8_t _inputIndex, BinaryInputId id) {
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    if (_inputIndex >= m_binaryInputs.size()) {
      throw ItemNotFoundException("Invalid binary input index");
    }
    uint8_t val = (static_cast<int>(id) & 0xf) << 4;
    if (_inputIndex == m_binaryInputs.size() - 1) {
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

  uint8_t Device::getBinaryInputCount() const {
    return (uint8_t) m_binaryInputs.size();
  }

  void Device::setBinaryInputTargetId(uint8_t index, uint8_t targetGroupId) {
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    getBinaryInput(index)->setTargetId(targetGroupId);
  }

  void Device::setBinaryInputId(uint8_t index, BinaryInputId inputId) {
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    getBinaryInput(index)->setInputId(inputId);
  }

  void Device::setBinaryInputType(uint8_t index, BinaryInputType inputType) {
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    getBinaryInput(index)->setInputType(inputType);
  }

  void Device::clearBinaryInputs() {
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    m_binaryInputs.clear();
  }

  void Device::initStates(const std::vector<DeviceStateSpec_t>& stateSpecs) {
    if (stateSpecs.empty()) {
      return;
    }
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    PropertyNodePtr node;
    if (m_pPropertyNode != NULL) {
      node = m_pPropertyNode->getPropertyByName("states");
      if (node != NULL) {
        node->getParentNode()->removeChild(node);
      }
      node = m_pPropertyNode->createProperty("states");
    }
    m_states.clear();

    BOOST_FOREACH(const DeviceStateSpec_t& stateSpec, stateSpecs) {
      const std::string& stateName = stateSpec.Name;
      try {
        boost::shared_ptr<State> state = boost::make_shared<State>(sharedFromThis(), stateName);
        std::vector<std::string> values;
        values.push_back(State::INVALID);
        values.insert(values.end(), stateSpec.Values.begin(), stateSpec.Values.end());
        state->setValueRange(values);
        try {
          getApartment().allocateState(state);
        } catch (ItemDuplicateException& ex) {
          state = getApartment().getNonScriptState(state->getName());
        }
        m_states[stateName] = state;

        if (m_pPropertyNode != NULL) {
          PropertyNodePtr entry = node->createProperty(stateName);
          PropertyNodePtr stateValueNode = state->getPropertyNode()->getProperty("value");
          if (stateValueNode != NULL) {
            PropertyNodePtr stateValueAlias = entry->createProperty("stateValue");
            stateValueAlias->alias(stateValueNode);
          }
        }
      } catch (std::runtime_error& ex) {
          log("Device::initStates:" + dsuid2str(m_DSID)
              + " state:" + stateName + " what:" + ex.what(), lsError);
          throw ex;
      }
    }
  }

  void Device::clearStates() {
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    BOOST_FOREACH(const States::value_type& state, m_states) {
      try {
        m_pApartment->removeState(state.second);
      } catch (ItemNotFoundException& e) {
        log(std::string("Apartment::removeDevice: Unknown state: ") + e.what(), lsWarning);
      }
    }
    m_states.clear();
  }

  void Device::setStateValue(const std::string& name, const std::string& value) {
    log("Device::setStateValue name:" + name + " value:" + value, lsDebug);
    try {
      BOOST_FOREACH(const States::value_type& state, m_states) {
        if (state.first == name) {
          state.second->setState(coDsmApi, value);
        }
      }
    } catch(std::runtime_error& e) {
      log("Device::setStateValue name:" + name
          + " value:" + value + " what:" + e.what(), lsWarning);
    }
  }

  void Device::setStateValues(const std::vector<std::pair<std::string, std::string> >& values) {
    BOOST_FOREACH(const States::value_type& statePair, m_states) {
      const std::string* newValue = &State::INVALID;
      const std::string& stateName = statePair.first;
      try {
        typedef std::pair<std::string, std::string> StringStringPair;
        BOOST_FOREACH(const StringStringPair& value, values) {
          if (stateName == value.first) {
            newValue = &(value.second);
            break;
          }
        }
        log("Device::setStateValues name:" + stateName + " value:" + *newValue, lsDebug);
        statePair.second->setState(coDsmApi, *newValue);
      } catch(std::runtime_error& e) {
        log("Device::setStateValues name:" + stateName
            + " value:" + *newValue + " what:" + e.what(), lsWarning);
      }
    }
  }

  void Device::handleBinaryInputEvent(const int index, BinaryInputState inputState) {
    getBinaryInput(index)->handleEvent(inputState);
  }

  void Device::setBinaryInputs(const std::vector<DeviceBinaryInputSpec_t>& _binaryInputs) {
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    PropertyNodePtr binaryInputNode;
    if (m_pPropertyNode != NULL) {
      binaryInputNode = m_pPropertyNode->getPropertyByName("binaryInputs");
      if (binaryInputNode != NULL) {
        binaryInputNode->getParentNode()->removeChild(binaryInputNode);
      }
      binaryInputNode = m_pPropertyNode->createProperty("binaryInputs");
    }
    m_binaryInputs.clear();

    for (std::vector<DeviceBinaryInputSpec_t>::const_iterator it = _binaryInputs.begin();
        it != _binaryInputs.end();
        ++it) {
      auto binaryInputIndex = m_binaryInputs.size();
      m_binaryInputs.push_back(boost::make_shared<DeviceBinaryInput>(*this, *it, binaryInputIndex));
      auto& binaryInput = m_binaryInputs.back();

      if (m_pPropertyNode != NULL) {
        std::string bpath = std::string("binaryInput") + intToString(binaryInputIndex);
        PropertyNodePtr entry = binaryInputNode->createProperty(bpath);
        entry->createProperty("targetGroupId")
                ->linkToProxy(PropertyProxyReference<int>(binaryInput->m_targetGroupId));
        entry->createProperty("inputType")
                ->linkToProxy(PropertyProxyReference<int, BinaryInputType>(binaryInput->m_inputType));
        entry->createProperty("inputId")
                ->linkToProxy(PropertyProxyReference<int, BinaryInputId>(binaryInput->m_inputId));
        entry->createProperty("inputIndex")
                ->linkToProxy(PropertyProxyReference<int>(binaryInput->m_inputIndex));
        PropertyNodePtr stateNode = binaryInput->getState().getPropertyNode();
        PropertyNodePtr stateValueNode = stateNode->getProperty("value");
        if (stateValueNode != NULL) {
          PropertyNodePtr stateValueAlias = entry->createProperty("stateValue");
          stateValueAlias->alias(stateValueNode);
        }
      }
    }
  }

  const std::vector<boost::shared_ptr<DeviceBinaryInput> >& Device::getBinaryInputs() const {
    return m_binaryInputs;
  }

  const boost::shared_ptr<DeviceBinaryInput> Device::getBinaryInput(uint8_t _inputIndex) const {
    if (_inputIndex >= getBinaryInputCount()) {
      throw ItemNotFoundException(std::string("Device::getBinaryInput: index out of bounds"));
    }
    return m_binaryInputs[_inputIndex];
  }

  uint8_t Device::getSensorCount() const {
    return (uint8_t) m_sensorInputCount;
  }

  void Device::setSensors(const std::vector<DeviceSensorSpec_t>& _sensorInputs) {
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    PropertyNodePtr sensorInputNode;
    if (m_pPropertyNode != NULL) {
      sensorInputNode = m_pPropertyNode->getPropertyByName("sensorInputs");
      if (sensorInputNode != NULL) {
        sensorInputNode->getParentNode()->removeChild(sensorInputNode);
      }
      sensorInputNode = m_pPropertyNode->createProperty("sensorInputs");
    }
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

      boost::shared_ptr<DeviceSensor_t> binput = boost::make_shared<DeviceSensor_t>();
      binput->m_sensorIndex = m_sensorInputCount;
      binput->m_sensorType = it->sensorType;
      binput->m_sensorPollInterval = it->SensorPollInterval;
      binput->m_sensorBroadcastFlag = it->SensorBroadcastFlag;
      binput->m_sensorPushConversionFlag = it->SensorConversionFlag;
      binput->m_sensorValue = 0;
      binput->m_sensorValueFloat = 0;
      binput->m_sensorValueTS = DateTime::NullDate;
      binput->m_sensorValueValidity = false;
      m_sensorInputs.push_back(binput);

      if (m_pPropertyNode != NULL) {
        std::string bpath = std::string("sensorInput") + intToString(m_sensorInputCount);
        PropertyNodePtr entry = sensorInputNode->createProperty(bpath);
        entry->createProperty("type")
                ->linkToProxy(PropertyProxyReference<int, SensorType>(m_sensorInputs[m_sensorInputCount]->m_sensorType));
        entry->createProperty("index")
                ->linkToProxy(PropertyProxyReference<int>(m_sensorInputs[m_sensorInputCount]->m_sensorIndex));
        entry->createProperty("valid")
                ->linkToProxy(PropertyProxyReference<bool>(m_sensorInputs[m_sensorInputCount]->m_sensorValueValidity));
        entry->createProperty("value")
                ->linkToProxy(PropertyProxyReference<double>(m_sensorInputs[m_sensorInputCount]->m_sensorValueFloat));
        entry->createProperty("valueDS")
                ->linkToProxy(PropertyProxyReference<int, unsigned int>(m_sensorInputs[m_sensorInputCount]->m_sensorValue));
        entry->createProperty("timestamp")
                ->linkToProxy(PropertyProxyMemberFunction<DateTime, std::string, false>(m_sensorInputs[m_sensorInputCount]->m_sensorValueTS, &DateTime::toString));
      }

      m_sensorInputCount ++;
    }
  }

  void Device::setOutputChannels(const std::vector<int>& _outputChannels) {
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    PropertyNodePtr outputChannelNode;
    if (m_pPropertyNode != NULL) {
      outputChannelNode = m_pPropertyNode->getPropertyByName("outputChannels");
      if (outputChannelNode != NULL) {
        outputChannelNode->getParentNode()->removeChild(outputChannelNode);
      }
      outputChannelNode = m_pPropertyNode->createProperty("outputChannels");
    }
    m_outputChannelCount = 0;
    m_outputChannels.clear();

    // The first entry of the table describes the default channel. Do not add to channel list.
    if (_outputChannels.size() >= 1) {
      std::string bpath = std::string("outputChannelDefault");
      PropertyNodePtr entry = outputChannelNode->createProperty(bpath);
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
        std::string bpath = std::string("outputChannel") + intToString(m_outputChannelCount);
        PropertyNodePtr entry = outputChannelNode->createProperty(bpath);
        entry->createProperty("channelID")->setIntegerValue(m_outputChannels[m_outputChannelCount]);
      }

      m_outputChannelCount ++;
    }
  }

  int Device::getOutputChannelIndex(int _channelId) const {
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

  int Device::getOutputChannel(int _index) const {
    if (_index > m_outputChannelCount) {
      return -1;
    }
    return m_outputChannels[_index];
  }

  int Device::getOutputChannelCount() const {
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

  const boost::shared_ptr<DeviceSensor_t> Device::getSensorByType(SensorType _sensorType) const {
    for (size_t i = 0; i < getSensorCount(); i++) {
      boost::shared_ptr<DeviceSensor_t> sensor = m_sensorInputs.at(i);
      if (sensor->m_sensorType == _sensorType) {
        return sensor;
      }
    }

    throw ItemNotFoundException(std::string("Device::getSensor: no sensor with given type found"));
  }

  void Device::setSensorValue(int _sensorIndex, unsigned int _sensorValue) const {
    if (_sensorIndex >= getSensorCount()) {
      throw ItemNotFoundException(std::string("Device::setSensorValue: index out of bounds"));
    }
    DateTime now;
    m_sensorInputs[_sensorIndex]->m_sensorValue = _sensorValue;
    m_sensorInputs[_sensorIndex]->m_sensorValueFloat =
        sensorValueToDouble(m_sensorInputs[_sensorIndex]->m_sensorType, _sensorValue);
    m_sensorInputs[_sensorIndex]->m_sensorValueTS = now;
    m_sensorInputs[_sensorIndex]->m_sensorValueValidity = true;
  }

  void Device::setSensorValue(int _sensorIndex, double _sensorValue) const {
    if (_sensorIndex >= getSensorCount()) {
      throw ItemNotFoundException(std::string("Device::setSensorValue: index out of bounds"));
    }
    DateTime now;
    m_sensorInputs[_sensorIndex]->m_sensorValueFloat = _sensorValue;
    m_sensorInputs[_sensorIndex]->m_sensorValue =
            doubleToSensorValue(m_sensorInputs[_sensorIndex]->m_sensorType, _sensorValue);
    m_sensorInputs[_sensorIndex]->m_sensorValueTS = now;
    m_sensorInputs[_sensorIndex]->m_sensorValueValidity = true;
  }

  void Device::setSensorDataValidity(int _sensorIndex, bool _valid) const {
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
    return ((m_OemState == DEVICE_OEM_VALID) &&
            !m_OemIsIndependent &&
            (m_OemSerialNumber > 0) &&
            isPresent() &&
            _otherDev->isPresent() &&
            !_otherDev->getOemIsIndependent() &&
            (_otherDev->getOemInfoState() == DEVICE_OEM_VALID) &&
            (_otherDev->getDSID() != m_DSID) &&
            (_otherDev->getDSMeterDSID() != m_DSMeterDSID) &&
            (_otherDev->getOemEan() == m_OemEanNumber) &&
            (_otherDev->getOemSerialNumber() == m_OemSerialNumber));
  }

  void Device::setConfigLock(bool _lockConfig) {
    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    m_IsConfigLocked = _lockConfig;
  }

  void Device::setVdcDevice(bool _isVdcDevice)
  {
    m_isVdcDevice = _isVdcDevice;
    publishVdcToPropertyTree();
  }

  std::string Device::getDisplayID() const
  {
    dsid_t dsid;
    std::string displayID;
    if (dsuid_to_dsid(m_DSID, &dsid)) {
      displayID = dsid2str(dsid);
      displayID = displayID.substr(displayID.size() - 8);
    } else if (!m_VdcHardwareGuid.empty()) {
      displayID = m_VdcHardwareGuid.substr(m_VdcHardwareGuid.find(":") + 1);
    } else {
      displayID = dsuid2str(m_DSID).substr(0, 8) + "\u2026";
    }
    if (m_DSID.id[16] != 0) {
      displayID += "-" + intToString(m_DSID.id[16]);
    }
    return displayID;
  }

  uint8_t Device::getDeviceUMVRelayValue()
  {
    if ((getDeviceType() != DEVICE_TYPE_UMV) || (getDeviceNumber() != 200)) {
      throw std::runtime_error("unsupported configuration for this device");
    }
    return getDeviceConfig(CfgClassFunction, CfgFunction_UMV_Relay_Config);
  }

  void Device::setDeviceUMVRelayValue(uint8_t _value)
  {
    if ((getDeviceType() != DEVICE_TYPE_UMV) || (getDeviceNumber() != 200)) {
      throw std::runtime_error("unsupported configuration for this device");
    }

    setDeviceConfig(CfgClassFunction, CfgFunction_UMV_Relay_Config, _value);
  }

  bool Device::hasBlinkSettings() {
    if ((getDeviceType() == DEVICE_TYPE_UMR) ||
        (getDeviceClass() == DEVICE_CLASS_GE) ||
        (((getDeviceType() == DEVICE_TYPE_KL) ||
          (getDeviceType() == DEVICE_TYPE_ZWS)) &&
         (getDeviceClass() == DEVICE_CLASS_SW))) {
            return true;
    }

    return false;
  }
  void Device::setDeviceUMRBlinkRepetitions(uint8_t _count) {
    if (hasBlinkSettings()) {
      setDeviceConfig(CfgClassFunction, CfgFunction_FCount1, _count);

    } else {
      throw std::runtime_error("unsupported configuration for this device");
    }
  }

  void Device::setDeviceUMROnDelay(double _delay) {
    if (hasBlinkSettings()) {
      _delay = _delay * 1000.0; // convert from seconds to ms

      if ((_delay < 0) || (round(_delay / UMR_DELAY_STEPS) > UCHAR_MAX)) {
        throw std::runtime_error("invalid delay value");
      }
      uint8_t value = (uint8_t)round(_delay / UMR_DELAY_STEPS);
      setDeviceConfig(CfgClassFunction, CfgFunction_FOnTime1, value);
    } else {
      throw std::runtime_error("unsupported configuration for this device");
    }
  }

  void Device::setDeviceUMROffDelay(double _delay) {
    if (hasBlinkSettings()) {
      _delay = _delay * 1000.0; // convert from seconds to ms

      if ((_delay < 0) || (round(_delay / UMR_DELAY_STEPS) > UCHAR_MAX)) {
        throw std::runtime_error("invalid delay value");
      }
      uint8_t value = (uint8_t)round(_delay / UMR_DELAY_STEPS);
      setDeviceConfig(CfgClassFunction, CfgFunction_FOffTime1, value);
    } else {
      throw std::runtime_error("unsupported configuration for this device");
    }
  }

  void Device::getDeviceUMRDelaySettings(double *_ondelay, double *_offdelay,
                                         uint8_t  *_count) {
    if (hasBlinkSettings()) {
      uint16_t value = getDeviceConfigWord(CfgClassFunction, CfgFunction_FOnTime1);
      *_ondelay = (double)((value & 0xff) * UMR_DELAY_STEPS) / 1000.0;
      *_count = (uint8_t)(value >> 8) & 0xff;

      uint8_t value2 = getDeviceConfig(CfgClassFunction, CfgFunction_FOffTime1);
      *_offdelay = (value2 * UMR_DELAY_STEPS) / 1000.0; // convert to seconds
    } else {
      throw std::runtime_error("unsupported configuration for this device");
    }
  }

  std::vector<int> Device::getLockedScenes() {
    std::vector<int> ls;

    foreach (auto&& gid, m_groupIds) {
      if (isAppUserGroup(gid)) {
        boost::shared_ptr<Cluster> cluster = m_pApartment->getCluster(gid);
        if (cluster->isConfigurationLocked()) {
            const std::vector<int>& locks = cluster->getLockedScenes();
            ls.insert(ls.end(), locks.begin(), locks.end());
        }
      }
    }

    return ls;
  }

  bool Device::isInLockedCluster() {
    foreach (auto&& gid, m_groupIds) {
      if (isAppUserGroup(gid)) {
        boost::shared_ptr<Cluster> cluster = m_pApartment->getCluster(gid);
        if (cluster->isConfigurationLocked()) {
          return true;
        }
      }
    }

    return false;
  }

  int Device::multiDeviceIndex() const {
    uint8_t deviceCount = 1;
    uint8_t deviceIndex = 0;
    if ((getDeviceType() == DEVICE_TYPE_UMV) && (getDeviceNumber() == 200) &&
        (getDeviceClass() == DEVICE_CLASS_GE)) {
      deviceCount = 4;
    } else if ((getDeviceType() == DEVICE_TYPE_UMR) &&
               (getDeviceNumber() == 200) &&
               (getDeviceClass() == DEVICE_CLASS_SW)) {
      deviceCount = 4;
    } else if ((getDeviceType() == DEVICE_TYPE_SK) &&
               (getDeviceNumber() == 204)) {
      deviceCount = 2;
    } else if ((m_FunctionID & 0xffc0) == 0x1000) {
      switch (m_FunctionID & 0x7) {
        case 0: deviceCount = 1; break;
        case 1: deviceCount = 2; break;
        case 2: deviceCount = 4; break;
        case 7: deviceCount = 1; break;
      }
    } else if ((m_FunctionID & 0x0fc0) == 0x0100) {
      switch (m_FunctionID & 0x3) {
        case 0: deviceCount = 1; break;
        case 1: deviceCount = 1; break;
        case 2: deviceCount = 2; break;
        case 3: deviceCount = 4; break;
      }
    }

    uint32_t serial;
    if (dsuid_get_serial_number(&m_DSID, &serial) == 0) {
      deviceIndex = (uint8_t)(serial & 0xff) % deviceCount;
    }
    return deviceIndex;
  }

  void Device::setCardinalDirection(CardinalDirection_t _direction, bool _initial) {
    assert(valid(_direction));

    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);
    if (m_cardinalDirection != _direction) {
      m_cardinalDirection = _direction;
      if (!_initial) {
        checkAutoCluster();
      }
    }
  }

  void Device::setWindProtectionClass(WindProtectionClass_t _klass, bool _initial) {
    assert(valid(_klass));

    boost::recursive_mutex::scoped_lock lock(m_deviceMutex);

    if (!_initial) {
      if (getDeviceClass() != DEVICE_CLASS_GR) {
        throw std::runtime_error("Wind protection class can only be set on grey devices");
      }
      if ((getProductID() == ProductID_KL_210) && !(_klass <= wpc_awning_class_3)) {
        throw std::runtime_error("Wind protection class " + intToString(_klass) + "not applicable for awning devices");
      }
      if ((getProductID() != ProductID_KL_210) && !((_klass == wpc_none) || (_klass >= wpc_blind_class_1))) {
        throw std::runtime_error("Wind protection class " + intToString(_klass) + "not applicable for blinds devices");
      }
    }
    if (m_windProtectionClass != _klass) {
      m_windProtectionClass = _klass;
      if (!_initial) {
        checkAutoCluster();
      }
    }
  }

  uint16_t Device::getDeviceMaxMotionTime() {
    DeviceFeatures_t features = getDeviceFeatures();
    if (!features.posTimeMax) {
      throw std::runtime_error("Maximum motion time setting not supported"
                               "by this device");
    }

    uint16_t hw = getDeviceConfigWord(CfgClassFunction, CfgFunction_Shade_PosTimeMax);
    // 100 halfwaves = 1 second
    return (hw / 100);
  }

  void Device::setDeviceMaxMotionTime(uint16_t seconds) {
    DeviceFeatures_t features = getDeviceFeatures();
    if (!features.posTimeMax) {
      throw std::runtime_error("Maximum motion time setting not supported"
                               "by this device");
    }

    if ((seconds * 100) > USHRT_MAX) {
      throw std::runtime_error("Seconds value unsupported: too large");
    }

    setDeviceConfig16(CfgClassFunction, CfgFunction_Shade_PosTimeMax, seconds * 100);
  }

  uint8_t Device::getSWThresholdAddress() const {
    if (getDeviceType() == DEVICE_TYPE_KM ||
        getDeviceType() == DEVICE_TYPE_SDM ||
        getDeviceType() == DEVICE_TYPE_SDS ||
        getDeviceType() == DEVICE_TYPE_TKM) {
      return CfgFunction_KM_SWThreshold;
    }
    if ((getDeviceType() == DEVICE_TYPE_KL || getDeviceType() == DEVICE_TYPE_ZWS) &&
        (getDeviceClass() == DEVICE_CLASS_GE || getDeviceClass() == DEVICE_CLASS_SW)) {
      return CfgFunction_KL_SWThreshold;
    }
    if (getDeviceType() == DEVICE_TYPE_UMV && getDeviceClass() == DEVICE_CLASS_GE) {
      return CfgFunction_UMV_SWThreshold;
    }
    if (getDeviceType() == DEVICE_TYPE_TNY && getDeviceClass() == DEVICE_CLASS_SW && isMainDevice()) {
      return CfgFunction_Tiny_SWThreshold;
    }
    if (getDeviceClass() == DEVICE_CLASS_BL) {
      return CfgFunction_Valve_SWThreshold;
    }
    if (getDeviceType() == DEVICE_TYPE_UMR && getDeviceClass() == DEVICE_CLASS_SW) {
      return CfgFunction_UMR_SWThreshold;
    }
    throw std::runtime_error("Device does not support changing the switching threshold");
  }

  void Device::setSwitchThreshold(uint8_t _threshold) {
    if (_threshold == 0 || _threshold == 255) {
      throw std::runtime_error("Threshold must not be 0 or 255");
    }

    setDeviceConfig(CfgClassFunction, getSWThresholdAddress(), _threshold);
  }

  uint8_t Device::getSwitchThreshold() {
    return getDeviceConfig(CfgClassFunction, getSWThresholdAddress());
  }

  void Device::setTemperatureOffset(int8_t _offset) {
    if (getDeviceType() != DEVICE_TYPE_SK) {
      throw std::runtime_error("Temperature offset setting is not supported by this device");
    }
    setDeviceConfig(CfgClassFunction, CfgFunction_SK_TempOffsetExt, _offset);
  }

  int8_t Device::getTemperatureOffset() {
    if (getDeviceType() != DEVICE_TYPE_SK) {
      throw std::runtime_error("Temperature offset setting is not supported by this device");
    }
    return getDeviceConfig(CfgClassFunction, CfgFunction_SK_TempOffsetExt);
  }

  void Device::setPairedDevices(int _num) {
    bool update = (m_pairedDevices != _num);

    m_pairedDevices = _num;

    if (isMainDevice() && update && (m_pPropertyNode != NULL)) {
      PropertyNodePtr paired = m_pPropertyNode->getPropertyByName("pairedDevices");
      if (paired != NULL) {
        m_pPropertyNode->removeChild(paired);
      }

      if (_num > 0) {
        dsuid_t current = getDSID();
        for (int i = 0; i < _num - 1; i++) {
          PropertyNodePtr sub = m_pPropertyNode->
              createProperty("pairedDevices/device" + intToString(i));
          dsuid_t next;
          dsuid_get_next_dsuid(current, &next);
          sub->createProperty("dSUID")->setStringValue(dsuid2str(next));
          current = next;
        }
      }
    }
  }

  int Device::getPairedDevices() const {
    if ((getDeviceType() == DEVICE_TYPE_TNY) ||
        ((getDeviceType() == DEVICE_TYPE_SK) && (getDeviceNumber() == 204))) {
      return m_pairedDevices;
    }

    return 0;
  }

  void Device::setVisibility(bool _isVisible) {
    bool wasVisible = m_visible;
    m_visible = _isVisible;
    if ((getDeviceType() == DEVICE_TYPE_TNY) ||
        ((getDeviceType() == DEVICE_TYPE_SK) && (getDeviceNumber() == 204))) {
      if (wasVisible && !m_visible) {
        removeFromPropertyTree();
      } else if (!wasVisible && m_visible) {
        publishToPropertyTree();
      }
    }
  }

  void Device::setDeviceVisibility(bool _isVisible) {
    if ((getDeviceType() == DEVICE_TYPE_TNY) ||
        ((getDeviceType() == DEVICE_TYPE_SK) && (getDeviceNumber() == 204))) {
      if (isMainDevice()) {
        throw std::runtime_error("Visibility setting not allowed on main device");
      }

      uint8_t val = getDeviceConfig(CfgClassDevice, CfgFunction_DeviceActive);
      uint8_t bit4 = ((val & (1 << 4)) == 0); // inverted logic, 0 is "active"
      if (bit4 != _isVisible) {
        val ^= 1 << 4;
        setDeviceConfig(CfgClassDevice, CfgFunction_DeviceActive, val);
        setVisibility(_isVisible);
      }
    } else {
      throw std::runtime_error("Visibility setting is not supported by this device");
    }
  }

  bool Device::isVisible() const {
    if ((getDeviceType() == DEVICE_TYPE_TNY) ||
        ((getDeviceType() == DEVICE_TYPE_SK) && (getDeviceNumber() == 204))) {
      return m_visible;
    }

    return true;
  }

  bool Device::isMainDevice() const {
    if (m_pairedDevices == 0) {
      return true;
    }

    uint32_t serial;
    if (dsuid_get_serial_number(&m_DSID, &serial) == 0) {
      if ((serial % m_pairedDevices) != 0) {
        return false;
      }
    }
    return true;
  }

  dsuid_t Device::getMainDeviceDSUID() const {
    if (!isMainDevice()) {
      dsuid_t main_dsuid;
      if (dsuid_get_main_dsuid(&m_DSID, m_pairedDevices, &main_dsuid) == 0) {
        return main_dsuid;
      }
    }

    return getDSID();
  }

  void Device::callAction(const std::string& actionId, const vdcapi::PropertyElement& params) {
    if (!m_isVdcDevice) {
      throw std::runtime_error("CallAction can be called only on vdc devices.");
    }
    DeviceBusInterface* deviceBusInterface = m_pApartment->getDeviceBusInterface();
    if (!deviceBusInterface) {
      throw std::runtime_error("Bus interface not available");
    }

    google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> actionCall;
    vdcapi::PropertyElement* param0 = actionCall.Add();
    param0->set_name("id");
    param0->mutable_value()->set_v_string(actionId);

    if (params.elements_size()) {
      vdcapi::PropertyElement* param1 = actionCall.Add();
      param1->set_name("params");
      *param1->mutable_elements() = params.elements();
    }
    deviceBusInterface->genericRequest(*this, "invokeDeviceAction", actionCall);

    //action finished with success -> raise event
    auto deviceReference = boost::make_shared<DeviceReference>(getDSID(), &getApartment());
    boost::shared_ptr<Event> evt = createDeviceActionEvent(deviceReference, actionId, params);
    DSS::getInstance()->getEventQueue().pushEvent(evt);
  }

  void Device::setProperty(const vdcapi::PropertyElement& propertyElement) {
    if (!m_isVdcDevice) {
      throw std::runtime_error("CallAction can be called only on vdc devices.");
    }
    DeviceBusInterface* deviceBusInterface = m_pApartment->getDeviceBusInterface();
    if (!deviceBusInterface) {
      throw std::runtime_error("Bus interface not available");
    }
    google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> setPropertyParams;
    vdcapi::PropertyElement* devicePropertiesElement = setPropertyParams.Add();
    devicePropertiesElement->set_name("deviceProperties");
    *devicePropertiesElement->add_elements() = propertyElement;
    deviceBusInterface->setProperty(*this, setPropertyParams);
  }

  void Device::setCustomAction(const std::string& id, const std::string& title, const std::string& action,
                               const vdcapi::PropertyElement& actionParams) {
    if (!m_isVdcDevice) {
      throw std::runtime_error("CallAction can be called only on vdc devices.");
    }
    DeviceBusInterface* deviceBusInterface = m_pApartment->getDeviceBusInterface();
    if (!deviceBusInterface) {
      throw std::runtime_error("Bus interface not available");
    }
    if (!beginsWith(id, "custom.")) {
      throw std::runtime_error("Custom action id must start with 'custom.'");
    }

    google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> setPropertyParams;
    vdcapi::PropertyElement* customActionsElement = setPropertyParams.Add();
    customActionsElement->set_name("customActions");
    vdcapi::PropertyElement* actionElement = customActionsElement->add_elements();
    actionElement->set_name(id);
    if (!action.empty()) { // empty action means remove
      {
        vdcapi::PropertyElement* element = actionElement->add_elements();
        element->set_name("title");
        element->mutable_value()->set_v_string(title);
      }
      {
        vdcapi::PropertyElement* element = actionElement->add_elements();
        element->set_name("action");
        element->mutable_value()->set_v_string(action);
      }
      {
        vdcapi::PropertyElement* element = actionElement->add_elements();
        element->set_name("params");
        *element->mutable_elements() = actionParams.elements();
      }
    }
    deviceBusInterface->setProperty(*this, setPropertyParams);

    auto deviceReference = boost::make_shared<DeviceReference>(getDSID(), &getApartment());
    boost::shared_ptr<Event> evt = createDeviceCustomActionChangedEvent(deviceReference, id, action, title, actionParams);
    DSS::getInstance()->getEventQueue().pushEvent(evt);
  }

  vdcapi::Message Device::getVdcProperty(
      const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& query) {
    if (!m_isVdcDevice) {
      throw std::runtime_error("getVdcProperty can be called only on vdc devices.");
    }
    DeviceBusInterface* deviceBusInterface = m_pApartment->getDeviceBusInterface();
    if (!deviceBusInterface) {
      throw std::runtime_error("Bus interface not available");
    }
    return deviceBusInterface->getProperty(*this, query);
  }

  void Device::setVdcProperty(
      const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& query) {
    if (!m_isVdcDevice) {
      throw std::runtime_error("getVdcProperty can be called only on vdc devices.");
    }
    DeviceBusInterface* deviceBusInterface = m_pApartment->getDeviceBusInterface();
    if (!deviceBusInterface) {
      throw std::runtime_error("Bus interface not available");
    }
    deviceBusInterface->setProperty(*this, query);
  }

  void Device::setVdcSpec(VdsdSpec_t &&x) {
    m_vdcSpec = std::unique_ptr<VdsdSpec_t>(new VdsdSpec_t(std::move(x)));
  }

  std::ostream& operator<<(std::ostream& stream, const Device& x) {
    return stream << x.getName();
  }

} // namespace dss
