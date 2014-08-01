/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Author: Johannes Winkelmann, <johannes.winkelmann@aizo.com>

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


#ifndef DSIDHELPER_H_
#define DSIDHELPER_H_

#include <digitalSTROM/ds.h>
#include <iomanip>
#include "ds485types.h"

namespace dss {

    /*
  struct dsid_helper {
    static void toDsmapiDsid(const dsuid_t& dss_dsid, dsid_t& target) {
#warning TODO DSUID: implement mapping
        SetNullDsud(target);
    }
    
    static void toDssDsid(const dsid_t& dsid, dsuid_t& dss_dsid) {
#warning TODO DSUID: implement mapping
        SetNullDsuid(dss_dsid);
    }
  };
  */

} // namespace dss

#endif /* DSIDHELPER_H_ */  

