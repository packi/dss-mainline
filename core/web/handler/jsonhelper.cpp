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

#include "core/foreach.h"

#include "core/web/json.h"

#include "core/datetools.h"

#include "core/model/devicereference.h"
#include "core/model/device.h"
#include "core/model/set.h"
#include "core/model/group.h"
#include "core/model/zone.h"
#include "core/model/apartment.h"

namespace dss {

  boost::shared_ptr<JSONObject> toJSON(const DeviceReference& _device) {
    DateTime tmp_date;

    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("id", _device.getDSID().toString());
    result->addProperty("isSwitch", _device.hasSwitch());
    result->addProperty("name", _device.getName());
    result->addProperty("functionID", _device.getFunctionID());
    result->addProperty("productRevision", _device.getDevice()->getRevisionID());
    result->addProperty("productID", _device.getDevice()->getProductID());
    result->addProperty("meterDSID", _device.getDevice()->getDSMeterDSID().toString());
    result->addProperty("busID", _device.getDevice()->getShortAddress());
    result->addProperty("isPresent", _device.getDevice()->isPresent());

    tmp_date = _device.getDevice()->getLastDiscovered();
    result->addProperty("lastDiscovered", tmp_date.fromUTC());

    tmp_date = _device.getDevice()->getFirstSeen();
    result->addProperty("firstSeen", tmp_date.fromUTC());

    result->addProperty("on", _device.getDevice()->isOn());
    result->addProperty("locked", _device.getDevice()->getIsLockedInDSM());
    return result;
  } // toJSON(DeviceReference)

  boost::shared_ptr<JSONArrayBase> toJSON(const Set& _set) {
    boost::shared_ptr<JSONArrayBase> result(new JSONArrayBase());

    for(int iDevice = 0; iDevice < _set.length(); iDevice++) {
      const DeviceReference& d = _set.get(iDevice);
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
      devicesArr->add(devices[iDevice].getDSID().toString());
    }
    return result;
  } // toJSON(Group)

  boost::shared_ptr<JSONObject> toJSON(Zone& _zone, bool _includeDevices) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("id", _zone.getID());
    result->addProperty("name", _zone.getName());
    result->addProperty("isPresent", _zone.isPresent());
    if(_zone.getFirstZoneOnDSMeter() != NullDSID) {
      result->addProperty("firstZoneOnDSMeter", _zone.getFirstZoneOnDSMeter().toString());
    }

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
