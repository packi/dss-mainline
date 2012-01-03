/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
            Michael Tross, aizo GmbH <michael.tross@aizo.com>

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
//gsoap dss service namespace: https://localhost:8080/dss.wsdl
//gsoap dss service port: https://localhost:8081
//gsoap dss schema namespace:  urn:dss:1.0
//gsoap dss service type: MyType

#import "stl.h"

/** \file */

typedef unsigned long xsd__unsignedInt;
typedef unsigned long long xsd__unsignedLong;

/** Authenticates your ip to the system.
 * The token received will be used in any subsequent call. The ip/token pair
 * identifies a session. A Session will time out after n minutes of no activity (default 5).*/
int dss__Authenticate(char* _userName, char* _password, std::string& token);
int dss__AuthenticateAsApplication(char* _applicationToken, std::string& token);
int dss__RequestApplicationToken(char* _applicationName, std::string& result);
/** Terminates a session. All resources allocate by the session will be
 * released. */
int dss__SignOff(char* _token, int& result);

/** Creates a set containing all devices which are contained in a group named _groupName.
 * @see dss__CreateEmptySet */
int dss__ApartmentCreateSetFromGroup(char* _token, char* _groupName, std::string& result);
/** Creates a set containing all devices in the given array
 * @see dss__CreateEmptySet*/
int dss__ApartmentCreateSetFromDeviceIDs(char* _token, std::vector<std::string> _ids, std::string& result);
/** Creates a set containing all devices in the given array */
int dss__ApartmentCreateSetFromDeviceNames(char* _token, std::vector<std::string> _names, std::string& result);
/** Creates a set containing all devices */
int dss__ApartmentGetDevices(char* _token, std::string& result);

/** Returns the device ID for the given _deviceName */
int dss__ApartmentGetDeviceIDByName(char* _token, char* _deviceName, std::string& deviceID);

int dss__ApartmentGetName(char* _token, std::string& result);
int dss__ApartmentSetName(char* _token, char* _name, bool& result);

/** Adds the given device to the set */
int dss__SetAddDeviceByName(char* _token, char* _setSpec, char* _name, std::string& result);
/** Adds the given device to the set */
int dss__SetAddDeviceByID(char* _token, char* _setSpec, char* _deviceID, std::string& result);
/** Removes the device from the set */
int dss__SetRemoveDevice(char* _token, char* _setSpec, char* _deviceID, std::string& result);
/** Combines two sets into another set */
int dss__SetCombine(char* _token, char* _setSpec1, char* _setSpec2, std::string& result);
/** Removes all devices contained in _SetIDToRemove from _setID and copies those into setID */
int dss__SetRemove(char* _token, char* _setSpec, char* _setSpecToRemove, std::string& result);
/** Removes all devices which don't belong to the specified group */
int dss__SetByGroup(char* _token, char* _setSpec, int _groupID, std::string& result);
/** Returns an array containing all device ids contained in the given set */
int dss__SetGetContainedDevices(char* _token, char* _setSpec, std::vector<std::string>& deviceIDs);

/** Looks up the group id for the given group name */
int dss__ApartmentGetGroupByName(char* _token, char* _groupName, int& groupID);
/** Looks up the zone id for the given zone */
int dss__ApartmentGetZoneByName(char* _token, char* _zoneName, int& zoneID);
/** Returns an array containing all zone ids */
int dss__ApartmentGetZoneIDs(char* _token, std::vector<int>& zoneIDs);

//==================================================== Manipulation

//--------------------------- Set

/** Sends a turn on command to all devices contained in the set */
int dss__SetTurnOn(char* _token, char* _setSpec, bool& result);
/** Sends a turn off command to all devices contained in the set */
int dss__SetTurnOff(char* _token, char* _setSpec, bool& result);
/** Increases the main value (e.g. brightness) for each device contained in the set. */
int dss__SetIncreaseValue(char* _token, char* _setSpec, bool& result);
/** Decreases the main value (e.g. brightness) for each device contained in the set. */
int dss__SetDecreaseValue(char* _token, char* _setSpec, bool& result);

