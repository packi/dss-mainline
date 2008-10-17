//gsoap dss service name: dss
//gsoap dss service namespace: http://localhost:8080/dss.wsdl
//gsoap dss service port: http://localhost:8081
//gsoap dss schema namespace:  urn:dss:1.0
//gsoap dss service type: MyType

/** \file */

typedef unsigned long xsd__unsignedInt;

class IntArray {
public:
  xsd__unsignedInt* __ptr;
  int __size;
};

class StringArray {
public:
  char** __ptr;
  int __size;
};

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
int dss__Apartment_CreateSetFromGroup(int _token, char* _groupName, int& setID);
/** Creates a set containing all devices in the given array
 * @see dss__CreateEmptySet*/
int dss__Apartment_CreateSetFromDeviceIDs(int _token, IntArray _ids, int& setID);
/** Creates a set containing all devices in the given array */
int dss__Apartment_CreateSetFromDeviceNames(int _token, StringArray _names, int& setID);
/** Creates an empty set.
 * The set is allocated on the server and must be freed either by a SignOff or a call to FreeSet.*/
int dss__Apartment_CreateEmptySet(int _token, int& setID);
/** Creates a set containing all devices */
int dss__Apartment_GetDevices(int _token, int& setID);
/** Returns the device ID for the given _deviceName */
int dss__Apartment_GetDeviceIDByName(int _token, char* _deviceName, int& deviceID);

/** Adds the given device to the set */
int dss__Set_AddDeviceByName(int _token, int _setID, char* _name, bool& result);
/** Adds the given device to the set */
int dss__Set_AddDeviceByID(int _token, int _setID, int _deviceID, bool& result);
/** Removes the device from the set */
int dss__Set_RemoveDevice(int _token, int _setID, int _deviceID, bool& result);
/** Combines two sets into another set */
int dss__Set_Combine(int _token, int _setID1, int _setID2, int& setID);
/** Removes all devices contained in _SetIDToRemove from _setID and copies those into setID */
int dss__Set_Remove(int _token, int _setID, int _setIDToRemove, int& setID);
/** Removes all devices which don't belong to the specified group */
int dss__Set_ByGroup(int _token, int _setID, int _groupID, int& setID);
/** Returns an array containing all device ids contained in the given set */
int dss__Set_GetContainedDevices(int _token, int _setID, IntArray& deviceIDs);

/** Looks up the group id for the given group name */
int dss__Apartment_GetGroupByName(int _token, char* _groupName, int& groupID);
/** Looks up the zone id for the given zone */
int dss__Apartment_GetZoneByName(int _token, char* _zoneName, int& zoneID);
/** Returns an array containing all zone ids */
int dss__Apartment_GetZoneIDs(int _token, IntArray& zoneIDs);

//==================================================== Manipulation

//--------------------------- Set

/** Sends a turn on command to all devices contained in the set */
int dss__Set_TurnOn(int _token, int _setID, bool& result);
/** Sends a turn off command to all devices contained in the set */
int dss__Set_TurnOff(int _token, int _setID, bool& result);
/** Increases the param described by _paramID for each device contained in the set. If _paramID
 *  == -1 the default parameter will be increased. */
int dss__Set_IncreaseValue(int _token, int _setID, int _paramID, bool& result);
/** Decreases the param described by _paramID for each device contained in the set. If _paramID
 *  == -1 the default parameter will be decreased. */
int dss__Set_DecreaseValue(int _token, int _setID, int _paramID, bool& result);

/** Enables all previously disabled devices in the set. */
int dss__Set_Enable(int _token, int _setID, bool& result);
/** Disables all devices in the set. */
int dss__Set_Disable(int _token, int _setID, bool& result);
/** Starts dimming the given parameter on all devices contained in the set. If _directionUp is
 * true, the dimming will increase the parameter specified by _paramID. If _paramID == -1 the
 * default parameter will be dimmed */
