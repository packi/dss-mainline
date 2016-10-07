/*
    Copyright (c) 2016 digitalSTROM AG, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#include "vdc-info.h"
#include "src/model/device.h"
#include "src/vdc-db.h"
#include "src/protobufjson.h"
#include "src/messages/vdc-messages.pb.h"
#include "src/vdc-element-reader.h"
#include "src/vdc-connection.h"
#include "foreach.h"


namespace dss {
namespace vdcInfo {

void addSpec(const Device& device, JSONWriter& json) {
  const std::string& oemEan = device.getOemEanAsString();
  const auto& spec = device.getVdcSpec();
  json.add("class", spec.deviceClass);
  json.add("classVersion", spec.deviceClassVersion);
  json.add("oemEanNumber", oemEan);
  json.add("model", spec.model);
  json.add("modelVersion", spec.modelVersion);
  json.add("hardwareGuid", spec.hardwareGuid);
  json.add("hardwareModelGuid", spec.hardwareModelGuid);
  json.add("vendorId", spec.vendorId);
  json.add("vendorName", spec.vendorName);
}

void addStateDescriptions(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json) {
  const std::string& oemEan = device.getOemEanAsString();
  auto states = db.getStates(oemEan, langCode);
  json.startObject("stateDescriptions");
  foreach (auto &state, states) {
    json.startObject(state.name);
    json.add("title", state.title);
    json.startObject("options");
    foreach (auto desc, state.values) {
      json.add(desc.first, desc.second); // non-tranlated: translated
    }
    json.endObject();
    json.endObject();
  }
  json.endObject();
}

void addEventDescriptions(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json) {
  const std::string& oemEan = device.getOemEanAsString();
  const auto& events = db.getEvents(oemEan, langCode);
  json.startObject("eventDescriptions");
  foreach (const auto& event, events) {
    json.startObject(event.name);
    json.add("title", event.title);
    json.endObject();
  }
  json.endObject();
}

void addPropertyDescriptions(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json) {
  const std::string& oemEan = device.getOemEanAsString();
  auto props = db.getProperties(oemEan, langCode); // throws
  json.startObject("propertyDescriptions");

  foreach (auto &prop, props) {
    json.startObject(prop.name);
    json.add("title", prop.title);
    json.add("readOnly", prop.readonly);
    json.endObject();
  }
  json.endObject();
}

void addActionDescriptions(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json) {
  const std::string& oemEan = device.getOemEanAsString();
  auto actions = db.getActions(oemEan, langCode);
  json.startObject("actionDescriptions");
  foreach (const VdcDb::ActionDesc &action, actions) {
    json.startObject(action.name);
    json.add("title", action.title);
    json.startObject("params");
    foreach (auto p, action.params) {
      json.startObject(p.name);
      json.add("title", p.title);
      json.add("default", p.defaultValue);
      json.endObject();
    }
    json.endObject();
    json.endObject();
  }
  json.endObject();
}

void addStandardActions(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json) {
  const std::string& oemEan = device.getOemEanAsString();
  auto stdActions = db.getStandardActions(oemEan, langCode);
  json.startObject("standardActions");
  foreach (auto &action, stdActions) {
    json.startObject(action.name);
    json.add("title", action.title);
    json.startObject("params");
    foreach (auto arg, action.args) {
      json.add(arg.first, arg.second);
    }
    json.endObject();
    json.endObject();
  }
  json.endObject();
}

void addCustomActions(Device& device, JSONWriter& json) {
  if (!device.isPresent() || !device.isVdcDevice()) {
    json.startObject("customActions");
    json.endObject();
    return;
  }

  google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
  query.Add()->set_name("customActions");
  vdcapi::Message message = device.getVdcProperty(query);
  VdcElementReader reader(message.vdc_response_get_property().properties());
  json.add("customActions");
  ProtobufToJSon::processElementsPretty(reader["customActions"].childElements(), json);
}

Filter parseFilter(const std::string& filterParam) {
  Filter filter = {};
  std::vector<std::string> items = dss::splitString(filterParam, ',');
  BOOST_FOREACH(const auto& item, items) {
    if (item == "spec") {
      filter.spec = 1;
    } else if (item == "stateDesc") {
      filter.stateDesc = 1;
    } else if (item == "eventDesc") {
      filter.eventDesc = 1;
    } else if (item == "propertyDesc") {
      filter.propertyDesc = 1;
    } else if (item == "actionDesc") {
      filter.actionDesc = 1;
    } else if (item == "standardActions") {
      filter.stdActions = 1;
    } else if (item == "customActions") {
      filter.customActions = 1;
    }
  }
  return filter;
}

void addByFilter(VdcDb& db, Device& device, Filter filter,
                   const std::string& langCode, JSONWriter& json) {
  if (filter.spec) {
    addSpec(device, json);
  }
  if (filter.stateDesc) {
    addStateDescriptions(db, device, langCode, json);
  }
  if (filter.eventDesc) {
    addEventDescriptions(db, device, langCode, json);
  }
  if (filter.propertyDesc) {
    addPropertyDescriptions(db, device, langCode, json);
  }
  if (filter.actionDesc) {
    addActionDescriptions(db, device, langCode, json);
  }
  if (filter.stdActions) {
    addStandardActions(db, device, langCode, json);
  }
  if (filter.customActions) {
    addCustomActions(device, json);
  }
}

} // namespace vdcInfo
} // namespace