/** Sets the parameter specified by _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__SetSetValue(char* _token, char* _setSpec, unsigned char _value, bool& result);

/** Calls the scene _sceneNr on all devices contained int the set _setID. */
int dss__SetCallScene(char* _token, char* _setSpec, int _sceneNr, bool& result);
/** Saves the scene _sceneNr on all devices contained int the set _setID. */
int dss__SetSaveScene(char* _token, char* _setSpec, int _sceneNr, bool& result);
/** Restore the last scene if current is _sceneNr on all devices contained int the set _setID. */
int dss__SetUndoScene(char* _token, char* _setSpec, int _sceneNr, bool& result);
/** Restore the last scene on all devices contained int the set _setID. */
int dss__SetUndoLastScene(char* _token, char* _setSpec, bool& result);

//--------------------------- Apartment

/** Sends a turn on command to all devices contained in the group */
int dss__ApartmentTurnOn(char* _token, int _groupID, bool& result);
/** Sends a turn off command to all devices contained in the group */
int dss__ApartmentTurnOff(char* _token, int _groupID, bool& result);
/** Increases the main value (e.g. brightness) for each device contained in the group. */
int dss__ApartmentIncreaseValue(char* _token, int _groupID, bool& result);
/** Decreases the main value (e.g. brightness) for each device contained in the group. */
int dss__ApartmentDecreaseValue(char* _token, int _groupID, bool& result);

/** Sets the parameter specified by _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__ApartmentSetValue(char* _token, int _groupID, unsigned char _value, bool& result);

/** Calls the scene _sceneNr on all devices contained int the group _groupID. */
int dss__ApartmentCallScene(char* _token, int _groupID, int _sceneNr, bool& result);
/** Saves the scene _sceneNr on all devices contained int the group _groupID. */
int dss__ApartmentSaveScene(char* _token, int _groupID, int _sceneNr, bool& result);
/** Restore the last scene if current is _sceneNr. */
int dss__ApartmentUndoScene(char* _token, int _groupID, int _sceneNr, bool& result);
/** Restore the last scene. */
int dss__ApartmentUndoLastScene(char* _token, int _groupID, bool& result);

int dss__ApartmentBlink(char* _token, int _groupID, bool& result);

/** Rescans the bus for new devices/circuits */
int dss__ApartmentRescan(char* _token, bool& result);

//--------------------------- Circuit
/** Rescans the circuit for new/lost devices */
int dss__CircuitRescan(char* _token, char* _dsid, bool& result);

//--------------------------- Zone

/** Sends a turn on command to all devices contained in the zone/group */
int dss__ZoneTurnOn(char* _token, int _zoneID, int _groupID, bool& result);
/** Sends a turn off command to all devices contained in the group */
int dss__ZoneTurnOff(char* _token, int _zoneID, int _groupID, bool& result);
/** Increases the main value (e.g. brightness) for each device contained in the zone/group. */
int dss__ZoneIncreaseValue(char* _token, int _zoneID, int _groupID, bool& result);
/** Decreases the main value (e.g. brightness) for each device contained in the zone/group. */
int dss__ZoneDecreaseValue(char* _token, int _zoneID, int _groupID, bool& result);

/** Sets the parameter specified by _paramID to _value. If _paramID == -1 the default parameter
 * will be set. */
int dss__ZoneSetValue(char* _token, int _zoneID, int _groupID, unsigned char _value, bool& result);

/** Calls the scene _sceneNr on all devices contained int the zone/group _groupID. */
int dss__ZoneCallScene(char* _token, int _zoneID, int _groupID, int _sceneNr, bool _force = false, bool& result);
/** Saves the scene _sceneNr on all devices contained int the zone/group _groupID. */
int dss__ZoneSaveScene(char* _token, int _zoneID, int _groupID, int _sceneNr, bool& result);
/** Restore the last scene if current is _sceneNr. */
int dss__ZoneUndoScene(char* _token, int _zoneID, int _groupID, int _sceneID, bool& result);
/** Restore the last scene. */
int dss__ZoneUndoLastScene(char* _token, int _zoneID, int _groupID, bool& result);
/** Send a generic blink command to all devices in the zone */
int dss__ZoneBlink(char* _token, int _zoneID, int _groupID, bool& result);
/** Send a sensoric value into the zone */
int dss__ZonePushSensorValue(char* _token, int _zoneID, char *_sourceDeviceID, int _sensorType, int _sensorValue, bool& result);

//--------------------------- Device

