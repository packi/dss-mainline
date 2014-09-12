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

#ifndef _DS485_TYPES_H_INCLUDED
#define _DS485_TYPES_H_INCLUDED

#include <stdint.h>
#include <string>
#include <sstream>
#include <digitalSTROM/ds.h>

namespace dss {

/** Bus id of a device */
typedef uint16_t devid_t;

  const devid_t ShortAddressStaleDevice = 0xFFFF;

  std::string dsuid2str(dsuid_t dsuid);
  dsuid_t str2dsuid(std::string dsuid_str);
  dsid_t str2dsid(std::string dsid_str);
  std::string dsid2str(dsid_t dsid);
  uint32_t dsuid2serial(dsuid_t dsuid);
  bool IsEvenDsuid(dsuid_t dsuid);
  bool dsuid_get_next_dsuid(const dsuid_t dsuid, dsuid_t *next);
  bool dsuid_to_dsid(const dsuid_t dsuid, dsid_t *out);
  dsuid_t dsuid_from_dsid(const dsid_t& dsid);
}

#endif//__DSS_DSUID_H__
