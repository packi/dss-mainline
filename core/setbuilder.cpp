/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#include "setbuilder.h"

#include <vector>
#include <stdexcept>
#include <cassert>

#include "base.h"
#include "core/model/set.h"
#include "core/model/zone.h"
#include "core/model/apartment.h"
#include "core/model/group.h"
#include "core/model/modelconst.h"

using std::vector;

namespace dss {

  SetBuilder::SetBuilder(Apartment& _apartment)
  : m_Apartment(_apartment)
  {
  } // ctor

  void SetBuilder::skipWhitespace(unsigned int& _index) {
    while((_index < m_SetDescription.size()) && isspace(m_SetDescription[_index])) {
      _index++;
    }
  } // skipWhitespace

  std::string SetBuilder::readParameter(unsigned int& _index) {
    skipWhitespace(_index);
    int start = _index;
    std::string::size_type pos = m_SetDescription.find_first_of(",)", _index);
    if(pos == std::string::npos) {
      throw std::runtime_error("expected , or )");
    }
    _index = pos;
    std::string result = m_SetDescription.substr(start, _index - start);
    skipWhitespace(_index);
    return result;
  } // readParameter

  int SetBuilder::readInt(unsigned int& _index) {
    return strToInt(trim(readParameter(_index)));
  } // readInt

  dss_dsid_t SetBuilder::readDSID(unsigned int& _index) {
    return dss_dsid_t::fromString(trim(readParameter(_index)));
  } // readDSID

  std::string SetBuilder::readString(unsigned int& _index) {
    std::string result = trim(readParameter(_index));
    if(result.size() >= 2) {
      char first = result[0];
      char last = result[result.size()-1];
      if((first == '\'') && (last == '\'')) {
        result = result.substr(1, result.size() - 2);
        return result;
      }
    }
    throw std::runtime_error("String should be enclosed by \"'\"");
  } // readString

  Set SetBuilder::restrictByFunction(const std::string& _functionName, unsigned int& _index, const Set& _set, boost::shared_ptr<const Group> _group) {
    if(_index >= m_SetDescription.size()) {
      throw std::range_error("_index is out of bounds");
    }
    assert(m_SetDescription[_index-1] == '(');

    Set result;

    if(_functionName == "dsid") {
      dss_dsid_t dsid = readDSID(_index);
      result.addDevice(_set.getByDSID(dsid));
    } else if(_functionName == "zone") {
      if(m_SetDescription[_index] == '\'') {
        std::string zoneName = readString(_index);
        result = _set.getByZone(zoneName);
      } else {
        int zoneID = readInt(_index);
        result = _set.getByZone(zoneID);
      }
    } else if(_functionName == "group") {
      if(m_SetDescription[_index] == '\'') {
        std::string groupName = readString(_index);
        result = _set.getByGroup(groupName);
      } else {
        int groupID = readInt(_index);
        result = _set.getByGroup(groupID);
      }
    } else if(_functionName == "fid") {
      int fid = readInt(_index);
      result = _set.getByFunctionID(fid);
    } else if(_functionName == "tag") {
      std::string tagName = readString(_index);
      result = _set.getByTag(tagName);
    } else if(_functionName == "add") {
      Set inner = parseSet(_index, _group->getDevices(), _group);
      result = _set.combine(inner);
    } else if(_functionName == "remove") {
      Set inner = parseSet(_index, _group->getDevices(), _group);
      result = _set.remove(inner);
    } else if(_functionName == "empty") {
      // nop
    } else if(_functionName == "addDevices") {
      result = _set;
      do {
        if(m_SetDescription[_index] == ',') {
          _index++;
        }
        if(m_SetDescription[_index] == '\'') {
          std::string name = readString(_index);
          result.addDevice(m_Apartment.getDeviceByName(name));
        } else {
          dss_dsid_t dsid = readDSID(_index);
          result.addDevice(m_Apartment.getDeviceByDSID(dsid));
        }
      } while(m_SetDescription[_index] == ',');
    }
    assert(m_SetDescription[_index] == ')' || m_SetDescription[_index] == ',');
    if(m_SetDescription[_index] == ',') {
      throw std::runtime_error("superfluous parameter detected at position " + intToString(_index));
    }
    _index++; // skip over closing bracket

    return result;
  }

  Set SetBuilder::restrictBy(const std::string& _identifier, const Set& _set, boost::shared_ptr<const Group> _group) {

    try {
      DeviceReference ref = _group->getDevices().getByName(_identifier);
      Set result;
      result.addDevice(ref);
      return result;
    } catch(ItemNotFoundException&) {
    }

    // check for a global group
    try {
      return _set.getByGroup(_identifier);
    } catch(ItemNotFoundException&) {
    }

    // check for a zone name
    try {
      return _set.getByZone(_identifier);
    } catch(ItemNotFoundException&) {
    }

    // return empty set
    Set result;
    return result;
  } // restrictBy


