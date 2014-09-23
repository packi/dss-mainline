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

#include "jsonhelper.h"

#include "src/foreach.h"

#include "src/web/json.h"

#include "src/datetools.h"

#include "src/model/devicereference.h"
#include "src/model/device.h"
#include "src/model/set.h"
#include "src/model/group.h"
#include "src/model/zone.h"
#include "src/model/state.h"
#include "src/model/apartment.h"
#include "src/dss.h"
#include "src/model/modulator.h"

namespace dss {

  boost::shared_ptr<JSONObject> toJSON(const DeviceReference& _device) {
    DateTime tmp_date;

    boost::shared_ptr<JSONObject> result(new JSONObject());
    dsid_t dsid;
    if (dsuid_to_dsid(_device.getDSID(), &dsid)) {
      result->addProperty("id", dsid2str(dsid));
    } else {
      result->addProperty("id", dsuid2str(_device.getDSID()));
    }
    result->addProperty("dSUID", dsuid2str(_device.getDSID()));
    result->addProperty("GTIN", _device.getDevice()->getGTIN());
    result->addProperty("name", _device.getName());
    result->addProperty("functionID", _device.getFunctionID());
    result->addProperty("productRevision", _device.getDevice()->getRevisionID());
    result->addProperty("productID", _device.getDevice()->getProductID());
    result->addProperty("hwInfo", _device.getDevice()->getHWInfo());
    result->addProperty("OemStatus", _device.getDevice()->getOemStateAsString());
    result->addProperty("OemEanNumber", _device.getDevice()->getOemEanAsString());
    result->addProperty("OemSerialNumber", _device.getDevice()->getOemSerialNumber());
    result->addProperty("OemPartNumber", _device.getDevice()->getOemPartNumber());
    result->addProperty("OemProductInfoState", _device.getDevice()->getOemProductInfoStateAsString());
    result->addProperty("OemProductURL", _device.getDevice()->getOemProductURL());
    result->addProperty("OemInternetState", _device.getDevice()->getOemInetStateAsString());
    result->addProperty("OemIsIndependent", _device.getDevice()->getOemIsIndependent());
    result->addProperty("isVdcDevice", _device.getDevice()->isVdcDevice());
    result->addProperty("VdcModelGuid", _device.getDevice()->getVdcModelGuid());
    result->addProperty("VdcVendorGuid", _device.getDevice()->getVdcVendorGuid());
    result->addProperty("VdcOemGuid", _device.getDevice()->getVdcOemGuid());
    result->addProperty("VdcConfigURL", _device.getDevice()->getVdcConfigURL());
    result->addProperty("VdcHardwareGuid", _device.getDevice()->getVdcHardwareGuid());
    result->addProperty("VdcHardwareInfo", _device.getDevice()->getVdcHardwareInfo());
    result->addProperty("VdcHardwareVersion", _device.getDevice()->getVdcHardwareVersion());

    if(_device.getDevice()->isPresent()) {
      dsid_t dsid;
      if (dsuid_to_dsid(_device.getDevice()->getDSMeterDSID(), &dsid)) {
        result->addProperty("meterDSID", dsid2str(dsid));
      } else {
        result->addProperty("meterDSID", dsuid2str(_device.getDevice()->getDSMeterDSID()));
      }
      result->addProperty("meterDSUID", dsuid2str(_device.getDevice()->getDSMeterDSID()));
      std::string dSMName;
      try {
        dSMName = DSS::getInstance()->getApartment().getDSMeterByDSID(_device.getDevice()->getDSMeterDSID())->getName();
      } catch(std::runtime_error&) {
      }
      result->addProperty("meterName", dSMName);
      result->addProperty("busID", _device.getDevice()->getShortAddress());
    } else {
      dsid_t dsid;
      if (dsuid_to_dsid(_device.getDevice()->getLastKnownDSMeterDSID(), &dsid)) {
        result->addProperty("meterDSID", dsid2str(dsid));
      } else {
        result->addProperty("meterDSID", dsuid2str(_device.getDevice()->getLastKnownDSMeterDSID()));
      }
      result->addProperty("meterDSUID", dsuid2str(_device.getDevice()->getLastKnownDSMeterDSID()));
      std::string dSMName;
      try {
        dSMName = DSS::getInstance()->getApartment().getDSMeterByDSID(_device.getDevice()->getDSMeterDSID())->getName();
      } catch(std::runtime_error&) {
      }
      result->addProperty("meterName", dSMName);
      result->addProperty("busID", _device.getDevice()->getLastKnownShortAddress());
    }
    result->addProperty("zoneID", _device.getDevice()->getZoneID());
    result->addProperty("isPresent", _device.getDevice()->isPresent());
    result->addProperty("isValid", _device.getDevice()->isValid());

    tmp_date = _device.getDevice()->getLastDiscovered();
    result->addProperty("lastDiscovered", tmp_date);

    tmp_date = _device.getDevice()->getFirstSeen();
    result->addProperty("firstSeen", tmp_date);

    tmp_date = _device.getDevice()->getInactiveSince();
    result->addProperty("inactiveSince", tmp_date);

    result->addProperty("on", _device.getDevice()->isOn());
    result->addProperty("locked", _device.getDevice()->getIsLockedInDSM());
    result->addProperty("configurationLocked", _device.getDevice()->isConfigLocked());
    result->addProperty("outputMode", _device.getDevice()->getOutputMode());
    result->addProperty("buttonID", _device.getDevice()->getButtonID());
    result->addProperty("buttonActiveGroup", _device.getDevice()->getButtonActiveGroup());
    result->addProperty("buttonGroupMembership", _device.getDevice()->getButtonGroupMembership());
    result->addProperty("buttonInputMode", _device.getDevice()->getButtonInputMode());
    result->addProperty("buttonInputIndex", _device.getDevice()->getButtonInputIndex());
    result->addProperty("buttonInputCount", _device.getDevice()->getButtonInputCount());
    result->addProperty("AKMInputProperty", _device.getDevice()->getAKMInputProperty());

    boost::shared_ptr<JSONArray<int> > groupsArr(new JSONArray<int>());
    result->addElement("groups", groupsArr);
    std::bitset<63> deviceGroups = _device.getDevice()->getGroupBitmask();
    for (int g = 1; g <= 63; g++) {
      if (deviceGroups.test(g-1)) {
        groupsArr->add(g);
      }
    }

    boost::shared_ptr<JSONArrayBase> binaryInputArr(new JSONArrayBase());
    result->addElement("binaryInputs", binaryInputArr);
    const std::vector<boost::shared_ptr<DeviceBinaryInput_t> > binaryInputs = _device.getDevice()->getBinaryInputs();
    result->addProperty("binaryInputCount", (int)binaryInputs.size());
    result->addProperty("sensorInputCount", (int)(_device.getDevice()->getSensorCount()));
    for (std::vector<boost::shared_ptr<DeviceBinaryInput_t> >::const_iterator it = binaryInputs.begin(); it != binaryInputs.end(); ++it) {
      boost::shared_ptr<JSONObject> element(new JSONObject());
      element->addProperty("targetGroupType", (*it)->m_targetGroupType);
      element->addProperty("targetGroup", (*it)->m_targetGroupId);
      element->addProperty("inputType", (*it)->m_inputType);
      element->addProperty("inputId", (*it)->m_inputId);
      element->addProperty("state", _device.getDevice()->getBinaryInputState((*it)->m_inputIndex)->getState());
      binaryInputArr->addElement("", element);
    }

    // check if device has invalid sensor values
    uint8_t sensorCount = _device.getDevice()->getSensorCount();
    bool sensorFlag = true;
    for (uint8_t s = 0; s < sensorCount; s++) {
      if (!_device.getDevice()->isSensorDataValid(s)) {
        sensorFlag = false;
        break;
      }
    }
    result->addProperty("sensorDataValid", sensorFlag);
    return result;
  } // toJSON(DeviceReference)

