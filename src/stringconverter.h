/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>

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

#ifndef __STRING_CONVERTER_H__
#define __STRING_CONVERTER_H__

#include <string>
#include <iconv.h>

namespace dss {

class StringConverter
{
public:
  StringConverter(const char* _from_encoding, const char* _to_encoding);
  ~StringConverter();
  std::string convert(std::string _str);

protected:
  iconv_t m_cd;
  bool m_dirty;
  const char *m_to_encoding;
};

}
#endif
