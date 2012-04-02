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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "base.h"
#include "stringconverter.h"

namespace dss {

StringConverter::StringConverter(const char* _from_encoding,
                                 const char* _to_encoding) :
                                                   m_dirty(false),
                                                   m_to_encoding(_to_encoding) {
  m_cd = iconv_open(m_to_encoding, _from_encoding);
  if (m_cd == (iconv_t)(-1)) {
    m_cd = (iconv_t)(0);
    throw DSSException(std::string("iconv: ") + strerror(errno));
  }
}

std::string StringConverter::convert(std::string _str) {
  std::string ret_str;
  std::string err_str;
  int ret;

  if (_str.empty()) {
    return _str;
  }

  size_t buffer_size = _str.length() * 4;
  const char *in = _str.c_str();
  char *out = (char *)malloc(buffer_size);
  if (!out) {
    throw DSSException("Could not allocate memory for string conversion!");
  }

  const char *in_copy = in;
  char *out_copy = out;

  const char **in_ptr = &in_copy;
  char **out_ptr = &out_copy;

  size_t in_bytes = _str.length();
  size_t out_bytes = buffer_size;

  if (m_dirty) {
    iconv(m_cd, NULL, 0, NULL, 0);
    m_dirty = false;
  }

  err_str = std::string("iconv: could not convert to ") + m_to_encoding +
            " encoding, ";
  ret = iconv(m_cd, (char **)in_ptr, &in_bytes, out_ptr, &out_bytes);
  if (ret == -1) {
    switch (errno) {
      case EILSEQ:
        err_str = err_str + "invalid character sequence!";
        break;
      case EINVAL:
        err_str = err_str + "incomplete multibyte sequence!";
        break;
      case E2BIG:
        err_str = err_str + "insufficient space in output buffer!";
        break;
      default:
        err_str = err_str + strerror(errno);
        break;
    }
    *out_copy = 0;
    m_dirty = true;
    if (out) {
      free(out);
    }
    throw DSSException(err_str);
  }
  ret_str = std::string(out, out_copy - out);
  free(out);
  return ret_str;

}

StringConverter::~StringConverter() {
  if (m_cd != (iconv_t)(0)) {
    iconv_close(m_cd);
  }
}

}
