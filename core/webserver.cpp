#include "webserver.h"
#include "logger.h"
#include "model.h"
#include "dss.h"
#include "event.h"
#include "unix/ds485proxy.h"
#include "sim/dssim.h"
#include "propertysystem.h"

#include <iostream>
#include <boost/shared_ptr.hpp>

namespace dss {
  //============================================= WebServer

  WebServer::WebServer(DSS* _pDSS)
  : Subsystem(_pDSS, "WebServer"),
    Thread("WebServer")
  {
    Logger::GetInstance()->Log("Starting Webserver...");
    m_SHttpdContext = shttpd_init();
    DSS::GetInstance()->GetPropertySystem().SetStringValue("/config/webserver/ports", "8080", true);
    DSS::GetInstance()->GetPropertySystem().SetStringValue("/config/webserver/webroot", "data/webroot", true);
  } // ctor

  WebServer::~WebServer() {
    shttpd_fini(m_SHttpdContext);
  } // dtor

  void WebServer::Initialize() {
    Subsystem::Initialize();

    string ports = DSS::GetInstance()->GetPropertySystem().GetStringValue("/config/webserver/ports");
    Log("Webserver: Listening on port(s) " + ports);
    shttpd_set_option(m_SHttpdContext, "ports", ports.c_str());

    string aliases = string("/=") + DSS::GetInstance()->GetPropertySystem().GetStringValue("/config/webserver/webroot");
    Log("Webserver: Configured aliases: " + aliases);
    shttpd_set_option(m_SHttpdContext, "aliases", aliases.c_str());

    shttpd_register_uri(m_SHttpdContext, "/config", &HTTPListOptions, NULL);
    shttpd_register_uri(m_SHttpdContext, "/json/*", &JSONHandler, NULL);
  } // Initialize

  void WebServer::Start() {
    Subsystem::Start();
    Run();
  } // Start

  void WebServer::Execute() {
    Log("Webserver started", lsInfo);
    while(!m_Terminated) {
      shttpd_poll(m_SHttpdContext, 1000);
    }
  } // Execute

  const char* HTTPCodeToMessage(const int _code) {
    if(_code == 400) {
      return "Bad Request";
    } else if(_code == 401) {
      return "Unauthorized\r\nWWW-Authenticate: Basic realm=\"dSS\"";
    } else if(_code == 403) {
      return "Forbidden";
    } else {
      return "OK";
    }
  }

  void WebServer::EmitHTTPHeader(int _code, struct shttpd_arg* _arg, const std::string& _contentType) {
    stringstream sstream;
    sstream << "HTTP/1.1 " << _code << ' ' << HTTPCodeToMessage(_code) << "\r\n";
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

  string ToJSONValue(const int& _value) {
    return IntToString(_value);
  } // ToJSONValue

  string ToJSONValue(const bool& _value) {
    if(_value) {
      return "true";
    } else {
      return "false";
    }
  }

  string ToJSONValue(const string& _value) {
    return string("\"") + _value + '"';
  } // ToJSONValue

  string ToJSONValue(const DeviceReference& _device) {
    stringstream sstream;
    sstream << "{ \"id\": \"" << _device.GetDSID() << "\""
            << ", \"isSwitch\": " << ToJSONValue(_device.IsSwitch())
            << ", \"name\": \"" << _device.GetDevice().GetName()
            << "\", \"on\": " << ToJSONValue(_device.IsOn()) << " }";
    return sstream.str();
  }

  string ToJSONValue(const Set& _set, const string& _arrayName) {
    stringstream sstream;
    sstream << "\"" << _arrayName << "\":[";
    bool firstDevice = true;
    for(int iDevice = 0; iDevice < _set.Length(); iDevice++) {
      const DeviceReference& d = _set.Get(iDevice);
      if(firstDevice) {
        firstDevice = false;
      } else {
        sstream << ",";
      }
      sstream << ToJSONValue(d);
    }
    sstream << "]";
    return sstream.str();
  }

  string ToJSONValue(Apartment& _apartment) {
  	stringstream sstream;
  	sstream << "{ apartment: { zones: [";
	  vector<Zone*>& zones = _apartment.GetZones();
	  bool first = true;
	  for(vector<Zone*>::iterator ipZone = zones.begin(), e = zones.end();
	      ipZone != e; ++ipZone)
	  {
	  	Zone* pZone = *ipZone;
	  	if(!first) {
	  	  sstream << ", ";
	  	} else {
	  		first = false;
	  	}
	  	sstream << "{ id: " << pZone->GetZoneID() << ",";
	  	string name = pZone->GetName();
	  	if(name.size() == 0) {
	  		name = string("Zone ") + IntToString(pZone->GetZoneID());
	  	}
	  	sstream << "name: " << ToJSONValue(name) << ", ";

	  	Set devices = pZone->GetDevices();
      sstream << ToJSONValue(devices, "devices");

	  	sstream << "} ";
	  }
    sstream << "]} }";
	  return sstream.str();
  }

  Set GetUnassignedDevices() {
    Apartment& apt = DSS::GetInstance()->GetApartment();
    Set devices = apt.GetZone(0).GetDevices();

    vector<Zone*>& zones = apt.GetZones();
    for(vector<Zone*>::iterator ipZone = zones.begin(), e = zones.end();
        ipZone != e; ++ipZone)
    {
      Zone* pZone = *ipZone;
      if(pZone->GetZoneID() == 0) {
        // zone 0 holds all devices, so we're going to skip it
        continue;
      }

      devices = devices.Remove(pZone->GetDevices());
    }

    return devices;
  }

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
        sstream << ToJSONValue(d);
      }
      sstream << "]}";

