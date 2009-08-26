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
/** Terminates a session. All resources allocate by the session (e.g. sets) will be
 * released. */
int dss__SignOff(int _token, int& result);
/** Frees the set (_setID) on the server. */
int dss__FreeSet(int _token, int _setID, bool& result);

/** Creates a set containing all devices which are contained in a group named _groupName.
 * @see dss__CreateEmptySet */
int dss__ApartmentCreateSetFromGroup(int _token, char* _groupName, int& setID);
/** Creates a set containing all devices in the given array
 * @see dss__CreateEmptySet*/
int dss__ApartmentCreateSetFromDeviceIDs(int _token, std::vector<std::string> _ids, int& setID);
/** Creates a set containing all devices in the given array */
int dss__ApartmentCreateSetFromDeviceNames(int _token, std::vector<std::string> _names, int& setID);
/** Creates an empty set.
 * The set is allocated on the server and must be freed either by a SignOff or a call to FreeSet.*/
int dss__ApartmentCreateEmptySet(int _token, int& setID);
/** Creates a set containing all devices */
int dss__ApartmentGetDevices(int _token, int& setID);
/** Returns the device ID for the given _deviceName */
int dss__ApartmentGetDeviceIDByName(int _token, char* _deviceName, std::string& deviceID);

/** Adds the given device to the set */
int dss__SetAddDeviceByName(int _token, int _setID, char* _name, bool& result);
/** Adds the given device to the set */
int dss__SetAddDeviceByID(int _token, int _setID, char* _deviceID, bool& result);
/** Removes the device from the set */
int dss__SetRemoveDevice(int _token, int _setID, char* _deviceID, bool& result);
/** Combines two sets into another set */
int dss__SetCombine(int _token, int _setID1, int _setID2, int& setID);
/** Removes all devices contained in _SetIDToRemove from _setID and copies those into setID */
int dss__SetRemove(int _token, int _setID, int _setIDToRemove, int& setID);
/** Removes all devices which don't belong to the specified group */
int dss__SetByGroup(int _token, int _setID, int _groupID, int& setID);
/** Returns an array containing all device ids contained in the given set */
int dss__SetGetContainedDevices(int _token, int _setID, std::vector<std::string>& deviceIDs);

/** Looks up the group id for the given group name */
int dss__ApartmentGetGroupByName(int _token, char* _groupName, int& groupID);
/** Looks up the zone id for the given zone */
int dss__ApartmentGetZoneByName(int _token, char* _zoneName, int& zoneID);
/** Returns an array containing all zone ids */
int dss__ApartmentGetZoneIDs(int _token, std::vector<int>& zoneIDs);

//==================================================== Manipulation

//--------------------------- Set

/** Sends a turn on command to all devices contained in the set */
int dss__SetTurnOn(int _token, int _setID, bool& result);
/** Sends a turn off command to all devices contained in the set */
int dss__SetTurnOff(int _token, int _setID, bool& result);
/** Increases the param described by _paramID for each device contained in the set. If _paramID
 *  == -1 the default parameter will be increased. */
int dss__SetIncreaseValue(int _token, int _setID, int _paramID, bool& result);
/** Decreases the param described by _paramID for each device contained in the set. If _paramID
 *  == -1 the default parameter will be decreased. */
int dss__SetDecreaseValue(int _token, int _setID, int _paramID, bool& result);

/** Enables all previously disabled devices in the set. */
int dss__SetEnable(int _token, int _setID, bool& result);
/** Disables all devices in the set. */
int dss__SetDisable(int _token, int _setID, bool& result);
/** Starts dimming the given parameter on all devices contained in the set. If _directionUp is
 * true, the dimming will increase the parameter specified by _paramID. If _paramID == -1 the
 * default parameter will be dimmed */
int dss__SetStartDim(int _token, int _setID, bool _directionUp, int _paramID, bool& result);
/** Stops dimming the given parameter on all devices contained in the set. */
int dss__SetEndDim(int _token, int _setID, int _paramID, bool& result);
/** Sets the parameter specified by _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__SetSetValue(int _token, int _setID, double _value, int _paramID, bool& result);

/** Calls the scene _sceneNr on all devices contained int the set _setID. */
int dss__SetCallScene(int _token, int _setID, int _sceneNr, bool& result);
/** Saves the scene _sceneNr on all devices contained int the set _setID. */
int dss__SetSaveScene(int _token, int _setID, int _sceneNr, bool& result);

//--------------------------- Apartment

/** Sends a turn on command to all devices contained in the group */
int dss__ApartmentTurnOn(int _token, int _groupID, bool& result);
/** Sends a turn off command to all devices contained in the group */
int dss__ApartmentTurnOff(int _token, int _groupID, bool& result);
/** Increases the param described by _paramID for each device contained in the group. If _paramID
 *  == -1 the default parameter will be increased. */
int dss__ApartmentIncreaseValue(int _token, int _groupID, int _paramID, bool& result);
/** Decreases the param described by _paramID for each device contained in the group. If _paramID
 *  == -1 the default parameter will be decreased. */
int dss__ApartmentDecreaseValue(int _token, int _groupID, int _paramID, bool& result);

/** Enables all previously disabled devices in the group. */
int dss__ApartmentEnable(int _token, int _groupID, bool& result);
/** Disables all devices in the group. */
int dss__ApartmentDisable(int _token, int _groupID, bool& result);
/** Starts dimming the given parameter on all devices contained in the group. If _directionUp is
 * true, the dimming will increase the parameter specified by _paramID. If _paramID == -1 the
 * default parameter will be dimmed */
