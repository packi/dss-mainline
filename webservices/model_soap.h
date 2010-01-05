/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

//gsoap dss service name: dss
//gsoap dss service namespace: http://localhost:8080/dss.wsdl
//gsoap dss service port: http://localhost:8081
//gsoap dss schema namespace:  urn:dss:1.0
//gsoap dss service type: MyType

#import "stl.h"

/** \file */

typedef unsigned long xsd__unsignedInt;
typedef unsigned long long xsd__unsignedLong;

/** Authenticates your ip to the system.
 * The token received will be used in any subsequent call. The ip/token pair
 * identifies a session. A Session will time out after n minutes of no activity (default 5).*/
int dss__Authenticate(char* _userName, char* _password, int& token);
/** Terminates a session. All resources allocate by the session will be
 * released. */
int dss__SignOff(int _token, int& result);

/** Creates a set containing all devices which are contained in a group named _groupName.
 * @see dss__CreateEmptySet */
int dss__ApartmentCreateSetFromGroup(int _token, char* _groupName, std::string& result);
/** Creates a set containing all devices in the given array
 * @see dss__CreateEmptySet*/
int dss__ApartmentCreateSetFromDeviceIDs(int _token, std::vector<std::string> _ids, std::string& result);
/** Creates a set containing all devices in the given array */
int dss__ApartmentCreateSetFromDeviceNames(int _token, std::vector<std::string> _names, std::string& result);
/** Creates a set containing all devices */
int dss__ApartmentGetDevices(int _token, std::string& result);

/** Returns the device ID for the given _deviceName */
int dss__ApartmentGetDeviceIDByName(int _token, char* _deviceName, std::string& deviceID);

/** Adds the given device to the set */
int dss__SetAddDeviceByName(int _token, char* _setSpec, char* _name, std::string& result);
/** Adds the given device to the set */
int dss__SetAddDeviceByID(int _token, char* _setSpec, char* _deviceID, std::string& result);
/** Removes the device from the set */
int dss__SetRemoveDevice(int _token, char* _setSpec, char* _deviceID, std::string& result);
/** Combines two sets into another set */
int dss__SetCombine(int _token, char* _setSpec1, char* _setSpec2, std::string& result);
/** Removes all devices contained in _SetIDToRemove from _setID and copies those into setID */
int dss__SetRemove(int _token, char* _setSpec, char* _setSpecToRemove, std::string& result);
/** Removes all devices which don't belong to the specified group */
int dss__SetByGroup(int _token, char* _setSpec, int _groupID, std::string& result);
/** Returns an array containing all device ids contained in the given set */
int dss__SetGetContainedDevices(int _token, char* _setSpec, std::vector<std::string>& deviceIDs);

/** Looks up the group id for the given group name */
int dss__ApartmentGetGroupByName(int _token, char* _groupName, int& groupID);
/** Looks up the zone id for the given zone */
int dss__ApartmentGetZoneByName(int _token, char* _zoneName, int& zoneID);
/** Returns an array containing all zone ids */
int dss__ApartmentGetZoneIDs(int _token, std::vector<int>& zoneIDs);

//==================================================== Manipulation

//--------------------------- Set

/** Sends a turn on command to all devices contained in the set */
int dss__SetTurnOn(int _token, char* _setSpec, bool& result);
/** Sends a turn off command to all devices contained in the set */
int dss__SetTurnOff(int _token, char* _setSpec, bool& result);
/** Increases the main value (e.g. brightness) for each device contained in the set. */
int dss__SetIncreaseValue(int _token, char* _setSpec, bool& result);
/** Decreases the main value (e.g. brightness) for each device contained in the set. */
int dss__SetDecreaseValue(int _token, char* _setSpec, bool& result);

/** Starts dimming the main parameter (e.g. brightness) on all devices contained in the set. If _directionUp is
 * true, the dimming will increase the parameter */
int dss__SetStartDim(int _token, char* _setSpec, bool _directionUp, bool& result);
/** Stops dimming on all devices contained in the set. */
int dss__SetEndDim(int _token, char* _setSpec, bool& result);
/** Sets the parameter specified by _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__SetSetValue(int _token, char* _setSpec, double _value, bool& result);

/** Calls the scene _sceneNr on all devices contained int the set _setID. */
int dss__SetCallScene(int _token, char* _setSpec, int _sceneNr, bool& result);
/** Saves the scene _sceneNr on all devices contained int the set _setID. */
int dss__SetSaveScene(int _token, char* _setSpec, int _sceneNr, bool& result);

