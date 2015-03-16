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


#include "src/propertyquery.h"
#include "web/webrequests.h"

#include <cassert>
#include <iostream>

#include "src/propertysystem.h"

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
      std::vector<std::string> properties = extractPropertyList(part);
      m_PartList.push_back(part_t(part, properties));
    }
    m_PartList.erase(m_PartList.begin());
  } // parseParts

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

  void PropertyQuery::addProperties(part_t& _part, JSONWriter& json, dss::PropertyNodePtr _node) {
    foreach(std::string subprop, _part.properties) {
      log(std::string(__func__) + " from node: <" + _node->getName() + "> filter: " + subprop, lsDebug);
      if(subprop == "*") {
        for(int iChild = 0; iChild < _node->getChildCount(); iChild++) {
          PropertyNodePtr childNode = _node->getChild(iChild);
          if (childNode != NULL) {
            addProperty(json, childNode);
          }
        }
      } else {
        PropertyNodePtr node = _node->getPropertyByName(subprop);
        if (node != NULL) {
          addProperty(json, node);
        }
      }
    }
    return;
  } // addProperties

  /**
   * Handles one level of the query
   *
   * query1 = ../part(property1,property2)/...
   * qeury2 = ../part/...
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
    part_t& part = m_PartList[_partIndex];
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
   * qeury2 = ../part/...
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
    part_t& part = m_PartList[_partIndex];
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

} // namespace dss