int dss__ApartmentStartDim(int _token, int _groupID, bool _directionUp, int _paramID, bool& result);
/** Stops dimming the given parameter on all devices contained in the group. */
int dss__ApartmentEndDim(int _token, int _groupID, int _paramID, bool& result);
/** Sets the parameter specified by _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__ApartmentSetValue(int _token, int _groupID, double _value, int _paramID, bool& result);

/** Calls the scene _sceneNr on all devices contained int the group _groupID. */
int dss__ApartmentCallScene(int _token, int _groupID, int _sceneNr, bool& result);
/** Saves the scene _sceneNr on all devices contained int the group _groupID. */
int dss__ApartmentSaveScene(int _token, int _groupID, int _sceneNr, bool& result);


//--------------------------- Zone

/** Sends a turn on command to all devices contained in the zone/group */
int dss__ZoneTurnOn(int _token, int _zoneID, int _groupID, bool& result);
/** Sends a turn off command to all devices contained in the group */
int dss__ZoneTurnOff(int _token, int _zoneID, int _groupID, bool& result);
/** Increases the param described by _paramID for each device contained in the zone/group. If _paramID
 *  == -1 the default parameter will be increased. */
int dss__ZoneIncreaseValue(int _token, int _zoneID, int _groupID, int _paramID, bool& result);
/** Decreases the param described by _paramID for each device contained in the zone/group. If _paramID
 *  == -1 the default parameter will be decreased. */
int dss__ZoneDecreaseValue(int _token, int _zoneID, int _groupID, int _paramID, bool& result);

/** Enables all previously disabled devices in the zone/group. */
int dss__ZoneEnable(int _token, int _zoneID, int _groupID, bool& result);
/** Disables all devices in the zone/group. */
int dss__ZoneDisable(int _token, int _zoneID, int _groupID, bool& result);
/** Starts dimming the given parameter on all devices contained in the group/zone. If _directionUp is
 * true, the dimming will increase the parameter specified by _paramID. If _paramID == -1 the
 * default parameter will be dimmed */
int dss__ZoneStartDim(int _token, int _zoneID, int _groupID, bool _directionUp, int _paramID, bool& result);
/** Stops dimming the given parameter on all devices contained in the zone/group. */
int dss__ZoneEndDim(int _token, int _zoneID, int _groupID, int _paramID, bool& result);
/** Sets the parameter specified by _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__ZoneSetValue(int _token, int _zoneID, int _groupID, double _value, int _paramID, bool& result);

/** Calls the scene _sceneNr on all devices contained int the zone/group _groupID. */
int dss__ZoneCallScene(int _token, int _zoneID, int _groupID, int _sceneNr, bool& result);
/** Saves the scene _sceneNr on all devices contained int the zone/group _groupID. */
int dss__ZoneSaveScene(int _token, int _zoneID, int _groupID, int _sceneNr, bool& result);

//--------------------------- Device

/** Sends a turn on command to the device. */
int dss__DeviceTurnOn(int _token, char* _deviceID, bool& result);
/** Sends a turn off command to the device. */
int dss__DeviceTurnOff(int _token, char* _deviceID, bool& result);
/** Increases the parameter specified by _paramID on the device. If _paramID == -1
 * the default parameter will be increased */
int dss__DeviceIncreaseValue(int _token, char* _deviceID, int _paramID, bool& result);
/** Decreases the parameter specified by _paramID on the device. If _paramID == -1
 * the default parameter will be decreased */
int dss__DeviceDecreaseValue(int _token, char* _deviceID, int _paramID, bool& result);
/** Enables the device. */
int dss__DeviceEnable(int _token, char* _deviceID, bool& result);
/** Disables the device. */
int dss__DeviceDisable(int _token, char* _deviceID, bool& result);
/** Starts dimming the given parameter. If _directionUp is true, the dimming will increase
 * the parameter specified by _paramID. If _paramID == -1 the default parameter will be
 * dimmed. */
int dss__DeviceStartDim(int _token, char* _deviceID, bool _directionUp, int _paramID, bool& result);
/** Stops dimming the given parameter. If _parameterID == -1 dimming the default parameter
 * will be stopped. */
int dss__DeviceEndDim(int _token, char* _deviceID, int _paramID, bool& result);
/** Sets the value of the parameter _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__DeviceSetValue(int _token, char* _deviceID, double _value, int _paramID, bool& result);
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

int dss__ModulatorGetPowerConsumption(int _token, int _modulatorID, xsd__unsignedInt& result);

//==================================================== Organization

//These calls may be restricted to privileged users.

/** Returns an integer array of modulators known to the dss. */
int dss__ApartmentGetModulatorIDs(int _token, std::vector<std::string>& ids);
/** Retuns the name of the given modulator. */
int dss__ModulatorGetName(int _token, char* _modulatorID, std::string& name);
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
/** Simulates a key press on the specified switch and button
  * @param _kind One of ("click", "touch", "touchend")
  */
//int dss__SwitchSimulateKeypress(int _token, char* _deviceID, int _buttonNr, char* _kind, bool& result);


//==================================================== Events

int dss__EventRaise(int _token, char* _eventName, char* _context, char* _parameter, char* _location, bool& result);


// =================================================== Properties

int dss__PropertyGetType(int _token, std::string _propertyName, std::string& result);

int dss__PropertySetInt(int _token, std::string _propertyName, int _value, bool _mayCreate = true, bool& result);
int dss__PropertySetString(int _token, std::string _propertyName, char* _value, bool _mayCreate = true, bool& result);
int dss__PropertySetBool(int _token, std::string _propertyName, bool _value, bool _mayCreate = true, bool& result);

int dss__PropertyGetInt(int _token, std::string _propertyName, int& result);
int dss__PropertyGetString(int _token, std::string _propertyName, std::string& result);
int dss__PropertyGetBool(int _token, std::string _propertyName, bool& result);

int dss__PropertyGetChildren(int _token, std::string _propertyName, std::vector<std::string>& result);


