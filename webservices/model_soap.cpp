#include "soapH.h"
#include "core/dss.h"
#include "core/logger.h"
#include "core/event.h"
#include "webservices/webservices.h"
#include "core/sim/dssim.h"
#include "core/propertysystem.h"

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

int dss__Test(struct soap *soap, char*, std::vector<int>& res) {
  res.push_back(1);
  res.push_back(2);
  return SOAP_OK;
}

inline dss::dsid_t FromSOAP(const char* _dsid) {
  dss::dsid_t result = dss::dsid_t::FromString(_dsid);
  return result;
}


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

int AuthorizeAndGetDevice(struct soap *soap, const int _token, char* _devID, dss::DeviceReference& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  result = dss::DeviceReference(FromSOAP(_devID), apt);
  try {
    result.GetDevice();
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Device not found", NULL);
  }
  return SOAP_OK;
} // AuthorizeAndGetDevice

int AuthorizeAndGetModulator(struct soap *soap, const int _token, char* _modulatorDSID, dss::Modulator& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    result = apt.GetModulatorByDSID(FromSOAP(_modulatorDSID));
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Modulator not found", NULL);
  }
  return SOAP_OK;
} // AuthorizeAndGetModulator

int AuthorizeAndGetModulatorByBusID(struct soap *soap, const int _token, int _modulatorID, dss::Modulator& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    result = apt.GetModulatorByBusID(_modulatorID);
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Modulator not found", NULL);
  }
  return SOAP_OK;
} // AuthorizeAndGetModulatorID

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

int AuthorizeAndGetGroupOfZone(struct soap *soap, const int _token, const int _zoneID, const int _groupID, dss::Group& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    dss::Group* pResult = apt.GetZone(_zoneID).GetGroup(_groupID);
    if(pResult == NULL) {
      return soap_receiver_fault(soap, "Group not found", NULL);
    }
    result = *pResult;
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Zone not found", NULL);
  }
  return SOAP_OK;
 } // AuthorizeAndGetGroupOfZone


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

int dss__ApartmentCreateSetFromGroup(struct soap *soap, int _token,  char* _groupName, int& setID) {
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

int dss__ApartmentCreateSetFromDeviceIDs(struct soap *soap, int _token, std::vector<std::string> _ids, int& setID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    dss::Set set;
    for(unsigned int iID = 0; iID < _ids.size(); iID++) {
      dss::Device& dev = apt.GetDeviceByDSID(FromSOAP(_ids[iID].c_str()));
      set.AddDevice(dev);
    }
    sess.AddSet(set, setID);
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Unknown device", NULL);
  }
  return SOAP_OK;
}

int dss__ApartmentCreateSetFromDeviceNames(struct soap *soap, int _token,  std::vector<std::string> _names, int& setID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    dss::Set set;
    for(unsigned int iName = 0; iName < _names.size(); iName++) {
      dss::Device& dev = apt.GetDeviceByName(_names[iName]);
      set.AddDevice(dev);
    }
    sess.AddSet(set, setID);
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Unknown device", NULL);
  }
  return SOAP_OK;
}

int dss__ApartmentCreateEmptySet(struct soap *soap, int _token, int& setID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token).AllocateSet(setID);
  return SOAP_OK;
}

int dss__ApartmentGetDevices(struct soap *soap, int _token, int& setID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::WebServiceSession& sess = dss::DSS::GetInstance()->GetWebServices().GetSession(soap, _token);
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  sess.AddSet(apt.GetDevices(), setID);
  return SOAP_OK;
}

