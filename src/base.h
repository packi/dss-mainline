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

#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED

#include <string>
#include <vector>
#include <stdint.h>
#include <type_traits>
#include <unordered_map>

// well known macro mapped to the state-of-the-art C++ construct
#define ARRAY_SIZE(x) std::extent<decltype(x)>::value

namespace dss {

  template<class t>
  bool contains(const std::vector<t>& _v, const t _item) {
    return find(_v.begin(), _v.end(), _item) != _v.end();
  }

  //============================================= Conversion helpers

  int strToInt(const std::string& _strValue);

  unsigned int strToUInt(const std::string& _strValue);
  int strToIntDef(const std::string& _strValue, const int _default);
  unsigned int strToUIntDef(const std::string& _strValue, const unsigned int _default);
  unsigned long long strToULongLong(const std::string& _strValue);
  unsigned long long strToULongLongDef(const std::string& _strValue, const unsigned int _default);

  std::string intToString(const long long, const bool _hex = false);
  std::string uintToString(const long long unsigned, bool _hex = false);

  double strToDouble(const std::string& _strValue);
  double strToDouble(const std::string& _strValue, const double _default);

  std::string unsignedLongIntToHexString(const unsigned long long _value);
  std::string unsignedLongIntToString(const unsigned long long _value);

  std::string doubleToString(const double _value);
  double roundDigits(const double input, const int digits);

  std::vector<std::string> splitString(const std::string& _source, const char _delimiter, bool _trimEntries = false);
  void replaceAll(std::string& s, const std::string& a, const std::string& b);
  bool endsWith(const std::string& str, const std::string& searchString);
  bool beginsWith(const std::string& str, const std::string& searchString);

  std::pair<std::string, std::string> splitIntoKeyValue(const std::string& _keyValue);

  std::string trim(const std::string& _str);
  std::string join(const std::vector<std::string>& _strings, const std::string& _delimiter);
  /**
   * car(/foo/bar/baz) -> ('foo', 'bar/baz')
   * https://en.wikipedia.org/wiki/CAR_and_CDR
   */
  std::string carCdrPath(std::string &path);

  std::string urlDecode(const std::string& _in);
  std::string urlEncode(const std::string& _in);

  std::string truncateUTF8String(const std::string& _in, int _maxBytes);

  std::string hexEncodeByteArray(const unsigned char *ar, unsigned int len);
  inline std::string hexEncodeByteArray(const std::vector<unsigned char> &v) {
    return hexEncodeByteArray(&v[0], v.size());
  }

  //============================================= Encoding helpers

  uint16_t crc16(unsigned const char* _data, const int _size);
  uint16_t update_crc(uint16_t _crc, const unsigned char& c);

  //============================================= Filesystem helpers

  bool rwAccess(const std::string& _filename);

  ///< read whole file into string
  /// throws exception
  std::string readFile(const std::string& filename);

  //============================================= Helper classes

  class Properties {
  private:
    std::unordered_map<std::string, std::string> m_Container;
  public:
    bool has(const std::string& _key) const;
    void set(const std::string& _key, const std::string& _value);
    const std::string& get(const std::string& _key) const;
    const std::string& get(const std::string& _key, const std::string& _default) const;

    bool unset(const std::string& _key);

    const std::unordered_map<std::string, std::string>& getContainer() const { return m_Container; }
    std::string toString() const;
  };

  template<class t>
  void scrubVector(std::vector<t*>& _vector) {
    while(!_vector.empty()) {
      t* elem = *_vector.begin();
      _vector.erase(_vector.begin());
      delete elem;
    }
  } // scrubVector

  void sleepSeconds( const unsigned int _seconds );
  void sleepMS( const unsigned int _ms );

  std::string addTrailingBackslash(const std::string& _path);
  std::string getTempDir();
  bool syncFile(const std::string& _path);
  std::string doIndent(const int _indent);
  std::string XMLStringEscape(const std::string& str);
}

#endif
