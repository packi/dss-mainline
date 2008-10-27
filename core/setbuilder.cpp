/*
 * setbuilder.cpp
 *
 *  Created on: Oct 27, 2008
 *      Author: packi
 */

#include "base.h"

#include <vector>

using std::vector;

namespace dss {

  SetBuilder::SetBuilder() {

  } // ctor

  Set SetBuilder::RestrictBy(const string& _identifier, const Set& _set, const Zone& _zone) {

  } // RestrictBy

  Set SetBuilder::BuildSet(const string& _setDescription, const Zone* _context) {
	  Set result;
	  Zone* context = _context;
	  if(_context == NULL || BeginsWith(_setDescription, ".")) {
		  context = DSS::GetInstance()->GetApartment()->GetZone(0);
		  result = context->GetDevices();
	  } else {
		  result = _context->GetDevices();
	  }
    vector<string> splitted = SplitString(_setDescription, ".");
    for(vector<string>::iterator iPart = splitted.begin(), e = splitted.end();
       iPart != e; ++iPart)
    {
    	if(Trim(*iPart).size() != 0) {

    	}
    }
  } // BuildSet

} // namespace dss
