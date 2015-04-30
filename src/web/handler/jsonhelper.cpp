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
    _json.add("dSUID", dsuid2str(_device.getDSID()));
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
    _json.add("isVdcDevice", _device.getDevice()->isVdcDevice());
    if (_device.getDevice()->isVdcDevice()) {
      _json.add("VdcHardwareModelGuid", _device.getDevice()->getVdcHardwareModelGuid());
      _json.add("VdcModelUID", _device.getDevice()->getVdcModelUID());
      _json.add("VdcVendorGuid", _device.getDevice()->getVdcVendorGuid());
      _json.add("VdcOemGuid", _device.getDevice()->getVdcOemGuid());
      _json.add("VdcConfigURL", _device.getDevice()->getVdcConfigURL());
      _json.add("VdcHardwareGuid", _device.getDevice()->getVdcHardwareGuid());
      _json.add("VdcHardwareInfo", _device.getDevice()->getVdcHardwareInfo());
      _json.add("VdcHardwareVersion", _device.getDevice()->getVdcHardwareVersion());
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
      _json.add("meterDSUID", dsuid2str(_device.getDevice()->getDSMeterDSID()));
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
      _json.add("meterDSUID", dsuid2str(_device.getDevice()->getLastKnownDSMeterDSID()));
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
    _json.add("buttonGroupMembership", _device.getDevice()->getButtonGroupMembership());
    _json.add("buttonInputMode", _device.getDevice()->getButtonInputMode());
    _json.add("buttonInputIndex", _device.getDevice()->getButtonInputIndex());
    _json.add("buttonInputCount", _device.getDevice()->getButtonInputCount());
    _json.add("AKMInputProperty", _device.getDevice()->getAKMInputProperty());

    _json.startArray("groups");
    std::bitset<63> deviceGroups = _device.getDevice()->getGroupBitmask();
    for (int g = 1; g <= 63; g++) {
      if (deviceGroups.test(g-1)) {
        _json.add(g);
      }
    }
    _json.endArray();

    const std::vector<boost::shared_ptr<DeviceBinaryInput_t> > binaryInputs = _device.getDevice()->getBinaryInputs();
    _json.add("binaryInputCount", (int)binaryInputs.size());
    _json.startArray("binaryInputs");
    for (std::vector<boost::shared_ptr<DeviceBinaryInput_t> >::const_iterator it = binaryInputs.begin(); it != binaryInputs.end(); ++it) {
      _json.startObject();
      _json.add("targetGroupType", (*it)->m_targetGroupType);
      _json.add("targetGroup", (*it)->m_targetGroupId);
      _json.add("inputType", (*it)->m_inputType);
      _json.add("inputId", (*it)->m_inputId);
      _json.add("state", _device.getDevice()->getBinaryInputState((*it)->m_inputIndex)->getState());
      _json.endObject();
    }
    _json.endArray();
    _json.add("sensorInputCount", (int)(_device.getDevice()->getSensorCount()));

    _json.startArray("sensors");
    const std::vector<boost::shared_ptr<DeviceSensor_t> > sensors = _device.getDevice()->getSensors();
    for (size_t i = 0; i < sensors.size(); i++) {
      _json.add(sensors.at(i)->m_sensorType);
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
    _json.endObject();
  } // toJSON(DeviceReference)

  void toJSON(const Set& _set, JSONWriter& _json) {
    for(int iDevice = 0; iDevice < _set.length(); iDevice++) {
      const DeviceReference& d = _set.get(iDevice);
      if (d.getDevice()->is2WaySlave() == true) {
        // do not render "slave" devices
        continue;
      }
      toJSON(d, _json);
    }
  } // toJSON(Set,Name)

  void toJSON(boost::shared_ptr<const Group> _group, JSONWriter& _json) {
    _json.startObject();
    _json.add("id", _group->getID());
    _json.add("name", _group->getName());
    _json.add("color", _group->getStandardGroupID());
    _json.add("isPresent", _group->isPresent());
    _json.add("isValid", _group->isValid());

    _json.startArray("devices");
    Set devices = _group->getDevices();
    for(int iDevice = 0; iDevice < devices.length(); iDevice++) {
      if (devices[iDevice].getDevice()->is2WaySlave()) {
        // do not render "slave" devices
        continue;
      }
      _json.add(dsuid2str(devices[iDevice].getDSID()));
    }
    _json.endArray();
    _json.endObject();
  } // toJSON(Group)

  void toJSON(boost::shared_ptr<const Cluster> _cluster, JSONWriter& _json) {
    _json.startObject();
    _json.add("id", _cluster->getID());
    _json.add("name", _cluster->getName());
    _json.add("color", _cluster->getStandardGroupID());
    _json.add("isPresent", _cluster->isPresent());
    _json.add("isValid", _cluster->isValid());
    _json.add("CardinalDirection", _cluster->getLocation());
    _json.add("ProtectionClass", _cluster->getProtectionClass());
    _json.add("isAutomatic", _cluster->isAutomatic());

    _json.startArray("devices");
    Set devices = _cluster->getDevices();
    for(int iDevice = 0; iDevice < devices.length(); iDevice++) {
      if (devices[iDevice].getDevice()->is2WaySlave()) {
        // do not render "slave" devices
        continue;
      }
      _json.add(dsuid2str(devices[iDevice].getDSID()));
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
