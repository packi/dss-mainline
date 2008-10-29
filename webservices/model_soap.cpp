#include "soapH.h"
#include "../core/dss.h"
#include "../core/logger.h"

#include <vector>
#include <string>
#include <boost/foreach.hpp>

//==================================================== Helpers

int NotAuthorized(struct soap *soap) {
  return soap_receiver_fault(soap, "Not Authorized", NULL);
} // NotAuthorized

bool IsAuthorized(struct soap *soap, const int _token) {
  bool result = dss::DSS::GetInstance()->GetWebServices().IsAuthorized(soap, _token);
  if(result) {
    dss::Logger::GetInstance()->Log(string("User with token '") + dss::IntToString(_token) + "' is authorized");
  } else {
    dss::Logger::GetInstance()->Log(string("User with token '") + dss::IntToString(_token) + "' is *not* authorized", lsWarning);
  }
  return result;
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
    result = apt.GetDeviceByDSID(_devID);
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Device not found", NULL);
  }
  return SOAP_OK;
} // AuthorizeAndGetDevice

int AuthorizeAndGetModulator(struct soap *soap, const int _token, const unsigned long _modulatorDSID, dss::Modulator& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    result = apt.GetModulatorByDSID(_modulatorDSID);
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Modulator not found", NULL);
  }
  return SOAP_OK;
} // AuthorizeAndGetModulator


int AuthorizeAndGetGroup(struct soap *soap, const int _token, const int _groupID, dss::Group& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    result = apt.GetGroup(_groupID);
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Group not found", NULL);
  }
  return SOAP_OK;
 } // AuthorizeAndGetGroup

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

int dss__FreeSet(struct soap *soap, int _token, int _setID, bool& result) {
  dss::Set set;
  int res = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(res != SOAP_OK) {
    return res;
  }

  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  sess.FreeSet(_setID);
  result = true;
  return SOAP_OK;
} // dss__FreeSet

int dss__Apartment_CreateSetFromGroup(struct soap *soap, int _token,  char* _groupName, int& setID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    sess.AddSet(apt.GetDevices().GetByGroup(_groupName), setID);
  } catch(dss::ItemNotFoundException& _ex) {
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
      dss::Device& dev = apt.GetDeviceByDSID(_ids.__ptr[iID]);
      set.AddDevice(dev);
    }
    sess.AddSet(set, setID);
  } catch(dss::ItemNotFoundException& _ex) {
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
  } catch(dss::ItemNotFoundException& _ex) {
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
    deviceID = apt.GetDevices().GetByName(_deviceName).GetDSID();
  } catch(dss::ItemNotFoundException& _ex) {
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
  } catch(dss::ItemNotFoundException& _ex) {
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
    dss::DeviceReference devRef = apt.GetDevices().GetByBusID(_deviceID);
    origSet.AddDevice(devRef);
    result = true;
  } catch(dss::ItemNotFoundException& _ex) {
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
    dss::DeviceReference devRef = apt.GetDevices().GetByBusID(_deviceID);
    origSet.RemoveDevice(devRef);
    result = true;
  } catch(dss::ItemNotFoundException& _ex) {
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
  } catch(dss::ItemNotFoundException& _ex) {
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
  } catch(dss::ItemNotFoundException& _ex) {
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
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
} // dss__Set_ByGroup

int dss__Set_GetContainedDevices(struct soap* soap, int _token, int _setID, IntArray& deviceIDs) {
  dss::Set set;
  int res = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(res != SOAP_OK) {
    return res;
  }

  int numDevices = set.Length();
  deviceIDs.__size = numDevices;
  deviceIDs.__ptr = (long unsigned int*)soap_malloc(soap, numDevices * sizeof(long unsigned int));
  for(int iDeviceID = 0; iDeviceID < numDevices; iDeviceID++) {
    deviceIDs.__ptr[iDeviceID] = set.Get(iDeviceID).GetDSID();
  }

  return SOAP_OK;
} // dss__Set_GetContainerDevices

//==================================================== Apartment

int dss__Apartment_GetGroupByName(struct soap *soap, int _token, char* _groupName, int& groupID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    groupID = apt.GetGroup(_groupName).GetID();
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find group", NULL);
  }
  return SOAP_OK;
}

int dss__Apartment_GetZoneByName(struct soap *soap, int _token,  char* _zoneName, int& zoneID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    zoneID = apt.GetZone(_zoneName).GetZoneID();
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find zone", NULL);
  }
  return SOAP_OK;
}

