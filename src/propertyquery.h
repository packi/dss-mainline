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

#ifndef PROPERTYQUERY_H_INCLUDED
#define PROPERTYQUERY_H_INCLUDED

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "src/logger.h"

namespace dss {

  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;
  class JSONWriter;

  class KeyValueContainer {
  public:
    std::string name;
    std::string value;
    KeyValueContainer(std::string& _name, std::string& _value)
    : name(_name), value(_value)
    {}
    KeyValueContainer(std::string& _name)
    : name(_name), value("")
    {}
  }; // KeyValueContainer

  class PropertyContainer {
  public:
    std::string name;
    std::vector<KeyValueContainer> properties;
    bool child;
    PropertyContainer(std::string _name, std::vector<KeyValueContainer> _properties, bool _child)
    : name(_name), properties(_properties), child(_child)
    {}
  }; // PropertyContainer

  class PropertyQuery {
    __DECL_LOG_CHANNEL__

  public:
    PropertyQuery(PropertyNodePtr _pProperty, const std::string& _query)
    : m_pProperty(_pProperty),
      m_Query(_query)
    {
      parseParts();
    }

    void run(JSONWriter& json);
    void run2(JSONWriter& json);
  private:
    void parseParts();

    std::vector<KeyValueContainer> splitKeyValue(std::vector<std::string> input);
    std::vector<std::string> evalPropertyList(std::string& _input, std::string& _output_key);

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
    void runFor(PropertyNodePtr _parentNode, unsigned int _partIndex,
                JSONWriter& json);

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
    void runFor2(PropertyNodePtr _parentNode, unsigned int _partIndex,
                 JSONWriter& json);
    /**
     * Filter children of _node by part and store
     * and extracted values in JSON object
     *
     * @param _part contains filter
     * @param _parentElement values are added to it
     * @param _node element that should be filtered
     * @return JSON object with the added properties
     */
    void addProperties(PropertyContainer& _part, JSONWriter& json, dss::PropertyNodePtr _node);

    /**
     * Read int/float/bool value from node and store
     * in JSON object
     */
    void addProperty(JSONWriter& json,
                     dss::PropertyNodePtr _node);

    PropertyNodePtr m_pProperty;
    std::string m_Query;

  protected:
    std::vector<PropertyContainer> m_PartList;
  }; // PropertyQuery

} // namespace dss

#endif
