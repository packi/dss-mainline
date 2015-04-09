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

#include "dss.h"
#include "model/apartment.h"
#include "model/device.h"

#include "src/propertyquery.h"
#include "web/webrequests.h"

#include <cassert>
#include <iostream>

#include "src/propertysystem.h"
#include "src/protobufjson.h"

#include "src/foreach.h"
#include "src/base.h"

namespace dss {

  __DEFINE_LOG_CHANNEL__(PropertyQuery, lsInfo);

  std::vector<std::string> extractPropertyList(std::string& _part) {
    std::size_t bracketPos = _part.find('(');
    if(bracketPos != std::string::npos) {
      std::size_t bracketEndPos = _part.find(')');
      std::string propListStr = _part.substr(bracketPos + 1, bracketEndPos - bracketPos - 1);
      _part.erase(bracketPos, _part.size() - bracketPos);
      return splitString(propListStr, ',', true);
    }
    return std::vector<std::string>();
  }

  void PropertyQuery::parseParts() {
    std::vector<std::string> parts = splitString(m_Query, '/');
    foreach(std::string part, parts) {
      // if loop is run more than once: elements are on the same level.
      // They must not be considered as children
      bool child = true;
      do {
        std::string key;
        std::vector<std::string> properties = evalPropertyList(part, key);
        std::vector<KeyValueContainer> container = splitKeyValue(properties);
        m_PartList.push_back(PropertyContainer(key, container, child));
        // if loop is repeated -> the next PropertyContainer is not a child.
        // the PropertyContainer is on the same level.
        child = false;
      } while(part.size());
    }
    m_PartList.erase(m_PartList.begin());
  } // parseParts

  std::vector<KeyValueContainer> PropertyQuery::splitKeyValue(std::vector<std::string> input) {
    std::vector<KeyValueContainer> output;
    foreach (std::string& item, input) {
      std::vector<std::string> data  = splitString(item, '=', true);
      if (data.size() >= 2) {
        output.push_back(KeyValueContainer(data.at(0), data.at(1)));
      } else {
        output.push_back(KeyValueContainer(data.at(0)));
      }
    }
    return output;
  }

  // _input: Key1(valname1=value1,valname2=value2),Key2,Key3(valname1=value1)
  std::vector<std::string> PropertyQuery::evalPropertyList(std::string& _input, std::string& _output_key) {
    std::size_t colonPos   = _input.find(',');
    std::size_t bracketPos = _input.find('(');
    // Key without values
    if ((colonPos != std::string::npos) &&
        (colonPos < bracketPos)) {
        _output_key = _input.substr (0, colonPos);
        _input.erase(0, ++colonPos);
    } else {
      // assign key from value up to bracket position or end of string
      _output_key = _input.substr (0, bracketPos);

      if(bracketPos != std::string::npos) {
        std::size_t bracketEndPos = _input.find(')');
        if (bracketEndPos == std::string::npos) {
          throw std::runtime_error("illegal query: missing ')'");
        }
        std::string propListStr = _input.substr(bracketPos + 1, bracketEndPos - bracketPos - 1);
        std::size_t eraseEnd = _input.find(',',bracketEndPos);
        if (eraseEnd == std::string::npos) {
            eraseEnd = bracketEndPos + 1; // ')' remove inclusive
        } else {
            ++eraseEnd; // ',' remove inclusive
        }
        _input.erase(0, eraseEnd);
        return splitString(propListStr, ',', true);
      } else {
        _input.erase();
      }
    }
    return std::vector<std::string>();
  } // evalPropertyList

  void PropertyQuery::addProperty(JSONWriter& json,
                                  PropertyNodePtr node) {
    if (node->getValueType() == vTypeNone) {
      /* ignore container type */
      return;
    }
    log(std::string(__func__) + " " + node->getName() + ": " + node->getAsString(), lsDebug);
    switch (node->getValueType()) {
    case vTypeInteger:
      json.add(node->getName(), node->getIntegerValue());
      break;
    case vTypeFloating:
      json.add(node->getName(), node->getFloatingValue());
      break;
    case vTypeBoolean:
      json.add(node->getName(), node->getBoolValue());
      break;
    case vTypeNone:
      break;
    case vTypeString:
    default:
      json.add(node->getName(), node->getAsString());
      break;
    }
    return;
  }

  void PropertyQuery::addProperties(PropertyContainer& _part, JSONWriter& json, dss::PropertyNodePtr _node) {
    foreach(KeyValueContainer& subprop, _part.properties) {
      log(std::string(__func__) + " from node: <" + _node->getName() + "> filter: " + subprop.name, lsDebug);
      if(subprop.name == "*") {
        for(int iChild = 0; iChild < _node->getChildCount(); iChild++) {
          PropertyNodePtr childNode = _node->getChild(iChild);
          if (childNode != NULL) {
            addProperty(json, childNode);
          }
        }
      } else {
        PropertyNodePtr node = _node->getPropertyByName(subprop.name);
        if (node != NULL) {
          addProperty(json, node);
        }
      }
    }
  } // addProperties

