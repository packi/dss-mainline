#include "soapH.h"
#include "core/dss.h"
#include "core/logger.h"
#include "core/event.h"
#include "webservices/webservices.h"
#include "core/sim/dssim.h"
#include "core/propertysystem.h"
#include "core/setbuilder.h"
#include "core/foreach.h"

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

inline dss::dsid_t FromSOAP(const char* _dsid) {
  dss::dsid_t result = dss::dsid_t::fromString(_dsid);
  return result;
}


//==================================================== Helpers

int NotAuthorized(struct soap *soap) {
  return soap_receiver_fault(soap, "Not Authorized", NULL);
} // notAuthorized

bool IsAuthorized(struct soap *soap, const int _token) {
  bool result = dss::DSS::getInstance()->getWebServices().isAuthorized(soap, _token);
  if(result) {
    dss::Logger::getInstance()->log(std::string("User with token '") + dss::intToString(_token) + "' is authorized");
  } else {
    dss::Logger::getInstance()->log(std::string("User with token '") + dss::intToString(_token) + "' is *not* authorized", lsWarning);
  }
  return result;
} // isAuthorized

int AuthorizeAndGetSession(struct soap *soap, const int _token, dss::WebServiceSession** result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  *result = &dss::DSS::getInstance()->getWebServices().getSession(soap, _token);
  return SOAP_OK;
} // AuthorizeAndGetSession

int AuthorizeAndGetSet(struct soap *soap, const int _token, const char* _setSpec, dss::Set& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  dss::SetBuilder builder(apt);
  result = builder.buildSet(_setSpec, NULL);
  return SOAP_OK;
} // authorizeAndGetSet

int AuthorizeAndGetDevice(struct soap *soap, const int _token, char* _devID, dss::DeviceReference& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  result = dss::DeviceReference(FromSOAP(_devID), &apt);
  try {
    result.getDevice();
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Device not found", NULL);
  }
  return SOAP_OK;
} // authorizeAndGetDevice

int AuthorizeAndGetModulator(struct soap *soap, const int _token, char* _modulatorDSID, dss::Modulator& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    result = apt.getModulatorByDSID(FromSOAP(_modulatorDSID));
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Modulator not found", NULL);
  }
  return SOAP_OK;
} // authorizeAndGetModulator

int AuthorizeAndGetModulatorByBusID(struct soap *soap, const int _token, int _modulatorID, dss::Modulator& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    result = apt.getModulatorByBusID(_modulatorID);
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Modulator not found", NULL);
  }
  return SOAP_OK;
} // authorizeAndGetModulatorID

int AuthorizeAndGetGroup(struct soap *soap, const int _token, const int _groupID, dss::Group& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    result = apt.getGroup(_groupID);
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Group not found", NULL);
  }
  return SOAP_OK;
 } // authorizeAndGetGroup

int AuthorizeAndGetGroupOfZone(struct soap *soap, const int _token, const int _zoneID, const int _groupID, dss::Group& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    dss::Group* pResult = apt.getZone(_zoneID).getGroup(_groupID);
    if(pResult == NULL) {
      return soap_receiver_fault(soap, "Group not found", NULL);
    }
    result = *pResult;
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Zone not found", NULL);
  }
  return SOAP_OK;
 } // authorizeAndGetGroupOfZone


//==================================================== Callbacks

int dss__Authenticate(struct soap *soap, char* _userName, char* _password, int& token) {
  dss::DSS::getInstance()->getWebServices().newSession(soap, token);
  return SOAP_OK;
} // dss__Authenticate

int dss__SignOff(struct soap *soap, int _token, int& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::DSS::getInstance()->getWebServices().deleteSession(soap, _token);
  return SOAP_OK;
} // dss__SignOff

int dss__ApartmentCreateSetFromGroup(struct soap *soap, int _token, char* _groupName, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    apt.getDevices().getByGroup(_groupName); // check that the group exists
    result = std::string(".group('") + _groupName + "')";
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Unknown group", NULL);
  }
  return SOAP_OK;
} // dss__ApartmentCreateSetFromGroup

