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

#include "jsonhelper.h"

#include "src/foreach.h"

#include "src/datetools.h"

#include "src/model/devicereference.h"
#include "src/model/device.h"
#include "src/model/set.h"
#include "src/model/group.h"
#include "src/model/cluster.h"
#include "src/model/zone.h"
#include "src/model/state.h"
#include "src/model/apartment.h"
#include "src/dss.h"
#include "src/model/modulator.h"
#include "src/web/webrequests.h"

namespace dss {

  void toJSON(const DeviceReference& _device, JSONWriter& _json) {
    DateTime tmp_date;

    _json.startObject();
    dsid_t dsid;
    if (dsuid_to_dsid(_device.getDSID(), &dsid)) {
      _json.add("id", dsid2str(dsid));
    } else {
      _json.add("id", "");
    }
    _json.add("DisplayID", _device.getDevice()->getDisplayID());
    _json.add("dSUID", _device.getDSID());
    _json.add("GTIN", _device.getDevice()->getGTIN());
    _json.add("name", _device.getName());
    _json.add("dSUIDIndex", _device.getDevice()->multiDeviceIndex());
    _json.add("functionID", _device.getFunctionID());
    _json.add("productRevision", _device.getDevice()->getRevisionID());
    _json.add("productID", _device.getDevice()->getProductID());
    _json.add("hwInfo", _device.getDevice()->getHWInfo());
    _json.add("OemStatus", _device.getDevice()->getOemStateAsString());
    _json.add("OemEanNumber", _device.getDevice()->getOemEanAsString());
    _json.add("OemSerialNumber", _device.getDevice()->getOemSerialNumber());
    _json.add("OemPartNumber", _device.getDevice()->getOemPartNumber());
    _json.add("OemProductInfoState", _device.getDevice()->getOemProductInfoStateAsString());
    _json.add("OemProductURL", _device.getDevice()->getOemProductURL());
    _json.add("OemInternetState", _device.getDevice()->getOemInetStateAsString());
    _json.add("OemIsIndependent", _device.getDevice()->getOemIsIndependent());
    _json.add("modelFeatures", _device.getDevice()->getModelFeatures());
    _json.add("isVdcDevice", _device.getDevice()->isVdcDevice());
    if (_device.getDevice()->isVdcDevice()) {
      _json.add("VdcHardwareModelGuid", _device.getDevice()->getVdcHardwareModelGuid());
      _json.add("VdcModelUID", _device.getDevice()->getVdcModelUID());
      _json.add("VdcModelVersion", _device.getDevice()->getVdcModelVersion());
      _json.add("VdcVendorGuid", _device.getDevice()->getVdcVendorGuid());
      _json.add("VdcOemGuid", _device.getDevice()->getVdcOemGuid());
      _json.add("VdcConfigURL", _device.getDevice()->getVdcConfigURL());
      _json.add("VdcHardwareGuid", _device.getDevice()->getVdcHardwareGuid());
      _json.add("VdcHardwareInfo", _device.getDevice()->getVdcHardwareInfo());
      _json.add("VdcHardwareVersion", _device.getDevice()->getVdcHardwareVersion());
      _json.add("hasActions", bool(_device.getDevice()->getHasActions() ));
    }
    if (_device.getDevice()->isValveDevice()) {
      _json.add("ValveType", _device.getDevice()->getValveTypeAsString());
    }

    if(_device.getDevice()->isPresent()) {
      dsid_t dsid;
      if (dsuid_to_dsid(_device.getDevice()->getDSMeterDSID(), &dsid)) {
        _json.add("meterDSID", dsid2str(dsid));
      } else {
        _json.add("meterDSID", "");
      }
      _json.add("meterDSUID", _device.getDevice()->getDSMeterDSID());
      std::string dSMName;
      try {
        dSMName = DSS::getInstance()->getApartment().getDSMeterByDSID(_device.getDevice()->getDSMeterDSID())->getName();
      } catch(std::runtime_error&) {
      }
      _json.add("meterName", dSMName);
      _json.add("busID", _device.getDevice()->getShortAddress());
    } else {
      dsid_t dsid;
      if (dsuid_to_dsid(_device.getDevice()->getLastKnownDSMeterDSID(), &dsid)) {
        _json.add("meterDSID", dsid2str(dsid));
      } else {
        _json.add("meterDSID", "");
      }
      _json.add("meterDSUID", _device.getDevice()->getLastKnownDSMeterDSID());
      std::string dSMName;
      try {
        dSMName = DSS::getInstance()->getApartment().getDSMeterByDSID(_device.getDevice()->getDSMeterDSID())->getName();
      } catch(std::runtime_error&) {
      }
      _json.add("meterName", dSMName);
      _json.add("busID", _device.getDevice()->getLastKnownShortAddress());
    }
    _json.add("zoneID", _device.getDevice()->getZoneID());
    _json.add("isPresent", _device.getDevice()->isPresent());
    _json.add("isValid", _device.getDevice()->isValid());

    tmp_date = _device.getDevice()->getLastDiscovered();
    _json.add("lastDiscovered", tmp_date);

    tmp_date = _device.getDevice()->getFirstSeen();
    _json.add("firstSeen", tmp_date);

    tmp_date = _device.getDevice()->getInactiveSince();
    _json.add("inactiveSince", tmp_date);

    _json.add("on", _device.getDevice()->isOn());
    _json.add("locked", _device.getDevice()->getIsLockedInDSM());
    _json.add("configurationLocked", _device.getDevice()->isConfigLocked());
    _json.add("outputMode", _device.getDevice()->getOutputMode());
    _json.add("buttonID", _device.getDevice()->getButtonID());
    _json.add("buttonActiveGroup", _device.getDevice()->getButtonActiveGroup());
    _json.add("buttonGroupMembership", _device.getDevice()->getButtonActiveGroup()); // deprecated
    _json.add("buttonInputMode", _device.getDevice()->getButtonInputMode());
    _json.add("buttonInputIndex", _device.getDevice()->getButtonInputIndex());
    _json.add("buttonInputCount", _device.getDevice()->getButtonInputCount());
    _json.add("AKMInputProperty", _device.getDevice()->getAKMInputProperty());

    _json.startArray("groups");
    for (int g = 0; g < _device.getDevice()->getGroupsCount(); g++) {
      _json.add(_device.getDevice()->getGroupIdByIndex(g));
    }
    _json.endArray();

    auto&& binaryInputs = _device.getDevice()->getBinaryInputs();
    _json.add("binaryInputCount", (int)binaryInputs.size());
    _json.startArray("binaryInputs");
    for (auto&& it = binaryInputs.begin(); it != binaryInputs.end(); ++it) {
      _json.startObject();
      _json.add("targetGroup", (*it)->m_targetGroupId);
      _json.add("inputType", static_cast<int>((*it)->m_inputType));
      _json.add("inputId", static_cast<int>((*it)->m_inputId));
      _json.add("state", (*it)->getState().getState());
      _json.endObject();
    }
    _json.endArray();
    _json.add("sensorInputCount", (int)(_device.getDevice()->getSensorCount()));

    _json.startArray("sensors");
    const std::vector<boost::shared_ptr<DeviceSensor_t> > sensors = _device.getDevice()->getSensors();
    for (size_t i = 0; i < sensors.size(); i++) {
      _json.startObject();
      _json.add("type", static_cast<int>(sensors.at(i)->m_sensorType));
      _json.add("valid", sensors.at(i)->m_sensorValueValidity);
      _json.add("value", sensors.at(i)->m_sensorValueFloat);
      _json.endObject();
    }
    _json.endArray();

    // check if device has invalid sensor values
    uint8_t sensorCount = _device.getDevice()->getSensorCount();
    bool sensorFlag = true;
    for (uint8_t s = 0; s < sensorCount; s++) {
      if (!_device.getDevice()->isSensorDataValid(s)) {
        sensorFlag = false;
        break;
      }
    }
    _json.add("sensorDataValid", sensorFlag);

    _json.startArray("outputChannels");
    for (int i = 0; i < _device.getDevice()->getOutputChannelCount(); i++) {
      boost::shared_ptr<DeviceChannel_t> pChannel = _device.getDevice()->getOutputChannel(i);
      _json.startObject();
      _json.add("channelId", pChannel->m_channelId);
      _json.add("channelType", pChannel->m_channelType);
      _json.add("channelIndex", pChannel->m_channelIndex);
      _json.add("channelName", pChannel->m_channelName);
      _json.endObject();
    }
    _json.endArray();

    // check if this is a "main" device, if yes render dSUIDs of all
    // partner devices
    _json.startArray("pairedDevices");
    dsuid_t dsuid = _device.getDevice()->getDSID();
    // exclude main device in the list
    if (_device.getDevice()->isMainDevice() &&
        (_device.getDevice()->getPairedDevices() > 0)) {
      for (int pd = 0; pd < (_device.getDevice()->getPairedDevices() - 1); pd++) {
          dsuid_t next;
          dsuid_get_next_dsuid(dsuid, &next);
          _json.add(next);
          dsuid = next;
      }
    }
    _json.endArray();

    _json.endObject();
  } // toJSON(DeviceReference)

