/*
    Copyright (c) 2009,2011 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
            Michael Tross, aizo GmbH <michael.tross@aizo.com>
            Christian Hitz, aizo AG <christian.hitz@aizo.com>

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

#include "webservices/soapH.h"
#include "webservices/webservices.h"
#include "src/dss.h"
#include "src/logger.h"
#include "src/event.h"
#include "src/sim/dssim.h"
#include "src/propertysystem.h"
#include "src/propertyquery.h"
#include "src/setbuilder.h"
#include "src/foreach.h"
#include "src/model/apartment.h"
#include "src/model/set.h"
#include "src/model/device.h"
#include "src/model/devicereference.h"
#include "src/model/set.h"
#include "src/model/zone.h"
#include "src/model/group.h"
#include "src/model/modulator.h"
#include "src/session.h"
#include "src/sessionmanager.h"
#include "src/security/security.h"
#include "src/structuremanipulator.h"
#include "src/businterface.h"
#include "src/metering/metering.h"
#include "src/web/json.h"
#include "src/sceneaccess.h"

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
  dss::DSS::getInstance()->getSecurity().signOff();
  bool result = dss::DSS::getInstance()->getWebServices().isAuthorized(soap, _token);
  if(result) {
    dss::Logger::getInstance()->log(std::string("User with token '") + _token + "' is authorized");
    boost::shared_ptr<dss::Session> session = dss::DSS::getInstance()->getWebServices().getSession(soap, _token);
    session->touch();
    dss::DSS::getInstance()->getSecurity().authenticate(session);
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
  result = builder.buildSet(_setSpec, boost::shared_ptr<dss::Group>());
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

int AuthorizeAndGetZone(struct soap *soap, const char* _token, const int _zoneID, boost::shared_ptr<dss::Zone>& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  try {
    result = apt.getZone(_zoneID);
  } catch(dss::ItemNotFoundException& _ex) {
    return soap_receiver_fault(soap, "Zone not found", NULL);
  }
  return SOAP_OK;
} // authorizeAndGetZone

//==================================================== Callbacks

int dss__Authenticate(struct soap *soap, char* _userName, char* _password, std::string& token) {
  dss::SessionManager& manager = dss::DSS::getInstance()->getSessionManager();
  manager.getSecurity()->signOff();
  if(manager.getSecurity()->authenticate(_userName, _password)) {
    token = dss::DSS::getInstance()->getWebServices().getSessionManager()->registerSession();
    manager.getSession(token)->inheritUserFromSecurity();
    return SOAP_OK;
  } else {
    return soap_sender_fault(soap, "Invalid username or password", NULL);
  }
} // dss__Authenticate

int dss__AuthenticateAsApplication(struct soap *soap, char* _applicationToken, std::string& token) {
  dss::SessionManager& manager = dss::DSS::getInstance()->getSessionManager();
  manager.getSecurity()->signOff();
  if(manager.getSecurity()->authenticateApplication(_applicationToken)) {
    token = dss::DSS::getInstance()->getWebServices().getSessionManager()->registerApplicationSession();
    manager.getSession(token)->inheritUserFromSecurity();
    return SOAP_OK;
  } else {
    return soap_sender_fault(soap, "Invalid application token", NULL);
  }
} // dss__Authenticate

int dss__RequestApplicationToken(struct soap *soap, char* _applicationName, std::string& result) {
  dss::SessionManager& manager = dss::DSS::getInstance()->getSessionManager();
  std::string applicationToken = manager.generateToken();
  manager.getSecurity()->loginAsSystemUser("Temporary access to create token");
  manager.getSecurity()->createApplicationToken(_applicationName,
                                                applicationToken);
  result = applicationToken;
  return SOAP_OK;
} // dss__RequestApplicationToken

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

int dss__ApartmentRemoveMeter(struct soap *soap, char* _token, char* _dsMeterID, bool& result) {
  boost::shared_ptr<dss::DSMeter> mod;
  int getResult = AuthorizeAndGetDSMeter(soap, _token, _dsMeterID, mod);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  dss::DSS::getInstance()->getApartment().removeDSMeter(FromSOAP(_dsMeterID));
  result = true;
  return SOAP_OK;
}

int dss__ApartmentRemoveInactiveMeters(struct soap *soap, char* _token, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  apt.removeInactiveMeters();
  result = true;
  return SOAP_OK;
}

int dss__ApartmentGetPowerConsumption(struct soap *soap, char* _token, unsigned long& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  int accumulatedConsumption = 0;
  foreach(boost::shared_ptr<dss::DSMeter> pDSMeter, apt.getDSMeters()) {
    accumulatedConsumption += pDSMeter->getPowerConsumption();
  }
  result = accumulatedConsumption;
  return SOAP_OK;
}

int dss__ApartmentGetEnergyMeterValue(struct soap *soap, char* _token, unsigned long& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apt = dss::DSS::getInstance()->getApartment();
  int accumulatedMeterValue = 0;
  foreach(boost::shared_ptr<dss::DSMeter> pDSMeter, apt.getDSMeters()) {
    accumulatedMeterValue += pDSMeter->getEnergyMeterValue();
  }
  result = accumulatedMeterValue;
  return SOAP_OK;
}

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
  set.turnOn(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__SetTurnOff(struct soap *soap, char* _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.turnOff(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__SetIncreaseValue(struct soap *soap, char* _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.increaseValue(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__SetDecreaseValue(struct soap *soap, char* _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.decreaseValue(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__SetSetValue(struct soap *soap, char* _token, char* _setSpec, uint8_t _value, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.setValue(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN, _value);
  result = true;
  return SOAP_OK;
}

int dss__SetCallScene(struct soap *soap, char* _token, char* _setSpec, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.callScene(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN, _sceneID);
  result = true;
  return SOAP_OK;
}

int dss__SetSaveScene(struct soap *soap, char* _token, char* _setSpec, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.saveScene(dss::IDeviceInterface::coSOAP, _sceneID);
  result = true;
  return SOAP_OK;
}

int dss__SetUndoScene(struct soap *soap, char* _token, char* _setSpec, int _sceneID, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.undoScene(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN, _sceneID);
  result = true;
  return SOAP_OK;
}

int dss__SetUndoLastScene(struct soap *soap, char* _token, char* _setSpec, bool& result) {
  dss::Set set;
  int getResult = AuthorizeAndGetSet(soap, _token, _setSpec, set);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  set.undoSceneLast(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
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
  group->turnOn(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentTurnOff(struct soap *soap, char* _token, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->turnOff(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentIncreaseValue(struct soap *soap, char* _token, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->increaseValue(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentDecreaseValue(struct soap *soap, char* _token, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->decreaseValue(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentSetValue(struct soap *soap, char* _token, int _groupID, uint8_t _value, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->setValue(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN, _value);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentCallScene(struct soap *soap, char* _token, int _groupID, int _sceneID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->callScene(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN, _sceneID, false);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentSaveScene(struct soap *soap, char* _token, int _groupID, int _sceneID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->saveScene(dss::IDeviceInterface::coSOAP, _sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentUndoScene(struct soap *soap, char* _token, int _groupID, int _sceneID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->undoScene(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN, _sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentUndoLastScene(struct soap *soap, char* _token, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->undoSceneLast(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__ApartmentBlink(struct soap *soap, char* _token, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroup(soap, _token, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->blink(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
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
  group->turnOn(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__ZoneTurnOff(struct soap *soap, char* _token, int _zoneID, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->turnOff(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__ZoneIncreaseValue(struct soap *soap, char* _token, int _zoneID, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->increaseValue(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__ZoneDecreaseValue(struct soap *soap, char* _token, int _zoneID, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->decreaseValue(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__ZoneSetValue(struct soap *soap, char* _token, int _zoneID, int _groupID, uint8_t _value, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->setValue(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN, _value);
  result = true;
  return SOAP_OK;
}

int dss__ZoneCallScene(struct soap *soap, char* _token, int _zoneID, int _groupID, int _sceneID, bool _force, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->callScene(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN, _sceneID, _force);
  result = true;
  return SOAP_OK;
}

int dss__ZoneSaveScene(struct soap *soap, char* _token, int _zoneID, int _groupID, int _sceneID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->saveScene(dss::IDeviceInterface::coSOAP, _sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneUndoScene(struct soap *soap, char* _token, int _zoneID, int _groupID, int _sceneID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->undoScene(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN, _sceneID);
  result = true;
  return SOAP_OK;
}

int dss__ZoneUndoLastScene(struct soap *soap, char* _token, int _zoneID, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->undoSceneLast(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__ZoneBlink(struct soap *soap, char* _token, int _zoneID, int _groupID, bool& result) {
  boost::shared_ptr<dss::Group> group;
  int getResult = AuthorizeAndGetGroupOfZone(soap, _token, _zoneID, _groupID, group);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  group->blink(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__ZonePushSensorValue(struct soap *soap, char* _token, int _zoneID, char *_sourceDeviceID, int _sensorType, int _sensorValue, bool& result) {
  boost::shared_ptr<dss::Zone> zone;
  dss::DSS& dssRef = *dss::DSS::getInstance();
  dss::Apartment& aptRef = dssRef.getApartment();

  int getResult = AuthorizeAndGetZone(soap, _token, _zoneID, zone);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  try {
    dss::dss_dsid_t sourceDSID = dss::dsid::fromString(_sourceDeviceID);
    dss::StructureManipulator manipulator(*dssRef.getBusInterface().getStructureModifyingBusInterface(),
                                          *dssRef.getBusInterface().getStructureQueryBusInterface(),
                                          aptRef);
    manipulator.sensorPush(zone, sourceDSID, _sensorType, _sensorValue);
  } catch(std::runtime_error& _ex) {
    return soap_receiver_fault(soap, "Error handling request", NULL);
  } catch(std::invalid_argument&) {
    return soap_sender_fault(soap, "Error parsing dsid", NULL);
  }
  result = true;
  return SOAP_OK;
}

//---------------------------------- Device


int dss__DeviceBlink(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.blink(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__DeviceTurnOn(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.turnOn(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__DeviceTurnOff(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.turnOff(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__DeviceIncreaseValue(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.increaseValue(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__DeviceDecreaseValue(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.decreaseValue(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
  result = true;
  return SOAP_OK;
}

int dss__DeviceCallScene(struct soap *soap, char* _token, char* _deviceID, int _sceneID, bool _force, bool& result) {
  dss::DeviceReference device(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.callScene(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN, _sceneID, _force);
  result = true;
  return SOAP_OK;
}

int dss__DeviceSaveScene(struct soap *soap, char* _token, char* _deviceID, int _sceneID, bool& result) {
  dss::DeviceReference device(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.saveScene(dss::IDeviceInterface::coSOAP, _sceneID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceUndoScene(struct soap *soap, char* _token, char* _deviceID, int _sceneID, bool& result) {
  dss::DeviceReference device(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.undoScene(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN, _sceneID);
  result = true;
  return SOAP_OK;
}

int dss__DeviceUndoLastScene(struct soap *soap, char* _token, char* _deviceID, bool& result) {
  dss::DeviceReference device(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, device);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  device.undoSceneLast(dss::IDeviceInterface::coSOAP, dss::SAC_UNKNOWN);
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
  result = dev.getDevice()->getDeviceConfig(_configClass, _configIndex);
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
  result = dev.getDevice()->getDeviceConfigWord(_configClass, _configIndex);
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
  dev.getDevice()->setDeviceConfig(_configClass, _configIndex, _value);
  result = true;
  return SOAP_OK;
}

int dss__DeviceSetOutvalue(struct soap *soap, char* _token, char* _deviceID, unsigned char _valueIndex, unsigned short int _value, bool& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  dev.getDevice()->setDeviceOutputValue(_valueIndex, _value);
  result = true;
  return SOAP_OK;
}

int dss__DeviceGetOutvalue(struct soap *soap, char* _token, char* _deviceID, unsigned char _valueIndex, unsigned short int& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  result = dev.getDevice()->getDeviceOutputValue(_valueIndex);
  return SOAP_OK;
}

int dss__DeviceGetSensorType(struct soap *soap, char* _token, char* _deviceID, unsigned char _sensorIndex, unsigned char& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  result = dev.getDevice()->getDeviceSensorType(_sensorIndex);
  return SOAP_OK;
}

int dss__DeviceGetSensorValue(struct soap *soap, char* _token, char* _deviceID, unsigned char _sensorIndex, unsigned short int& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  result = dev.getDevice()->getDeviceSensorValue(_sensorIndex);
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
  dev.getDevice()->setDeviceValue(_value);
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

int dss__DeviceGetTransmissionQuality(struct soap *soap, char* _token, char* _deviceID, dss__TransmissionQuality& result) {
  dss::DeviceReference dev(dss::NullDSID, NULL);
  int getResult = AuthorizeAndGetDevice(soap, _token, _deviceID, dev);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  std::pair<uint8_t, uint16_t> p = dev.getDevice()->getDeviceTransmissionQuality();
  result.downstream = p.first;
  result.upstream = p.second;
  return SOAP_OK;
}

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

int dss__DSMeterGetEnergyMeterValue(struct soap *soap, char* _token, char* _dsMeterID, unsigned long& result) {
  boost::shared_ptr<dss::DSMeter> mod;
  int getResult = AuthorizeAndGetDSMeter(soap, _token, _dsMeterID, mod);
  if(getResult != SOAP_OK) {
    return getResult;
  }

  result = mod->getEnergyMeterValue();
  return SOAP_OK;
}


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
    const dss::HashMapStringString& props =  origEvent.getProperties().getContainer();
    for(dss::HashMapStringString::const_iterator iParam = props.begin(), e = props.end();
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

int dss__PropertyRemove(struct soap *soap, char* _token, std::string _propertyName, bool& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::getInstance()->getPropertySystem();
  dss::PropertyNodePtr node = propSys.getProperty(_propertyName);
  if(node == NULL) {
    return soap_sender_fault(soap, "Property does not exist", NULL);
  }

  dss::PropertyNode* pParentNode = node->getParentNode();
  if(pParentNode != NULL) {
    pParentNode->removeChild(node);
  } else {
    return soap_sender_fault(soap, "Can't remove root node", NULL);
  }

  result = true;
  return SOAP_OK;
} // dss__PropertyRemove

void transformJSONResult(std::string _name, boost::shared_ptr<dss::JSONElement> _element, dss__PropertyQueryEntry& _parent) {
  boost::shared_ptr<dss::JSONArrayBase> array = boost::dynamic_pointer_cast<dss::JSONArrayBase>(_element);
  boost::shared_ptr<dss::JSONObject> object = boost::dynamic_pointer_cast<dss::JSONObject>(_element);
  if(array != NULL) {
    dss__PropertyQueryEntry result;
    result.name = _name;
    for(int iElement = 0; iElement < array->getElementCount(); iElement++) {
      transformJSONResult("", array->getElement(iElement), result);
    }
    _parent.results.push_back(result);
  } else if(object != NULL) {
    dss__PropertyQueryEntry result;
    result.name = _name;
    for(int iElement = 0; iElement < object->getElementCount(); iElement++) {
      transformJSONResult(object->getElementName(iElement), object->getElement(iElement), result);
    }
    _parent.results.push_back(result);
  } else {
    dss__Property prop;
    prop.name = _name;
    boost::shared_ptr<dss::JSONValue<int> > intValue = boost::dynamic_pointer_cast<dss::JSONValue<int> >(_element);
    boost::shared_ptr<dss::JSONValue<bool> > boolValue = boost::dynamic_pointer_cast<dss::JSONValue<bool> >(_element);
    boost::shared_ptr<dss::JSONValue<std::string> > stringValue = boost::dynamic_pointer_cast<dss::JSONValue<std::string> >(_element);
    if(intValue != NULL) {
      prop.type = "int";
      prop.value = dss::intToString(intValue->getValue());
    } else if(boolValue != NULL) {
      prop.type = "bool";
      prop.value = boolValue->getValue() ? "true" : "false";
    } else if(stringValue != NULL) {
      prop.type = "string";
      prop.value = stringValue->getValue();
    }
    _parent.properties.push_back(prop);
  }
}

int dss__PropertyQuery(struct soap *soap, char* _token, std::string _query, dss__PropertyQueryEntry& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }

  dss::PropertySystem& propSys = dss::DSS::getInstance()->getPropertySystem();
  dss::PropertyQuery propertyQuery(propSys.getRootNode(), _query);
  boost::shared_ptr<dss::JSONObject> jsonResult = boost::shared_dynamic_cast<dss::JSONObject>(propertyQuery.run());

  result.name = "root";
  transformJSONResult("", jsonResult, result);

  return SOAP_OK;
} // dss__PropertyQuery

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
                                          *dssRef.getBusInterface().getStructureQueryBusInterface(),
                                          aptRef);
    manipulator.addDeviceToZone(dev, zone);
  } catch(std::runtime_error& _ex) {
    return soap_receiver_fault(soap, "Error handling request", NULL);
  } catch(std::invalid_argument&) {
    return soap_sender_fault(soap, "Error parsing dsid", NULL);
  }
  return SOAP_OK;
} // dss__StructureAddDeviceToZone


//==================================================== Metering

int dss__MeteringGetResolutions(struct soap *soap, char* _token, std::vector<dss__MeteringResolutions>& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Metering& metering = dss::DSS::getInstance()->getMetering();
  std::vector<boost::shared_ptr<dss::MeteringConfigChain> > meteringConfig = metering.getConfig();

  foreach(boost::shared_ptr<dss::MeteringConfigChain> pChain, meteringConfig) {
    for(int iConfig = 0; iConfig < pChain->size(); iConfig++) {
      dss__MeteringResolutions resolution;
      resolution.resolution = pChain->getResolution(iConfig);
      result.push_back(resolution);
    }
  }
  return SOAP_OK;
}

int dss__MeteringGetSeries(struct soap *soap, char* _token, std::vector<dss__MeteringSeries>& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  std::vector<boost::shared_ptr<dss::DSMeter> > dsMeters = dss::DSS::getInstance()->getApartment().getDSMeters();
  foreach(boost::shared_ptr<dss::DSMeter> dsMeter, dsMeters) {
    dss__MeteringSeries seriesEnergy;
    seriesEnergy.dsid = dsMeter->getDSID().toString();
    seriesEnergy.type = "energy";
    result.push_back(seriesEnergy);
    dss__MeteringSeries seriesEnergyDelta;
    seriesEnergyDelta.dsid = dsMeter->getDSID().toString();
    seriesEnergyDelta.type = "energyDelta";
    result.push_back(seriesEnergyDelta);
    dss__MeteringSeries seriesConsumption;
    seriesConsumption.dsid = dsMeter->getDSID().toString();
    seriesConsumption.type = "consumption";
    result.push_back(seriesConsumption);
  }
  return SOAP_OK;
}

int dss__MeteringGetValues(struct soap *soap, char* _token, char* _dsMeterID,
                           std::string _type, int _resolution, std::string* _unit,
                           int* _startTime, int* _endTime, int* _valueCount,
                           std::vector<dss__MeteringValue>& result) {
  boost::shared_ptr<dss::DSMeter> pMeter;
  int getResult = AuthorizeAndGetDSMeter(soap, _token, _dsMeterID, pMeter);
  if(getResult != SOAP_OK) {
    return getResult;
  }
  std::vector<boost::shared_ptr<dss::DSMeter> > pMeters;
  pMeters.push_back(pMeter);

  dss::Metering::SeriesTypes seriesType = dss::Metering::etConsumption;
  bool energyInWh = true;
  if(_type == "energy") {
    seriesType = dss::Metering::etEnergy;
  } else if (_type == "energyDelta") {
    seriesType = dss::Metering::etEnergyDelta;
  } else {
    if(_type != "consumption") {
      return soap_sender_fault(soap, "Expected 'energy' or 'consumption' for parameter 'type'", NULL);
    }
  }

  if ((seriesType == dss::Metering::etEnergy) || (seriesType == dss::Metering::etEnergyDelta)) {
    if(_unit != NULL) {
      if(*_unit == "Ws") {
        energyInWh = false;
      } else if (*_unit != "Wh") {
        return soap_sender_fault(soap, "Expected 'Ws' or 'Wh' for parameter 'unit'", NULL);
      }
    }
  }

  dss::DateTime startTime(dss::DateTime::NullDate);
  dss::DateTime endTime(dss::DateTime::NullDate);
  int valueCount = 0;
  if (_startTime != NULL) {
    startTime = dss::DateTime(*_startTime);
  }
  if (_endTime != NULL) {
    endTime = dss::DateTime(*_endTime);
  }
  if (_valueCount != NULL) {
    valueCount = *_valueCount;
  }

  dss::Metering& metering = dss::DSS::getInstance()->getMetering();
  boost::shared_ptr<std::deque<dss::Value> > pSeries = metering.getSeries(pMeters,
                                                                          _resolution,
                                                                          seriesType,
                                                                          energyInWh,
                                                                          startTime,
                                                                          endTime,
                                                                          valueCount);
  if(pSeries != NULL) {
    for(std::deque<dss::Value>::iterator iValue = pSeries->begin(),
        e = pSeries->end();
        iValue != e;
        ++iValue)
    {
      dss__MeteringValue value;
      value.timestamp = iValue->getTimeStamp().secondsSinceEpoch();
      value.value = iValue->getValue();
      result.push_back(value);
    }
  } else {
    std::string faultMessage = "Could not find data for '" + _type + "' and resolution '" + dss::intToString(_resolution) + "'";
    return soap_sender_fault(soap, faultMessage.c_str(), NULL);
  }
  return SOAP_OK;
}

int dss__MeteringGetLastest(struct soap *soap, char* _token, std::string _from, std::string _type, std::string* _unit, std::vector<dss__MeteringValuePerDevice>& result) {
  if(!IsAuthorized(soap, _token)) {
    return NotAuthorized(soap);
  }
  dss::Apartment& apartment = dss::DSS::getInstance()->getApartment();

  if(_type.empty() || ((_type != "consumption") && (_type != "energy"))) {
    return soap_sender_fault(soap, "Invalid or missing type parameter", NULL);
  }

  dss::MeterSetBuilder builder(apartment);
  std::vector<boost::shared_ptr<dss::DSMeter> > meters;
  try {
    meters = builder.buildSet(_from);
  } catch(std::runtime_error& e) {
    std::string failureMessage = std::string("Couldn't parse parameter 'from': '") + e.what() + "'";
    return soap_sender_fault(soap, failureMessage.c_str(), NULL);
  }

  bool isEnergy = false;
  int energyQuotient = 3600;
  if(_type == "energy") {
    isEnergy = true;
    if(_unit != NULL) {
      if(*_unit == "Ws") {
        energyQuotient = 1;
      } else if (*_unit != "Wh") {
        return soap_sender_fault(soap, "Expected 'Ws' or 'Wh' for parameter 'unit'", NULL);
      }
    }
  } else {
    if(_type != "consumption") {
      return soap_sender_fault(soap, "Expected 'energy' or 'consumption' for parameter 'type'", NULL);
    }
  }

  foreach(boost::shared_ptr<dss::DSMeter> dsMeter, meters) {
    dss__MeteringValuePerDevice value;
    value.dsid = dsMeter->getDSID().toString();
    dss::DateTime temp_date = isEnergy ? dsMeter->getCachedEnergyMeterTimeStamp() : dsMeter->getCachedPowerConsumptionTimeStamp();
    value.timestamp = temp_date.secondsSinceEpoch();
    value.value = isEnergy ? (dsMeter->getCachedEnergyMeterValue() / energyQuotient) : dsMeter->getCachedPowerConsumption();
    result.push_back(value);
  }
  return SOAP_OK;
}