int dss__ApartmentCreateSetFromDeviceIDs(struct soap *soap, int _token, std::vector<std::string> _ids, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    result = "addDevices(";
    std::vector<std::string> dsids;
    for(unsigned int iID = 0; iID < _ids.size(); iID++) {
      dss::Device& dev = apt.getDeviceByDSID(FromSOAP(_ids[iID].c_str()));
      dsids.push_back(dev.getDSID().toString());
    }
    result += dss::join(dsids, ",");
    result += ")";
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Unknown device", NULL);
  }
  return SOAP_OK;
} // dss__ApartmentCreateSetFromDeviceIDs

int dss__ApartmentCreateSetFromDeviceNames(struct soap *soap, int _token,  std::vector<std::string> _names, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    dss::Set set;
    result = "addDevices(";
    std::vector<std::string> names;
    for(unsigned int iName = 0; iName < _names.size(); iName++) {
      dss::Device& dev = apt.getDeviceByName(_names[iName]);
      names.push_back("'" + dev.getName() + "'");
    }
    result += dss::join(names, ",");
    result += ")";
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Unknown device", NULL);
  }
  return SOAP_OK;
} // dss__ApartmentCreateSetFromDeviceNames

int dss__ApartmentGetDevices(struct soap *soap, int _token, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  result = ".";
  return SOAP_OK;
} // dss__ApartmenGetDevices

int dss__ApartmentGetDeviceIDByName(struct soap *soap, int _token,  char* _deviceName, std::string& deviceID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    dss::dsid_t dsid = apt.getDevices().getByName(_deviceName).getDSID();
    std::string asString = dsid.toString();
    deviceID = soap_strdup(soap, asString.c_str());
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
}

//==================================================== Set

int dss__SetAddDeviceByName(struct soap *soap, int _token, char* _setSpec, char* _name, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    dss::DeviceReference devRef = apt.getDevices().getByName(_name);
    result = _setSpec;
    if(!result.empty()) {
      result += ".";
    }
    result += "addDevices('" + std::string(_name) + "')";
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
} // dss__SetAddDeviceByName

int dss__SetAddDeviceByID(struct soap *soap, int _token, char* _setSpec, char* _deviceID, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    dss::DeviceReference devRef = apt.getDevices().getByDSID(FromSOAP(_deviceID));
    result = _setSpec;
    if(!result.empty()) {
      result += ".";
    }
    result += "dsid(" + devRef.getDSID().toString() + ")";
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
} // dss__SetAddDeviceByID

int dss__SetRemoveDevice(struct soap *soap, int _token, char* _setSpec, char* _deviceID, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    dss::DeviceReference devRef = apt.getDevices().getByDSID(FromSOAP(_deviceID));
    result = _setSpec;
    result += ".remove(.dsid(" + devRef.getDSID().toString() + "))";
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
} // dss__SetRemoveDevice

int dss__SetCombine(struct soap *soap, int _token, char* _setSpec1, char* _setSpec2, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  result = _setSpec1;
  result += ".combine(";
  result += _setSpec2;
  result += ")";
  return SOAP_OK;
} // dss_SetCombine

int dss__SetRemove(struct soap *soap, int _token, char* _setSpec, char* _setSpecToRemove, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  result = _setSpec;
  result += ".remove(";
  result += _setSpecToRemove;
  result += ")";
  return SOAP_OK;
} // dss__SetRemove

int dss__SetByGroup(struct soap *soap, int _token, char* _setSpec, int _groupID, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  result = _setSpec;
  result += ".group(" + dss::intToString(_groupID) + ")";
  return SOAP_OK;
} // dss__SetByGroup

