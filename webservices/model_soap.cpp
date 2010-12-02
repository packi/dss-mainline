/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

#include "soapH.h"
#include "core/dss.h"
#include "core/logger.h"
#include "core/event.h"
#include "webservices/webservices.h"
#include "core/sim/dssim.h"
#include "core/propertysystem.h"
#include "core/setbuilder.h"
#include "core/foreach.h"
#include "core/model/apartment.h"
#include "core/model/set.h"
#include "core/model/device.h"
#include "core/model/devicereference.h"
#include "core/model/set.h"
#include "core/model/zone.h"
#include "core/model/group.h"
#include "core/model/modulator.h"
#include "core/structuremanipulator.h"
#include "core/businterface.h"

inline dss::dss_dsid_t FromSOAP(const char* _dsid) {
  dss::dss_dsid_t result;
  try {
    result = dss::dss_dsid_t::fromString(_dsid);
  } catch (std::invalid_argument&) {
    result = dss::NullDSID;
  }
  return result;
}


//==================================================== Helpers

int NotAuthorized(struct soap *soap) {
  return soap_receiver_fault(soap, "Not Authorized", NULL);
} // notAuthorized

bool IsAuthorized(struct soap *soap, const char* _token) {
  bool result = dss::DSS::getInstance()->getWebServices().isAuthorized(soap, _token);
  if(result) {
    dss::Logger::getInstance()->log(std::string("User with token '") + _token + "' is authorized");
  } else {
    dss::Logger::getInstance()->log(std::string("User with token '") + _token + "' is *not* authorized", lsWarning);
  }
  return result;
} // isAuthorized

int AuthorizeAndGetSession(struct soap *soap, const char* _token, dss::Session** result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  *result = dss::DSS::getInstance()->getWebServices().getSession(soap, _token).get();
  return SOAP_OK;
} // AuthorizeAndGetSession

int AuthorizeAndGetSet(struct soap *soap, const char* _token, const char* _setSpec, dss::Set& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  dss::SetBuilder builder(apt);
  result = builder.buildSet(_setSpec, boost::shared_ptr<dss::Zone>());
  return SOAP_OK;
} // authorizeAndGetSet

int AuthorizeAndGetDevice(struct soap *soap, const char* _token, char* _devID, dss::DeviceReference& result) {
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

int AuthorizeAndGetDSMeter(struct soap *soap, const char* _token, char* _dsMeterDSID, boost::shared_ptr<dss::DSMeter>& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    result = apt.getDSMeterByDSID(FromSOAP(_dsMeterDSID));
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "DSMeter not found", NULL);
  }
  return SOAP_OK;
} // authorizeAndGetDSMeter

int AuthorizeAndGetGroup(struct soap *soap, const char* _token, const int _groupID, boost::shared_ptr<dss::Group>& result) {
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

int AuthorizeAndGetGroupOfZone(struct soap *soap, const char* _token, const int _zoneID, const int _groupID, boost::shared_ptr<dss::Group>& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    boost::shared_ptr<dss::Group> pResult = apt.getZone(_zoneID)->getGroup(_groupID);
    if(pResult == NULL) {
      return soap_receiver_fault(soap, "Group not found", NULL);
    }
    result = pResult;
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Zone not found", NULL);
  }
  return SOAP_OK;
} // authorizeAndGetGroupOfZone


//==================================================== Callbacks

int dss__Authenticate(struct soap *soap, char* _userName, char* _password, std::string& token) {
  token = dss::DSS::getInstance()->getWebServices().newSession(soap);
  return SOAP_OK;
} // dss__Authenticate

int dss__SignOff(struct soap *soap, char* _token, int& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::DSS::getInstance()->getWebServices().deleteSession(soap, _token);
  return SOAP_OK;
} // dss__SignOff

