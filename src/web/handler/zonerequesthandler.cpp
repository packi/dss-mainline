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


#include "zonerequesthandler.h"

#include <digitalSTROM/dsuid/dsuid.h>

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
#include "src/web/json.h"

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
          boost::shared_ptr<JSONObject> resultObj = boost::make_shared<JSONObject>();
          resultObj->addProperty("scene", lastScene);
          return success(resultObj);
        } else if(_request.getMethod() == "getName") {
          boost::shared_ptr<JSONObject> resultObj = boost::make_shared<JSONObject>();
          resultObj->addProperty("name", pZone->getName());
          return success(resultObj);
        } else if(_request.getMethod() == "setName") {
          if (_request.hasParameter("newName")) {

            std::string newName = st.convert(_request.getParameter("newName"));
            newName = escapeHTML(newName);

            pZone->setName(newName);
          } else {
            return failure("missing parameter 'newName'");
          }
          return success();
        } else if(_request.getMethod() == "sceneSetName") {
          if(pGroup == NULL) {
            return failure("Need group to work");
          }
          int sceneNumber = strToIntDef(_request.getParameter("sceneNumber"), -1);
          if(sceneNumber == -1) {
            return failure("Need valid parameter 'sceneNumber'");
          }
          if(!SceneHelper::isInRange(sceneNumber, pGroup->getZoneID())) {
            return failure("Parameter 'sceneNumber' out of bounds ('" + intToString(sceneNumber) + "')");
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
            return failure("missing parameter 'newName'");
          }
          return success();
        } else if(_request.getMethod() == "sceneGetName") {
          if(pGroup == NULL) {
            return failure("Need group to work");
          }
          int sceneNumber = strToIntDef(_request.getParameter("sceneNumber"), -1);
          if(sceneNumber == -1) {
            return failure("Need valid parameter 'sceneNumber'");
          }
          if(!SceneHelper::isInRange(sceneNumber, pGroup->getZoneID())) {
            return failure("Parameter 'sceneNumber' out of bounds ('" + intToString(sceneNumber) + "')");
          }

          std::string sceneName = pGroup->getSceneName(sceneNumber);
          boost::shared_ptr<JSONObject> resultObj = boost::make_shared<JSONObject>();
          resultObj->addProperty("name", sceneName);
          return success(resultObj);
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
          boost::shared_ptr<JSONObject> resultObj = boost::make_shared<JSONObject>();
          boost::shared_ptr<JSONArray<int> > reachableArray = boost::make_shared<JSONArray<int> >();
          resultObj->addElement("reachableScenes", reachableArray);
          for(int iBit = 0; iBit < 64; iBit++) {
            if((reachableScenes & (1uLL << iBit)) != 0uLL) {
              reachableArray->add(iBit);
            }
          }
          return success(resultObj);

        } else if(_request.getMethod() == "getTemperatureControlStatus") {
          if (pZone->getID() == 0) {
            return failure("Zone id 0 is invalid");
          }

          boost::shared_ptr<JSONObject> resultObj = boost::make_shared<JSONObject>();
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingStatus_t hStatus = pZone->getHeatingStatus();
          ZoneSensorStatus_t hSensors = pZone->getSensorStatus();
          if (IsNullDsuid(hProp.m_HeatingControlDSUID)) {
            return failure("Not a heating control device");
          }

          resultObj->addProperty("IsConfigured", true);
          resultObj->addProperty("ControlMode", hProp.m_HeatingControlMode);
          resultObj->addProperty("ControlState", hProp.m_HeatingControlState);
          resultObj->addProperty("ControlDSUID", dsuid2str(hProp.m_HeatingControlDSUID));

          switch (hProp.m_HeatingControlMode) {
            case HeatingControlModeIDOff:
              break;
            case HeatingControlModeIDPID:
              resultObj->addProperty("OperationMode", hStatus.m_OperationMode);
              resultObj->addProperty("TemperatureValue", hSensors.m_TemperatureValue);
              resultObj->addProperty("TemperatureValueTime", hSensors.m_TemperatureValueTS.toISO8601());
              resultObj->addProperty("NominalValue", hStatus.m_NominalValue);
              resultObj->addProperty("NominalValueTime", hStatus.m_NominalValueTS.toISO8601());
              resultObj->addProperty("ControlValue", hStatus.m_ControlValue);
              resultObj->addProperty("ControlValueTime", hStatus.m_ControlValueTS.toISO8601());
              break;
            case HeatingControlModeIDZoneFollower:
              resultObj->addProperty("ControlValue", hStatus.m_ControlValue);
              resultObj->addProperty("ControlValueTime", hStatus.m_ControlValueTS.toISO8601());
              break;
            case HeatingControlModeIDFixed:
            case HeatingControlModeIDManual:
              resultObj->addProperty("OperationMode", hStatus.m_OperationMode);
              resultObj->addProperty("ControlValue", hStatus.m_ControlValue);
              break;
          }
          return success(resultObj);

        } else if(_request.getMethod() == "getTemperatureControlConfig") {
          if (pZone->getID() == 0) {
            return failure("Zone id 0 is invalid");
          }

          boost::shared_ptr<JSONObject> resultObj = boost::make_shared<JSONObject>();
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingConfigSpec_t hConfig;

          memset(&hConfig, 0, sizeof(hConfig));
          if (IsNullDsuid(hProp.m_HeatingControlDSUID)) {
            return failure("Not a heating control device");
          } else {
            resultObj->addProperty("IsConfigured", true);
            hConfig = m_Apartment.getBusInterface()->getStructureQueryBusInterface()->getZoneHeatingConfig(
                hProp.m_HeatingControlDSUID, pZone->getID());
          }

          resultObj->addProperty("ControlDSUID", dsuid2str(hProp.m_HeatingControlDSUID));
          resultObj->addProperty("ControlMode", hConfig.ControllerMode);
          resultObj->addProperty("EmergencyValue", hConfig.EmergencyValue - 100);
          switch (hProp.m_HeatingControlMode) {
            case HeatingControlModeIDOff:
              break;
            case HeatingControlModeIDPID:
              resultObj->addProperty("CtrlKp", (double)hConfig.Kp * 0.025);
              resultObj->addProperty("CtrlTs", hConfig.Ts);
              resultObj->addProperty("CtrlTi", hConfig.Ti);
              resultObj->addProperty("CtrlKd", hConfig.Kd);
              resultObj->addProperty("CtrlImin", (double)hConfig.Imin * 0.025);
              resultObj->addProperty("CtrlImax", (double)hConfig.Imax * 0.025);
              resultObj->addProperty("CtrlYmin", hConfig.Ymin - 100);
              resultObj->addProperty("CtrlYmax", hConfig.Ymax - 100);
              resultObj->addProperty("CtrlAntiWindUp", (hConfig.AntiWindUp > 0));
              resultObj->addProperty("CtrlKeepFloorWarm", (hConfig.KeepFloorWarm > 0));
              break;
            case HeatingControlModeIDZoneFollower:
              resultObj->addProperty("ReferenceZone", hConfig.SourceZoneId);
              resultObj->addProperty("CtrlOffset", hConfig.Offset);
              break;
            case HeatingControlModeIDManual:
              resultObj->addProperty("ManualValue", hConfig.ManualValue - 100);
              break;
            case HeatingControlModeIDFixed:
              break;
          }
          return success(resultObj);

        } else if(_request.getMethod() == "setTemperatureControlConfig") {
          if (pZone->getID() == 0) {
            return failure("Zone id 0 is invalid");
          }

          boost::shared_ptr<JSONObject> resultObj = boost::make_shared<JSONObject>();
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingConfigSpec_t hConfig;

          if (_request.hasParameter("ControlDSUID")) {
            dsuid_from_string(_request.getParameter("ControlDSUID").c_str(), &hProp.m_HeatingControlDSUID);
          }

          memset(&hConfig, 0, sizeof(hConfig));
          if (IsNullDsuid(hProp.m_HeatingControlDSUID)) {
            return failure("Not a heating control device");
          } else {
            resultObj->addProperty("IsConfigured", true);
            hConfig = m_Apartment.getBusInterface()->getStructureQueryBusInterface()->getZoneHeatingConfig(
                hProp.m_HeatingControlDSUID, pZone->getID());
          }

          if (_request.hasParameter("ControlMode")) {
            _request.getParameter("ControlMode", hConfig.ControllerMode);
          }
          if (_request.hasParameter("ReferenceZone")) {
            _request.getParameter("ReferenceZone", hConfig.SourceZoneId);
          }
          if (_request.hasParameter("CtrlOffset")) {
            int offset;
            _request.getParameter("CtrlOffset", offset);
            hConfig.Offset = offset;
          }
          if (_request.hasParameter("EmergencyValue")) {
            _request.getParameter("EmergencyValue", hConfig.EmergencyValue);
            hConfig.EmergencyValue += 100;
          }
          if (_request.hasParameter("ManualValue")) {
            _request.getParameter("ManualValue", hConfig.ManualValue);
            hConfig.ManualValue += 100;
          }
          if (_request.hasParameter("CtrlYmin")) {
            _request.getParameter("CtrlYmin", hConfig.Ymin);
            hConfig.Ymin += 100;
          }
          if (_request.hasParameter("CtrlYmax")) {
            _request.getParameter("CtrlYmax", hConfig.Ymax);
            hConfig.Ymax += 100;
          }
          double tempDouble = 0;
          _request.getParameter("CtrlKp", tempDouble);
          hConfig.Kp = (uint16_t)tempDouble * 40;
          _request.getParameter("CtrlTi", hConfig.Ti);
          _request.getParameter("CtrlTs", hConfig.Ts);
          _request.getParameter("CtrlKd", hConfig.Kd);
          _request.getParameter("CtrlImin", tempDouble);
          hConfig.Imin = tempDouble * 40;
          _request.getParameter("CtrlImax", tempDouble);
          hConfig.Imax = tempDouble * 40;
          bool tempBool;
          _request.getParameter("CtrlAntiWindUp", tempBool);
          hConfig.AntiWindUp = tempBool ? 1 : 0;
          _request.getParameter("CtrlKeepFloorWarm", tempBool);
          hConfig.KeepFloorWarm = tempBool ? 1 : 0;

          StructureManipulator manipulator(*m_pStructureBusInterface, *m_pStructureQueryBusInterface, m_Apartment);
          manipulator.setZoneHeatingConfig(pZone, hProp.m_HeatingControlDSUID, hConfig);
          return success(resultObj);

        } else if(_request.getMethod() == "getTemperatureControlValues") {
          if (pZone->getID() == 0) {
            return failure("Zone id 0 is invalid");
          }

          boost::shared_ptr<JSONObject> resultObj = boost::make_shared<JSONObject>();
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingOperationModeSpec_t hOpValues;

          memset(&hOpValues, 0, sizeof(hOpValues));
          if (IsNullDsuid(hProp.m_HeatingControlDSUID)) {
            return failure("Not a heating control device");
          } else {
            resultObj->addProperty("IsConfigured", true);
            hOpValues = m_Apartment.getBusInterface()->getStructureQueryBusInterface()->getZoneHeatingOperationModes(
                hProp.m_HeatingControlDSUID, pZone->getID());
          }

          switch (hProp.m_HeatingControlMode) {
          case HeatingControlModeIDOff:
            break;
          case HeatingControlModeIDPID:
            resultObj->addProperty("Off",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode0));
            resultObj->addProperty("Comfort",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode1));
            resultObj->addProperty("Economy",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode2));
            resultObj->addProperty("NotUsed",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode3));
            resultObj->addProperty("Night",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode4));
            resultObj->addProperty("Holiday",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hOpValues.OpMode5));
            break;
          case HeatingControlModeIDZoneFollower:
            break;
          case HeatingControlModeIDFixed:
            resultObj->addProperty("Off",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode0));
            resultObj->addProperty("Comfort",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode1));
            resultObj->addProperty("Economy",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode2));
            resultObj->addProperty("NotUsed",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode3));
            resultObj->addProperty("Night",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode4));
            resultObj->addProperty("Holiday",
                SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hOpValues.OpMode5));
            break;

          }
          return success(resultObj);

        } else if(_request.getMethod() == "setTemperatureControlValues") {
          if (pZone->getID() == 0) {
            return failure("Zone id 0 is invalid");
          }

          boost::shared_ptr<JSONObject> resultObj = boost::make_shared<JSONObject>();
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
            return failure("Cannot set control values in current mode");
          }

          memset(&hOpValues, 0, sizeof(hOpValues));
          if (IsNullDsuid(hProp.m_HeatingControlDSUID)) {
            return failure("Not a heating control device");
          } else {
            resultObj->addProperty("IsConfigured", true);
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

          return success(resultObj);

        } else if(_request.getMethod() == "setTemperatureControlState") {
          if (pZone->getID() == 0) {
            return failure("Zone id 0 is invalid");
          }

          boost::shared_ptr<JSONObject> resultObj = boost::make_shared<JSONObject>();
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingStateSpec_t hState;

          if (IsNullDsuid(hProp.m_HeatingControlDSUID)) {
            return failure("Not a heating control device");
          } else {
            resultObj->addProperty("IsConfigured", true);
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
              return failure("Invalid mode for the control state");
            }
          }
          hState.State = (uint8_t) value;

          m_Apartment.getBusInterface()->getStructureModifyingBusInterface()->setZoneHeatingState(
              hProp.m_HeatingControlDSUID, pZone->getID(), hState);

        } else if(_request.getMethod() == "getTemperatureControlInternals") {
          if (pZone->getID() == 0) {
            return failure("Zone id 0 is invalid");
          }

          boost::shared_ptr<JSONObject> resultObj = boost::make_shared<JSONObject>();
          ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
          ZoneHeatingInternalsSpec_t hInternals;

          if (IsNullDsuid(hProp.m_HeatingControlDSUID)) {
            return failure("Not a heating control device");
          }

          resultObj->addProperty("IsConfigured", true);
          resultObj->addProperty("ControlDSUID", dsuid2str(hProp.m_HeatingControlDSUID));
          resultObj->addProperty("ControlMode", hProp.m_HeatingControlMode);
          resultObj->addProperty("ControlState", hProp.m_HeatingControlState);

          if (hProp.m_HeatingControlMode != HeatingControlModeIDPID) {
            return failure("Not a PID controller");
          }
          if (hProp.m_HeatingControlState != HeatingControlStateIDInternal) {
            return failure("PID controller not running");
          }
          memset(&hInternals, 0, sizeof(ZoneHeatingInternalsSpec_t));
          hInternals = m_Apartment.getBusInterface()->getStructureQueryBusInterface()->getZoneHeatingInternals(
              hProp.m_HeatingControlDSUID, pZone->getID());

          resultObj->addProperty("CtrlTRecent", (double) SceneHelper::sensorToFloat12(SensorIDTemperatureIndoors, hInternals.Trecent));
          resultObj->addProperty("CtrlTReference", (double) SceneHelper::sensorToFloat12(SensorIDRoomTemperatureSetpoint, hInternals.Treference));
          resultObj->addProperty("CtrlTError", (double) hInternals.TError * 0.025);
          resultObj->addProperty("CtrlTErrorPrev", (double) hInternals.TErrorPrev * 0.025);
          resultObj->addProperty("CtrlIntegral", (double) hInternals.Integral * 0.025);
          resultObj->addProperty("CtrlYp", (double) hInternals.Yp * 0.01);
          resultObj->addProperty("CtrlYi", (double) hInternals.Yi * 0.01);
          resultObj->addProperty("CtrlYd", (double) hInternals.Yd *0.01);
          resultObj->addProperty("CtrlY", (double) SceneHelper::sensorToFloat12(SensorIDRoomTemperatureControlVariable, hInternals.Y));
          resultObj->addProperty("CtrlAntiWindUp", (unsigned long int) hInternals.AntiWindUp);

          return success(resultObj);

        } else if(_request.getMethod() == "setSensorSource") {
          if (pZone->getID() == 0) {
            return failure("Not allowed to assign sensor for zone 0");
          }
          std::string dsuidStr = _request.getParameter("dsuid");
          if (dsuidStr.empty()) {
            return failure("Missing parameter 'dsuid'");
          }
          dsuid_t dsuid = str2dsuid(dsuidStr);

          int type = strToIntDef(_request.getParameter("sensorType"), -1);
          if (type < 0) {
            return failure("Missing or invalid parameter 'sensorType'");
          }
          boost::shared_ptr<Device> dev = m_Apartment.getDeviceByDSID(dsuid);
          StructureManipulator manipulator(*m_pStructureBusInterface, *m_pStructureQueryBusInterface, m_Apartment);
          manipulator.setZoneSensor(pZone, type, dev);
          return success();
        } else if(_request.getMethod() == "clearSensorSource") {
          if (pZone->getID() == 0) {
            return success();
          }
          int type = strToIntDef(_request.getParameter("sensorType"), -1);
          if (type < 0) {
            return failure("Missing or invalid parameter 'sensorType'");
          }
          StructureManipulator manipulator(*m_pStructureBusInterface, *m_pStructureQueryBusInterface, m_Apartment);
          manipulator.resetZoneSensor(pZone, type);
          return success();
        } else if(_request.getMethod() == "getAssignedSensors") {
          boost::shared_ptr<JSONObject> resultObj = boost::make_shared<JSONObject>();
          boost::shared_ptr<JSONArrayBase> sensors = boost::make_shared<JSONArrayBase>();
          resultObj->addElement("sensors", sensors);
          std::vector<boost::shared_ptr<MainZoneSensor_t> > slist = pZone->getAssignedSensors();
          for (std::vector<boost::shared_ptr<MainZoneSensor_t> >::iterator it = slist.begin();
              it != slist.end();
              it ++) {
            boost::shared_ptr<JSONObject> sensor = boost::make_shared<JSONObject>();
            boost::shared_ptr<MainZoneSensor_t> devSensor = *it;
            sensors->addElement("", sensor);
            sensor->addProperty("sensorType", devSensor->m_sensorType);
            sensor->addProperty("dsuid", dsuid2str(devSensor->m_DSUID));
          }
          return success(resultObj);

        } else if(_request.getMethod() == "getSensorValues") {
          ZoneSensorStatus_t sensorStatus = pZone->getSensorStatus();
          boost::shared_ptr<JSONObject> resultObj = boost::make_shared<JSONObject>();
          boost::shared_ptr<JSONArrayBase> values = boost::make_shared<JSONArrayBase>();

          resultObj->addProperty("id", pZone->getID());
          resultObj->addProperty("name", pZone->getName());
          resultObj->addElement("values", values);

          if (sensorStatus.m_TemperatureValueTS != DateTime::NullDate) {
            boost::shared_ptr<JSONObject> svalue = boost::make_shared<JSONObject>();
            values->addElement("", svalue);
            svalue->addProperty("TemperatureValue", sensorStatus.m_TemperatureValue);
            svalue->addProperty("TemperatureValueTime", sensorStatus.m_TemperatureValueTS.toISO8601_ms());
          }
          if (sensorStatus.m_HumidityValueTS != DateTime::NullDate) {
            boost::shared_ptr<JSONObject> svalue = boost::make_shared<JSONObject>();
            values->addElement("", svalue);
            svalue->addProperty("HumidityValue", sensorStatus.m_HumidityValue);
            svalue->addProperty("HumidityValueTime", sensorStatus.m_HumidityValueTS.toISO8601_ms());
          }
          if (sensorStatus.m_CO2ConcentrationValueTS != DateTime::NullDate) {
            boost::shared_ptr<JSONObject> svalue = boost::make_shared<JSONObject>();
            values->addElement("", svalue);
            svalue->addProperty("CO2concentrationValue", sensorStatus.m_CO2ConcentrationValue);
            svalue->addProperty("CO2concentrationValueTime", sensorStatus.m_CO2ConcentrationValueTS.toISO8601_ms());
          }
          if (sensorStatus.m_BrightnessValueTS != DateTime::NullDate) {
            boost::shared_ptr<JSONObject> svalue = boost::make_shared<JSONObject>();
            values->addElement("", svalue);
            svalue->addProperty("BrightnessValue", sensorStatus.m_BrightnessValue);
            svalue->addProperty("BrightnessValueTime", sensorStatus.m_BrightnessValueTS.toISO8601_ms());
          }
          return success(resultObj);

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

} // namespace dss
