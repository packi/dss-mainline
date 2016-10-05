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

#include "src/web/webrequests.h"
#include "src/model/device.h"
#include "src/vdc-db.h"
#include "src/protobufjson.h"
#include "src/messages/vdc-messages.pb.h"
#include "src/vdc-element-reader.h"
#include "src/vdc-connection.h"
#include "foreach.h"
#include "vdchelper.h"


namespace dss {

void GetVdcSpec(const Device& device, JSONWriter& json) {
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

void GetVdcStateDescriptions(const Device& device, const std::string& langCode, JSONWriter& json) {
  VdcDb db;
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

void GetVdcPropertyDescriptions(const Device& device, const std::string& langCode, JSONWriter& json) {
  VdcDb db;
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

void GetVdcActionDescriptions(const Device& device, const std::string& langCode, JSONWriter& json) {
  VdcDb db;
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

void GetVdcStandardActions(const Device& device, const std::string& langCode, JSONWriter& json) {
  VdcDb db;
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

void GetVdcCustomActions(Device& device, JSONWriter& json) {
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

std::bitset<6> ParseVdcInfoFilter(const std::string& filterParam) {
  std::bitset<6> filter;
  std::vector<std::string> render = dss::splitString(filterParam, ',');
  for (size_t i = 0; i < render.size(); i++) {
    if (render.at(i) == "spec") {
      filter.set(VdcInfoFilterSpec);
    } else if (render.at(i) == "stateDesc") {
      filter.set(VdcInfoFilterStateDesc);
    } else if (render.at(i) == "propertyDesc") {
      filter.set(VdcInfoFilterPropertyDesc);
    } else if (render.at(i) == "actionDesc") {
      filter.set(VdcInfoFilterActionDesc);
    } else if (render.at(i) == "standardActions") {
      filter.set(VdcInfoFilterStdActions);
    } else if (render.at(i) == "customActions") {
      filter.set(VdcInfoFilterCustomActions);
    }
  }
  return filter;
}

void RenderVdcInfo(Device& device, const std::bitset<6>& filter,
                   const std::string& langCode, JSONWriter& json) {
  if (filter.test(VdcInfoFilterSpec)) {
    GetVdcSpec(device, json);
  }
  if (filter.test(VdcInfoFilterStateDesc)) {
    GetVdcStateDescriptions(device, langCode, json);
  }
  if (filter.test(VdcInfoFilterPropertyDesc)) {
    GetVdcPropertyDescriptions(device, langCode, json);
  }
  if (filter.test(VdcInfoFilterActionDesc)) {
    GetVdcActionDescriptions(device, langCode, json);
  }
  if (filter.test(VdcInfoFilterStdActions)) {
    GetVdcStandardActions(device, langCode, json);
  }
  if (filter.test(VdcInfoFilterCustomActions)) {
    GetVdcCustomActions(device, json);
  }
}

} // namespace

