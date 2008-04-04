/*
 *  base.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/2/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
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
                                                                                
  template<> struct hash< std::wstring >                                                       
  {                                                                                           
    size_t operator()( const std::wstring& x ) const                                           
    {                                                                                         
      return hash< const wchar_t* >()( x.c_str() );                                              
    }                                                                                         
  };                                                                                          
  template<> struct hash< const std::wstring >                                                       
  {                                                                                           
    size_t operator()( const std::wstring& x ) const                                           
    {                                                                                         
      return hash< const wchar_t* >()( x.c_str() );                                              
    }                                                                                         
  };          
}      


using namespace std;

namespace dss {
  
  
  //============================================= Common types
  typedef hash_map<wstring, wstring> HashMapWStringWString;
  typedef hash_map<const wstring, wstring> HashMapConstWStringWString;
  //============================================= Encoding helpers
  template<class strclass>
  int StrToInt(const strclass& _strValue);
  
  template<typename strclass>
  int StrToInt(const strclass* _strValue);

  //============================================= Encoding helpers
  const wstring FromUTF8(const char* _utf8string, int _len);
  const string ToUTF8(const wchar_t* _wcharString, int _len);
  
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
  }; // DSSException
  
}

#endif