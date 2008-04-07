/*
 *  dss.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/2/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef DSS_H_INCLUDED
#define DSS_H_INCLUDED

#include "base.h"
#include "thread.h"

#include <cstdio>
#include <string>
#include <sstream>

#include "shttpd.h"
#include "defs.h"


using namespace std;

namespace dss {

  class DSS;
  class WebServer;
  class Config;
  
  class WebServer : public Thread {
  private:
    struct shttpd_ctx* m_SHttpdContext;
  protected:
    static void HTTPListOptions(struct shttpd_arg* _arg);
    static void EmitHTTPHeader(int _code, struct shttpd_arg* _arg);
  public:
    WebServer();
    ~WebServer();
    
    void Initialize(Config& _config);
    
    virtual void Execute();
  };
  
  class Config {
  private:
    // this field has to be mutable because accessing it by operator[] is non-constw
    mutable HashMapConstWStringWString m_OptionsByName; 
  public:
    bool HasOption(const wstring& _name) const;
    void ReadFromXML(const wstring& _fileName);
    
    void AssertHasOption(const wstring& _name) const;

    template <class t>
    t GetOptionAs(const wstring& _name) const;

    template <class t>
    t GetOptionAs(const wstring& _name, const t _defaultValue) const {
      if(HasOption(_name)) {
        return GetOptionAs<t>(_name);
      }
      return _defaultValue;
    }    
    
    template <class t>
    void SetOptionAs(const wstring& _name, const t& _value) {
      wstringstream strstream;
      strstream << _value;
      m_OptionsByName[_name] = strstream.str();
    } // SetOptionAs<t>
    
    const HashMapConstWStringWString& GetOptions() { return m_OptionsByName; };
  }; // Config

  class DSS {
  private:
    static DSS* m_Instance;
    WebServer m_WebServer;
    Config m_Config;
    
    DSS() { };
    
    void LoadConfig();
  public:
    void Run();
    
    static DSS* GetInstance();
    Config& GetConfig() { return m_Config; };
  }; // DSS
  
  class NoSuchOptionException : DSSException {
  private:
    const wstring m_OptionName;
  public:
    NoSuchOptionException(const wstring& _optionName) 
    : DSSException( string("Item not found:") ),
      m_OptionName(_optionName)
    {};
    
    ~NoSuchOptionException() throw() {};
    
    const wstring& GetOptionName() {
      return m_OptionName;
    } // GetOptionName
  }; // NoSuchOptionException
  
}

#endif DSS_H_INLUDED