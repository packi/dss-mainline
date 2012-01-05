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

#include <openssl/sha.h>
#include <openssl/md5.h>

#include <string>

namespace dss {

  class Hasher {
  public:
    Hasher();

    template <class t>
    void add(const t& _t);

    std::string str();
  private:
    SHA256_CTX m_Context;
    bool m_Finalized;
  }; // Hasher

  class HasherMD5 {
  public:
    HasherMD5();

    template <class t>
    void add(const t& _t);

    std::string str();
  private:
    MD5_CTX m_Context;
    bool m_Finalized;
  }; // HasherMD5

} // namespace dss