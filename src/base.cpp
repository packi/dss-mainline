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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "base.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <sstream>
#include <iomanip>

#include <curl/curl.h>

#include "foreach.h"
#include <limits>

#include "exception.h"
#include "logger.h"
#include "stringconverter.h"

namespace dss {

  //============================================= std::string parsing/formatting/conversion

  std::string trim(const std::string& _str) {
    std::string result = _str;
    std::string::size_type notwhite = result.find_first_not_of( " \t\n\r" );
    result.erase(0,notwhite);
    notwhite = result.find_last_not_of( " \t\n\r" );
    result.erase( notwhite + 1 );
    return result;
  } // trim

  int strToInt(const std::string& _strValue) {
    if(!_strValue.empty()) {
      char* endp;
      int result = strtol(_strValue.c_str(), &endp, 0);
      if(*endp == '\0') {
        return result;
      }
    }
    throw std::invalid_argument(std::string("strToInt: Could not parse value: '") + _strValue + "'");
  } // strToInt

  int strToIntDef(const std::string& _strValue, const int _default) {
    if(!_strValue.empty()) {
      char* endp;
      int result = strtol(_strValue.c_str(), &endp, 0);
      if(*endp == '\0') {
        return result;
      }
    }
    return _default;
  } // strToIntDef

  unsigned int strToUInt(const std::string& _strValue) {
    if(!_strValue.empty()) {
      char* endp;
      unsigned int result = strtoul(_strValue.c_str(), &endp, 0);
      if(*endp == '\0') {
        return result;
      }
    }
    throw std::invalid_argument(std::string("strToUInt: Could not parse value: '") + _strValue + "'");
  } // strToUInt

  unsigned int strToUIntDef(const std::string& _strValue, const unsigned int _default) {
    if(!_strValue.empty()) {
      char* endp;
      unsigned int result = strtoul(_strValue.c_str(), &endp, 0);
      if(*endp == '\0') {
        return result;
      }
    }
    return _default;
  } // strToUIntDef

  unsigned long long strToULongLong(const std::string& _strValue) {
    if(!_strValue.empty()) {
      char* endp;
      unsigned long long result = strtoull(_strValue.c_str(), &endp, 0);
      if(*endp == '\0') {
        return result;
      }
    }
    throw std::invalid_argument(std::string("strToUInt: Could not parse value: '") + _strValue + "'");
  } // strToULongLong

  unsigned long long strToULongLongDef(const std::string& _strValue, const unsigned int _default) {
    if(!_strValue.empty()) {
      char* endp;
      unsigned long long result = strtoull(_strValue.c_str(), &endp, 0);
      if(*endp == '\0') {
        return result;
      }
    }
    return _default;
  } // strToULongLongDef

  double strToDouble(const std::string& _strValue) {
    if(!_strValue.empty()) {
      char* endp;
      double result = strtod(_strValue.c_str(), &endp);
      if(*endp == '\0') {
        return result;
      }
    }
    throw std::invalid_argument(std::string("strToDouble: Could not parse value: '") + _strValue + "'");
  } // strToDouble

  double strToDouble(const std::string& _strValue, const double _default) {
    if(!_strValue.empty()) {
      char* endp;
      double result = strtod(_strValue.c_str(), &endp);
      if(*endp == '\0') {
        return result;
      }
    }
    return _default;
  } // strToDouble

  std::string doubleToString(const double _value) {
    std::stringstream sstream;
    sstream << std::setprecision(12) << _value;
    return sstream.str();
  } // doubleToString

  double roundDigits(const double input, const int digits) {
    double output = input;
    output *= pow(10, digits);
    output = round(output);
    output /= pow(10, digits);
    return output;
  }

  std::string intToString(const long long _int, bool _hex) {
    //
    // http://en.cppreference.com/w/cpp/types/numeric_limits/digits10
    // ... any number with this many decimal digits can be converted to a value
    // of type T and back to decimal form, without change due to rounding or
    // overflow.
    // It's a lower bound, the type may also hold numbers with one digit more
    // but not all of them
    // +3 for sign, '\0' terminator and upper bound
    //
    const int max_size = std::numeric_limits<long long>::digits10 + 3;
    char buffer[max_size] = { 0 };
    int n;
    if (_hex) {
      n = snprintf(buffer, max_size, "0x%llx", _int);
    } else {
      n = snprintf(buffer, max_size, "%lld", _int);
    }
    assert(n < max_size);
    return std::string(buffer);
  } // intToString

