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
#include "src/model/apartment.h"
#include "src/vdc-db.h"
#include "src/protobufjson.h"
#include "src/messages/vdc-messages.pb.h"
#include "src/vdc-element-reader.h"
#include "src/vdc-connection.h"
#include "foreach.h"


namespace dss {
namespace vdcInfo {

static void addParameterDescriptions(VdcDb& db, const VdcDb::PropertyDesc& prop, JSONWriter& json) {
  switch (prop.typeId) {
    case VdcDb::propertyTypeId::integer:
    case VdcDb::propertyTypeId::numeric:
      json.add("type", prop.typePostfix.empty() ? "numeric" : prop.typePostfix);
      json.add("min", prop.minValue);
      json.add("max", prop.maxValue);
      json.add("resolution", prop.resolution);
      json.add("siunit", prop.siUnit);
      json.add("default", prop.defaultValue);
      break;
    case VdcDb::propertyTypeId::string:
      json.add("type", prop.typePostfix.empty() ? "string" : prop.typePostfix);
      json.add("default", prop.defaultValue);
      break;
    case VdcDb::propertyTypeId::enumeration:
      json.add("type", "enumeration");
      json.add("default", prop.defaultValue);
      //TODO: add option list
      break;
  }
}

static std::string convertHardwareGuid(const std::string &guid) {
  std::string hwid;
  if (beginsWith(guid, "macaddress:")) {
    hwid = "MAC " + guid.substr(11);
  } else if (beginsWith(guid, "enoceanaddress:")) {
    hwid = "EnOcean ID " + guid.substr(15);
  } else if (guid.find(':') != std::string::npos) {
    hwid = guid.substr(guid.find(':') + 1);
  } else {
    hwid = guid;
  }
  return hwid;
}

void addSpec(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json) {
  const std::string& dsDeviceGTIN = device.getOemEanAsString();
  auto dbspec = db.getSpec(dsDeviceGTIN, langCode);
  const auto& spec = device.getVdcSpec();

  std::map<std::string, std::string> specTable;
  specTable["name"] = device.getName();
  specTable["dsDeviceGTIN"] = dsDeviceGTIN;
  specTable["model"] = spec.model;
  specTable["modelVersion"] = spec.modelVersion;
  specTable["hardwareGuid"] = convertHardwareGuid(spec.hardwareGuid);
  specTable["hardwareModelGuid"] = spec.hardwareModelGuid;
  specTable["vendorId"] = spec.vendorId;
  specTable["vendorName"] = spec.vendorName;
  specTable["class"] = spec.deviceClass;
  specTable["classVersion"] = spec.deviceClassVersion;

  json.startObject("spec");
  foreach (auto &s, dbspec) {
    json.startObject(s.name);
    json.add("title", s.title);
    json.add("tags", s.tags);
    if (specTable.find(s.name) != specTable.end()) {
      json.add("value", specTable[s.name]);
    } else {
      json.add("value", s.value);
    }
    json.endObject();
  }
  json.endObject();
}

void addStateDescriptions(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json) {
  const std::string& oemEan = device.getOemEanAsString();
  auto states = db.getStates(oemEan, langCode);
  json.startObject("stateDescriptions");
  foreach (auto &state, states) {
    json.startObject(state.name);
    json.add("title", state.title);
    json.add("tags", state.tags);
    json.startObject("options");
    foreach (auto desc, state.values) {
      json.add(desc.first, desc.second); // non-translated: translated
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
    json.add("tags", prop.tags);
    addParameterDescriptions(db, prop, json);
    json.endObject();
  }
  json.endObject();
}

void addSensorDescriptions(VdcDb& db, const Device& device, const std::string& langCode, JSONWriter& json) {
  const std::string& oemEan = device.getOemEanAsString();
  auto sensors = db.getSensors(oemEan, langCode); // throws
  json.startObject("sensorDescriptions");
  foreach (auto &sensor, sensors) {
    json.startObject(sensor.prop.name);
    json.add("title", sensor.prop.title);
    json.add("tags", sensor.prop.tags);
    addParameterDescriptions(db, sensor.prop, json);
    json.add("dsIndex", strToInt(sensor.sensorIndex));
    try {
      json.add("dsType", static_cast<int> (device.getSensor(strToInt(sensor.sensorIndex))->m_sensorType));
    } catch (ItemNotFoundException& e) {}
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
      json.add("tags", p.tags);
      addParameterDescriptions(db, p, json);
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
    json.add("action", action.action_name);
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

void addOperationalValues(VdcDb& db, Device& device, const std::string& langCode, JSONWriter& json) {
  if (!device.isPresent() || !device.isVdcDevice()) {
    json.startObject("operational");
    json.startObject("states");
    json.endObject();
    json.startObject("properties");
    json.endObject();
    json.startObject("sensors");
    json.endObject();
    json.endObject();
    return;
  }

  google::protobuf::RepeatedPtrField<vdcapi::PropertyElement> query;
  query.Add()->set_name("deviceStates");
  query.Add()->set_name("deviceProperties");
  vdcapi::Message message = device.getVdcProperty(query);
  VdcElementReader reader(message.vdc_response_get_property().properties());

  const std::string& oemEan = device.getOemEanAsString();
  auto states = db.getStates(oemEan, langCode);
  auto props = db.getProperties(oemEan, langCode);
  auto sensors = db.getSensors(oemEan, langCode);

  json.startObject("operational");
  json.startObject("states");
  VdcElementReader deviceStatesReader = reader["deviceStates"];
  for (VdcElementReader::iterator it = deviceStatesReader.begin(); it != deviceStatesReader.end(); it++) {
    VdcElementReader stateReader = *it;
    std::string stateName = stateReader.getName();
    json.startObject(stateName);
    foreach (const auto &state, states) {
      if (state.name == stateName) {
        json.add("title", state.title);
        json.add("value");
        std::string propValue;
        if (stateReader["value"].hasValue()) {
          propValue = stateReader["value"].getValueAsString();
          json.add(propValue);
        } else {
          json.addNull();
        }
        foreach (const auto &desc, state.values) {
          if (desc.first == propValue) {
            json.add("displayValue", desc.second);
          }
        }
        break;
      }
    }
    json.endObject();
  }
  json.endObject();

  json.startObject("properties");
  VdcElementReader devicePropertiesReader = reader["deviceProperties"];
  for (VdcElementReader::iterator it = devicePropertiesReader.begin(); it != devicePropertiesReader.end(); it++) {
    VdcElementReader propReader = *it;
    std::string propName = propReader.getName();
    std::string propTitle = propName;
    json.startObject(propName);
    foreach (const auto &prop, props) {
      if (prop.name == propName) {
        propTitle = prop.title;
        break;
      }
    }
    json.add("title", propTitle);
    json.add("value");
    std::string propValue;
    if (propReader.hasValue()) {
      propValue = propReader.getValueAsString();
      json.add(propValue);
    } else {
      json.addNull();
    }
    json.endObject();
  }
  json.endObject();

  json.startObject("sensors");
  foreach (const auto &sensor, sensors) {
    DeviceSensorValue_t value;
    int sIndex = strToInt(sensor.sensorIndex);
    json.startObject(sensor.prop.name);
    json.add("title", sensor.prop.title);
    json.add("value");
    try {
      value = device.getDeviceSensorValueEx(sIndex);
      json.add(value.value);
    } catch(std::runtime_error& e) {
      json.addNull();
    }
    json.add("timestamp", value.timestamp.toISO8601_ms());
    json.endObject();
  }
  json.endObject();

  json.endObject();
}

Filter parseFilter(const std::string& filterParam) {
  Filter filter = {};
  if (filterParam.empty()) {
    filter.spec = 1;
    filter.stateDesc = 1;
    filter.eventDesc = 1;
    filter.propertyDesc = 1;
    filter.sensorDesc = 1;
    filter.actionDesc = 1;
    filter.stdActions = 1;
    filter.customActions = 1;
    filter.operational = 1;
    return filter;
  }
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
    } else if (item == "sensorDesc") {
      filter.sensorDesc = 1;
    } else if (item == "actionDesc") {
      filter.actionDesc = 1;
    } else if (item == "standardActions") {
      filter.stdActions = 1;
    } else if (item == "customActions") {
      filter.customActions = 1;
    } else if (item == "operational") {
      filter.operational = 1;
    }
  }
  return filter;
}

void addByFilter(VdcDb& db, Device& device, Filter filter,
                   const std::string& langCode, JSONWriter& json) {
  if (filter.spec) {
    addSpec(db, device, langCode, json);
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
  if (filter.sensorDesc) {
    addSensorDescriptions(db, device, langCode, json);
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
  if (filter.operational) {
    addOperationalValues(db, device, langCode, json);
  }
}

} // namespace vdcInfo
} // namespace