  Set SetBuilder::parseSet(unsigned int& _index, const Set& _set, boost::shared_ptr<const Group> _context) {
    skipWhitespace(_index);
    if(_index >= m_SetDescription.size()) {
      return _set;
    }
    // scan forward to a delimiter
    std::string::size_type pos = m_SetDescription.find_first_of(".(),", _index);
    bool end = false;
    if(pos == std::string::npos) {
      pos = m_SetDescription.size() - 1;
      end = true;
    } else if(pos == m_SetDescription.size() - 1) {
      end = true;
    }
    std::string entry = m_SetDescription.substr(_index, pos + 1 - _index );
    if(entry == ".") {
      _index = pos + 1;
      Set newRoot = m_Apartment.getDevices();
      return parseSet(_index, newRoot, _context);
    } else if(m_SetDescription[pos] == '(') {
      _index = pos + 1;
      Set tmp = restrictByFunction(entry.erase(entry.size()-1), _index, _set, _context);
      skipWhitespace(_index);
      // don't recurse if we're at the end or we were parsing a parameter
      if((_index < m_SetDescription.size()) && (m_SetDescription[_index] != ',') && (m_SetDescription[_index] != ')')) {
        _index++;
        return parseSet(_index, tmp, _context);
      } else {
        return tmp;
      }
    } else {
      std::string item = entry;
      bool parsingParam = endsWith(item, ",") || endsWith(item, ")");
      if(!end || parsingParam) {
        item.erase(item.size()-1);
      }
      Set tmp = restrictBy(trim(item), _set, _context);
      _index = pos + 1;

      if(!end && !parsingParam) {
        return parseSet(_index, tmp, _context);
      }
      if(parsingParam) {
        _index--; // make sure we're positioned on the closing token
      }
      return tmp;
    }
    return _set;
  }

  Set SetBuilder::buildSet(const std::string& _setDescription, boost::shared_ptr<const Zone> _context) {
    if(_context != NULL) {
      return buildSet(_setDescription, _context->getGroup(GroupIDBroadcast));
    } else {
      return buildSet(_setDescription, boost::shared_ptr<const Group>());
    }
  }

  Set SetBuilder::buildSet(const std::string& _setDescription, boost::shared_ptr<const Group> _context) {
    Set result;
    m_SetDescription = _setDescription;
    boost::shared_ptr<const Group> context = _context;
    unsigned int index = 0;
    if(beginsWith(_setDescription, ".")) {
      context = m_Apartment.getZone(0)->getGroup(GroupIDBroadcast);
      result = context->getDevices();
      index = 1;
    } else if(_context != NULL) {
      result = _context->getDevices();
    } else {
      context = m_Apartment.getZone(0)->getGroup(GroupIDBroadcast);
    }
    result = parseSet(index, result, context);

    return result;
  } // buildSet


  //================================================== MeterSetBuilder

  MeterSetBuilder::MeterSetBuilder(Apartment& _apartment)
  : m_Apartment(_apartment)
  {
  } // ctor

  std::vector<boost::shared_ptr<DSMeter> > MeterSetBuilder::buildSet(const std::string& _setDescription) {
    std::vector<boost::shared_ptr<DSMeter> > result;
    std::string from = _setDescription;
    if(from.find(".meters(") == 0) {
      from = from.substr(8);
    } else {
      throw std::runtime_error("Expected '.meters(' at the start of the description");
    }

    if((from.length() > 0) && (from.at(from.length()-1) == ')')) {
      from = from.substr(0, from.length()-1);
    } else {
      throw std::runtime_error("No closing ')' found");
    }

    if(from == "all") {
      result = m_Apartment.getDSMeters();
    } else {
      std::vector<std::string> dsids = dss::splitString(from, ',', true);
      for(size_t i = 0; i < dsids.size(); i++) {
        std::string strId = dsids.at(i);
        if(strId.empty()) {
          throw std::runtime_error("Invalid dsid in description: " + strId);
        }

        dss_dsid_t dsid;
        try {
          dsid = dss_dsid_t::fromString(strId);
        } catch(std::invalid_argument&) {
          throw std::runtime_error("Invalid dsid in description: " + strId);
        }

        try {
          boost::shared_ptr<DSMeter> dsMeter = m_Apartment.getDSMeterByDSID(dsid);
          result.push_back(dsMeter);
        } catch (std::runtime_error&) {
          throw std::runtime_error("Could not find dsMeter with given dsid.");
        }

      }
    }
    return result;
  } // buildSet


} // namespace dss
