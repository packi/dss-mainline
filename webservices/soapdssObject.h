/* soapdssObject.h
   Generated by gSOAP 2.7.15 from model_soap.h
   Copyright(C) 2000-2009, Robert van Engelen, Genivia Inc. All Rights Reserved.
   This part of the software is released under one of the following licenses:
   GPL, the gSOAP public license, or Genivia's license for commercial use.
*/

#ifndef soapdssObject_H
#define soapdssObject_H
#include "soapH.h"

// soap_accept() timeout in seconds
#define SOAP_ACCEPT_TIMEOUT 2

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

    bind_flags = SO_REUSEADDR;
    accept_timeout = SOAP_ACCEPT_TIMEOUT; // seconds
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


SOAP_FMAC5 int SOAP_FMAC6 dss__Authenticate(struct soap*, char *_userName, char *_password, std::string &token);

SOAP_FMAC5 int SOAP_FMAC6 dss__SignOff(struct soap*, char *_token, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentCreateSetFromGroup(struct soap*, char *_token, char *_groupName, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentCreateSetFromDeviceIDs(struct soap*, char *_token, std::vector<std::string >_ids, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentCreateSetFromDeviceNames(struct soap*, char *_token, std::vector<std::string >_names, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentGetDevices(struct soap*, char *_token, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentGetDeviceIDByName(struct soap*, char *_token, char *_deviceName, std::string &deviceID);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentGetName(struct soap*, char *_token, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentSetName(struct soap*, char *_token, char *_name, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetAddDeviceByName(struct soap*, char *_token, char *_setSpec, char *_name, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetAddDeviceByID(struct soap*, char *_token, char *_setSpec, char *_deviceID, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetRemoveDevice(struct soap*, char *_token, char *_setSpec, char *_deviceID, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetCombine(struct soap*, char *_token, char *_setSpec1, char *_setSpec2, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetRemove(struct soap*, char *_token, char *_setSpec, char *_setSpecToRemove, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetByGroup(struct soap*, char *_token, char *_setSpec, int _groupID, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetGetContainedDevices(struct soap*, char *_token, char *_setSpec, std::vector<std::string >&deviceIDs);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentGetGroupByName(struct soap*, char *_token, char *_groupName, int &groupID);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentGetZoneByName(struct soap*, char *_token, char *_zoneName, int &zoneID);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentGetZoneIDs(struct soap*, char *_token, std::vector<int >&zoneIDs);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetTurnOn(struct soap*, char *_token, char *_setSpec, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetTurnOff(struct soap*, char *_token, char *_setSpec, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetIncreaseValue(struct soap*, char *_token, char *_setSpec, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetDecreaseValue(struct soap*, char *_token, char *_setSpec, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetSetValue(struct soap*, char *_token, char *_setSpec, unsigned char _value, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetCallScene(struct soap*, char *_token, char *_setSpec, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__SetSaveScene(struct soap*, char *_token, char *_setSpec, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentTurnOn(struct soap*, char *_token, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentTurnOff(struct soap*, char *_token, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentIncreaseValue(struct soap*, char *_token, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentDecreaseValue(struct soap*, char *_token, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentSetValue(struct soap*, char *_token, int _groupID, unsigned char _value, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentCallScene(struct soap*, char *_token, int _groupID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentSaveScene(struct soap*, char *_token, int _groupID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentRescan(struct soap*, char *_token, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__CircuitRescan(struct soap*, char *_token, char *_dsid, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneTurnOn(struct soap*, char *_token, int _zoneID, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneTurnOff(struct soap*, char *_token, int _zoneID, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneIncreaseValue(struct soap*, char *_token, int _zoneID, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneDecreaseValue(struct soap*, char *_token, int _zoneID, int _groupID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneSetValue(struct soap*, char *_token, int _zoneID, int _groupID, unsigned char _value, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneCallScene(struct soap*, char *_token, int _zoneID, int _groupID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneSaveScene(struct soap*, char *_token, int _zoneID, int _groupID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceTurnOn(struct soap*, char *_token, char *_deviceID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceTurnOff(struct soap*, char *_token, char *_deviceID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceIncreaseValue(struct soap*, char *_token, char *_deviceID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceDecreaseValue(struct soap*, char *_token, char *_deviceID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceSetValue(struct soap*, char *_token, char *_deviceID, unsigned char _value, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceSetConfig(struct soap*, char *_token, char *_deviceID, unsigned char _configClass, unsigned char _configIndex, unsigned char _value, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceGetConfig(struct soap*, char *_token, char *_deviceID, unsigned char _configClass, unsigned char _configIndex, unsigned char &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceGetConfigWord(struct soap*, char *_token, char *_deviceID, unsigned char _configClass, unsigned char _configIndex, unsigned short &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceCallScene(struct soap*, char *_token, char *_deviceID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceSaveScene(struct soap*, char *_token, char *_deviceID, int _sceneNr, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceGetName(struct soap*, char *_token, char *_deviceID, char **result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceSetName(struct soap*, char *_token, char *_deviceID, char *_name, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceGetZoneID(struct soap*, char *_token, char *_deviceID, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceAddTag(struct soap*, char *_token, char *_deviceID, char *_tag, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceRemoveTag(struct soap*, char *_token, char *_deviceID, char *_tag, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceHasTag(struct soap*, char *_token, char *_deviceID, char *_tag, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceGetTags(struct soap*, char *_token, char *_deviceID, std::vector<std::string >&result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceLock(struct soap*, char *_token, char *_deviceID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceUnlock(struct soap*, char *_token, char *_deviceID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceGetIsLocked(struct soap*, char *_token, char *_deviceID, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DSMeterGetPowerConsumption(struct soap*, char *_token, char *_dsMeterID, unsigned long &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentGetDSMeterIDs(struct soap*, char *_token, std::vector<std::string >&ids);

SOAP_FMAC5 int SOAP_FMAC6 dss__DSMeterGetName(struct soap*, char *_token, char *_dsMeterID, std::string &name);

SOAP_FMAC5 int SOAP_FMAC6 dss__DSMeterSetName(struct soap*, char *_token, char *_dsMeterID, char *_name, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentAllocateZone(struct soap*, char *_token, int &zoneID);

SOAP_FMAC5 int SOAP_FMAC6 dss__ApartmentDeleteZone(struct soap*, char *_token, int _zoneID, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneSetName(struct soap*, char *_token, int _zoneID, char *_name, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__ZoneGetName(struct soap*, char *_token, int _zoneID, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__GroupSetName(struct soap*, char *_token, int _zoneID, int _groupID, char *_name, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__GroupGetName(struct soap*, char *_token, int _zoneID, int _groupID, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__DeviceGetFunctionID(struct soap*, char *_token, char *_deviceID, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__EventRaise(struct soap*, char *_token, char *_eventName, char *_context, char *_parameter, char *_location, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__EventWaitFor(struct soap*, char *_token, int _timeout, std::vector<dss__Event >&result);

SOAP_FMAC5 int SOAP_FMAC6 dss__EventSubscribeTo(struct soap*, char *_token, std::string _name, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertyGetType(struct soap*, char *_token, std::string _propertyName, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertySetInt(struct soap*, char *_token, std::string _propertyName, int _value, bool _mayCreate, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertySetString(struct soap*, char *_token, std::string _propertyName, char *_value, bool _mayCreate, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertySetBool(struct soap*, char *_token, std::string _propertyName, bool _value, bool _mayCreate, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertyGetInt(struct soap*, char *_token, std::string _propertyName, int &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertyGetString(struct soap*, char *_token, std::string _propertyName, std::string &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertyGetBool(struct soap*, char *_token, std::string _propertyName, bool &result);

SOAP_FMAC5 int SOAP_FMAC6 dss__PropertyGetChildren(struct soap*, char *_token, std::string _propertyName, std::vector<std::string >&result);

SOAP_FMAC5 int SOAP_FMAC6 dss__StructureAddDeviceToZone(struct soap*, char *_token, char *_deviceID, int _zoneID, bool &result);

#endif