  void toJSON(boost::shared_ptr<const DSMeter> _dsMeter, JSONWriter& _json) {
    _json.startObject();
    _json.add("name", _dsMeter->getName());
    dsid_t dsid;
    if (dsuid_to_dsid(_dsMeter->getDSID(), &dsid)) {
      _json.add("dsid", dsid2str(dsid));
    } else {
      _json.add("dsid", "");
    }
    _json.add("dSUID", _dsMeter->getDSID());
    _json.add("DisplayID", _dsMeter->getDisplayID());
    _json.add("hwVersion", 0);
    _json.add("hwVersionString", _dsMeter->getHardwareVersion());
    _json.add("swVersion", _dsMeter->getSoftwareVersion());
    _json.add("armSwVersion", _dsMeter->getArmSoftwareVersion());
    _json.add("dspSwVersion", _dsMeter->getDspSoftwareVersion());
    _json.add("apiVersion", _dsMeter->getApiVersion());
    _json.add("hwName", _dsMeter->getHardwareName());
    _json.add("isPresent", _dsMeter->isPresent());
    _json.add("isValid", _dsMeter->isValid());
    _json.add("busMemberType", _dsMeter->getBusMemberType());
    _json.add("hasDevices", _dsMeter->getCapability_HasDevices());
    _json.add("hasMetering", _dsMeter->getCapability_HasMetering());
    _json.add("VdcConfigURL", _dsMeter->getVdcConfigURL());
    _json.add("VdcModelUID", _dsMeter->getVdcModelUID());
    _json.add("VdcHardwareGuid", _dsMeter->getVdcHardwareGuid());
    _json.add("VdcHardwareModelGuid", _dsMeter->getVdcHardwareModelGuid());
    _json.add("VdcImplementationId", _dsMeter->getVdcImplementationId());
    _json.add("VdcVendorGuid", _dsMeter->getVdcVendorGuid());
    _json.add("VdcOemGuid", _dsMeter->getVdcOemGuid());
    std::bitset<8> flags = _dsMeter->getPropertyFlags();
    _json.add("ignoreActionsFromNewDevices", flags.test(4));
    _json.endObject();
  }