int dss__Apartment_GetZoneIDs(struct soap *soap, int _token, IntArray& zoneIDs) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  std::vector<dss::Zone*>& zones = apt.GetZones();

  int numZones = zones.size();
  zoneIDs.__size = numZones;
  zoneIDs.__ptr = (long unsigned int*)soap_malloc(soap, numZones * sizeof(long unsigned int));
  for(int iZoneID = 0; iZoneID < numZones; iZoneID++) {
    zoneIDs.__ptr[iZoneID] = zones[iZoneID]->GetZoneID();
  }

  return SOAP_OK;
}

//==================================================== Manipulation

//--------------------------- Set

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

int dss__Set_CallScene(struct soap *soap, int _token, int _setID, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.CallScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__Set_SaveScene(struct soap *soap, int _token, int _setID, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.SaveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__Set_UndoScene(struct soap *soap, int _token, int _setID, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.UndoScene(_sceneID);
  result = true;
  return SOAP_OK;
}

//---------------------------------- Group

int dss__Group_TurnOn(struct soap *soap, int _token, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.TurnOn();
  result = true;
  return SOAP_OK;
}

int dss__Group_TurnOff(struct soap *soap, int _token, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.TurnOff();
  result = true;
  return SOAP_OK;
}

int dss__Group_IncreaseValue(struct soap *soap, int _token, int _groupID, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.IncreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__Group_DecreaseValue(struct soap *soap, int _token, int _groupID, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.DecreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__Group_Enable(struct soap *soap, int _token, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.Enable();
  result = true;
  return SOAP_OK;
}

int dss__Group_Disable(struct soap *soap, int _token, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.Disable();
  result = true;
  return SOAP_OK;
}

int dss__Group_StartDim(struct soap *soap, int _token, int _groupID, bool _directionUp, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.StartDim(_directionUp, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__Group_EndDim(struct soap *soap, int _token, int _groupID, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.EndDim(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__Group_SetValue(struct soap *soap, int _token, int _groupID, double _value, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.SetValue(_value, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__Group_CallScene(struct soap *soap, int _token, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.CallScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__Group_SaveScene(struct soap *soap, int _token, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.SaveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__Group_UndoScene(struct soap *soap, int _token, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.UndoScene(_sceneID);
  result = true;
  return SOAP_OK;
}

//---------------------------------- Device

int dss__Device_TurnOn(struct soap *soap, int _token, unsigned long _deviceID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.TurnOn();
  result = true;
  return SOAP_OK;
}

int dss__Device_TurnOff(struct soap *soap, int _token, unsigned long _deviceID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.TurnOff();
  result = true;
  return SOAP_OK;
}

int dss__Device_IncreaseValue(struct soap *soap, int _token, unsigned long _deviceID, int _paramID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.IncreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__Device_DecreaseValue(struct soap *soap, int _token, unsigned long _deviceID, int _paramID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.DecreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__Device_Enable(struct soap *soap, int _token, unsigned long _deviceID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.Enable();
  result = true;
  return SOAP_OK;
}

int dss__Device_Disable(struct soap *soap, int _token, unsigned long _deviceID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.Disable();
  result = true;
  return SOAP_OK;
}

int dss__Device_StartDim(struct soap *soap, int _token, unsigned long _deviceID, bool _directionUp, int _paramID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.StartDim(_directionUp, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__Device_EndDim(struct soap *soap, int _token, unsigned long _deviceID, int _paramID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.EndDim(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__Device_SetValue(struct soap *soap, int _token, unsigned long _deviceID, double _value, int _paramID, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.SetValue(_value, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__Device_CallScene(struct soap *soap, int _token, int _deviceID, int _sceneID, bool& result) {
  dss::Device device(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.CallScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__Device_SaveScene(struct soap *soap, int _token, int _deviceID, int _sceneID, bool& result) {
  dss::Device device(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.SaveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__Device_UndoScene(struct soap *soap, int _token, int _deviceID, int _sceneID, bool& result) {
  dss::Device device(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.UndoScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__Device_GetValue(struct soap *soap, int _token, unsigned long _deviceID, int _paramID, double& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  result = dev.GetValue(_paramID);
  return SOAP_OK;
}

int dss__Device_GetName(struct soap *soap, int _token, unsigned long _deviceID, char** result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  const char* devname = dev.GetName().c_str();
  char* resMem = (char*)soap_malloc(soap, dev.GetName().size()+1);
  strcpy(resMem, devname);
  result[0] = resMem;
  return SOAP_OK;
} // dss__Device_GetName

int dss__Device_GetFunctionID(struct soap *soap, int _token, unsigned long _deviceID, int& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
	return getResult;
  }
  result = dev.GetFunctionID();
  return SOAP_OK;
} // dss__Device_GetFunctionID

int dss__Switch_GetGroupID(struct soap *soap, int _token, unsigned long _deviceID, int& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dss::DSIDInterface* simDev = dss::DSS::GetInstance()->GetModulatorSim().GetSimulatedDevice(_deviceID);
  dss::DSIDSimSwitch* sw = NULL;
  if(simDev != NULL && (sw = dynamic_cast<dss::DSIDSimSwitch*>(simDev)) != NULL) {
  	result = dss::DSS::GetInstance()->GetModulatorSim().GetGroupForSwitch(sw);
  } else {
    for(int iGroup = 1; iGroup < 9; iGroup++) {
      if(dev.IsInGroup(iGroup)) {
        result = iGroup;
        break;
      }
    }
  }
  return SOAP_OK;
} // dss__Switch_GetGroupID

int dss__Switch_SimulateKeypress(struct soap *soap, int _token, unsigned long _deviceID, int _buttonNr, char* _kind, bool& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dss::DSIDInterface* simDev = dss::DSS::GetInstance()->GetModulatorSim().GetSimulatedDevice(_deviceID);
  dss::DSIDSimSwitch* sw = NULL;
  if(simDev != NULL && (sw = dynamic_cast<dss::DSIDSimSwitch*>(simDev)) != NULL) {
    dss::ButtonPressKind kind = dss::Click;
    if(string(_kind) == "touch") {
      kind = dss::Touch;
    } else if(string(_kind) == "touchend") {
      kind = dss::TouchEnd;
    }
    dss::DSS::GetInstance()->GetModulatorSim().ProcessButtonPress(*sw, _buttonNr, kind);
    return SOAP_OK;
  }
  return soap_sender_fault(soap, "Could not find switch", NULL);
} // dss__Switch_SimulateKeypress

int dss__Device_GetZoneID(struct soap *soap, int _token, unsigned long _deviceID, int& result) {
  dss::Device dev(-1, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  result = dev.GetZoneID();
  return SOAP_OK;
} // dss__Device_GetZoneID

//==================================================== Information

int dss__Device_GetDSID(struct soap *soap, int _token, unsigned long _deviceID, unsigned long& result) {
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
  ids.__ptr = (long unsigned int*)soap_malloc(soap, sizeof(long unsigned int) * modulators.size());
  ids.__size = modulators.size();

  for(int iModulator = 0; iModulator < ids.__size; iModulator++) {
    ids.__ptr[iModulator] = modulators[iModulator]->GetDSID();
  }

  return SOAP_OK;
}

int dss__Modulator_GetDSID(struct soap *soap, int _token, int _modulatorID, unsigned long& dsid) {
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

int dss__Apartment_AllocateZone(struct soap *soap, int _token, int& zoneID) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Apartment_DeleteZone(struct soap *soap, int _token, int _zoneID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Zone_AddDevice(struct soap *soap, int _token, int _zoneID, int _deviceID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Zone_RemoveDevice(struct soap *soap, int _token, int _zoneID, int _deviceID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Zone_SetName(struct soap *soap, int _token, int _zoneID, char* _name, int& result) {
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

int dss__Event_Raise(struct soap *soap, int _token, int _eventID, int _sourceID, dss__inParameter _params, int& result) {
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


  //dss::Event evt(_eventID, _sourceID);
  //dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
 // apt.OnEvent();
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Event_GetActionNames(struct soap *soap, int _token,  StringArray& names) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Event_GetActionParamsTemplate(struct soap *soap, int _token,  char* _name, dss__outParameter& paramsTemplate) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Event_Subscribe(struct soap *soap, int _token, IntArray _eventIDs, IntArray _sourceIDs, char* _actionName, dss__inParameter _params, int& subscriptionID) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Event_Unsubscribe(struct soap *soap, int _token, int _subscriptionID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Event_Schedule(struct soap *soap, int _token,  char* _icalString, int _eventID, dss__inParameter _params, int& scheduledEventID) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Event_DeleteSchedule(struct soap *soap, int _token, int _scheduleEventID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