int dss__Set_StartDim(int _token, int _setID, bool _directionUp, int _paramID, bool& result);
/** Stops dimming the given parameter on all devices contained in the set. */
int dss__Set_EndDim(int _token, int _setID, int _paramID, bool& result);
/** Sets the parameter specified by _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__Set_SetValue(int _token, int _setID, double _value, int _paramID, bool& result);

/** Calls the scene _sceneNr on all devices contained int the set _setID. */
int dss_Set_CallScene(int _token, int _setID, int _sceneNr, bool& result);
/** Saves the scene _sceneNr on all devices contained int the set _setID. */
int dss_Set_SaveScene(int _token, int _setID, int _sceneNr, bool& result);

//--------------------------- Group

/** Sends a turn on command to all devices contained in the group */
int dss__Group_TurnOn(int _token, int _groupID, bool& result);
/** Sends a turn off command to all devices contained in the group */
int dss__Group_TurnOff(int _token, int _groupID, bool& result);
/** Increases the param described by _paramID for each device contained in the group. If _paramID
 *  == -1 the default parameter will be increased. */
int dss__Group_IncreaseValue(int _token, int _groupID, int _paramID, bool& result);
/** Decreases the param described by _paramID for each device contained in the group. If _paramID
 *  == -1 the default parameter will be decreased. */
int dss__Group_DecreaseValue(int _token, int _groupID, int _paramID, bool& result);

/** Enables all previously disabled devices in the group. */
int dss__Group_Enable(int _token, int _groupID, bool& result);
/** Disables all devices in the group. */
int dss__Group_Disable(int _token, int _groupID, bool& result);
/** Starts dimming the given parameter on all devices contained in the group. If _directionUp is
 * true, the dimming will increase the parameter specified by _paramID. If _paramID == -1 the
 * default parameter will be dimmed */
int dss__Group_StartDim(int _token, int _groupID, bool _directionUp, int _paramID, bool& result);
/** Stops dimming the given parameter on all devices contained in the group. */
int dss__Group_EndDim(int _token, int _groupID, int _paramID, bool& result);
/** Sets the parameter specified by _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__Group_SetValue(int _token, int _groupID, double _value, int _paramID, bool& result);

/** Calls the scene _sceneNr on all devices contained int the group _groupID. */
int dss_Group_CallScene(int _token, int _groupID, int _sceneNr, bool& result);
/** Saves the scene _sceneNr on all devices contained int the group _groupID. */
int dss_Group_SaveScene(int _token, int _groupID, int _sceneNr, bool& result);


//--------------------------- Device

/** Sends a turn on command to the device. */
int dss__Device_TurnOn(int _token, xsd__unsignedInt _deviceID, bool& result);
/** Sends a turn off command to the device. */
int dss__Device_TurnOff(int _token, xsd__unsignedInt _deviceID, bool& result);
/** Increases the parameter specified by _paramID on the device. If _paramID == -1
 * the default parameter will be increased */
int dss__Device_IncreaseValue(int _token, xsd__unsignedInt _deviceID, int _paramID, bool& result);
/** Decreases the parameter specified by _paramID on the device. If _paramID == -1
 * the default parameter will be decreased */
int dss__Device_DecreaseValue(int _token, xsd__unsignedInt _deviceID, int _paramID, bool& result);
/** Enables the device. */
int dss__Device_Enable(int _token, xsd__unsignedInt _deviceID, bool& result);
/** Disables the device. */
int dss__Device_Disable(int _token, xsd__unsignedInt _deviceID, bool& result);
/** Starts dimming the given parameter. If _directionUp is true, the dimming will increase
 * the parameter specified by _paramID. If _paramID == -1 the default parameter will be
 * dimmed. */
int dss__Device_StartDim(int _token, xsd__unsignedInt _deviceID, bool _directionUp, int _paramID, bool& result);
/** Stops dimming the given parameter. If _parameterID == -1 dimming the default parameter
 * will be stopped. */
int dss__Device_EndDim(int _token, xsd__unsignedInt _deviceID, int _paramID, bool& result);
/** Sets the value of the parameter _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__Device_SetValue(int _token, xsd__unsignedInt _deviceID, double _value, int _paramID, bool& result);
/** Returns the value of the parameter _paramID. If _paramID == -1 the value of the default parameter
 * will be returned. */
int dss__Device_GetValue(int _token, xsd__unsignedInt _deviceID, int _paramID, double& result);


