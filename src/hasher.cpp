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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "hasher.h"

#include <cassert>
#include <sstream>

#include "base.h"

namespace dss {

  //============================================= Hasher

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
    return hexEncodeByteArray(digest, SHA256_DIGEST_LENGTH);
  } // str

  //============================================= HasherMD5

  HasherMD5::HasherMD5()
  : m_Finalized(false)
  {
    MD5_Init(&m_Context);
  } // ctor

  template<>
  void HasherMD5::add<std::string>(const std::string& _t) {
    assert(m_Finalized == false);
    MD5_Update(&m_Context, _t.c_str(), _t.length());
  } // add<std::string>

  template<>
  void HasherMD5::add<unsigned int>(const unsigned int& _t) {
    assert(m_Finalized == false);
    MD5_Update(&m_Context, &_t, sizeof(_t));
  }

  std::string HasherMD5::str() {
    assert(m_Finalized == false);
    m_Finalized = true;
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &m_Context);
    return hexEncodeByteArray(digest, MD5_DIGEST_LENGTH);
  } // str

} // namespace dss