  void toJSON(const Set& _set, JSONWriter& _json, bool _showHidden) {
    for(int iDevice = 0; iDevice < _set.length(); iDevice++) {
      const DeviceReference& d = _set.get(iDevice);
      if (d.getDevice()->is2WaySlave() == true) {
        // do not render "slave" devices
        continue;
      }
      if (!_showHidden && !d.getDevice()->isVisible() && d.getDevice()->isPresent()) {
        // do not render hidden TNY devices except for inactive ones
        continue;
      }
      toJSON(d, _json);
    }
  } // toJSON(Set,Name)

  void toJSON(boost::shared_ptr<const Group> _group, JSONWriter& _json) {
    _json.startObject();
    _json.add("id", _group->getID());
    _json.add("name", _group->getName());
    _json.add("color", _group->getColor());
    _json.add("applicationType", static_cast<int>(_group->getApplicationType()));
    _json.add("isPresent", _group->isPresent());
    _json.add("isValid", _group->isValid());
    _group->serializeApplicationConfiguration(_group->getApplicationConfiguration(), _json);

    _json.startArray("devices");
    Set devices = _group->getDevices();
    for(int iDevice = 0; iDevice < devices.length(); iDevice++) {
      if (devices[iDevice].getDevice()->is2WaySlave()) {
        // do not render "slave" devices
        continue;
      }
      if (devices[iDevice].getDevice()->isVisible() != true) {
        // do not render hidden TNY devices
        continue;
      }
      _json.add(devices[iDevice].getDSID());
    }
    _json.endArray();
    _json.endObject();
  } // toJSON(Group)

