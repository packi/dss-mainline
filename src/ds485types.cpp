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

#include "ds485types.h"

#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <stdlib.h>
#include <digitalSTROM/dsuid/dsuid.h>

namespace dss {

std::string dsuid2str(dsuid_t dsuid)
{
  std::ostringstream str;
  for (int i = 0; i < DSUID_SIZE; ++i) {
    str << std::hex << std::setw(2) << std::setfill('0') << (int)dsuid.id[i];
  }
  return str.str();
}

dsuid_t str2dsuid(std::string dsuid_str)
{
  dsuid_t dsuid;

  if (::dsuid_from_string(dsuid_str.c_str(), &dsuid) != DSUID_RC_OK)
  {
    throw std::runtime_error("can not convert from string, invalid dSUID");
  }
  return dsuid;
}

/**
 * Logic:
 * - If there is a value in dsuidStr it is expected to be a dSUID
 * - dsidStr could contain a dSID od a dSUID
 */
dsuid_t dsidOrDsuid2dsuid(std::string dsidStr, std::string dsuidStr)
{
  dsuid_t dsuid;
  if (dsuidStr.empty()) {
    try {
      dsid_t dsid = str2dsid(dsidStr);
      dsuid = dsuid_from_dsid(&dsid);
    } catch (std::runtime_error& e) {
      dsuid = str2dsuid(dsuidStr);
    }
  } else {
    dsuid = str2dsuid(dsuidStr);
  }
  return dsuid;
}

dsid_t str2dsid(std::string dsid_str)
{
  dsid_t dsid;

  if (dsid_str.length() != 24) {
    throw std::runtime_error("can not convert from string, invalid dSID");
  }

  size_t i = 0;
  size_t count = 0;
  while (i < 24) {
    std::string number = dsid_str.substr(i, 2);
    dsid.id[count] = (unsigned char)strtol(number.c_str(), NULL, 16);
    i = i + 2;
    count++;
  }

  return dsid;
}

std::string dsid2str(dsid_t dsid) {
  std::ostringstream str;
  for (int i = 0; i < 12; ++i) {
    str << std::hex
        << std::setw(2)
        << std::setfill('0')
        << (int)dsid.id[i];
   }
   return str.str();
}

uint32_t dsuid2serial(dsuid_t dsuid) {
  uint32_t serial = 0;
  if (::dsuid_get_serial_number(&dsuid, &serial) != DSUID_RC_OK) {
    throw std::runtime_error("could not extract serial number from dSUID" +
                              dsuid2str(dsuid));
  }
  return serial;
}

bool IsEvenDsuid(dsuid_t dsuid) {
  uint32_t serial = 0;
  if (dsuid_get_serial_number(&dsuid, &serial) != DSUID_RC_OK) {
    throw std::runtime_error("could not extract serial number from dSUID" +
                              dsuid2str(dsuid));
  }

  return !(serial % 2);
}

bool dsuid_get_next_dsuid(const dsuid_t dsuid, dsuid_t *out) {
  if (::dsuid_get_next_dsuid(&dsuid, out) != DSUID_RC_OK) {
    memset(out, 0, sizeof(dsid_t));
    return false;
  }
  return true;
}

bool dsuid_to_dsid(const dsuid_t dsuid, dsid_t *dsid) {
  if (::dsuid_to_dsid(&dsuid, dsid) != DSUID_RC_OK) {
    memset(dsid, 0, sizeof(dsid_t));
    return false;
  }
  return true;
}

dsuid_t dsuid_from_dsid(const dsid_t& dsid) {
  dsuid_t dsuid;
  dsuid = ::dsuid_from_dsid(&dsid);
  return dsuid;
}

} // namespace
