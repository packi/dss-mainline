#include "webserver.h"
#include "logger.h"
#include "model.h"
#include "dss.h"
#include "shttpd.h"

#include <boost/shared_ptr.hpp>

namespace dss {
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

    string aliases = string("/=") + _config.GetOptionAs<string>("webserverroot", "data/");
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
        sstream << "{ \"id\": " << d.GetDSID() 
                << ", \"name\": \"" << d.GetDevice().GetName() 
                << "\", \"on\": " << (d.IsOn() ? "true" : "false") << " }";        
      }
      sstream << "]}";
        
      shttpd_printf(_arg, sstream.str().c_str());
    } else if(method == "turnon") {
      EmitHTTPHeader(200, _arg, "application/json");
      
      string devidStr = paramMap["device"];
      if(!devidStr.empty()) {
        int devid = StrToUInt(devidStr);
        DSS::GetInstance()->GetApartment().GetDeviceByDSID(devid).TurnOn();
        shttpd_printf(_arg, "{ok:1}");
      } else {
        shttpd_printf(_arg, "{ok:0}");
      }
    } else if(method == "turnoff") {
      
      string devidStr = paramMap["device"];
      if(!devidStr.empty()) {
        int devid = StrToUInt(devidStr);
        DSS::GetInstance()->GetApartment().GetDeviceByDSID(devid).TurnOff();
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
  
}
