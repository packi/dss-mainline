#include "soapH.h"
#include "../core/dss.h"

#include <vector>
#include <string>
#include <boost/foreach.hpp>

//==================================================== Helpers

int NotAuthorized(struct soap *soap) {
  return soap_receiver_fault(soap, "Not Authorized", NULL);
} // NotAuthorized

bool IsAuthorized(struct soap *soap, const int _token) {
  return dss::DSS::GetInstance()->GetWebServices().IsAuthorized(soap, _token);
} // IsAuthorized

int AuthorizeAndGetSet(struct soap *soap, const int _token, const int _setID, dss::Set& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  if(!sess.HasSetWithID(_setID)) {
    return soap_receiver_fault(soap, "The Set with the given id does not exist",  NULL);
  }
  result = sess.GetSetByID(_setID);
  return SOAP_OK;
} // AuthorizeAndGetSet

int AuthorizeAndGetDevice(struct soap *soap, const int _token, const int _devID, dss::Device& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    result = apt.GetDeviceByID(_devID);
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Device not found", NULL);
  }
  return SOAP_OK;
} // AuthorizeAndGetDevice

int AuthorizeAndGetModulator(struct soap *soap, const int _token, const int _modulatorID, dss::Modulator& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    result = apt.GetModulator(_modulatorID);
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Modulator not found", NULL);
  }
  return SOAP_OK;
}

//==================================================== Callbacks

int dss__Authenticate(struct soap *soap, char* _userName, char* _password, int& token) {
  dss::DSS::GetInstance()->GetWebServices().NewSession(soap, token);
  return SOAP_OK;
} // dss__Authenticate

int dss__SignOff(struct soap *soap, int _token, int& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::DSS::GetInstance()->GetWebServices().DeleteSession(soap, _token);
  return SOAP_OK;
}

int dss__Apartment_CreateSetFromGroup(struct soap *soap, int _token,  char* _groupName, int& setID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    sess.AddSet(apt.GetDevices().GetByGroup(_groupName), setID);
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Unknown group", NULL);
  }
  return SOAP_OK;
}

int dss__Apartment_CreateSetFromDeviceIDs(struct soap *soap, int _token, IntArray _ids, int& setID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    dss::Set set;
    for(int iID = 0; iID < _ids.__size; iID++) {
      dss::Device& dev = apt.GetDeviceByID(_ids.__ptr[iID]);
      set.AddDevice(dev);
    }
    sess.AddSet(set, setID);
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Unknown device", NULL);
  }
  return SOAP_OK;
}

int dss__Apartment_CreateSetFromDeviceNames(struct soap *soap, int _token,  StringArray _names, int& setID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    dss::Set set;
    for(int iName = 0; iName < _names.__size; iName++) {
      dss::Device& dev = apt.GetDeviceByName(_names.__ptr[iName]);
      set.AddDevice(dev);
    }
    sess.AddSet(set, setID);
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Unknown device", NULL);
  }
  return SOAP_OK;
}

int dss__Apartment_CreateEmptySet(struct soap *soap, int _token, int& setID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token).AllocateSet(setID);
  return SOAP_OK;
}

int dss__Apartment_GetDevices(struct soap *soap, int _token, int& setID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  sess.AddSet(apt.GetDevices(), setID);
  return SOAP_OK;
}

int dss__Apartment_GetDeviceIDByName(struct soap *soap, int _token,  char* _deviceName, int& deviceID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    deviceID = apt.GetDevices().GetByName(_deviceName).GetID();
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
}

