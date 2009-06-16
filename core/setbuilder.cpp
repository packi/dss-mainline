/*
 * setbuilder.cpp
 *
 *  Created on: Oct 27, 2008
 *      Author: packi
 */

#include "setbuilder.h"
#include "base.h"
#include "dss.h"

#include <vector>
#include <stdexcept>

using std::vector;

namespace dss {

  SetBuilder::SetBuilder() {

  } // ctor

  Set SetBuilder::restrictByFunction(const string& _identifier, const Set& _set, const Zone& _zone) {
    string::size_type openingBracket = _identifier.find('(');
    if(openingBracket == string::npos) {
      throw runtime_error("SetBuilder::restrictByFunction: No '(' found in identifier : '" + _identifier + "'");
    }
    string::size_type closingBracket = _identifier.find(')', openingBracket);
    if(openingBracket == string::npos) {
      throw runtime_error("SetBuilder::restrictByFunction: No ')' found after '(' in identifier : '" + _identifier + "'");
    }
    if(_identifier.find_first_not_of("\r\n \t", closingBracket + 1) != string::npos) {
      throw runtime_error("SetBuilder::restrictByFunction: Garbage found after parameter '" + _identifier + "'");
    }

    string functionName = trim(_identifier.substr(0, openingBracket));
    string parameter = _identifier.substr(openingBracket + 1, closingBracket - openingBracket - 1);
    vector<string> paramList = splitString(parameter, ',', true);

    Set result;
    if(functionName == "dsid") {
      if(paramList.size() == 1) {
        result.addDevice(_set.getByDSID(dsid_t::fromString(paramList[0])));
      } else {
        throw runtime_error("SetBuilder::restrictByFunction: dsid requires exactly one parameter.");
      }
    } else if(functionName == "zone") {
      if(paramList.size() == 1) {
        result = _set.getByZone(strToInt(paramList[0]));
      } else {
        throw runtime_error("SetBuilder::restrictByFunction: zone requires exactly one parameter.");
      }
    } else if(functionName == "fid") {
      if(paramList.size() == 1) {
        result = _set.getByFunctionID(strToInt(paramList[0]));
      } else {
        throw runtime_error("SetBuilder::restrictByFunction: fid requires exactly one parameter");
      }
    }
    return result;
  }

  Set SetBuilder::restrictBy(const string& _identifier, const Set& _set, const Zone& _zone) {

    if(_identifier.find('(') != string::npos) {
      return restrictByFunction(_identifier, _set, _zone);
    } else {

      try {
        DeviceReference ref = _zone.getDevices().getByName(_identifier);
        Set result;
        result.addDevice(ref);
        return result;
      } catch(exception&) {
      }

      // TODO: we might need to query zone 0 for the identifier too
      Group* grp = _zone.getGroup(_identifier);
      if(grp != NULL) {
        Set result = _set.getByGroup(*grp);
        return result;
      }
    }
    // return empty set
    // TODO: throw exception?
    Set result;
    return result;
  } // restrictBy

  Set SetBuilder::buildSet(const string& _setDescription, const Zone* _context) {
	  Set result;
	  const Zone* context = _context;
	  if(_context == NULL || beginsWith(_setDescription, ".")) {
		  context = &DSS::getInstance()->getApartment().getZone(0);
		  result = context->getDevices();
	  } else {
		  result = _context->getDevices();
	  }
    vector<string> splitted = splitString(_setDescription, '.');
    for(vector<string>::iterator iPart = splitted.begin(), e = splitted.end();
       iPart != e; ++iPart)
    {
      if(iPart->size() > 0) {
        result = restrictBy(*iPart, result, *context);
      }
    }
    return result;
  } // buildSet

} // namespace dss
