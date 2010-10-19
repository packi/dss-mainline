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
#include "base.h"

#include <sstream>

namespace dss {

  const dss_dsid_t NullDSID(0,0);

  std::string dsid::toString() const {
    std::stringstream sstream;
    sstream.fill('0');
    sstream.width(16);
    sstream << std::hex << upper;
    sstream.width(8);
    sstream << lower;
    return sstream.str();
  }

  dss_dsid_t dsid::fromString(const std::string& _string) {
    dss_dsid_t result;
    std::string remainder = _string;
    result.upper = 0ll;
    result.lower = 0;

    // parse the string in 4 Byte chunks
    if(!remainder.empty()) {
      int nChar = std::min((int)remainder.size(), 8);
      int start = remainder.size() - nChar;
      result.lower = strToUInt("0x" + remainder.substr(start,nChar));
      remainder.erase(start, nChar);
    }

    if(!remainder.empty()) {
      int nChar = std::min((int)remainder.size(), 8);
      int start = remainder.size() - nChar;
      result.upper = strToUInt("0x" + remainder.substr(start,nChar));
      remainder.erase(start, nChar);
    }

    if(!remainder.empty()) {
      int nChar = std::min((int)remainder.size(), 8);
      int start = remainder.size() - nChar;
      result.upper |= ((uint64_t)strToUInt("0x" + remainder.substr(start,nChar)) << 32);
      remainder.erase(start, nChar);
    }

    return result;
  }

}
