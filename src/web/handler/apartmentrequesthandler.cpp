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

#include <digitalSTROM/dsuid/dsuid.h>
#include "apartmentrequesthandler.h"

#include "src/foreach.h"
#include "src/model/modelconst.h"

#include "src/web/json.h"

#include "src/model/apartment.h"
#include "src/model/zone.h"
#include "src/model/modulator.h"
#include "src/model/device.h"
#include "src/model/deviceinterface.h"
#include "src/model/group.h"
#include "src/model/set.h"
#include "src/model/modelmaintenance.h"
#include "src/stringconverter.h"
#include "util.h"
#include "jsonhelper.h"

namespace dss {

  //=========================================== ApartmentRequestHandler

  ApartmentRequestHandler::ApartmentRequestHandler(Apartment& _apartment,
          ModelMaintenance& _modelMaintenance)
  : m_Apartment(_apartment), m_ModelMaintenance(_modelMaintenance)
  { }

  boost::shared_ptr<JSONObject> ApartmentRequestHandler::getReachableGroups(const RestfulRequest& _request) {
    boost::shared_ptr<JSONObject> resultObj(new JSONObject());
    boost::shared_ptr<JSONArrayBase> resultZones(new JSONArrayBase());
    resultObj->addElement("zones", resultZones);

    std::vector<boost::shared_ptr<Zone> > zones =
                                DSS::getInstance()->getApartment().getZones();
    for (size_t i = 0; i < zones.size(); i++) {
      boost::shared_ptr<Zone> zone = zones.at(i);
      if ((zone == NULL) || (zone->getID() == 0)) {
        continue;
      }

      boost::shared_ptr<JSONObject> resultZone(new JSONObject());
      resultZones->addElement("", resultZone);
      resultZone->addProperty("zoneID", zone->getID());
      resultZone->addProperty("name", zone->getName());
      boost::shared_ptr<JSONArray<int> > resultGroups(new JSONArray<int>());
      resultZone->addElement("groups", resultGroups);

      std::vector<boost::shared_ptr<Group> > groups = zone->getGroups();

      for (size_t g = 0; g < groups.size(); g++) {
        boost::shared_ptr<Group> group = groups.at(g);
        if ((group->getID() == 0) || (group->getID() == 8) ||
            (group->getID() == 6) || (group->getID() == 7)) {
          continue;
        }

        Set devices = group->getDevices();
        for (int d = 0; d < devices.length(); d++) {
          boost::shared_ptr<Device> device = devices.get(d).getDevice();
          if ((device->isPresent() == true) && (device->getOutputMode() != 0)) {
            resultGroups->add(group->getID());
            break;
          }
        } // devices loop
      } // groups loop
    } // zones loop

    return success(resultObj);
  }

  boost::shared_ptr<JSONObject> ApartmentRequestHandler::removeMeter(const RestfulRequest& _request) {
    std::string dsidStr = _request.getParameter("dsid");
    std::string dsuidStr = _request.getParameter("dsuid");
    if(dsidStr.empty() && dsuidStr.empty()) {
      return failure("Missing parameter 'dsuid'");
    }

    dsuid_t meterID;
    
    if (dsuidStr.empty()) {
      dsid_t dsid = str2dsid(dsidStr);
      meterID = dsuid_from_dsid(&dsid);
    } else {
      meterID = str2dsuid(dsuidStr);
    }

    boost::shared_ptr<DSMeter> meter = DSS::getInstance()->getApartment().getDSMeterByDSID(meterID);

    if(meter->isPresent()) {
      return failure("Cannot remove present meter");
    }

    m_Apartment.removeDSMeter(meterID);
    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return success();
  }

