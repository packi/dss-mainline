/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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

  boost::shared_ptr<RestfulAPI> WebServerAPI::createRestfulAPI() {
    boost::shared_ptr<RestfulAPI> api(new RestfulAPI());
    RestfulClass& clsApartment = api->addClass("apartment")
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
    clsApartment.addMethod("setValue")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("value", "integer", true)
      .withDocumentation("Sets the output value of all devices of the apartment to value.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("callScene")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("sceneNumber", "integer", true)
      .withParameter("force", "boolean", false)
      .withDocumentation("Calls the scene sceneNumber on all devices of the apartment.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("saveScene")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("sceneNumber", "integer", true)
      .withDocumentation("Saves the current output value to sceneNumber.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("undoScene")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("sceneNumber", "integer", true)
      .withDocumentation("Undos setting the value of sceneNumber.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("getConsumption")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Returns the consumption of all devices in the apartment in mW.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsApartment.addMethod("getStructure")
      .withParameter("sceneNumber", "integer", true)
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

    clsApartment.addMethod("removeMeter")
      .withParameter("dsid", "dsid", true)
      .withDocumentation("Removes non present dSMs from the dSS.");

    clsApartment.addMethod("removeInactiveMeters")
      .withDocumentation("Removes all non present dSMs from the dSS.");

    RestfulClass& clsZone = api->addClass("zone")
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
    clsZone.addMethod("setValue")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("value", "integer", true)
      .withDocumentation("Sets the output value of all devices in the zone.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("callScene")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("sceneNumber", "integer", true)
      .withParameter("force", "boolean", false)
      .withDocumentation("Sets the scene sceneNumber on all devices in the zone.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("saveScene")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("sceneNumber", "integer", true)
      .withDocumentation("Saves the current output value to sceneNumber of all devices in the zone.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("undoScene")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withParameter("sceneNumber", "integer", true)
      .withDocumentation("Undos the setting of a scene value.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("getConsumption")
      .withParameter("groupID", "integer", false)
      .withParameter("groupName", "string", false)
      .withDocumentation("Returns the consumption of all devices in the zone in mW.", "If groupID or groupName are specified, only devices contained in this group will be addressed");
    clsZone.addMethod("sceneSetName")
      .withParameter("sceneNumber", "integer", true)
      .withParameter("newName", "string", true)
      .withDocumentation("Sets the name of the scene on the given group");
    clsZone.addMethod("sceneGetName")
      .withParameter("sceneNumber", "integer", true)
      .withDocumentation("Returns the name of the scene on the given group");
    clsZone.addMethod("getReachableScenes")
      .withDocumentation("Returns the list of used scenes in a zone depending on the present components");
    clsZone.addMethod("pushSensorValues")
      .withParameter("sourceDSID", "string", true)
      .withParameter("sensorType", "integer", true)
      .withParameter("sensorValue", "integer", true)
      .withDocumentation("Send a sensor value to the components of a zone");

    RestfulClass& clsDevice = api->addClass("device")
        .withInstanceParameter("dsid", "dsid", false)
        .withInstanceParameter("name", "string", false)
        .requireOneOf("dsid", "name");
    clsDevice.addMethod("getName")
       .withDocumentation("Returns the name of the device");
    clsDevice.addMethod("setName")
      .withParameter("newName", "string", true)
      .withDocumentation("Sets the name of the device to newName");
    clsDevice.addMethod("getSpec")
      .withDocumentation("Retrieves device information such as function, product and revision ids");
    clsDevice.addMethod("getGroups")
      .withDocumentation("Returns an array of groups the device is in");
    clsDevice.addMethod("getState")
      .withDocumentation("Returns true if the device is on", "Meaning that the last called scene was not: SceneOff, SceneMin, SceneDeepOff, or SceneStandBy");
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
    clsDevice.addMethod("setConfig")
      .withParameter("value", "integer", true)
      .withParameter("class", "integer", true)
      .withParameter("index", "integer", true)
      .withDocumentation("Sets the value of config class at offset index to value");
    clsDevice.addMethod("getConfig")
      .withParameter("value", "integer", true)
      .withParameter("class", "integer", true)
      .withParameter("index", "integer", true)
      .withDocumentation("Gets the value of config class at offset index");
    clsDevice.addMethod("getConfigWord")
      .withParameter("value", "integer", true)
      .withParameter("class", "integer", true)
      .withParameter("index", "integer", true)
      .withDocumentation("Gets the value of config class at offset index");
    clsDevice.addMethod("setJokerGroup")
      .withParameter("groupID", "integer", true)
      .withDocumentation("Sets the color group of a joker device");
    clsDevice.addMethod("setButtonID")
      .withParameter("buttonID", "integer", true)
      .withDocumentation("Sets the button id of a device");
    clsDevice.addMethod("setButtonInputMode")
      .withParameter("modeID", "integer", true)
      .withDocumentation("Sets the input mode of a device button");
    clsDevice.addMethod("setOutputMode")
      .withParameter("modeID", "integer", true)
      .withDocumentation("Sets the output operation mode");
    clsDevice.addMethod("setProgMode")
      .withParameter("mode", "string", true)
      .withDocumentation("Enables or disables the programming mode of a device.", "The valid mode values are: enabled or disabled.");
    clsDevice.addMethod("getTransmissionQuality")
      .withDocumentation("Request upstream and downstream quality information");
    clsDevice.addMethod("getOutputValue")
      .withParameter("offset", "integer", true)
      .withDocumentation("Gets the device output value from parameter at the given offset.", "The available parameters and offsets depend on the features of the hardware components.");
    clsDevice.addMethod("setOutputValue")
      .withParameter("offset", "integer", true)
      .withParameter("value", "integer", true)
      .withDocumentation("Sets the device output value at the given offset.", "The available parameters and offsets depend on the features of the hardware components.");
    clsDevice.addMethod("getSceneMode")
      .withParameter("sceneID", "integer", true)
      .withDocumentation("Gets the device configuration for a specific scene command.");
    clsDevice.addMethod("getSceneMode")
      .withParameter("sceneID", "integer", true)
      .withDocumentation("Gets the device configuration for a specific scene command.");
    clsDevice.addMethod("setSceneMode")
      .withParameter("sceneID", "integer", true)
      .withParameter("dontCare", "integer", false)
      .withParameter("specialMode", "integer", false)
      .withParameter("flashMode", "integer", false)
      .withParameter("ledconIndex", "integer", false)
      .withParameter("dimtimeIndex", "integer", false)
      .withDocumentation("Sets the configuration flags for a specific scene command.");
    clsDevice.addMethod("getTransitionTime")
      .withParameter("dimetimeIndex", "integer", true)
      .withDocumentation("Gets the timing configuration preset.");
    clsDevice.addMethod("setTransitionTime")
      .withParameter("dimetimeIndex", "integer", true)
      .withParameter("up", "integer", true)
      .withParameter("down", "integer", true)
      .withDocumentation("Sets the timing configuration preset.");
    clsDevice.addMethod("getLedMode")
      .withParameter("ledconIndex", "integer", true)
      .withDocumentation("Gets the led configuration flags preset.");
    clsDevice.addMethod("setLedMode")
      .withParameter("ledconIndex", "integer", true)
      .withParameter("colorSelect", "integer", false)
      .withParameter("modeSelect", "integer", false)
      .withParameter("dimMode", "integer", false)
      .withParameter("rgbMode", "integer", false)
      .withParameter("groupColorMode", "integer", false)
      .withDocumentation("Sets the led configuration flags preset.");
    clsDevice.addMethod("getSensorValue")
      .withParameter("sensorIndex", "integer", true)
      .withDocumentation("Request the sensor value of a given index.");
    clsDevice.addMethod("getSensorType")
      .withParameter("sensorIndex", "integer", true)
      .withDocumentation("Request the sensor type code of a given index.");
    clsDevice.addMethod("callScene")
      .withParameter("sceneNumber", "integer", true)
      .withParameter("force", "boolean", false)
      .withDocumentation("Calls scene sceneNumber on the device.");
    clsDevice.addMethod("saveScene")
      .withParameter("sceneNumber", "integer", true)
      .withDocumentation("Saves the current outputvalue to sceneNumber.");
    clsDevice.addMethod("undoScene")
      .withParameter("sceneNumber", "integer", true)
      .withDocumentation("Undos saving the scene value for sceneNumber");
    clsDevice.addMethod("getConsumption")
      .withDocumentation("Returns the consumption of the device in mW.", "Note that this works only for simulated devices at the moment.");
    clsDevice.addMethod("addTag")
      .withParameter("tag", "string", true)
      .withDocumentation("Adds the tag 'tag'");
    clsDevice.addMethod("removeTag")
      .withParameter("tag", "string", true)
      .withDocumentation("Removes the tag 'tag'");
    clsDevice.addMethod("hasTag")
      .withParameter("tag", "string", true)
      .withDocumentation("Returns hasTag: true if tagged 'tag'");
    clsDevice.addMethod("getTags")
      .withDocumentation("Returns an array of all tags of the device");
    clsDevice.addMethod("lock")
      .withDocumentation("Tells the dSM to never forget this device even if it's not found on the bus.");
    clsDevice.addMethod("unlock")
      .withDocumentation("Tells the dSM to that it's okay forget this device.");

    RestfulClass& clsCircuit = api->addClass("circuit")
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

    RestfulClass& clsProp = api->addClass("property")
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
    clsProp.addMethod("setFlag")
        .withParameter("flag", "string", true)
        .withParameter("value", "boolean", true)
        .withDocumentation("Sets flag to a given value, supported flags are READABLE, WRITEABLE and ARCHIVE");
    clsProp.addMethod("getFlags")
        .withDocumentation("Returns an array of flags and their values");
    clsProp.addMethod("query")
        .withParameter("query", "string", true)
        .withDocumentation("Returns a part of the tree specified by query. All queries start from the root. "
                           "The properties to be included have to be put in parentheses. A query to get all "
                           "device from zone4 would look like this: '/apartment/zones/zone4/*(ZoneID,name)'");
    clsProp.addMethod("remove")
        .withDocumentation("Removes a node from the property tree");

    RestfulClass& clsEvent = api->addClass("event");
    clsEvent.addMethod("raise")
       .withParameter("name", "string", true)
       .withParameter("context", "string", false)
       .withParameter("location", "string", false)
       .withDocumentation("Raises an event", "The context describes the source of the event. The location, if provided, defines where any action that is taken "
           "by any subscription should happen.");
    clsEvent.addMethod("subscribe")
        .withParameter("name", "string", true)
        .withParameter("subscriptionID", "integer", true)
        .withDocumentation("Subscribes to an event given by the name. The subscriptionID is a unique id that is defined by the subscriber. It is possible to subscribe to several events, using the same subscription id, this allows to retrieve a grouped output of the events (i.e. get output of all subscribed by the given id)");
    clsEvent.addMethod("unsubscribe")
        .withParameter("name", "string", true)
        .withParameter("subscriptionID", "integer", true)
        .withDocumentation("Unsubscribes from an event given by the name. The subscriptionID is a unique id that was used in the subscribe call.");
    clsEvent.addMethod("get")
        .withParameter("subscriptionID", "integer", true)
        .withParameter("timeout", "integer", false)
        .withDocumentation("Get event information and output. The subscriptionID is a unique id that was used in the subscribe call. All events, subscribed with the given id will be handled by this call. A timout, in case no events are taken place, can be specified (in MS). By default the timeout is disabled: 0 (zero), if no events occur the call will block.");

    RestfulClass& clsSystem = api->addClass("system");
    clsSystem.addMethod("version")
      .withDocumentation("Returns the dss version",
                         "This method returns the version std::string of the dss");
    clsSystem.addMethod("time")
      .withDocumentation("Returns the dSS time in UTC seconds since epoch");
    clsSystem.addMethod("login")
      .withParameter("user", "string", true)
      .withParameter("password", "string", true)
      .withDocumentation("Creates a new session using the credentials provided");
    clsSystem.addMethod("logout")
      .withDocumentation("Destroys the session and signs out the user");
    clsSystem.addMethod("loggedInUser")
      .withDocumentation("Returns the name of the currently logged in user");
    clsSystem.addMethod("setPassword")
      .withParameter("password", "string", true)
      .withDocumentation("Changes the currently logged in users password");
    clsSystem.addMethod("requestApplicationToken")
      .withParameter("applicationName", "string", true)
      .withDocumentation("Returns a token for paswordless login. The token will need to be approved by a user first. The caller must not be logged in.");
    clsSystem.addMethod("enableToken")
      .withParameter("applicationToken", "string", true)
      .withDocumentation("Enables a application token.");
    clsSystem.addMethod("revokeToken")
      .withParameter("applicationToken", "string", true)
      .withDocumentation("Revokes an application token");

    RestfulClass& clsSet = api->addClass("set")
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
    clsSet.addMethod("setValue")
      .withParameter("value", "integer", true)
      .withDocumentation("Sets the output value of all devices of the set to value.");
    clsSet.addMethod("callScene")
      .withParameter("sceneNumber", "integer", true)
      .withParameter("force", "boolean", false)
      .withDocumentation("Calls the scene sceneNumber on all devices of the set.");
    clsSet.addMethod("saveScene")
      .withParameter("sceneNumber", "integer", true)
      .withDocumentation("Saves the current output value to sceneNumber.");
    clsSet.addMethod("undoScene")
      .withParameter("sceneNumber", "integer", true)
      .withDocumentation("Undoes setting the value of sceneNumber.");
    clsSet.addMethod("getConsumption")
      .withDocumentation("Returns the consumption of all devices in the set in mW.");

    RestfulClass& clsStructure = api->addClass("structure");
    clsStructure.addMethod("zoneAddDevice")
       .withParameter("deviceID", "integer", true)
       .withParameter("zone", "integer", true)
       .withDocumentation("Adds a device to zone");

    clsStructure.addMethod("addZone")
        .withParameter("zoneID", "integer", true)
        .withDocumentation("Adds a zone with the given id");
    clsStructure.addMethod("removeZone")
        .withParameter("zoneID", "integer", true)
        .withDocumentation("Removes a zone with the given id");
    clsStructure.addMethod("removeDevice")
        .withParameter("deviceID", "integer", true)
        .withDocumentation("Removes a device.", "Only devices that are no longer present (isPresent flag is not set) can be removed.");
    clsStructure.addMethod("persistSet")
       .withParameter("set", "string", true)
       .withParameter("groupID", "integer", false)
       .withDocumentation("Creates a group containing all devices contained in the set");
    clsStructure.addMethod("unpersistSet")
       .withParameter("set", "string", true)
       .withDocumentation("Removes the group associated with the set");
    clsStructure.addMethod("addGroup")
        .withParameter("zoneID", "integer", true)
        .withParameter("groupID", "integer", true)
        .withDocumentation("Adds a group in the zone with the given id");
    clsStructure.addMethod("groupAddDevice")
        .withParameter("deviceID", "integer", true)
        .withParameter("groupID", "integer", true)
        .withDocumentation("Adds a device to the group");
    clsStructure.addMethod("groupRemoveDevice")
        .withParameter("deviceID", "integer", true)
        .withParameter("groupID", "integer", true)
        .withDocumentation("Removes the device from the group");

    RestfulClass& clsMetering = api->addClass("metering");
    clsMetering.addMethod("getResolutions")
      .withDocumentation("Returns all resolutions stored on this dSS");
    clsMetering.addMethod("getSeries");
    clsMetering.addMethod("getValues");
    clsMetering.addMethod("getLatest")
      .withParameter("type", "string", true)
      .withParameter("from", "string", true)
      .withDocumentation("Returns cached energy meter value (in Wh) or cached power consumption value (in mW). The type parameter defines what should be returned, valid types are 'energy' and 'consumption'. The from parameter follows the set-syntax, currently it supports: .meters(dsid1,dsid2,...) and .meters(all)");

    return api;
  } // createRestfulAPI

} // namespace dss