//--------------------------- Apartment

/** Sends a turn on command to all devices contained in the group */
int dss__ApartmentTurnOn(int _token, int _groupID, bool& result);
/** Sends a turn off command to all devices contained in the group */
int dss__ApartmentTurnOff(int _token, int _groupID, bool& result);
/** Increases the main value (e.g. brightness) for each device contained in the group. */
int dss__ApartmentIncreaseValue(int _token, int _groupID, bool& result);
/** Decreases the main value (e.g. brightness) for each device contained in the group. */
int dss__ApartmentDecreaseValue(int _token, int _groupID, bool& result);

/** Starts dimming the main parameter on all devices contained in the group. If _directionUp is
 * true, the dimming will increase the parameter. */
int dss__ApartmentStartDim(int _token, int _groupID, bool _directionUp, bool& result);
/** Stops dimming the given parameter on all devices contained in the group. */
int dss__ApartmentEndDim(int _token, int _groupID, bool& result);
/** Sets the parameter specified by _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__ApartmentSetValue(int _token, int _groupID, double _value, bool& result);

/** Calls the scene _sceneNr on all devices contained int the group _groupID. */
int dss__ApartmentCallScene(int _token, int _groupID, int _sceneNr, bool& result);
/** Saves the scene _sceneNr on all devices contained int the group _groupID. */
int dss__ApartmentSaveScene(int _token, int _groupID, int _sceneNr, bool& result);

/** Rescans the bus for new devices/circuits */
int dss__ApartmentRescan(int _token, bool& result);

//--------------------------- Circuit
/** Rescans the circuit for new/lost devices */
int dss__CircuitRescan(int _token, char* _dsid, bool& result);

//--------------------------- Zone

/** Sends a turn on command to all devices contained in the zone/group */
int dss__ZoneTurnOn(int _token, int _zoneID, int _groupID, bool& result);
/** Sends a turn off command to all devices contained in the group */
int dss__ZoneTurnOff(int _token, int _zoneID, int _groupID, bool& result);
/** Increases the main value (e.g. brightness) for each device contained in the zone/group. */
int dss__ZoneIncreaseValue(int _token, int _zoneID, int _groupID, bool& result);
/** Decreases the main value (e.g. brightness) for each device contained in the zone/group. */
int dss__ZoneDecreaseValue(int _token, int _zoneID, int _groupID, bool& result);

/** Starts dimming the main parameter (e.g. brightness) on all devices contained in the group/zone. If _directionUp is
 * true, the dimming will increase the parameter. */
int dss__ZoneStartDim(int _token, int _zoneID, int _groupID, bool _directionUp, bool& result);
/** Stops dimming on all devices contained in the zone/group. */
int dss__ZoneEndDim(int _token, int _zoneID, int _groupID, bool& result);
/** Sets the parameter specified by _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__ZoneSetValue(int _token, int _zoneID, int _groupID, double _value, bool& result);

/** Calls the scene _sceneNr on all devices contained int the zone/group _groupID. */
int dss__ZoneCallScene(int _token, int _zoneID, int _groupID, int _sceneNr, bool& result);
/** Saves the scene _sceneNr on all devices contained int the zone/group _groupID. */
int dss__ZoneSaveScene(int _token, int _zoneID, int _groupID, int _sceneNr, bool& result);

//--------------------------- Device

/** Sends a turn on command to the device. */
int dss__DeviceTurnOn(int _token, char* _deviceID, bool& result);
/** Sends a turn off command to the device. */
int dss__DeviceTurnOff(int _token, char* _deviceID, bool& result);
/** Increases the main value (e.g. brightness) on the device. */
int dss__DeviceIncreaseValue(int _token, char* _deviceID, bool& result);
/** Decreases the main value (e.g. brightness) on the device. */
int dss__DeviceDecreaseValue(int _token, char* _deviceID, bool& result);
/** Enables the device. */
int dss__DeviceEnable(int _token, char* _deviceID, bool& result);
/** Disables the device. */
int dss__DeviceDisable(int _token, char* _deviceID, bool& result);
/** Starts dimming the main parameter (e.g. brightness). If _directionUp is true, the dimming will increase
 * the parameter. */