/** Calls the scene _sceneNr on the device identified by _deviceID. */
int dss_Device_CallScene(int _token, xsd__unsignedInt _deviceID, int _sceneNr, bool& result);
/** Saves the scene _sceneNr on the device identified by _devicdID. */
int dss_Device_SaveScene(int _token, xsd__unsignedInt _deviceID, int _sceneNr, bool& result);

/** Returns the name of a device */
int dss__Device_GetName(int _token, xsd__unsignedInt _deviceID, char** result);

/** Returns the zone id of the specified device */
int dss__Device_GetZoneID(int _token, xsd__unsignedInt _deviceID, int& result);

//==================================================== Information

/** Returns the DSID of the given device */
int dss__Device_GetDSID(int _token, xsd__unsignedInt _deviceID, xsd__unsignedInt& result);

//==================================================== Organization

//These calls may be restricted to privileged users.

/** Returns an integer array of modulators known to the dss. */
int dss__Apartment_GetModulatorIDs(int _token, IntArray& ids);
/** Returns the DSID of the given modulator. */
int dss__Modulator_GetDSID(int _token, int _modulatorID, xsd__unsignedInt& dsid);
/** Retuns the name of the given modulator. */
int dss__Modulator_GetName(int _token, int _modulatorID, char** name);
/** Allocates a zone */
int dss__Apartment_AllocateZone(int _token, int& zoneID);
/** Deletes a previously allocated zone. */
int dss__Apartment_DeleteZone(int _token, int _zoneID, int& result);
/** Adds a device to a zone */
int dss__Zone_AddDevice(int _token, int _zoneID, int _deviceID, int& result);
/** Removes a device from a zone */
int dss__Zone_RemoveDevice(int _token, int _zoneID, int _deviceID, int& result);
/** Sets the name of a zone to _name */
int dss__Zone_SetName(int _token, int _zoneID, char* _name, int& result);
/** Allocates a user-defined group. */
int dss__Apartment_AllocateUserGroup(int _token, int& groupID);
/** Revmoes a previously allocated group. */
int dss__Group_RemoveUserGroup(int _token, int _groupID, int& result);
/** Adds a device to the given group. */
int dss__Group_AddDevice(int _token, int _groupID, int _deviceID, int& result);
/** Removes a device from the given group. */
int dss__Group_RemoveDevice(int _token, int _groupID, int _deviceID, int& result);

/** Returns the function id of the specified device */
int dss__Device_GetFunctionID(int _token, xsd__unsignedInt _deviceID, int& result);
/** Returns the group id of the specified switch */
int dss__Switch_GetGroupID(int _token, xsd__unsignedInt _deviceID, int& result);
/** Simulates a key press on the specified switch and button
  * @param _kind One of ("click", "touch", "touchend")
  */
int dss__Switch_SimulateKeypress(int _token, xsd__unsignedInt _deviceID, int _buttonNr, char* _kind, bool& result);

class dss__inParameter {
public:
  StringArray* values;
  StringArray* names;
};

class dss__outParameter {
public:
  StringArray* values;
  StringArray* names;
};

//==================================================== Events

/** Raises an event with the given _eventID from _sourceID with _params */
int dss__Event_Raise(int _token, int _eventID, int _sourceID, dss__inParameter _params, int& result);
/** Returns an array of the names of all registered actions */
int dss__Event_GetActionNames(int _token, StringArray& names);
/** Returns a parameter template for the given action _name. */
int dss__Event_GetActionParamsTemplate(int _token, char* _name, dss__outParameter& paramsTemplate);
/** Adds a subscriptions to the events specified in _eventIDs if risen from one of _sourceIDs. If an
 * event with said properties occurs, the action identified by _actionName will be executed with the given _params. */
int dss__Event_Subscribe(int _token, IntArray _eventIDs, IntArray _sourceIDs, char* _actionName, dss__inParameter _params, int& subscriptionID);
/** Cancels a subscription. */
int dss__Event_Unsubscribe(int _token, int _subscriptionID, int& result);
/** Schedules an event. */
int dss__Event_Schedule(int _token, char* _icalString, int _eventID, dss__inParameter _params, int& scheduledEventID);
/** Cancels a scheduled event. */
int dss__Event_DeleteSchedule(int _token, int _scheduleEventID, int& result);
