#include "webserver.h"
#include "logger.h"
#include "model.h"
#include "dss.h"
#include "event.h"
#include "unix/ds485proxy.h"
#include "sim/dssim.h"
#include "propertysystem.h"
#include "foreach.h"

#include <iostream>
#include <sstream>

#include <boost/shared_ptr.hpp>

namespace dss {
  //============================================= WebServer

  WebServer::WebServer(DSS* _pDSS)
  : Subsystem(_pDSS, "WebServer"),
    Thread("WebServer")
  {
    Logger::GetInstance()->Log("Starting Webserver...");
    m_SHttpdContext = shttpd_init();
    DSS::GetInstance()->GetPropertySystem().SetStringValue(GetConfigPropertyBasePath() + "ports", "8080", true);
    DSS::GetInstance()->GetPropertySystem().SetStringValue(GetConfigPropertyBasePath() + "webroot", "data/webroot/", true);
  } // ctor

  WebServer::~WebServer() {
    shttpd_fini(m_SHttpdContext);
  } // dtor

  void WebServer::Initialize() {
    Subsystem::Initialize();

    string ports = DSS::GetInstance()->GetPropertySystem().GetStringValue(GetConfigPropertyBasePath() + "ports");
    Log("Webserver: Listening on port(s) " + ports);
    shttpd_set_option(m_SHttpdContext, "ports", ports.c_str());

    string aliases = string("/=") + DSS::GetInstance()->GetPropertySystem().GetStringValue(GetConfigPropertyBasePath() + "webroot");
    Log("Webserver: Configured aliases: " + aliases);
    shttpd_set_option(m_SHttpdContext, "aliases", aliases.c_str());

    shttpd_register_uri(m_SHttpdContext, "/browse/*", &HTTPListOptions, NULL);
    shttpd_register_uri(m_SHttpdContext, "/json/*", &JSONHandler, NULL);
  } // Initialize

  void WebServer::DoStart() {
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
    std::stringstream sstream;
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
  } // ToJSONValue(int)

  string ToJSONValue(const bool& _value) {
    if(_value) {
      return "true";
    } else {
      return "false";
    }
  } // ToJSONValue(bool)

  string ToJSONValue(const string& _value) {
    return string("\"") + _value + '"';
  } // ToJSONValue(const string&)

  string ToJSONValue(const char* _value) {
    return ToJSONValue(string(_value));
  } // ToJSONValue(const char*)

  string ResultToJSON(const bool _ok, const string& _message = "") {
    stringstream sstream;
    sstream << "{ " << ToJSONValue("ok") << ":" << ToJSONValue(_ok);
    if(!_message.empty()) {
      sstream << ", " << ToJSONValue("message") << ":" << ToJSONValue(_message);
    }
    sstream << "}";
    return sstream.str();
  } // ResultToJSON

  string JSONOk(const string& _innerResult) {
    stringstream sstream;
    sstream << "{ " << ToJSONValue("ok") << ":" << ToJSONValue(true) << ", " << ToJSONValue("result")<<  ": " << _innerResult << " }";
    return sstream.str();
  }

  string ToJSONValue(const DeviceReference& _device) {
    std::stringstream sstream;
    sstream << "{ \"id\": \"" << _device.GetDSID().ToString() << "\""
            << ", \"isSwitch\": " << ToJSONValue(_device.HasSwitch())
            << ", \"name\": " << ToJSONValue(_device.GetDevice().GetName())
            << ", \"fid\": " << ToJSONValue(_device.GetDevice().GetFunctionID())
            << ", \"circuitID\":" << ToJSONValue(_device.GetDevice().GetModulatorID())
            << ", \"busID\":"  << ToJSONValue(_device.GetDevice().GetShortAddress())
            << ", \"on\": " << ToJSONValue(_device.IsOn()) << " }";
    return sstream.str();
  } // ToJSONValue(DeviceReference)

  string ToJSONValue(const Set& _set, const string& _arrayName) {
    std::stringstream sstream;
    sstream << ToJSONValue(_arrayName) << ":[";
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
  } // ToJSONValue(Set,Name)

