/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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

#include "webrequests.h"

#include <sstream>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "core/event.h"
#include "core/ds485/ds485proxy.h"
#include "core/ds485/framebucketcollector.h"
#include "core/sim/dssim.h"
#include "core/propertysystem.h"
#include "core/foreach.h"

#include "core/model/apartment.h"
#include "core/model/set.h"
#include "core/model/device.h"
#include "core/model/devicereference.h"
#include "core/model/zone.h"
#include "core/model/group.h"
#include "core/model/modulator.h"
#include "core/model/modelevent.h"
#include "core/model/modelmaintenance.h"

#include "core/setbuilder.h"
#include "core/structuremanipulator.h"

#include "json.h"


namespace dss {


  //=========================================== Globals

  boost::shared_ptr<JSONObject> ToJSONValue(const DeviceReference& _device) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("id", _device.getDSID().toString());
    result->addProperty("isSwitch", _device.hasSwitch());
    result->addProperty("name", _device.getName());
    result->addProperty("fid", _device.getFunctionID());
    result->addProperty("circuitID", _device.getDevice().getDSMeterID());
    result->addProperty("busID", _device.getDevice().getShortAddress());
    result->addProperty("isPresent", _device.getDevice().isPresent());
    result->addProperty("lastDiscovered", _device.getDevice().getLastDiscovered());
    result->addProperty("firstSeen", _device.getDevice().getFirstSeen());
    result->addProperty("on", _device.getDevice().isOn());
    return result;
  } // toJSONValue(DeviceReference)

  boost::shared_ptr<JSONArrayBase> ToJSONValue(const Set& _set) {
    boost::shared_ptr<JSONArrayBase> result(new JSONArrayBase());

    for(int iDevice = 0; iDevice < _set.length(); iDevice++) {
      const DeviceReference& d = _set.get(iDevice);
      result->addElement("", ToJSONValue(d));
    }
    return result;
  } // toJSONValue(Set,Name)

  boost::shared_ptr<JSONObject> ToJSONValue(const Group& _group) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("id", _group.getID());
    result->addProperty("name", _group.getName());
    result->addProperty("isPresent", _group.isPresent());

    boost::shared_ptr<JSONArrayBase> devicesArr(new JSONArrayBase());
    result->addElement("devices", devicesArr);
    Set devices = _group.getDevices();
    for(int iDevice = 0; iDevice < devices.length(); iDevice++) {
      boost::shared_ptr<JSONObject> dev(new JSONObject());
      dev->addProperty("id", devices[iDevice].getDSID().toString());
    }
    return result;
  } // toJSONValue(Group)

  boost::shared_ptr<JSONObject> ToJSONValue(Zone& _zone, bool _includeDevices = true) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("id", _zone.getID());
    std::string name = _zone.getName();
    if(name.empty()) {
      name = std::string("Zone ") + intToString(_zone.getID());
    }
    result->addProperty("name", name);
    result->addProperty("isPresent", _zone.isPresent());
    if(_zone.getFirstZoneOnDSMeter() != -1) {
      result->addProperty("firstZoneOnDSMeter", _zone.getFirstZoneOnDSMeter());
    }

    if(_includeDevices) {
      result->addElement("devices", ToJSONValue(_zone.getDevices()));
      boost::shared_ptr<JSONArrayBase> groups(new JSONArrayBase());
      result->addElement("groups", groups);
      foreach(Group* pGroup, _zone.getGroups()) {
        groups->addElement("", ToJSONValue(*pGroup));
      }
    }

    return result;
  } // toJSONValue(Zone)

  boost::shared_ptr<JSONObject> ToJSONValue(Apartment& _apartment) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    boost::shared_ptr<JSONObject> apartment(new JSONObject());
    result->addElement("apartment", apartment);
    boost::shared_ptr<JSONArrayBase> zonesArr(new JSONArrayBase());
    apartment->addElement("zones", zonesArr);

    vector<Zone*>& zones = _apartment.getZones();
    foreach(Zone* pZone, zones) {
      zonesArr->addElement("", ToJSONValue(*pZone));
    }
    return result;
  } // toJSONValue(Apartment)

  Set GetUnassignedDevices() {
    Apartment& apt = DSS::getInstance()->getApartment();
    Set devices = apt.getZone(0).getDevices();

    vector<Zone*>& zones = apt.getZones();
    for(vector<Zone*>::iterator ipZone = zones.begin(), e = zones.end();
        ipZone != e; ++ipZone)
    {
      Zone* pZone = *ipZone;
      if(pZone->getID() == 0) {
        // zone 0 holds all devices, so we're going to skip it
        continue;
      }

      devices = devices.remove(pZone->getDevices());
    }

    return devices;
  } // getUnassignedDevices


  //=========================================== DeviceInterfaceRequestHandler

  boost::shared_ptr<JSONObject> DeviceInterfaceRequestHandler::handleDeviceInterfaceRequest(const RestfulRequest& _request, IDeviceInterface* _interface) {
    assert(_interface != NULL);
    if(_request.getMethod() == "turnOn") {
      _interface->turnOn();
      return success();
    } else if(_request.getMethod() == "turnOff") {
      _interface->turnOff();
      return success();
    } else if(_request.getMethod() == "increaseValue") {
      _interface->increaseValue();
      return success();
    } else if(_request.getMethod() == "decreaseValue") {
      _interface->decreaseValue();
      return success();
    } else if(_request.getMethod() == "startDim") {
      std::string direction = _request.getParameter("direction");
      if(direction == "up") {
        _interface->startDim(true);
      } else {
        _interface->startDim(false);
      }
      return success();
    } else if(_request.getMethod() == "endDim") {
      _interface->endDim();
      return success();
    } else if(_request.getMethod() == "setValue") {
      std::string valueStr = _request.getParameter("value");
      int value = strToIntDef(valueStr, -1);
      if(value == -1) {
        return failure("invalid or missing parameter value: '" + valueStr + "'");
      } else {
        _interface->setValue(value);
      }
      return success();
    } else if(_request.getMethod() == "callScene") {
      std::string sceneStr = _request.getParameter("sceneNr");
      int sceneID = strToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        _interface->callScene(sceneID);
      } else {
        return failure("invalid sceneNr: '" + sceneStr + "'");
      }
      return success();
    } else if(_request.getMethod() == "saveScene") {
      std::string sceneStr = _request.getParameter("sceneNr");
      int sceneID = strToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        _interface->saveScene(sceneID);
      } else {
        return failure("invalid sceneNr: '" + sceneStr + "'");
      }
      return success();
    } else if(_request.getMethod() == "undoScene") {
      std::string sceneStr = _request.getParameter("sceneNr");
      int sceneID = strToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        _interface->undoScene(sceneID);
      } else {
        return failure("invalid sceneNr: '" + sceneStr + "'");
      }
      return success();
    } else if(_request.getMethod() == "getConsumption") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("consumption", _interface->getPowerConsumption());
      return success(resultObj);
    }
    throw std::runtime_error("Unknown function");
  } // handleRequest

  bool DeviceInterfaceRequestHandler::isDeviceInterfaceCall(const RestfulRequest& _request) {
    return _request.getMethod() == "turnOn"
        || _request.getMethod() == "turnOff"
        || _request.getMethod() == "increaseValue"
        || _request.getMethod() == "decreaseValue"
        || _request.getMethod() == "startDim"
        || _request.getMethod() == "endDim"
        || _request.getMethod() == "setValue"
        || _request.getMethod() == "callScene"
        || _request.getMethod() == "saveScene"
        || _request.getMethod() == "undoScene"
        || _request.getMethod() == "getConsumption";
  } // isDeviceInterfaceCall


  //=========================================== ApartmentRequestHandler
  
  boost::shared_ptr<JSONObject> ApartmentRequestHandler::jsonHandleRequest(const RestfulRequest& _request, Session* _session) {
    std::string errorMessage;
    if(_request.getMethod() == "getConsumption") {
      int accumulatedConsumption = 0;
      foreach(DSMeter* pDSMeter, getDSS().getApartment().getDSMeters()) {
        accumulatedConsumption += pDSMeter->getPowerConsumption();
      }
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("consumption", accumulatedConsumption);
      return success(resultObj);
    } else if(isDeviceInterfaceCall(_request)) {
      IDeviceInterface* interface = NULL;
      std::string groupName = _request.getParameter("groupName");
      std::string groupIDString = _request.getParameter("groupID");
      if(!groupName.empty()) {
        try {
          Group& grp = getDSS().getApartment().getGroup(groupName);
          interface = &grp;
        } catch(std::runtime_error& e) {
          return failure("Could not find group with name '" + groupName + "'");
        }
      } else if(!groupIDString.empty()) {
        try {
          int groupID = strToIntDef(groupIDString, -1);
          if(groupID != -1) {
            Group& grp = getDSS().getApartment().getGroup(groupID);
            interface = &grp;
          } else {
            return failure("Could not parse group id '" + groupIDString + "'");
          }
        } catch(std::runtime_error& e) {
          return failure("Could not find group with ID '" + groupIDString + "'");
        }
      }
      if(interface != NULL) {
        interface = &getDSS().getApartment().getGroup(GroupIDBroadcast);
        return success();
      }
      return failure("No interface");
    } else {
      if(_request.getMethod() == "getStructure") {
        return success(ToJSONValue(getDSS().getApartment()));
      } else if(_request.getMethod() == "getDevices") {
        Set devices;
        if(_request.getParameter("unassigned").empty()) {
          devices = getDSS().getApartment().getDevices();
        } else {
          devices = GetUnassignedDevices();
        }

        return success(ToJSONValue(devices));
/* TODO: re-enable
      } else if(_request.getMethod() == "login") {
        int token = m_LastSessionID;
        m_Sessions[token] = Session(token);
        return "{" + ToJSONValue("token") + ": " + ToJSONValue(token) + "}";
*/
      } else if(_request.getMethod() == "getCircuits") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        boost::shared_ptr<JSONArrayBase> circuits(new JSONArrayBase());

        resultObj->addElement("circuits", circuits);
        vector<DSMeter*>& dsMeters = getDSS().getApartment().getDSMeters();
        foreach(DSMeter* dsMeter, dsMeters) {
          boost::shared_ptr<JSONObject> circuit(new JSONObject());
          circuits->addElement("", circuit);
          circuit->addProperty("name", dsMeter->getName());
          circuit->addProperty("dsid", dsMeter->getDSID().toString());
          circuit->addProperty("busid", dsMeter->getBusID());
          circuit->addProperty("hwVersion", dsMeter->getHardwareVersion());
          circuit->addProperty("swVersion", dsMeter->getSoftwareVersion());
          circuit->addProperty("hwName", dsMeter->getHardwareName());
          circuit->addProperty("deviceType", dsMeter->getDeviceType());
          circuit->addProperty("isPresent", dsMeter->isPresent());
        }
        return success(resultObj);
      } else if(_request.getMethod() == "getName") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("name", getDSS().getApartment().getName());
        return success(resultObj);
      } else if(_request.getMethod() == "setName") {
        getDSS().getApartment().setName(_request.getParameter("newName"));
        return success();
      } else if(_request.getMethod() == "rescan") {
        std::vector<DSMeter*> mods = getDSS().getApartment().getDSMeters();
        foreach(DSMeter* pDSMeter, mods) {
          pDSMeter->setIsValid(false);
        }
        return success();
      } else {
        throw std::runtime_error("Unhandled function");
      }
    }
  } // handleApartmentCall


  //=========================================== ZoneRequestHandler

  boost::shared_ptr<JSONObject> ZoneRequestHandler::jsonHandleRequest(const RestfulRequest& _request, Session* _session) {
    bool ok = true;
    std::string errorMessage;
    std::string zoneName = _request.getParameter("name");
    std::string zoneIDString = _request.getParameter("id");
    Zone* pZone = NULL;
    if(!zoneIDString.empty()) {
      int zoneID = strToIntDef(zoneIDString, -1);
      if(zoneID != -1) {
        try {
          Zone& zone = getDSS().getApartment().getZone(zoneID);
          pZone = &zone;
        } catch(std::runtime_error& e) {
          ok = false;
          errorMessage = "Could not find zone with id '" + zoneIDString + "'";
        }
      } else {
        ok = false;
        errorMessage = "Could not parse id '" + zoneIDString + "'";
      }
    } else if(!zoneName.empty()) {
      try {
        Zone& zone = getDSS().getApartment().getZone(zoneName);
        pZone = &zone;
      } catch(std::runtime_error& e) {
        ok = false;
        errorMessage = "Could not find zone named '" + zoneName + "'";
      }
    } else {
      ok = false;
      errorMessage = "Need parameter name or id to identify zone";
    }
    if(ok) {
      Group* pGroup = NULL;
      std::string groupName = _request.getParameter("groupName");
      std::string groupIDString = _request.getParameter("groupID");
      if(!groupName.empty()) {
        try {
          pGroup = pZone->getGroup(groupName);
          if(pGroup == NULL) {
            // TODO: this might better be done by the zone
            throw std::runtime_error("dummy");
          }
        } catch(std::runtime_error& e) {
          errorMessage = "Could not find group with name '" + groupName + "'";
          ok = false;
        }
      } else if(!groupIDString.empty()) {
        try {
          int groupID = strToIntDef(groupIDString, -1);
          if(groupID != -1) {
            pGroup = pZone->getGroup(groupID);
            if(pGroup == NULL) {
              // TODO: this might better be done by the zone
              throw std::runtime_error("dummy");
            }
          } else {
            errorMessage = "Could not parse group id '" + groupIDString + "'";
            ok = false;
          }
        } catch(std::runtime_error& e) {
          errorMessage = "Could not find group with ID '" + groupIDString + "'";
          ok = false;
        }
      }
      if(ok) {
        if(isDeviceInterfaceCall(_request)) {
          IDeviceInterface* interface = NULL;
          if(pGroup != NULL) {
            interface = pGroup;
          }
          if(ok) {
            if(interface == NULL) {
              interface = pZone;
            }
            return handleDeviceInterfaceRequest(_request, interface);
          }
        } else if(_request.getMethod() == "getLastCalledScene") {
          int lastScene = 0;
          if(pGroup != NULL) {
            lastScene = pGroup->getLastCalledScene();
          } else if(pZone != NULL) {
            lastScene = pZone->getGroup(0)->getLastCalledScene();
          } else {
            // should never reach here because ok, would be false
            assert(false);
          }
          boost::shared_ptr<JSONObject> resultObj(new JSONObject());
          resultObj->addProperty("scene", lastScene);
          return success(resultObj);
        } else if(_request.getMethod() == "getName") {
          boost::shared_ptr<JSONObject> resultObj(new JSONObject());
          resultObj->addProperty("name", pZone->getName());
          return success(resultObj);
        } else if(_request.getMethod() == "setName") {
          pZone->setName(_request.getParameter("newName"));
          return success();
        } else {
          throw std::runtime_error("Unhandled function");
        }
      }
    }
    if(!ok) {
      return failure(errorMessage);
    } else {
      return success(); // TODO: check this, we shouldn't get here
    }
  } // handleRequest


  //=========================================== DeviceRequestHandler

  boost::shared_ptr<JSONObject> DeviceRequestHandler::jsonHandleRequest(const RestfulRequest& _request, Session* _session) {
    bool ok = true;
    std::string errorMessage;
    std::string deviceName = _request.getParameter("name");
    std::string deviceDSIDString = _request.getParameter("dsid");
    Device* pDevice = NULL;
    if(!deviceDSIDString.empty()) {
      dsid_t deviceDSID = dsid_t::fromString(deviceDSIDString);
      if(!(deviceDSID == NullDSID)) {
        try {
          Device& device = getDSS().getApartment().getDeviceByDSID(deviceDSID);
          pDevice = &device;
        } catch(std::runtime_error& e) {
          ok = false;
          errorMessage = "Could not find device with dsid '" + deviceDSIDString + "'";
        }
      } else {
        ok = false;
        errorMessage = "Could not parse dsid '" + deviceDSIDString + "'";
      }
    } else if(!deviceName.empty()) {
      try {
        Device& device = getDSS().getApartment().getDeviceByName(deviceName);
        pDevice = &device;
      } catch(std::runtime_error&  e) {
        ok = false;
        errorMessage = "Could not find device named '" + deviceName + "'";
      }
    } else {
      ok = false;
      errorMessage = "Need parameter name or dsid to identify device";
    }
    if(ok) {
      if(isDeviceInterfaceCall(_request)) {
        return handleDeviceInterfaceRequest(_request, pDevice);
      } else if(_request.getMethod() == "getGroups") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        int numGroups = pDevice->getGroupsCount();

        boost::shared_ptr<JSONArrayBase> groups(new JSONArrayBase());
        resultObj->addElement("groups", groups);
        for(int iGroup = 0; iGroup < numGroups; iGroup++) {
          try {
            Group& group = pDevice->getGroupByIndex(iGroup);
            boost::shared_ptr<JSONObject> groupObj(new JSONObject());
            groups->addElement("", groupObj);

            groupObj->addProperty("id", group.getID());
            if(!group.getName().empty()) {
              groupObj->addProperty("name", group.getName());
            }
          } catch(std::runtime_error&) {
            Logger::getInstance()->log("DeviceRequestHandler: Group only present at device level");
          }
        }
        return success(resultObj);
      } else if(_request.getMethod() == "getState") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("isOn", pDevice->isOn());
        return success(resultObj);
      } else if(_request.getMethod() == "getName") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("name", pDevice->getName());
        return success(resultObj);
      } else if(_request.getMethod() == "setName") {
        pDevice->setName(_request.getParameter("newName"));
        return success();
      } else if(_request.getMethod() == "enable") {
        pDevice->enable();
        return success();
      } else if(_request.getMethod() == "disable") {
        pDevice->disable();
        return success();
      } else if(_request.getMethod() == "setRawValue") {
        int value = strToIntDef(_request.getParameter("value"), -1);
        if(value == -1) {
          return failure("Invalid or missing parameter 'value'");
        }
        int parameterID = strToIntDef(_request.getParameter("parameterID"), -1);
        if(parameterID == -1) {
          return failure("Invalid or missing parameter 'parameterID'");
        }
        int size = strToIntDef(_request.getParameter("size"), -1);
        if(size == -1) {
          return failure("Invalid or missing parameter 'size'");
        }

        pDevice->setRawValue(value, parameterID, size);
        return success();
      } else {
        throw std::runtime_error("Unhandled function");
      }
    } else {
      if(ok) {
        return success();
      } else {
        return failure(errorMessage);
      }
    }
  } // handleRequest


  //=========================================== CircuitRequestHandler

  boost::shared_ptr<JSONObject> CircuitRequestHandler::jsonHandleRequest(const RestfulRequest& _request, Session* _session) {
    std::string idString = _request.getParameter("id");
    if(idString.empty()) {
      return failure("missing parameter id");
    }
    dsid_t dsid = dsid_t::fromString(idString);
    if(dsid == NullDSID) {
      return failure("could not parse dsid");
    }
    try {
      DSMeter& dsMeter = getDSS().getApartment().getDSMeterByDSID(dsid);
      if(_request.getMethod() == "getName") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("name", dsMeter.getName());
        return success(resultObj);
      } else if(_request.getMethod() == "setName") {
        dsMeter.setName(_request.getParameter("newName"));
        return success();
      } else if(_request.getMethod() == "getEnergyBorder") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("orange", dsMeter.getEnergyLevelOrange());
        resultObj->addProperty("red", dsMeter.getEnergyLevelRed());
        return success(resultObj);
      } else if(_request.getMethod() == "getConsumption") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("consumption", dsMeter.getPowerConsumption());
        return success(resultObj);
      } else if(_request.getMethod() == "getEnergyMeterValue") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("metervalue", dsMeter.getEnergyMeterValue());
        return success(resultObj);
      } else if(_request.getMethod() == "rescan") {
        dsMeter.setIsValid(false);
        return success();
      } else {
        throw std::runtime_error("Unhandled function");
      }
    } catch(std::runtime_error&) {
      return failure("could not find dsMeter with given dsid");
    }
  } // handleRequest


  //=========================================== SetRequestHandler

  boost::shared_ptr<JSONObject> SetRequestHandler::jsonHandleRequest(const RestfulRequest& _request, Session* _session) {
    if(_request.getMethod() == "fromApartment") {
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("self", ".");
      return success(resultObj);
    } else {
      std::string self = trim(_request.getParameter("self"));
      if(self.empty()) {
        return failure("missing parameter 'self'");
      }

      if(_request.getMethod() == "byZone") {
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        if(!_request.getParameter("zoneID").empty()) {
          additionalPart += "zone(" + _request.getParameter("zoneID") + ")";
        } else if(!_request.getParameter("zoneName").empty()) {
          additionalPart += _request.getParameter("zoneName");
        } else {
          return failure("missing either zoneID or zoneName");
        }

        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("self", self + additionalPart);
        return success(resultObj);
      } else if(_request.getMethod() == "byGroup") {
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        if(!_request.getParameter("groupID").empty()) {
          additionalPart += "group(" + _request.getParameter("groupID") + ")";
        } else if(!_request.getParameter("groupName").empty()) {
          additionalPart += _request.getParameter("groupName");
        } else {
          return failure("missing either groupID or groupName");
        }

        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("self", self + additionalPart);
        return success(resultObj);
      } else if(_request.getMethod() == "byDSID") {
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        if(!_request.getParameter("dsid").empty()) {
          additionalPart += "dsid(" + _request.getParameter("dsid") + ")";
        } else {
          return failure("missing parameter dsid");
        }

        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("self", self + additionalPart);
        return success(resultObj);
      } else if(_request.getMethod() == "getDevices") {
        SetBuilder builder(getDSS().getApartment());
        Set set = builder.buildSet(self, NULL);
        return success(ToJSONValue(set));
      } else if(_request.getMethod() == "add") {
        std::string other = _request.getParameter("other");
        if(other.empty()) {
          return failure("missing parameter other");
        }
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        additionalPart += "add(" + other + ")";

        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("self", self + additionalPart);
        return success(resultObj);
      } else if(_request.getMethod() == "subtract") {
        std::string other = _request.getParameter("other");
        if(other.empty()) {
          return failure("missing parameter other");
        }
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        additionalPart += "subtract(" + other + ")";

        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("self", self + additionalPart);
        return success(resultObj);
      } else if(isDeviceInterfaceCall(_request)) {
        SetBuilder builder(getDSS().getApartment());
        Set set = builder.buildSet(self, NULL);
        return handleDeviceInterfaceRequest(_request, &set);
      } else {
        throw std::runtime_error("Unhandled function");
      }
    }
  } // handleRequest



  //================================================== WebServerRequestHandlerJSON

  std::string WebServerRequestHandlerJSON::handleRequest(const RestfulRequest& _request, Session* _session) {
    boost::shared_ptr<JSONObject> response = jsonHandleRequest(_request, _session);
    if(response != NULL) {
      return response->toString();
    }
    return "";
  } // handleRequest

  boost::shared_ptr<JSONObject> WebServerRequestHandlerJSON::success() {
    return success(boost::shared_ptr<JSONObject>());
  }

  boost::shared_ptr<JSONObject> WebServerRequestHandlerJSON::success(const std::string& _message) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("ok", true);
    result->addProperty("message", _message);
    return result;
  }

  boost::shared_ptr<JSONObject> WebServerRequestHandlerJSON::success(boost::shared_ptr<JSONElement> _innerResult) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("ok", true);
    if(_innerResult != NULL) {
      result->addElement("result", _innerResult);
    }
    return result;
  }

  boost::shared_ptr<JSONObject> WebServerRequestHandlerJSON::failure(const std::string& _message) {
    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("ok", false);
    if(!_message.empty()) {
      result->addProperty("message", _message);
    }
    Logger::getInstance()->log("JSON call failed: '" + _message + "'");
    return result;
  }

} // namespace dss
