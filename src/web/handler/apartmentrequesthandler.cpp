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

#include "src/event.h"
#include "src/model/apartment.h"
#include "src/model/zone.h"
#include "src/model/modelconst.h"
#include "src/model/modulator.h"
#include "src/model/device.h"
#include "src/model/deviceinterface.h"
#include "src/model/group.h"
#include "src/model/cluster.h"
#include "src/model/set.h"
#include "src/model/state.h"
#include "src/model/modelmaintenance.h"
#include "src/stringconverter.h"
#include "src/model-features.h"
#include "src/vdc-db.h"
#include "util.h"
#include "jsonhelper.h"
#include "vdc-info.h"
#include "comm-channel.h"

#include "zonerequesthandler.h"

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
      if (zone == NULL) {
        continue;
      }

      json.startObject();
      json.add("zoneID", zone->getID());
      json.add("name", zone->getName());
      json.startArray("groups");

      std::vector<boost::shared_ptr<Group> > groups = zone->getGroups();
      for (size_t g = 0; g < groups.size(); g++) {
        boost::shared_ptr<Group> group = groups.at(g);
        if (group->getID() == GroupIDBroadcast) {
            continue;
        }
        if (zone->getID() == 0) {
          if (group->getID() < GroupIDGlobalAppMin) {
            continue;
          }
        } else {
          if ((group->getID() == GroupIDBlack) || (group->getID() == GroupIDRed) || (group->getID() == GroupIDGreen)) {
            continue;
          }
        }

        bool hasRelevantDevices = false;
        Set devices = group->getDevices();
        for (int d = 0; d < devices.length(); d++) {
          boost::shared_ptr<Device> device = devices.get(d).getDevice();
          if ((device->hasOutput() && (device->getOutputMode() != 0)) || device->getHasActions()) {
            hasRelevantDevices = true;
            break;
          }
        } // devices loop

        if (hasRelevantDevices || group->hasConnectedDevices()) {
          json.add(group->getID());
        }
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

        bool showHidden = false;
        _request.getParameter("showHidden", showHidden);

        JSONWriter json(JSONWriter::jsonArrayResult);
        toJSON(devices, json, showHidden);
        return json.successJSON();
      } else if(_request.getMethod() == "getCircuits") {
        JSONWriter json;

        json.startArray("circuits");
        std::vector<boost::shared_ptr<DSMeter> > dsMeters = m_Apartment.getDSMeters();
        foreach(boost::shared_ptr<DSMeter> dsMeter, dsMeters) {
          toJSON(dsMeter, json);
        }
        json.endArray();
        return json.successJSON();
      } else if(_request.getMethod() == "getVdcs") {
        JSONWriter json;

        std::string implementationId = _request.getParameter("implementationId");
        if (implementationId.empty()) {
          return JSONWriter::failure("Missing parameter 'implementationId'");
        }

        json.startArray("vdcs");
        std::vector<boost::shared_ptr<DSMeter> > dsMeters = m_Apartment.getDSMeters();
        foreach(boost::shared_ptr<DSMeter> dsMeter, dsMeters) {
          if (dsMeter->getVdcImplementationId() != implementationId) {
            continue;
          }
          toJSON(dsMeter, json);
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
          json.add("id", pZone->getID());
          json.add("name", pZone->getName());
          ZoneRequestHandler::addTemperatureControlStatus(json, pZone);
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
          json.add("id", pZone->getID());
          json.add("name", pZone->getName());
          ZoneRequestHandler::addTemperatureControlConfig(json, pZone);
          json.endObject();
        }
        json.endArray();
        return json.successJSON();

      } else if(_request.getMethod() == "getTemperatureControlConfig2") {
        JSONWriter json;

        json.startObject("zones");
        std::vector<boost::shared_ptr<Zone> > zoneList = m_Apartment.getZones();
        foreach(boost::shared_ptr<Zone> pZone, zoneList) {
          if (pZone->getID() == 0) {
            continue;
          }
          json.startObject(ds::str(pZone->getID()));
          json.add("name", pZone->getName());

          json.startObject("config");
          ZoneRequestHandler::addTemperatureControlConfig2(json, pZone);
          json.endObject();

          json.endObject();
        }
        json.endObject();
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
          json.add("id", pZone->getID());
          json.add("name", pZone->getName());
          ZoneRequestHandler::addTemperatureControlValues(json, pZone);
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

          foreach (auto&& devSensor, pZone->getAssignedSensors()) {
            json.startObject();
            json.add("sensorType", devSensor.m_sensorType);
            json.add("dsuid", devSensor.m_DSUID);
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

        json.startObject("weather");
        if (aSensorStatus.m_WeatherTS != DateTime::NullDate) {
          json.add("WeatherIconId", aSensorStatus.m_WeatherIconId);
          json.add("WeatherConditionId", aSensorStatus.m_WeatherConditionId);
          json.add("WeatherServiceId", aSensorStatus.m_WeatherServiceId);
          json.add("WeatherServiceTime", aSensorStatus.m_WeatherTS.toISO8601_ms());
        }
        json.endObject();

        json.startObject("outdoor");
        if (aSensorStatus.m_TemperatureTS != DateTime::NullDate) {
          json.startObject("temperature");
          json.add("value", aSensorStatus.m_TemperatureValue);
          json.add("time", aSensorStatus.m_TemperatureTS.toISO8601_ms());
          json.endObject();
        }
        if (aSensorStatus.m_HumidityTS != DateTime::NullDate) {
          json.startObject("humidity");
          json.add("value", aSensorStatus.m_HumidityValue);
          json.add("time", aSensorStatus.m_HumidityTS.toISO8601_ms());
          json.endObject();
        }
        if (aSensorStatus.m_BrightnessTS != DateTime::NullDate) {
          json.startObject("brightness");
          json.add("value", aSensorStatus.m_BrightnessValue);
          json.add("time", aSensorStatus.m_BrightnessTS.toISO8601_ms());
          json.endObject();
        }
        if (aSensorStatus.m_WindSpeedTS != DateTime::NullDate) {
          json.startObject("windspeed");
          json.add("value", aSensorStatus.m_WindSpeedValue);
          json.add("time", aSensorStatus.m_WindSpeedTS.toISO8601_ms());
          json.endObject();
        }
        if (aSensorStatus.m_WindDirectionTS != DateTime::NullDate) {
          json.startObject("winddirection");
          json.add("value", aSensorStatus.m_WindDirectionValue);
          json.add("time", aSensorStatus.m_WindDirectionTS.toISO8601_ms());
          json.endObject();
        }
        if (aSensorStatus.m_GustSpeedTS != DateTime::NullDate) {
          json.startObject("gustspeed");
          json.add("value", aSensorStatus.m_GustSpeedValue);
          json.add("time", aSensorStatus.m_GustSpeedTS.toISO8601_ms());
          json.endObject();
        }
        if (aSensorStatus.m_GustDirectionTS != DateTime::NullDate) {
          json.startObject("gustdirection");
          json.add("value", aSensorStatus.m_GustDirectionValue);
          json.add("time", aSensorStatus.m_GustDirectionTS.toISO8601_ms());
          json.endObject();
        }
        if (aSensorStatus.m_PrecipitationTS != DateTime::NullDate) {
          json.startObject("precipitation");
          json.add("value", aSensorStatus.m_PrecipitationValue);
          json.add("time", aSensorStatus.m_PrecipitationTS.toISO8601_ms());
          json.endObject();
        }
        if (aSensorStatus.m_AirPressureTS != DateTime::NullDate) {
          json.startObject("airpressure");
          json.add("value", aSensorStatus.m_AirPressureValue);
          json.add("time", aSensorStatus.m_AirPressureTS.toISO8601_ms());
          json.endObject();
        }
        json.endObject();

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

          if (sensorStatus.m_TemperatureValueTS != DateTime::NullDate && sensorStatus.m_TemperatureValue) {
            json.startObject();
            json.add("TemperatureValue", *sensorStatus.m_TemperatureValue);
            json.add("TemperatureValueTime", sensorStatus.m_TemperatureValueTS.toISO8601_ms());
            json.endObject();
          }
          if (sensorStatus.m_HumidityValueTS != DateTime::NullDate && sensorStatus.m_HumidityValue) {
            json.startObject();
            json.add("HumidityValue", *sensorStatus.m_HumidityValue);
            json.add("HumidityValueTime", sensorStatus.m_HumidityValueTS.toISO8601_ms());
            json.endObject();
          }
          if (sensorStatus.m_CO2ConcentrationValueTS != DateTime::NullDate && sensorStatus.m_CO2ConcentrationValue) {
            json.startObject();
            json.add("CO2concentrationValue", *sensorStatus.m_CO2ConcentrationValue);
            json.add("CO2concentrationValueTime", sensorStatus.m_CO2ConcentrationValueTS.toISO8601_ms());
            json.endObject();
          }
          if (sensorStatus.m_BrightnessValueTS != DateTime::NullDate && sensorStatus.m_BrightnessValue) {
            json.startObject();
            json.add("BrightnessValue", *sensorStatus.m_BrightnessValue);
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
        for (int colorID = ColorIDYellow; colorID <= ColorIDWhite; colorID++) {
          auto&& f = ModelFeatures::getInstance()->getFeatures(colorID);
          json.startObject(ModelFeatures::getInstance()->colorToString(colorID));

          for (size_t i = 0; i < f.size(); i++) {
            auto&& model = f.at(i);
            json.startObject(model->first);

            foreach(auto&& feature, *model->second) {
              if (auto&& name = modelFeatureName(feature)) {
                json.add(*name, true);
              }
            }
            json.endObject();
          }
          json.endObject();
        }
        json.endObject();
        
        json.startObject("reference");
        foreach (auto&& feature, ModelFeatures::getInstance()->getAvailableFeatures()) {
          if (auto&& name = modelFeatureName(feature)) {
            json.add(*name, false);
          }
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
          json.add("dsuid", lockedDevices.at(i).dsuid);

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
      } else if (_request.getMethod() == "setDevicesFirstSeen") {

        if (!_request.hasParameter("time")) {
          return JSONWriter::failure("missing parameter 'time'");
        }
        std::string  strTimestamp = _request.getParameter("time");
        DateTime setTime = DateTime::parseISO8601(strTimestamp);
        if (!m_Apartment.setDevicesFirstSeen(setTime)) {
          return JSONWriter::failure("can not set date. Date too old.");
        }

        // log X-DS-TrackingID
        if (_connection) {
          const char * data = mg_get_header(_connection, "X-DS-TrackingID");
          std::string token;
          if (data) {
            token = data;

            // create event, log entry ends in system-event.log
            boost::shared_ptr<Event> evtDevicesFirstSeen = boost::make_shared<Event>(EventName::DevicesFirstSeen);
            evtDevicesFirstSeen->setProperty("dateTime", setTime.toISO8601());
            evtDevicesFirstSeen->setProperty("X-DS-TrackingID", token);
            if (DSS::hasInstance()) {
              DSS::getInstance()->getEventQueue().pushEvent(evtDevicesFirstSeen);
            }

            Logger::getInstance()->log("ApartmentRequestHandler: setDevicesFirstSeen: Date set to: " + 
              setTime.toString() + "  X-DS-TrackingID: " + token, lsNotice);
          }
        }

        return JSONWriter::success();
      } else if (_request.getMethod() == "getDeviceBinaryInputs") {
        JSONWriter json;

        Apartment& apt = DSS::getInstance()->getApartment();
        Set devices = apt.getZone(0)->getDevices();

        json.startArray("devices");

        for (int d = 0; d < devices.length(); d++) {
          boost::shared_ptr<Device> device = devices.get(d).getDevice();
          if ((device->getDeviceType() == DEVICE_TYPE_AKM) &&
              (device->isPresent() == true)) {
            auto&& binaryInputs = device->getBinaryInputs();

            if (!binaryInputs.empty()) {
              json.startObject();
              json.add("dsuid", device->getDSID());

              json.startArray("binaryInputs");

              for (size_t i = 0; i < binaryInputs.size(); i++) {
                auto&& input = binaryInputs.at(i);
                json.startObject();
                json.add("targetGroup", input->m_targetGroupId);
                json.add("inputType", input->m_inputType);
                json.add("inputId", input->m_inputId);
                json.add("state", input->getState().getState());
                json.endObject();
              }

              json.endArray();
              json.endObject();
            }
          }
        }

        json.endArray();
        return json.successJSON();
      } else if (_request.getMethod() == "getDeviceInfo") {
        std::string filterParam;
        int onlyActive = 0;
        std::string activeStr;

        // "filter" can be a comma separated combination of:
        // spec
        // stateDesc
        // propertyDesc
        // actionDesc
        // standardActions
        // customActions
        _request.getParameter("filter", filterParam);
        _request.getParameter("onlyActive", activeStr);
        onlyActive = strToIntDef(activeStr, 0);

        std::string langCode("");
        _request.getParameter("lang", langCode);

        auto filter = vdcInfo::parseFilter(filterParam);

        VdcDb db(*DSS::getInstance());
        JSONWriter json;

        Apartment& apt = DSS::getInstance()->getApartment();
        // get all present devices
        Set devices = apt.getZone(0)->getDevices();
        if (onlyActive) {
            devices = devices.getByPresence(true);
        }

        json.startArray("devices");

        for (int d = 0; d < devices.length(); d++) {
          boost::shared_ptr<Device> device = devices.get(d).getDevice();

          if (!device->getHasActions()) {
            continue;
          }
 
          json.startObject();
          json.add("dSUID", device->getDSID());
          // do not fail the whole set if one devices messes up
          try {
            vdcInfo::addByFilter(db, *device, filter, langCode, json);
          } catch (std::exception& e) {
            Logger::getInstance()->log(std::string("Could get device properties for device ") + dsuid2str(device->getDSID()) + ": " + e.what(), lsError);
          }
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
