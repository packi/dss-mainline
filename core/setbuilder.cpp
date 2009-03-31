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

  Set SetBuilder::RestrictByFunction(const string& _identifier, const Set& _set, const Zone& _zone) {
    string::size_type openingBracket = _identifier.find('(');
    if(openingBracket == string::npos) {
      throw runtime_error("SetBuilder::RestrictByFunction: No '(' found in identifier : '" + _identifier + "'");
    }
    string::size_type closingBracket = _identifier.find(')', openingBracket);
    if(openingBracket == string::npos) {
      throw runtime_error("SetBuilder::RestrictByFunction: No ')' found after '(' in identifier : '" + _identifier + "'");
    }
    if(_identifier.find_first_not_of("\r\n \t", closingBracket + 1) != string::npos) {
      throw runtime_error("SetBuilder::RestrictByFunction: Garbage found after parameter '" + _identifier + "'");
    }

    string functionName = Trim(_identifier.substr(0, openingBracket));
    string parameter = _identifier.substr(openingBracket + 1, closingBracket - openingBracket - 1);
    vector<string> paramList = SplitString(parameter, ',', true);

    Set result;
    if(functionName == "dsid") {
      if(paramList.size() == 1) {
        result.AddDevice(_set.GetByDSID(dsid_t::FromString(paramList[0])));
      } else {
        throw runtime_error("SetBuilder::RestrictByFunction: dsid requires exactly one parameter.");
      }
    } else if(functionName == "zone") {
      if(paramList.size() == 1) {
        result = _set.GetByZone(StrToInt(paramList[0]));
      } else {
        throw runtime_error("SetBuilder::RestrictByFunction: zone requires exactly one parameter.");
      }
    }
    return result;
  }

  Set SetBuilder::RestrictBy(const string& _identifier, const Set& _set, const Zone& _zone) {

    if(_identifier.find('(') != string::npos) {
      return RestrictByFunction(_identifier, _set, _zone);
    } else {

      try {
        DeviceReference ref = _zone.GetDevices().GetByName(_identifier);
        Set result;
        result.AddDevice(ref);
        return result;
      } catch(exception&) {
      }

      // TODO: we might need to query zone 0 for the identifier too
      Group* grp = _zone.GetGroup(_identifier);
      if(grp != NULL) {
        Set result = _set.GetByGroup(*grp);
        return result;
      }
    }
    // return empty set
    // TODO: throw exception?
    Set result;
    return result;
  } // RestrictBy

  Set SetBuilder::BuildSet(const string& _setDescription, const Zone* _context) {
	  Set result;
	  const Zone* context = _context;
	  if(_context == NULL || BeginsWith(_setDescription, ".")) {
		  context = &DSS::GetInstance()->GetApartment().GetZone(0);
		  result = context->GetDevices();
	  } else {
		  result = _context->GetDevices();
	  }
    vector<string> splitted = SplitString(_setDescription, '.');
    for(vector<string>::iterator iPart = splitted.begin(), e = splitted.end();
       iPart != e; ++iPart)
    {
      if(iPart->size() > 0) {
        result = RestrictBy(*iPart, result, *context);
      }
    }
    return result;
  } // BuildSet

} // namespace dss
