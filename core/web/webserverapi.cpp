/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2010 futureLAB AG, Winterthur, Switzerland

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

#include "webserverapi.h"

#include "restful.h"

namespace dss {
 
  RestfulAPI WebServerAPI::createRestfulAPI() {
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
        .withDocumentation("Returns the std::string value of the property", "This will fail if the property is not of type 'string'.");
    clsProp.addMethod("getInteger")
        .withDocumentation("Returns the integer value of the property", "This will fail if the property is not of type 'integer'.");
    clsProp.addMethod("getBoolean")
        .withDocumentation("Returns the boolean value of the property", "This will fail if the property is not of type 'boolean'.");
    clsProp.addMethod("setString")
        .withParameter("value", "string", true)
        .withDocumentation("Sets the std::string value of the property", "This will fail if the property is not of type 'string'.");
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
                         "This method returns the version std::string of the dss");

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
    return api;
  } // createRestfulAPI

} // namespace dss