  std::string uintToString(long long unsigned int _int, bool _hex) {
    // +2 for '\0' terminator and upper bound
    const int max_size = std::numeric_limits<long long unsigned>::digits10 + 2;
    char buffer[max_size] = { 0 };
    int n;
    if (_hex) {
      n = snprintf(buffer, max_size, "0x%llx", _int);
    } else {
      n = snprintf(buffer, max_size, "%llu", _int);
    }
    assert(n < max_size);
    return std::string(buffer);
  } // uintToString

  std::string hexEncodeByteArray(const unsigned char *a, unsigned int len) {
    std::ostringstream s;
    s << std::hex;
    s.fill('0');
    for (unsigned i = 0; i < len; i++) {
      s.width(2);
      s << static_cast<unsigned int>(a[i] & 0x0ff);
    }
    return s.str();
  }

  std::string unsignedLongIntToHexString(const unsigned long long _value) {
    std::stringstream sstream;
    sstream << std::hex << _value;
    return sstream.str();
  }

  std::string unsignedLongIntToString(const unsigned long long _value) {
    std::stringstream sstream;
    sstream << _value;
    return sstream.str();
  }

  std::size_t findAndUnescape(std::string& str, const char c, std::size_t startPos) {
    std::size_t pos = str.find(c, startPos);
    while ((pos != std::string::npos) && (pos > 0) && (str.at(pos - 1) == '\\')) {
      str.erase(pos - 1, 1);
      pos = str.find(c, pos);
    }
    return pos;
  }

  std::vector<std::string> splitString(const std::string& _source,
                                       const char _delimiter,
                                       bool _trimEntries) {
    std::vector<std::string> result;
    std::string curString = _source;
    std::string::size_type delimPos = 0, startField = 0, skip = 0;

    while (startField < curString.size()) {
      delimPos = curString.find(_delimiter, skip);

      if ((delimPos > 0) && (delimPos != std::string::npos)) {
        if (curString.at(delimPos - 1) == '\\') {
          // remove escape character
          curString.erase(delimPos - 1, 1);
          skip = delimPos;
          continue;
        }
      }

      if (delimPos != std::string::npos) {
        if (_trimEntries) {
          result.push_back(trim(curString.substr(startField, delimPos - startField)));
        } else {
          result.push_back(curString.substr(startField, delimPos - startField));
        }
        skip = startField = delimPos + 1;
        if (curString.size() - skip == 0) {
          // in case of trailing delimiter, but no actual field data, we add an
          // empty string. not sure why we need this, probably nobody does
          result.push_back("");
        }
      } else {
        if (_trimEntries) {
          result.push_back(trim(curString.substr(startField)));
        } else {
          result.push_back(curString.substr(startField));
        }
        break;
      }
    }
    return result;
  } // splitString

  void replaceAll(std::string& s, const std::string& a, const std::string& b) {
    for (std::string::size_type i = s.find(a);
         i != std::string::npos;
         i = s.find(a, i + b.size()))
      s.replace(i, a.size(), b);
  } // replaceAll

  std::string urlDecode(const std::string& _in) {
    std::string result = "";

    std::string::size_type lastPos = 0;
    std::string::size_type pos = _in.find('%', 0);
    while(pos != std::string::npos && _in.length() > (pos + 2)) {
      if(lastPos != pos) {
        std::string add = _in.substr(lastPos, pos - lastPos);
        replaceAll(add, "+", " ");
        result += add;
      }
      std::string hex = _in.substr(pos+1, 2);
      char* ptr;
      char tmp = (char)strtol(hex.c_str(), &ptr, 16);
      if(*ptr != '\0') {
        return _in;
      }
      result += tmp;
      lastPos = pos + 3;
      pos = _in.find('%', lastPos);
    }
    if(lastPos < _in.length()) {
      std::string end = _in.substr(lastPos, std::string::npos);
      replaceAll(end, "+", " ");
      result += end;
    }
    return result;
  } // urlDecode

  /**
   * http://en.wikipedia.org/wiki/Percent-encoding#Percent-encoding_reserved_characters
   * civetweb url_encode does not escape "$,;()";
   */
  std::string urlEncode(const std::string& _in) {
    static CURL *handle = NULL;
    char *escaped;

    if (handle == NULL) {
      handle = curl_easy_init();
      if (!handle) {
        throw std::bad_alloc();
      }
    }
    escaped = curl_easy_escape(handle, _in.c_str(), _in.length());
    if (!escaped) {
      throw std::bad_alloc();
    }
    std::string ret = escaped;
    curl_free(escaped);
    return ret;
  }

