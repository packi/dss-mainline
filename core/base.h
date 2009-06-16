/*
 *  base.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/2/08.
 *  Copyright:
 *  (c) 2008 by
 *  futureLAB AG
 *  Schwalmenackerstrasse 4
 *  CH-8400 Winterthur / Schweiz
 *  Alle Rechte vorbehalten.
 *  Jede Art der Vervielfaeltigung, Verbreitung,
 *  Auswertung oder Veraenderung - auch auszugsweise -
 *  ist ohne vorgaengige schriftliche Genehmigung durch
 *  die futureLAB AG untersagt.
 *
 * Last change $Date$
 * by $Author$
 */

#ifndef BASE_H_INCLUDED
#define BASE_H_INCLUDED

#include <string>
#include <vector>
#include <cwchar>

#ifndef WIN32
  #include <ext/hash_map>
#else
  #include <hash_map>
#endif
#include <stdexcept>

#ifndef WIN32
using namespace __gnu_cxx;
#else
using namespace stdext;
#endif

namespace __gnu_cxx
{

  template<> struct hash<const wchar_t*>  {
    size_t operator()(const wchar_t* x) const {
      size_t result = static_cast<size_t>(12342349871239342341ULL);
      int len = wcslen(x);
      for(;len >= 0; len--) {
        result ^= static_cast<size_t>(*x++);
        result += static_cast<size_t>(987243059872341ULL);
      }
      return result;
    }
  };

  template<>
  struct hash< std::wstring >  {
    size_t operator()( const std::wstring& x ) const  {
      return hash< const wchar_t* >()( x.c_str() );
    }
  };

  template<>
  struct hash< const std::wstring > {
    size_t operator()( const std::wstring& x ) const {
      return hash< const wchar_t* >()( x.c_str() );
    }
  };

  template<>
  struct hash<const std::string> {
    size_t operator()(const std::string& x) const {
      return hash<const char*>()(x.c_str());
    }
  };
}


using namespace std;

namespace dss {


  template<class t>
  bool contains(const vector<t>& _v, const t _item) {
    return find(_v.begin(), _v.end(), _item) != _v.end();
  }


  //============================================= Common types
  typedef hash_map<wstring, wstring> HashMapWStringWString;
  typedef hash_map<const wstring, wstring> HashMapConstWStringWString;

  typedef hash_map<string, string> HashMapStringString;
  typedef hash_map<const string, string> HashMapConstStringString;
  //============================================= Conversion helpers

  int strToInt(const string& _strValue);

  unsigned int strToUInt(const string& _strValue);
  int strToIntDef(const string& _strValue, const int _default);

  string intToString(const int _int, const bool _hex = false);
  string uintToString(unsigned long int _int);

  double strToDouble(const string& _strValue);
  double strToDouble(const string& _strValue, const double _default);

  string unsignedLongIntToHexString(const unsigned long long _value);

  string doubleToString(const double _value);

  template <class t>
  t dateToISOString(const struct tm* _dateTime);
  struct tm dateFromISOString(const char* _dateTimeAsString);

  extern const char* theISOFormatString;

  vector<string> splitString(const string& _source, const char _delimiter, bool _trimEntries = false);
  void replaceAll(string& s, const string& a, const string& b);
  bool endsWith(const string& str, const string& searchString);
  bool beginsWith(const string& str, const string& searchString);

  string urlDecode(const string& _in);

  //============================================= Encoding helpers
  const wstring fromUTF8(const char* _utf8string, int _len);
  const string toUTF8(const wchar_t* _wcharString, int _len);

  const wstring fromUTF8(const string& _utf8string);
  const string toUTF8(const wstring& _wcharString);

  bool fileExists( const char* _fileName );
  bool fileExists( const string& _fileName );

  uint16_t crc16(unsigned const char* _data, const int _size);
  uint16_t update_crc(uint16_t _crc, const unsigned char& c);

  string trim(const string& _str);

  //============================================= Helper classes

  class Properties {
  private:
    HashMapConstStringString m_Container;
  public:
    bool has(const string& _key) const;
    void set(const string& _key, const string& _value);
    const string& get(const string& _key) const;
    const string& get(const string& _key, const string& _default) const;

    bool unset(const string& _key);

    const HashMapConstStringString& getContainer() const { return m_Container; }
  };

  template <typename resCls>
  class ResourceHolder {
  protected:
    resCls* m_Resource;

    resCls* release() {
      resCls* result = m_Resource;
      m_Resource = NULL;
      return result;
    }
  public:
    explicit ResourceHolder(resCls* _resource = NULL) : m_Resource(_resource) {}

    ResourceHolder(ResourceHolder<resCls>& _other)
    : m_Resource(_other.release())
    {
    }

    ResourceHolder<resCls>& operator=(ResourceHolder<resCls>& _rhs) {
      m_Resource = _rhs.release();
      return *this;
    }

  }; // ResourceHolder

  class Finalizable {
  public:
    virtual void finalize() = 0;
    virtual ~Finalizable() {}
  }; // Finalizable

  class Finalizer {
  private:
    Finalizable& m_Finalizable;
  public:
    Finalizer(Finalizable& _finalizable)
    : m_Finalizable(_finalizable)
    {}

    virtual ~Finalizer() {
      m_Finalizable.finalize();
    }
  };

  template<class t>
  void scrubVector(vector<t*>& _vector) {
    while(!_vector.empty()) {
      t* elem = *_vector.begin();
      _vector.erase(_vector.begin());
      delete elem;
    }
  } // ScrubVector

  void sleepSeconds( const unsigned int _seconds );
  void sleepMS( const unsigned int _ms );

  //============================================= Exception

  class DSSException : public runtime_error {
  public:
    DSSException(const string& _message)
      : runtime_error( _message )
    { }

    virtual ~DSSException() throw() {}
  }; // DSSException



}

#endif
