/* soapdssObject.h
   Generated by gSOAP 2.7.10 from model_soap.h
   Copyright(C) 2000-2008, Robert van Engelen, Genivia Inc. All Rights Reserved.
   This part of the software is released under one of the following licenses:
   GPL, the gSOAP public license, or Genivia's license for commercial use.
*/

#ifndef soapdssObject_H
#define soapdssObject_H
#include "soapH.h"

/******************************************************************************\
 *                                                                            *
 * Service Object                                                             *
 *                                                                            *
\******************************************************************************/

class dssService : public soap
{    public:
	dssService()
	{ static const struct Namespace namespaces[] =
{
	{"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
	{"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
	{"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
	{"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
	{"dss", "urn:dss:1.0", NULL, NULL},
	{NULL, NULL, NULL, NULL}
};
	if (!this->namespaces) this->namespaces = namespaces; };
	virtual ~dssService() { };
	/// Bind service to port (returns master socket or SOAP_INVALID_SOCKET)
	virtual	SOAP_SOCKET bind(const char *host, int port, int backlog) { return soap_bind(this, host, port, backlog); };
	/// Accept next request (returns socket or SOAP_INVALID_SOCKET)
	virtual	SOAP_SOCKET accept() { return soap_accept(this); };
	/// Serve this request (returns error code or SOAP_OK)
	virtual	int serve() { return soap_serve(this); };
};

/******************************************************************************\
 *                                                                            *
 * Service Operations (you should define these globally)                      *
 *                                                                            *
\******************************************************************************/


SOAP_FMAC5 int SOAP_FMAC6 dss__Authenticate(struct soap*, char *_userName, char *_password, int &token);

SOAP_FMAC5 int SOAP_FMAC6 dss__SignOff(struct soap*, int _token, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__FreeSet(struct soap*, int _token, int _setID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentCreateSetFromGroup(struct soap*, int _token, char *_groupName, int &setID);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentCreateSetFromDeviceIDs(struct soap*, int _token, std::vector<std::string >_ids, int &setID);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentCreateSetFromDeviceNames(struct soap*, int _token, std::vector<std::string >_names, int &setID);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentCreateEmptySet(struct soap*, int _token, int &setID);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentGetDevices(struct soap*, int _token, int &setID);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentGetDeviceIDByName(struct soap*, int _token, char *_deviceName, std::string &deviceID);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetAddDeviceByName(struct soap*, int _token, int _setID, char *_name, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetAddDeviceByID(struct soap*, int _token, int _setID, char *_deviceID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetRemoveDevice(struct soap*, int _token, int _setID, char *_deviceID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetCombine(struct soap*, int _token, int _setID1, int _setID2, int &setID);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetRemove(struct soap*, int _token, int _setID, int _setIDToRemove, int &setID);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetByGroup(struct soap*, int _token, int _setID, int _groupID, int &setID);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetGetContainedDevices(struct soap*, int _token, int _setID, std::vector<std::string >&deviceIDs);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentGetGroupByName(struct soap*, int _token, char *_groupName, int &groupID);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentGetZoneByName(struct soap*, int _token, char *_zoneName, int &zoneID);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentGetZoneIDs(struct soap*, int _token, std::vector<int >&zoneIDs);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetTurnOn(struct soap*, int _token, int _setID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetTurnOff(struct soap*, int _token, int _setID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetIncreaseValue(struct soap*, int _token, int _setID, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetDecreaseValue(struct soap*, int _token, int _setID, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetEnable(struct soap*, int _token, int _setID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetDisable(struct soap*, int _token, int _setID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetStartDim(struct soap*, int _token, int _setID, bool _directionUp, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetEndDim(struct soap*, int _token, int _setID, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetSetValue(struct soap*, int _token, int _setID, double _value, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetCallScene(struct soap*, int _token, int _setID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetSaveScene(struct soap*, int _token, int _setID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentTurnOn(struct soap*, int _token, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentTurnOff(struct soap*, int _token, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentIncreaseValue(struct soap*, int _token, int _groupID, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentDecreaseValue(struct soap*, int _token, int _groupID, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentEnable(struct soap*, int _token, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentDisable(struct soap*, int _token, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentStartDim(struct soap*, int _token, int _groupID, bool _directionUp, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentEndDim(struct soap*, int _token, int _groupID, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentSetValue(struct soap*, int _token, int _groupID, double _value, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentCallScene(struct soap*, int _token, int _groupID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentSaveScene(struct soap*, int _token, int _groupID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentRescan(struct soap*, int _token, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__CircuitRescan(struct soap*, int _token, char *_dsid, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneTurnOn(struct soap*, int _token, int _zoneID, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneTurnOff(struct soap*, int _token, int _zoneID, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneIncreaseValue(struct soap*, int _token, int _zoneID, int _groupID, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneDecreaseValue(struct soap*, int _token, int _zoneID, int _groupID, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneEnable(struct soap*, int _token, int _zoneID, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneDisable(struct soap*, int _token, int _zoneID, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneStartDim(struct soap*, int _token, int _zoneID, int _groupID, bool _directionUp, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneEndDim(struct soap*, int _token, int _zoneID, int _groupID, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneSetValue(struct soap*, int _token, int _zoneID, int _groupID, double _value, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneCallScene(struct soap*, int _token, int _zoneID, int _groupID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneSaveScene(struct soap*, int _token, int _zoneID, int _groupID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceTurnOn(struct soap*, int _token, char *_deviceID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceTurnOff(struct soap*, int _token, char *_deviceID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceIncreaseValue(struct soap*, int _token, char *_deviceID, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceDecreaseValue(struct soap*, int _token, char *_deviceID, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceEnable(struct soap*, int _token, char *_deviceID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceDisable(struct soap*, int _token, char *_deviceID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceStartDim(struct soap*, int _token, char *_deviceID, bool _directionUp, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceEndDim(struct soap*, int _token, char *_deviceID, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceSetValue(struct soap*, int _token, char *_deviceID, double _value, int _paramID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceGetValue(struct soap*, int _token, char *_deviceID, int _paramID, double &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceCallScene(struct soap*, int _token, char *_deviceID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceSaveScene(struct soap*, int _token, char *_deviceID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceGetName(struct soap*, int _token, char *_deviceID, char **result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceGetZoneID(struct soap*, int _token, char *_deviceID, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ModulatorGetPowerConsumption(struct soap*, int _token, int _modulatorID, unsigned long &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentGetModulatorIDs(struct soap*, int _token, std::vector<std::string >&ids);

SOAP_FMAC5 int SOAP_FMAC6 dss__ModulatorGetName(struct soap*, int _token, char *_modulatorID, std::string &name);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentAllocateZone(struct soap*, int _token, int &zoneID);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentDeleteZone(struct soap*, int _token, int _zoneID, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__Zone_AddDevice(struct soap*, int _token, int _zoneID, char *_deviceID, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__Zone_RemoveDevice(struct soap*, int _token, int _zoneID, char *_deviceID, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__Zone_SetName(struct soap*, int _token, int _zoneID, char *_name, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentAllocateUserGroup(struct soap*, int _token, int &groupID);

SOAP_FMAC5 int SOAP_FMAC6 dss__GroupRemoveUserGroup(struct soap*, int _token, int _groupID, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__GroupAddDevice(struct soap*, int _token, int _groupID, char *_deviceID, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__GroupRemoveDevice(struct soap*, int _token, int _groupID, char *_deviceID, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceGetFunctionID(struct soap*, int _token, char *_deviceID, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SwitchGetGroupID(struct soap*, int _token, char *_deviceID, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__EventRaise(struct soap*, int _token, char *_eventName, char *_context, char *_parameter, char *_location, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__EventWaitFor(struct soap*, int _token, int _timeout, std::vector<dss__Event >&result);

SOAP_FMAC5 int SOAP_FMAC6 dss__EventSubscribeTo(struct soap*, int _token, std::string _name, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertyGetType(struct soap*, int _token, std::string _propertyName, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertySetInt(struct soap*, int _token, std::string _propertyName, int _value, bool _mayCreate, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertySetString(struct soap*, int _token, std::string _propertyName, char *_value, bool _mayCreate, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertySetBool(struct soap*, int _token, std::string _propertyName, bool _value, bool _mayCreate, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertyGetInt(struct soap*, int _token, std::string _propertyName, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertyGetString(struct soap*, int _token, std::string _propertyName, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertyGetBool(struct soap*, int _token, std::string _propertyName, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertyGetChildren(struct soap*, int _token, std::string _propertyName, std::vector<std::string >&result);

#endif