int dss__ApartmentCreateSetFromGroup(struct soap *soap, char* _token, char* _groupName, std::string& result) {
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

int dss__ApartmentCreateSetFromDeviceIDs(struct soap *soap, char* _token, std::vector<std::string> _ids, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    result = "addDevices(";
    std::vector<std::string> dsids;
    for(unsigned int iID = 0; iID < _ids.size(); iID++) {
      boost::shared_ptr<dss::Device> dev = apt.getDeviceByDSID(FromSOAP(_ids[iID].c_str()));
      dsids.push_back(dev->getDSID().toString());
    }
    result += dss::join(dsids, ",");
    result += ")";
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Unknown device", NULL);
  }
  return SOAP_OK;
} // dss__ApartmentCreateSetFromDeviceIDs

int dss__ApartmentCreateSetFromDeviceNames(struct soap *soap, char* _token,  std::vector<std::string> _names, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    dss::Set set;
    result = "addDevices(";
    std::vector<std::string> names;
    for(unsigned int iName = 0; iName < _names.size(); iName++) {
      boost::shared_ptr<dss::Device> dev = apt.getDeviceByName(_names[iName]);
      names.push_back("'" + dev->getName() + "'");
    }
    result += dss::join(names, ",");
    result += ")";
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Unknown device", NULL);
  }
  return SOAP_OK;
} // dss__ApartmentCreateSetFromDeviceNames

int dss__ApartmentGetDevices(struct soap *soap, char* _token, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  result = ".";
  return SOAP_OK;
} // dss__ApartmenGetDevices

int dss__ApartmentGetDeviceIDByName(struct soap *soap, char* _token,  char* _deviceName, std::string& deviceID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    dss::dss_dsid_t dsid = apt.getDevices().getByName(_deviceName).getDSID();
    std::string asString = dsid.toString();
    deviceID = soap_strdup(soap, asString.c_str());
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find device", NULL);
  }
  return SOAP_OK;
}

int dss__ApartmentGetName(struct soap *soap, char* _token, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  result = apt.getName();
  return SOAP_OK;
} // dss__ApartmentGetName

int dss__ApartmentSetName(struct soap *soap, char* _token, char* _name, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  apt.setName(_name);
  result = true;
  return SOAP_OK;
} // dss__ApartmentSetName


//==================================================== Set

int dss__SetAddDeviceByName(struct soap *soap, char* _token, char* _setSpec, char* _name, std::string& result) {
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

int dss__SetAddDeviceByID(struct soap *soap, char* _token, char* _setSpec, char* _deviceID, std::string& result) {
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

int dss__SetRemoveDevice(struct soap *soap, char* _token, char* _setSpec, char* _deviceID, std::string& result) {
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

int dss__SetCombine(struct soap *soap, char* _token, char* _setSpec1, char* _setSpec2, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  result = _setSpec1;
  result += ".combine(";
  result += _setSpec2;
  result += ")";
  return SOAP_OK;
} // dss_SetCombine

int dss__SetRemove(struct soap *soap, char* _token, char* _setSpec, char* _setSpecToRemove, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  result = _setSpec;
  result += ".remove(";
  result += _setSpecToRemove;
  result += ")";
  return SOAP_OK;
} // dss__SetRemove

int dss__SetByGroup(struct soap *soap, char* _token, char* _setSpec, int _groupID, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  result = _setSpec;
  result += ".group(" + dss::intToString(_groupID) + ")";
  return SOAP_OK;
} // dss__SetByGroup

int dss__SetGetContainedDevices(struct soap* soap, char* _token, char* _setSpec, std::vector<std::string>& deviceIDs) {
  dss::Set set;
  int res = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(res != SOAP_OK) {
    return res;
  }

  int numDevices = set.length();
  for(int iDeviceID = 0; iDeviceID < numDevices; iDeviceID++) {
    dss::dss_dsid_t dsid = set.get(iDeviceID).getDSID();
    deviceIDs.push_back(dsid.toString());
  }

  return SOAP_OK;
} // dss__SetGetContainerDevices

//==================================================== Apartment

int dss__ApartmentGetGroupByName(struct soap *soap, char* _token, char* _groupName, int& groupID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    groupID = apt.getGroup(_groupName)->getID();
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find group", NULL);
  }
  return SOAP_OK;
}

int dss__ApartmentGetZoneByName(struct soap *soap, char* _token,  char* _zoneName, int& zoneID) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    zoneID = apt.getZone(_zoneName)->getID();
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Could not find zone", NULL);
  }
  return SOAP_OK;
}