  string ToJSONValue(Zone& _zone) {
    std::stringstream sstream;
    sstream << "{ \"id\": " << _zone.GetZoneID() << ",";
    string name = _zone.GetName();
    if(name.size() == 0) {
      name = string("Zone ") + IntToString(_zone.GetZoneID());
    }
    sstream << "\"name\": " << ToJSONValue(name) << ", ";

    Set devices = _zone.GetDevices();
    sstream << ToJSONValue(devices, "devices");

    sstream << "} ";
    return sstream.str();
  } // ToJSONValue(Zone)

  string ToJSONValue(Apartment& _apartment) {
  	std::stringstream sstream;
  	sstream << "{ \"apartment\": { \"zones\": [";
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
	  	sstream << ToJSONValue(*pZone);
	  }
    sstream << "]} }";
	  return sstream.str();
  } // ToJSONValue(Apartment)

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
  } // GetUnassignedDevices

  template<class t>
  string ToJSONArray(const vector<t>& _v);

  template<>
  string ToJSONArray(const vector<int>& _v) {
    std::stringstream arr;
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

  string WebServer::CallDeviceInterface(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, IDeviceInterface* _interface, Session* _session) {
    bool ok = true;
    string errorString;
    assert(_interface != NULL);
    if(EndsWith(_method, "/turnOn")) {
      _interface->TurnOn();
    } else if(EndsWith(_method, "/turnOff")) {
      _interface->TurnOff();
    } else if(EndsWith(_method, "/increaseValue")) {
      _interface->IncreaseValue();
    } else if(EndsWith(_method, "/decreaseValue")) {
      _interface->DecreaseValue();
    } else if(EndsWith(_method, "/enable")) {
      _interface->Enable();
    } else if(EndsWith(_method, "/disable")) {
      _interface->Disable();
    } else if(EndsWith(_method, "/startDim")) {
      string direction = _parameter["direction"];
      if(direction == "up") {
        _interface->StartDim(true);
      } else {
        _interface->StartDim(false);
      }
    } else if(EndsWith(_method, "/endDim")) {
      _interface->EndDim();
    } else if(EndsWith(_method, "/setValue")) {
      // nop
    } else if(EndsWith(_method, "/callScene")) {
      string sceneStr = _parameter["sceneNr"];
      int sceneID = StrToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        _interface->CallScene(sceneID);
      } else {
        errorString = "invalid sceneNr: '" + sceneStr + "'";
        ok = false;
      }
    } else if(EndsWith(_method, "/saveScene")) {
      string sceneStr = _parameter["sceneNr"];
      int sceneID = StrToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        _interface->SaveScene(sceneID);
      } else {
        errorString = "invalid sceneNr: '" + sceneStr + "'";
        ok = false;
      }
    } else if(EndsWith(_method, "/undoScene")) {
      string sceneStr = _parameter["sceneNr"];
      int sceneID = StrToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        _interface->UndoScene(sceneID);
      } else {
        errorString = "invalid sceneNr: '" + sceneStr + "'";
        ok = false;
      }
    } else if(EndsWith(_method, "/getConsumption")) {
      return "{ " + ToJSONValue("consumption") + ": " +  UIntToString(_interface->GetPowerConsumption()) +"}";
    }
    return ResultToJSON(ok, errorString);
  } // CallDeviceInterface

  bool WebServer::IsDeviceInterfaceCall(const std::string& _method) {
    return EndsWith(_method, "/turnOn")
        || EndsWith(_method, "/turnOff")
        || EndsWith(_method, "/increaseValue")
        || EndsWith(_method, "/decreaseValue")
        || EndsWith(_method, "/enable")
        || EndsWith(_method, "/disable")
        || EndsWith(_method, "/startDim")
        || EndsWith(_method, "/endDim")
        || EndsWith(_method, "/setValue")
        || EndsWith(_method, "/callScene")
        || EndsWith(_method, "/saveScene")
        || EndsWith(_method, "/undoScene")
        || EndsWith(_method, "/getConsumption");
  } // IsDeviceInterfaceCall

  string WebServer::HandleApartmentCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    bool ok = true;
    string errorMessage;
    _handled = true;
    if(EndsWith(_method, "/getConsumption")) {
      int accumulatedConsumption = 0;
      foreach(Modulator* pModulator, GetDSS().GetApartment().GetModulators()) {
        accumulatedConsumption += pModulator->GetPowerConsumption();
      }
      return "{ " + ToJSONValue("consumption") + ": " +  UIntToString(accumulatedConsumption) +"}";
    } else if(IsDeviceInterfaceCall(_method)) {
      IDeviceInterface* interface = NULL;
      string groupName = _parameter["groupName"];
      string groupIDString = _parameter["groupID"];
      if(!groupName.empty()) {
        try {
          Group& grp = GetDSS().GetApartment().GetGroup(groupName);
          interface = &grp;
        } catch(runtime_error& e) {
          errorMessage = "Could not find group with name '" + groupName + "'";
          ok = false;
        }
      } else if(!groupIDString.empty()) {
        try {
          int groupID = StrToIntDef(groupIDString, -1);
          if(groupID != -1) {
            Group& grp = GetDSS().GetApartment().GetGroup(groupID);
            interface = &grp;
          } else {
            errorMessage = "Could not parse group id '" + groupIDString + "'";
            ok = false;
          }
        } catch(runtime_error& e) {
          errorMessage = "Could not find group with ID '" + groupIDString + "'";
          ok = false;
        }
      }
      if(ok) {
        if(interface == NULL) {
          interface = &GetDSS().GetApartment().GetGroup(GroupIDBroadcast);
        }
        return CallDeviceInterface(_method, _parameter, _arg, interface, _session);
      } else {
        stringstream sstream;
        sstream << "{ ok: " << ToJSONValue(ok) << ", message: " << ToJSONValue(errorMessage) << " }";
        return sstream.str();
      }
    } else {
      string result;
      if(EndsWith(_method, "/getStructure")) {
        result = ToJSONValue(GetDSS().GetApartment());
      } else if(EndsWith(_method, "/getDevices")) {
        Set devices;
        if(_parameter["unassigned"].empty()) {
          devices = GetDSS().GetApartment().GetDevices();
        } else {
          devices = GetUnassignedDevices();
        }

        result = "{" + ToJSONValue(devices, "devices") + "}";
      } else if(EndsWith(_method, "/login")) {
        int token = m_LastSessionID;
        m_Sessions[token] = Session(token);
        return "{" + ToJSONValue("token") + ": " + ToJSONValue(token) + "}";
      } else {
        _handled = false;
      }
      return result;
    }
  } // HandleApartmentCall

  string WebServer::HandleZoneCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    bool ok = true;
    string errorMessage;
    _handled = true;
    string zoneName = _parameter["name"];
    string zoneIDString = _parameter["id"];
    Zone* pZone = NULL;
    if(!zoneName.empty()) {
      try {
        Zone& zone = GetDSS().GetApartment().GetZone(zoneName);
        pZone = &zone;
      } catch(runtime_error&  e) {
        ok = false;
        errorMessage = "Could not find zone named '" + zoneName + "'";
      }
    } else if(!zoneIDString.empty()) {
      int zoneID = StrToIntDef(zoneIDString, -1);
      if(zoneID != -1) {
        try {
          Zone& zone = GetDSS().GetApartment().GetZone(zoneID);
          pZone = &zone;
        } catch(runtime_error& e) {
          ok = false;
          errorMessage = "Could not find zone with id '" + zoneIDString + "'";
        }
      } else {
        ok = false;
        errorMessage = "Could not parse id '" + zoneIDString + "'";
      }
    } else {
      ok = false;
      errorMessage = "Need parameter name or id to identify zone";
    }
    if(ok) {
      if(IsDeviceInterfaceCall(_method)) {
        IDeviceInterface* interface = NULL;
        if(ok) {
          string groupName = _parameter["groupName"];
          string groupIDString = _parameter["groupID"];
          if(!groupName.empty()) {
            try {
              Group* grp = pZone->GetGroup(groupName);
              if(grp == NULL) {
                // TODO: this might better be done by the zone
                throw runtime_error("dummy");
              }
              interface = grp;
            } catch(runtime_error& e) {
              errorMessage = "Could not find group with name '" + groupName + "'";
              ok = false;
            }
          } else if(!groupIDString.empty()) {
            try {
              int groupID = StrToIntDef(groupIDString, -1);
              if(groupID != -1) {
                Group* grp = pZone->GetGroup(groupID);
                if(grp == NULL) {
                  // TODO: this might better be done by the zone
                  throw runtime_error("dummy");
                }
                interface = grp;
              } else {
                errorMessage = "Could not parse group id '" + groupIDString + "'";
                ok = false;
              }
            } catch(runtime_error& e) {
              errorMessage = "Could not find group with ID '" + groupIDString + "'";
              ok = false;
            }
          }
        }
        if(ok) {
          if(interface == NULL) {
            interface = pZone;
          }
          return CallDeviceInterface(_method, _parameter, _arg, interface, _session);
        }
      } else {
        _handled = false;
        return "";
      }
    }
    if(!ok) {
      return ResultToJSON(ok, errorMessage);
    } else {
      return "";
    }
  } // HandleZoneCall

  string WebServer::HandleDeviceCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    bool ok = true;
    string errorMessage;
    _handled = true;
    string deviceName = _parameter["name"];
    string deviceDSIDString = _parameter["dsid"];
    Device* pDevice = NULL;
    if(!deviceName.empty()) {
      try {
        Device& device = GetDSS().GetApartment().GetDeviceByName(deviceName);
        pDevice = &device;
      } catch(runtime_error&  e) {
        ok = false;
        errorMessage = "Could not find device named '" + deviceName + "'";
      }
    } else if(!deviceDSIDString.empty()) {
      dsid_t deviceDSID = dsid_t::FromString(deviceDSIDString);
      if(!(deviceDSID == NullDSID)) {
        try {
          Device& device = GetDSS().GetApartment().GetDeviceByDSID(deviceDSID);
          pDevice = &device;
        } catch(runtime_error& e) {
          ok = false;
          errorMessage = "Could not find device with dsid '" + deviceDSIDString + "'";
        }
      } else {
        ok = false;
        errorMessage = "Could not parse dsid '" + deviceDSIDString + "'";
      }
    } else {
      ok = false;
      errorMessage = "Need parameter name or dsid to identify device";
    }
    if(ok) {
      if(IsDeviceInterfaceCall(_method)) {
        return CallDeviceInterface(_method, _parameter, _arg, pDevice, _session);
      } else if(BeginsWith(_method, "device/getGroups")) {
        int numGroups = pDevice->GetGroupsCount();
        stringstream sstream;
        sstream << "{ " << ToJSONValue("groups") << ": [";
        bool first = true;
        for(int iGroup = 0; iGroup < numGroups; iGroup++) {
          if(!first) {
            sstream << ", ";
          }
          first = false;
          try {
            Group& group = pDevice->GetGroupByIndex(iGroup);
            sstream << "{ " << ToJSONValue("id") << ":" << group.GetID();
            if(!group.GetName().empty()) {
              sstream << ", " << ToJSONValue("name") << ":" << ToJSONValue(group.GetName());
            }
          } catch (runtime_error&) {
            Log("Group only present at device level");
          }
          sstream << "}";
        }
        sstream << "]}";
        return sstream.str();
      } else if(BeginsWith(_method, "device/getState")) {
        stringstream sstream;
        sstream << "{ " << ToJSONValue("isOn") << ":" << ToJSONValue(pDevice->IsOn()) << " }";
        return JSONOk(sstream.str());
      } else {
        _handled = false;
        return "";
      }
    } else {
      return ResultToJSON(ok, errorMessage);
    }
  } // HandleDeviceCall

  string WebServer::HandleSetCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {

  } // HandleSetCall

  string WebServer::HandlePropertyCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {

  } // HandlePropertyCall

  string WebServer::HandleEventCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    _handled = true;
    string result;
    if(EndsWith(_method, "/raise")) {
      string name = _parameter["name"];
      string location = _parameter["location"];
      string context = _parameter["context"];

      boost::shared_ptr<Event> e(new Event(name));
      if(!context.empty()) {
        e->SetContext(context);
      }
      if(!location.empty()) {
        e->SetLocation(location);
      }
      GetDSS().GetEventQueue().PushEvent(e);
      return ResultToJSON(true);
    } else {
      _handled = false;
    }
    return result;
  } // HandleEventCall

  string WebServer::HandleSimCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    _handled = true;
    if(BeginsWith(_method, "sim/switch")) {
      if(EndsWith(_method, "/switch/pressed")) {
        int buttonNr = StrToIntDef(_parameter["buttonnr"], -1);
        if(buttonNr == -1) {
          return ResultToJSON(false, "Invalid button number");
        }

        int zoneID = StrToIntDef(_parameter["zoneID"], -1);
        if(zoneID == -1) {
          return ResultToJSON(false, "Could not parse zoneID");
        }
        int groupID = StrToIntDef(_parameter["groupID"], -1);
        if(groupID == -1) {
          return ResultToJSON(false, "Could not parse groupID");
        }
        try {
          Zone& zone = GetDSS().GetApartment().GetZone(zoneID);
          Group* pGroup = zone.GetGroup(groupID);

          if(pGroup == NULL) {
            return ResultToJSON(false, "Could not find group");
          }

          switch(buttonNr) {
          case 1: // upper-left
          case 3: // upper-right
          case 7: // lower-left
          case 9: // lower-right
            break;
          case 2: // up
            pGroup->IncreaseValue();
            break;
          case 8: // down
            pGroup->DecreaseValue();
            break;
          case 4: // left
            pGroup->PreviousScene();
            break;
          case 6: // right
            pGroup->NextScene();
            break;
          case 5:
            {
              if(groupID == GroupIDGreen) {
                pGroup->CallScene(SceneBell);
              } else if(groupID == GroupIDRed){
                pGroup->CallScene(SceneAlarm);
              } else {
                const int lastScene = pGroup->GetLastCalledScene();
                if(lastScene == dss::SceneOff || lastScene == dss::SceneDeepOff ||
                   lastScene == dss::SceneStandBy || lastScene == dss::ScenePanic)
                {
                  pGroup->CallScene(dss::Scene1);
                } else {
                  pGroup->CallScene(dss::SceneOff);
                }
              }
            }
            break;
          default:
            return ResultToJSON(false, "Invalid button nr (range is 1..9)");
          }
        } catch(runtime_error&) {
          return ResultToJSON(false, "Could not find zone");
        }
      } else {
        _handled = false;
        return "";
      }
    } else if(BeginsWith(_method, "sim/addDevice")) {
      string type = _parameter["type"];
      string dsidStr = _parameter["dsid"];
      // TODO: not finished yet ;)
    } else {
      _handled = false;
    }
    return "";
  } // HandleSimCall

  string WebServer::HandleDebugCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    _handled = true;
    if(EndsWith(_method, "/sendFrame")) {
      int destination = StrToIntDef(_parameter["destination"],0) & 0x3F;
      bool broadcast = _parameter["broadcast"] == "true";
      int counter = StrToIntDef(_parameter["counter"], 0x00) & 0x03;
      int command = StrToIntDef(_parameter["command"], 0x09 /* request */) & 0x00FF;
      int length = StrToIntDef(_parameter["length"], 0x00) & 0x0F;

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
        uint8_t byte = StrToIntDef(_parameter[string("payload_") + IntToString(iByte+1)], 0xFF);
        cout << "b" << dec << iByte << ": " << hex << (int)byte << "\n";
        frame->GetPayload().Add<uint8_t>(byte);
      }
      cout << dec <<"done" << endl;
      DS485Interface* intf = &DSS::GetInstance()->GetDS485Interface();
      DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(intf);
      if(proxy != NULL) {
        proxy->SendFrame(*frame);
      } else {
        delete frame;
      }
      return ResultToJSON(true);
    } else {
      _handled = false;
      return "";
    }
  } // HandleDebugCall

  void WebServer::JSONHandler(struct shttpd_arg* _arg) {
    const string urlid = "/json/";
    string uri = shttpd_get_env(_arg, "REQUEST_URI");
    HashMapConstStringString paramMap = ParseParameter(shttpd_get_env(_arg, "QUERY_STRING"));

    string method = uri.substr(uri.find(urlid) + urlid.size());

    WebServer& self = DSS::GetInstance()->GetWebServer();

    Session* session = NULL;
    string tokenStr = paramMap["token"];
    if(!tokenStr.empty()) {
      int token = StrToIntDef(tokenStr, -1);
      if(token != -1) {
        SessionByID::iterator iEntry = self.m_Sessions.find(token);
        if(iEntry != self.m_Sessions.end()) {
          if(iEntry->second->IsStillValid()) {
            Session& s = *iEntry->second;
            session = &s;
          }
        }
      }
    }


    string result;
    bool handled = false;
    if(BeginsWith(method, "apartment/")) {
      result = self.HandleApartmentCall(method, paramMap, _arg, handled, session);
    } else if(BeginsWith(method, "zone/")) {
      result = self.HandleZoneCall(method, paramMap, _arg, handled, session);
    } else if(BeginsWith(method, "device/")) {
      result = self.HandleDeviceCall(method, paramMap, _arg, handled, session);
    } else if(BeginsWith(method, "set/")) {
      result = self.HandleSetCall(method, paramMap, _arg, handled, session);
    } else if(BeginsWith(method, "property/")) {
      result = self.HandlePropertyCall(method, paramMap, _arg, handled, session);
    } else if(BeginsWith(method, "event/")) {
      result = self.HandleEventCall(method, paramMap, _arg, handled, session);
    } else if(BeginsWith(method, "sim/")) {
      result = self.HandleSimCall(method, paramMap, _arg, handled, session);
    } else if(BeginsWith(method, "debug/")) {
      result = self.HandleDebugCall(method, paramMap, _arg, handled, session);
    }
    EmitHTTPHeader(200, _arg, "application/json");
    if(!handled) {
      result = "{ ok: " + ToJSONValue(false) + ", message: " + ToJSONValue("Call to unknown function") + " }";
    }
    shttpd_printf(_arg, result.c_str());
    _arg->flags |= SHTTPD_END_OF_OUTPUT;
   // return;

    if(BeginsWith(method, "structure/")) {
      if(method == "structure/zoneAddDevice") {
      	bool ok = true;
        EmitHTTPHeader(200, _arg, "application/json");

        string devidStr = paramMap["devid"];
        if(!devidStr.empty()) {
          dsid_t devid = dsid::FromString(devidStr);

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

    const string urlid = "/browse";
    string uri = shttpd_get_env(_arg, "REQUEST_URI");
    uri = URLDecode(uri);
    HashMapConstStringString paramMap = ParseParameter(shttpd_get_env(_arg, "QUERY_STRING"));

    string path = uri.substr(uri.find(urlid) + urlid.size());
    shttpd_printf(_arg, "<html><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><body>");

    std::stringstream stream;
    stream << "<h1>" << path << "</h1>";
    stream << "<ul>";
    PropertyNode* node = DSS::GetInstance()->GetPropertySystem().GetProperty(path);
    if(node != NULL) {
      for(int iNode = 0; iNode < node->GetChildCount(); iNode++) {
        PropertyNode* cnode = node->GetChild(iNode);

        stream << "<li>";
        if(cnode->GetChildCount() != 0) {
          stream << "<a href=\"/browse" << path << "/" << cnode->GetDisplayName() << "\">" << cnode->GetDisplayName() << "</a>";
        } else {
          stream << cnode->GetDisplayName();
        }
        stream << " : ";
        stream << GetValueTypeAsString(cnode->GetValueType()) << " : ";
        switch(cnode->GetValueType()) {
        case vTypeBoolean:
          stream << cnode->GetBoolValue();
          break;
        case vTypeInteger:
          stream << cnode->GetIntegerValue();
          break;
        case vTypeNone:
          break;
        case vTypeString:
          stream << cnode->GetStringValue();
          break;
        default:
          stream << "unknown value";
        }
      }
    }

    stream << "</ul></body></html>";
    shttpd_printf(_arg, stream.str().c_str());

    _arg->flags |= SHTTPD_END_OF_OUTPUT;
  } // HTTPListOptions

}
