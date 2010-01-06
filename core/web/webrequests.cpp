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
#include <boost/filesystem.hpp>

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

#include "core/metering/metering.h"
#include "core/metering/series.h"
#include "core/metering/seriespersistence.h"
#include "core/setbuilder.h"
#include "core/structuremanipulator.h"


namespace dss {

  //=========================================== Globals

  std::string ToJSONValue(const int& _value) {
    return intToString(_value);
  } // toJSONValue(int)

  std::string ToJSONValue(const double& _value) {
    return doubleToString(_value);
  } // toJSONValue(double)

  std::string ToJSONValue(const bool& _value) {
    if(_value) {
      return "true";
    } else {
      return "false";
    }
  } // toJSONValue(bool)

  std::string ToJSONValue(const std::string& _value) {
    return std::string("\"") + _value + '"';
  } // toJSONValue(const std::string&)

  std::string ToJSONValue(const char* _value) {
    return ToJSONValue(std::string(_value));
  } // toJSONValue(const char*)

  std::string ResultToJSON(const bool _ok, const std::string& _message = "") {
    std::ostringstream sstream;
    sstream << "{ " << ToJSONValue("ok") << ":" << ToJSONValue(_ok);
    if(!_message.empty()) {
      sstream << ", " << ToJSONValue("message") << ":" << ToJSONValue(_message);
    }
    sstream << "}";
    if(!_ok) {
      Logger::getInstance()->log("JSON call failed: '" + _message + "'");
    }
    return sstream.str();
  } // resultToJSON

  std::string JSONOk(const std::string& _innerResult = "") {
    std::ostringstream sstream;
    sstream << "{ " << ToJSONValue("ok") << ":" << ToJSONValue(true);
    if(!_innerResult.empty()) {
      sstream << ", " << ToJSONValue("result")<<  ": " << _innerResult;
    }
    sstream << " }";
    return sstream.str();
  }

  std::string ToJSONValue(const DeviceReference& _device) {
    std::ostringstream sstream;
    sstream << "{ \"id\": \"" << _device.getDSID().toString() << "\""
            << ", \"isSwitch\": " << ToJSONValue(_device.hasSwitch())
            << ", \"name\": " << ToJSONValue(_device.getDevice().getName())
            << ", \"fid\": " << ToJSONValue(_device.getDevice().getFunctionID())
            << ", \"circuitID\":" << ToJSONValue(_device.getDevice().getDSMeterID())
            << ", \"busID\":"  << ToJSONValue(_device.getDevice().getShortAddress())
            << ", \"isPresent\":"  << ToJSONValue(_device.getDevice().isPresent())
            << ", \"lastDiscovered\":"  << ToJSONValue(int(_device.getDevice().getLastDiscovered().secondsSinceEpoch()))
            << ", \"firstSeen\":"  << ToJSONValue(int(_device.getDevice().getFirstSeen().secondsSinceEpoch()))
            << ", \"on\": " << ToJSONValue(_device.isOn()) << " }";
    return sstream.str();
  } // toJSONValue(DeviceReference)

  std::string ToJSONValue(const Set& _set, const std::string& _arrayName) {
    std::ostringstream sstream;
    sstream << ToJSONValue(_arrayName) << ":[";
    bool firstDevice = true;
    for(int iDevice = 0; iDevice < _set.length(); iDevice++) {
      const DeviceReference& d = _set.get(iDevice);
      if(firstDevice) {
        firstDevice = false;
      } else {
        sstream << ",";
      }
      sstream << ToJSONValue(d);
    }
    sstream << "]";
    return sstream.str();
  } // toJSONValue(Set,Name)

  std::string ToJSONValue(const Group& _group) {
    std::ostringstream sstream;
    sstream << "{ " << ToJSONValue("id") << ": " << ToJSONValue(_group.getID()) << ",";
    sstream << ToJSONValue("name") << ": " << ToJSONValue(_group.getName()) << ", ";
    sstream << ToJSONValue("isPresent") << ": " << ToJSONValue(_group.isPresent()) << ", ";
    sstream << ToJSONValue("devices") << ": [";
    Set devices = _group.getDevices();
    bool first = true;
    for(int iDevice = 0; iDevice < devices.length(); iDevice++) {
      if(!first) {
        sstream << " , ";
      } else {
        first = false;
      }
      sstream << " { " << ToJSONValue("id") << ": " << ToJSONValue(devices[iDevice].getDSID().toString()) << " }";
    }
    sstream << "]} ";
    return sstream.str();
  } // toJSONValue(Group)

  std::string ToJSONValue(Zone& _zone, bool _includeDevices = true) {
    std::ostringstream sstream;
    sstream << "{ \"id\": " << _zone.getID() << ",";
    std::string name = _zone.getName();
    if(name.size() == 0) {
      name = std::string("Zone ") + intToString(_zone.getID());
    }
    sstream << ToJSONValue("name") << ": " << ToJSONValue(name) << ", ";
    sstream << ToJSONValue("isPresent") << ": " << ToJSONValue(_zone.isPresent());
    if(_zone.getFirstZoneOnDSMeter() != -1) {
      sstream << ", " << ToJSONValue("firstZoneOnDSMeter")
              << ": " << ToJSONValue(_zone.getFirstZoneOnDSMeter());
    }

    if(_includeDevices) {
      sstream << ", ";
      Set devices = _zone.getDevices();
      sstream << ToJSONValue(devices, "devices");
      sstream << "," << ToJSONValue("groups") << ": [";
      bool first = true;
      foreach(Group* pGroup, _zone.getGroups()) {
        if(!first) {
          sstream << ",";
        }
        first = false;
        sstream  << ToJSONValue(*pGroup);
      }
      sstream << "] ";
    }

    sstream << "} ";
    return sstream.str();
  } // toJSONValue(Zone)