int dss__SetGetContainedDevices(struct soap* soap, int _token, char* _setSpec, std::vector<std::string>& deviceIDs) {
  dss::Set set;
  int res = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(res != SOAP_OK) {
    return res;
  }

  int numDevices = set.length();
  for(int iDeviceID = 0; iDeviceID < numDevices; iDeviceID++) {
    dss::dsid_t dsid = set.get(iDeviceID).getDSID();
    deviceIDs.push_back(dsid.toString());
  }

  return SOAP_OK;
} // dss__SetGetContainerDevices

//==================================================== Apartment

int dss__ApartmentGetGroupByName(struct soap *soap, int _token, char* _groupName, int& groupID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    groupID = apt.getGroup(_groupName).getID();
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find group", NULL);
  }
  return SOAP_OK;
}

int dss__ApartmentGetZoneByName(struct soap *soap, int _token,  char* _zoneName, int& zoneID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    zoneID = apt.getZone(_zoneName).getID();
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find zone", NULL);
  }
  return SOAP_OK;
}

int dss__ApartmentGetZoneIDs(struct soap *soap, int _token, std::vector<int>& zoneIDs) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  std::vector<dss::Zone*>& zones = apt.getZones();

  int numZones = zones.size();
//  zoneIDs.__size = numZones;
  //zoneIDs.__ptr = (long unsigned int*)soap_malloc(soap, numZones * sizeof(long unsigned int));
  for(int iZoneID = 0; iZoneID < numZones; iZoneID++) {
    zoneIDs.push_back(zones[iZoneID]->getID());
  }

  return SOAP_OK;
} // dss__ApartmentGetZoneIDs

int dss__ApartmentRescan(struct soap *soap, int _token, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  std::vector<dss::Modulator*> mods = apt.getModulators();
  foreach(dss::Modulator* pModulator, mods) {
    pModulator->setIsValid(false);
  }

  return SOAP_OK;
} // dss__ApartmentRescan

//==================================================== Circuit

int dss__CircuitRescan(struct soap *soap, int _token, char* _dsid, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  dss::dsid_t dsid;
  try {
    dsid = dss::dsid_t::fromString(_dsid);
  } catch(std::invalid_argument&) {
    return soap_sender_fault(soap, "Error parsing dsid", NULL);
  }
  try {
    dss::Modulator& mod = apt.getModulatorByDSID(dsid);
    try {
      result = apt.scanModulator(mod);
    } catch(std::runtime_error&) {
      soap_receiver_fault(soap, "Error scanning bus", NULL);
    }
  } catch(dss::ItemNotFoundException&) {
    return soap_sender_fault(soap, "Could not find modulator", NULL);
  }

  return SOAP_OK;
} // dss__CircuitRescan


//==================================================== Manipulation

//--------------------------- Set

int dss__SetTurnOn(struct soap *soap, int _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.turnOn();
  result = true;
  return SOAP_OK;
}

int dss__SetTurnOff(struct soap *soap, int _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.turnOff();
  result = true;
  return SOAP_OK;
}

int dss__SetIncreaseValue(struct soap *soap, int _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.increaseValue();
  result = true;
  return SOAP_OK;
}

int dss__SetDecreaseValue(struct soap *soap, int _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.decreaseValue();
  result = true;
  return SOAP_OK;
}

int dss__SetStartDim(struct soap *soap, int _token, char* _setSpec, bool _directionUp, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.startDim(_directionUp);
  result = true;
  return SOAP_OK;
}

int dss__SetEndDim(struct soap *soap, int _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.endDim();
  result = true;
  return SOAP_OK;
}

