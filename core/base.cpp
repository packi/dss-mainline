/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland
    Copyright (c) 2008 Patrick Staehlin <me@packi.ch>

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

#include "base.h"

#include <sys/stat.h>
#include <cstring>
#include <sstream>

#include "foreach.h"

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
    sstream << _value;
    return sstream.str();
  } // doubleToString

  std::string intToString(const int _int, bool _hex) {
    std::stringstream sstream;
    if(_hex) {
      sstream << std::hex << "0x";
    }
    sstream << _int;
    return sstream.str();
  } // intToString

  std::string uintToString(unsigned long int _int) {
    std::stringstream sstream;
    sstream << _int;
    return sstream.str();
  } // uintToString

  std::string unsignedLongIntToHexString(const unsigned long long _value) {
    std::stringstream sstream;
    sstream << std::hex << _value;
    return sstream.str();
  }

  const char* theISOFormatString = "%Y-%m-%d %H:%M:%S";

  template <>
  std::string dateToISOString( const struct tm* _dateTime ) {
    char buf[ 20 ];
    strftime( buf, 20, theISOFormatString, _dateTime );
    std::string result = buf;
    return result;
  } // dateToISOString


  struct tm dateFromISOString( const char* _dateTimeAsString ) {
    struct tm result;
    memset(&result, '\0', sizeof(result));
    strptime( _dateTimeAsString, theISOFormatString, &result );
    return result;
  } // dateFromISOString

  std::vector<std::string> splitString(const std::string& _source, const char _delimiter, bool _trimEntries) {
    std::vector<std::string> result;
    std::string curString = _source;
    while(!curString.empty()) {
      std::string::size_type delimPos = curString.find(_delimiter, 0);
      if(_trimEntries) {
        result.push_back(trim(curString.substr(0, delimPos)));
      } else {
        result.push_back(curString.substr(0, delimPos));
      }
      if(delimPos != std::string::npos) {
        curString = curString.substr(delimPos+1, std::string::npos);
        if(curString.size() == 0) {
          result.push_back("");
        }
      } else {
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
  }

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

  //================================================== System utilities

  void sleepSeconds( const unsigned int _seconds ) {
#ifdef WIN32
    Sleep( _seconds * 1000 );
#else
    sleep( _seconds );
#endif
  } // sleepSeconds

  void sleepMS( const unsigned int _ms ) {
#ifdef WIN32
    Sleep( _ms );
#else
    usleep( _ms * 1000 );
#endif
  }

  //================================================== Properties

  bool Properties::has(const std::string& _key) const {
    HashMapConstStringString::const_iterator iEntry = m_Container.find(_key);
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
      HashMapConstStringString::const_iterator iEntry = m_Container.find(_key);
      return iEntry->second;
    }
  } // get

  const std::string& Properties::get(const std::string& _key, const std::string& _default) const {
    if(has(_key)) {
      // not using operator[] here because its non-const
      HashMapConstStringString::const_iterator iEntry = m_Container.find(_key);
      return iEntry->second;
    } else {
      return _default;
    }
  } // get(with default)

  bool Properties::unset(const std::string& _key) {
    HashMapConstStringString::iterator iEntry = m_Container.find(_key);
    if(iEntry != m_Container.end()) {
      m_Container.erase(iEntry);
      return true;
    }
    return false;
  } // unset


}
