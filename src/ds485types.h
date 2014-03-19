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

namespace dss {

  /** Bus id of a device */
  typedef uint16_t devid_t;

  const devid_t ShortAddressStaleDevice = 0xFFFF;

  /** DSID of a device/dsMeter */

  typedef struct dsid{
    uint64_t upper;
    uint32_t lower;

    dsid()
    : upper(0), lower(0) {}

    dsid(uint64_t _upper, uint32_t _lower)
    : upper(_upper), lower(_lower) {}

    bool operator==(const dsid& _other) const {
      return (upper == _other.upper) && (lower == _other.lower);
    }

    bool operator!=(const dsid& _other) const {
      return !(_other == *this);
    }

    dsid& operator=(const dsid& _other) {
      upper = _other.upper;
      lower = _other.lower;
      return *this;
    }

    std::string toString() const;
    static dsid fromString(const std::string& _string);
  } dss_dsid_t;


  extern const dss_dsid_t NullDSID;

  typedef enum  {
    Click,
    Touch,
    TouchEnd,
    ShortClick,
    DoubleClick,
    ProgrammClick
  } ButtonPressKind;

}

#endif