  void toJSON(boost::shared_ptr<const Cluster> _cluster, JSONWriter& _json) {
    _json.startObject();
    _json.add("id", _cluster->getID());
    _json.add("name", _cluster->getName());
    _json.add("color", _cluster->getColor());
    _json.add("applicationType", static_cast<int>(_cluster->getApplicationType()));
    _json.add("isPresent", _cluster->isPresent());
    _json.add("isValid", _cluster->isValid());
    _cluster->serializeApplicationConfiguration(_cluster->getApplicationConfiguration(), _json);
    _json.add("CardinalDirection", toString(_cluster->getLocation()));
    _json.add("ProtectionClass", _cluster->getProtectionClass());
    _json.add("isAutomatic", _cluster->isAutomatic());
    _json.add("configurationLock", _cluster->isConfigurationLocked());

    _json.startArray("devices");
    Set devices = _cluster->getDevices();
    for(int iDevice = 0; iDevice < devices.length(); iDevice++) {
      if (devices[iDevice].getDevice()->is2WaySlave()) {
        // do not render "slave" devices
        continue;
      }
      if (devices[iDevice].getDevice()->isVisible() != true) {
        // do not render hidden TNY devices
        continue;
      }
      _json.add(devices[iDevice].getDSID());
    }
    _json.endArray();
    _json.endObject();
  } // toJSON(Cluster)

  void toJSON(Zone& _zone, JSONWriter& _json, bool _includeDevices) {
    _json.startObject();
    _json.add("id", _zone.getID());
    _json.add("name", _zone.getName());
    _json.add("isPresent", _zone.isPresent());

    if(_includeDevices) {
      _json.startArray("devices");
      toJSON(_zone.getDevices(), _json);
      _json.endArray();
      _json.startArray("groups");
      foreach(boost::shared_ptr<Group> pGroup, _zone.getGroups()) {
        toJSON(pGroup, _json);
      }
      _json.endArray();
    }

    _json.endObject();
  } // toJSON(Zone)

  void toJSON(Apartment& _apartment, JSONWriter& _json) {
    _json.startObject("apartment");

    _json.startArray("clusters");
    std::vector<boost::shared_ptr<Cluster> > clusters = _apartment.getClusters();
    foreach (boost::shared_ptr<Cluster> pCluster, clusters) {
      if (pCluster->getApplicationType() == ApplicationType::None) {
        continue;
      }
      toJSON(static_cast<boost::shared_ptr<const Cluster> >(pCluster), _json);
    }
    _json.endArray();

    _json.startArray("zones");
    std::vector<boost::shared_ptr<Zone> > zones = _apartment.getZones();
    foreach(boost::shared_ptr<Zone> pZone, zones) {
      toJSON(*pZone, _json);
    }
    _json.endArray();
    _json.endObject();
  } // toJSON(Apartment)

}
