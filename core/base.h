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
  bool Contains(const vector<t>& _v, const t _item) {
    return find(_v.begin(), _v.end(), _item) != _v.end();
  }
    
  
  //============================================= Common types
  typedef hash_map<wstring, wstring> HashMapWStringWString;
  typedef hash_map<const wstring, wstring> HashMapConstWStringWString;
  
  typedef hash_map<string, string> HashMapStringString;
  typedef hash_map<const string, string> HashMapConstStringString;
  //============================================= Conversion helpers
  template<class strclass>
  int StrToInt(const strclass& _strValue);
  
  template<typename strclass>
  int StrToInt(const strclass* _strValue);
  
  string IntToString(const int _int);
  
  template <class t>
  t DateToISOString( struct tm* _dateTime );

  //============================================= Encoding helpers
  const wstring FromUTF8(const char* _utf8string, int _len);
  const string ToUTF8(const wchar_t* _wcharString, int _len);
  
  const wstring FromUTF8(const string& _utf8string);
  const string ToUTF8(const wstring& _wcharString);
  
  bool FileExists( const char* _fileName );
  bool FileExists( const string& _fileName );
  
  //============================================= Helper classes
  
  template <typename resCls>
  class ResourceHolder {
  protected:
    resCls* m_Resource;

    resCls* Release() {
      resCls* result = m_Resource;
      m_Resource = NULL;
      return result;
    }
  public:
    explicit ResourceHolder(resCls* _resource = NULL) : m_Resource(_resource) {};
    
    ResourceHolder(ResourceHolder<resCls>& _other)
    : m_Resource(_other.Release())
    {
    }
    
    ResourceHolder<resCls>& operator=(ResourceHolder<resCls>& _rhs) {
      m_Resource = _rhs.Release();
      return *this;
    }

  }; // ResourceHolder
  
  class Finalizable {
  public:
    virtual void Finalize() = 0;
  }; // Finalizable
  
  class Finalizer {
  private:
    Finalizable& m_Finalizable;
  public:
    Finalizer(Finalizable& _finalizable)
    : m_Finalizable(_finalizable)
    {}
    
    ~Finalizer() {
      m_Finalizable.Finalize();
    }
  };
  
  //============================================= Exception
  
  class DSSException : public runtime_error {
  public:
    DSSException(const string& _message) 
      : runtime_error( _message )
    { }
    
    virtual ~DSSException() throw() {};
  }; // DSSException
  
}

#endif