  std::string truncateUTF8String(const std::string& _in, int _maxBytes) {
    assert(_maxBytes >= 0);

    if (_maxBytes == 0) {
      return "";
    } else if (static_cast<unsigned>(_maxBytes) >= _in.length()) {
      return _in;
    }

    char* buffer = static_cast<char*>(malloc(_maxBytes+1));
    strncpy(buffer, _in.c_str(), _maxBytes);
    buffer[_maxBytes] = '\0';
    int len = strlen(buffer);
    if(len == _maxBytes) {
      char* pos = buffer + (_maxBytes - 1);
      if((*pos & 0x80) == 0x80) {
        // go backwards through the string until we've found the start-byte
        // (MSB and MSB+1 set)
        while((pos > buffer) && (*pos & 0xC0) == 0x80) {
          pos--;
        }
        int skippedBytes = len - (pos - buffer);
        unsigned char mask = 0xFF;
        unsigned char compareTo = 0xFF;
        // a start byte always looks like:
        // 1(1)*0xxxxxx, where the number of ones on the left correspond to the
        // number of bytes in the character followed by a zero. We're creating
        // a mask that includes the ones and the zero and compare that to the
        // expected bit-pattern.
        switch(skippedBytes) {
        case 6:
          mask = 0xFE;
          compareTo = 0xFC;
          break;
        case 5:
          mask = 0xFC;
          compareTo = 0xF8;
          break;
        case 4:
          mask = 0xF8;
          compareTo = 0xF0;
          break;
        case 3:
          mask = 0xF0;
          compareTo = 0xE0;
          break;
        case 2:
          mask = 0xE0;
          compareTo = 0xC0;
          break;
        }
        if(mask != 0xFF) {
          // if the mask doesn't match we've found a broken character
          if((*pos & mask) != compareTo) {
            *pos = '\0';
          }
        } else {
          // if we haven't found a mask, the character is invalid anyway
          *pos = '\0';
        }
      }
    }
    std::string result(buffer);
    free(buffer);
    return result;
  } // truncateUTF8String

  bool endsWith( const std::string& str, const std::string& searchString ) {
    std::string::size_type lenStr = str.length();
    std::string::size_type lenSearch = searchString.length();
    return (lenStr >= lenSearch) &&
    (str.compare( lenStr - lenSearch, lenSearch, searchString ) == 0);
  } // endsWith


  bool beginsWith( const std::string& str, const std::string& searchString ) {
    std::string::size_type lenStr = str.length();
    std::string::size_type lenSearch = searchString.length();
    return (lenStr >= lenSearch) &&
    (str.compare( 0, lenSearch, searchString ) == 0);
  } // beginsWith

  std::pair<std::string, std::string> splitIntoKeyValue(const std::string& _keyValue) {
    std::string key;
    std::string value;
    std::string::size_type delimPos = _keyValue.find('=', 0);
    if(delimPos != std::string::npos) {
      key = _keyValue.substr(0, delimPos);
      value = _keyValue.substr(delimPos + 1, std::string::npos);
    }
    return std::make_pair(key, value);
  } // splitIntoKeyValue

  std::string join(const std::vector<std::string>& _strings, const std::string& _delimiter) {
    std::ostringstream sstream;
    bool first = true;
    foreach(const std::string& string, _strings) {
      if(first) {
        first = false;
        sstream << string;
      } else {
        sstream << _delimiter << string;
      }
    }
    return sstream.str();
  } // join

  /**
   * car(/foo/bar/baz) -> ('foo', 'bar/baz')
   * https://en.wikipedia.org/wiki/CAR_and_CDR
   */
  std::string carCdrPath(std::string &path) {
    std::string car;
    std::string::size_type slashPos = path.find('/');
    if (slashPos != std::string::npos) {
      car = path.substr(0, slashPos);
      path.erase(0, slashPos + 1);
    } else {
      car = path;
      path.clear();
    }
    return car;
  }

  uint16_t update_crc(uint16_t _crc, const unsigned char& c) {
	int i;
    uint16_t result = _crc ^ c;

    for( i=8;i;i--){
	  result = (result >> 1) ^ ((result & 1) ? 0x8408 : 0 );
    }
    return result;
  } // update_crc

  uint16_t crc16(unsigned const char* _data, const int _size) {
    uint16_t result = 0x0000;
    for(int iByte = 0; iByte < _size; iByte++) {
      result = update_crc(result, _data[iByte]);
    }
    return result;
  } // cRC16

  //================================================== Filesystem helpers
  bool rwAccess(const std::string& _filename)
  {
    if (access(_filename.c_str(), R_OK | W_OK) == 0) {
      return true;
    }

    return false;
  }

