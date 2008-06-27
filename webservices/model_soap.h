//gsoap dss service name: dss
//gsoap dss service namespace: http://localhost/dss.wsdl
//gsoap dss service location: http://localhost:8081/
//gsoap dss service executable: 
//gsoap dss schema namespace: urn:copy
#ifndef _MODEL_SOAP_H_INCLUDED
#define _MODEL_SOAP_H_INCLUDED
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

int dss__Authenticate(char* _userName, char* _password, int& token);
int dss__SignOff(int _token, int& result);

int dss__Apartment_CreateSetFromGroup(char* _groupName, int& setID);
int dss__Apartment_CreateSetFromDeviceIDs(IntArray _ids, int& setID);
int dss__Apartment_CreateSetFromDeviceNames(StringArray _names, int& setID);
int dss__Apartment_CreateEmptySet(int& setID);
int dss__Apartment_GetDevices(int& setID);
int dss__Apartment_GetDeviceIDByName(char* _deviceName, int& deviceID);

int dss__Set_AddDeviceByName(int _setID, char* _name, int& setID);
int dss__Set_AddDeviceByID(int _setID, int _deviceID,int& setID);
int dss__Set_Combine(int _setID1, int _setID2, int& setID);
int dss__Set_Remove(int _setID, int _setIDToRemove, int& setID);
int dss__Set_ByGroup(int _setID, int _groupID, int& setID);
int dss__Set_RemoveDevice(int _setID, int _deviceID, int& setID);
int dss__Set_AddDevice(int _setID, int _deviceID, int& setID);

int dss__Apartment_GetGroupByName(int _groupName, int& groupID);
int dss__Apartment_GetRoomByName(char* _roomName, int& roomID);
int dss__Apartment_GetRoomIDs(IntArray& roomIDs);

//==================================================== Manipulation

int dss__Set_TurnOn(int _setID, int& result);
int dss__Set_TurnOff(int _setID, int& result);
int dss__Set_IncreaseValue(int _setID, int _paramID, int& result);
int dss__Set_DecreaseValue(int _setID, int _paramID, int& result);

int dss__Set_Enable(int _setID, int& result);
int dss__Set_Disable(int _setID, int& result);
int dss__Set_StartDim(int _setID, bool _directionUp, int _paramID, int& result);
int dss__Set_EndDim(int _setID, int _paramID, int& result);
int dss__Set_SetValue(int _setID, float _value, int _paramID, int& result);

int dss__Device_TurnOn(int _deviceID, int& result);
int dss__Device_TurnOff(int _deviceID, int& result);
int dss__Device_IncreaseValue(int _deviceID, int _paramID, int& result);
int dss__Device_DecreaseValue(int _deviceID, int _paramID, int& result);
int dss__Device_Enable(int _deviceID, int& result);
int dss__Device_Disable(int _deviceID, int& result);
int dss__Device_StartDim(int _deviceID, bool _directionUp, int _paramID, int& result);
int dss__Device_EndDim(int _deviceID, int _paramID, int& result);
int dss__Device_SetValue(int _deviceID, float _value, int _paramID, int& result);
int dss__Device_GetValue(int _deviceID, float& result);

//==================================================== Information

int dss__Device_GetDSID(int _deviceID, DSID& result);

//==================================================== Organization

//These calls may be restricted to privileged users.

int dss__Apartment_GetModulatorIDs(IntArray& ids);
int dss__Modulator_GetDSID(int _modulatorID, DSID& dsid);
int dss__Modulator_GetName(int _modulatorID, char** name);

int dss__Apartment_AllocateRoom(int& roomID);
int dss__Apartment_DeleteRoom(int _roomID, int& result);
int dss__Room_AddDevice(int _roomID, int _deviceID, int& result);
int dss__Room_RemoveDevice(int _roomID, int _deviceID, int& result);
int dss__Room_SetName(int _roomID, char* _name, int& result);
int dss__Apartment_AllocateUserGroup(int& groupID);
int dss__Group_RemoveUserGroup(int _groupID, int& result);
int dss__Group_AddDevice(int _groupID, int _deviceID, int& result);
int dss__Group_RemoveDevice(int _groupID, int _deviceID, int& result);

class Parameter {
public:
  StringArray* values;
  StringArray* names;
};
  
int dss__Event_Raise(int _eventID, int _sourceID, Parameter _params, int& result);
int dss__Event_GetActionNames(StringArray& names);
int dss__Event_GetActionParamsTemplate(char* _name, Parameter& paramsTemplate);
int dss__Event_Subscribe(IntArray _eventIDs, IntArray _sourceIDs, char* _actionName, Parameter _params, int& subscriptionID);
int dss__Event_Unsubscribe(int _subscriptionID, int& result);
int dss__Event_Schedule(char* _icalString, int _eventID, Parameter _params, int& scheduledEventID);
int dss__Event_DeleteSchedule(int _scheduleEventID, int& result);
#endif