int dss__DeviceStartDim(int _token, char* _deviceID, bool _directionUp, bool& result);
/** Stops dimming. */
int dss__DeviceEndDim(int _token, char* _deviceID, bool& result);
/** Sets the value of the parameter _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__DeviceSetValue(int _token, char* _deviceID, double _value, bool& result);
/** Returns the value of the parameter _paramID. If _paramID == -1 the value of the default parameter
 * will be returned. */
int dss__DeviceGetValue(int _token, char* _deviceID, int _paramID, double& result);


/** Calls the scene _sceneNr on the device identified by _deviceID. */
int dss__DeviceCallScene(int _token, char* _deviceID, int _sceneNr, bool& result);
/** Saves the scene _sceneNr on the device identified by _devicdID. */
int dss__DeviceSaveScene(int _token, char* _deviceID, int _sceneNr, bool& result);

/** Returns the name of a device */
int dss__DeviceGetName(int _token, char* _deviceID, char** result);

/** Returns the zone id of the specified device */
int dss__DeviceGetZoneID(int _token, char* _deviceID, int& result);


//==================================================== Information

int dss__DSMeterGetPowerConsumption(int _token, int _dsMeterID, xsd__unsignedInt& result);

//==================================================== Organization

//These calls may be restricted to privileged users.

/** Returns an integer array of dsMeters known to the dss. */
int dss__ApartmentGetDSMeterIDs(int _token, std::vector<std::string>& ids);
/** Retuns the name of the given dsMeter. */
int dss__DSMeterGetName(int _token, char* _dsMeterID, std::string& name);
/** Allocates a zone */
int dss__ApartmentAllocateZone(int _token, int& zoneID);
/** Deletes a previously allocated zone. */
int dss__ApartmentDeleteZone(int _token, int _zoneID, int& result);
/** Adds a device to a zone */
int dss__Zone_AddDevice(int _token, int _zoneID, char* _deviceID, int& result);
/** Removes a device from a zone */
int dss__Zone_RemoveDevice(int _token, int _zoneID, char* _deviceID, int& result);
/** Sets the name of a zone to _name */
int dss__Zone_SetName(int _token, int _zoneID, char* _name, int& result);
/** Allocates a user-defined group. */
int dss__ApartmentAllocateUserGroup(int _token, int& groupID);
/** Revmoes a previously allocated group. */
int dss__GroupRemoveUserGroup(int _token, int _groupID, int& result);
/** Adds a device to the given group. */
int dss__GroupAddDevice(int _token, int _groupID, char* _deviceID, int& result);
/** Removes a device from the given group. */
int dss__GroupRemoveDevice(int _token, int _groupID, char* _deviceID, int& result);

/** Returns the function id of the specified device */
int dss__DeviceGetFunctionID(int _token, char* _deviceID, int& result);
/** Returns the group id of the specified switch */
int dss__SwitchGetGroupID(int _token, char* _deviceID, int& result);


//==================================================== Events

class dss__Event {
public:
  std::string name;
  std::vector<std::string> parameter;
};

int dss__EventRaise(int _token, char* _eventName, char* _context, char* _parameter, char* _location, bool& result);
int dss__EventWaitFor(int _token, int _timeout, std::vector<dss__Event>& result);
int dss__EventSubscribeTo(int _token, std::string _name, std::string& result);


// =================================================== Properties

int dss__PropertyGetType(int _token, std::string _propertyName, std::string& result);

int dss__PropertySetInt(int _token, std::string _propertyName, int _value, bool _mayCreate = true, bool& result);
int dss__PropertySetString(int _token, std::string _propertyName, char* _value, bool _mayCreate = true, bool& result);
int dss__PropertySetBool(int _token, std::string _propertyName, bool _value, bool _mayCreate = true, bool& result);

int dss__PropertyGetInt(int _token, std::string _propertyName, int& result);
int dss__PropertyGetString(int _token, std::string _propertyName, std::string& result);
int dss__PropertyGetBool(int _token, std::string _propertyName, bool& result);

int dss__PropertyGetChildren(int _token, std::string _propertyName, std::vector<std::string>& result);