  // throws exception
  std::string readFile(const std::string& filename) {
    try {
      std::string data;
      std::ifstream file(filename.c_str(), std::ios::ate | std::ios::in);
      file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

      data.clear();
      data.reserve(file.tellg());
      file.seekg(0, std::ios::beg);
      data.append(std::istreambuf_iterator<char>(file.rdbuf()),
                  std::istreambuf_iterator<char>());
      return data;
    } catch (std::exception &e) {
      // rethrow with more meaningful message that includes filename.
      throw std::runtime_error(std::string() + "Failed to read file:" + filename + " e.what():" + e.what());
    }
  }

  //================================================== System utilities

  void sleepSeconds( const unsigned int _seconds ) {
    sleep( _seconds );
  } // sleepSeconds

  void sleepMS( const unsigned int _ms ) {
    usleep( _ms * 1000 );
  }

  std::string getTempDir() {
    std::string result = addTrailingBackslash(P_tmpdir);
    return result;
  }

  std::string addTrailingBackslash(const std::string& _path) {
    if(!_path.empty() && (_path.at(_path.length() - 1) != '/')) {
      return _path+ "/";
    }
    return _path;
  } // addTrailingBackslash

  bool syncFile(const std::string& _path) {
    int fd = open(_path.c_str(), O_APPEND);
    if(fd != -1) {
      fsync(fd);
      close(fd);
      return true;
    }
    return false;
  } // syncFile

  std::string doIndent(const int _indent) {
    if (_indent <= 0) {
      return "";
    } else {
      return "    " + doIndent(_indent - 1);
    }
  }

  /* Escape reserved characters in XML
   * inspired by http://mediatomb.svn.sourceforge.net/viewvc/mediatomb/trunk/mediatomb/src/mxml/node.cc
   * and http://pugixml.googlecode.com/svn/tags/latest/src/pugixml.cpp
   *
   * if string is not valid UTF-8 reset to "" (empty)
   */
  std::string XMLStringEscape(const std::string& str) {
    std::stringstream sstream;

    StringConverter st("UTF-8", "UTF-8");
    try {
      st.convert(str);
    } catch (DSSException &ex) {
      Logger::getInstance()->log("cleared out invalid UTF-8 string " + str,
                                 lsWarning);
      return "";
    }

    signed char *ptr = (signed char *)str.c_str();
    while (ptr && *ptr)
    {
        switch (*ptr)
        {
            case '<' : sstream << "&lt;"; break;
            case '>' : sstream << "&gt;"; break;
            case '&' : sstream << "&amp;"; break;
            case '"' : sstream << "&quot;"; break;
            case '\'' : sstream << "&apos;"; break;
                       // handle control codes
            default  : if (((*ptr >= 0x00) && (*ptr <= 0x1f) &&
                            (*ptr != 0x09) && (*ptr != 0x0d) &&
                            (*ptr != 0x0a)) || (*ptr == 0x7f)) {
                         unsigned int ch = static_cast<unsigned int>(*ptr++);
                         sstream << "&#" << static_cast<char>((ch / 10) + '0') << static_cast<char>((ch % 10) + '0') << ';';
                       } else {
                         sstream << *ptr;
                       }
                       break;
        }
        ptr++;
    }
    return sstream.str();
  }

  //================================================== Properties

  bool Properties::has(const std::string& _key) const {
    auto iEntry = m_Container.find(_key);
    return iEntry != m_Container.end();
  } // has

  void Properties::set(const std::string& _key, const std::string& _value) {
    m_Container[_key] = _value;
  } // set

  const std::string& Properties::get(const std::string& _key) const {
    if(!has(_key)) {
      throw std::runtime_error(std::string("could not find value for '") + _key + "' in properties");
    } else {
      // not using operator[] here because its non-const
      auto iEntry = m_Container.find(_key);
      return iEntry->second;
    }
  } // get

  const std::string& Properties::get(const std::string& _key, const std::string& _default) const {
    if(has(_key)) {
      // not using operator[] here because its non-const
      auto iEntry = m_Container.find(_key);
      return iEntry->second;
    } else {
      return _default;
    }
  } // get(with default)

  bool Properties::unset(const std::string& _key) {
    auto iEntry = m_Container.find(_key);
    if(iEntry != m_Container.end()) {
      m_Container.erase(iEntry);
      return true;
    }
    return false;
  } // unset

  std::string Properties::toString() const {
    std::string ret;
    const char *delim = "";
    foreach (auto&& value, m_Container) {
      ret += delim + value.first + ":" + value.second;
      delim = " ";
    }
    return ret;
  }
}
