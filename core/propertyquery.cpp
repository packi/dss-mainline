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

#include "core/propertyquery.h"

#include <cassert>
#include <iostream>

#include "core/propertysystem.h"
#include "core/web/json.h"

#include "core/foreach.h"
#include "core/base.h"

namespace dss {


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

  boost::shared_ptr<JSONElement> PropertyQuery::addProperties(part_t& _part,
                                    boost::shared_ptr<JSONElement> _parentElement,
                                    dss::PropertyNodePtr _node) {
    boost::shared_ptr<JSONObject> obj(new JSONObject());
    foreach(std::string subprop, _part.properties) {
      PropertyNodePtr node = _node->getPropertyByName(subprop);
      if(node != NULL) {
        switch (node->getValueType()) {
          case vTypeInteger:
            obj->addProperty(subprop, node->getIntegerValue());
            break;
          case vTypeBoolean:
            obj->addProperty(subprop, node->getBoolValue());
            break;
          case vTypeNone:
          case vTypeString:
          default:
            obj->addProperty(subprop, node->getAsString());
            break;
        }
      }
    }
    _parentElement->addElement(_node->getName(), obj);
    return obj;
  } // addProperties

  void PropertyQuery::runFor(PropertyNodePtr _parentNode,
                             unsigned int _partIndex,
                             boost::shared_ptr<JSONElement> _parentElement) {
    assert(_partIndex < m_PartList.size());
    part_t& part = m_PartList[_partIndex];
    bool hasSubpart = m_PartList.size() > (_partIndex + 1);
    if(!part.properties.empty()) {
      boost::shared_ptr<JSONElement> container(new JSONArrayBase());
      if(part.name == "*") {
        _parentElement->addElement(_parentNode->getName(), container);
      } else {
        _parentElement->addElement(part.name, container);
      }
      for(int iChild = 0; iChild < _parentNode->getChildCount(); iChild++) {
        PropertyNodePtr childNode = _parentNode->getChild(iChild);
        if((part.name == "*") || (childNode->getName() == part.name)) {
          boost::shared_ptr<JSONElement> elem = addProperties(part, container, childNode);
          if(hasSubpart) {
            runFor(childNode, _partIndex + 1, elem);
          }
        }
      }
    } else if(hasSubpart) {
      for(int iChild = 0; iChild < _parentNode->getChildCount(); iChild++) {
        PropertyNodePtr childNode = _parentNode->getChild(iChild);
        if((part.name == "*") || (childNode->getName() == part.name)) {
          runFor(childNode, _partIndex + 1, _parentElement);
        }
      }
    }
  } // runFor

  boost::shared_ptr<JSONElement> PropertyQuery::run() {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    if(beginsWith(m_Query, m_pProperty->getName())) {
      runFor(m_pProperty, 0, result);
    }
    return result;
  } // run

} // namespace dss
