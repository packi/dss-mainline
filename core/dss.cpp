/*
 *  dss.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/2/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "dss.h"
#include "logger.h"
#include "xmlwrapper.h"
#include "modeljs.h"

#include <cassert>
#include <string>
#include <iostream>

using namespace std;

namespace dss {
  
  //============================================ EventRunner
  
  const bool DebugEventRunner = false;
   
  int EventRunner::GetSize() const {
    return m_ScheduledEvents.size();
  } // GetSize
  
  const ScheduledEvent& EventRunner::GetEvent(const int _idx) const {
    return m_ScheduledEvents.at(_idx);
  } // GetEvent
  
  void EventRunner::RemoveEvent(const int _idx) {
    boost::ptr_vector<ScheduledEvent>::iterator it = m_ScheduledEvents.begin();
    advance(it, _idx);
    m_ScheduledEvents.erase(it);    
  } // RemoveEvent
  
  void EventRunner::AddEvent(ScheduledEvent* _scheduledEvent) {
    m_ScheduledEvents.push_back(_scheduledEvent);
    m_NewItem.Signal();
  } // AddEvent
  
  DateTime EventRunner::GetNextOccurence() {
    DateTime now;
    DateTime result = now.AddYear(10);
    if(DebugEventRunner) {
      cout << "*********" << endl;
    }
    for(boost::ptr_vector<ScheduledEvent>::iterator ipSchedEvt = m_ScheduledEvents.begin(), e = m_ScheduledEvents.end();
        ipSchedEvt != e; ++ipSchedEvt) 
    {
      DateTime next = ipSchedEvt->GetSchedule().GetNextOccurence(now);
      if(DebugEventRunner) {
        cout << "next:   " << next << endl;
        cout << "result: " << result << endl;
      }
      result = min(result, next);
      if(DebugEventRunner) {
        cout << "chosen: " << result << endl;
      }
    }
    return result;
  }
  
  void EventRunner::Execute() {
    while(!m_Terminated) {
      if(m_ScheduledEvents.empty()) {
        m_NewItem.WaitFor(1000);
      } else {
        DateTime now;
        m_WakeTime = GetNextOccurence();
        int sleepSeconds = m_WakeTime.Difference(now);
       
        // Prevent loops when a cycle takes less than 1s
        if(sleepSeconds == 0) {
          m_NewItem.WaitFor(1000);
          continue;
        }
          
        if(!m_NewItem.WaitFor(sleepSeconds * 1000)) {
          RaisePendingEvents(m_WakeTime, 10);
        }
      }
    }
  } // Execute
  
  
  void EventRunner::RaisePendingEvents(DateTime& _from, int _deltaSeconds) {
    DateTime virtualNow = _from.AddSeconds(-_deltaSeconds/2);
    if(DebugEventRunner) {
      cout << "vNow:    " << virtualNow << endl;
    }
    for(boost::ptr_vector<ScheduledEvent>::iterator ipSchedEvt = m_ScheduledEvents.begin(), e = m_ScheduledEvents.end();
        ipSchedEvt != e; ++ipSchedEvt) 
    {
      DateTime nextOccurence = ipSchedEvt->GetSchedule().GetNextOccurence(virtualNow);
      if(DebugEventRunner) {
        cout << "nextOcc: " << nextOccurence << endl;
        cout << "diff:    " << nextOccurence.Difference(virtualNow) << endl;
      }
      if(abs(nextOccurence.Difference(virtualNow)) <= _deltaSeconds/2) {
        DSS::GetInstance()->GetApartment().OnEvent(ipSchedEvt->GetEvent());
      }
    }    
  } // RaisePendingEvents

  //============================================= DSS
  
  DSS* DSS::m_Instance = NULL;
  
  DSS* DSS::GetInstance() {
    if(m_Instance == NULL) {
      m_Instance = new DSS();
    }
    assert(m_Instance != NULL);
    return m_Instance;
  } // GetInstance
  
  void DSS::Run() {
    Logger::GetInstance()->Log("DSS stating up....", lsInfo);
    LoadConfig();
    m_WebServer.Initialize(m_Config);
    m_WebServer.Run();
    
    m_ModulatorSim.Initialize();
    m_Apartment.Run();
    
    boost::shared_ptr<Event> evt(new Event(1003, 0));
    DateTime startingTime;
    startingTime.SetSecond(0);
    boost::shared_ptr<RepeatingSchedule> sch(new RepeatingSchedule(Minutely, 1, startingTime));
        
    ScheduledEvent* schEvt = new ScheduledEvent(evt, sch);
    schEvt->SetName("Run test2.js every minute");
    m_EventRunner.AddEvent(schEvt);
    
    ActionJS jsAction;
    Arguments args;
    args.SetValue("script", "/Users/packi/sources/dss/trunk/data/test2.js");
    vector<int> eventIDs;
    eventIDs.push_back(1003);
    Subscription& sub = m_Apartment.Subscribe(jsAction, args, eventIDs);
    sub.SetName("I'm a subscription");

    while(true) {
      sleep(1000);
    }
  } // Run
  
  void DSS::LoadConfig() {
    Logger::GetInstance()->Log("Loading config", lsInfo);
    m_Config.ReadFromXML("/Users/packi/sources/dss/trunk/data/config.xml");
  } // LoadConfig

  //============================================= WebServer
  
  WebServer::WebServer() 
  : Thread(true, "WebServer")
  {
    m_SHttpdContext = shttpd_init();
  } // ctor
  
  WebServer::~WebServer() {
    shttpd_fini(m_SHttpdContext);
  } // dtor
  
  void WebServer::Initialize(Config& _config) {
    string ports = _config.GetOptionAs("webserverport", "8080");
    Logger::GetInstance()->Log(string("Webserver: Listening on port(s) ") + ports);
    shttpd_set_option(m_SHttpdContext, "ports", ports.c_str());

    string aliases = string("/=") + _config.GetOptionAs<string>("webserverroot", "/Users/packi/sources/dss/data/");
    Logger::GetInstance()->Log(string("Webserver: Configured aliases: ") + aliases);
    shttpd_set_option(m_SHttpdContext, "aliases", aliases.c_str());    

    shttpd_register_uri(m_SHttpdContext, "/config", &HTTPListOptions, NULL);    
    shttpd_register_uri(m_SHttpdContext, "/json/*", &JSONHandler, NULL);
  } // Initialize
  
  void WebServer::Execute() {
    Logger::GetInstance()->Log("Webserver started", lsInfo);
    while(!m_Terminated) {
      shttpd_poll(m_SHttpdContext, 1000);
    }
  } // Execute
  
  void WebServer::EmitHTTPHeader(int _code, struct shttpd_arg* _arg, string _contentType) {
    stringstream sstream;
    sstream << "HTTP/1.1 " << _code << " OK\r\n";
    sstream << "Content-Type: " << _contentType << "; charset=utf-8\r\n\r\n";
    shttpd_printf(_arg, sstream.str().c_str());
  } // EmitHTTPHeader

  
  HashMapConstStringString ParseParameter(const char* _params) {
    HashMapConstStringString result;
    if(_params != NULL) {
      vector<string> paramList = SplitString(_params, '&');
      for(vector<string>::iterator iParam = paramList.begin(); iParam < paramList.end(); ++iParam) {
        vector<string> nameValue = SplitString(*iParam, '=');
        if(nameValue.size() != 2) {
          result[*iParam] = "";
        } else {
          result[URLDecode(nameValue.at(0))] = URLDecode(nameValue.at(1));
        }
      }
    }
    return result;
  } // ParseParameter
    
  template<class t>
  string ToJSONValue(const t& _value);
  
  template<>
  string ToJSONValue(const int& _value) {
    return IntToString(_value);
  } // ToJSONValue
  
  template<>
  string ToJSONValue(const string& _value) {
    return string("\"") + _value + '"';
  } // ToJSONValue

  template<class t>
  string ToJSONArray(const vector<t>& _v);
  
  template<>
  string ToJSONArray(const vector<int>& _v) {
    stringstream arr;
    arr << "[";
    bool first = true;
    vector<int>::const_iterator iV;
    vector<int>::const_iterator e;
    for(iV = _v.begin(), e = _v.end();
        iV != e; ++iV)
    {
      if(!first) {
        arr << ",";
      }
      arr << ToJSONValue(*iV);
      first = false;
    }
    arr << "]";
    return arr.str();
  } // ToJSONArray<int>

  
  void WebServer::JSONHandler(struct shttpd_arg* _arg) {
    const string urlid = "/json/";
    string uri = shttpd_get_env(_arg, "REQUEST_URI");
    HashMapConstStringString paramMap = ParseParameter(shttpd_get_env(_arg, "QUERY_STRING"));

    string method = uri.substr(uri.find(urlid) + urlid.size());
    
    if(method == "getdevices") {
      EmitHTTPHeader(200, _arg, "application/json");
      Set devices = DSS::GetInstance()->GetApartment().GetDevices();
      
      stringstream sstream;
      sstream << "{\"devices\":[";
      bool first = true;
      for(int iDevice = 0; iDevice < devices.Length(); iDevice++) {
        DeviceReference& d = devices.Get(iDevice);
        if(first) {
          first = false;
        } else {
          sstream << ",";
        }
        sstream << "{ \"id\": " << d.GetID() 
                << ", \"name\": \"" << d.GetDevice().GetName() 
                << "\", \"on\": " << (d.IsOn() ? "true" : "false") << " }";        
      }
      sstream << "]}";
        
      shttpd_printf(_arg, sstream.str().c_str());
    } else if(method == "turnon") {
      EmitHTTPHeader(200, _arg, "application/json");
      
      string devidStr = paramMap["device"];
      if(!devidStr.empty()) {
        int devid = StrToInt(devidStr);
        DSS::GetInstance()->GetApartment().GetDeviceByID(devid).TurnOn();
        shttpd_printf(_arg, "{ok:1}");
      } else {
        shttpd_printf(_arg, "{ok:0}");
      }
    } else if(method == "turnoff") {
      
      string devidStr = paramMap["device"];
      if(!devidStr.empty()) {
        int devid = StrToInt(devidStr);
        DSS::GetInstance()->GetApartment().GetDeviceByID(devid).TurnOff();
        shttpd_printf(_arg, "{ok:1}");
      } else {
        shttpd_printf(_arg, "{ok:0}");
      }
    } else if(BeginsWith(method, "subscription/")) {
      if(method == "subscription/getlist") {
        EmitHTTPHeader(200, _arg, "application/json");

        stringstream response;
        response << "{ \"subscriptions\":[";
        
        bool first = true;
        int numSubscriptions = DSS::GetInstance()->GetApartment().GetSubscriptionCount();
        for(int iSubscription = 0; iSubscription < numSubscriptions; iSubscription++) {
          Subscription& sub = DSS::GetInstance()->GetApartment().GetSubscription(iSubscription);
          if(!first) {
            response << ",";
          }
          response << "{ \"id\":" << sub.GetID() << "," 
                   <<   "\"name\":" << ToJSONValue(sub.GetName()) << ","
                   <<   "\"evtids\":" << ToJSONArray<int>(sub.GetEventIDs()) << ","
                   <<   "\"srcids\":" << ToJSONArray<int>(sub.GetSourceIDs())
                   << "}";
          first = false;
        }
        response << "]}";
        
        shttpd_printf(_arg, response.str().c_str());
      } else if(method == "subscription/unsubscribe") {
        EmitHTTPHeader(200, _arg, "application/json");
        
        if(!paramMap["id"].empty()) {
          int subscriptionID = StrToInt(paramMap["id"]);
          
          DSS::GetInstance()->GetApartment().Unsubscribe(subscriptionID);
          
          shttpd_printf(_arg, "{ok:1}");        
        } else {
          shttpd_printf(_arg, "{ok:0}");        
        }
        
      } else if(method == "subscription/subscribe") {
        EmitHTTPHeader(200, _arg, "application/json");
        
        string evtIDsString = paramMap["evtids"];
        string sourceIDsString = paramMap["sourceids"];
        string actionName = paramMap["action"];
        string fileParam = paramMap["file"];
      }
    } else if(BeginsWith(method, "event/")) {
      EmitHTTPHeader(200, _arg, "application/json");
      
      
      if(method == "event/getlist") {
        stringstream response;
        response << "{ \"events\":[";
        bool first = true;
        int numEvents = DSS::GetInstance()->GetEventRunner().GetSize();
        for(int iEvent = 0; iEvent < numEvents; iEvent++) {
          const ScheduledEvent& schedEvt = DSS::GetInstance()->GetEventRunner().GetEvent(iEvent);
          if(!first) {
            response << ",";
          }
          response << "{ id: " << iEvent << "," 
                   <<   "\"evtid\":" << schedEvt.GetEvent().GetID() << ","
                   <<   "\"sourceid\":" << schedEvt.GetEvent().GetSource() << ","
                   <<   "\"name\": \"" << schedEvt.GetName() << "\""
                   << "}";
          first = false;
        }
        response << "]}";
        
        shttpd_printf(_arg, response.str().c_str());
      } else if(method == "event/remove") {
        int eventID = StrToInt(paramMap["id"]);
        DSS::GetInstance()->GetEventRunner().RemoveEvent(eventID);
        shttpd_printf(_arg, "{ok:1}");        
      } else if(paramMap["evtid"].empty() || paramMap["sourceid"].empty()) {
        shttpd_printf(_arg, "{ok:0}");
      } else {
        int eventID = StrToInt(paramMap["evtid"]);
        int sourceID = StrToInt(paramMap["sourceid"]);
        if(method == "event/raise") {
          Event evt(eventID, sourceID);
          DSS::GetInstance()->GetApartment().OnEvent(evt);
          shttpd_printf(_arg, "{ok:1}");
        } else if(method == "event/schedule") {
          if(paramMap["schedule"].empty() || paramMap["start"].empty()) {
            shttpd_printf(_arg, "{ok:0}");
          } else {
            string schedule = paramMap["schedule"];
            string startTimeISO = paramMap["start"];
            string name = paramMap["name"];
            
            boost::shared_ptr<Event> evt(new Event(eventID, sourceID));
            boost::shared_ptr<Schedule> sch(new ICalSchedule(schedule, startTimeISO));
            
            ScheduledEvent* scheduledEvent = new ScheduledEvent(evt, sch);
            if(!name.empty()) {
              scheduledEvent->SetName(name);
            }
            
            DSS::GetInstance()->GetEventRunner().AddEvent(scheduledEvent);
            shttpd_printf(_arg, "{ok:1}");
          }          
        } else {
          shttpd_printf(_arg, "{ok:0}");
        }
      }
    } else {
     // shttpd_printf(_arg, "hello %s, method %s, params %s", uri.c_str(), method.c_str(), params.c_str());
    }
    _arg->flags |= SHTTPD_END_OF_OUTPUT;
  } // JSONHandler
  
  void WebServer::HTTPListOptions(struct shttpd_arg* _arg) {
    EmitHTTPHeader(200, _arg);
    shttpd_printf(_arg, "%s", "<html><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><body>");
    shttpd_printf(_arg, "%s", "<h1>Configuration</h1>");
    
    stringstream stream;
    stream << "<ul>";
    
    const HashMapConstStringString& options = DSS::GetInstance()->GetConfig().GetOptions();
    for(HashMapConstStringString::const_iterator iOption = options.begin(); iOption != options.end(); ++iOption) {
      stream << "<li>" << iOption->first << " = " << iOption->second << "</li>";
    }
    stream << "</ul></body></html>";
    shttpd_printf(_arg, stream.str().c_str());
    
    _arg->flags |= SHTTPD_END_OF_OUTPUT;
  } // HTTPListOptions
  
  //============================================= Config
  
  bool Config::HasOption(const string& _name) const {
    return m_OptionsByName.find(_name) != m_OptionsByName.end();
  } // HasOption
  
  void Config::AssertHasOption(const string& _name) const {
    if(!HasOption(_name)) {
      throw new NoSuchOptionException(_name);
    }
  } // AssertHasOption
  
  const int ConfigVersion = 1;

  void Config::ReadFromXML(const string& _fileName) {
    XMLDocumentFileReader reader(_fileName);
    XMLNode rootNode = reader.GetDocument().GetRootNode();
    if(rootNode.GetName() == "config") {
      if(StrToInt(rootNode.GetAttributes()["version"]) == ConfigVersion) {
        Logger::GetInstance()->Log("Right, version, going to read items");
        
        XMLNodeList items = rootNode.GetChildren();
        for(XMLNodeList::iterator it = items.begin(); it != items.end(); ++it) {
          if(it->GetName() == "item") {
            try {
              XMLNode& nameNode = it->GetChildByName("name");
              if(nameNode.GetChildren().size() == 0) {
                continue;
              }
              nameNode = (nameNode.GetChildren())[0];
              
              XMLNode& valueNode = it->GetChildByName("value");
              if(valueNode.GetChildren().size() == 0) {
                continue;
              }
              valueNode = (valueNode.GetChildren())[0];
              m_OptionsByName[nameNode.GetContent()] = valueNode.GetContent();
            } catch(XMLException& _e) {
              Logger::GetInstance()->Log(string("Error loading XML-File: ") + _e.what(), lsError);
            }
          }
        }
      }
    }
  } // ReadFromXML
  
  template <>
  int Config::GetOptionAs(const string& _name) const {
    AssertHasOption(_name);

    return StrToInt(m_OptionsByName[_name]);
  } // GetOptionAs<int>
  
  template <>
  string Config::GetOptionAs(const string& _name) const {
    AssertHasOption(_name);
    
    return m_OptionsByName[_name];
  } // GetOptionAs<string>
                                          
  template <>
  const char* Config::GetOptionAs(const string& _name) const {
    return GetOptionAs<string>(_name).c_str();
  } // GetOptionAs<const char*>
  
}