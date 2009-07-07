/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#include "webserver.h"
#include "logger.h"
#include "model.h"
#include "dss.h"
#include "event.h"
#include "unix/ds485proxy.h"
#include "sim/dssim.h"
#include "propertysystem.h"
#include "foreach.h"
#include "core/web/restful.h"
#include "core/web/restfulapiwriter.h"
#include "metering/metering.h"
#include "metering/series.h"
#include "metering/seriespersistence.h"

#include <iostream>
#include <sstream>

#include <boost/shared_ptr.hpp>

namespace dss {
  //============================================= WebServer

  WebServer::WebServer(DSS* _pDSS)
  : Subsystem(_pDSS, "WebServer"),
    Thread("WebServer")
  {
    Logger::getInstance()->log("Starting Webserver...");
    m_SHttpdContext = shttpd_init();
  } // ctor

  WebServer::~WebServer() {
    shttpd_fini(m_SHttpdContext);
  } // dtor

  void WebServer::initialize() {
    Subsystem::initialize();

    DSS::getInstance()->getPropertySystem().setStringValue(getConfigPropertyBasePath() + "webroot", getDSS().getDataDirectory() + "webroot/", true, false);
    DSS::getInstance()->getPropertySystem().setStringValue(getConfigPropertyBasePath() + "ports", "8080", true, false);

    RestfulAPI api;
    RestfulClass& clsApartment = api.addClass("apartment")
       .withDocumentation("A wrapper for global functions as well as adressing all devices connected to the dSS");
    clsApartment.addMethod("getName")
      .withDocumentation("Returns the name of the apartment");
    clsApartment.addMethod("setName")
      .withParameter("newName", "string", true)
      .withDocumentation("Sets the name of the apartment to newName");
    clsApartment.addMethod("turnOn")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Turns on all devices of the apartment.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("turnOff")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Turns off all devices of the apartment.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("increaseValue")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Increases the main value on all devices of the apartment.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("decreaseValue")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Decreases the main value on all devices of the apartment.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("enable")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Enables all devices of the apartment.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("disable")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Disables all devices of the apartment.", "A disabled device will react only to an enable call. If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("startDim")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("direction", "string", false)
      .withDocumentation("Starts dimming the devices of the apartment.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("endDim")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Stops dimming the devices of the apartment.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("setValue")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("value", "integer", true)
      .withDocumentation("Sets the output value of all devices of the apartment to value.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("callScene")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("sceneNr", "integer", true)
      .withDocumentation("Calls the scene sceneNr on all devices of the apartment.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("saveScene")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("sceneNr", "integer", true)
      .withDocumentation("Saves the current output value to sceneNr.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("undoScene")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("sceneNr", "integer", true)
      .withDocumentation("Undos setting the value of sceneNr.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("getConsumption")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Returns the consumption of all devices in the apartment in mW.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("getStructure")
      .withParameter("sceneNr", "integer", true)
      .withDocumentation("Returns an object containing the structure of the apartment.");
    clsApartment.addMethod("getDevices")
      .withParameter("unassigned", "boolean", false)
      .withDocumentation("Returns the list of devices in the apartment.", "If unassigned is true, only devices that are not assigned to a zone get returned");
    clsApartment.addStaticMethod("login")
      .withDocumentation("Returns a session token");
    clsApartment.addMethod("getCircuits")
      .withDocumentation("Returns a list of the circuits present in the apartment");

    RestfulClass& clsZone = api.addClass("zone")
        .withInstanceParameter("id", "integer", false)
        .withInstanceParameter("name", "string", false)
        .requireOneOf("id", "name");
    clsZone.addMethod("getName")
      .withDocumentation("Returns the name of the zone.");
    clsZone.addMethod("setName")
      .withParameter("newName", "string", true)
      .withDocumentation("Sets the name of the zone to newName.");
    clsZone.addMethod("turnOn")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Turns on all devices in the zone.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("turnOff")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Turns off all devices in the zone.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("increaseValue")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Increases the main value of all devices in the zone.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("decreaseValue")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Decreases the main value of all devices in the zone.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("enable")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Enables all devices in the zone.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("disable")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Disables all devices in the zone.", "Disabled devices will react only to a enable call. If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("startDim")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("direction", "string", false)
      .withDocumentation("Starts dimming the main value of all devices in the zone.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("endDim")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Stops dimming of all devices in the zone.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("setValue")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("value", "integer", true)
      .withDocumentation("Sets the output value of all devices in the zone.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("callScene")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("sceneNr", "integer", true)
      .withDocumentation("Sets the scene sceneNr on all devices in the zone.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("saveScene")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("sceneNr", "integer", true)
      .withDocumentation("Saves the current output value to sceneNr of all devices in the zone.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("undoScene")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("sceneNr", "integer", true)
      .withDocumentation("Undos the setting of a scene value.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("getConsumption")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Returns the consumption of all devices in the zone in mW.", "If groupID or groupName are specified, only devices contained in this group will be addressed");

    RestfulClass& clsDevice = api.addClass("device")
        .withInstanceParameter("dsid", "integer", false)
        .withInstanceParameter("name", "string", false)
        .requireOneOf("dsid", "name");
    clsDevice.addMethod("getName")
       .withDocumentation("Returns the name of the device");
    clsDevice.addMethod("setName")
      .withParameter("newName", "string", true)
      .withDocumentation("Sets the name of the device to newName");
    clsDevice.addMethod("getGroups")
      .withDocumentation("Returns an array of groups the device is in");
    clsDevice.addMethod("getState")
      .withDocumentation("Returns the state of the device");
    clsDevice.addMethod("getLocation")
      .withDocumentation("Returns the location of the device.");
    clsDevice.addMethod("setLocation")
      .withParameter("x", "double", false)
      .withParameter("y", "double", false)
      .withParameter("z", "double", false)
      .withDocumentation("Sets the location of the device.");
    clsDevice.addMethod("turnOn")
      .withDocumentation("Turns on the device.", "This will call SceneMax on the device.");
    clsDevice.addMethod("turnOff")
      .withDocumentation("Turns off the device.", "This will call SceneMin on the device.");
    clsDevice.addMethod("increaseValue")
      .withDocumentation("Increases the default value of the device.");
    clsDevice.addMethod("decreaseValue")
      .withDocumentation("Decreases the default value of the device.");
    clsDevice.addMethod("enable")
      .withDocumentation("Enables the device.");
    clsDevice.addMethod("disable")
      .withDocumentation("Disables the device.", "A disabled device will only react to enable calls.");
    clsDevice.addMethod("startDim")
      .withParameter("direction", "string", false)
      .withDocumentation("Starts dimming the device.", "If direction equals 'up' it will dim up, otherwise down.");
    clsDevice.addMethod("endDim")
      .withDocumentation("Stops dimming.");
    clsDevice.addMethod("setValue")
      .withParameter("value", "integer", true)
      .withDocumentation("Sets the output value of the device to value");
    clsDevice.addMethod("callScene")
      .withParameter("sceneNr", "integer", true)
      .withDocumentation("Calls scene sceneNr on the device.");
    clsDevice.addMethod("saveScene")
      .withParameter("sceneNr", "integer", true)
      .withDocumentation("Saves the current outputvalue to sceneNr.");
    clsDevice.addMethod("undoScene")
      .withParameter("sceneNr", "integer", true)
      .withDocumentation("Undos saving the scene value for sceneNr");
    clsDevice.addMethod("getConsumption")
      .withDocumentation("Returns the consumption of the device in mW.", "Note that this works only for simulated devices at the moment.");

    RestfulClass& clsCircuit = api.addClass("circuit")
       .withInstanceParameter("id", "dsid", true);
    clsCircuit.addMethod("getName")
       .withDocumentation("Returns the name of the circuit.");
    clsCircuit.addMethod("setName")
       .withParameter("newName", "string", true)
       .withDocumentation("Sets the name of the circuit to newName.");
    clsCircuit.addMethod("getEnergyBorder")
       .withDocumentation("Returns the energy borders (orange, red).");
    clsCircuit.addMethod("getConsumption")
       .withDocumentation("Returns the consumption of all connected devices in mW");
    clsCircuit.addMethod("getEnergyMeterValue")
       .withDocumentation("Returns the meter-value in Wh");

    RestfulAPIWriter::WriteToXML(api, "doc/json_api.xml");
  } // initialize

  void WebServer::doStart() {
    run();
  } // start

  void WebServer::execute() {
    string ports = DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "ports");
    log("Webserver: Listening on port(s) " + ports);
    shttpd_set_option(m_SHttpdContext, "ports", ports.c_str());

    string aliases = string("/=") + DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "webroot");
    log("Webserver: Configured aliases: " + aliases);
    shttpd_set_option(m_SHttpdContext, "aliases", aliases.c_str());

    shttpd_register_uri(m_SHttpdContext, "/browse/*", &httpBrowseProperties, NULL);
    shttpd_register_uri(m_SHttpdContext, "/json/*", &jsonHandler, NULL);

    log("Webserver started", lsInfo);
    while(!m_Terminated) {
      shttpd_poll(m_SHttpdContext, 1000);
    }
  } // execute

  const char* httpCodeToMessage(const int _code) {
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

  void WebServer::emitHTTPHeader(int _code, struct shttpd_arg* _arg, const std::string& _contentType) {
    std::stringstream sstream;
    sstream << "HTTP/1.1 " << _code << ' ' << httpCodeToMessage(_code) << "\r\n";
    sstream << "Content-Type: " << _contentType << "; charset=utf-8\r\n\r\n";
    shttpd_printf(_arg, sstream.str().c_str());
  } // emitHTTPHeader

  HashMapConstStringString ParseParameter(const char* _params) {
    HashMapConstStringString result;
    if(_params != NULL) {
      vector<string> paramList = splitString(_params, '&');
      for(vector<string>::iterator iParam = paramList.begin(); iParam != paramList.end(); ++iParam) {
        vector<string> nameValue = splitString(*iParam, '=');
        if(nameValue.size() != 2) {
          result[*iParam] = "";
        } else {
          result[urlDecode(nameValue.at(0))] = urlDecode(nameValue.at(1));
        }
      }
    }
    return result;
  } // parseParameter

  string ToJSONValue(const int& _value) {
    return intToString(_value);
  } // toJSONValue(int)

  string ToJSONValue(const double& _value) {
    return doubleToString(_value);
  } // toJSONValue(double)

  string ToJSONValue(const bool& _value) {
    if(_value) {
      return "true";
    } else {
      return "false";
    }
  } // toJSONValue(bool)

  string ToJSONValue(const string& _value) {
    return string("\"") + _value + '"';
  } // toJSONValue(const string&)

  string ToJSONValue(const char* _value) {
    return ToJSONValue(string(_value));
  } // toJSONValue(const char*)

  string ResultToJSON(const bool _ok, const string& _message = "") {
    stringstream sstream;
    sstream << "{ " << ToJSONValue("ok") << ":" << ToJSONValue(_ok);
    if(!_message.empty()) {
      sstream << ", " << ToJSONValue("message") << ":" << ToJSONValue(_message);
    }
    sstream << "}";
    return sstream.str();
  } // resultToJSON

  string JSONOk(const string& _innerResult = "") {
    stringstream sstream;
    sstream << "{ " << ToJSONValue("ok") << ":" << ToJSONValue(true);
    if(!_innerResult.empty()) {
      sstream << ", " << ToJSONValue("result")<<  ": " << _innerResult;
    }
    sstream << " }";
    return sstream.str();
  }

  string ToJSONValue(const DeviceReference& _device) {
    std::stringstream sstream;
    sstream << "{ \"id\": \"" << _device.getDSID().toString() << "\""
            << ", \"isSwitch\": " << ToJSONValue(_device.hasSwitch())
            << ", \"name\": " << ToJSONValue(_device.getDevice().getName())
            << ", \"fid\": " << ToJSONValue(_device.getDevice().getFunctionID())
            << ", \"circuitID\":" << ToJSONValue(_device.getDevice().getModulatorID())
            << ", \"busID\":"  << ToJSONValue(_device.getDevice().getShortAddress())
            << ", \"on\": " << ToJSONValue(_device.isOn()) << " }";
    return sstream.str();
  } // toJSONValue(DeviceReference)

  string ToJSONValue(const Set& _set, const string& _arrayName) {
    std::stringstream sstream;
    sstream << ToJSONValue(_arrayName) << ":[";
    bool firstDevice = true;
    for(int iDevice = 0; iDevice < _set.length(); iDevice++) {
      const DeviceReference& d = _set.get(iDevice);
      if(firstDevice) {
        firstDevice = false;
      } else {
        sstream << ",";
      }
      sstream << ToJSONValue(d);
    }
    sstream << "]";
    return sstream.str();
  } // toJSONValue(Set,Name)

  string ToJSONValue(const Group& _group) {
    std::stringstream sstream;
    sstream << "{ " << ToJSONValue("id") << ": " << ToJSONValue(_group.getID()) << ",";
    sstream << ToJSONValue("name") << ": " << ToJSONValue(_group.getName()) << ", ";
    sstream << ToJSONValue("devices") << ": [";
    Set devices = _group.getDevices();
    bool first = true;
    for(int iDevice = 0; iDevice < devices.length(); iDevice++) {
      if(!first) {
        sstream << " , ";
      } else {
        first = false;
      }
      sstream << " { " << ToJSONValue("id") << ": " << ToJSONValue(devices[iDevice].getDSID().toString()) << " }";
    }
    sstream << "]} ";
    return sstream.str();
  } // toJSONValue(Group)

  string ToJSONValue(Zone& _zone) {
    std::stringstream sstream;
    sstream << "{ \"id\": " << _zone.getZoneID() << ",";
    string name = _zone.getName();
    if(name.size() == 0) {
      name = string("Zone ") + intToString(_zone.getZoneID());
    }
    sstream << "\"name\": " << ToJSONValue(name) << ", ";

    Set devices = _zone.getDevices();
    sstream << ToJSONValue(devices, "devices");
    sstream << "," << ToJSONValue("groups") << ": [";
    bool first = true;
    foreach(Group* pGroup, _zone.getGroups()) {
      if(!first) {
        sstream << ",";
      }
      first = false;
      sstream  << ToJSONValue(*pGroup);
    }
    sstream << "] ";

    sstream << "} ";
    return sstream.str();
  } // toJSONValue(Zone)

  string ToJSONValue(Apartment& _apartment) {
  	std::stringstream sstream;
  	sstream << "{ \"apartment\": { \"zones\": [";
	  vector<Zone*>& zones = _apartment.getZones();
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
  } // toJSONValue(Apartment)

  Set GetUnassignedDevices() {
    Apartment& apt = DSS::getInstance()->getApartment();
    Set devices = apt.getZone(0).getDevices();

    vector<Zone*>& zones = apt.getZones();
    for(vector<Zone*>::iterator ipZone = zones.begin(), e = zones.end();
        ipZone != e; ++ipZone)
    {
      Zone* pZone = *ipZone;
      if(pZone->getZoneID() == 0) {
        // zone 0 holds all devices, so we're going to skip it
        continue;
      }

      devices = devices.remove(pZone->getDevices());
    }

    return devices;
  } // getUnassignedDevices

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
  } // toJSONArray<int>

  string WebServer::callDeviceInterface(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, IDeviceInterface* _interface, Session* _session) {
    bool ok = true;
    string errorString;
    assert(_interface != NULL);
    if(endsWith(_method, "/turnOn")) {
      _interface->turnOn();
    } else if(endsWith(_method, "/turnOff")) {
      _interface->turnOff();
    } else if(endsWith(_method, "/increaseValue")) {
      _interface->increaseValue();
    } else if(endsWith(_method, "/decreaseValue")) {
      _interface->decreaseValue();
    } else if(endsWith(_method, "/enable")) {
      _interface->enable();
    } else if(endsWith(_method, "/disable")) {
      _interface->disable();
    } else if(endsWith(_method, "/startDim")) {
      string direction = _parameter["direction"];
      if(direction == "up") {
        _interface->startDim(true);
      } else {
        _interface->startDim(false);
      }
    } else if(endsWith(_method, "/endDim")) {
      _interface->endDim();
    } else if(endsWith(_method, "/setValue")) {
      int value = strToIntDef(_parameter["value"], -1);
      if(value == -1) {
        errorString = "invalid or missing parameter value: '" + _parameter["value"] + "'";
        ok = false;
      } else {
        _interface->setValue(value);
      }
    } else if(endsWith(_method, "/callScene")) {
      string sceneStr = _parameter["sceneNr"];
      int sceneID = strToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        _interface->callScene(sceneID);
      } else {
        errorString = "invalid sceneNr: '" + sceneStr + "'";
        ok = false;
      }
    } else if(endsWith(_method, "/saveScene")) {
      string sceneStr = _parameter["sceneNr"];
      int sceneID = strToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        _interface->saveScene(sceneID);
      } else {
        errorString = "invalid sceneNr: '" + sceneStr + "'";
        ok = false;
      }
    } else if(endsWith(_method, "/undoScene")) {
      string sceneStr = _parameter["sceneNr"];
      int sceneID = strToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        _interface->undoScene(sceneID);
      } else {
        errorString = "invalid sceneNr: '" + sceneStr + "'";
        ok = false;
      }
    } else if(endsWith(_method, "/getConsumption")) {
      return "{ " + ToJSONValue("consumption") + ": " +  uintToString(_interface->getPowerConsumption()) +"}";
    }
    return ResultToJSON(ok, errorString);
  } // callDeviceInterface

  bool WebServer::isDeviceInterfaceCall(const std::string& _method) {
    return endsWith(_method, "/turnOn")
        || endsWith(_method, "/turnOff")
        || endsWith(_method, "/increaseValue")
        || endsWith(_method, "/decreaseValue")
        || endsWith(_method, "/enable")
        || endsWith(_method, "/disable")
        || endsWith(_method, "/startDim")
        || endsWith(_method, "/endDim")
        || endsWith(_method, "/setValue")
        || endsWith(_method, "/callScene")
        || endsWith(_method, "/saveScene")
        || endsWith(_method, "/undoScene")
        || endsWith(_method, "/getConsumption");
  } // isDeviceInterfaceCall

  string WebServer::handleApartmentCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    bool ok = true;
    string errorMessage;
    _handled = true;
    if(endsWith(_method, "/getConsumption")) {
      int accumulatedConsumption = 0;
      foreach(Modulator* pModulator, getDSS().getApartment().getModulators()) {
        accumulatedConsumption += pModulator->getPowerConsumption();
      }
      return "{ " + ToJSONValue("consumption") + ": " +  uintToString(accumulatedConsumption) +"}";
    } else if(isDeviceInterfaceCall(_method)) {
      IDeviceInterface* interface = NULL;
      string groupName = _parameter["groupName"];
      string groupIDString = _parameter["groupID"];
      if(!groupName.empty()) {
        try {
          Group& grp = getDSS().getApartment().getGroup(groupName);
          interface = &grp;
        } catch(runtime_error& e) {
          errorMessage = "Could not find group with name '" + groupName + "'";
          ok = false;
        }
      } else if(!groupIDString.empty()) {
        try {
          int groupID = strToIntDef(groupIDString, -1);
          if(groupID != -1) {
            Group& grp = getDSS().getApartment().getGroup(groupID);
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
          interface = &getDSS().getApartment().getGroup(GroupIDBroadcast);
        }
        return callDeviceInterface(_method, _parameter, _arg, interface, _session);
      } else {
        stringstream sstream;
        sstream << "{ ok: " << ToJSONValue(ok) << ", message: " << ToJSONValue(errorMessage) << " }";
        return sstream.str();
      }
    } else {
      string result;
      if(endsWith(_method, "/getStructure")) {
        result = ToJSONValue(getDSS().getApartment());
      } else if(endsWith(_method, "/getDevices")) {
        Set devices;
        if(_parameter["unassigned"].empty()) {
          devices = getDSS().getApartment().getDevices();
        } else {
          devices = GetUnassignedDevices();
        }

        result = "{" + ToJSONValue(devices, "devices") + "}";
      } else if(endsWith(_method, "/login")) {
        int token = m_LastSessionID;
        m_Sessions[token] = Session(token);
        return "{" + ToJSONValue("token") + ": " + ToJSONValue(token) + "}";
      } else if(endsWith(_method, "/getCircuits")) {
        stringstream sstream;
        sstream << "{ " << ToJSONValue("circuits") << ": [";
        bool first = true;
        vector<Modulator*>& modulators = getDSS().getApartment().getModulators();
        foreach(Modulator* modulator, modulators) {
          if(!first) {
            sstream << ",";
          }
          first = false;
          sstream << "{ " << ToJSONValue("name") << ": " << ToJSONValue(modulator->getName());
          sstream << ", " << ToJSONValue("dsid") << ": " << ToJSONValue(modulator->getDSID().toString());
          sstream << ", " << ToJSONValue("busid") << ": " << ToJSONValue(modulator->getBusID());
          sstream << "}";
        }
        sstream << "]}";
        return JSONOk(sstream.str());
      } else if(endsWith(_method, "/getName")) {
        stringstream sstream;
        sstream << "{" << ToJSONValue("name") << ":" << ToJSONValue(getDSS().getApartment().getName()) << "}";
        return JSONOk(sstream.str());
      } else if(endsWith(_method, "/setName")) {
        getDSS().getApartment().setName(_parameter["newName"]);
        result = ResultToJSON(true);
      } else {
        _handled = false;
      }
      return result;
    }
  } // handleApartmentCall

  string WebServer::handleZoneCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    bool ok = true;
    string errorMessage;
    _handled = true;
    string zoneName = _parameter["name"];
    string zoneIDString = _parameter["id"];
    Zone* pZone = NULL;
    if(!zoneIDString.empty()) {
      int zoneID = strToIntDef(zoneIDString, -1);
      if(zoneID != -1) {
        try {
          Zone& zone = getDSS().getApartment().getZone(zoneID);
          pZone = &zone;
        } catch(runtime_error& e) {
          ok = false;
          errorMessage = "Could not find zone with id '" + zoneIDString + "'";
        }
      } else {
        ok = false;
        errorMessage = "Could not parse id '" + zoneIDString + "'";
      }
    } else if(!zoneName.empty()) {
      try {
        Zone& zone = getDSS().getApartment().getZone(zoneName);
        pZone = &zone;
      } catch(runtime_error& e) {
        ok = false;
        errorMessage = "Could not find zone named '" + zoneName + "'";
      }
    } else {
      ok = false;
      errorMessage = "Need parameter name or id to identify zone";
    }
    if(ok) {
      Group* pGroup = NULL;
      string groupName = _parameter["groupName"];
      string groupIDString = _parameter["groupID"];
      if(!groupName.empty()) {
        try {
          pGroup = pZone->getGroup(groupName);
          if(pGroup == NULL) {
            // TODO: this might better be done by the zone
            throw runtime_error("dummy");
          }
        } catch(runtime_error& e) {
          errorMessage = "Could not find group with name '" + groupName + "'";
          ok = false;
        }
      } else if(!groupIDString.empty()) {
        try {
          int groupID = strToIntDef(groupIDString, -1);
          if(groupID != -1) {
            pGroup = pZone->getGroup(groupID);
            if(pGroup == NULL) {
              // TODO: this might better be done by the zone
              throw runtime_error("dummy");
            }
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
        if(isDeviceInterfaceCall(_method)) {
          IDeviceInterface* interface = NULL;
          if(pGroup != NULL) {
            interface = pGroup;
          }
          if(ok) {
            if(interface == NULL) {
              interface = pZone;
            }
            return callDeviceInterface(_method, _parameter, _arg, interface, _session);
          }
        } else if(endsWith(_method, "/getLastCalledScene")) {
          int lastScene = 0;
          if(pGroup != NULL) {
            lastScene = pGroup->getLastCalledScene();
          } else if(pZone != NULL) {
            lastScene = pZone->getGroup(0)->getLastCalledScene();
          } else {
            // should never reach here because ok, would be false
            assert(false);
          }
          stringstream sstream;
          sstream << "{" << ToJSONValue("scene") << ":" << ToJSONValue(lastScene) << "}";
          return JSONOk(sstream.str());
        } else if(endsWith(_method, "/getName")) {
          stringstream sstream;
          sstream << "{" << ToJSONValue("name") << ":" << ToJSONValue(pZone->getName()) << "}";
          return JSONOk(sstream.str());
        } else if(endsWith(_method, "/setName")) {
          pZone->setName(_parameter["newName"]);
          return ResultToJSON(true);
        } else {
          _handled = false;
          return "";
        }
      }
    }
    if(!ok) {
      return ResultToJSON(ok, errorMessage);
    } else {
      return "";
    }
  } // handleZoneCall

  string WebServer::handleDeviceCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    bool ok = true;
    string errorMessage;
    _handled = true;
    string deviceName = _parameter["name"];
    string deviceDSIDString = _parameter["dsid"];
    Device* pDevice = NULL;
    if(!deviceDSIDString.empty()) {
      dsid_t deviceDSID = dsid_t::fromString(deviceDSIDString);
      if(!(deviceDSID == NullDSID)) {
        try {
          Device& device = getDSS().getApartment().getDeviceByDSID(deviceDSID);
          pDevice = &device;
        } catch(runtime_error& e) {
          ok = false;
          errorMessage = "Could not find device with dsid '" + deviceDSIDString + "'";
        }
      } else {
        ok = false;
        errorMessage = "Could not parse dsid '" + deviceDSIDString + "'";
      }
    } else if(!deviceName.empty()) {
      try {
        Device& device = getDSS().getApartment().getDeviceByName(deviceName);
        pDevice = &device;
      } catch(runtime_error&  e) {
        ok = false;
        errorMessage = "Could not find device named '" + deviceName + "'";
      }
    } else {
      ok = false;
      errorMessage = "Need parameter name or dsid to identify device";
    }
    if(ok) {
      if(isDeviceInterfaceCall(_method)) {
        return callDeviceInterface(_method, _parameter, _arg, pDevice, _session);
      } else if(beginsWith(_method, "device/getGroups")) {
        int numGroups = pDevice->getGroupsCount();
        stringstream sstream;
        sstream << "{ " << ToJSONValue("groups") << ": [";
        bool first = true;
        for(int iGroup = 0; iGroup < numGroups; iGroup++) {
          if(!first) {
            sstream << ", ";
          }
          first = false;
          try {
            Group& group = pDevice->getGroupByIndex(iGroup);
            sstream << "{ " << ToJSONValue("id") << ":" << group.getID();
            if(!group.getName().empty()) {
              sstream << ", " << ToJSONValue("name") << ":" << ToJSONValue(group.getName());
            }
          } catch (runtime_error&) {
            log("Group only present at device level");
          }
          sstream << "}";
        }
        sstream << "]}";
        return sstream.str();
      } else if(beginsWith(_method, "device/getState")) {
        stringstream sstream;
        sstream << "{ " << ToJSONValue("isOn") << ":" << ToJSONValue(pDevice->isOn()) << " }";
        return JSONOk(sstream.str());
      } else if(beginsWith(_method, "device/getName")) {
        stringstream sstream;
        sstream << "{ " << ToJSONValue("name") << ":" << ToJSONValue(pDevice->getName()) << " }";
        return JSONOk(sstream.str());
      } else if(beginsWith(_method, "device/setName")) {
        pDevice->setName(_parameter["newName"]);
        return ResultToJSON(true);
      } else if(beginsWith(_method, "device/getLocation")) {
        const DeviceLocation& location = pDevice->getLocation();
        stringstream sstream;
        sstream << "{" << ToJSONValue("x") << ":" << ToJSONValue(location.get<0>()) << ","
                       << ToJSONValue("y") << ":" << ToJSONValue(location.get<1>()) << ","
                       << ToJSONValue("z") << ":" << ToJSONValue(location.get<2>())
                << "}";
        return JSONOk(sstream.str());
      } else if(beginsWith(_method, "device/setLocation")) {
        DeviceLocation location = pDevice->getLocation();
        string strParam = _parameter["x"];
        if(!strParam.empty()) {
          boost::get<0>(location) = strToDouble(strParam);
        }
        strParam = _parameter["y"];
        if(!strParam.empty()) {
          boost::get<1>(location) = strToDouble(strParam);
        }
        strParam = _parameter["z"];
        if(!strParam.empty()) {
          boost::get<2>(location) = strToDouble(strParam);
        }
        pDevice->setLocation(location);
        return ResultToJSON(true);
      } else {
        _handled = false;
        return "";
      }
    } else {
      return ResultToJSON(ok, errorMessage);
    }
  } // handleDeviceCall

  string WebServer::handleCircuitCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    _handled = true;
    string idString = _parameter["id"];
    if(idString.empty()) {
      return ResultToJSON(false, "missing parameter id");
    }
    dsid_t dsid = dsid_t::fromString(idString);
    if(dsid == NullDSID) {
      return ResultToJSON(false, "could not parse dsid");
    }
    try {
      Modulator& modulator = getDSS().getApartment().getModulatorByDSID(dsid);
      if(endsWith(_method, "circuit/getName")) {
        return JSONOk("{ " + ToJSONValue("name") + ": " + ToJSONValue(modulator.getName()) + "}");
      } else if(endsWith(_method, "circuit/setName")) {
        modulator.setName(_parameter["newName"]);
        return ResultToJSON(true);
      } else if(endsWith(_method, "circuit/getEnergyBorder")) {
        stringstream sstream;
        sstream << "{" << ToJSONValue("orange") << ":" << ToJSONValue(modulator.getEnergyLevelOrange());
        sstream << "," << ToJSONValue("red") << ":" << ToJSONValue(modulator.getEnergyLevelRed());
        sstream << "}";
        return JSONOk(sstream.str());
      } else if(endsWith(_method, "/getConsumption")) {
        return JSONOk("{ " + ToJSONValue("consumption") + ": " +  uintToString(modulator.getPowerConsumption()) +"}");
      } else if(endsWith(_method, "/getEnergyMeterValue")) {
        return JSONOk("{ " + ToJSONValue("metervalue") + ": " +  uintToString(modulator.getEnergyMeterValue()) +"}");
      } else {
        _handled = false;
      }
    } catch(runtime_error&) {
      return ResultToJSON(false, "could not find modulator with given dsid");
    }
    return "";
  } // handleCircuitCall


  string WebServer::handleSetCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    _handled = false;
    return "";
  } // handleSetCall

  string WebServer::handlePropertyCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    _handled = true;
    string propName = _parameter["path"];
    if(propName.empty()) {
      return ResultToJSON(false, "Need parameter \'path\' for property operations");
    }
    PropertyNode* node = getDSS().getPropertySystem().getProperty(propName);

    if(endsWith(_method, "/getString")) {
      if(node == NULL) {
        return ResultToJSON(false, "Could not find node named '" + propName + "'");
      }
      try {
        stringstream sstream;
        sstream << "{ " << ToJSONValue("value") << ": " << ToJSONValue(node->getStringValue()) << "}";
        return JSONOk(sstream.str());
      } catch(PropertyTypeMismatch& ex) {
        return ResultToJSON(false, string("Error getting property: '") + ex.what() + "'");
      }
    } else if(endsWith(_method, "/getInteger")) {
      if(node == NULL) {
        return ResultToJSON(false, "Could not find node named '" + propName + "'");
      }
      try {
        stringstream sstream;
        sstream << "{ " << ToJSONValue("value") << ": " << ToJSONValue(node->getIntegerValue()) << "}";
        return JSONOk(sstream.str());
      } catch(PropertyTypeMismatch& ex) {
        return ResultToJSON(false, string("Error getting property: '") + ex.what() + "'");
      }
    } else if(endsWith(_method, "/getBoolean")) {
      if(node == NULL) {
        return ResultToJSON(false, "Could not find node named '" + propName + "'");
      }
      try {
        stringstream sstream;
        sstream << "{ " << ToJSONValue("value") << ": " << ToJSONValue(node->getBoolValue()) << "}";
        return JSONOk(sstream.str());
      } catch(PropertyTypeMismatch& ex) {
        return ResultToJSON(false, string("Error getting property: '") + ex.what() + "'");
      }
    } else if(endsWith(_method, "/setString")) {
      string value = _parameter["value"];
      if(node == NULL) {
        node = getDSS().getPropertySystem().createProperty(propName);
      }
      try {
        node->setStringValue(value);
      } catch(PropertyTypeMismatch& ex) {
        return ResultToJSON(false, string("Error setting property: '") + ex.what() + "'");
      }
      return JSONOk();
    } else if(endsWith(_method, "/setBoolean")) {
      string strValue = _parameter["value"];
      bool value;
      if(strValue == "true") {
        value = true;
      } else if(strValue == "false") {
        value = false;
      } else {
        return ResultToJSON(false, "Expected 'true' or 'false' for parameter 'value' but got: '" + strValue + "'");
      }
      if(node == NULL) {
        node = getDSS().getPropertySystem().createProperty(propName);
      }
      try {
        node->setBooleanValue(value);
      } catch(PropertyTypeMismatch& ex) {
        return ResultToJSON(false, string("Error setting property: '") + ex.what() + "'");
      }
      return JSONOk();
    } else if(endsWith(_method, "/setInteger")) {
      string strValue = _parameter["value"];
      int value;
      try {
        value = strToInt(strValue);
      } catch(...) {
        return ResultToJSON(false, "Could not convert parameter 'value' to string. Got: '" + strValue + "'");
      }
      if(node == NULL) {
        node = getDSS().getPropertySystem().createProperty(propName);
      }
      try {
        node->setIntegerValue(value);
      } catch(PropertyTypeMismatch& ex) {
        return ResultToJSON(false, string("Error setting property: '") + ex.what() + "'");
      }
      return JSONOk();
    } else if(endsWith(_method, "/getChildren")) {
      if(node == NULL) {
        return ResultToJSON(false, "Could not find node named '" + propName + "'");
      }
      stringstream sstream;
      sstream << "[";
      bool first = true;
      for(int iChild = 0; iChild < node->getChildCount(); iChild++) {
        if(!first) {
          sstream << ",";
        }
        first = false;
        PropertyNode* cnode = node->getChild(iChild);
        sstream << "{" << ToJSONValue("name") << ":" << ToJSONValue(cnode->getName());
        sstream << "," << ToJSONValue("type") << ":" << ToJSONValue(getValueTypeAsString(cnode->getValueType()));
        sstream << "}";
      }
      sstream << "]";
      return JSONOk(sstream.str());
    } else if(endsWith(_method, "/getType")) {
      if(node == NULL) {
        return ResultToJSON(false, "Could not find node named '" + propName + "'");
      }
      stringstream sstream;
      sstream << ":" << ToJSONValue("type") << ":" << ToJSONValue(getValueTypeAsString(node->getValueType())) << "}";
      return JSONOk(sstream.str());
    } else {
      _handled = false;
    }
    return "";
  } // handlePropertyCall

  string WebServer::handleEventCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    _handled = true;
    string result;
    if(endsWith(_method, "/raise")) {
      string name = _parameter["name"];
      string location = _parameter["location"];
      string context = _parameter["context"];

      boost::shared_ptr<Event> e(new Event(name));
      if(!context.empty()) {
        e->setContext(context);
      }
      if(!location.empty()) {
        e->setLocation(location);
      }
      getDSS().getEventQueue().pushEvent(e);
      return ResultToJSON(true);
    } else {
      _handled = false;
    }
    return result;
  } // handleEventCall

