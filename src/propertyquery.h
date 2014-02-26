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

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "src/logger.h"

namespace dss {

  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;
  class JSONElement;
  class JSONObject;
  class JSONArrayBase;

  class PropertyQuery {
    __DECL_LOG_CHANNEL__

  public:
    PropertyQuery(PropertyNodePtr _pProperty, const std::string& _query)
    : m_pProperty(_pProperty),
      m_Query(_query)
    {
      parseParts();
    }

    boost::shared_ptr<JSONElement> run();
    boost::shared_ptr<JSONElement> run2();
  private:
    class part_t {
    public:
      std::string name;
      std::vector<std::string> properties;
      part_t(std::string _name, std::vector<std::string> _properties)
      : name(_name), properties(_properties)
      { }
    }; // part_t
  private:
    void parseParts();

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
    void runFor(PropertyNodePtr _parentNode, unsigned int _partIndex,
                boost::shared_ptr<JSONElement> _parentElement);

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
    void runFor2(PropertyNodePtr _parentNode, unsigned int _partIndex,
                 boost::shared_ptr<JSONElement> _parentElement);
    /**
     * Filter children of _node by part and store
     * and extracted values in JSON object
     *
     * @param _part contains filter
     * @param _parentElement values are added to it
     * @param _node element that should be filtered
     * @return JSON object with the added properties
     */
    boost::shared_ptr<JSONElement>
      addProperties(part_t& _part,
                    boost::shared_ptr<JSONElement> _parentElement,
                    dss::PropertyNodePtr _node);

    /**
     * Read int/float/bool value from node and store
     * in JSON object
     */
    boost::shared_ptr<JSONElement>
      addProperty(boost::shared_ptr<JSONObject> _obj,
                  dss::PropertyNodePtr _node);

    PropertyNodePtr m_pProperty;
    std::string m_Query;
    std::vector<part_t> m_PartList;
  }; // PropertyQuery

} // namespace dss