  std::string ToJSONValue(Apartment& _apartment) {
    std::ostringstream sstream;
    sstream << "{ \"apartment\": { \"zones\": [";
    vector<Zone*>& zones = _apartment.getZones();
    bool first = true;
    for(vector<Zone*>::iterator ipZone = zones.begin(), e = zones.end();
        ipZone != e; ++ipZone)
    {
      Zone* pZone = *ipZone;
      if(!first) {
        sstream << ", ";
      } else {
        first = false;
      }
      sstream << ToJSONValue(*pZone);
    }
    sstream << "]} }";
    return sstream.str();
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

  template<class t>
  std::string ToJSONArray(const vector<t>& _v);

  template<>
  std::string ToJSONArray(const vector<int>& _v) {
    std::ostringstream arr;
    arr << "[";
    bool first = true;
    vector<int>::const_iterator iV;
    vector<int>::const_iterator e;
    for(iV = _v.begin(), e = _v.end();
        iV != e; ++iV)
    {
      if(!first) {
        arr << ",";
      }
      arr << ToJSONValue(*iV);
      first = false;
    }
    arr << "]";
    return arr.str();
  } // toJSONArray<int>
  
  //=========================================== DeviceInterfaceRequestHandler

  std::string DeviceInterfaceRequestHandler::handleDeviceInterfaceRequest(const RestfulRequest& _request, IDeviceInterface* _interface) {
    bool ok = true;
    std::string errorString;
    assert(_interface != NULL);
    if(_request.getMethod() == "turnOn") {
      _interface->turnOn();
    } else if(_request.getMethod() == "turnOff") {
      _interface->turnOff();
    } else if(_request.getMethod() == "increaseValue") {
      _interface->increaseValue();
    } else if(_request.getMethod() == "decreaseValue") {
      _interface->decreaseValue();
    } else if(_request.getMethod() == "startDim") {
      std::string direction = _request.getParameter("direction");
      if(direction == "up") {
        _interface->startDim(true);
      } else {
        _interface->startDim(false);
      }
    } else if(_request.getMethod() == "endDim") {
      _interface->endDim();
    } else if(_request.getMethod() == "setValue") {
      std::string valueStr = _request.getParameter("value");
      int value = strToIntDef(valueStr, -1);
      if(value == -1) {
        errorString = "invalid or missing parameter value: '" + valueStr + "'";
        ok = false;
      } else {
        _interface->setValue(value);
      }
    } else if(_request.getMethod() == "callScene") {
      std::string sceneStr = _request.getParameter("sceneNr");
      int sceneID = strToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        _interface->callScene(sceneID);
      } else {
        errorString = "invalid sceneNr: '" + sceneStr + "'";
        ok = false;
      }
    } else if(_request.getMethod() == "saveScene") {
      std::string sceneStr = _request.getParameter("sceneNr");
      int sceneID = strToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        _interface->saveScene(sceneID);
      } else {
        errorString = "invalid sceneNr: '" + sceneStr + "'";
        ok = false;
      }
    } else if(_request.getMethod() == "undoScene") {
      std::string sceneStr = _request.getParameter("sceneNr");
      int sceneID = strToIntDef(sceneStr, -1);
      if(sceneID != -1) {
        _interface->undoScene(sceneID);
      } else {
        errorString = "invalid sceneNr: '" + sceneStr + "'";
        ok = false;
      }
    } else if(_request.getMethod() == "getConsumption") {
      return "{ " + ToJSONValue("consumption") + ": " +  uintToString(_interface->getPowerConsumption()) +"}";
    }
    return ResultToJSON(ok, errorString);
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
  
