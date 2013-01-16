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

namespace dss {

  boost::shared_ptr<JSONObject> toJSON(const DeviceReference& _device) {
    DateTime tmp_date;

    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("id", _device.getDSID().toString());
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
    if(_device.getDevice()->isPresent()) {
      result->addProperty("meterDSID", _device.getDevice()->getDSMeterDSID().toString());
      result->addProperty("busID", _device.getDevice()->getShortAddress());
    } else {
      result->addProperty("meterDSID", _device.getDevice()->getLastKnownDSMeterDSID().toString());
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
    result->addProperty("outputMode", _device.getDevice()->getOutputMode());
    result->addProperty("buttonID", _device.getDevice()->getButtonID());
    result->addProperty("buttonActiveGroup", _device.getDevice()->getButtonActiveGroup());
    result->addProperty("buttonInputMode", _device.getDevice()->getButtonInputMode());
    result->addProperty("buttonInputIndex", _device.getDevice()->getButtonInputIndex());
    result->addProperty("buttonInputCount", _device.getDevice()->getButtonInputCount());

    boost::shared_ptr<JSONArray<std::string> > groupsArr(new JSONArray<std::string>());
    result->addElement("groups", groupsArr);
    std::bitset<63> deviceGroups = _device.getDevice()->getGroupBitmask();
    for (int g = 1; g <= 63; g++) {
      if (deviceGroups.test(g-1)) {
        groupsArr->add(intToString(g));
      }
    }

    boost::shared_ptr<JSONArrayBase> binaryInputArr(new JSONArrayBase());
    result->addElement("binaryInputs", binaryInputArr);
    const std::vector<boost::shared_ptr<DeviceBinaryInput_t> > binaryInputs = _device.getDevice()->getBinaryInputs();
    for (std::vector<boost::shared_ptr<DeviceBinaryInput_t> >::const_iterator it = binaryInputs.begin(); it != binaryInputs.end(); ++it) {
      boost::shared_ptr<JSONObject> element(new JSONObject());
      element->addProperty("targetGroupType", (*it)->m_targetGroupType);
      element->addProperty("targetGroup", (*it)->m_targetGroupId);
      element->addProperty("inputType", (*it)->m_inputType);
      element->addProperty("inputId", (*it)->m_inputId);
      element->addProperty("state", _device.getDevice()->getBinaryInputState((*it)->m_inputIndex)->getState());
      binaryInputArr->addElement("", element);
    }

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
    result->addProperty("isPresent", _group->isPresent());

    boost::shared_ptr<JSONArray<std::string> > devicesArr(new JSONArray<std::string>());
    result->addElement("devices", devicesArr);
    Set devices = _group->getDevices();
    for(int iDevice = 0; iDevice < devices.length(); iDevice++) {
      if (devices[iDevice].getDevice()->is2WaySlave()) {
        // do not render "slave" devices
        continue;
      }
      devicesArr->add(devices[iDevice].getDSID().toString());
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