  string WebServer::handleStructureCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    _handled = true;
    if(endsWith(_method, "structure/zoneAddDevice")) {
      bool ok = true;

      string devidStr = _parameter["devid"];
      if(!devidStr.empty()) {
        dsid_t devid = dsid::fromString(devidStr);

        Device& dev = DSS::getInstance()->getApartment().getDeviceByDSID(devid);

        string zoneIDStr = _parameter["zone"];
        if(!zoneIDStr.empty()) {
          try {
            int zoneID = strToInt(zoneIDStr);
            DeviceReference devRef(dev, DSS::getInstance()->getApartment());
            DSS::getInstance()->getApartment().getZone(zoneID).addDevice(devRef);
          } catch(runtime_error&) {
            ok = false;
          }
        }
        return ResultToJSON(ok, "");
      } else {
        return ResultToJSON(false, "Need parameter devid");
      }
    } else if(endsWith(_method, "structure/addZone")) {
      bool ok = false;
      int zoneID = -1;
      int modulatorID = -1;

      string zoneIDStr = _parameter["zoneID"];
      if(!zoneIDStr.empty()) {
        zoneID = strToIntDef(zoneIDStr, -1);
      }
      string modIDStr = _parameter["modulatorID"];
      if(!modIDStr.empty()) {
        modulatorID = strToIntDef(modIDStr, -1);
      }
      if(zoneID != -1 && modulatorID != -1) {
        try {
          Modulator& modulator = DSS::getInstance()->getApartment().getModulatorByBusID(modulatorID);
          DSS::getInstance()->getApartment().allocateZone(modulator, zoneID);
          ok = true;
        } catch(runtime_error&) {
        }
      }
      return ResultToJSON(true, "");
    } else {
      _handled = false;
      return "";
    }
  } // handleStructureCall

  string WebServer::handleSimCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    _handled = true;
    if(beginsWith(_method, "sim/switch")) {
      if(endsWith(_method, "/switch/pressed")) {
        int buttonNr = strToIntDef(_parameter["buttonnr"], -1);
        if(buttonNr == -1) {
          return ResultToJSON(false, "Invalid button number");
        }

        int zoneID = strToIntDef(_parameter["zoneID"], -1);
        if(zoneID == -1) {
          return ResultToJSON(false, "Could not parse zoneID");
        }
        int groupID = strToIntDef(_parameter["groupID"], -1);
        if(groupID == -1) {
          return ResultToJSON(false, "Could not parse groupID");
        }
        try {
          Zone& zone = getDSS().getApartment().getZone(zoneID);
          Group* pGroup = zone.getGroup(groupID);

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
            pGroup->increaseValue();
            break;
          case 8: // down
            pGroup->decreaseValue();
            break;
          case 4: // left
            pGroup->previousScene();
            break;
          case 6: // right
            pGroup->nextScene();
            break;
          case 5:
            {
              if(groupID == GroupIDGreen) {
                getDSS().getApartment().getGroup(0).callScene(SceneBell);
              } else if(groupID == GroupIDRed){
                getDSS().getApartment().getGroup(0).callScene(SceneAlarm);
              } else {
                const int lastScene = pGroup->getLastCalledScene();
                if(lastScene == SceneOff || lastScene == SceneDeepOff ||
                   lastScene == SceneStandBy || lastScene == ScenePanic)
                {
                  pGroup->callScene(Scene1);
                } else {
                  pGroup->callScene(SceneOff);
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
    } else if(beginsWith(_method, "sim/addDevice")) {
      string type = _parameter["type"];
      string dsidStr = _parameter["dsid"];
      // TODO: not finished yet ;)
    } else {
      _handled = false;
    }
    return "";
  } // handleSimCall

  string WebServer::handleDebugCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    _handled = true;
    if(endsWith(_method, "/sendFrame")) {
      int destination = strToIntDef(_parameter["destination"],0) & 0x3F;
      bool broadcast = _parameter["broadcast"] == "true";
      int counter = strToIntDef(_parameter["counter"], 0x00) & 0x03;
      int command = strToIntDef(_parameter["command"], 0x09 /* request */) & 0x00FF;
      int length = strToIntDef(_parameter["length"], 0x00) & 0x0F;

      std::cout
           << "sending frame: "
           << "\ndest:    " << destination
           << "\nbcst:    " << broadcast
           << "\ncntr:    " << counter
           << "\ncmd :    " << command
           << "\nlen :    " << length << std::endl;

      DS485CommandFrame* frame = new DS485CommandFrame();
      frame->getHeader().setBroadcast(broadcast);
      frame->getHeader().setDestination(destination);
      frame->getHeader().setCounter(counter);
      frame->setCommand(command);
      for(int iByte = 0; iByte < length; iByte++) {
        uint8_t byte = strToIntDef(_parameter[string("payload_") + intToString(iByte+1)], 0xFF);
        cout << "b" << dec << iByte << ": " << hex << (int)byte << "\n";
        frame->getPayload().add<uint8_t>(byte);
      }
      cout << dec <<"done" << endl;
      DS485Interface* intf = &DSS::getInstance()->getDS485Interface();
      DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(intf);
      if(proxy != NULL) {
        proxy->sendFrame(*frame);
      } else {
        delete frame;
      }
      return ResultToJSON(true);
    } else {
      _handled = false;
      return "";
    }
  } // handleDebugCall
  
  string WebServer::handleMeteringCall(const std::string& _method, HashMapConstStringString& _parameter, struct shttpd_arg* _arg, bool& _handled, Session* _session) {
    if(endsWith(_method, "/getResolutions")) {
      _handled = true;
      std::vector<boost::shared_ptr<MeteringConfigChain> > meteringConfig = getDSS().getMetering().getConfig();
      stringstream sstream;
      sstream << "{" << ToJSONValue("resolutions") << ":" << "[";
      for(unsigned int iConfig = 0; iConfig < meteringConfig.size(); iConfig++) {
        boost::shared_ptr<MeteringConfigChain> cConfig = meteringConfig[iConfig];
        for(int jConfig = 0; jConfig < cConfig->size(); jConfig++) {
          sstream << "{" << ToJSONValue("type") << ":" << (cConfig->isEnergy() ? ToJSONValue("energy") : ToJSONValue("consumption") ) << ","
                  << ToJSONValue("unit") << ":" << ToJSONValue(cConfig->getUnit()) << ","
                  << ToJSONValue("resolution") << ":" << ToJSONValue(cConfig->getResolution(jConfig)) << "}";
          if(jConfig < cConfig->size() && iConfig < meteringConfig.size()) {
            sstream << ",";
          }
        }
      }
      sstream << "]"  << "}";
      return JSONOk(sstream.str());
    } else if(endsWith(_method, "/getSeries")) {
      _handled = true;
      stringstream sstream;
      sstream << "{ " << ToJSONValue("series") << ": [";
      bool first = true;
      vector<Modulator*>& modulators = getDSS().getApartment().getModulators();
      foreach(Modulator* modulator, modulators) {
        if(!first) {
          sstream << ",";
        }
        first = false;
        sstream << "{ " << ToJSONValue("dsid") << ": " << ToJSONValue(modulator->getDSID().toString());
        sstream << ", " << ToJSONValue("type") << ": " << ToJSONValue("energy");
        sstream << "},";
        sstream << "{ " << ToJSONValue("dsid") << ": " << ToJSONValue(modulator->getDSID().toString());
        sstream << ", " << ToJSONValue("type") << ": " << ToJSONValue("consumption");
        sstream << "}";
      }
      sstream << "]}";
      return JSONOk(sstream.str());
    } else if(endsWith(_method, "/getValues")) { //?dsid=;n=,resolution=,type=
      _handled = true;
      std::string errorMessage;
      std::string deviceDSIDString = _parameter["dsid"];
      std::string resolutionString = _parameter["resolution"];
      std::string typeString = _parameter["type"];
      std::string fileSuffix;
      std::string storageLocation;
      std::string seriesPath;
      int resolution;
      bool energy;
      if(!deviceDSIDString.empty()) {
        dsid_t deviceDSID = dsid_t::fromString(deviceDSIDString);
        if(!(deviceDSID == NullDSID)) {
          try {
            getDSS().getApartment().getModulatorByDSID(deviceDSID);
          } catch(runtime_error& e) {
            return ResultToJSON(false, "Could not find device with dsid '" + deviceDSIDString + "'");
          }
        } else {
         return ResultToJSON(false, "Could not parse dsid '" + deviceDSIDString + "'");
        }
      } else {
        return ResultToJSON(false, "Could not parse dsid '" + deviceDSIDString + "'");
      }
      resolution = strToIntDef(resolutionString, -1);
      if(resolution == -1) {
        return ResultToJSON(false, "Need could not parse resolution '" + resolutionString + "'");
      }
      if(typeString.empty()) {        
        return ResultToJSON(false, "Need a type, 'energy' or 'consumption'");
      } else {
        if(typeString == "consumption") {
          energy = false;
        } else if(typeString == "energy") {
          energy = true;
        } else {
          return ResultToJSON(false, "Invalide type '" + typeString + "'");
        }
      }
      if(!resolutionString.empty()) {
        std::vector<boost::shared_ptr<MeteringConfigChain> > meteringConfig = getDSS().getMetering().getConfig();
        storageLocation = getDSS().getMetering().getStorageLocation();
        for(unsigned int iConfig = 0; iConfig < meteringConfig.size(); iConfig++) {
          boost::shared_ptr<MeteringConfigChain> cConfig = meteringConfig[iConfig];
          for(int jConfig = 0; jConfig < cConfig->size(); jConfig++) {
            if(cConfig->isEnergy() == energy && cConfig->getResolution(jConfig) == resolution) {
              fileSuffix = cConfig->getFilenameSuffix(jConfig);
            }
          }
        }
        if(fileSuffix.empty()) {
          return ResultToJSON(false, "No data for '" + typeString + "' and resolution '" + resolutionString + "'");
        } else {
          seriesPath = storageLocation + deviceDSIDString + "_" + fileSuffix + ".xml";
          log("_Trying to load series from " + seriesPath);
          if(fileExists(seriesPath)) {
            SeriesReader<CurrentValue> reader;
            boost::shared_ptr<Series<CurrentValue> > s = boost::shared_ptr<Series<CurrentValue> >(reader.readFromXML(seriesPath));
            std::deque<CurrentValue>* values = s->getExpandedValues();
            bool first = true;
            stringstream sstream;
            sstream << "{ " ;
            sstream << ToJSONValue("dsmid") << ":" << ToJSONValue(deviceDSIDString) << ",";
            sstream << ToJSONValue("type") << ":" << ToJSONValue(typeString) << ",";
            sstream << ToJSONValue("resolution") << ":" << ToJSONValue(resolutionString) << ",";
            sstream << ToJSONValue("values") << ": [";
            for(std::deque<CurrentValue>::iterator iValue = values->begin(), e = values->end(); iValue != e; iValue++)
            {
              if(!first) {
                sstream << ",";
              }
              first = false;
              sstream << "[" << iValue->getTimeStamp().secondsSinceEpoch()  << "," << iValue->getValue() << "]";
            }
            sstream << "]}";
            delete values;
            return JSONOk(sstream.str());
          } else {
            return ResultToJSON(false, "No data-file for '" + typeString + "' and resolution '" + resolutionString + "'");
          }
        }
      } else {
        return ResultToJSON(false, "Could not parse resolution '" + resolutionString + "'");
      }    
    } else if(endsWith(_method, "/getAggregatedValues")) { //?set=;n=,resolution=;type=
      _handled = false;
      return "";
    }
    _handled = false;
    return "";
  } // handleMeteringCall

  void WebServer::jsonHandler(struct shttpd_arg* _arg) {
    const string urlid = "/json/";
    string uri = shttpd_get_env(_arg, "REQUEST_URI");
    HashMapConstStringString paramMap = ParseParameter(shttpd_get_env(_arg, "QUERY_STRING"));

    string method = uri.substr(uri.find(urlid) + urlid.size());

    WebServer& self = DSS::getInstance()->getWebServer();
    self.log("Processing call to " + method);

    Session* session = NULL;
    string tokenStr = paramMap["token"];
    if(!tokenStr.empty()) {
      int token = strToIntDef(tokenStr, -1);
      if(token != -1) {
        SessionByID::iterator iEntry = self.m_Sessions.find(token);
        if(iEntry != self.m_Sessions.end()) {
          if(iEntry->second->isStillValid()) {
            Session& s = *iEntry->second;
            session = &s;
          }
        }
      }
    }

    string result;
    bool handled = false;
    if(beginsWith(method, "apartment/")) {
      result = self.handleApartmentCall(method, paramMap, _arg, handled, session);
    } else if(beginsWith(method, "zone/")) {
      result = self.handleZoneCall(method, paramMap, _arg, handled, session);
    } else if(beginsWith(method, "device/")) {
      result = self.handleDeviceCall(method, paramMap, _arg, handled, session);
    } else if(beginsWith(method, "circuit/")) {
      result = self.handleCircuitCall(method, paramMap, _arg, handled, session);
    } else if(beginsWith(method, "set/")) {
      result = self.handleSetCall(method, paramMap, _arg, handled, session);
    } else if(beginsWith(method, "property/")) {
      result = self.handlePropertyCall(method, paramMap, _arg, handled, session);
    } else if(beginsWith(method, "event/")) {
      result = self.handleEventCall(method, paramMap, _arg, handled, session);
    } else if(beginsWith(method, "structure/")) {
      result = self.handleStructureCall(method, paramMap, _arg, handled, session);
    } else if(beginsWith(method, "sim/")) {
      result = self.handleSimCall(method, paramMap, _arg, handled, session);
    } else if(beginsWith(method, "debug/")) {
      result = self.handleDebugCall(method, paramMap, _arg, handled, session);
    } else if(beginsWith(method, "metering/")) {
      result = self.handleMeteringCall(method, paramMap, _arg, handled, session);
    }

    if(!handled) {
      emitHTTPHeader(404, _arg, "application/json");
      result = "{ ok: " + ToJSONValue(false) + ", message: " + ToJSONValue("Call to unknown function") + " }";
      self.log("Unknown function '" + method + "'", lsError);
    } else {
      emitHTTPHeader(200, _arg, "application/json");
    }
    shttpd_printf(_arg, result.c_str());
    _arg->flags |= SHTTPD_END_OF_OUTPUT;
  } // jsonHandler

  void WebServer::httpBrowseProperties(struct shttpd_arg* _arg) {
    emitHTTPHeader(200, _arg);

    const string urlid = "/browse";
    string uri = shttpd_get_env(_arg, "REQUEST_URI");
    uri = urlDecode(uri);
    HashMapConstStringString paramMap = ParseParameter(shttpd_get_env(_arg, "QUERY_STRING"));

    string path = uri.substr(uri.find(urlid) + urlid.size());
    if(path.empty()) {
      path = "/";
    }
    shttpd_printf(_arg, "<html><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><body>");

    std::stringstream stream;
    stream << "<h1>" << path << "</h1>";
    stream << "<ul>";
    PropertyNode* node = DSS::getInstance()->getPropertySystem().getProperty(path);
    if(node != NULL) {
      for(int iNode = 0; iNode < node->getChildCount(); iNode++) {
        PropertyNode* cnode = node->getChild(iNode);

        stream << "<li>";
        if(cnode->getChildCount() != 0) {
          stream << "<a href=\"/browse" << path << "/" << cnode->getDisplayName() << "\">" << cnode->getDisplayName() << "</a>";
        } else {
          stream << cnode->getDisplayName();
        }
        stream << " : ";
        stream << getValueTypeAsString(cnode->getValueType()) << " : ";
        switch(cnode->getValueType()) {
        case vTypeBoolean:
          stream << cnode->getBoolValue();
          break;
        case vTypeInteger:
          stream << cnode->getIntegerValue();
          break;
        case vTypeNone:
          break;
        case vTypeString:
          stream << cnode->getStringValue();
          break;
        default:
          stream << "unknown value";
        }
      }
    }

    stream << "</ul></body></html>";
    shttpd_printf(_arg, stream.str().c_str());

    _arg->flags |= SHTTPD_END_OF_OUTPUT;
  } // httpBrowseProperties

}