  std::string ApartmentRequestHandler::handleRequest(const RestfulRequest& _request, Session* _session) {
    bool ok = true;
    std::string errorMessage;
    if(_request.getMethod() == "getConsumption") {
      int accumulatedConsumption = 0;
      foreach(DSMeter* pDSMeter, getDSS().getApartment().getDSMeters()) {
        accumulatedConsumption += pDSMeter->getPowerConsumption();
      }
      return "{ " + ToJSONValue("consumption") + ": " +  uintToString(accumulatedConsumption) +"}";
    } else if(isDeviceInterfaceCall(_request)) {
      IDeviceInterface* interface = NULL;
      std::string groupName = _request.getParameter("groupName");
      std::string groupIDString = _request.getParameter("groupID");
      if(!groupName.empty()) {
        try {
          Group& grp = getDSS().getApartment().getGroup(groupName);
          interface = &grp;
        } catch(std::runtime_error& e) {
          errorMessage = "Could not find group with name '" + groupName + "'";
          ok = false;
        }
      } else if(!groupIDString.empty()) {
        try {
          int groupID = strToIntDef(groupIDString, -1);
          if(groupID != -1) {
            Group& grp = getDSS().getApartment().getGroup(groupID);
            interface = &grp;
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
        if(interface == NULL) {
          interface = &getDSS().getApartment().getGroup(GroupIDBroadcast);
        }
        return handleDeviceInterfaceRequest(_request, interface);
      } else {
        std::ostringstream sstream;
        sstream << "{ ok: " << ToJSONValue(ok) << ", message: " << ToJSONValue(errorMessage) << " }";
        return sstream.str();
      }
    } else {
      std::string result;
      if(_request.getMethod() == "getStructure") {
        result = ToJSONValue(getDSS().getApartment());
      } else if(_request.getMethod() == "getDevices") {
        Set devices;
        if(_request.getParameter("unassigned").empty()) {
          devices = getDSS().getApartment().getDevices();
        } else {
          devices = GetUnassignedDevices();
        }

        result = "{" + ToJSONValue(devices, "devices") + "}";
/* TODO: re-enable
      } else if(_request.getMethod() == "login") {
        int token = m_LastSessionID;
        m_Sessions[token] = Session(token);
        return "{" + ToJSONValue("token") + ": " + ToJSONValue(token) + "}";
*/
      } else if(_request.getMethod() == "getCircuits") {
        std::ostringstream sstream;
        sstream << "{ " << ToJSONValue("circuits") << ": [";
        bool first = true;
        vector<DSMeter*>& dsMeters = getDSS().getApartment().getDSMeters();
        foreach(DSMeter* dsMeter, dsMeters) {
          if(!first) {
            sstream << ",";
          }
          first = false;
          sstream << "{ " << ToJSONValue("name") << ": " << ToJSONValue(dsMeter->getName());
          sstream << ", " << ToJSONValue("dsid") << ": " << ToJSONValue(dsMeter->getDSID().toString());
          sstream << ", " << ToJSONValue("busid") << ": " << ToJSONValue(dsMeter->getBusID());
          sstream << ", " << ToJSONValue("hwVersion") << ": " << ToJSONValue(dsMeter->getHardwareVersion());
          sstream << ", " << ToJSONValue("swVersion") << ": " << ToJSONValue(dsMeter->getSoftwareVersion());
          sstream << ", " << ToJSONValue("hwName") << ": " << ToJSONValue(dsMeter->getHardwareName());
          sstream << ", " << ToJSONValue("deviceType") << ": " << ToJSONValue(dsMeter->getDeviceType());
          sstream << ", " << ToJSONValue("isPresent") << ": " << ToJSONValue(dsMeter->isPresent());
          sstream << "}";
        }
        sstream << "]}";
        return JSONOk(sstream.str());
      } else if(_request.getMethod() == "getName") {
        std::ostringstream sstream;
        sstream << "{" << ToJSONValue("name") << ":" << ToJSONValue(getDSS().getApartment().getName()) << "}";
        return JSONOk(sstream.str());
      } else if(_request.getMethod() == "setName") {
        getDSS().getApartment().setName(_request.getParameter("newName"));
        result = ResultToJSON(true);
      } else if(_request.getMethod() == "rescan") {
        std::vector<DSMeter*> mods = getDSS().getApartment().getDSMeters();
        foreach(DSMeter* pDSMeter, mods) {
          pDSMeter->setIsValid(false);
        }
        result = ResultToJSON(true);
      } else {
        throw std::runtime_error("Unhandled function");
      }
      return result;
    }
  } // handleApartmentCall


  //=========================================== ZoneRequestHandler

  std::string ZoneRequestHandler::handleRequest(const RestfulRequest& _request, Session* _session) {
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
          std::ostringstream sstream;
          sstream << "{" << ToJSONValue("scene") << ":" << ToJSONValue(lastScene) << "}";
          return JSONOk(sstream.str());
        } else if(_request.getMethod() == "getName") {
          std::ostringstream sstream;
          sstream << "{" << ToJSONValue("name") << ":" << ToJSONValue(pZone->getName()) << "}";
          return JSONOk(sstream.str());
        } else if(_request.getMethod() == "setName") {
          pZone->setName(_request.getParameter("newName"));
          return ResultToJSON(true);
        } else {
          throw std::runtime_error("Unhandled function");
        }
      }
    }
    if(!ok) {
      return ResultToJSON(ok, errorMessage);
    } else {
      return "";
    }
  } // handleRequest


  //=========================================== DeviceRequestHandler

  std::string DeviceRequestHandler::handleRequest(const RestfulRequest& _request, Session* _session) {
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
        int numGroups = pDevice->getGroupsCount();
        std::ostringstream sstream;
        sstream << "{ " << ToJSONValue("groups") << ": [";
        bool first = true;
        for(int iGroup = 0; iGroup < numGroups; iGroup++) {
          if(!first) {
            sstream << ", ";
          }
          first = false;
          try {
            Group& group = pDevice->getGroupByIndex(iGroup);
            sstream << "{ " << ToJSONValue("id") << ":" << group.getID();
            if(!group.getName().empty()) {
              sstream << ", " << ToJSONValue("name") << ":" << ToJSONValue(group.getName());
            }
          } catch(std::runtime_error&) {
            Logger::getInstance()->log("DeviceRequestHandler: Group only present at device level");
          }
          sstream << "}";
        }
        sstream << "]}";
        return sstream.str();
      } else if(_request.getMethod() == "getState") {
        std::ostringstream sstream;
        sstream << "{ " << ToJSONValue("isOn") << ":" << ToJSONValue(pDevice->isOn()) << " }";
        return JSONOk(sstream.str());
      } else if(_request.getMethod() == "getName") {
        std::ostringstream sstream;
        sstream << "{ " << ToJSONValue("name") << ":" << ToJSONValue(pDevice->getName()) << " }";
        return JSONOk(sstream.str());
      } else if(_request.getMethod() == "setName") {
        pDevice->setName(_request.getParameter("newName"));
        return ResultToJSON(true);
      } else if(_request.getMethod() == "enable") {
        pDevice->enable();
        return ResultToJSON(true);
      } else if(_request.getMethod() == "disable") {
        pDevice->disable();
        return ResultToJSON(true);
      } else if(_request.getMethod() == "setRawValue") {
        int value = strToIntDef(_request.getParameter("value"), -1);
        if(value == -1) {
          return ResultToJSON(false, "Invalid or missing parameter 'value'");
        }
        int parameterID = strToIntDef(_request.getParameter("parameterID"), -1);
        if(parameterID == -1) {
          return ResultToJSON(false, "Invalid or missing parameter 'parameterID'");
        }
        int size = strToIntDef(_request.getParameter("size"), -1);
        if(size == -1) {
          return ResultToJSON(false, "Invalid or missing parameter 'size'");
        }

        pDevice->setRawValue(value, parameterID, size);
        return JSONOk();
      } else {
        throw std::runtime_error("Unhandled function");
      }
    } else {
      return ResultToJSON(ok, errorMessage);
    }
  } // handleRequest


  //=========================================== CircuitRequestHandler

  std::string CircuitRequestHandler::handleRequest(const RestfulRequest& _request, Session* _session) {
    std::string idString = _request.getParameter("id");
    if(idString.empty()) {
      return ResultToJSON(false, "missing parameter id");
    }
    dsid_t dsid = dsid_t::fromString(idString);
    if(dsid == NullDSID) {
      return ResultToJSON(false, "could not parse dsid");
    }
    try {
      DSMeter& dsMeter = getDSS().getApartment().getDSMeterByDSID(dsid);
      if(_request.getMethod() == "getName") {
        return JSONOk("{ " + ToJSONValue("name") + ": " + ToJSONValue(dsMeter.getName()) + "}");
      } else if(_request.getMethod() == "setName") {
        dsMeter.setName(_request.getParameter("newName"));
        return ResultToJSON(true);
      } else if(_request.getMethod() == "getEnergyBorder") {
        std::ostringstream sstream;
        sstream << "{" << ToJSONValue("orange") << ":" << ToJSONValue(dsMeter.getEnergyLevelOrange());
        sstream << "," << ToJSONValue("red") << ":" << ToJSONValue(dsMeter.getEnergyLevelRed());
        sstream << "}";
        return JSONOk(sstream.str());
      } else if(_request.getMethod() == "getConsumption") {
        return JSONOk("{ " + ToJSONValue("consumption") + ": " +  uintToString(dsMeter.getPowerConsumption()) +"}");
      } else if(_request.getMethod() == "getEnergyMeterValue") {
        return JSONOk("{ " + ToJSONValue("metervalue") + ": " +  uintToString(dsMeter.getEnergyMeterValue()) +"}");
      } else if(_request.getMethod() == "rescan") {
        dsMeter.setIsValid(false);
        return ResultToJSON(true);
      } else {
        throw std::runtime_error("Unhandled function");
      }
    } catch(std::runtime_error&) {
      return ResultToJSON(false, "could not find dsMeter with given dsid");
    }
    return "";
  } // handleRequest


  //=========================================== SetRequestHandler

  std::string SetRequestHandler::handleRequest(const RestfulRequest& _request, Session* _session) {
    if(_request.getMethod() == "fromApartment") {
      return JSONOk("{'self': '.'}");
    } else {
      std::string self = trim(_request.getParameter("self"));
      if(self.empty()) {
        return ResultToJSON(false, "missing parameter 'self'");
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
          return ResultToJSON(false, "missing either zoneID or zoneName");
        }

        std::ostringstream sstream;
        sstream << "{" << ToJSONValue("self") << ":" << ToJSONValue(self + additionalPart) << "}";
        return JSONOk(sstream.str());
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
          return ResultToJSON(false, "missing either groupID or groupName");
        }

        std::ostringstream sstream;
        sstream << "{" << ToJSONValue("self") << ":" << ToJSONValue(self + additionalPart) << "}";
        return JSONOk(sstream.str());
      } else if(_request.getMethod() == "byDSID") {
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        if(!_request.getParameter("dsid").empty()) {
          additionalPart += "dsid(" + _request.getParameter("dsid") + ")";
        } else {
          return ResultToJSON(false, "missing parameter dsid");
        }

        std::ostringstream sstream;
        sstream << "{" << ToJSONValue("self") << ":" << ToJSONValue(self + additionalPart) << "}";
        return JSONOk(sstream.str());
      } else if(_request.getMethod() == "getDevices") {
        SetBuilder builder(getDSS().getApartment());
        Set set = builder.buildSet(self, NULL);
        return JSONOk("{" + ToJSONValue(set, "devices") + "}");
      } else if(_request.getMethod() == "add") {
        std::string other = _request.getParameter("other");
        if(other.empty()) {
          return ResultToJSON(false, "missing parameter other");
        }
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        additionalPart += "add(" + other + ")";

        std::ostringstream sstream;
        sstream << "{" << ToJSONValue("self") << ":" << ToJSONValue(self + additionalPart) << "}";
        return JSONOk(sstream.str());
      } else if(_request.getMethod() == "subtract") {
        std::string other = _request.getParameter("other");
        if(other.empty()) {
          return ResultToJSON(false, "missing parameter other");
        }
        std::string additionalPart;
        if(self != ".") {
          additionalPart = ".";
        }
        additionalPart += "subtract(" + other + ")";

        std::ostringstream sstream;
        sstream << "{" << ToJSONValue("self") << ":" << ToJSONValue(self + additionalPart) << "}";
        return JSONOk(sstream.str());
      } else if(isDeviceInterfaceCall(_request)) {
        SetBuilder builder(getDSS().getApartment());
        Set set = builder.buildSet(self, NULL);
        return handleDeviceInterfaceRequest(_request, &set);
      } else {
        throw std::runtime_error("Unhandled function");
      }

    }
    return "";
  } // handleRequest


  //=========================================== PropertyRequestHandler

  std::string PropertyRequestHandler::handleRequest(const RestfulRequest& _request, Session* _session) {
    std::string propName = _request.getParameter("path");
    if(propName.empty()) {
      return ResultToJSON(false, "Need parameter \'path\' for property operations");
    }
    PropertyNodePtr node = getDSS().getPropertySystem().getProperty(propName);

    if(_request.getMethod() == "getString") {
      if(node == NULL) {
        return ResultToJSON(false, "Could not find node named '" + propName + "'");
      }
      try {
        std::ostringstream sstream;
        sstream << "{ " << ToJSONValue("value") << ": " << ToJSONValue(node->getStringValue()) << "}";
        return JSONOk(sstream.str());
      } catch(PropertyTypeMismatch& ex) {
        return ResultToJSON(false, std::string("Error getting property: '") + ex.what() + "'");
      }
    } else if(_request.getMethod() == "getInteger") {
      if(node == NULL) {
        return ResultToJSON(false, "Could not find node named '" + propName + "'");
      }
      try {
        std::ostringstream sstream;
        sstream << "{ " << ToJSONValue("value") << ": " << ToJSONValue(node->getIntegerValue()) << "}";
        return JSONOk(sstream.str());
      } catch(PropertyTypeMismatch& ex) {
        return ResultToJSON(false, std::string("Error getting property: '") + ex.what() + "'");
      }
    } else if(_request.getMethod() == "getBoolean") {
      if(node == NULL) {
        return ResultToJSON(false, "Could not find node named '" + propName + "'");
      }
      try {
        std::ostringstream sstream;
        sstream << "{ " << ToJSONValue("value") << ": " << ToJSONValue(node->getBoolValue()) << "}";
        return JSONOk(sstream.str());
      } catch(PropertyTypeMismatch& ex) {
        return ResultToJSON(false, std::string("Error getting property: '") + ex.what() + "'");
      }
    } else if(_request.getMethod() == "setString") {
      std::string value = _request.getParameter("value");
      if(node == NULL) {
        node = getDSS().getPropertySystem().createProperty(propName);
      }
      try {
        node->setStringValue(value);
      } catch(PropertyTypeMismatch& ex) {
        return ResultToJSON(false, std::string("Error setting property: '") + ex.what() + "'");
      }
      return JSONOk();
    } else if(_request.getMethod() == "setBoolean") {
      std::string strValue = _request.getParameter("value");
      bool value;
      if(strValue == "true") {
        value = true;
      } else if(strValue == "false") {
        value = false;
      } else {
        return ResultToJSON(false, "Expected 'true' or 'false' for parameter 'value' but got: '" + strValue + "'");
      }
      if(node == NULL) {
        node = getDSS().getPropertySystem().createProperty(propName);
      }
      try {
        node->setBooleanValue(value);
      } catch(PropertyTypeMismatch& ex) {
        return ResultToJSON(false, std::string("Error setting property: '") + ex.what() + "'");
      }
      return JSONOk();
    } else if(_request.getMethod() == "setInteger") {
      std::string strValue = _request.getParameter("value");
      int value;
      try {
        value = strToInt(strValue);
      } catch(...) {
        return ResultToJSON(false, "Could not convert parameter 'value' to std::string. Got: '" + strValue + "'");
      }
      if(node == NULL) {
        node = getDSS().getPropertySystem().createProperty(propName);
      }
      try {
        node->setIntegerValue(value);
      } catch(PropertyTypeMismatch& ex) {
        return ResultToJSON(false, std::string("Error setting property: '") + ex.what() + "'");
      }
      return JSONOk();
    } else if(_request.getMethod() == "getChildren") {
      if(node == NULL) {
        return ResultToJSON(false, "Could not find node named '" + propName + "'");
      }
      std::ostringstream sstream;
      sstream << "[";
      bool first = true;
      for(int iChild = 0; iChild < node->getChildCount(); iChild++) {
        if(!first) {
          sstream << ",";
        }
        first = false;
        PropertyNodePtr cnode = node->getChild(iChild);
        sstream << "{" << ToJSONValue("name") << ":" << ToJSONValue(cnode->getName());
        sstream << "," << ToJSONValue("type") << ":" << ToJSONValue(getValueTypeAsString(cnode->getValueType()));
        sstream << "}";
      }
      sstream << "]";
      return JSONOk(sstream.str());
    } else if(_request.getMethod() == "getType") {
      if(node == NULL) {
        return ResultToJSON(false, "Could not find node named '" + propName + "'");
      }
      std::ostringstream sstream;
      sstream << ":" << ToJSONValue("type") << ":" << ToJSONValue(getValueTypeAsString(node->getValueType())) << "}";
      return JSONOk(sstream.str());
    } else {
      throw std::runtime_error("Unhandled function");
    }
    return "";
  } // handleRequest


  //=========================================== EventRequestHandler

  std::string EventRequestHandler::handleRequest(const RestfulRequest& _request, Session* _session) {
    if(_request.getMethod() == "raise") {
      std::string name = _request.getParameter("name");
      std::string location = _request.getParameter("location");
      std::string context = _request.getParameter("context");
      std::string parameter = _request.getParameter("parameter");

      boost::shared_ptr<Event> evt(new Event(name));
      if(!context.empty()) {
        evt->setContext(context);
      }
      if(!location.empty()) {
        evt->setLocation(location);
      }
      std::vector<std::string> params = dss::splitString(parameter, ';');
      for(std::vector<std::string>::iterator iParam = params.begin(), e = params.end();
          iParam != e; ++iParam)
      {
        std::vector<std::string> nameValue = dss::splitString(*iParam, '=');
        if(nameValue.size() == 2) {
          dss::Logger::getInstance()->log("WebServer::handleEventCall: Got parameter '" + nameValue[0] + "'='" + nameValue[1] + "'");
          evt->setProperty(nameValue[0], nameValue[1]);
        } else {
          dss::Logger::getInstance()->log(std::string("Invalid parameter found WebServer::handleEventCall: ") + *iParam );
        }
      }

      getDSS().getEventQueue().pushEvent(evt);
      return ResultToJSON(true);
    } else {
      throw std::runtime_error("Unhandled function");
    }
  } // handleRequest
  

  //=========================================== SystemRequestHandler

  std::string SystemRequestHandler::handleRequest(const RestfulRequest& _request, Session* _session) {
    if(_request.getMethod() == "version") {
      return ResultToJSON(true, DSS::getInstance()->versionString());
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest


  //=========================================== StructureRequestHandler

  std::string StructureRequestHandler::handleRequest(const RestfulRequest& _request, Session* _session) {
    StructureManipulator manipulator(*getDSS().getDS485Interface().getStructureModifyingBusInterface(), getDSS().getApartment());
    if(_request.getMethod() == "zoneAddDevice") {
      std::string devidStr = _request.getParameter("devid");
      if(!devidStr.empty()) {
        dsid_t devid = dsid::fromString(devidStr);

        Device& dev = DSS::getInstance()->getApartment().getDeviceByDSID(devid);
        if(!dev.isPresent()) {
          return ResultToJSON(false, "cannot add nonexisting device to a zone");
        }

        std::string zoneIDStr = _request.getParameter("zone");
        if(!zoneIDStr.empty()) {
          try {
            int zoneID = strToInt(zoneIDStr);
            DeviceReference devRef(dev, &DSS::getInstance()->getApartment());
            try {
              Zone& zone = getDSS().getApartment().getZone(zoneID);
              manipulator.addDeviceToZone(dev, zone);
            } catch(ItemNotFoundException&) {
              return ResultToJSON(false, "Could not find zone");
            }
          } catch(std::runtime_error& err) {
            return ResultToJSON(false, err.what());
          }
        }
        return ResultToJSON(true);
      } else {
        return ResultToJSON(false, "Need parameter devid");
      }
    } else if(_request.getMethod() == "addZone") {
      int zoneID = -1;

      std::string zoneIDStr = _request.getParameter("zoneID");
      if(!zoneIDStr.empty()) {
        zoneID = strToIntDef(zoneIDStr, -1);
      }
      if(zoneID != -1) {
        getDSS().getApartment().allocateZone(zoneID);
      } else {
        return ResultToJSON(false, "could not find zone");
      }
      return ResultToJSON(true, "");
    } else if(_request.getMethod() == "removeZone") {
      int zoneID = -1;

      std::string zoneIDStr = _request.getParameter("zoneID");
      if(!zoneIDStr.empty()) {
        zoneID = strToIntDef(zoneIDStr, -1);
      }
      if(zoneID != -1) {
        try {
          Zone& zone = getDSS().getApartment().getZone(zoneID);
          if(zone.getFirstZoneOnDSMeter() != -1) {
            return ResultToJSON(false, "Cannot delete a primary zone");
          }
          if(zone.getDevices().length() > 0) {
            return ResultToJSON(false, "Cannot delete a non-empty zone");
          }
          getDSS().getApartment().removeZone(zoneID);
          getDSS().getModelMaintenance().addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
          return JSONOk();
        } catch(ItemNotFoundException&) {
          return ResultToJSON(false, "Could not find zone");
        }
      } else {
        return ResultToJSON(false, "Missing parameter zoneID");
      }
    } else {
      throw std::runtime_error("Unhandled function");
    }
  } // handleRequest


  //=========================================== SimRequestHandler

  std::string SimRequestHandler::handleRequest(const RestfulRequest& _request, Session* _session) {
    if(_request.getMethod() == "switch") {
      if(_request.getMethod() == "switch/pressed") {
        int buttonNr = strToIntDef(_request.getParameter("buttonnr"), -1);
        if(buttonNr == -1) {
          return ResultToJSON(false, "Invalid button number");
        }

        int zoneID = strToIntDef(_request.getParameter("zoneID"), -1);
        if(zoneID == -1) {
          return ResultToJSON(false, "Could not parse zoneID");
        }
        int groupID = strToIntDef(_request.getParameter("groupID"), -1);
        if(groupID == -1) {
          return ResultToJSON(false, "Could not parse groupID");
        }
        try {
          Zone& zone = getDSS().getApartment().getZone(zoneID);
          Group* pGroup = zone.getGroup(groupID);

          if(pGroup == NULL) {
            return ResultToJSON(false, "Could not find group");
          }

          switch(buttonNr) {
          case 1: // upper-left
          case 3: // upper-right
          case 7: // lower-left
          case 9: // lower-right
            break;
          case 2: // up
            pGroup->increaseValue();
            break;
          case 8: // down
            pGroup->decreaseValue();
            break;
          case 4: // left
            pGroup->previousScene();
            break;
          case 6: // right
            pGroup->nextScene();
            break;
          case 5:
            {
              if(groupID == GroupIDGreen) {
                getDSS().getApartment().getGroup(0).callScene(SceneBell);
              } else if(groupID == GroupIDRed){
                getDSS().getApartment().getGroup(0).callScene(SceneAlarm);
              } else {
                const int lastScene = pGroup->getLastCalledScene();
                if(lastScene == SceneOff || lastScene == SceneDeepOff ||
                  lastScene == SceneStandBy || lastScene == ScenePanic)
                {
                  pGroup->callScene(Scene1);
                } else {
                  pGroup->callScene(SceneOff);
                }
              }
            }
            break;
          default:
            return ResultToJSON(false, "Invalid button nr (range is 1..9)");
          }
        } catch(std::runtime_error&) {
          return ResultToJSON(false, "Could not find zone");
        }
      }
    } else if(_request.getMethod() == "addDevice") {
      //std::string type = _parameter["type"];
      //std::string dsidStr = _parameter["dsid"];
      // TODO: not finished yet ;)
      return "";
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest


  //=========================================== DebugRequestHandler

  std::string DebugRequestHandler::handleRequest(const RestfulRequest& _request, Session* _session) {
    if(_request.getMethod() == "sendFrame") {
      int destination = strToIntDef(_request.getParameter("destination"),0) & 0x3F;
      bool broadcast = _request.getParameter("broadcast") == "true";
      int counter = strToIntDef(_request.getParameter("counter"), 0x00) & 0x03;
      int command = strToIntDef(_request.getParameter("command"), 0x09 /* request */) & 0x00FF;
      int length = strToIntDef(_request.getParameter("length"), 0x00) & 0x0F;

      std::cout
          << "sending frame: "
          << "\ndest:    " << destination
          << "\nbcst:    " << broadcast
          << "\ncntr:    " << counter
          << "\ncmd :    " << command
          << "\nlen :    " << length << std::endl;

      DS485CommandFrame* frame = new DS485CommandFrame();
      frame->getHeader().setBroadcast(broadcast);
      frame->getHeader().setDestination(destination);
      frame->getHeader().setCounter(counter);
      frame->setCommand(command);
      for(int iByte = 0; iByte < length; iByte++) {
        uint8_t byte = strToIntDef(_request.getParameter(std::string("payload_") + intToString(iByte+1)), 0xFF);
        std::cout << "b" << std::dec << iByte << ": " << std::hex << (int)byte << "\n";
        frame->getPayload().add<uint8_t>(byte);
      }
      std::cout << std::dec << "done" << std::endl;
      DS485Interface* intf = &DSS::getInstance()->getDS485Interface();
      DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(intf);
      if(proxy != NULL) {
        proxy->sendFrame(*frame);
      } else {
        delete frame;
      }
      return ResultToJSON(true);
    } else if(_request.getMethod() == "dSLinkSend") {
      std::string deviceDSIDString =_request.getParameter("dsid");
      Device* pDevice = NULL;
      if(!deviceDSIDString.empty()) {
        dsid_t deviceDSID = dsid_t::fromString(deviceDSIDString);
        if(!(deviceDSID == NullDSID)) {
          try {
            Device& device = getDSS().getApartment().getDeviceByDSID(deviceDSID);
            pDevice = &device;
          } catch(std::runtime_error& e) {
            return ResultToJSON(false ,"Could not find device with dsid '" + deviceDSIDString + "'");
          }
        } else {
          return ResultToJSON(false, "Could not parse dsid '" + deviceDSIDString + "'");
        }
      } else {
        return ResultToJSON(false, "Missing parameter 'dsid'");
      }

      int iValue = strToIntDef(_request.getParameter("value"), -1);
      if(iValue == -1) {
        return ResultToJSON(false, "Missing parameter 'value'");
      }
      if(iValue < 0 || iValue > 0x00ff) {
        return ResultToJSON(false, "Parameter 'value' is out of range (0-0xff)");
      }
      bool writeOnly = false;
      bool lastValue = false;
      if(_request.getParameter("writeOnly") == "true") {
        writeOnly = true;
      }
      if(_request.getParameter("lastValue") == "true") {
        lastValue = true;
      }
      uint8_t result;
      try {
        result = pDevice->dsLinkSend(iValue, lastValue, writeOnly);
      } catch(std::runtime_error& e) {
        return ResultToJSON(false, std::string("Error: ") + e.what());
      }
      if(writeOnly) {
        return ResultToJSON(true);
      } else {
        std::ostringstream sstream;
        sstream << "{" << ToJSONValue("value") << ":" << ToJSONValue(result) << "}";
        return JSONOk(sstream.str());
      }
    } else if(_request.getMethod() == "pingDevice") {
      std::string deviceDSIDString = _request.getParameter("dsid");
      if(deviceDSIDString.empty()) {
        return ResultToJSON(false, "Missing parameter 'dsid'");
      }
      try {
        dsid_t deviceDSID = dsid_t::fromString(deviceDSIDString);
        Device& device = getDSS().getApartment().getDeviceByDSID(deviceDSID);
        DS485CommandFrame* frame = new DS485CommandFrame();
        frame->getHeader().setBroadcast(true);
        frame->getHeader().setDestination(device.getDSMeterID());
        frame->setCommand(CommandRequest);
        frame->getPayload().add<uint8_t>(FunctionDeviceGetTransmissionQuality);
        frame->getPayload().add<uint16_t>(device.getShortAddress());
        DS485Interface* intf = &DSS::getInstance()->getDS485Interface();
        DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(intf);
        if(proxy != NULL) {
          boost::shared_ptr<FrameBucketCollector> bucket = proxy->sendFrameAndInstallBucket(*frame, FunctionDeviceGetTransmissionQuality);
          bucket->waitForFrame(2000);

          boost::shared_ptr<DS485CommandFrame> recFrame = bucket->popFrame();
          if(recFrame == NULL) {
            return ResultToJSON(false, "No result received");
          }
          PayloadDissector pd(recFrame->getPayload());
          pd.get<uint8_t>();
          int errC = int(pd.get<uint16_t>());
          if(errC < 0) {
            return ResultToJSON(false, "dSM reported error-code: " + intToString(errC));
          }
          pd.get<uint16_t>(); // device address
          int qualityHK = pd.get<uint16_t>();
          int qualityRK = pd.get<uint16_t>();
          std::ostringstream sstream;
          sstream << "{" << ToJSONValue("qualityHK") << ":" << ToJSONValue(qualityHK) << ",";
          sstream << ToJSONValue("qualityRK") << ":" << ToJSONValue(qualityRK) << "}";
          return JSONOk(sstream.str());
        } else {
          delete frame;
          return ResultToJSON(false, "Proxy has a wrong type or is null");
        }
      } catch(ItemNotFoundException&) {
        return ResultToJSON(false ,"Could not find device with dsid '" + deviceDSIDString + "'");
      } catch(std::invalid_argument&) {
        return ResultToJSON(false, "Could not parse dsid '" + deviceDSIDString + "'");
      }
    } else if(_request.getMethod() == "resetZone") {
      std::string zoneIDStr = _request.getParameter("zoneID");
      int zoneID;
      try {
        zoneID = strToInt(zoneIDStr);
      } catch(std::runtime_error&) {
        return ResultToJSON(false, "Could not parse Zone ID");
      }
      DS485CommandFrame* frame = new DS485CommandFrame();
      frame->getHeader().setBroadcast(true);
      frame->getHeader().setDestination(0);
      frame->setCommand(CommandRequest);
      frame->getPayload().add<uint8_t>(FunctionZoneRemoveAllDevicesFromZone);
      frame->getPayload().add<uint8_t>(zoneID);
      DS485Interface* intf = &DSS::getInstance()->getDS485Interface();
      DS485Proxy* proxy = dynamic_cast<DS485Proxy*>(intf);
      if(proxy != NULL) {
        proxy->sendFrame(*frame);
        return ResultToJSON(true, "Please restart your dSMs");
      } else {
        delete frame;
        return ResultToJSON(false, "Proxy has a wrong type or is null");
      }
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest


  //=========================================== MeteringRequestHandler

  std::string MeteringRequestHandler::handleRequest(const RestfulRequest& _request, Session* _session) {
    if(_request.getMethod() == "getResolutions") {
      std::vector<boost::shared_ptr<MeteringConfigChain> > meteringConfig = getDSS().getMetering().getConfig();
      std::ostringstream sstream;
      sstream << "{" << ToJSONValue("resolutions") << ":" << "[";
      for(unsigned int iConfig = 0; iConfig < meteringConfig.size(); iConfig++) {
        boost::shared_ptr<MeteringConfigChain> cConfig = meteringConfig[iConfig];
        for(int jConfig = 0; jConfig < cConfig->size(); jConfig++) {
          sstream << "{" << ToJSONValue("type") << ":" << (cConfig->isEnergy() ? ToJSONValue("energy") : ToJSONValue("consumption") ) << ","
                  << ToJSONValue("unit") << ":" << ToJSONValue(cConfig->getUnit()) << ","
                  << ToJSONValue("resolution") << ":" << ToJSONValue(cConfig->getResolution(jConfig)) << "}";
          if(jConfig < cConfig->size() && iConfig < meteringConfig.size()) {
            sstream << ",";
          }
        }
      }
      sstream << "]"  << "}";
      return JSONOk(sstream.str());
    } else if(_request.getMethod() == "getSeries") {
      std::ostringstream sstream;
      sstream << "{ " << ToJSONValue("series") << ": [";
      bool first = true;
      vector<DSMeter*>& dsMeters = getDSS().getApartment().getDSMeters();
      foreach(DSMeter* dsMeter, dsMeters) {
        if(!first) {
          sstream << ",";
        }
        first = false;
        sstream << "{ " << ToJSONValue("dsid") << ": " << ToJSONValue(dsMeter->getDSID().toString());
        sstream << ", " << ToJSONValue("type") << ": " << ToJSONValue("energy");
        sstream << "},";
        sstream << "{ " << ToJSONValue("dsid") << ": " << ToJSONValue(dsMeter->getDSID().toString());
        sstream << ", " << ToJSONValue("type") << ": " << ToJSONValue("consumption");
        sstream << "}";
      }
      sstream << "]}";
      return JSONOk(sstream.str());
    } else if(_request.getMethod() == "getValues") { //?dsid=;n=,resolution=,type=
      std::string errorMessage;
      std::string deviceDSIDString = _request.getParameter("dsid");
      std::string resolutionString = _request.getParameter("resolution");
      std::string typeString = _request.getParameter("type");
      std::string fileSuffix;
      std::string storageLocation;
      std::string seriesPath;
      int resolution;
      bool energy;
      if(!deviceDSIDString.empty()) {
        dsid_t deviceDSID = dsid_t::fromString(deviceDSIDString);
        if(!(deviceDSID == NullDSID)) {
          try {
            getDSS().getApartment().getDSMeterByDSID(deviceDSID);
          } catch(std::runtime_error& e) {
            return ResultToJSON(false, "Could not find device with dsid '" + deviceDSIDString + "'");
          }
        } else {
        return ResultToJSON(false, "Could not parse dsid '" + deviceDSIDString + "'");
        }
      } else {
        return ResultToJSON(false, "Could not parse dsid '" + deviceDSIDString + "'");
      }
      resolution = strToIntDef(resolutionString, -1);
      if(resolution == -1) {
        return ResultToJSON(false, "Need could not parse resolution '" + resolutionString + "'");
      }
      if(typeString.empty()) {
        return ResultToJSON(false, "Need a type, 'energy' or 'consumption'");
      } else {
        if(typeString == "consumption") {
          energy = false;
        } else if(typeString == "energy") {
          energy = true;
        } else {
          return ResultToJSON(false, "Invalide type '" + typeString + "'");
        }
      }
      if(!resolutionString.empty()) {
        std::vector<boost::shared_ptr<MeteringConfigChain> > meteringConfig = getDSS().getMetering().getConfig();
        storageLocation = getDSS().getMetering().getStorageLocation();
        for(unsigned int iConfig = 0; iConfig < meteringConfig.size(); iConfig++) {
          boost::shared_ptr<MeteringConfigChain> cConfig = meteringConfig[iConfig];
          for(int jConfig = 0; jConfig < cConfig->size(); jConfig++) {
            if(cConfig->isEnergy() == energy && cConfig->getResolution(jConfig) == resolution) {
              fileSuffix = cConfig->getFilenameSuffix(jConfig);
            }
          }
        }
        if(fileSuffix.empty()) {
          return ResultToJSON(false, "No data for '" + typeString + "' and resolution '" + resolutionString + "'");
        } else {
          seriesPath = storageLocation + deviceDSIDString + "_" + fileSuffix + ".xml";
          log("_Trying to load series from " + seriesPath);
          if(boost::filesystem::exists(seriesPath)) {
            SeriesReader<CurrentValue> reader;
            boost::shared_ptr<Series<CurrentValue> > s = boost::shared_ptr<Series<CurrentValue> >(reader.readFromXML(seriesPath));
            boost::shared_ptr<std::deque<CurrentValue> > values = s->getExpandedValues();
            bool first = true;
            std::ostringstream sstream;
            sstream << "{ " ;
            sstream << ToJSONValue("dsmid") << ":" << ToJSONValue(deviceDSIDString) << ",";
            sstream << ToJSONValue("type") << ":" << ToJSONValue(typeString) << ",";
            sstream << ToJSONValue("resolution") << ":" << ToJSONValue(resolutionString) << ",";
            sstream << ToJSONValue("values") << ": [";
            for(std::deque<CurrentValue>::iterator iValue = values->begin(), e = values->end(); iValue != e; iValue++)
            {
              if(!first) {
                sstream << ",";
              }
              first = false;
              sstream << "[" << iValue->getTimeStamp().secondsSinceEpoch()  << "," << iValue->getValue() << "]";
            }
            sstream << "]}";
            return JSONOk(sstream.str());
          } else {
            return ResultToJSON(false, "No data-file for '" + typeString + "' and resolution '" + resolutionString + "'");
          }
        }
      } else {
        return ResultToJSON(false, "Could not parse resolution '" + resolutionString + "'");
      }
    } else if(_request.getMethod() == "getAggregatedValues") { //?set=;n=,resolution=;type=
      // TODO: implement
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest

} // namespace dss