  WebServerResponse ApartmentRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    std::string errorMessage;
    if(_request.getMethod() == "getConsumption") {
      int accumulatedConsumption = 0;
      foreach(boost::shared_ptr<DSMeter> pDSMeter, m_Apartment.getDSMeters()) {
        accumulatedConsumption += pDSMeter->getPowerConsumption();
      }
      boost::shared_ptr<JSONObject> resultObj(new JSONObject());
      resultObj->addProperty("consumption", accumulatedConsumption);
      return success(resultObj);
    } else if(isDeviceInterfaceCall(_request)) {
      boost::shared_ptr<IDeviceInterface> interface;
      std::string groupName = _request.getParameter("groupName");
      std::string groupIDString = _request.getParameter("groupID");
      if(!groupName.empty()) {
        try {
          interface = m_Apartment.getGroup(groupName);
        } catch(std::runtime_error& e) {
          return failure("Could not find group with name '" + groupName + "'");
        }
      } else if(!groupIDString.empty()) {
        try {
          int groupID = strToIntDef(groupIDString, -1);
          if(groupID != -1) {
            interface = m_Apartment.getGroup(groupID);
          } else {
            return failure("Could not parse group ID '" + groupIDString + "'");
          }
        } catch(std::runtime_error& e) {
          return failure("Could not find group with ID '" + groupIDString + "'");
        }
      }
      if(interface == NULL) {
        interface = m_Apartment.getGroup(GroupIDBroadcast);
      }

      return handleDeviceInterfaceRequest(_request, interface, _session);

    } else {
      if(_request.getMethod() == "getStructure") {
        return success(toJSON(m_Apartment));
      } else if(_request.getMethod() == "getDevices") {
        Set devices;
        if(_request.getParameter("unassigned").empty()) {
          devices = m_Apartment.getDevices();
        } else {
          devices = getUnassignedDevices();
        }

        return success(toJSON(devices));
      } else if(_request.getMethod() == "getCircuits") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        boost::shared_ptr<JSONArrayBase> circuits(new JSONArrayBase());

        resultObj->addElement("circuits", circuits);
        std::vector<boost::shared_ptr<DSMeter> > dsMeters = m_Apartment.getDSMeters();
        foreach(boost::shared_ptr<DSMeter> dsMeter, dsMeters) {
          boost::shared_ptr<JSONObject> circuit(new JSONObject());
          circuits->addElement("", circuit);
          circuit->addProperty("name", dsMeter->getName());
          try {
            circuit->addProperty("dsid", dsid2str(dsuid_to_dsid(dsMeter->getDSID())));
          } catch (std::runtime_error &err) {
            Logger::getInstance()->log(err.what());
          }
          circuit->addProperty("dSUID", dsuid2str(dsMeter->getDSID()));
          circuit->addProperty("hwVersion", dsMeter->getHardwareVersion());
          circuit->addProperty("armSwVersion", dsMeter->getArmSoftwareVersion());
          circuit->addProperty("dspSwVersion", dsMeter->getDspSoftwareVersion());
          circuit->addProperty("apiVersion", dsMeter->getApiVersion());
          circuit->addProperty("hwName", dsMeter->getHardwareName());
          circuit->addProperty("isPresent", dsMeter->isPresent());
          circuit->addProperty("isValid", dsMeter->isValid());
          circuit->addProperty("busMemberType", dsMeter->getBusMemberType());
          circuit->addProperty("hasDevices", dsMeter->getCapability_HasDevices());
          circuit->addProperty("hasMetering", dsMeter->getCapability_HasMetering());
          std::bitset<8> flags = dsMeter->getPropertyFlags();
          circuit->addProperty("ignoreActionsFromNewDevices", flags.test(4));
        }
        return success(resultObj);
      } else if(_request.getMethod() == "getName") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        resultObj->addProperty("name", m_Apartment.getName());
        return success(resultObj);
      } else if(_request.getMethod() == "setName") {
        if (_request.hasParameter("newName")) {
          StringConverter st("UTF-8", "UTF-8");
          std::string newName = st.convert(_request.getParameter("newName"));
          newName = escapeHTML(newName);

          m_Apartment.setName(newName);
          DSS::getInstance()->getBonjourHandler().restart();
        } else {
          return failure("missing parameter 'newName'");
        }
        return success();
      } else if(_request.getMethod() == "rescan") {
        std::vector<boost::shared_ptr<DSMeter> > mods = m_Apartment.getDSMeters();
        foreach(boost::shared_ptr<DSMeter> pDSMeter, mods) {
          pDSMeter->setIsValid(false);
        }
        return success();
      } else if(_request.getMethod() == "removeMeter") {
        return removeMeter(_request);
      } else if(_request.getMethod() == "removeInactiveMeters") {
        m_Apartment.removeInactiveMeters();
        return success();
      } else if(_request.getMethod() == "getReachableGroups") {
        return getReachableGroups(_request);
      } else if(_request.getMethod() == "getLockedScenes") {
        std::list<uint32_t> scenes = CommChannel::getInstance()->getLockedScenes();
        boost::shared_ptr<JSONObject> result(new JSONObject());
        boost::shared_ptr<JSONArray<int> > lockedScenes(new JSONArray<int>());
        result->addElement("lockedScenes", lockedScenes);
        while (!scenes.empty()) {
          lockedScenes->add((int)scenes.front());
          scenes.pop_front();
        }
        return success(result);

      } else if(_request.getMethod() == "getTemperatureControlStatus") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        boost::shared_ptr<JSONArrayBase> zones(new JSONArrayBase());

        resultObj->addElement("zones", zones);
        std::vector<boost::shared_ptr<Zone> > zoneList = m_Apartment.getZones();
        foreach(boost::shared_ptr<Zone> pZone, zoneList) {
          if (pZone->getID() == 0) {
            continue;
          }
          boost::shared_ptr<JSONObject> zone(new JSONObject());
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingStatus_t hStatus = pZone->getHeatingStatus();
          zones->addElement("", zone);
          zone->addProperty("id", pZone->getID());
          zone->addProperty("name", pZone->getName());
          zone->addProperty("ControlMode", hProp.m_HeatingControlMode);
          if (IsNullDsuid(hProp.m_HeatingControlDSUID)) {
            zone->addProperty("IsConfigured", false);
          } else {
            zone->addProperty("IsConfigured", true);
          }

          switch (hProp.m_HeatingControlMode) {
            case HeatingControlModeIDOff:
              break;
            case HeatingControlModeIDPID:
              zone->addProperty("OperationMode", hStatus.m_OperationMode);
              zone->addProperty("TemperatureValue", hStatus.m_TemperatureValue);
              zone->addProperty("TemperatureValueTime", hStatus.m_TemperatureValueTS.toISO8601());
              zone->addProperty("NominalValue", hStatus.m_NominalValue);
              zone->addProperty("NominalValueTime", hStatus.m_NominalValueTS.toISO8601());
              zone->addProperty("ControlValue", hStatus.m_ControlValue);
              zone->addProperty("ControlValueTime", hStatus.m_ControlValueTS.toISO8601());
              break;
            case HeatingControlModeIDZoneFollower:
              zone->addProperty("ControlValue", hStatus.m_ControlValue);
              zone->addProperty("ControlValueTime", hStatus.m_ControlValueTS.toISO8601());
              break;
            case HeatingControlModeIDFixed:
              zone->addProperty("OperationMode", hStatus.m_OperationMode);
              zone->addProperty("ControlValue", hStatus.m_ControlValue);
              break;
          }
        }
        return success(resultObj);

      } else if(_request.getMethod() == "getTemperatureControlConfig") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        boost::shared_ptr<JSONArrayBase> zones(new JSONArrayBase());

        resultObj->addElement("zones", zones);
        std::vector<boost::shared_ptr<Zone> > zoneList = m_Apartment.getZones();
        foreach(boost::shared_ptr<Zone> pZone, zoneList) {
          if (pZone->getID() == 0) {
            continue;
          }
          boost::shared_ptr<JSONObject> zone(new JSONObject());
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          zones->addElement("", zone);
          zone->addProperty("id", pZone->getID());
          zone->addProperty("name", pZone->getName());
          zone->addProperty("ControlDSUID", dsuid2str(hProp.m_HeatingControlDSUID));

          ZoneHeatingConfigSpec_t hConfig;
          memset(&hConfig, 0, sizeof(hConfig));
          if (IsNullDsuid(hProp.m_HeatingControlDSUID)) {
            zone->addProperty("IsConfigured", false);
            continue;
          } else {
            zone->addProperty("IsConfigured", true);
            hConfig = m_Apartment.getBusInterface()->getStructureQueryBusInterface()->getZoneHeatingConfig(
                hProp.m_HeatingControlDSUID, pZone->getID());
          }

          zone->addProperty("ControlMode", hConfig.ControllerMode);
          switch (hProp.m_HeatingControlMode) {
            case HeatingControlModeIDOff:
              break;
            case HeatingControlModeIDPID:
              zone->addProperty("CtrlKp", hConfig.Kp);
              zone->addProperty("CtrlTs", hConfig.Ts);
              zone->addProperty("CtrlTi", hConfig.Ti);
              zone->addProperty("CtrlKd", hConfig.Kd);
              zone->addProperty("CtrlImin", hConfig.Imin);
              zone->addProperty("CtrlImax", hConfig.Imax);
              zone->addProperty("CtrlYmin", hConfig.Ymin);
              zone->addProperty("CtrlYmax", hConfig.Ymax);
              zone->addProperty("CtrlAntiWindUp", hConfig.AntiWindUp);
              zone->addProperty("CtrlKeepFloorWarm", hConfig.KeepFloorWarm);
              break;
            case HeatingControlModeIDZoneFollower:
              zone->addProperty("ReferenceZone", hConfig.SourceZoneId);
              zone->addProperty("CtrlOffset", hConfig.Offset);
              break;
            case HeatingControlModeIDFixed:
              break;
          }
        }
        return success(resultObj);

      } else if(_request.getMethod() == "getAssignedSensors") {
        boost::shared_ptr<JSONObject> resultObj(new JSONObject());
        boost::shared_ptr<JSONArrayBase> zones(new JSONArrayBase());

        resultObj->addElement("zones", zones);
        std::vector<boost::shared_ptr<Zone> > zoneList = m_Apartment.getZones();
        foreach(boost::shared_ptr<Zone> pZone, zoneList) {
          if (pZone->getID() == 0) {
            continue;
          }
          boost::shared_ptr<JSONObject> zone(new JSONObject());
          zones->addElement("zones", zone);
          zone->addProperty("id", pZone->getID());
          zone->addProperty("name", pZone->getName());

          boost::shared_ptr<JSONArrayBase> sensors(new JSONArrayBase());
          zone->addElement("sensors", sensors);

          std::vector<boost::shared_ptr<MainZoneSensor_t> > slist = pZone->getAssignedSensors();
          for (std::vector<boost::shared_ptr<MainZoneSensor_t> >::iterator it = slist.begin();
              it != slist.end();
              it ++) {
            boost::shared_ptr<JSONObject> sensor(new JSONObject());
            boost::shared_ptr<MainZoneSensor_t> devSensor = *it;
            sensors->addElement("", sensor);
            sensor->addProperty("type", devSensor->m_sensorType);
            sensor->addProperty("dsuid", dsuid2str(devSensor->m_DSUID));
          }
        }
        return success(resultObj);

      } else {
        throw std::runtime_error("Unhandled function");
      }
    }
  } // handleApartmentCall

  Set ApartmentRequestHandler::getUnassignedDevices() {
    Apartment& apt = DSS::getInstance()->getApartment();
    Set devices = apt.getZone(0)->getDevices();

    std::vector<boost::shared_ptr<Zone> > zones = apt.getZones();
    for(std::vector<boost::shared_ptr<Zone> >::iterator ipZone = zones.begin(), e = zones.end();
        ipZone != e; ++ipZone)
    {
      boost::shared_ptr<Zone> pZone = *ipZone;
      if(pZone->getID() == 0) {
        // zone 0 holds all devices, so we're going to skip it
        continue;
      }

      devices = devices.remove(pZone->getDevices());
    }

    return devices;
  } // getUnassignedDevices



} // namespace dss