  boost::shared_ptr<JSONArrayBase> toJSON(const Set& _set) {
    boost::shared_ptr<JSONArrayBase> result(new JSONArrayBase());

    for(int iDevice = 0; iDevice < _set.length(); iDevice++) {
      const DeviceReference& d = _set.get(iDevice);
      if (d.getDevice()->is2WaySlave() == true) {
        // do not render "slave" devices
        continue;
      }
      result->addElement("", toJSON(d));
    }
    return result;
  } // toJSON(Set,Name)

  boost::shared_ptr<JSONObject> toJSON(boost::shared_ptr<const Group> _group) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("id", _group->getID());
    result->addProperty("name", _group->getName());
    result->addProperty("color", _group->getStandardGroupID());
    result->addProperty("isPresent", _group->isPresent());
    result->addProperty("isValid", _group->isValid());

    boost::shared_ptr<JSONArray<std::string> > devicesArr(new JSONArray<std::string>());
    result->addElement("devices", devicesArr);
    Set devices = _group->getDevices();
    for(int iDevice = 0; iDevice < devices.length(); iDevice++) {
      if (devices[iDevice].getDevice()->is2WaySlave()) {
        // do not render "slave" devices
        continue;
      }
      devicesArr->add(dsuid2str(devices[iDevice].getDSID()));
    }
    return result;
  } // toJSON(Group)

  boost::shared_ptr<JSONObject> toJSON(Zone& _zone, bool _includeDevices) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("id", _zone.getID());
    result->addProperty("name", _zone.getName());
    result->addProperty("isPresent", _zone.isPresent());

    if(_includeDevices) {
      result->addElement("devices", toJSON(_zone.getDevices()));
      boost::shared_ptr<JSONArrayBase> groups(new JSONArrayBase());
      result->addElement("groups", groups);
      foreach(boost::shared_ptr<Group> pGroup, _zone.getGroups()) {
        groups->addElement("", toJSON(pGroup));
      }
    }

    return result;
  } // toJSON(Zone)

  boost::shared_ptr<JSONObject> toJSON(Apartment& _apartment) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    boost::shared_ptr<JSONObject> apartment(new JSONObject());
    result->addElement("apartment", apartment);
    boost::shared_ptr<JSONArrayBase> zonesArr(new JSONArrayBase());
    apartment->addElement("zones", zonesArr);

    std::vector<boost::shared_ptr<Zone> > zones = _apartment.getZones();
    foreach(boost::shared_ptr<Zone> pZone, zones) {
      zonesArr->addElement("", toJSON(*pZone));
    }
    return result;
  } // toJSON(Apartment)

}