/** Sends a blink command to the device. */
int dss__DeviceBlink(char* _token, char* _deviceID, bool& result);
/** Sends a turn on command to the device. */
int dss__DeviceTurnOn(char* _token, char* _deviceID, bool& result);
/** Sends a turn off command to the device. */
int dss__DeviceTurnOff(char* _token, char* _deviceID, bool& result);
/** Increases the main value (e.g. brightness) on the device. */
int dss__DeviceIncreaseValue(char* _token, char* _deviceID, bool& result);
/** Decreases the main value (e.g. brightness) on the device. */
int dss__DeviceDecreaseValue(char* _token, char* _deviceID, bool& result);
/** Set main value (e.g. brightness) on the device. */
int dss__DeviceSetValue(char* _token, char* _deviceID, unsigned char _value, bool& result);
/** Sets the configuration value of the given config class and config index */
int dss__DeviceSetConfig(char* _token, char* _deviceID, unsigned char _configClass, unsigned char _configIndex, unsigned char _value, bool& result);
/** Returns configuration value of the given config class and config index */
int dss__DeviceGetConfig(char* _token, char* _deviceID, unsigned char _configClass, unsigned char _configIndex, unsigned char& result);
/** Returns configuration value of the given config class and config index */
int dss__DeviceGetConfigWord(char* _token, char* _deviceID, unsigned char _configClass, unsigned char _configIndex, unsigned short int& result);
/** Sets the configuration value of the given config class and config index */
int dss__DeviceSetOutvalue(char* _token, char* _deviceID, unsigned char _valueIndex, unsigned short int _value, bool& result);
/** Returns configuration value of the given config class and config index */
int dss__DeviceGetOutvalue(char* _token, char* _deviceID, unsigned char _valueIndex, unsigned short int& result);
/** Returns sensor type code of the given sensor index */
int dss__DeviceGetSensorType(char* _token, char* _deviceID, unsigned char _sensorIndex, unsigned char& result);
/** Returns sensor value of the given sensor index */
int dss__DeviceGetSensorValue(char* _token, char* _deviceID, unsigned char _sensorIndex, unsigned short int& result);

/** Calls the scene _sceneNr on the device identified by _deviceID. */
int dss__DeviceCallScene(char* _token, char* _deviceID, int _sceneNr, bool _force = false, bool& result);
/** Saves the scene _sceneNr on the device identified by _devicdID. */
int dss__DeviceSaveScene(char* _token, char* _deviceID, int _sceneNr, bool& result);
/** Restore the last scene if current is _sceneNr. */
int dss__DeviceUndoScene(char* _token, char* _deviceID, int _sceneID, bool& result);
/** Restore the last scene. */
int dss__DeviceUndoLastScene(char* _token, char* _deviceID, bool& result);

/** Returns the name of a device */
int dss__DeviceGetName(char* _token, char* _deviceID, char** result);
/** Sets the name of a device */
int dss__DeviceSetName(char* _token, char* _deviceID, char* _name, bool& result);

/** Returns the zone id of the specified device */
int dss__DeviceGetZoneID(char* _token, char* _deviceID, int& result);

int dss__DeviceAddTag(char* _token, char* _deviceID, char* _tag, bool& result);
int dss__DeviceRemoveTag(char* _token, char* _deviceID, char* _tag, bool& result);
int dss__DeviceHasTag(char* _token, char* _deviceID, char* _tag, bool& result);
int dss__DeviceGetTags(char* _token, char* _deviceID, std::vector<std::string>& result);
int dss__DeviceLock(char* _token, char* _deviceID, bool& result);
int dss__DeviceUnlock(char* _token, char* _deviceID, bool& result);
int dss__DeviceGetIsLocked(char* _token, char* _deviceID, bool& result);

class dss__TransmissionQuality {
public:
  int upstream;
  int downstream;
};

int dss__DeviceGetTransmissionQuality(char* _token, char* _deviceID, dss__TransmissionQuality& result);

//==================================================== Information

int dss__DSMeterGetPowerConsumption(char* _token, char* _dsMeterID, xsd__unsignedInt& result);
int dss__DSMeterGetEnergyMeterValue(char* _token, char* _dsMeterID, xsd__unsignedInt& result);

//==================================================== Organization

//These calls may be restricted to privileged users.

