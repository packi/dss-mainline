/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

  dsid_t SetBuilder::readDSID(unsigned int& _index) {
    return dsid_t::fromString(trim(readParameter(_index)));
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

  Set SetBuilder::restrictByFunction(const std::string& _functionName, unsigned int& _index, const Set& _set, const Zone& _zone) {
    if(_index >= m_SetDescription.size()) {
      throw std::range_error("_index is out of bounds");
    }
    assert(m_SetDescription[_index-1] == '(');

    Set result;

    if(_functionName == "dsid") {
      dsid_t dsid = readDSID(_index);
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
    } else if(_functionName == "add") {
      Set inner = parseSet(_index, _zone.getDevices(), _zone);
      result = _set.combine(inner);
    } else if(_functionName == "remove") {
      Set inner = parseSet(_index, _zone.getDevices(), _zone);
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
          dsid_t dsid = readDSID(_index);
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

  Set SetBuilder::restrictBy(const std::string& _identifier, const Set& _set, const Zone& _zone) {

    try {
      DeviceReference ref = _zone.getDevices().getByName(_identifier);
      Set result;
      result.addDevice(ref);
      return result;
    } catch(ItemNotFoundException&) {
    }

    // check for a local group
    Group* grp = _zone.getGroup(_identifier);
    if(grp != NULL) {
      Set result = _set.getByGroup(*grp);
      return result;
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


  Set SetBuilder::parseSet(unsigned int& _index, const Set& _set, const Zone& _context) {
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

  Set SetBuilder::buildSet(const std::string& _setDescription, const Zone* _context) {
	  Set result;
	  m_SetDescription = _setDescription;
	  const Zone* context = _context;
    unsigned int index = 0;
	  if((_context == NULL) || beginsWith(_setDescription, ".")) {
		  context = &m_Apartment.getZone(0);
		  result = context->getDevices();
		  index = 0;
	  } else {
		  result = _context->getDevices();
	  }
    result = parseSet(index, result, *context);

    return result;
  } // buildSet

} // namespace dss
