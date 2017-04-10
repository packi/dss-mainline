/*
    Copyright (c) 2015 digitalSTROM.org, Zurich, Switzerland

    Author: Remy Mahler, remy.mahler@digitalstrom.com

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

#include "protobufjson.h"
#include "base.h"
#include "foreach.h"
#include "logger.h"
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string.hpp>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <json.h>
#include "ds485types.h"

using namespace google::protobuf;

namespace dss {

  void ProtobufToJSon::processGenericResponse(const ::vdcapi::GenericResponse& response, JSONWriter& writer) {
    writer.startArray("response");
    writer.startObject();
    writer.add("code",response.code());
    writer.endObject();
    if (response.has_description()) {
      writer.startObject();
      writer.add("message",response.description());
      writer.endObject();
    }
    writer.endArray();
  }

  void ProtobufToJSon::processElements(const RepeatedPtrField< ::vdcapi::PropertyElement >& _elements,
                                       JSONWriter& _writer) {
    _writer.startArray("elements");
    for (int i = 0; i < _elements.size(); ++i) {
      const ::vdcapi::PropertyElement& tempElement = _elements.Get(i);

      if (tempElement.has_value()) {

        _writer.startObject();
        const vdcapi::PropertyValue& value = tempElement.value();

        if (tempElement.name().size() > 0) {
          _writer.add("name", tempElement.name().c_str());
        }
        if (value.has_v_bool()) {
          _writer.add("value", value.v_bool());
          _writer.add("type", "boolean");
        } else if (value.has_v_double()) {
          _writer.add("value", value.v_double());
          _writer.add("type", "double");
        } else if (value.has_v_int64()) {
          _writer.add("value", (int)value.v_int64());
          _writer.add("type", "int");
        } else if (value.has_v_uint64()) {
          _writer.add("value", (long long int)value.v_uint64());
          _writer.add("type", "uint");
        } else if (value.has_v_string()) {
          _writer.add("value", value.v_string().c_str());
          _writer.add("type", "string");
        }
        _writer.endObject();

        if (tempElement.elements_size() > 0) {
          processElements(tempElement.elements(), _writer);
        }

      } else {
        _writer.startObject();
        _writer.add("name");
        _writer.add(tempElement.name().c_str());
        processElements(tempElement.elements(), _writer);
        _writer.endObject();
      }
    }
    _writer.endArray();
  }

  void ProtobufToJSon::protoPropertyToJson(const vdcapi::Message& _message, JSONWriter& _writer) {
    switch (_message.type()) {
    case vdcapi::VDSM_REQUEST_GET_PROPERTY:
      _writer.add("type", "RequestGetProperty");
      processElements(_message.vdsm_request_get_property().query(), _writer);
      break;
    case vdcapi::VDC_RESPONSE_GET_PROPERTY:
      _writer.add("type", "ResponseGetProperty");
      processElements(_message.vdc_response_get_property().properties(), _writer);
      break;
    case vdcapi::VDSM_REQUEST_SET_PROPERTY:
      _writer.add("type", "RequestSetProperty");
      processElements(_message.vdsm_request_set_property().properties(), _writer);
      break;
    case vdcapi::GENERIC_RESPONSE:
      _writer.add("type", "GenericResponse");
      processGenericResponse(_message.generic_response(), _writer);
      break;
    default:
      Logger::getInstance()->log("can not process protobuf message of type: " + intToString(_message.type()), lsWarning);
      throw std::runtime_error("could not parse response message");
    }
  }

  void PropertyContainerToProtobuf::assignValue(vdcapi::PropertyElement* _element, std::string& _value) {
    if (_value.compare("true") == 0) { // boolean
      vdcapi::PropertyValue* valueElement = _element->mutable_value();
      valueElement->set_v_bool(true);
    } else if (_value.compare("false") == 0) { // boolean
      vdcapi::PropertyValue* valueElement = _element->mutable_value();
      valueElement->set_v_bool(false);
    }  else  if (_value.find('\"') != std::string::npos) { // string
      vdcapi::PropertyValue* valueElement = _element->mutable_value();
      _value.erase (std::remove(_value.begin(), _value.end(), '\"'), _value.end());
      valueElement->set_v_string(_value);
    } else if (_value.find('i') != std::string::npos) { // signed
      vdcapi::PropertyValue* valueElement = _element->mutable_value();
      _value.erase (std::remove(_value.begin(), _value.end(), 'i'), _value.end());
      valueElement->set_v_int64(strToInt(_value));
    } else if (_value.find('u') != std::string::npos) { // unsigned
      vdcapi::PropertyValue* valueElement = _element->mutable_value();
      _value.erase (std::remove(_value.begin(), _value.end(), 'u'), _value.end());
      valueElement->set_v_uint64(strToUInt(_value));
    } else if (_value.find('f') != std::string::npos) { // double
      vdcapi::PropertyValue* valueElement = _element->mutable_value();
      _value.erase (std::remove(_value.begin(), _value.end(), 'f'), _value.end());
      valueElement->set_v_double(strToDouble(_value));
    } else {
      Logger::getInstance()->log("can not process value: " + _value, lsWarning);
    }
  }

  void PropertyContainerToProtobuf::setupMessage(vdcapi::PropertyElement* _rootProp, std::vector<PropertyContainer> _parts) {
    if (_parts.size() > 0) {
      vdcapi::PropertyElement* newProp = _rootProp->add_elements();
      newProp->set_name(_parts.at(0).name);
      if (_parts.at(0).properties.size() > 0) {
        foreach (KeyValueContainer& container, _parts.at(0).properties) {
          vdcapi::PropertyElement* element = newProp->add_elements();
          element->set_name(container.name);
          assignValue(element, container.value);
        }
      }
      _parts.erase(_parts.begin());

      if ((_parts.size() > 0) &&
          (_parts.at(0).child)) {
        setupMessage(newProp, _parts);
      } else {
        setupMessage(_rootProp, _parts);
      }
    }
  }

  void PropertyContainerToProtobuf::startSetupMessage(vdcapi::vdsm_RequestSetProperty* _prop,
                                                      std::vector<PropertyContainer> _parts) {
    if (_parts.size() > 0) {
      vdcapi::PropertyElement* rootProp = NULL;

      if (_parts.at(0).name.empty()) {
        if (_parts.at(0).properties.size() > 0) {
          foreach (KeyValueContainer& container, _parts.at(0).properties) {
            vdcapi::PropertyElement* element = _prop->add_properties();
            element->set_name(container.name);
            assignValue(element, container.value);
          }
        }
      } else {
        rootProp = _prop->add_properties();
        rootProp->set_name(_parts.at(0).name);
        foreach (KeyValueContainer& container, _parts.at(0).properties) {
          vdcapi::PropertyElement* element = rootProp->add_elements();
          element->set_name(container.name);
          assignValue(element, container.value);
        }
      }
      _parts.erase(_parts.begin());
      if (_parts.size() > 0) {
        if (_parts.at(0).child) {
          rootProp = _prop->add_properties();
          setupMessage(rootProp, _parts);
        } else {
          startSetupMessage(_prop, _parts);
        }
      }
    }
  }

  void PropertyContainerToProtobuf::startSetupMessage(vdcapi::vdsm_RequestGetProperty* _prop,
                                                      std::vector<PropertyContainer> _parts) {
    if (_parts.size() > 0) {
      vdcapi::PropertyElement* rootProp = NULL;

      if (_parts.at(0).name.empty()) {
        if (_parts.at(0).properties.size() > 0) {
          foreach (KeyValueContainer& container, _parts.at(0).properties) {
            vdcapi::PropertyElement* element = _prop->add_query();
            element->set_name(container.name);
            assignValue(element, container.value);
          }
        }
      } else {
        rootProp = _prop->add_query();
        rootProp->set_name(_parts.at(0).name);
        foreach (KeyValueContainer& container, _parts.at(0).properties) {
          vdcapi::PropertyElement* element = rootProp->add_elements();
          element->set_name(container.name);
          assignValue(element, container.value);
        }
      }
      _parts.erase(_parts.begin());
      if (_parts.size() > 0) {
        if (_parts.at(0).child) {
          rootProp = _prop->add_query();
          setupMessage(rootProp, _parts);
        } else {
          startSetupMessage(_prop, _parts);
        }
      }
    }
  }

  vdcapi::Message PropertyContainerToProtobuf::propertyContainerToProtobuf(const bool _bSetProperty,
      const std::vector<PropertyContainer>& _parts, const dsuid_t& _deviceDsuid) {
    vdcapi::Message message;
    if (_bSetProperty) {
      message.set_type(vdcapi::VDSM_REQUEST_SET_PROPERTY);
      vdcapi::vdsm_RequestSetProperty* setprop =
          message.mutable_vdsm_request_set_property();
      setprop->set_dsuid(dsuid2str(_deviceDsuid));
      startSetupMessage(setprop, _parts);
    } else {
      message.set_type(vdcapi::VDSM_REQUEST_GET_PROPERTY);
      vdcapi::vdsm_RequestGetProperty* getprop =
          message.mutable_vdsm_request_get_property();
      getprop->set_dsuid(dsuid2str(_deviceDsuid));
      startSetupMessage(getprop, _parts);
    }
    return message;
  }

  PropertyContainerToProtobuf::ProtoData PropertyContainerToProtobuf::convertPropertyContainerToProtobuf(std::vector<PropertyContainer> _parts) {

    ProtoData data;
    // assumpition:
    // first element: setProperty or getProperty
    // parameter of first element deviceId = dsuid_of_vdc_device
    if (_parts.size() < 1) {
      throw std::runtime_error("can not convert property container to protobuf. Not enough items.");
    }

    if (_parts.at(0).name.compare("setProperty") == 0) {
      data.setProperty = true;
    } else if (_parts.at(0).name.compare("getProperty") == 0) {
      data.setProperty = false;
    } else {
      throw std::runtime_error("can not convert property container to protobuf. Set or Get property not given");
    }

    if (_parts.at(0).properties.size() > 0) {
      data.deviceDsuid = str2dsuid(_parts.at(0).properties.at(0).value); // can throw
    } else {
      throw std::runtime_error("can not get device Id");
    }
    _parts.erase(_parts.begin());
    data.message = propertyContainerToProtobuf(data.setProperty, _parts, data.deviceDsuid);
    return data;
  }

  void ProtobufToJSon::processElementsPretty(const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& _elements,
        JSONWriter& _writer) {
    bool isObject = _elements.size() == 0 || !_elements.Get(0).name().empty();
    if (isObject) {
      _writer.startObject();
    } else {
      _writer.startArray();
    }
    for (int i = 0; i < _elements.size(); ++i) {
      const ::vdcapi::PropertyElement& tempElement = _elements.Get(i);
      if (isObject) {
        _writer.add(tempElement.name().c_str());
      }

      if (tempElement.has_value()) {
        const vdcapi::PropertyValue& value = tempElement.value();
        if (value.has_v_bool()) {
          _writer.add(value.v_bool());
        } else if (value.has_v_double()) {
          _writer.add(value.v_double());
        } else if (value.has_v_int64()) {
          _writer.add((long long) value.v_int64());
        } else if (value.has_v_uint64()) {
          _writer.add((unsigned long long) value.v_uint64());
        } else if (value.has_v_string()) {
          _writer.add(value.v_string().c_str());
        } else {
          _writer.addNull();
        }
      } else if (tempElement.elements_size() == 0) {
        _writer.addNull(); // should it be null, [] or {}?
      } else {
        processElementsPretty(tempElement.elements(), _writer);
      }
    }
    if (isObject) {
      _writer.endObject();
    } else {
      _writer.endArray();
    }
  }

  static vdcapi::PropertyElement jsonToElementRecursive(json_object* jsonObject) {
    vdcapi::PropertyElement vdcElement;
    switch (json_object_get_type(jsonObject)) {
      case json_type_null:
        break;
      case json_type_boolean:
        vdcElement.mutable_value()->set_v_bool(json_object_get_boolean(jsonObject));
        break;
      case json_type_double:
        vdcElement.mutable_value()->set_v_double(json_object_get_double(jsonObject));
        break;
      case json_type_int:
        vdcElement.mutable_value()->set_v_int64(json_object_get_int64(jsonObject));
        break;
      case json_type_string:
        vdcElement.mutable_value()->set_v_string(json_object_get_string(jsonObject));
        break;
      case json_type_object: {
        json_object_object_foreach(jsonObject, key, value) {
          *vdcElement.add_elements() = jsonToElementRecursive(value);
          vdcElement.mutable_elements()->Mutable(vdcElement.elements_size() - 1)->set_name(key);
        }
        break;
      }
      case json_type_array:
        for (int i = 0; i < json_object_array_length(jsonObject); i++) {
          *vdcElement.add_elements() = jsonToElementRecursive(json_object_array_get_idx(jsonObject, i));
        }
        break;
    }
    return vdcElement;
  }

  vdcapi::PropertyElement ProtobufToJSon::jsonToElement(const std::string& jsonText) {
    struct json_tokener* tok = json_tokener_new();
    BOOST_SCOPE_EXIT(tok) {
      json_tokener_free(tok);
    } BOOST_SCOPE_EXIT_END
    json_object* jsonObject = json_tokener_parse_ex(tok, jsonText.c_str(), -1);
    if (!jsonObject) {
      throw std::runtime_error(json_tokener_error_desc(json_tokener_get_error(tok)));
    }
    BOOST_SCOPE_EXIT(jsonObject) {
      json_object_put(jsonObject);
    } BOOST_SCOPE_EXIT_END

    return jsonToElementRecursive(jsonObject);
  }


} //namespace

