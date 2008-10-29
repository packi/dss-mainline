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

using std::vector;

namespace dss {

  SetBuilder::SetBuilder() {

  } // ctor

  Set SetBuilder::RestrictBy(const string& _identifier, const Set& _set, const Zone& _zone) {

    try {
      DeviceReference ref = _zone.GetDevices().GetByName(_identifier);
      Set result;
      result.AddDevice(ref);
      return result;
    } catch(runtime_error&) {
    }

    try {
      DeviceReference ref = _zone.GetDevices().GetByDSID(StrToUInt(_identifier));
      Set result;
      result.AddDevice(ref);
      return result;
    } catch(runtime_error&) {
    }

    // TODO: we might need to query zone 0 for the identifier too
    Group* grp = _zone.GetGroup(_identifier);
    if(grp != NULL) {
      Set result = _set.GetByGroup(*grp);
      return result;
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
      result = RestrictBy(*iPart, result, *context);
    }
    return result;
  } // BuildSet

} // namespace dss
