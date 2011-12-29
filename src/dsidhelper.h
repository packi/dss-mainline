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

  struct dsid_helper {
    static void toDsmapiDsid(const dss_dsid_t& dss_dsid, dsid_t& target) {
      for (int i = 0; i < 8; ++i) {
        target.id[7-i] = dss_dsid.upper >> i*8 & 0xff;
      }
      for (int i = 0; i < 4; ++i) {
        target.id[11-i] = dss_dsid.lower >> i*8 & 0xff;
      }
    }
    
    static void toDssDsid(const dsid_t& dsid, dss_dsid_t& dss_dsid) {
      dss_dsid.upper = 0;
      dss_dsid.lower = 0;
      
      for (int i = 0; i < 8; ++i) {
        dss_dsid.upper |= ((uint64_t)dsid.id[i]) << 8*(7-i);
      }
      for (int i = 0; i < 4; ++i) {
        dss_dsid.lower |= dsid.id[8+i] << 8*(3-i);
      }
    }

    static std::string toString(dsid_t& target) {
      std::ostringstream str;
      for (int i = 0; i < 12; ++i) {
        str << std::hex
            << std::setw(2)
            << std::setfill('0')
            << (int)target.id[i]
            << " ";
      }
      return str.str();
    }
  };

} // namespace dss

#endif /* DSIDHELPER_H_ */  