/** Returns an integer array of dsMeters known to the dss. */
int dss__ApartmentGetDSMeterIDs(char* _token, std::vector<std::string>& ids);
/** Retuns the name of the given dsMeter. */
int dss__DSMeterGetName(char* _token, char* _dsMeterID, std::string& name);
/** Sets the name of the given dsMeter. */
int dss__DSMeterSetName(char* _token, char* _dsMeterID, char*  _name, bool& result);
/** Allocates a zone */
int dss__ApartmentAllocateZone(char* _token, int& zoneID);
/** Deletes a previously allocated zone. */
int dss__ApartmentDeleteZone(char* _token, int _zoneID, int& result);
int dss__ApartmentRemoveMeter(char* _token, char* _dsMeterID, bool& result);
int dss__ApartmentRemoveInactiveMeters(char* _token, bool& result);
int dss__ApartmentGetPowerConsumption(char* _token, xsd__unsignedInt& result);
int dss__ApartmentGetEnergyMeterValue(char* _token, xsd__unsignedInt& result);
/** Sets the name of a zone to _name */
int dss__ZoneSetName(char* _token, int _zoneID, char* _name, bool& result);
/** Returns the name of a zone */
int dss__ZoneGetName(char* _token, int _zoneID, std::string& result);
int dss__GroupSetName(char* _token, int _zoneID, int _groupID, char* _name, bool& result);
int dss__GroupGetName(char* _token, int _zoneID, int _groupID, std::string& result);

/** Returns the function id of the specified device */
int dss__DeviceGetFunctionID(char* _token, char* _deviceID, int& result);


//==================================================== Events

class dss__Event {
public:
  std::string name;
  std::vector<std::string> parameter;
};

int dss__EventRaise(char* _token, char* _eventName, char* _context, char* _parameter, char* _location, bool& result);
int dss__EventWaitFor(char* _token, int _timeout, std::vector<dss__Event>& result);
int dss__EventSubscribeTo(char* _token, std::string _name, std::string& result);


// =================================================== Properties

int dss__PropertyGetType(char* _token, std::string _propertyName, std::string& result);

int dss__PropertySetInt(char* _token, std::string _propertyName, int _value, bool _mayCreate = true, bool& result);
int dss__PropertySetString(char* _token, std::string _propertyName, char* _value, bool _mayCreate = true, bool& result);
int dss__PropertySetBool(char* _token, std::string _propertyName, bool _value, bool _mayCreate = true, bool& result);

int dss__PropertyGetInt(char* _token, std::string _propertyName, int& result);
int dss__PropertyGetString(char* _token, std::string _propertyName, std::string& result);
int dss__PropertyGetBool(char* _token, std::string _propertyName, bool& result);

int dss__PropertyGetChildren(char* _token, std::string _propertyName, std::vector<std::string>& result);
int dss__PropertyRemove(char* _token, std::string _propertyName, bool& result);

class dss__Property {
  std::string name;
  std::string value;
  std::string type;
};

class dss__PropertyQueryEntry {
  std::string name;
  std::vector<dss__Property> properties;
  std::vector<dss__PropertyQueryEntry> results;
};

int dss__PropertyQuery(char* _token, std::string _query, dss__PropertyQueryEntry& result);

//==================================================== Structure

int dss__StructureAddDeviceToZone(char* _token, char* _deviceID, int _zoneID, bool& result);

//==================================================== Metering

class dss__MeteringResolutions {
  std::string type;
  std::string unit;
  int resolution;
};
int dss__MeteringGetResolutions(char* _token, std::vector<dss__MeteringResolutions>& result);

class dss__MeteringSeries {
  std::string dsid;
  std::string type;
};
int dss__MeteringGetSeries(char* _token, std::vector<dss__MeteringSeries>& result);

class dss__MeteringValue {
  int timestamp;
  int value;
};
int dss__MeteringGetValues(char* _token, char* _dsMeterID, std::string _type, int _resolution, std::string* _unit, std::vector<dss__MeteringValue>& result);
class dss__MeteringValuePerDevice {
  std::string dsid;
  int timestamp;
  int value;
};
int dss__MeteringGetLastest(char* _token, std::string _from, std::string _type, std::string* _unit, std::vector<dss__MeteringValuePerDevice>& result);