      shttpd_printf(_arg, sstream.str().c_str());
    } else if(method == "getunassigneddevices") {
      EmitHTTPHeader(200, _arg, "application/json");

      string result = "{" + ToJSONValue(GetUnassignedDevices(), "devices") + "}";
      shttpd_printf(_arg, result.c_str());
    } else if(method == "event/raise") {
      EmitHTTPHeader(200, _arg, "application/json");
      string name = paramMap["name"];
      string location = paramMap["location"];
      string context = paramMap["context"];

      boost::shared_ptr<Event> e(new Event(name));
      if(!context.empty()) {
        e->SetContext(context);
      }
      if(!location.empty()) {
        e->SetLocation(location);
      }
      DSS::GetInstance()->GetEventQueue().PushEvent(e);
      shttpd_printf(_arg, "{ok:1}");
    } else if(method == "turnon") {
      EmitHTTPHeader(200, _arg, "application/json");

      string devidStr = paramMap["device"];
      string zoneIDStr = paramMap["zone"];
      if(!devidStr.empty()) {
        int devid = StrToUInt(devidStr);
        DSS::GetInstance()->GetApartment().GetDeviceByDSID(devid).TurnOn();
        shttpd_printf(_arg, "{ok:1}");
      } else if(!zoneIDStr.empty()) {
        int zoneID = StrToUInt(zoneIDStr);
        DSS::GetInstance()->GetApartment().GetZone(zoneID).GetDevices().TurnOn();
        shttpd_printf(_arg, "{ok:1}");
      } else {
        shttpd_printf(_arg, "{ok:0}");
      }
    } else if(method == "turnoff") {

      string devidStr = paramMap["device"];
      string zoneIDStr = paramMap["zone"];
      if(!devidStr.empty()) {
        int devid = StrToUInt(devidStr);
        DSS::GetInstance()->GetApartment().GetDeviceByDSID(devid).TurnOff();
        shttpd_printf(_arg, "{ok:1}");
      } else if(!zoneIDStr.empty()) {
        int zoneID = StrToUInt(zoneIDStr);
        DSS::GetInstance()->GetApartment().GetZone(zoneID).GetDevices().TurnOff();
        shttpd_printf(_arg, "{ok:1}");
      } else {
        shttpd_printf(_arg, "{ok:0}");
      }
    } else if(method == "sendframe") {
      int destination = StrToIntDef(paramMap["destination"],0) & 0x3F;
      bool broadcast = paramMap["broadcast"] == "true";
      int counter = StrToIntDef(paramMap["counter"], 0x00) & 0x03;
      int command = StrToIntDef(paramMap["command"], 0x09 /* request */) & 0x00FF;
      int length = StrToIntDef(paramMap["length"], 0x00) & 0x0F;

      std::cout
           << "sending frame: "
           << "\ndest:    " << destination
           << "\nbcst:    " << broadcast
           << "\ncntr:    " << counter
           << "\ncmd :    " << command
           << "\nlen :    " << length << std::endl;

      DS485CommandFrame* frame = new DS485CommandFrame();
      frame->GetHeader().SetBroadcast(broadcast);
      frame->GetHeader().SetDestination(destination);
      frame->GetHeader().SetCounter(counter);
      frame->SetCommand(command);
      for(int iByte = 0; iByte < length; iByte++) {
        uint8 byte = StrToIntDef(paramMap[string("payload_") + IntToString(iByte+1)], 0xFF);
        cout << "b" << dec << iByte << ": " << hex << (int)byte << "\n";
        frame->GetPayload().Add<uint8>(byte);
      }
      cout << dec <<"done" << endl;
      DS485Interface* intf = &DSS::GetInstance()->GetDS485Interface();
      DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(intf);
      if(proxy != NULL) {
        proxy->SendFrame(*frame, true);
      } else {
        delete frame;
      }
    } else if(method == "getapartment") {
    	EmitHTTPHeader(200, _arg, "application/json");

    	shttpd_printf(_arg, ToJSONValue(DSS::GetInstance()->GetApartment()).c_str());
    } else if(BeginsWith(method, "sim/switch/")) {
      EmitHTTPHeader(200, _arg, "application/json");

      bool fail = true;
      string devidStr = paramMap["device"];
      if(method == "sim/switch/getstate") {
        if(!devidStr.empty()) {
          dsid_t devid = StrToUInt(devidStr);
          DSIDInterface* dev = DSS::GetInstance()->GetModulatorSim().GetSimulatedDevice(devid);
          DSIDSimSwitch* sw = NULL;
          if(dev != NULL && (sw = dynamic_cast<DSIDSimSwitch*>(dev)) != NULL) {
            stringstream sstream;
            sstream << "{ groupid: " << DSS::GetInstance()->GetModulatorSim().GetGroupForSwitch(sw)
                    << " }";
            shttpd_printf(_arg, sstream.str().c_str());
            fail = false;
          }
        }
      } else if(method == "sim/switch/pressed") {
        if(!devidStr.empty()) {
          dsid_t devid = StrToUInt(devidStr);
          int buttonNr = StrToIntDef(paramMap["buttonnr"], 1);
          DSIDInterface* dev = DSS::GetInstance()->GetModulatorSim().GetSimulatedDevice(devid);
          DSIDSimSwitch* sw = NULL;
          if(dev != NULL && (sw = dynamic_cast<DSIDSimSwitch*>(dev)) != NULL) {
            ButtonPressKind kind = Click;
            string kindStr = paramMap["kind"];
            if(kindStr == "touch") {
              kind = Touch;
            } else if(kindStr == "touchend") {
              kind = TouchEnd;
            }
            DSS::GetInstance()->GetModulatorSim().ProcessButtonPress(*sw, buttonNr, kind);
            shttpd_printf(_arg, "{ok:1}");
            fail = false;
          }
        }
      }
      if(fail) {
        shttpd_printf(_arg, "{ok:0}");
      }
/*
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
          response << "{ \"id\": " << sub.GetID() << ","
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
      */
    } else if(BeginsWith(method, "structure/")) {
      if(method == "structure/zoneAddDevice") {
      	bool ok = true;
        EmitHTTPHeader(200, _arg, "application/json");

        string devidStr = paramMap["devid"];
        if(!devidStr.empty()) {
          dsid_t devid = StrToUInt(devidStr);

          Device& dev = DSS::GetInstance()->GetApartment().GetDeviceByDSID(devid);

          string zoneIDStr = paramMap["zone"];
          if(!zoneIDStr.empty()) {
          	try {
              int zoneID = StrToInt(zoneIDStr);
              DSS::GetInstance()->GetApartment().GetZone(zoneID).AddDevice(DeviceReference(dev, DSS::GetInstance()->GetApartment()));
          	} catch(runtime_error&) {
          		ok = false;
          	}
          }
          if(ok) {
            shttpd_printf(_arg, "{ok:1}");
          } else {
          	shttpd_printf(_arg, "{ok:0}");
          }
        } else {
          shttpd_printf(_arg, "{ok:0}");
        }
      } else if(method == "structure/addZone") {
        bool ok = false;
        EmitHTTPHeader(200, _arg, "application/json");
        int zoneID = -1;
        int modulatorID = -1;
        string zoneIDStr = paramMap["zoneID"];
        if(!zoneIDStr.empty()) {
          zoneID = StrToIntDef(zoneIDStr, -1);
        }
        string modIDStr = paramMap["modulatorID"];
        if(!modIDStr.empty()) {
          modulatorID = StrToIntDef(modIDStr, -1);
        }
        if(zoneID != -1 && modulatorID != -1) {
          try {
            Modulator& modulator = DSS::GetInstance()->GetApartment().GetModulatorByBusID(modulatorID);
            DSS::GetInstance()->GetApartment().AllocateZone(modulator, zoneID);
            ok = true;
          } catch(runtime_error&) {
          }
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
    // TODO: dump config tree into xml with a nice stylesheet
/*
    const HashMapConstStringString& options = DSS::GetInstance()->GetConfig().GetOptions();
    for(HashMapConstStringString::const_iterator iOption = options.begin(); iOption != options.end(); ++iOption) {
      stream << "<li>" << iOption->first << " = " << iOption->second << "</li>";
    }
*/
    stream << "</ul></body></html>";
    shttpd_printf(_arg, stream.str().c_str());

    _arg->flags |= SHTTPD_END_OF_OUTPUT;
  } // HTTPListOptions

}
