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

#ifndef WIN32
  #include <ext/hash_map>
#else
  #include <hash_map>
#endif
#include <stdexcept>

#ifndef WIN32
#define HASH_NAMESPACE __gnu_cxx
#else
#definne HASH_NAMESPACE stdext;
#endif

#define HASH_MAP HASH_NAMESPACE::hash_map

namespace HASH_NAMESPACE
{
  template<>
  struct hash<const std::string> {
    size_t operator()(const std::string& x) const {
      return HASH_NAMESPACE::hash<const char*>()(x.c_str());
    }
  };
}

namespace dss {

  template<class t>
  bool contains(const std::vector<t>& _v, const t _item) {
    return find(_v.begin(), _v.end(), _item) != _v.end();
  }


  //============================================= Common types
  typedef HASH_MAP<std::string, std::string> HashMapStringString;
  typedef HASH_MAP<const std::string, std::string> HashMapConstStringString;

  //============================================= Conversion helpers

  int strToInt(const std::string& _strValue);

  unsigned int strToUInt(const std::string& _strValue);
  int strToIntDef(const std::string& _strValue, const int _default);

  std::string intToString(const int _int, const bool _hex = false);
  std::string uintToString(unsigned long int _int);

  double strToDouble(const std::string& _strValue);
  double strToDouble(const std::string& _strValue, const double _default);

  std::string unsignedLongIntToHexString(const unsigned long long _value);

  std::string doubleToString(const double _value);

  template <class t>
  t dateToISOString(const struct tm* _dateTime);
  struct tm dateFromISOString(const char* _dateTimeAsString);

  extern const char* theISOFormatString;

  std::vector<std::string> splitString(const std::string& _source, const char _delimiter, bool _trimEntries = false);
  void replaceAll(std::string& s, const std::string& a, const std::string& b);
  bool endsWith(const std::string& str, const std::string& searchString);
  bool beginsWith(const std::string& str, const std::string& searchString);

  std::string trim(const std::string& _str);
  std::string join(const std::vector<std::string>& _strings, const std::string& _delimiter);

  std::string urlDecode(const std::string& _in);

  //============================================= Encoding helpers

  uint16_t crc16(unsigned const char* _data, const int _size);
  uint16_t update_crc(uint16_t _crc, const unsigned char& c);


  //============================================= Helper classes

  class Properties {
  private:
    HashMapConstStringString m_Container;
  public:
    bool has(const std::string& _key) const;
    void set(const std::string& _key, const std::string& _value);
    const std::string& get(const std::string& _key) const;
    const std::string& get(const std::string& _key, const std::string& _default) const;

    bool unset(const std::string& _key);

    const HashMapConstStringString& getContainer() const { return m_Container; }
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

  std::string getTempDir();

  //============================================= Exception

  class DSSException : public std::runtime_error {
  public:
    DSSException(const std::string& _message)
      : runtime_error( _message )
    { }

    virtual ~DSSException() throw() {}
  }; // DSSException



}

#endif