int dss__SetSetValue(struct soap *soap, int _token, char* _setSpec, double _value, int _paramID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.setValue(_value, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__SetCallScene(struct soap *soap, int _token, char* _setSpec, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.callScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__SetSaveScene(struct soap *soap, int _token, char* _setSpec, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.saveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__SetUndoScene(struct soap *soap, int _token, char* _setSpec, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.undoScene(_sceneID);
  result = true;
  return SOAP_OK;
}

//---------------------------------- Apartment

int dss__ApartmentTurnOn(struct soap *soap, int _token, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.turnOn();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentTurnOff(struct soap *soap, int _token, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.turnOff();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentIncreaseValue(struct soap *soap, int _token, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.increaseValue();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentDecreaseValue(struct soap *soap, int _token, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.decreaseValue();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentStartDim(struct soap *soap, int _token, int _groupID, bool _directionUp, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.startDim(_directionUp);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentEndDim(struct soap *soap, int _token, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.endDim();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentSetValue(struct soap *soap, int _token, int _groupID, double _value, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.setValue(_value, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentCallScene(struct soap *soap, int _token, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.callScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentSaveScene(struct soap *soap, int _token, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.saveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentUndoScene(struct soap *soap, int _token, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.undoScene(_sceneID);
  result = true;
  return SOAP_OK;
}

//---------------------------------- Apartment

int dss__ZoneTurnOn(struct soap *soap, int _token, int _zoneID, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.turnOn();
  result = true;
  return SOAP_OK;
}

int dss__ZoneTurnOff(struct soap *soap, int _token, int _zoneID, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.turnOff();
  result = true;
  return SOAP_OK;
}

int dss__ZoneIncreaseValue(struct soap *soap, int _token, int _zoneID, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.increaseValue();
  result = true;
  return SOAP_OK;
}

int dss__ZoneDecreaseValue(struct soap *soap, int _token, int _zoneID, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.decreaseValue();
  result = true;
  return SOAP_OK;
}

int dss__ZoneStartDim(struct soap *soap, int _token, int _zoneID, int _groupID, bool _directionUp, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.startDim(_directionUp);
  result = true;
  return SOAP_OK;
}

int dss__ZoneEndDim(struct soap *soap, int _token, int _zoneID, int _groupID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.endDim();
  result = true;
  return SOAP_OK;
}

int dss__ZoneSetValue(struct soap *soap, int _token, int _zoneID, int _groupID, double _value, int _paramID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.setValue(_value, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneCallScene(struct soap *soap, int _token, int _zoneID, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.callScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneSaveScene(struct soap *soap, int _token, int _zoneID, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.saveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneUndoScene(struct soap *soap, int _token, int _zoneID, int _groupID, int _sceneID, bool& result) {
  dss::Group group(-1, 0, dss::DSS::getInstance()->getApartment());
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group.undoScene(_sceneID);
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
  dev.turnOn();
  result = true;
  return SOAP_OK;
}

int dss__DeviceTurnOff(struct soap *soap, int _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.turnOff();
  result = true;
  return SOAP_OK;
}

int dss__DeviceIncreaseValue(struct soap *soap, int _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.increaseValue();
  result = true;
  return SOAP_OK;
}

int dss__DeviceDecreaseValue(struct soap *soap, int _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.decreaseValue();
  result = true;
  return SOAP_OK;
}

int dss__DeviceEnable(struct soap *soap, int _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.enable();
  result = true;
  return SOAP_OK;
}

int dss__DeviceDisable(struct soap *soap, int _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.disable();
  result = true;
  return SOAP_OK;
}

int dss__DeviceStartDim(struct soap *soap, int _token, char* _deviceID, bool _directionUp, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.startDim(_directionUp);
  result = true;
  return SOAP_OK;
}

int dss__DeviceEndDim(struct soap *soap, int _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.endDim();
  result = true;
  return SOAP_OK;
}

int dss__DeviceSetValue(struct soap *soap, int _token, char* _deviceID, double _value, int _paramID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.setValue(_value, _paramID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceCallScene(struct soap *soap, int _token, char* _deviceID, int _sceneID, bool& result) {
  dss::DeviceReference device(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.callScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceSaveScene(struct soap *soap, int _token, char* _deviceID, int _sceneID, bool& result) {
  dss::DeviceReference device(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.saveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceUndoScene(struct soap *soap, int _token, char* _deviceID, int _sceneID, bool& result) {
  dss::DeviceReference device(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.undoScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceGetValue(struct soap *soap, int _token, char* _deviceID, int _paramID, double& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  result = dev.getDevice().getValue(_paramID);
  return SOAP_OK;
}

int dss__DeviceGetName(struct soap *soap, int _token, char* _deviceID, char** result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  *result = soap_strdup(soap, dev.getName().c_str());
  return SOAP_OK;
} // dss__DeviceGetName

int dss__DeviceGetFunctionID(struct soap *soap, int _token, char* _deviceID, int& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
	return getResult;
  }
  result = dev.getDevice().getFunctionID();
  return SOAP_OK;
} // dss__DeviceGetFunctionID

int dss__SwitchGetGroupID(struct soap *soap, int _token, char* _deviceID, int& result) {
 /*
  dss::DeviceReference devRef(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, devRef);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dss::DSIDInterface* simDev = dss::DSS::getInstance()->getSimulation().getSimulatedDevice(FromSOAP(_deviceID));
  dss::DSIDSimSwitch* sw = NULL;
  if(simDev != NULL && (sw = dynamic_cast<dss::DSIDSimSwitch*>(simDev)) != NULL) {
  	result = dss::DSS::getInstance()->getSimulation().getGroupForSwitch(sw);
  } else {
    dss::Device& dev = devRef.getDevice();
    for(int iGroup = 1; iGroup < 9; iGroup++) {
      if(dev.isInGroup(iGroup)) {
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
  dss::DSIDInterface* simDev = dss::DSS::getInstance()->getSimulation().getSimulatedDevice(FromSOAP(_deviceID));
  dss::DSIDSimSwitch* sw = NULL;
  if(simDev != NULL && (sw = dynamic_cast<dss::DSIDSimSwitch*>(simDev)) != NULL) {
    dss::ButtonPressKind kind = dss::Click;
    if(std::string(_kind) == "touch") {
      kind = dss::Touch;
    } else if(std::string(_kind) == "touchend") {
      kind = dss::TouchEnd;
    }
    dss::DSS::getInstance()->getSimulation().processButtonPress(*sw, _buttonNr, kind);
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

  result = dev.getDevice().getZoneID();
  return SOAP_OK;
} // dss__DeviceGetZoneID


//==================================================== Information

int dss__ModulatorGetPowerConsumption(struct soap *soap, int _token, int _modulatorID, unsigned long& result) {
  dss::Modulator mod(dss::NullDSID);
  int getResult = AuthorizeAndGetModulatorByBusID(soap, _token, _modulatorID, mod);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  result = mod.getPowerConsumption();
  return SOAP_OK;
} // dss__ModulatorGetPowerConsumption


//==================================================== Organization

//These calls may be restricted to privileged users.

int dss__ApartmentGetModulatorIDs(struct soap *soap, int _token, std::vector<std::string>& ids) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();

  std::vector<dss::Modulator*>& modulators = apt.getModulators();

  for(unsigned int iModulator = 0; iModulator < modulators.size(); iModulator++) {
    dss::dsid_t dsid = modulators[iModulator]->getDSID();
    ids.push_back(dsid.toString());
  }

  return SOAP_OK;
}

int dss__ModulatorGetName(struct soap *soap, int _token, char* _modulatorID, std::string& name) {
  dss::Modulator mod(dss::NullDSID);
  int getResult = AuthorizeAndGetModulator(soap, _token, _modulatorID, mod);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  name = mod.getName();
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
      evt->setLocation(_location);
    }
    if(_context != NULL && strlen(_context) > 0) {
      evt->setContext(_context);
    }
    if(_parameter != NULL && strlen(_parameter) > 0) {
      vector<std::string> params = dss::splitString(_parameter, ';');
      for(vector<std::string>::iterator iParam = params.begin(), e = params.end();
          iParam != e; ++iParam)
      {
        vector<std::string> nameValue = dss::splitString(*iParam, '=');
        if(nameValue.size() == 2) {
          dss::Logger::getInstance()->log("SOAP::eventRaise: Got parameter '" + nameValue[0] + "'='" + nameValue[1] + "'");
          evt->setProperty(nameValue[0], nameValue[1]);
        } else {
          dss::Logger::getInstance()->log(std::string("Invalid parameter found SOAP::eventRaise: ") + *iParam );
        }
      }
    }
    dss::DSS::getInstance()->getEventQueue().pushEvent(evt);
  } else {
    return soap_sender_fault(soap, "eventname must be provided", NULL);
  }

  result = true;
  return SOAP_OK;
} // dss__EventRaise

int dss__EventWaitFor(struct soap *soap, int _token, int _timeout, std::vector<dss__Event>& result) {
  dss::WebServiceSession* session;
  int getResult = AuthorizeAndGetSession(soap, _token, &session);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  session->waitForEvent(_timeout, soap);

  while(session->hasEvent()) {
    dss::Event origEvent = session->popEvent();
    dss__Event evt;
    evt.name = origEvent.getName();
    const dss::HashMapConstStringString& props =  origEvent.getProperties().getContainer();
    for(dss::HashMapConstStringString::const_iterator iParam = props.begin(), e = props.end();
        iParam != e; ++iParam)
    {
      std::string paramString = iParam->first + "=" + iParam->second;
      evt.parameter.push_back(paramString);
    }
    result.push_back(evt);
  }

  return SOAP_OK;
} // dss__EventWaitFor

int dss__EventSubscribeTo(struct soap *soap, int _token, std::string _name, std::string& result) {
  dss::WebServiceSession* session;
  int getResult = AuthorizeAndGetSession(soap, _token, &session);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  result = session->subscribeTo(_name);

  return SOAP_OK;
} // dss__EventSubscribeTo


//==================================================== Properties

int dss__PropertyGetType(struct soap *soap, int _token, std::string _propertyName, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::getInstance()->getPropertySystem();
  dss::PropertyNodePtr node = propSys.getProperty(_propertyName);
  if(node == NULL) {
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  result = dss::getValueTypeAsString(node->getValueType());
  return SOAP_OK;
} // dss__PropertyGetType

int dss__PropertySetInt(struct soap *soap, int _token, std::string _propertyName, int _value, bool _mayCreate, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    result = false;
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::getInstance()->getPropertySystem();
  if(!propSys.setIntValue(_propertyName, _value, _mayCreate)) {
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

  dss::PropertySystem& propSys = dss::DSS::getInstance()->getPropertySystem();
  if(!propSys.setStringValue(_propertyName, _value, _mayCreate)) {
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

  dss::PropertySystem& propSys = dss::DSS::getInstance()->getPropertySystem();
  if(!propSys.setBoolValue(_propertyName, _value, _mayCreate)) {
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

  dss::PropertySystem& propSys = dss::DSS::getInstance()->getPropertySystem();
  dss::PropertyNodePtr node = propSys.getProperty(_propertyName);
  if(node == NULL) {
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  result = node->getIntegerValue();
  return SOAP_OK;
} // dss__PropertyGetInt

int dss__PropertyGetString(struct soap *soap, int _token, std::string _propertyName, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::getInstance()->getPropertySystem();
  dss::PropertyNodePtr node = propSys.getProperty(_propertyName);
  if(node == NULL) {
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  result = node->getStringValue();
  return SOAP_OK;
} // dss__PropertyGetString

int dss__PropertyGetBool(struct soap *soap, int _token, std::string _propertyName, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::getInstance()->getPropertySystem();
  dss::PropertyNodePtr node = propSys.getProperty(_propertyName);
  if(node == NULL) {
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  result = node->getBoolValue();
  return SOAP_OK;
} // dss__PropertyGetBool

int dss__PropertyGetChildren(struct soap *soap, int _token, std::string _propertyName, std::vector<std::string>& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::getInstance()->getPropertySystem();
  dss::PropertyNodePtr node = propSys.getProperty(_propertyName);
  if(node == NULL) {
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  for(int iChild = 0; iChild < node->getChildCount(); iChild++) {
    result.push_back(node->getChild(iChild)->getName());
  }

  return SOAP_OK;
} // dss__PropertyGetChildren

