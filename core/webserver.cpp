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
#include "web/webserverplugin.h"
#include "core/setbuilder.h"
#include "core/structuremanipulator.h"

#include <iostream>
#include <sstream>

#include <boost/shared_ptr.hpp>

namespace dss {
  //============================================= WebServer

  WebServer::WebServer(DSS* _pDSS)
    : Subsystem(_pDSS, "WebServer"), m_mgContext(0)
  {
  } // ctor

  WebServer::~WebServer() {
    if (m_mgContext) {
      mg_stop(m_mgContext);
    }
  } // dtor

  void WebServer::initialize() {
    Subsystem::initialize();

    log("Starting Webserver...");
    m_mgContext = mg_start();

    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "webroot", getDSS().getDataDirectory() + "webroot/", true, false);
    getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "ports", "8080", true, false);

    setupAPI();
  } // initialize

  void WebServer::loadPlugins() {
    PropertyNodePtr pluginsNode = getDSS().getPropertySystem().getProperty(getConfigPropertyBasePath() + "plugins");
    if(pluginsNode != NULL) {
      log("Found plugins node, trying to loading plugins", lsInfo);
      pluginsNode->foreachChildOf(*this, &WebServer::loadPlugin);
    }
  } // loadPlugins

  void WebServer::loadPlugin(PropertyNode& _node) {
    PropertyNodePtr pFileNode = _node.getProperty("file");
    PropertyNodePtr pURINode = _node.getProperty("uri");

    if(pFileNode == NULL) {
      log("loadPlugin: Missing subnode name 'file' on node " + _node.getDisplayName(), lsError);
      return;
    }
    if(pURINode == NULL) {
      log("loadPlugin: Missing subnode 'uri on node " + _node.getDisplayName(), lsError);
    }
    WebServerPlugin* plugin = new WebServerPlugin(pURINode->getStringValue(), pFileNode->getStringValue());
    try {
      plugin->load();
    } catch(std::runtime_error& e) {
      delete plugin;
      plugin = NULL;
      log(string("Cought exception while loading: ") + e.what(), lsError);
      return;
    }
    m_Plugins.push_back(plugin);

    log("Registering " + pFileNode->getStringValue() + " for URI '" + pURINode->getStringValue() + "'");
    mg_set_uri_callback(m_mgContext, pURINode->getStringValue().c_str(), &httpPluginCallback, plugin);
  } // loadPlugin

  void WebServer::setupAPI() {
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
    clsApartment.addMethod("rescan")
      .withDocumentation("Rescans all circuits of the apartment");

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
      .withParameter("value", "double", true)
      .withDocumentation("Sets the output value of the device to value");
    clsDevice.addMethod("setRawValue")
      .withParameter("value", "integer", true)
      .withParameter("parameterID", "integer", true)
      .withParameter("size", "integer", true)
      .withDocumentation("Sets the value of register parameterID to value");
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
    clsCircuit.addMethod("rescan")
       .withDocumentation("Rescans the circuit");

    RestfulClass& clsProp = api.addClass("property")
        .withInstanceParameter("path", "string", true);
    clsProp.addMethod("getString")
        .withDocumentation("Returns the string value of the property", "This will fail if the property is not of type 'string'.");
    clsProp.addMethod("getInteger")
        .withDocumentation("Returns the integer value of the property", "This will fail if the property is not of type 'integer'.");
    clsProp.addMethod("getBoolean")
        .withDocumentation("Returns the boolean value of the property", "This will fail if the property is not of type 'boolean'.");
    clsProp.addMethod("setString")
        .withParameter("value", "string", true)
        .withDocumentation("Sets the string value of the property", "This will fail if the property is not of type 'string'.");
    clsProp.addMethod("setInteger")
        .withParameter("value", "integer", true)
        .withDocumentation("Sets the integer value of the property", "This will fail if the property is not of type 'integer'.");
    clsProp.addMethod("setBoolean")
        .withParameter("value", "boolean", true)
        .withDocumentation("Sets the boolean value of the property", "This will fail if the property is not of type 'boolean'.");
    clsProp.addMethod("getChildren")
        .withParameter("value", "string", true)
        .withDocumentation("Returns an array of the nodes children");
    clsProp.addMethod("setString")
        .withDocumentation("Returns the type of the node");

    RestfulClass& clsEvent = api.addClass("event");
    clsEvent.addMethod("raise")
       .withParameter("name", "string", true)
       .withParameter("context", "string", false)
       .withParameter("location", "string", false)
       .withDocumentation("Raises an event", "The context describes the source of the event. The location, if provided, defines where any action that is taken "
           "by any subscription should happen.");
    
    RestfulClass& clsSystem = api.addClass("system");
    clsSystem.addMethod("version")
      .withDocumentation("Returns the dss version", 
                         "This method returns the version string of the dss");

    RestfulClass& clsSet = api.addClass("set")
        .withInstanceParameter("self", "string", false);
    clsSet.addMethod("fromApartment")
        .withDocumentation("Creates a set that contains all devices of the apartment");
    clsSet.addMethod("byZone")
        .withParameter("zoneID", "integer", false)
        .withParameter("zoneName", "string", false)
        .withDocumentation("Restricts the set to the given zone");
    clsSet.addMethod("byGroup")
        .withParameter("groupID", "integer", false)
        .withParameter("groupName", "integer", false)
        .withDocumentation("Restricts the set to the given group");
    clsSet.addMethod("byDSID")
        .withParameter("dsid", "dsid", true)
        .withDocumentation("Restricts the set to the given dsid");
    clsSet.addMethod("add")
        .withParameter("other", "string", true)
        .withDocumentation("Combines the set self with other");
    clsSet.addMethod("subtract")
        .withParameter("other", "string", true)
        .withDocumentation("Subtracts the set self from other");
    clsSet.addMethod("turnOn")
        .withDocumentation("Turns on all devices of the set.");
    clsSet.addMethod("turnOff")
        .withDocumentation("Turns off all devices of the set.");
    clsSet.addMethod("increaseValue")
        .withDocumentation("Increases the main value on all devices of the set.");
    clsSet.addMethod("decreaseValue")
        .withDocumentation("Decreases the main value on all devices of the set.");
    clsSet.addMethod("enable")
        .withDocumentation("Enables all devices of the set.");
    clsSet.addMethod("disable")
        .withDocumentation("Disables all devices of the set.", "A disabled device will react only to an enable call.");
    clsSet.addMethod("startDim")
      .withParameter("direction", "string", false)
      .withDocumentation("Starts dimming the devices of the set.");
    clsSet.addMethod("endDim")
      .withDocumentation("Stops dimming the devices of the set.");
    clsSet.addMethod("setValue")
      .withParameter("value", "integer", true)
      .withDocumentation("Sets the output value of all devices of the set to value.");
    clsSet.addMethod("callScene")
      .withParameter("sceneNr", "integer", true)
      .withDocumentation("Calls the scene sceneNr on all devices of the set.");
    clsSet.addMethod("saveScene")
      .withParameter("sceneNr", "integer", true)
      .withDocumentation("Saves the current output value to sceneNr.");
    clsSet.addMethod("undoScene")
      .withParameter("sceneNr", "integer", true)
      .withDocumentation("Undoes setting the value of sceneNr.");
    clsApartment.addMethod("getConsumption")
      .withDocumentation("Returns the consumption of all devices in the set in mW.");

    RestfulAPIWriter::writeToXML(api, "doc/json_api.xml");
  } // setupAPI

  void WebServer::doStart() {
    string ports = DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "ports");
    log("Webserver: Listening on port(s) " + ports);
    mg_set_option(m_mgContext, "ports", ports.c_str());

    string aliases = string("/=") + DSS::getInstance()->getPropertySystem().getStringValue(getConfigPropertyBasePath() + "webroot");
    log("Webserver: Configured aliases: " + aliases);
    mg_set_option(m_mgContext, "aliases", aliases.c_str());

    mg_set_uri_callback(m_mgContext, "/browse/*", &httpBrowseProperties, NULL);
    mg_set_uri_callback(m_mgContext, "/json/*", &jsonHandler, NULL);

    loadPlugins();

    log("Webserver started", lsInfo);
  } // start

  const char* httpCodeToMessage(const int _code) {
    if(_code == 400) {
      return "Bad Request";
    } else if(_code == 401) {
      return "Unauthorized\r\nWWW-Authenticate: Basic realm=\"dSS\"";
    } else if(_code == 403) {
      return "Forbidden";
    } else if(_code == 500) {
      return "Internal Server Error";
    } else {
      return "OK";
    }
  }

  void WebServer::emitHTTPHeader(int _code, struct mg_connection* _connection, const std::string& _contentType) {
    std::stringstream sstream;
    sstream << "HTTP/1.1 " << _code << ' ' << httpCodeToMessage(_code) << "\r\n";
    sstream << "Content-Type: " << _contentType << "; charset=utf-8\r\n\r\n";
    string tmp = sstream.str();
    mg_write(_connection, tmp.c_str(), tmp.length());
  } // emitHTTPHeader

  HashMapConstStringString parseParameter(const char* _params) {
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

  string WebServer::ResultToJSON(const bool _ok, const string& _message) {
    std::stringstream sstream;
    sstream << "{ " << ToJSONValue("ok") << ":" << ToJSONValue(_ok);
    if(!_message.empty()) {
      sstream << ", " << ToJSONValue("message") << ":" << ToJSONValue(_message);
    }
    sstream << "}";
    if(!_ok) {
      log("JSON call failed: '" + _message + "'");
    }
    return sstream.str();
  } // resultToJSON

  string JSONOk(const string& _innerResult = "") {
    std::stringstream sstream;
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
            << ", \"isPresent\":"  << ToJSONValue(_device.getDevice().isPresent())
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
    sstream << ToJSONValue("isPresent") << ": " << ToJSONValue(_group.isPresent()) << ", ";
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

  string ToJSONValue(Zone& _zone, bool _includeDevices = true) {
    std::stringstream sstream;
    sstream << "{ \"id\": " << _zone.getZoneID() << ",";
    string name = _zone.getName();
    if(name.size() == 0) {
      name = string("Zone ") + intToString(_zone.getZoneID());
    }
    sstream << ToJSONValue("name") << ": " << ToJSONValue(name) << ", ";
    sstream << ToJSONValue("isPresent") << ": " << ToJSONValue(_zone.isPresent());
    if(_zone.getFirstZoneOnModulator() != -1) {
      sstream << ", " << ToJSONValue("firstZoneOnModulator") 
              << ": " << ToJSONValue(_zone.getFirstZoneOnModulator());
    }

    if(_includeDevices) {
      sstream << ", ";
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
    }

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

  string WebServer::callDeviceInterface(const std::string& _method, HashMapConstStringString& _parameter, struct mg_connection* _connection, IDeviceInterface* _interface, Session* _session) {
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

  string WebServer::handleApartmentCall(const std::string& _method, HashMapConstStringString& _parameter, struct mg_connection* _connection, bool& _handled, Session* _session) {
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
        } catch(std::runtime_error& e) {
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
        } catch(std::runtime_error& e) {
          errorMessage = "Could not find group with ID '" + groupIDString + "'";
          ok = false;
        }
      }
      if(ok) {
        if(interface == NULL) {
          interface = &getDSS().getApartment().getGroup(GroupIDBroadcast);
        }
        return callDeviceInterface(_method, _parameter, _connection, interface, _session);
      } else {
        std::stringstream sstream;
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
        std::stringstream sstream;
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
          sstream << ", " << ToJSONValue("hwVersion") << ": " << ToJSONValue(modulator->getHardwareVersion());
          sstream << ", " << ToJSONValue("swVersion") << ": " << ToJSONValue(modulator->getSoftwareVersion());
          sstream << ", " << ToJSONValue("hwName") << ": " << ToJSONValue(modulator->getHardwareName());
          sstream << ", " << ToJSONValue("deviceType") << ": " << ToJSONValue(modulator->getDeviceType());
          sstream << ", " << ToJSONValue("isPresent") << ": " << ToJSONValue(modulator->isPresent());
          sstream << "}";
        }
        sstream << "]}";
        return JSONOk(sstream.str());
      } else if(endsWith(_method, "/getName")) {
        std::stringstream sstream;
        sstream << "{" << ToJSONValue("name") << ":" << ToJSONValue(getDSS().getApartment().getName()) << "}";
        return JSONOk(sstream.str());
      } else if(endsWith(_method, "/setName")) {
        getDSS().getApartment().setName(_parameter["newName"]);
        result = ResultToJSON(true);
      } else if(endsWith(_method, "/rescan")) {
        getDSS().getApartment().initializeFromBus();
        result = ResultToJSON(true);
      } else {
        _handled = false;
      }
      return result;
    }
  } // handleApartmentCall

  string WebServer::handleZoneCall(const std::string& _method, HashMapConstStringString& _parameter, struct mg_connection* _connection, bool& _handled, Session* _session) {
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
        } catch(std::runtime_error& e) {
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
      } catch(std::runtime_error& e) {
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
            throw std::runtime_error("dummy");
          }
        } catch(std::runtime_error& e) {
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
              throw std::runtime_error("dummy");
            }
          } else {
            errorMessage = "Could not parse group id '" + groupIDString + "'";
            ok = false;
          }
        } catch(std::runtime_error& e) {
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
            return callDeviceInterface(_method, _parameter, _connection, interface, _session);
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
          std::stringstream sstream;
          sstream << "{" << ToJSONValue("scene") << ":" << ToJSONValue(lastScene) << "}";
          return JSONOk(sstream.str());
        } else if(endsWith(_method, "/getName")) {
          std::stringstream sstream;
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

  string WebServer::handleDeviceCall(const std::string& _method, HashMapConstStringString& _parameter, struct mg_connection* _connection, bool& _handled, Session* _session) {
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
        } catch(std::runtime_error& e) {
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
      } catch(std::runtime_error&  e) {
        ok = false;
        errorMessage = "Could not find device named '" + deviceName + "'";
      }
    } else {
      ok = false;
      errorMessage = "Need parameter name or dsid to identify device";
    }
    if(ok) {
      if(isDeviceInterfaceCall(_method)) {
        return callDeviceInterface(_method, _parameter, _connection, pDevice, _session);
      } else if(beginsWith(_method, "device/getGroups")) {
        int numGroups = pDevice->getGroupsCount();
        std::stringstream sstream;
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
          } catch(std::runtime_error&) {
            log("Group only present at device level");
          }
          sstream << "}";
        }
        sstream << "]}";
        return sstream.str();
      } else if(beginsWith(_method, "device/getState")) {
        std::stringstream sstream;
        sstream << "{ " << ToJSONValue("isOn") << ":" << ToJSONValue(pDevice->isOn()) << " }";
        return JSONOk(sstream.str());
      } else if(beginsWith(_method, "device/getName")) {
        std::stringstream sstream;
        sstream << "{ " << ToJSONValue("name") << ":" << ToJSONValue(pDevice->getName()) << " }";
        return JSONOk(sstream.str());
      } else if(beginsWith(_method, "device/setName")) {
        pDevice->setName(_parameter["newName"]);
        return ResultToJSON(true);
      } else if(beginsWith(_method, "device/setRawValue")) {
        int value = strToIntDef(_parameter["value"], -1);
        if(value == -1) {
          return ResultToJSON(false, "Invalid or missing parameter 'value'");
        }
        int parameterID = strToIntDef(_parameter["parameterID"], -1);
        if(parameterID == -1) {
          return ResultToJSON(false, "Invalid or missing parameter 'parameterID'");
        }
        int size = strToIntDef(_parameter["size"], -1);
        if(size == -1) {
          return ResultToJSON(false, "Invalid or missing parameter 'size'");
        }

        pDevice->setRawValue(value, parameterID, size);
        return JSONOk();
      } else {
        _handled = false;
        return "";
      }
    } else {
      return ResultToJSON(ok, errorMessage);
    }
  } // handleDeviceCall

  string WebServer::handleCircuitCall(const std::string& _method, HashMapConstStringString& _parameter, struct mg_connection* _connection, bool& _handled, Session* _session) {
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
        std::stringstream sstream;
        sstream << "{" << ToJSONValue("orange") << ":" << ToJSONValue(modulator.getEnergyLevelOrange());
        sstream << "," << ToJSONValue("red") << ":" << ToJSONValue(modulator.getEnergyLevelRed());
        sstream << "}";
        return JSONOk(sstream.str());
      } else if(endsWith(_method, "/getConsumption")) {
        return JSONOk("{ " + ToJSONValue("consumption") + ": " +  uintToString(modulator.getPowerConsumption()) +"}");
      } else if(endsWith(_method, "/getEnergyMeterValue")) {
        return JSONOk("{ " + ToJSONValue("metervalue") + ": " +  uintToString(modulator.getEnergyMeterValue()) +"}");
      } else if(endsWith(_method, "/rescan")) {
        try {
          getDSS().getApartment().scanModulator(modulator);
          return ResultToJSON(true);
        } catch(std::runtime_error& err) {
          return ResultToJSON(false, err.what());
        }
      } else {
        _handled = false;
      }
    } catch(std::runtime_error&) {
      return ResultToJSON(false, "could not find modulator with given dsid");
    }
    return "";
  } // handleCircuitCall


  string WebServer::handleSetCall(const std::string& _method, HashMapConstStringString& _parameter, struct mg_connection* _connection, bool& _handled, Session* _session) {
    _handled = true;
    if(endsWith(_method, "/fromApartment")) {
      _handled = true;
      return JSONOk("{'self': '.'}");
    } else {
      std::string self = trim(_parameter["self"]);
      if(self.empty()) {
        return ResultToJSON(false, "missing parameter 'self'");
      }

      if(endsWith(_method, "/byZone")) {
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        if(!_parameter["zoneID"].empty()) {
          additionalPart += "zone(" + _parameter["zoneID"] + ")";
        } else if(!_parameter["zoneName"].empty()) {
          additionalPart += _parameter["zoneName"];
        } else {
          return ResultToJSON(false, "missing either zoneID or zoneName");
        }

        std::stringstream sstream;
        sstream << "{" << ToJSONValue("self") << ":" << ToJSONValue(self + additionalPart) << "}";
        return JSONOk(sstream.str());
      } else if(endsWith(_method, "/byGroup")) {
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        if(!_parameter["groupID"].empty()) {
          additionalPart += "group(" + _parameter["groupID"] + ")";
        } else if(!_parameter["groupName"].empty()) {
          additionalPart += _parameter["groupName"];
        } else {
          return ResultToJSON(false, "missing either groupID or groupName");
        }

        std::stringstream sstream;
        sstream << "{" << ToJSONValue("self") << ":" << ToJSONValue(self + additionalPart) << "}";
        return JSONOk(sstream.str());
      } else if(endsWith(_method, "/byDSID")) {
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        if(!_parameter["dsid"].empty()) {
          additionalPart += "dsid(" + _parameter["dsid"] + ")";
        } else {
          return ResultToJSON(false, "missing parameter dsid");
        }

        std::stringstream sstream;
        sstream << "{" << ToJSONValue("self") << ":" << ToJSONValue(self + additionalPart) << "}";
        return JSONOk(sstream.str());
      } else if(endsWith(_method, "/getDevices")) {
        SetBuilder builder(getDSS().getApartment());
        Set set = builder.buildSet(self, NULL);
        return JSONOk("{" + ToJSONValue(set, "devices") + "}");
      } else if(endsWith(_method, "/add")) {
        std::string other = _parameter["other"];
        if(other.empty()) {
          return ResultToJSON(false, "missing parameter other");
        }
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        additionalPart += "add(" + other + ")";

        std::stringstream sstream;
        sstream << "{" << ToJSONValue("self") << ":" << ToJSONValue(self + additionalPart) << "}";
        return JSONOk(sstream.str());
      } else if(endsWith(_method, "/subtract")) {
        std::string other = _parameter["other"];
        if(other.empty()) {
          return ResultToJSON(false, "missing parameter other");
        }
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        additionalPart += "subtract(" + other + ")";

        std::stringstream sstream;
        sstream << "{" << ToJSONValue("self") << ":" << ToJSONValue(self + additionalPart) << "}";
        return JSONOk(sstream.str());
      } else if(isDeviceInterfaceCall(_method)) {
        SetBuilder builder(getDSS().getApartment());
        Set set = builder.buildSet(self, NULL);
        return callDeviceInterface(_method, _parameter, _connection, &set, _session);
      } else {
        _handled = false;
      }

    }
    return "";
  } // handleSetCall

  string WebServer::handlePropertyCall(const std::string& _method, HashMapConstStringString& _parameter, struct mg_connection* _connection, bool& _handled, Session* _session) {
    _handled = true;
    string propName = _parameter["path"];
    if(propName.empty()) {
      return ResultToJSON(false, "Need parameter \'path\' for property operations");
    }
    PropertyNodePtr node = getDSS().getPropertySystem().getProperty(propName);

    if(endsWith(_method, "/getString")) {
      if(node == NULL) {
        return ResultToJSON(false, "Could not find node named '" + propName + "'");
      }
      try {
        std::stringstream sstream;
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
        std::stringstream sstream;
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
        std::stringstream sstream;
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
      std::stringstream sstream;
      sstream << "[";
      bool first = true;
      for(int iChild = 0; iChild < node->getChildCount(); iChild++) {
        if(!first) {
          sstream << ",";
        }
        first = false;
        PropertyNodePtr cnode = node->getChild(iChild);
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
      std::stringstream sstream;
      sstream << ":" << ToJSONValue("type") << ":" << ToJSONValue(getValueTypeAsString(node->getValueType())) << "}";
      return JSONOk(sstream.str());
    } else {
      _handled = false;
    }
    return "";
  } // handlePropertyCall

  string WebServer::handleEventCall(const std::string& _method, HashMapConstStringString& _parameter, struct mg_connection* _connection, bool& _handled, Session* _session) {
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

  string WebServer::handleSystemCall(const std::string& _method, HashMapConstStringString& _parameter, struct mg_connection* _connection, bool& _handled, Session* _session) {
    _handled = true;
    string result;
    if(endsWith(_method, "/version")) {
      return ResultToJSON(true, DSS::getInstance()->versionString());
    } else {
      _handled = false;
    }
    return result;
  } // handleEventCall

  string WebServer::handleStructureCall(const std::string& _method,
                                        HashMapConstStringString& _parameter,
                                        struct mg_connection* _connection,
                                        bool& _handled,
                                        Session* _session) {
    _handled = true;
    StructureManipulator manipulator(getDSS().getDS485Interface(), getDSS().getApartment());
    if(endsWith(_method, "structure/zoneAddDevice")) {
      string devidStr = _parameter["devid"];
      if(!devidStr.empty()) {
        dsid_t devid = dsid::fromString(devidStr);

        Device& dev = DSS::getInstance()->getApartment().getDeviceByDSID(devid);
        if(!dev.isPresent()) {
          return ResultToJSON(false, "cannot add nonexisting device to a zone");
        }

        string zoneIDStr = _parameter["zone"];
        if(!zoneIDStr.empty()) {
          try {
            int zoneID = strToInt(zoneIDStr);
            DeviceReference devRef(dev, DSS::getInstance()->getApartment());
            try {
              Zone& zone = getDSS().getApartment().getZone(zoneID);
              manipulator.addDeviceToZone(dev, zone);
            } catch(ItemNotFoundException&) {
              return ResultToJSON(false, "Could not find zone");
            }
          } catch(std::runtime_error& err) {
            return ResultToJSON(false, err.what());
          }
        }
        return ResultToJSON(true);
      } else {
        return ResultToJSON(false, "Need parameter devid");
      }
    } else if(endsWith(_method, "structure/addZone")) {
      bool ok = false;
      int zoneID = -1;

      string zoneIDStr = _parameter["zoneID"];
      if(!zoneIDStr.empty()) {
        zoneID = strToIntDef(zoneIDStr, -1);
      }
      if(zoneID != -1) {
        getDSS().getApartment().allocateZone(zoneID);
        ok = true;
      } else {
        ResultToJSON(false, "could not find zone");
      }
      return ResultToJSON(true, "");
    } else {
      _handled = false;
      return "";
    }
  } // handleStructureCall

  string WebServer::handleSimCall(const std::string& _method, HashMapConstStringString& _parameter, struct mg_connection* _connection, bool& _handled, Session* _session) {
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
        } catch(std::runtime_error&) {
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

  string WebServer::handleDebugCall(const std::string& _method, HashMapConstStringString& _parameter, struct mg_connection* _connection, bool& _handled, Session* _session) {
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
        std::cout << "b" << std::dec << iByte << ": " << std::hex << (int)byte << "\n";
        frame->getPayload().add<uint8_t>(byte);
      }
      std::cout << std::dec << "done" << std::endl;
      DS485Interface* intf = &DSS::getInstance()->getDS485Interface();
      DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(intf);
      if(proxy != NULL) {
        proxy->sendFrame(*frame);
      } else {
        delete frame;
      }
      return ResultToJSON(true);
    } else if(endsWith(_method, "debug/dSLinkSend")) {
      string deviceDSIDString = _parameter["dsid"];
      Device* pDevice = NULL;
      if(!deviceDSIDString.empty()) {
        dsid_t deviceDSID = dsid_t::fromString(deviceDSIDString);
        if(!(deviceDSID == NullDSID)) {
          try {
            Device& device = getDSS().getApartment().getDeviceByDSID(deviceDSID);
            pDevice = &device;
          } catch(std::runtime_error& e) {
            return ResultToJSON(false ,"Could not find device with dsid '" + deviceDSIDString + "'");
          }
        } else {
          return ResultToJSON(false, "Could not parse dsid '" + deviceDSIDString + "'");
        }
      } else {
        return ResultToJSON(false, "Missing parameter 'dsid'");
      }

      int iValue = strToIntDef(_parameter["value"], -1);
      if(iValue == -1) {
        return ResultToJSON(false, "Missing parameter 'value'");
      }
      if(iValue < 0 || iValue > 0x00ff) {
        return ResultToJSON(false, "Parameter 'value' is out of range (0-0xff)");
      }
      bool writeOnly = false;
      bool lastValue = false;
      if(_parameter["writeOnly"] == "true") {
        writeOnly = true;
      }
      if(_parameter["lastValue"] == "true") {
        lastValue = true;
      }
      uint8_t result;
      try {
        result = pDevice->dsLinkSend(iValue, lastValue, writeOnly);
      } catch(std::runtime_error& e) {
        return ResultToJSON(false, std::string("Error: ") + e.what());
      }
      if(writeOnly) {
        return ResultToJSON(true);
      } else {
        std::stringstream sstream;
        sstream << "{" << ToJSONValue("value") << ":" << ToJSONValue(result) << "}";
        return JSONOk(sstream.str());
      }
    } else {
      _handled = false;
      return "";
    }
  } // handleDebugCall

  string WebServer::handleMeteringCall(const std::string& _method, HashMapConstStringString& _parameter, struct mg_connection* _connection, bool& _handled, Session* _session) {
    if(endsWith(_method, "/getResolutions")) {
      _handled = true;
      std::vector<boost::shared_ptr<MeteringConfigChain> > meteringConfig = getDSS().getMetering().getConfig();
      std::stringstream sstream;
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
      std::stringstream sstream;
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
          } catch(std::runtime_error& e) {
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
            std::stringstream sstream;
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

  void WebServer::httpPluginCallback(struct mg_connection* _connection,
                                     const struct mg_request_info* _info,
                                     void* _userData) {
    if(_userData != NULL) {
      WebServerPlugin* plugin = static_cast<WebServerPlugin*>(_userData);
      WebServer& self = DSS::getInstance()->getWebServer();

      string uri = _info->uri;
      self.log("Plugin: Processing call to " + uri);

      self.pluginCalled(_connection, _info, *plugin, uri);
    }
  } // httpPluginCallback

  void WebServer::pluginCalled(struct mg_connection* _connection,
                               const struct mg_request_info* _info,
                               WebServerPlugin& plugin,
                               const std::string& _uri) {
    HashMapConstStringString paramMap = parseParameter(_info->query_string);

    string result;
    if(plugin.handleRequest(_uri, paramMap, getDSS(), result)) {
      emitHTTPHeader(200, _connection, "text/plain");
      mg_write(_connection, result.c_str(), result.length());
    } else {
      emitHTTPHeader(500, _connection, "text/plain");
      mg_printf(_connection, "error");
    }
  } // pluginCalled

  void WebServer::jsonHandler(struct mg_connection* _connection,
                              const struct mg_request_info* _info,
                              void* _userData) {
    const string urlid = "/json/";
    string uri = _info->uri;

    HashMapConstStringString paramMap = parseParameter(_info->query_string);

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
      result = self.handleApartmentCall(method, paramMap, _connection, handled, session);
    } else if(beginsWith(method, "zone/")) {
      result = self.handleZoneCall(method, paramMap, _connection, handled, session);
    } else if(beginsWith(method, "device/")) {
      result = self.handleDeviceCall(method, paramMap, _connection, handled, session);
    } else if(beginsWith(method, "circuit/")) {
      result = self.handleCircuitCall(method, paramMap, _connection, handled, session);
    } else if(beginsWith(method, "set/")) {
      result = self.handleSetCall(method, paramMap, _connection, handled, session);
    } else if(beginsWith(method, "property/")) {
      result = self.handlePropertyCall(method, paramMap, _connection, handled, session);
    } else if(beginsWith(method, "event/")) {
      result = self.handleEventCall(method, paramMap, _connection, handled, session);
    } else if(beginsWith(method, "system/")) {
      result = self.handleSystemCall(method, paramMap, _connection, handled, session);
    } else if(beginsWith(method, "structure/")) {
      result = self.handleStructureCall(method, paramMap, _connection, handled, session);
    } else if(beginsWith(method, "sim/")) {
      result = self.handleSimCall(method, paramMap, _connection, handled, session);
    } else if(beginsWith(method, "debug/")) {
      result = self.handleDebugCall(method, paramMap, _connection, handled, session);
    } else if(beginsWith(method, "metering/")) {
      result = self.handleMeteringCall(method, paramMap, _connection, handled, session);
    }

    if(!handled) {
      emitHTTPHeader(404, _connection, "application/json");
      std::ostringstream sstream;
      sstream << "{" << ToJSONValue("ok") << ":" << ToJSONValue(false) << ",";
      sstream << ToJSONValue("message") << ":" << ToJSONValue("Call to unknown function");
      sstream << "}";
      result = sstream.str();
      self.log("Unknown function '" + method + "'", lsError);
    } else {
      emitHTTPHeader(200, _connection, "application/json");
    }
    mg_write(_connection, result.c_str(), result.length());
  } // jsonHandler

  void WebServer::httpBrowseProperties(struct mg_connection* _connection,
                                       const struct mg_request_info* _info,
                                       void* _userData) {
    emitHTTPHeader(200, _connection);

    const string urlid = "/browse";
    string uri = _info->uri;
    HashMapConstStringString paramMap = parseParameter(_info->query_string);

    string path = uri.substr(uri.find(urlid) + urlid.size());
    if(path.empty()) {
      path = "/";
    }
    mg_printf(_connection, "<html><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><body>");

    std::stringstream stream;
    stream << "<h1>" << path << "</h1>";
    stream << "<ul>";
    PropertyNodePtr node = DSS::getInstance()->getPropertySystem().getProperty(path);
    if(node != NULL) {
      for(int iNode = 0; iNode < node->getChildCount(); iNode++) {
        PropertyNodePtr cnode = node->getChild(iNode);

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
    string tmp = stream.str();
    mg_write(_connection, tmp.c_str(), tmp.length());
  } // httpBrowseProperties

}