int dss__ApartmentGetDeviceIDByName(struct soap *soap, int _token,  char* _deviceName, std::string& deviceID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  try {
    dss::dsid_t dsid = apt.GetDevices().GetByName(_deviceName).GetDSID();
    string asString = dsid.ToString();
    deviceID = soap_strdup(soap, asString.c_str());
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
}

int dss__SetAddDeviceByName(struct soap *soap, int _token, int _setID, char* _name, bool& result) {
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

int dss__SetAddDeviceByID(struct soap *soap, int _token, int _setID, char* _deviceID, bool& result) {
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
    dss::DeviceReference devRef = apt.GetDevices().GetByDSID(FromSOAP(_deviceID));
    origSet.AddDevice(devRef);
    result = true;
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
}

int dss__SetRemoveDevice(struct soap *soap, int _token, int _setID, char* _deviceID, bool& result) {
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
    dss::DeviceReference devRef = apt.GetDevices().GetByDSID(FromSOAP(_deviceID));
    origSet.RemoveDevice(devRef);
    result = true;
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
}

int dss__SetCombine(struct soap *soap, int _token, int _setID1, int _setID2, int& setID) {
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

int dss__SetRemove(struct soap *soap, int _token, int _setID, int _setIDToRemove, int& setID) {
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

int dss__SetByGroup(struct soap *soap, int _token, int _setID, int _groupID, int& setID) {
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
} // dss__SetByGroup

int dss__SetGetContainedDevices(struct soap* soap, int _token, int _setID, std::vector<std::string>& deviceIDs) {
  dss::Set set;
  int res = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(res != SOAP_OK) {
    return res;
  }

  int numDevices = set.Length();
  for(int iDeviceID = 0; iDeviceID < numDevices; iDeviceID++) {
    dss::dsid_t dsid = set.Get(iDeviceID).GetDSID();
    deviceIDs.push_back(dsid.ToString());
  }

  return SOAP_OK;
} // dss__SetGetContainerDevices

//==================================================== Apartment

int dss__ApartmentGetGroupByName(struct soap *soap, int _token, char* _groupName, int& groupID) {
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

int dss__ApartmentGetZoneByName(struct soap *soap, int _token,  char* _zoneName, int& zoneID) {
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

int dss__ApartmentGetZoneIDs(struct soap *soap, int _token, std::vector<int>& zoneIDs) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();
  std::vector<dss::Zone*>& zones = apt.GetZones();

  int numZones = zones.size();
//  zoneIDs.__size = numZones;
  //zoneIDs.__ptr = (long unsigned int*)soap_malloc(soap, numZones * sizeof(long unsigned int));
  for(int iZoneID = 0; iZoneID < numZones; iZoneID++) {
    zoneIDs.push_back(zones[iZoneID]->GetZoneID());
  }

  return SOAP_OK;
}


//==================================================== Manipulation

//--------------------------- Set

int dss__SetTurnOn(struct soap *soap, int _token, int _setID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.TurnOn();
  result = true;
  return SOAP_OK;
}

int dss__SetTurnOff(struct soap *soap, int _token, int _setID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.TurnOff();
  result = true;
  return SOAP_OK;
}

int dss__SetIncreaseValue(struct soap *soap, int _token, int _setID, int _paramID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.IncreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__SetDecreaseValue(struct soap *soap, int _token, int _setID, int _paramID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.DecreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__SetEnable(struct soap *soap, int _token, int _setID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.Enable();
  result = true;
  return SOAP_OK;
}

int dss__SetDisable(struct soap *soap, int _token, int _setID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.Disable();
  result = true;
  return SOAP_OK;
}

int dss__SetStartDim(struct soap *soap, int _token, int _setID, bool _directionUp, int _paramID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.StartDim(_directionUp, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__SetEndDim(struct soap *soap, int _token, int _setID, int _paramID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.EndDim(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__SetSetValue(struct soap *soap, int _token, int _setID, double _value, int _paramID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.SetValue(_value, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__SetCallScene(struct soap *soap, int _token, int _setID, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.CallScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__SetSaveScene(struct soap *soap, int _token, int _setID, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.SaveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__SetUndoScene(struct soap *soap, int _token, int _setID, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setID, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.UndoScene(_sceneID);
  result = true;
  return SOAP_OK;
}

//---------------------------------- Apartment

int dss__ApartmentTurnOn(struct soap *soap, int _token, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.TurnOn();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentTurnOff(struct soap *soap, int _token, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.TurnOff();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentIncreaseValue(struct soap *soap, int _token, int _groupID, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.IncreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentDecreaseValue(struct soap *soap, int _token, int _groupID, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.DecreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentEnable(struct soap *soap, int _token, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.Enable();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentDisable(struct soap *soap, int _token, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.Disable();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentStartDim(struct soap *soap, int _token, int _groupID, bool _directionUp, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.StartDim(_directionUp, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentEndDim(struct soap *soap, int _token, int _groupID, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.EndDim(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentSetValue(struct soap *soap, int _token, int _groupID, double _value, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.SetValue(_value, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentCallScene(struct soap *soap, int _token, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.CallScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentSaveScene(struct soap *soap, int _token, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.SaveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentUndoScene(struct soap *soap, int _token, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.UndoScene(_sceneID);
  result = true;
  return SOAP_OK;
}

//---------------------------------- Apartment

int dss__ZoneTurnOn(struct soap *soap, int _token, int _zoneID, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.TurnOn();
  result = true;
  return SOAP_OK;
}

int dss__ZoneTurnOff(struct soap *soap, int _token, int _zoneID, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.TurnOff();
  result = true;
  return SOAP_OK;
}

int dss__ZoneIncreaseValue(struct soap *soap, int _token, int _zoneID, int _groupID, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.IncreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneDecreaseValue(struct soap *soap, int _token, int _zoneID, int _groupID, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.DecreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneEnable(struct soap *soap, int _token, int _zoneID, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.Enable();
  result = true;
  return SOAP_OK;
}

int dss__ZoneDisable(struct soap *soap, int _token, int _zoneID, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.Disable();
  result = true;
  return SOAP_OK;
}

int dss__ZoneStartDim(struct soap *soap, int _token, int _zoneID, int _groupID, bool _directionUp, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.StartDim(_directionUp, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneEndDim(struct soap *soap, int _token, int _zoneID, int _groupID, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.EndDim(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneSetValue(struct soap *soap, int _token, int _zoneID, int _groupID, double _value, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.SetValue(_value, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneCallScene(struct soap *soap, int _token, int _zoneID, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.CallScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneSaveScene(struct soap *soap, int _token, int _zoneID, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.SaveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneUndoScene(struct soap *soap, int _token, int _zoneID, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::GetInstance()->GetApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.UndoScene(_sceneID);
  result = true;
  return SOAP_OK;
}

//---------------------------------- Device

int dss__DeviceTurnOn(struct soap *soap, int _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.TurnOn();
  result = true;
  return SOAP_OK;
}

int dss__DeviceTurnOff(struct soap *soap, int _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.TurnOff();
  result = true;
  return SOAP_OK;
}

int dss__DeviceIncreaseValue(struct soap *soap, int _token, char* _deviceID, int _paramID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.IncreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceDecreaseValue(struct soap *soap, int _token, char* _deviceID, int _paramID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.DecreaseValue(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceEnable(struct soap *soap, int _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.Enable();
  result = true;
  return SOAP_OK;
}

int dss__DeviceDisable(struct soap *soap, int _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.Disable();
  result = true;
  return SOAP_OK;
}

int dss__DeviceStartDim(struct soap *soap, int _token, char* _deviceID, bool _directionUp, int _paramID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.StartDim(_directionUp, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceEndDim(struct soap *soap, int _token, char* _deviceID, int _paramID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.EndDim(_paramID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceSetValue(struct soap *soap, int _token, char* _deviceID, double _value, int _paramID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.SetValue(_value, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceCallScene(struct soap *soap, int _token, char* _deviceID, int _sceneID, bool& result) {
  dss::DeviceReference device(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.CallScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceSaveScene(struct soap *soap, int _token, char* _deviceID, int _sceneID, bool& result) {
  dss::DeviceReference device(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.SaveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceUndoScene(struct soap *soap, int _token, char* _deviceID, int _sceneID, bool& result) {
  dss::DeviceReference device(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.UndoScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceGetValue(struct soap *soap, int _token, char* _deviceID, int _paramID, double& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  result = dev.GetDevice().GetValue(_paramID);
  return SOAP_OK;
}

int dss__DeviceGetName(struct soap *soap, int _token, char* _deviceID, char** result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  *result = soap_strdup(soap, dev.GetName().c_str());
  return SOAP_OK;
} // dss__DeviceGetName

int dss__DeviceGetFunctionID(struct soap *soap, int _token, char* _deviceID, int& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
	return getResult;
  }
  result = dev.GetDevice().GetFunctionID();
  return SOAP_OK;
} // dss__DeviceGetFunctionID

int dss__SwitchGetGroupID(struct soap *soap, int _token, char* _deviceID, int& result) {
 /*
  dss::DeviceReference devRef(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, devRef);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dss::DSIDInterface* simDev = dss::DSS::GetInstance()->GetSimulation().GetSimulatedDevice(FromSOAP(_deviceID));
  dss::DSIDSimSwitch* sw = NULL;
  if(simDev != NULL && (sw = dynamic_cast<dss::DSIDSimSwitch*>(simDev)) != NULL) {
  	result = dss::DSS::GetInstance()->GetSimulation().GetGroupForSwitch(sw);
  } else {
    dss::Device& dev = devRef.GetDevice();
    for(int iGroup = 1; iGroup < 9; iGroup++) {
      if(dev.IsInGroup(iGroup)) {
        result = iGroup;
        break;
      }
    }
  }
  */
  return SOAP_OK;
} // dss__SwitchGetGroupID

int dss__SwitchSimulateKeypress(struct soap *soap, int _token, char* _deviceID, int _buttonNr, char* _kind, bool& result) {
/*
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dss::DSIDInterface* simDev = dss::DSS::GetInstance()->GetSimulation().GetSimulatedDevice(FromSOAP(_deviceID));
  dss::DSIDSimSwitch* sw = NULL;
  if(simDev != NULL && (sw = dynamic_cast<dss::DSIDSimSwitch*>(simDev)) != NULL) {
    dss::ButtonPressKind kind = dss::Click;
    if(string(_kind) == "touch") {
      kind = dss::Touch;
    } else if(string(_kind) == "touchend") {
      kind = dss::TouchEnd;
    }
    dss::DSS::GetInstance()->GetSimulation().ProcessButtonPress(*sw, _buttonNr, kind);
    return SOAP_OK;
  }
  */
  return soap_sender_fault(soap, "Could not find switch", NULL);
} // dss__SwitchSimulateKeypress

int dss__DeviceGetZoneID(struct soap *soap, int _token, char* _deviceID, int& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  result = dev.GetDevice().GetZoneID();
  return SOAP_OK;
} // dss__DeviceGetZoneID

//==================================================== Information

int dss__ModulatorGetPowerConsumption(struct soap *soap, int _token, int _modulatorID, unsigned long& result) {
  dss::Modulator mod(dss::NullDSID);
  int getResult = AuthorizeAndGetModulatorByBusID(soap, _token, _modulatorID, mod);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  result = mod.GetPowerConsumption();
  return SOAP_OK;
} // dss__ModulatorGetPowerConsumption


//==================================================== Organization

//These calls may be restricted to privileged users.

int dss__ApartmentGetModulatorIDs(struct soap *soap, int _token, std::vector<std::string>& ids) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::GetInstance()->GetApartment();

  std::vector<dss::Modulator*>& modulators = apt.GetModulators();

  for(unsigned int iModulator = 0; iModulator < modulators.size(); iModulator++) {
    dss::dsid_t dsid = modulators[iModulator]->GetDSID();
    ids.push_back(dsid.ToString());
  }

  return SOAP_OK;
}

int dss__ModulatorGetName(struct soap *soap, int _token, char* _modulatorID, std::string& name) {
  dss::Modulator mod(dss::NullDSID);
  int getResult = AuthorizeAndGetModulator(soap, _token, _modulatorID, mod);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  name = mod.GetName();
  return SOAP_OK;
}

int dss__ApartmentAllocateZone(struct soap *soap, int _token, int& zoneID) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__ApartmentDeleteZone(struct soap *soap, int _token, int _zoneID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Zone_AddDevice(struct soap *soap, int _token, int _zoneID, char* _deviceID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Zone_RemoveDevice(struct soap *soap, int _token, int _zoneID, char* _deviceID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__Zone_SetName(struct soap *soap, int _token, int _zoneID, char* _name, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__ApartmentAllocateUserGroup(struct soap *soap, int _token, int& groupID) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__GroupRemoveUserGroup(struct soap *soap, int _token, int _groupID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__GroupAddDevice(struct soap *soap, int _token, int _groupID, char* _deviceID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__GroupRemoveDevice(struct soap *soap, int _token, int _groupID, char* _deviceID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

//==================================================== Events

int dss__EventRaise(struct soap *soap, int _token, char* _eventName, char* _context, char* _parameter, char* _location, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    result = false;
    return NotAuthorized(soap);
  }

  if(_eventName != NULL && strlen(_eventName) > 0) {
    boost::shared_ptr<dss::Event> evt(new dss::Event(_eventName));
    if(_location != NULL && strlen(_location) > 0) {
      evt->SetLocation(_location);
    }
    if(_context != NULL && strlen(_context) > 0) {
      evt->SetContext(_context);
    }
    if(_parameter != NULL && strlen(_parameter) > 0) {
      vector<string> params = dss::SplitString(_parameter, ';');
      for(vector<string>::iterator iParam = params.begin(), e = params.end();
          iParam != e; ++iParam)
      {
        vector<string> nameValue = dss::SplitString(*iParam, '=');
        if(nameValue.size() == 2) {
          dss::Logger::GetInstance()->Log("SOAP::EventRaise: Got parameter '" + nameValue[0] + "'='" + nameValue[1] + "'");
          evt->SetProperty(nameValue[0], nameValue[1]);
        } else {
          dss::Logger::GetInstance()->Log(string("Invalid parameter found SOAP::EventRaise: ") + *iParam );
        }
      }
    }
    dss::DSS::GetInstance()->GetEventQueue().PushEvent(evt);
  } else {
    return soap_sender_fault(soap, "eventname must be provided", NULL);
  }

  result = true;
  return SOAP_OK;
} // dss__EventRaise

//==================================================== Properties

int dss__PropertyGetType(struct soap *soap, int _token, std::string _propertyName, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::GetInstance()->GetPropertySystem();
  dss::PropertyNode* node = propSys.GetProperty(_propertyName);
  if(node == NULL) {
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  result = dss::GetValueTypeAsString(node->GetValueType());
  return SOAP_OK;
} // dss__PropertyGetType

int dss__PropertySetInt(struct soap *soap, int _token, std::string _propertyName, int _value, bool _mayCreate, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    result = false;
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::GetInstance()->GetPropertySystem();
  if(!propSys.SetIntValue(_propertyName, _value, _mayCreate)) {
    result = false;
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  result = true;
  return SOAP_OK;
} // dss__PropertySetInt

int dss__PropertySetString(struct soap *soap, int _token, std::string _propertyName, char* _value, bool _mayCreate, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    result = false;
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::GetInstance()->GetPropertySystem();
  if(!propSys.SetStringValue(_propertyName, _value, _mayCreate)) {
    result = false;
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  result = true;
  return SOAP_OK;
} // dss__PropertySetString

int dss__PropertySetBool(struct soap *soap, int _token, std::string _propertyName, bool _value, bool _mayCreate, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    result = false;
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::GetInstance()->GetPropertySystem();
  if(!propSys.SetBoolValue(_propertyName, _value, _mayCreate)) {
    result = false;
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  result = true;
  return SOAP_OK;
} // dss__PropertySetBool

int dss__PropertyGetInt(struct soap *soap, int _token, std::string _propertyName, int& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::GetInstance()->GetPropertySystem();
  dss::PropertyNode* node = propSys.GetProperty(_propertyName);
  if(node == NULL) {
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  result = node->GetIntegerValue();
  return SOAP_OK;
} // dss__PropertyGetInt

int dss__PropertyGetString(struct soap *soap, int _token, std::string _propertyName, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::GetInstance()->GetPropertySystem();
  dss::PropertyNode* node = propSys.GetProperty(_propertyName);
  if(node == NULL) {
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  result = node->GetStringValue();
  return SOAP_OK;
} // dss__PropertyGetString

int dss__PropertyGetBool(struct soap *soap, int _token, std::string _propertyName, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::GetInstance()->GetPropertySystem();
  dss::PropertyNode* node = propSys.GetProperty(_propertyName);
  if(node == NULL) {
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  result = node->GetBoolValue();
  return SOAP_OK;
} // dss__PropertyGetBool

int dss__PropertyGetChildren(struct soap *soap, int _token, std::string _propertyName, std::vector<std::string>& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::GetInstance()->GetPropertySystem();
  dss::PropertyNode* node = propSys.GetProperty(_propertyName);
  if(node == NULL) {
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  for(int iChild = 0; iChild < node->GetChildCount(); iChild++) {
    result.push_back(node->GetChild(iChild)->GetName());
  }

  return SOAP_OK;
} // dss__PropertyGetChildren

