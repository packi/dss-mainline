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

#include <bitset>

#include "base.h"
#include "thread.h"
#include "shttpd.h"
#include "defs.h"
#include "ds485proxy.h"
#include "syncevent.h"

#include <cstdio>
#include <string>
#include <sstream>

using namespace std;

#include <boost/ptr_container/ptr_vector.hpp>

namespace dss {

  class DSS;
  class WebServer;
  class Config;
  class EventRunner;
  
  class WebServer : public Thread {
  private:
    struct shttpd_ctx* m_SHttpdContext;
  protected:
    static void JSONHandler(struct shttpd_arg* _arg);
    static void HTTPListOptions(struct shttpd_arg* _arg);
    static void EmitHTTPHeader(int _code, struct shttpd_arg* _arg, const string _contentType = "text/html");
  public:
    WebServer();
    ~WebServer();
    
    void Initialize(Config& _config);
    
    virtual void Execute();
  };
  
  class Config {
  private:
    // this field has to be mutable because accessing it by operator[] is non-constw
    mutable HashMapConstStringString m_OptionsByName; 
  public:
    bool HasOption(const string& _name) const;
    void ReadFromXML(const string& _fileName);
    
    void AssertHasOption(const string& _name) const;

    template <class t>
    t GetOptionAs(const string& _name) const;

    template <class t>
    t GetOptionAs(const string& _name, const t _defaultValue) const {
      if(HasOption(_name)) {
        return GetOptionAs<t>(_name);
      }
      return _defaultValue;
    }    
    
    template <class t>
    void SetOptionAs(const string& _name, const t& _value) {
      stringstream strstream;
      strstream << _value;
      m_OptionsByName[_name] = strstream.str();
    } // SetOptionAs<t>
    
    const HashMapConstStringString& GetOptions() { return m_OptionsByName; };
  }; // Config
  
  class EventRunner : Thread {
  private:
    boost::ptr_vector<ScheduledEvent> m_ScheduledEvents;
    
    DateTime GetNextOccurence();
    DateTime m_WakeTime;
    SyncEvent m_NewItem;
  public:
    void AddEvent(ScheduledEvent* _scheduledEvent);
    
    void RaisePendingEvents(DateTime& _from, int _deltaSeconds);
    
    int GetSize() const;
    const ScheduledEvent& GetEvent(const int _idx) const;
    void RemoveEvent(const int _idx);    
    
    virtual void Execute();
  }; // EventRunner
    

  /** Main class
    * 
    */
  class DSS {
  private:
    static DSS* m_Instance;
    WebServer m_WebServer;
    Config m_Config;
    DS485Proxy m_DS485Proxy;
    Apartment m_Apartment;
    DSModulatorSim m_ModulatorSim;
    EventRunner m_EventRunner;
    
    DSS() { };
    
    void LoadConfig();
  public:
    void Run();
    
    static DSS* GetInstance();
    Config& GetConfig() { return m_Config; };
    DS485Proxy& GetDS485Proxy() { return m_DS485Proxy; };
    Apartment& GetApartment() { return m_Apartment; };
    DSModulatorSim& GetModulatorSim() { return m_ModulatorSim; };
    EventRunner& GetEventRunner() { return m_EventRunner; };
  }; // DSS
  
  class NoSuchOptionException : DSSException {
  private:
    const string m_OptionName;
  public:
    NoSuchOptionException(const string& _optionName) 
    : DSSException( string("Item not found:") ),
      m_OptionName(_optionName)
    {};
    
    ~NoSuchOptionException() throw() {};
    
    const string& GetOptionName() {
      return m_OptionName;
    } // GetOptionName
  }; // NoSuchOptionException
  
}

#endif // DSS_H_INLUDED

