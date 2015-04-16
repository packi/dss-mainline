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

#include "zonerequesthandler.h"

#include <digitalSTROM/dsuid.h>

#include "src/model/zone.h"
#include "src/model/group.h"
#include "src/model/set.h"
#include "src/model/device.h"
#include "src/model/apartment.h"
#include "src/model/scenehelper.h"
#include "src/model/modelmaintenance.h"
#include "src/structuremanipulator.h"
#include "src/stringconverter.h"
#include "util.h"

namespace dss {


  //=========================================== ZoneRequestHandler

  ZoneRequestHandler::ZoneRequestHandler(Apartment& _apartment,
                                         StructureModifyingBusInterface* _pBusInterface,
                                         StructureQueryBusInterface* _pQueryBusInterface)
  : m_Apartment(_apartment),
    m_pStructureBusInterface(_pBusInterface),
    m_pStructureQueryBusInterface(_pQueryBusInterface)
  { }


  WebServerResponse ZoneRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    StringConverter st("UTF-8", "UTF-8");
    bool ok = true;
    std::string errorMessage;
    std::string zoneName = _request.getParameter("name");
    std::string zoneIDString = _request.getParameter("id");
    boost::shared_ptr<Zone> pZone;
    if(!zoneIDString.empty()) {
      int zoneID = strToIntDef(zoneIDString, -1);
      if(zoneID != -1) {
        try {
          pZone = m_Apartment.getZone(zoneID);
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
        pZone = m_Apartment.getZone(zoneName);
      } catch(std::runtime_error& e) {
        ok = false;
        errorMessage = "Could not find zone named '" + zoneName + "'";
      }
    } else {
      ok = false;
      errorMessage = "Need parameter name or id to identify zone";
    }
    if(ok) {
      boost::shared_ptr<Group> pGroup;
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
          boost::shared_ptr<IDeviceInterface> interface;
          if(pGroup != NULL) {
            interface = pGroup;
          }
          if(ok) {
            if(interface == NULL) {
              interface = pZone;
            }
            return handleDeviceInterfaceRequest(_request, interface, _session);
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
          JSONWriter json;
          json.add("scene", lastScene);
          return json.successJSON();
        } else if(_request.getMethod() == "getName") {
          JSONWriter json;
          json.add("name", pZone->getName());
          return json.successJSON();
        } else if(_request.getMethod() == "setName") {
          if (_request.hasParameter("newName")) {
            std::string newName = _request.getParameter("newName");
            newName = escapeHTML(newName);
            pZone->setName(newName);
          } else {
            return JSONWriter::failure("missing parameter 'newName'");
          }
          return JSONWriter::success();
        } else if(_request.getMethod() == "sceneSetName") {
          if(pGroup == NULL) {
            return JSONWriter::failure("Need group to work");
          }
          int sceneNumber = strToIntDef(_request.getParameter("sceneNumber"), -1);
          if(sceneNumber == -1) {
            return JSONWriter::failure("Need valid parameter 'sceneNumber'");
          }
          if(!SceneHelper::isInRange(sceneNumber, pGroup->getZoneID())) {
            return JSONWriter::failure("Parameter 'sceneNumber' out of bounds ('" + intToString(sceneNumber) + "')");
          }

          if (_request.hasParameter("newName")) {
            std::string name = st.convert(_request.getParameter("newName"));
            name = escapeHTML(name);

            pGroup->setSceneName(sceneNumber, name);
            StructureManipulator manipulator(*m_pStructureBusInterface, *m_pStructureQueryBusInterface, m_Apartment);
            manipulator.sceneSetName(pGroup, sceneNumber, name);
            // raise ModelDirty to force rewrite of model data to apartment.xml
            DSS::getInstance()->getModelMaintenance().addModelEvent(new ModelEvent(ModelEvent::etModelDirty));
          } else {
            return JSONWriter::failure("missing parameter 'newName'");
          }
          return JSONWriter::success();
        } else if(_request.getMethod() == "sceneGetName") {
          if(pGroup == NULL) {
            return JSONWriter::failure("Need group to work");
          }
          int sceneNumber = strToIntDef(_request.getParameter("sceneNumber"), -1);
          if(sceneNumber == -1) {
            return JSONWriter::failure("Need valid parameter 'sceneNumber'");
          }
          if(!SceneHelper::isInRange(sceneNumber, pGroup->getZoneID())) {
            return JSONWriter::failure("Parameter 'sceneNumber' out of bounds ('" + intToString(sceneNumber) + "')");
          }

          std::string sceneName = pGroup->getSceneName(sceneNumber);
          JSONWriter json;
          json.add("name", sceneName);
          return json.successJSON();
        } else if(_request.getMethod() == "getReachableScenes") {
          uint64_t reachableScenes = 0uLL;
          Set devicesInZone;
          if (pGroup) {
            devicesInZone = pZone->getDevices().getByGroup(pGroup);
          } else {
            devicesInZone = pZone->getDevices();
          }
          for(int iDevice = 0; iDevice < devicesInZone.length(); iDevice++) {
            DeviceReference& ref = devicesInZone.get(iDevice);
            int buttonID = ref.getDevice()->getButtonID();
            reachableScenes |= SceneHelper::getReachableScenesBitmapForButtonID(buttonID);
          }
          JSONWriter json;
          json.startArray("reachableScenes");
          for(int iBit = 0; iBit < 64; iBit++) {
            if((reachableScenes & (1uLL << iBit)) != 0uLL) {
              json.add(iBit);
            }
          }
          json.endArray();
          return json.successJSON();

        } else if(_request.getMethod() == "getTemperatureControlStatus") {
          if (pZone->getID() == 0) {
            return JSONWriter::failure("Zone id 0 is invalid");
          }

          JSONWriter json;
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingStatus_t hStatus = pZone->getHeatingStatus();
          ZoneSensorStatus_t hSensors = pZone->getSensorStatus();
          if (hProp.m_HeatingControlDSUID == DSUID_NULL) {
            return JSONWriter::failure("Not a heating control device");
          }

          json.add("IsConfigured", true);
          json.add("ControlMode", hProp.m_HeatingControlMode);
          json.add("ControlState", hProp.m_HeatingControlState);
          json.add("ControlDSUID", dsuid2str(hProp.m_HeatingControlDSUID));

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
            case HeatingControlModeIDManual:
              json.add("OperationMode", pZone->getHeatingOperationMode());
              json.add("ControlValue", hStatus.m_ControlValue);
              break;
          }
          return json.successJSON();

        } else if(_request.getMethod() == "getTemperatureControlConfig") {
          if (pZone->getID() == 0) {
            return JSONWriter::failure("Zone id 0 is invalid");
          }

          JSONWriter json;
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingConfigSpec_t hConfig;

          memset(&hConfig, 0, sizeof(hConfig));
          if (hProp.m_HeatingControlDSUID == DSUID_NULL) {
            return JSONWriter::failure("Not a heating control device");
          } else {
            json.add("IsConfigured", true);
            hConfig = m_Apartment.getBusInterface()->getStructureQueryBusInterface()->getZoneHeatingConfig(
                hProp.m_HeatingControlDSUID, pZone->getID());
          }

          json.add("ControlDSUID", dsuid2str(hProp.m_HeatingControlDSUID));
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
            case HeatingControlModeIDManual:
              json.add("ManualValue", hConfig.ManualValue - 100);
              break;
            case HeatingControlModeIDFixed:
              break;
          }
          return json.successJSON();

        } else if(_request.getMethod() == "setTemperatureControlConfig") {
          if (pZone->getID() == 0) {
            return JSONWriter::failure("Zone id 0 is invalid");
          }

          JSONWriter json;
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingConfigSpec_t hConfig;

          if (_request.hasParameter("ControlDSUID")) {
            dsuid_from_string(_request.getParameter("ControlDSUID").c_str(), &hProp.m_HeatingControlDSUID);
          }

          memset(&hConfig, 0, sizeof(hConfig));
          if (hProp.m_HeatingControlDSUID == DSUID_NULL) {
            return JSONWriter::failure("Not a heating control device");
          } else {
            json.add("IsConfigured", true);
            hConfig = m_Apartment.getBusInterface()->getStructureQueryBusInterface()->getZoneHeatingConfig(
                hProp.m_HeatingControlDSUID, pZone->getID());
          }

          if (_request.hasParameter("ControlMode")) {
            (void) _request.getParameter("ControlMode", hConfig.ControllerMode);
          }
          if (_request.hasParameter("ReferenceZone")) {
            (void) _request.getParameter("ReferenceZone", hConfig.SourceZoneId);
          }
          if (_request.hasParameter("CtrlOffset")) {
            int offset;
            (void) _request.getParameter("CtrlOffset", offset);
            hConfig.Offset = offset;
          }
          if (_request.hasParameter("EmergencyValue")) {
            (void) _request.getParameter("EmergencyValue", hConfig.EmergencyValue);
            hConfig.EmergencyValue += 100;
          }
          if (_request.hasParameter("ManualValue")) {
            (void) _request.getParameter("ManualValue", hConfig.ManualValue);
            hConfig.ManualValue += 100;
          }
          if (_request.hasParameter("CtrlYmin")) {
            (void) _request.getParameter("CtrlYmin", hConfig.Ymin);
            hConfig.Ymin += 100;
          }
          if (_request.hasParameter("CtrlYmax")) {
            (void) _request.getParameter("CtrlYmax", hConfig.Ymax);
            hConfig.Ymax += 100;
          }
          double tempDouble = 0;
          if (_request.getParameter("CtrlKp", tempDouble)) {
            hConfig.Kp = (uint16_t)tempDouble * 40;
          }
          (void) _request.getParameter("CtrlTi", hConfig.Ti);
          (void) _request.getParameter("CtrlTs", hConfig.Ts);
          (void) _request.getParameter("CtrlKd", hConfig.Kd);
          if (_request.getParameter("CtrlImin", tempDouble)) {
            hConfig.Imin = tempDouble * 40;
          }
          if (_request.getParameter("CtrlImax", tempDouble)) {
            hConfig.Imax = tempDouble * 40;
          }
          bool tempBool;
          if (_request.getParameter("CtrlAntiWindUp", tempBool)) {
            hConfig.AntiWindUp = tempBool ? 1 : 0;
          }
          if (_request.getParameter("CtrlKeepFloorWarm", tempBool)) {
            hConfig.KeepFloorWarm = tempBool ? 1 : 0;
          }

          StructureManipulator manipulator(*m_pStructureBusInterface, *m_pStructureQueryBusInterface, m_Apartment);
          manipulator.setZoneHeatingConfig(pZone, hProp.m_HeatingControlDSUID, hConfig);
          return json.successJSON();

        } else if(_request.getMethod() == "getTemperatureControlValues") {
          if (pZone->getID() == 0) {
            return JSONWriter::failure("Zone id 0 is invalid");
          }

          JSONWriter json;
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingOperationModeSpec_t hOpValues;

          memset(&hOpValues, 0, sizeof(hOpValues));
          if (hProp.m_HeatingControlDSUID == DSUID_NULL) {
            return JSONWriter::failure("Not a heating control device");
          } else {
            json.add("IsConfigured", true);
            hOpValues = m_Apartment.getBusInterface()->getStructureQueryBusInterface()->getZoneHeatingOperationModes(
                hProp.m_HeatingControlDSUID, pZone->getID());
          }

          switch (hProp.m_HeatingControlMode) {
          case HeatingControlModeIDOff:
            break;
          case HeatingControlModeIDPID:
            json.add("Off",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode0));
            json.add("Comfort",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode1));
            json.add("Economy",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode2));
            json.add("NotUsed",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode3));
            json.add("Night",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode4));
            json.add("Holiday",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode5));
            break;
          case HeatingControlModeIDZoneFollower:
            break;
          case HeatingControlModeIDFixed:
            json.add("Off",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode0));
            json.add("Comfort",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode1));
            json.add("Economy",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode2));
            json.add("NotUsed",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode3));
            json.add("Night",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode4));
            json.add("Holiday",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode5));
            break;

          }
          return json.successJSON();

        } else if(_request.getMethod() == "setTemperatureControlValues") {
          if (pZone->getID() == 0) {
            return JSONWriter::failure("Zone id 0 is invalid");
          }

          JSONWriter json;
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingOperationModeSpec_t hOpValues;
          int SensorConversion;
          int iValue;
          double fValue;

          if (hProp.m_HeatingControlMode == HeatingControlModeIDPID) {
            SensorConversion = SensorIDRoomTemperatureSetpoint;
          } else if (hProp.m_HeatingControlMode == HeatingControlModeIDFixed) {
            SensorConversion = SensorIDRoomTemperatureControlVariable;
          } else {
            return JSONWriter::failure("Cannot set control values in current mode");
          }

          memset(&hOpValues, 0, sizeof(hOpValues));
          if (hProp.m_HeatingControlDSUID == DSUID_NULL) {
            return JSONWriter::failure("Not a heating control device");
          } else {
            json.add("IsConfigured", true);
            hOpValues = m_Apartment.getBusInterface()->getStructureQueryBusInterface()->getZoneHeatingOperationModes(
                hProp.m_HeatingControlDSUID, pZone->getID());
          }

          if (_request.hasParameter("Off")) {
            try {
              iValue = strToUInt(_request.getParameter("Off"));
              hOpValues.OpMode0 = SceneHelper::sensorToSystem(SensorConversion, iValue);
            } catch(std::invalid_argument& e) {
              try {
                fValue = strToDouble(_request.getParameter("Off"));
                hOpValues.OpMode0 = SceneHelper::sensorToSystem(SensorConversion, fValue);
              } catch(std::invalid_argument& e) {}
            }
          }
          if (_request.hasParameter("Comfort")) {
            try {
              iValue = strToUInt(_request.getParameter("Comfort"));
              hOpValues.OpMode1 = SceneHelper::sensorToSystem(SensorConversion, iValue);
            } catch(std::invalid_argument& e) {
              try {
                fValue = strToDouble(_request.getParameter("Comfort"));
                hOpValues.OpMode1 = SceneHelper::sensorToSystem(SensorConversion, fValue);
              } catch(std::invalid_argument& e) {}
            }
          }
          if (_request.hasParameter("Economy")) {
            try {
              iValue = strToUInt(_request.getParameter("Economy"));
              hOpValues.OpMode2 = SceneHelper::sensorToSystem(SensorConversion, iValue);
            } catch(std::invalid_argument& e) {
              try {
                fValue = strToDouble(_request.getParameter("Economy"));
                hOpValues.OpMode2 = SceneHelper::sensorToSystem(SensorConversion, fValue);
              } catch(std::invalid_argument& e) {}
            }
          }
          if (_request.hasParameter("NotUsed")) {
            try {
              iValue = strToUInt(_request.getParameter("NotUsed"));
              hOpValues.OpMode3 = SceneHelper::sensorToSystem(SensorConversion, iValue);
            } catch(std::invalid_argument& e) {
              try {
                fValue = strToDouble(_request.getParameter("NotUsed"));
                hOpValues.OpMode3 = SceneHelper::sensorToSystem(SensorConversion, fValue);
              } catch(std::invalid_argument& e) {}
            }
          }
          if (_request.hasParameter("Night")) {
            try {
              iValue = strToUInt(_request.getParameter("Night"));
              hOpValues.OpMode4 = SceneHelper::sensorToSystem(SensorConversion, iValue);
            } catch(std::invalid_argument& e) {
              try {
                fValue = strToDouble(_request.getParameter("Night"));
                hOpValues.OpMode4 = SceneHelper::sensorToSystem(SensorConversion, fValue);
              } catch(std::invalid_argument& e) {}
            }
          }
          if (_request.hasParameter("Holiday")) {
            try {
              iValue = strToUInt(_request.getParameter("Holiday"));
              hOpValues.OpMode5 = SceneHelper::sensorToSystem(SensorConversion, iValue);
            } catch(std::invalid_argument& e) {
              try {
                fValue = strToDouble(_request.getParameter("Holiday"));
                hOpValues.OpMode5 = SceneHelper::sensorToSystem(SensorConversion, fValue);
              } catch(std::invalid_argument& e) {}
            }
          }

          m_Apartment.getBusInterface()->getStructureModifyingBusInterface()->setZoneHeatingOperationModes(
              hProp.m_HeatingControlDSUID, pZone->getID(), hOpValues);

          return json.successJSON();

        } else if(_request.getMethod() == "setTemperatureControlState") {
          if (pZone->getID() == 0) {
            return JSONWriter::failure("Zone id 0 is invalid");
          }

          JSONWriter json;
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingStateSpec_t hState;

          if (hProp.m_HeatingControlDSUID == DSUID_NULL) {
            return JSONWriter::failure("Not a heating control device");
          } else {
            json.add("IsConfigured", true);
            hState = m_Apartment.getBusInterface()->getStructureQueryBusInterface()->getZoneHeatingState(
                hProp.m_HeatingControlDSUID, pZone->getID());
          }

          int value = hState.State;
          std::string rState = _request.getParameter("ControlState");
          if (rState == "internal") {
            value = HeatingControlStateIDInternal;
          } else if (rState == "external") {
            value = HeatingControlStateIDExternal;
          } else {
            try {
              value = strToUInt(_request.getParameter("ControlState"));
            } catch(std::invalid_argument& e) {
              return JSONWriter::failure("Invalid mode for the control state");
            }
          }
          hState.State = (uint8_t) value;

          m_Apartment.getBusInterface()->getStructureModifyingBusInterface()->setZoneHeatingState(
              hProp.m_HeatingControlDSUID, pZone->getID(), hState);

        } else if(_request.getMethod() == "getTemperatureControlInternals") {
          if (pZone->getID() == 0) {
            return JSONWriter::failure("Zone id 0 is invalid");
          }

          JSONWriter json;
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingInternalsSpec_t hInternals;

          if (hProp.m_HeatingControlDSUID == DSUID_NULL) {
            return JSONWriter::failure("Not a heating control device");
          }

          json.add("IsConfigured", true);
          json.add("ControlDSUID", dsuid2str(hProp.m_HeatingControlDSUID));
          json.add("ControlMode", hProp.m_HeatingControlMode);
          json.add("ControlState", hProp.m_HeatingControlState);

          if (hProp.m_HeatingControlMode != HeatingControlModeIDPID) {
            return JSONWriter::failure("Not a PID controller");
          }
          if (hProp.m_HeatingControlState != HeatingControlStateIDInternal) {
            return JSONWriter::failure("PID controller not running");
          }
          memset(&hInternals, 0, sizeof(ZoneHeatingInternalsSpec_t));
          hInternals = m_Apartment.getBusInterface()->getStructureQueryBusInterface()->getZoneHeatingInternals(
              hProp.m_HeatingControlDSUID, pZone->getID());

          json.add("CtrlTRecent", (double) SceneHelper::sensorToFloat12(SensorIDTemperatureIndoors, hInternals.Trecent));
          json.add("CtrlTReference", (double) SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hInternals.Treference));
          json.add("CtrlTError", (double) hInternals.TError * 0.025);
          json.add("CtrlTErrorPrev", (double) hInternals.TErrorPrev * 0.025);
          json.add("CtrlIntegral", (double) hInternals.Integral * 0.025);
          json.add("CtrlYp", (double) hInternals.Yp * 0.01);
          json.add("CtrlYi", (double) hInternals.Yi * 0.01);
          json.add("CtrlYd", (double) hInternals.Yd *0.01);
          json.add("CtrlY", (double) SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hInternals.Y));
          json.add("CtrlAntiWindUp", hInternals.AntiWindUp);

          return json.successJSON();

        } else if(_request.getMethod() == "setSensorSource") {
          if (pZone->getID() == 0) {
            return JSONWriter::failure("Not allowed to assign sensor for zone 0");
          }
          std::string dsuidStr = _request.getParameter("dsuid");
          if (dsuidStr.empty()) {
            return JSONWriter::failure("Missing parameter 'dsuid'");
          }
          dsuid_t dsuid = str2dsuid(dsuidStr);

          int type = strToIntDef(_request.getParameter("sensorType"), -1);
          if (type < 0) {
            return JSONWriter::failure("Missing or invalid parameter 'sensorType'");
          }
          boost::shared_ptr<Device> dev = m_Apartment.getDeviceByDSID(dsuid);
          StructureManipulator manipulator(*m_pStructureBusInterface, *m_pStructureQueryBusInterface, m_Apartment);
          manipulator.setZoneSensor(pZone, type, dev);
          return JSONWriter::success();
        } else if(_request.getMethod() == "clearSensorSource") {
          if (pZone->getID() == 0) {
            return JSONWriter::success();
          }
          int type = strToIntDef(_request.getParameter("sensorType"), -1);
          if (type < 0) {
            return JSONWriter::failure("Missing or invalid parameter 'sensorType'");
          }
          StructureManipulator manipulator(*m_pStructureBusInterface, *m_pStructureQueryBusInterface, m_Apartment);
          manipulator.resetZoneSensor(pZone, type);
          return JSONWriter::success();
        } else if(_request.getMethod() == "getAssignedSensors") {
          JSONWriter json;
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
          return json.successJSON();

        } else if(_request.getMethod() == "getSensorValues") {
          ZoneSensorStatus_t sensorStatus = pZone->getSensorStatus();
          JSONWriter json;

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
          return json.successJSON();

        } else {
          throw std::runtime_error("Unhandled function");
        }
      }
    }
    if(!ok) {
      return JSONWriter::failure(errorMessage);
    } else {
      return JSONWriter::success(); // TODO: check this, we shouldn't get here
    }
  } // handleRequest

} // namespace dss