  /**
   * Handles one level of the query
   *
   * query1 = ../part(property1,property2)/...
   * query2 = ../part/...
   *
   * property is what we are extracting, part is what needs to match
   * query2 will extract nothing at this level
   *
   * returned json:
   *    {"prop1":val, "prop2": val, "part":[{},{}]}
   *
   * @see also runFor2
   */
  void PropertyQuery::runFor(PropertyNodePtr _parentNode,
                             unsigned int _partIndex,
                             JSONWriter& json) {

    log(std::string(__func__) + " Level" + intToString(_partIndex) + " : " +
        m_PartList[_partIndex].name, lsDebug);

    assert(_partIndex < m_PartList.size());
    PropertyContainer& part = m_PartList[_partIndex];
    bool hasSubpart = m_PartList.size() > (_partIndex + 1);

    if (!part.properties.empty()) {
      /* add current node */
      std::string name = (part.name == "*") ? _parentNode->getName() : part.name;
      log(std::string(__func__) + "   addElement " + name, lsDebug);
      json.startArray(name);
    }

    for (int iChild = 0; iChild < _parentNode->getChildCount(); iChild++) {
      PropertyNodePtr childNode = _parentNode->getChild(iChild);
      if ((part.name == "*") || (childNode->getName() == part.name)) {

        if (!part.properties.empty()) {
          json.startObject();
          addProperties(part, json, childNode);
        }

        if (hasSubpart) {
          runFor(childNode, _partIndex + 1, json);
        }
        if (!part.properties.empty()) {
          json.endObject();
        }
      }
    }

    if (!part.properties.empty()) {
      json.endArray();
    }
  } // runFor

  /**
   * Handles one level of the query
   *
   * query1 = ../part(property1,property2)/...
   * query2 = ../part/...
   *
   * property is what we are extracting, part is what needs to match
   * query2 will extract nothing at this level
   *
   * Similar to runFor but easier parsable json return
   * json:
   *    "part" : { "prop1" : val, "prop2" : val }
   *
   */
  void PropertyQuery::runFor2(PropertyNodePtr _parentNode,
                             unsigned int _partIndex,
                             JSONWriter& json) {

    log(std::string(__func__) + " Level" + intToString(_partIndex) + " : " +
        "node: <" + _parentNode->getName() + "> filter: " +
        m_PartList[_partIndex].name, lsDebug);

    assert(_partIndex < m_PartList.size());
    PropertyContainer& part = m_PartList[_partIndex];
    bool hasSubpart = m_PartList.size() > (_partIndex + 1);

    for (int iChild = 0; iChild < _parentNode->getChildCount(); iChild++) {
      PropertyNodePtr childNode = _parentNode->getChild(iChild);

      if (((part.name == "*") || (childNode->getName() == part.name)) &&
          (childNode->getValueType() == vTypeNone)) {
        /* none means node is not value node, but container */

        if (!part.properties.empty()) {
          json.startObject(childNode->getName());
          addProperties(part, json, childNode);
        }

        if (hasSubpart) {
          runFor2(childNode, _partIndex + 1, json);
        }

        if (!part.properties.empty()) {
          json.endObject();
        }
      }
    }
  } // runFor2

  void PropertyQuery::run(JSONWriter& json) {
    if(beginsWith(m_Query, m_pProperty->getName())) {
      runFor(m_pProperty, 0, json);
    }
    return;
  } // run

  void PropertyQuery::run2(JSONWriter& json) {
    if(beginsWith(m_Query, m_pProperty->getName())) {
      runFor2(m_pProperty, 0, json);
    }
    return;
  } // run2

  void PropertyQuery::vdcquery(JSONWriter& json) {
    if(beginsWith(m_Query, m_pProperty->getName())) {

      PropertyContainerToProtobuf::ProtoData data = PropertyContainerToProtobuf::convertPropertyContainerToProtobuf(m_PartList);

      uint8_t buffer_in [4096];
      uint8_t buffer_out[4096];
      uint16_t bs;

      if (DSS::hasInstance()) {
        memset(buffer_in,  0, sizeof(buffer_in));
        memset(buffer_out, 0, sizeof(buffer_out));
        if (!data.message.SerializeToArray(buffer_in, sizeof(buffer_in))) {
          throw std::runtime_error("could not serialize message");
        }
        boost::shared_ptr<Device> device = DSS::getInstance()->getApartment().getDeviceByDSID(data.deviceDsuid);
        DSS::getInstance()->getApartment().getBusInterface()->getStructureQueryBusInterface()->protobufMessageRequest(
            device->getDSMeterDSID(), data.message.ByteSize(), buffer_in, &bs, buffer_out);
      } else {
        return;
      }

      data.message.Clear();
      if (bs > sizeof(buffer_out)) {
        throw std::runtime_error("incoming message too large, dropping");
      }

      if (!data.message.ParseFromArray(buffer_out, bs)) {
        throw std::runtime_error("could not parse response message");
      }

      if (!ProtobufToJSon::protoPropertyToJson(data.message, json)) {
        throw std::runtime_error("could not parse response message");
      }
    }
  } // vdcquery;

} // namespace dss
