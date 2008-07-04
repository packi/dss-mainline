//gsoap dss service name: dss
//gsoap dss service namespace: http://localhost/dss.wsdl
//gsoap dss service location: http://localhost:8081/
//gsoap dss service executable: 
//gsoap dss schema namespace: urn:copy

class IntArray {
public:
  int* __ptr;
  int __size;
};

class StringArray {
public:
  char** __ptr;
  int __size;
};

typedef long long DSID;

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
int dss__Set_ByGroup(int _token, int _setID, int _groupID, int& setID);

int dss__Apartment_GetGroupByName(int _token, char* _groupName, int& groupID);
int dss__Apartment_GetRoomByName(int _token, char* _roomName, int& roomID);
int dss__Apartment_GetRoomIDs(int _token, IntArray& roomIDs);

//==================================================== Manipulation

int dss__Set_TurnOn(int _token, int _setID, bool& result);
int dss__Set_TurnOff(int _token, int _setID, bool& result);
int dss__Set_IncreaseValue(int _token, int _setID, int _paramID, bool& result);
int dss__Set_DecreaseValue(int _token, int _setID, int _paramID, bool& result);

int dss__Set_Enable(int _token, int _setID, bool& result);
int dss__Set_Disable(int _token, int _setID, bool& result);
int dss__Set_StartDim(int _token, int _setID, bool _directionUp, int _paramID, bool& result);
int dss__Set_EndDim(int _token, int _setID, int _paramID, bool& result);
int dss__Set_SetValue(int _token, int _setID, double _value, int _paramID, bool& result);

int dss__Device_TurnOn(int _token, int _deviceID, bool& result);
int dss__Device_TurnOff(int _token, int _deviceID, bool& result);
int dss__Device_IncreaseValue(int _token, int _deviceID, int _paramID, bool& result);
int dss__Device_DecreaseValue(int _token, int _deviceID, int _paramID, bool& result);
int dss__Device_Enable(int _token, int _deviceID, bool& result);
int dss__Device_Disable(int _token, int _deviceID, bool& result);
int dss__Device_StartDim(int _token, int _deviceID, bool _directionUp, int _paramID, bool& result);
int dss__Device_EndDim(int _token, int _deviceID, int _paramID, bool& result);
int dss__Device_SetValue(int _token, int _deviceID, double _value, int _paramID, bool& result);
int dss__Device_GetValue(int _token, int _deviceID, int _paramID, double& result);

//==================================================== Information

int dss__Device_GetDSID(int _token, int _deviceID, DSID& result);

//==================================================== Organization

//These calls may be restricted to privileged users.

int dss__Apartment_GetModulatorIDs(int _token, IntArray& ids);
int dss__Modulator_GetDSID(int _token, int _modulatorID, DSID& dsid);
int dss__Modulator_GetName(int _token, int _modulatorID, char** name);

int dss__Apartment_AllocateRoom(int _token, int& roomID);
int dss__Apartment_DeleteRoom(int _token, int _roomID, int& result);
int dss__Room_AddDevice(int _token, int _roomID, int _deviceID, int& result);
int dss__Room_RemoveDevice(int _token, int _roomID, int _deviceID, int& result);
int dss__Room_SetName(int _token, int _roomID, char* _name, int& result);
int dss__Apartment_AllocateUserGroup(int _token, int& groupID);
int dss__Group_RemoveUserGroup(int _token, int _groupID, int& result);
int dss__Group_AddDevice(int _token, int _groupID, int _deviceID, int& result);
int dss__Group_RemoveDevice(int _token, int _groupID, int _deviceID, int& result);

class Parameter {
public:
  StringArray* values;
  StringArray* names;
};

//==================================================== Events

int dss__Event_Raise(int _token, int _eventID, int _sourceID, Parameter _params, int& result);
int dss__Event_GetActionNames(int _token, StringArray& names);
int dss__Event_GetActionParamsTemplate(int _token, char* _name, Parameter& paramsTemplate);
int dss__Event_Subscribe(int _token, IntArray _eventIDs, IntArray _sourceIDs, char* _actionName, Parameter _params, int& subscriptionID);
int dss__Event_Unsubscribe(int _token, int _subscriptionID, int& result);
int dss__Event_Schedule(int _token, char* _icalString, int _eventID, Parameter _params, int& scheduledEventID);
int dss__Event_DeleteSchedule(int _token, int _scheduleEventID, int& result);