int dss__ApartmentGetZoneIDs(struct soap *soap, char* _token, std::vector<int>& zoneIDs) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  std::vector<boost::shared_ptr<dss::Zone> > zones = apt.getZones();

  int numZones = zones.size();
  for(int iZoneID = 0; iZoneID < numZones; iZoneID++) {
    zoneIDs.push_back(zones[iZoneID]->getID());
  }

  return SOAP_OK;
} // dss__ApartmentGetZoneIDs

int dss__ApartmentRescan(struct soap *soap, char* _token, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  std::vector<boost::shared_ptr<dss::DSMeter> > mods = apt.getDSMeters();
  foreach(boost::shared_ptr<dss::DSMeter> pDSMeter, mods) {
    pDSMeter->setIsValid(false);
  }

  return SOAP_OK;
} // dss__ApartmentRescan

//==================================================== Circuit

int dss__CircuitRescan(struct soap *soap, char* _token, char* _dsid, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  dss::dss_dsid_t dsid;
  try {
    dsid = dss::dss_dsid_t::fromString(_dsid);
  } catch(std::invalid_argument&) {
    return soap_sender_fault(soap, "Error parsing dsid", NULL);
  }
  try {
    boost::shared_ptr<dss::DSMeter> mod = apt.getDSMeterByDSID(dsid);
    mod->setIsValid(false);
  } catch(dss::ItemNotFoundException&) {
    return soap_sender_fault(soap, "Could not find dsMeter", NULL);
  }

  return SOAP_OK;
} // dss__CircuitRescan


//==================================================== Manipulation

//--------------------------- Set

int dss__SetTurnOn(struct soap *soap, char* _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.turnOn();
  result = true;
  return SOAP_OK;
}

int dss__SetTurnOff(struct soap *soap, char* _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.turnOff();
  result = true;
  return SOAP_OK;
}

int dss__SetIncreaseValue(struct soap *soap, char* _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.increaseValue();
  result = true;
  return SOAP_OK;
}

int dss__SetDecreaseValue(struct soap *soap, char* _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.decreaseValue();
  result = true;
  return SOAP_OK;
}

int dss__SetSetValue(struct soap *soap, char* _token, char* _setSpec, double _value, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.setValue(_value);
  result = true;
  return SOAP_OK;
}

