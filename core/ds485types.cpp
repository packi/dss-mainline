/*
 * ds485types.cpp
 *
 *  Created on: Feb 24, 2009
 *      Author: patrick
 */

#include "ds485types.h"
#include "base.h"

#include <sstream>

namespace dss {

  std::string dsid::ToString() const {
    std::stringstream sstream;
    sstream.fill('0');
    sstream.width(16);
    sstream << std::hex << upper;
    sstream.width(8);
    sstream << lower;
    return sstream.str();
  }

  dsid_t dsid::FromString(const std::string& _string) {
    dsid_t result;
    std::string remainder = _string;
    result.upper = 0ll;
    result.lower = 0;

    // parse the string in 4 Byte chunks
    if(!remainder.empty()) {
      int nChar = std::min((int)remainder.size(), 8);
      int start = remainder.size() - nChar;
      result.lower = StrToUInt("0x" + remainder.substr(start,nChar));
      remainder.erase(start, nChar);
    }

    if(!remainder.empty()) {
      int nChar = std::min((int)remainder.size(), 8);
      int start = remainder.size() - nChar;
      result.upper = StrToUInt("0x" + remainder.substr(start,nChar));
      remainder.erase(start, nChar);
    }

    if(!remainder.empty()) {
      int nChar = std::min((int)remainder.size(), 8);
      int start = remainder.size() - nChar;
      result.upper |= ((uint64_t)StrToUInt("0x" + remainder.substr(start,nChar)) << 32);
      remainder.erase(start, nChar);
    }

    return result;
  }

}
