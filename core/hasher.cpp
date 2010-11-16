/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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
#include "hasher.h"

#include <cassert>
#include <sstream>

namespace dss {

  Hasher::Hasher()
  : m_Finalized(false)
  {
    SHA256_Init(&m_Context);
  } // ctor

  template<>
  void Hasher::add<std::string>(const std::string& _t) {
    assert(m_Finalized == false);
    SHA256_Update(&m_Context, _t.c_str(), _t.length());
  } // add<std::string>

  template<>
  void Hasher::add<unsigned int>(const unsigned int& _t) {
    assert(m_Finalized == false);
    SHA256_Update(&m_Context, &_t, sizeof(_t));
  }

  std::string Hasher::str() {
    assert(m_Finalized == false);
    m_Finalized = true;
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_Final(digest, &m_Context);
    std::ostringstream sstr;
    sstr << std::hex;
    sstr.fill('0');
    for(int iByte = 0; iByte < SHA256_DIGEST_LENGTH; iByte++) {
      sstr.width(2);
      sstr << static_cast<unsigned int>(digest[iByte] & 0x0ff);
    }
    return sstr.str();
  } // str
  
  
} // namespace dss