int dss__Set_AddDeviceByName(struct soap *soap, int _token, int _setID, char* _name, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  if(!sess.HasSetWithID(_setID)) {
    return soap_receiver_fault(soap, "The Set with the given id does not exist",  NULL);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();  
  dss::Set& origSet = sess.GetSetByID(_setID);
  try {
    dss::DeviceReference devRef = apt.GetDevices().GetByName(_name);
    origSet.AddDevice(devRef);
    result = true;
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
}

int dss__Set_AddDeviceByID(struct soap *soap, int _token, int _setID, int _deviceID, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  if(!sess.HasSetWithID(_setID)) {
    return soap_receiver_fault(soap, "The Set with the given id does not exist",  NULL);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();  
  dss::Set& origSet = sess.GetSetByID(_setID);
  try {
    dss::DeviceReference devRef = apt.GetDevices().GetByID(_deviceID);
    origSet.AddDevice(devRef);
    result = true;
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
}

int dss__Set_RemoveDevice(struct soap *soap, int _token, int _setID, int _deviceID, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  if(!sess.HasSetWithID(_setID)) {
    return soap_receiver_fault(soap, "The Set with the given id does not exist",  NULL);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();  
  dss::Set& origSet = sess.GetSetByID(_setID);
  try {
    dss::DeviceReference devRef = apt.GetDevices().GetByID(_deviceID);
    origSet.RemoveDevice(devRef);
    result = true;
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
}

int dss__Set_Combine(struct soap *soap, int _token, int _setID1, int _setID2, int& setID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  if(!sess.HasSetWithID(_setID1)) {
    return soap_receiver_fault(soap, "Set1 with the given id does not exist",  NULL);
  }
  if(!sess.HasSetWithID(_setID2)) {
    return soap_receiver_fault(soap, "Set2 with the given id does not exist",  NULL);
  }
  dss::Set& set1 = sess.GetSetByID(_setID1);
  dss::Set& set2 = sess.GetSetByID(_setID2);  
  try {
    dss::Set resultingSet = set1.Combine(set2);
    sess.AddSet(resultingSet, setID);
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
}

int dss__Set_Remove(struct soap *soap, int _token, int _setID, int _setIDToRemove, int& setID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  if(!sess.HasSetWithID(_setID)) {
    return soap_receiver_fault(soap, "Set with the given id does not exist",  NULL);
  }
  if(!sess.HasSetWithID(_setIDToRemove)) {
    return soap_receiver_fault(soap, "Set to remove with the given id does not exist",  NULL);
  }
  dss::Set& set1 = sess.GetSetByID(_setID);
  dss::Set& set2 = sess.GetSetByID(_setIDToRemove);  
  try {
    dss::Set resultingSet = set1.Remove(set2);
    sess.AddSet(resultingSet, setID);
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
}

int dss__Set_ByGroup(struct soap *soap, int _token, int _setID, int _groupID, int& setID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  if(!sess.HasSetWithID(_setID)) {
    return soap_receiver_fault(soap, "The Set with the given id does not exist",  NULL);
  }
  dss::Set& origSet = sess.GetSetByID(_setID);
  try {
    dss::Set newSet = origSet.GetByGroup(_groupID);
    sess.AddSet(newSet, setID);
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
}

//==================================================== Apartment

int dss__Apartment_GetGroupByName(struct soap *soap, int _token, char* _groupName, int& groupID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();  
  try {
    groupID = apt.GetGroup(_groupName).GetID();
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Could not find group", NULL);
  }
  return SOAP_OK;
}

int dss__Apartment_GetRoomByName(struct soap *soap, int _token,  char* _roomName, int& roomID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();  
  try {
    roomID = apt.GetRoom(_roomName).GetRoomID();
  } catch(dss::ItemNotFoundException* _ex) {
    return soap_receiver_fault(soap, "Could not find room", NULL);
  }
  return SOAP_OK;
}

int dss__Apartment_GetRoomIDs(struct soap *soap, int _token, IntArray& roomIDs) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

//==================================================== Manipulation

int dss__Set_TurnOn(struct soap *soap, int _token, int _setID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.TurnOn();
  result = true;
  return SOAP_OK;
}

int dss__Set_TurnOff(struct soap *soap, int _token, int _setID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.TurnOff();
  result = true;
  return SOAP_OK;
}

int dss__Set_IncreaseValue(struct soap *soap, int _token, int _setID, int _paramID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.IncreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__Set_DecreaseValue(struct soap *soap, int _token, int _setID, int _paramID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.DecreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__Set_Enable(struct soap *soap, int _token, int _setID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.Enable();
  result = true;
  return SOAP_OK;
}

int dss__Set_Disable(struct soap *soap, int _token, int _setID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.Disable();
  result = true;
  return SOAP_OK;
}

int dss__Set_StartDim(struct soap *soap, int _token, int _setID, bool _directionUp, int _paramID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.StartDim(_directionUp, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__Set_EndDim(struct soap *soap, int _token, int _setID, int _paramID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.EndDim(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__Set_SetValue(struct soap *soap, int _token, int _setID, double _value, int _paramID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.SetValue(_value, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__Device_TurnOn(struct soap *soap, int _token, int _deviceID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.TurnOn();
  result = true;
  return SOAP_OK;
}

int dss__Device_TurnOff(struct soap *soap, int _token, int _deviceID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.TurnOff();
  result = true;
  return SOAP_OK;
}

int dss__Device_IncreaseValue(struct soap *soap, int _token, int _deviceID, int _paramID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.IncreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__Device_DecreaseValue(struct soap *soap, int _token, int _deviceID, int _paramID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.DecreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__Device_Enable(struct soap *soap, int _token, int _deviceID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.Enable();
  result = true;
  return SOAP_OK;
}

int dss__Device_Disable(struct soap *soap, int _token, int _deviceID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.Disable();
  result = true;
  return SOAP_OK;
}

int dss__Device_StartDim(struct soap *soap, int _token, int _deviceID, bool _directionUp, int _paramID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.StartDim(_directionUp, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__Device_EndDim(struct soap *soap, int _token, int _deviceID, int _paramID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.EndDim(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__Device_SetValue(struct soap *soap, int _token, int _deviceID, double _value, int _paramID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.SetValue(_value, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__Device_GetValue(struct soap *soap, int _token, int _deviceID, int _paramID, double& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  result = dev.GetValue(_paramID);
  return SOAP_OK;
}

//==================================================== Information

int dss__Device_GetDSID(struct soap *soap, int _token, int _deviceID, DSID& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

//==================================================== Organization

//These calls may be restricted to privileged users.

int dss__Apartment_GetModulatorIDs(struct soap *soap, int _token, IntArray& ids) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  
  std::vector<dss::Modulator*>& modulators = apt.GetModulators();
  
  // apparently gSOAP is handling the memory 
  ids.__ptr = (int*)malloc(sizeof(int) * modulators.size());
  ids.__size = modulators.size();
  
  for(int iModulator = 0; iModulator < ids.__size; iModulator++) {
    ids.__ptr[iModulator] = modulators[iModulator]->GetID();
  }
  
  return SOAP_OK;  
}

int dss__Modulator_GetDSID(struct soap *soap, int _token, int _modulatorID, DSID& dsid) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Modulator_GetName(struct soap *soap, int _token, int _modulatorID, char** name) {
  dss::Modulator mod(-1);
  int getResult = AuthorizeAndGetModulator(soap, _token, _modulatorID, mod);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  
  std::string tmpName = mod.GetName(); 
  *name = (char*)malloc(tmpName.length() * sizeof(char));
  strncpy(*name, tmpName.c_str(), tmpName.length());
  return SOAP_OK;
}

int dss__Apartment_AllocateRoom(struct soap *soap, int _token, int& roomID) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Apartment_DeleteRoom(struct soap *soap, int _token, int _roomID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Room_AddDevice(struct soap *soap, int _token, int _roomID, int _deviceID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Room_RemoveDevice(struct soap *soap, int _token, int _roomID, int _deviceID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Room_SetName(struct soap *soap, int _token, int _roomID, char* _name, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Apartment_AllocateUserGroup(struct soap *soap, int _token, int& groupID) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Group_RemoveUserGroup(struct soap *soap, int _token, int _groupID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Group_AddDevice(struct soap *soap, int _token, int _groupID, int _deviceID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Group_RemoveDevice(struct soap *soap, int _token, int _groupID, int _deviceID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

//==================================================== Events

int dss__Event_Raise(struct soap *soap, int _token, int _eventID, int _sourceID, Parameter _params, int& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  int numNames = 0;
  int numValues = 0;
  if(_params.names != NULL) {
    numNames = _params.names->__size;
  }
  if(_params.values != NULL) {
    numValues = _params.values->__size;
  }
  if(numNames != numValues) {
    soap_receiver_fault(soap, "Names and values of _params must have the same number of items", NULL);
  }
  
  
  dss::Event evt(_eventID, _sourceID);
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
 // apt.OnEvent();
}

int dss__Event_GetActionNames(struct soap *soap, int _token,  StringArray& names) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Event_GetActionParamsTemplate(struct soap *soap, int _token,  char* _name, Parameter& paramsTemplate) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Event_Subscribe(struct soap *soap, int _token, IntArray _eventIDs, IntArray _sourceIDs, char* _actionName, Parameter _params, int& subscriptionID) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Event_Unsubscribe(struct soap *soap, int _token, int _subscriptionID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Event_Schedule(struct soap *soap, int _token,  char* _icalString, int _eventID, Parameter _params, int& scheduledEventID) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Event_DeleteSchedule(struct soap *soap, int _token, int _scheduleEventID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