int dss__SetCallScene(struct soap *soap, char* _token, char* _setSpec, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.callScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__SetSaveScene(struct soap *soap, char* _token, char* _setSpec, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.saveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__SetUndoScene(struct soap *soap, char* _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.undoScene();
  result = true;
  return SOAP_OK;
}

//---------------------------------- Apartment

int dss__ApartmentTurnOn(struct soap *soap, char* _token, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->turnOn();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentTurnOff(struct soap *soap, char* _token, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->turnOff();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentIncreaseValue(struct soap *soap, char* _token, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->increaseValue();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentDecreaseValue(struct soap *soap, char* _token, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->decreaseValue();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentSetValue(struct soap *soap, char* _token, int _groupID, double _value, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->setValue(_value);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentCallScene(struct soap *soap, char* _token, int _groupID, int _sceneID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->callScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentSaveScene(struct soap *soap, char* _token, int _groupID, int _sceneID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->saveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentUndoScene(struct soap *soap, char* _token, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->undoScene();
  result = true;
  return SOAP_OK;
}


//---------------------------------- Zone

int dss__ZoneTurnOn(struct soap *soap, char* _token, int _zoneID, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->turnOn();
  result = true;
  return SOAP_OK;
}

int dss__ZoneTurnOff(struct soap *soap, char* _token, int _zoneID, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->turnOff();
  result = true;
  return SOAP_OK;
}

int dss__ZoneIncreaseValue(struct soap *soap, char* _token, int _zoneID, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->increaseValue();
  result = true;
  return SOAP_OK;
}

int dss__ZoneDecreaseValue(struct soap *soap, char* _token, int _zoneID, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->decreaseValue();
  result = true;
  return SOAP_OK;
}

int dss__ZoneSetValue(struct soap *soap, char* _token, int _zoneID, int _groupID, double _value, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->setValue(_value);
  result = true;
  return SOAP_OK;
}

int dss__ZoneCallScene(struct soap *soap, char* _token, int _zoneID, int _groupID, int _sceneID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->callScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneSaveScene(struct soap *soap, char* _token, int _zoneID, int _groupID, int _sceneID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->saveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneUndoScene(struct soap *soap, char* _token, int _zoneID, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->undoScene();
  result = true;
  return SOAP_OK;
}

//---------------------------------- Device

int dss__DeviceTurnOn(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.turnOn();
  result = true;
  return SOAP_OK;
}

int dss__DeviceTurnOff(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.turnOff();
  result = true;
  return SOAP_OK;
}

int dss__DeviceIncreaseValue(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.increaseValue();
  result = true;
  return SOAP_OK;
}

int dss__DeviceDecreaseValue(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.decreaseValue();
  result = true;
  return SOAP_OK;
}

int dss__DeviceCallScene(struct soap *soap, char* _token, char* _deviceID, int _sceneID, bool& result) {
  dss::DeviceReference device(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.callScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceSaveScene(struct soap *soap, char* _token, char* _deviceID, int _sceneID, bool& result) {
  dss::DeviceReference device(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.saveScene(_sceneID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceUndoScene(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference device(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.undoScene();
  result = true;
  return SOAP_OK;
}

int dss__DeviceGetConfig(struct soap *soap, char* _token, char* _deviceID, 
                        uint8_t _configClass, uint8_t _configIndex, 
                        uint8_t& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  result = dev.getDevice()->getConfig(_configClass, _configIndex);
  return SOAP_OK;
}

int dss__DeviceGetConfigWord(struct soap *soap, char* _token, char* _deviceID, 
                             uint8_t _configClass,
                             uint8_t _configIndex, uint16_t& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  result = dev.getDevice()->getConfigWord(_configClass, _configIndex);
  return SOAP_OK;
}

int dss__DeviceSetConfig(struct soap *soap, char* _token, char* _deviceID, 
                         uint8_t _configClass, uint8_t _configIndex, 
                         uint8_t _value, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.getDevice()->setConfig(_configClass, _configIndex, _value);
  result = true;
  return SOAP_OK;
}

int dss__DeviceSetValue(struct soap *soap, char* _token, char* _deviceID, 
                        uint8_t _value, bool& result)
{
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.getDevice()->setOutputValue(_value);
  result = true;
  return SOAP_OK;
}

int dss__DeviceGetName(struct soap *soap, char* _token, char* _deviceID, char** result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  *result = soap_strdup(soap, dev.getName().c_str());
  return SOAP_OK;
} // dss__DeviceGetName

int dss__DeviceSetName(struct soap *soap, char* _token, char* _deviceID, char* _name, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.getDevice()->setName(_name);
  result = true;
  return SOAP_OK;
} // dss__DeviceSetName

int dss__DeviceGetFunctionID(struct soap *soap, char* _token, char* _deviceID, int& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  result = dev.getDevice()->getFunctionID();
  return SOAP_OK;
} // dss__DeviceGetFunctionID

int dss__DeviceGetZoneID(struct soap *soap, char* _token, char* _deviceID, int& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  result = dev.getDevice()->getZoneID();
  return SOAP_OK;
} // dss__DeviceGetZoneID

int dss__DeviceAddTag(struct soap *soap, char* _token, char* _deviceID, char* _tag, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  dev.getDevice()->addTag(_tag);
  result = true;
  return SOAP_OK;
} // dss__DeviceAddTag

int dss__DeviceRemoveTag(struct soap *soap, char* _token, char* _deviceID, char* _tag, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  dev.getDevice()->removeTag(_tag);
  result = true;
  return SOAP_OK;
} // dss__DeviceRemoveTag

int dss__DeviceHasTag(struct soap *soap, char* _token, char* _deviceID, char* _tag, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  result = dev.getDevice()->hasTag(_tag);
  return SOAP_OK;
} // dss__DeviceHasTag

int dss__DeviceGetTags(struct soap *soap, char* _token, char* _deviceID, std::vector<std::string>& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  result = dev.getDevice()->getTags();
  return SOAP_OK;
} // dss__DeviceGetTags

int dss__DeviceLock(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  try {
    dev.getDevice()->lock();
    result = true;
  } catch(std::exception& e) {
    result = false;
  }
  return SOAP_OK;
} // dss__DeviceLock

int dss__DeviceUnlock(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  try {
    dev.getDevice()->unlock();
    result = true;
  } catch(std::exception& e) {
    result = false;
  }
  return SOAP_OK;
} // dss__DeviceUnlock

int dss__DeviceGetIsLocked(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  result = dev.getDevice()->getIsLockedInDSM();
  return SOAP_OK;
} // dss__DeviceGetIsLocked


//==================================================== Information

int dss__DSMeterGetPowerConsumption(struct soap *soap, char* _token, char* _dsMeterID, unsigned long& result) {
  boost::shared_ptr<dss::DSMeter> mod;
  int getResult = AuthorizeAndGetDSMeter(soap, _token, _dsMeterID, mod);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  result = mod->getPowerConsumption();
  return SOAP_OK;
} // dss__DSMeterGetPowerConsumption


//==================================================== Organization

//These calls may be restricted to privileged users.

int dss__ApartmentGetDSMeterIDs(struct soap *soap, char* _token, std::vector<std::string>& ids) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();

  std::vector<boost::shared_ptr<dss::DSMeter> > dsMeters = apt.getDSMeters();

  for(unsigned int iDSMeter = 0; iDSMeter < dsMeters.size(); iDSMeter++) {
    dss::dss_dsid_t dsid = dsMeters[iDSMeter]->getDSID();
    ids.push_back(dsid.toString());
  }

  return SOAP_OK;
}

int dss__DSMeterGetName(struct soap *soap, char* _token, char* _dsMeterID, std::string& name) {
  boost::shared_ptr<dss::DSMeter> mod;
  int getResult = AuthorizeAndGetDSMeter(soap, _token, _dsMeterID, mod);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  name = mod->getName();
  return SOAP_OK;
} // dss__DSMeterGetName

int dss__DSMeterSetName(struct soap *soap, char* _token, char* _dsMeterID, char*  _name, bool& result) {
  boost::shared_ptr<dss::DSMeter> mod;
  int getResult = AuthorizeAndGetDSMeter(soap, _token, _dsMeterID, mod);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  mod->setName(_name);
  result = true;
  return SOAP_OK;
} // dss__DSMeterSetName

int dss__ApartmentAllocateZone(struct soap *soap, char* _token, int& zoneID) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__ApartmentDeleteZone(struct soap *soap, char* _token, int _zoneID, int& result) {
  return soap_sender_fault(soap, "Not yet implemented", NULL);
}

int dss__ZoneSetName(struct soap *soap, char* _token, int _zoneID, char* _name, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    boost::shared_ptr<dss::Zone> zone = apt.getZone(_zoneID);
    zone->setName(_name);
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Zone not found", NULL);
  }
  return SOAP_OK;
} // dss__ZoneSetName

int dss__ZoneGetName(struct soap *soap, char* _token, int _zoneID, std::string& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    boost::shared_ptr<dss::Zone> zone = apt.getZone(_zoneID);
    result = zone->getName();
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Zone not found", NULL);
  }
  return SOAP_OK;
} // dss__ZoneGetName

int dss__GroupSetName(struct soap *soap, char* _token, int _zoneID, int _groupID, char* _name, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->setName(_name);
  result = true;
  return SOAP_OK;
} // dss__GroupSetName

int dss__GroupGetName(struct soap *soap, char* _token, int _zoneID, int _groupID, std::string& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  result = group->getName();
  return SOAP_OK;
} // dss__GroupGetName

//==================================================== Events

int dss__EventRaise(struct soap *soap, char* _token, char* _eventName, char* _context, char* _parameter, char* _location, bool& result) {
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
      std::vector<std::string> params = dss::splitString(_parameter, ';');
      for(std::vector<std::string>::iterator iParam = params.begin(), e = params.end();
          iParam != e; ++iParam)
      {
        std::vector<std::string> nameValue = dss::splitString(*iParam, '=');
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

int dss__EventWaitFor(struct soap *soap, char* _token, int _timeout, std::vector<dss__Event>& result) {
  dss::Session* session;
  int getResult = AuthorizeAndGetSession(soap, _token, &session);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  dss::WebServices* pServices = &dss::DSS::getInstance()->getWebServices();
  pServices->waitForEvent(session, _timeout, soap);

  while(pServices->hasEvent(session)) {
    dss::Event origEvent = pServices->popEvent(session);
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

int dss__EventSubscribeTo(struct soap *soap, char* _token, std::string _name, std::string& result) {
  dss::Session* session;
  int getResult = AuthorizeAndGetSession(soap, _token, &session);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  result = dss::DSS::getInstance()->getWebServices().subscribeTo(session, _name);

  return SOAP_OK;
} // dss__EventSubscribeTo


//==================================================== Properties

int dss__PropertyGetType(struct soap *soap, char* _token, std::string _propertyName, std::string& result) {
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

int dss__PropertySetInt(struct soap *soap, char* _token, std::string _propertyName, int _value, bool _mayCreate, bool& result) {
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

int dss__PropertySetString(struct soap *soap, char* _token, std::string _propertyName, char* _value, bool _mayCreate, bool& result) {
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

int dss__PropertySetBool(struct soap *soap, char* _token, std::string _propertyName, bool _value, bool _mayCreate, bool& result) {
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

int dss__PropertyGetInt(struct soap *soap, char* _token, std::string _propertyName, int& result) {
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

int dss__PropertyGetString(struct soap *soap, char* _token, std::string _propertyName, std::string& result) {
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

int dss__PropertyGetBool(struct soap *soap, char* _token, std::string _propertyName, bool& result) {
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

int dss__PropertyGetChildren(struct soap *soap, char* _token, std::string _propertyName, std::vector<std::string>& result) {
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

int dss__StructureAddDeviceToZone(struct soap *soap, char* _token, char* _deviceID, int _zoneID, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::DSS& dssRef = *dss::DSS::getInstance();
  dss::Apartment& aptRef = dssRef.getApartment();

  try {
    boost::shared_ptr<dss::Device> dev = aptRef.getDeviceByDSID(dss::dss_dsid_t::fromString(_deviceID));
    boost::shared_ptr<dss::Zone> zone = aptRef.getZone(_zoneID);
    dss::StructureManipulator manipulator(*dssRef.getBusInterface().getStructureModifyingBusInterface(),
                                          aptRef);
    manipulator.addDeviceToZone(dev, zone);
  } catch(std::runtime_error& _ex) {
    return soap_receiver_fault(soap, "Error handling request", NULL);
  }
  return SOAP_OK;
} // dss__StructureAddDeviceToZone

