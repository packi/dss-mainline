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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <algorithm>

#include "apartmentrequesthandler.h"

#include "foreach.h"

#include <digitalSTROM/dsuid.h>
#include <digitalSTROM/dsm-api-v2/dsm-api-const.h>
#include <boost/make_shared.hpp>

#include "src/model/apartment.h"
#include "src/model/zone.h"
#include "src/model/modelconst.h"
#include "src/model/modulator.h"
#include "src/model/device.h"
#include "src/model/deviceinterface.h"
#include "src/model/group.h"
#include "src/model/cluster.h"
#include "src/model/set.h"
#include "src/model/modelmaintenance.h"
#include "src/model/scenehelper.h"
#include "src/stringconverter.h"
#include "src/model-features.h"
#include "util.h"
#include "jsonhelper.h"

namespace dss {

  //=========================================== ApartmentRequestHandler

  ApartmentRequestHandler::ApartmentRequestHandler(Apartment& _apartment,
          ModelMaintenance& _modelMaintenance)
  : m_Apartment(_apartment), m_ModelMaintenance(_modelMaintenance)
  { }

  std::string ApartmentRequestHandler::getReachableGroups(const RestfulRequest& _request) {
    JSONWriter json;
    json.startArray("zones");

    std::vector<boost::shared_ptr<Zone> > zones =
                                DSS::getInstance()->getApartment().getZones();
    for (size_t i = 0; i < zones.size(); i++) {
      boost::shared_ptr<Zone> zone = zones.at(i);
      if ((zone == NULL) || (zone->getID() == 0)) {
        continue;
      }

      json.startObject();
      json.add("zoneID", zone->getID());
      json.add("name", zone->getName());
      json.startArray("groups");

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
            json.add(group->getID());
            break;
          }
        } // devices loop
      } // groups loop
      json.endArray();
      json.endObject();
    } // zones loop
    json.endArray();

    return json.successJSON();
  }

  std::string ApartmentRequestHandler::removeMeter(const RestfulRequest& _request) {
    std::string dsidStr = _request.getParameter("dsid");
    std::string dsuidStr = _request.getParameter("dsuid");
    if(dsidStr.empty() && dsuidStr.empty()) {
      return JSONWriter::failure("Missing parameter 'dsuid'");
    }

    dsuid_t meterID = dsidOrDsuid2dsuid(dsidStr, dsuidStr);

    boost::shared_ptr<DSMeter> meter = DSS::getInstance()->getApartment().getDSMeterByDSID(meterID);

    if(meter->isPresent()) {
      // ATTENTION: this is string is translated by the web UI
      return JSONWriter::failure("Cannot remove present meter");
    }

    m_Apartment.removeDSMeter(meterID);
    m_ModelMaintenance.addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
    return JSONWriter::success();
  }

  WebServerResponse ApartmentRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session, const struct mg_connection* _connection) {
    std::string errorMessage;
    if(_request.getMethod() == "getConsumption") {
      int accumulatedConsumption = 0;
      foreach(boost::shared_ptr<DSMeter> pDSMeter, m_Apartment.getDSMeters()) {
        accumulatedConsumption += pDSMeter->getPowerConsumption();
      }
      JSONWriter json;
      json.add("consumption", accumulatedConsumption);
      return json.successJSON();
    } else if(isDeviceInterfaceCall(_request)) {
      boost::shared_ptr<IDeviceInterface> interface;
      std::string groupName = _request.getParameter("groupName");
      std::string groupIDString = _request.getParameter("groupID");
      if(!groupName.empty()) {
        try {
          interface = m_Apartment.getGroup(groupName);
        } catch(std::runtime_error& e) {
          return JSONWriter::failure("Could not find group with name '" + groupName + "'");
        }
      } else if(!groupIDString.empty()) {
        try {
          int groupID = strToIntDef(groupIDString, -1);
          if(groupID != -1) {
            interface = m_Apartment.getGroup(groupID);
          } else {
            return JSONWriter::failure("Could not parse group ID '" + groupIDString + "'");
          }
        } catch(std::runtime_error& e) {
          return JSONWriter::failure("Could not find group with ID '" + groupIDString + "'");
        }
      }
      if(interface == NULL) {
        interface = m_Apartment.getGroup(GroupIDBroadcast);
      }

      return handleDeviceInterfaceRequest(_request, interface, _session);

    } else {
      if(_request.getMethod() == "getStructure") {
        JSONWriter json;
        toJSON(m_Apartment, json);
        return json.successJSON();
      } else if(_request.getMethod() == "getDevices") {
        Set devices;
        if(_request.getParameter("unassigned").empty()) {
          devices = m_Apartment.getDevices();
        } else {
          devices = getUnassignedDevices();
        }

        JSONWriter json(JSONWriter::jsonArrayResult);
        toJSON(devices, json);
        return json.successJSON();
      } else if(_request.getMethod() == "getCircuits") {
        JSONWriter json;

        json.startArray("circuits");
        std::vector<boost::shared_ptr<DSMeter> > dsMeters = m_Apartment.getDSMeters();
        foreach(boost::shared_ptr<DSMeter> dsMeter, dsMeters) {
          json.startObject();
          json.add("name", dsMeter->getName());
          dsid_t dsid;
          if (dsuid_to_dsid(dsMeter->getDSID(), &dsid)) {
            json.add("dsid", dsid2str(dsid));
          } else {
            json.add("dsid", "");
          }
          json.add("dSUID", dsuid2str(dsMeter->getDSID()));
          json.add("DisplayID", dsMeter->getDisplayID());
          json.add("hwVersion", 0);
          json.add("hwVersionString", dsMeter->getHardwareVersion());
          json.add("swVersion", dsMeter->getSoftwareVersion());
          json.add("armSwVersion", dsMeter->getArmSoftwareVersion());
          json.add("dspSwVersion", dsMeter->getDspSoftwareVersion());
          json.add("apiVersion", dsMeter->getApiVersion());
          json.add("hwName", dsMeter->getHardwareName());
          json.add("isPresent", dsMeter->isPresent());
          json.add("isValid", dsMeter->isValid());
          json.add("busMemberType", dsMeter->getBusMemberType());
          json.add("hasDevices", dsMeter->getCapability_HasDevices());
          json.add("hasMetering", dsMeter->getCapability_HasMetering());
          json.add("VdcConfigURL", dsMeter->getVdcConfigURL());
          std::bitset<8> flags = dsMeter->getPropertyFlags();
          json.add("ignoreActionsFromNewDevices", flags.test(4));
          json.endObject();
        }
        json.endArray();
        return json.successJSON();
      } else if(_request.getMethod() == "getName") {
        JSONWriter json;
        json.add("name", m_Apartment.getName());
        return json.successJSON();
      } else if(_request.getMethod() == "setName") {
        if (_request.hasParameter("newName")) {
          std::string newName = escapeHTML(_request.getParameter("newName"));
          m_Apartment.setName(newName);
          DSS::getInstance()->getBonjourHandler().restart();
        } else {
          return JSONWriter::failure("missing parameter 'newName'");
        }
        return JSONWriter::success();
      } else if(_request.getMethod() == "rescan") {
        std::vector<boost::shared_ptr<DSMeter> > mods = m_Apartment.getDSMeters();
        foreach(boost::shared_ptr<DSMeter> pDSMeter, mods) {
          pDSMeter->setIsValid(false);
        }
        return JSONWriter::success();
      } else if(_request.getMethod() == "removeMeter") {
        return removeMeter(_request);
      } else if(_request.getMethod() == "removeInactiveMeters") {
        m_Apartment.removeInactiveMeters();
        return JSONWriter::success();
      } else if(_request.getMethod() == "getReachableGroups") {
        return getReachableGroups(_request);
      } else if(_request.getMethod() == "getLockedScenes") {
        std::list<uint32_t> scenes = CommChannel::getInstance()->getLockedScenes();
        JSONWriter json;
        json.startArray("lockedScenes");
        while (!scenes.empty()) {
          json.add((int)scenes.front());
          scenes.pop_front();
        }
        json.endArray();
        return json.successJSON();

      } else if(_request.getMethod() == "getTemperatureControlStatus") {
        JSONWriter json;

        json.startArray("zones");
        std::vector<boost::shared_ptr<Zone> > zoneList = m_Apartment.getZones();
        foreach(boost::shared_ptr<Zone> pZone, zoneList) {
          if (pZone->getID() == 0) {
            continue;
          }
          json.startObject();
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingStatus_t hStatus = pZone->getHeatingStatus();
          ZoneSensorStatus_t hSensors = pZone->getSensorStatus();
          json.add("id", pZone->getID());
          json.add("name", pZone->getName());
          json.add("ControlMode", hProp.m_HeatingControlMode);
          json.add("ControlState", hProp.m_HeatingControlState);
          json.add("ControlDSUID", dsuid2str(hProp.m_HeatingControlDSUID));
          if (hProp.m_HeatingControlDSUID == DSUID_NULL) {
            json.add("IsConfigured", false);
          } else {
            json.add("IsConfigured", true);
          }

          switch (hProp.m_HeatingControlMode) {
            case HeatingControlModeIDOff:
              break;
            case HeatingControlModeIDPID:
              json.add("OperationMode", pZone->getHeatingOperationMode());
              json.add("TemperatureValue", hSensors.m_TemperatureValue);
              json.add("TemperatureValueTime", hSensors.m_TemperatureValueTS.toISO8601());
              json.add("NominalValue", hStatus.m_NominalValue);
              json.add("NominalValueTime", hStatus.m_NominalValueTS.toISO8601());
              json.add("ControlValue", hStatus.m_ControlValue);
              json.add("ControlValueTime", hStatus.m_ControlValueTS.toISO8601());
              break;
            case HeatingControlModeIDZoneFollower:
              json.add("ControlValue", hStatus.m_ControlValue);
              json.add("ControlValueTime", hStatus.m_ControlValueTS.toISO8601());
              break;
            case HeatingControlModeIDFixed:
              json.add("OperationMode", pZone->getHeatingOperationMode());
              json.add("ControlValue", hStatus.m_ControlValue);
              break;
          }
          json.endObject();
        }
        json.endArray();
        return json.successJSON();

      } else if(_request.getMethod() == "getTemperatureControlConfig") {
        JSONWriter json;

        json.startArray("zones");
        std::vector<boost::shared_ptr<Zone> > zoneList = m_Apartment.getZones();
        foreach(boost::shared_ptr<Zone> pZone, zoneList) {
          if (pZone->getID() == 0) {
            continue;
          }
          json.startObject();
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          json.add("id", pZone->getID());
          json.add("name", pZone->getName());
          json.add("ControlDSUID", dsuid2str(hProp.m_HeatingControlDSUID));

          ZoneHeatingConfigSpec_t hConfig;
          memset(&hConfig, 0, sizeof(hConfig));
          if (hProp.m_HeatingControlDSUID == DSUID_NULL) {
            json.add("IsConfigured", false);
            json.endObject();
            continue;
          }

          try {
            hConfig = m_Apartment.getBusInterface()->getStructureQueryBusInterface()->getZoneHeatingConfig(
                hProp.m_HeatingControlDSUID, pZone->getID());
          } catch (BusApiError& e) {
            if (e.error == ERROR_ZONE_NOT_FOUND) {
              json.add("IsConfigured", false);
              json.endObject();
              continue;
            }
            throw e;
          }

          json.add("IsConfigured", true);
          json.add("ControlMode", hConfig.ControllerMode);
          json.add("EmergencyValue", hConfig.EmergencyValue - 100);
          switch (hProp.m_HeatingControlMode) {
            case HeatingControlModeIDOff:
              break;
            case HeatingControlModeIDPID:
              json.add("CtrlKp", (double)hConfig.Kp * 0.025);
              json.add("CtrlTs", hConfig.Ts);
              json.add("CtrlTi", hConfig.Ti);
              json.add("CtrlKd", hConfig.Kd);
              json.add("CtrlImin", (double)hConfig.Imin * 0.025);
              json.add("CtrlImax", (double)hConfig.Imax * 0.025);
              json.add("CtrlYmin", hConfig.Ymin - 100);
              json.add("CtrlYmax", hConfig.Ymax - 100);
              json.add("CtrlAntiWindUp", (hConfig.AntiWindUp > 0));
              json.add("CtrlKeepFloorWarm", (hConfig.KeepFloorWarm > 0));
              break;
            case HeatingControlModeIDZoneFollower:
              json.add("ReferenceZone", hConfig.SourceZoneId);
              json.add("CtrlOffset", hConfig.Offset);
              break;
            case HeatingControlModeIDFixed:
              break;
          }
          json.endObject();
        }
        json.endArray();
        return json.successJSON();

      } else if(_request.getMethod() == "getTemperatureControlValues") {
        JSONWriter json;

        json.startArray("zones");
        std::vector<boost::shared_ptr<Zone> > zoneList = m_Apartment.getZones();
        foreach(boost::shared_ptr<Zone> pZone, zoneList) {
          if (pZone->getID() == 0) {
            continue;
          }
          json.startObject();
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          json.add("id", pZone->getID());
          json.add("name", pZone->getName());
          json.add("ControlDSUID", dsuid2str(hProp.m_HeatingControlDSUID));

          ZoneHeatingOperationModeSpec_t hOpValues;
          memset(&hOpValues, 0, sizeof(hOpValues));

          if (hProp.m_HeatingControlDSUID == DSUID_NULL) {
            json.add("IsConfigured", false);
            json.endObject();
            continue;
          }

          try {
            hOpValues = m_Apartment.getBusInterface()->getStructureQueryBusInterface()->getZoneHeatingOperationModes(
                hProp.m_HeatingControlDSUID, pZone->getID());
          } catch (BusApiError& e) {
            if (e.error == ERROR_ZONE_NOT_FOUND) {
              json.add("IsConfigured", false);
              json.endObject();
              continue;
            }
            throw e;
          }

          json.add("IsConfigured", true);
          switch (hProp.m_HeatingControlMode) {
          case HeatingControlModeIDOff:
            break;
          case HeatingControlModeIDPID:
            json.add("Off", SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode0));
            json.add("Comfort", SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode1));
            json.add("Economy", SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode2));
            json.add("NotUsed", SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode3));
            json.add("Night", SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode4));
            json.add("Holiday", SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode5));
            break;
          case HeatingControlModeIDZoneFollower:
            break;
          case HeatingControlModeIDFixed:
            json.add("Off", SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode0));
            json.add("Comfort", SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode1));
            json.add("Economy", SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode2));
            json.add("NotUsed", SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode3));
            json.add("Night", SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode4));
            json.add("Holiday", SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode5));
            break;

          }
          json.endObject();
        }
        json.endArray();
        return json.successJSON();

      } else if(_request.getMethod() == "getAssignedSensors") {
        JSONWriter json;

        json.startArray("zones");
        std::vector<boost::shared_ptr<Zone> > zoneList = m_Apartment.getZones();
        foreach(boost::shared_ptr<Zone> pZone, zoneList) {
          json.startObject();
          json.add("id", pZone->getID());
          json.add("name", pZone->getName());

          json.startArray("sensors");

          std::vector<boost::shared_ptr<MainZoneSensor_t> > slist = pZone->getAssignedSensors();
          for (std::vector<boost::shared_ptr<MainZoneSensor_t> >::iterator it = slist.begin();
              it != slist.end();
              it ++) {
            json.startObject();
            boost::shared_ptr<MainZoneSensor_t> devSensor = *it;
            json.add("sensorType", devSensor->m_sensorType);
            json.add("dsuid", dsuid2str(devSensor->m_DSUID));
            json.endObject();
          }
          json.endArray();
          json.endObject();
        }
        json.endArray();
        return json.successJSON();

      } else if(_request.getMethod() == "getSensorValues") {
        ApartmentSensorStatus_t aSensorStatus = m_Apartment.getSensorStatus();
        JSONWriter json;

        if (aSensorStatus.m_TemperatureValueTS != DateTime::NullDate) {
          json.add("TemperatureValue", aSensorStatus.m_TemperatureValue);
          json.add("TemperatureValueTime", aSensorStatus.m_TemperatureValueTS.toISO8601_ms());
        }
        if (aSensorStatus.m_HumidityValueTS != DateTime::NullDate) {
          json.add("HumidityValue", aSensorStatus.m_HumidityValue);
          json.add("HumidityValueTime", aSensorStatus.m_HumidityValueTS.toISO8601_ms());
        }
        if (aSensorStatus.m_BrightnessValueTS != DateTime::NullDate) {
          json.add("BrightnessValue", aSensorStatus.m_BrightnessValue);
          json.add("BrightnessValueTime", aSensorStatus.m_BrightnessValueTS.toISO8601_ms());
        }
        if (aSensorStatus.m_WeatherTS != DateTime::NullDate) {
          json.add("WeatherIconId", aSensorStatus.m_WeatherIconId);
          json.add("WeatherConditionId", aSensorStatus.m_WeatherConditionId);
          json.add("WeatherServiceId", aSensorStatus.m_WeatherServiceId);
          json.add("WeatherServiceTime", aSensorStatus.m_WeatherTS.toISO8601_ms());
        }

        json.startArray("zones");
        std::vector<boost::shared_ptr<Zone> > zoneList = m_Apartment.getZones();
        foreach(boost::shared_ptr<Zone> pZone, zoneList) {
          if (pZone->getID() == 0) {
            continue;
          }
          ZoneSensorStatus_t sensorStatus = pZone->getSensorStatus();
          json.startObject();
          json.add("id", pZone->getID());
          json.add("name", pZone->getName());

          json.startArray("values");

          if (sensorStatus.m_TemperatureValueTS != DateTime::NullDate) {
            json.startObject();
            json.add("TemperatureValue", sensorStatus.m_TemperatureValue);
            json.add("TemperatureValueTime", sensorStatus.m_TemperatureValueTS.toISO8601_ms());
            json.endObject();
          }
          if (sensorStatus.m_HumidityValueTS != DateTime::NullDate) {
            json.startObject();
            json.add("HumidityValue", sensorStatus.m_HumidityValue);
            json.add("HumidityValueTime", sensorStatus.m_HumidityValueTS.toISO8601_ms());
            json.endObject();
          }
          if (sensorStatus.m_CO2ConcentrationValueTS != DateTime::NullDate) {
            json.startObject();
            json.add("CO2concentrationValue", sensorStatus.m_CO2ConcentrationValue);
            json.add("CO2concentrationValueTime", sensorStatus.m_CO2ConcentrationValueTS.toISO8601_ms());
            json.endObject();
          }
          if (sensorStatus.m_BrightnessValueTS != DateTime::NullDate) {
            json.startObject();
            json.add("BrightnessValue", sensorStatus.m_BrightnessValue);
            json.add("BrightnessValueTime", sensorStatus.m_BrightnessValueTS.toISO8601_ms());
            json.endObject();
          }
          json.endArray();
          json.endObject();
        }
        json.endArray();
        return json.successJSON();
      } else if(_request.getMethod() == "getModelFeatures") {
        JSONWriter json;

        json.startObject("modelFeatures");
        for (int colorID = ColorIDYellow; colorID <= ColorIDBlack; colorID++) {
          std::vector<boost::shared_ptr<std::pair<std::string, boost::shared_ptr<const std::vector<int> > > > > f = ModelFeatures::getInstance()->getFeatures(colorID);
          json.startObject(ModelFeatures::getInstance()->colorToString(colorID));

          for (size_t i = 0; i < f.size(); i++) {
            boost::shared_ptr<std::pair<std::string, boost::shared_ptr<const std::vector<int> > > > model = f.at(i);
            json.startObject(model->first);

            for (size_t devf = 0; devf < model->second->size(); devf++) {
              int feature = model->second->at(devf);
              json.add(ModelFeatures::getInstance()->getFeatureName(feature), true);
            }
            json.endObject();
          }
          json.endObject();
        }
        json.endObject();
        
        json.startObject("reference");
        boost::shared_ptr<std::vector<int> > all = ModelFeatures::getInstance()->getAvailableFeatures();
        for (size_t a = 0; a < all->size(); a++) {
          json.add(ModelFeatures::getInstance()->getFeatureName(all->at(a)), false);
        }
        json.endObject();

        return json.successJSON();
      } else if (_request.getMethod() == "getClusterLocks") {
        JSONWriter json;

        std::pair<std::vector<DeviceLock_t>, std::vector<ZoneLock_t> > locks;
        std::vector<DeviceLock_t> lockedDevices;
        std::vector<ZoneLock_t> lockedZones;

        locks = m_Apartment.getClusterLocks();
        lockedDevices = locks.first;
        lockedZones = locks.second;

        json.startArray("lockedDevices");

        for (size_t i = 0; i < lockedDevices.size(); i++) {
          json.startObject();
          json.add("dsuid", dsuid2str(lockedDevices.at(i).dsuid));

          json.startArray("lockedScenes");
          for (size_t j = 0; j < lockedDevices.at(i).lockedScenes.size(); j++) {
            json.add(lockedDevices.at(i).lockedScenes.at(j));
          }
          json.endArray();

          json.endObject();
        }
        json.endArray();

        // lockedZones: [ { zoneID: xx, deviceClasses: [ { deviceClass: 3, lockedScenes: [ 1, 2, 3 ] } ] }, ... ]
        json.startArray("lockedZones");
        for (size_t i = 0; i < lockedZones.size(); i++) {
          ZoneLock_t zl = lockedZones.at(i);
          std::set<int> lockedApartmentScenes;
          json.startObject();
          json.add("zoneID", zl.zoneID);

          json.startArray("deviceClasses");
          for (size_t k = 0; k < zl.deviceClassLocks.size(); k++) {
            ClassLock_t cl = zl.deviceClassLocks.at(k);
            json.startObject();
            json.add("deviceClass", cl.deviceClass);
            json.startArray("lockedScenes");
            for (std::set<int>::iterator it = cl.lockedScenes.begin(); 
                                         it != cl.lockedScenes.end(); it++) {
              int scene = *it;
              json.add(scene);
              // collect apartment scenes
              if (scene >= SceneAutoStandBy) {
                  lockedApartmentScenes.insert(scene);
              }
            }
            json.endArray();
            json.endObject();
          }
          json.endArray();

          json.startArray("lockedApartmentScenes");

          for (std::set<int>::iterator it = lockedApartmentScenes.begin(); 
                                       it != lockedApartmentScenes.end(); it++) {
              json.add(*it);
          }

          json.endArray();

          json.endObject();
        }
        json.endArray();

        return json.successJSON();
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
