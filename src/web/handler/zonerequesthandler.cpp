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
#include <ds/log.h>

#include "src/foreach.h"
#include "src/model/apartment.h"
#include "src/model/device.h"
#include "src/model/group.h"
#include "src/model/modelmaintenance.h"
#include "src/model/scenehelper.h"
#include "src/model/set.h"
#include "src/model/zone.h"
#include "src/model/status.h"
#include "src/stringconverter.h"
#include "src/structuremanipulator.h"
#include "util.h"

#include <algorithm>

namespace dss {

//=========================================== ZoneRequestHandler

ZoneRequestHandler::ZoneRequestHandler(Apartment& _apartment, StructureModifyingBusInterface* _pBusInterface,
        StructureQueryBusInterface* _pQueryBusInterface)
        : m_Apartment(_apartment),
          m_pStructureBusInterface(_pBusInterface),
          m_pStructureQueryBusInterface(_pQueryBusInterface) {}

std::string ZoneRequestHandler::getLastCalledScene(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
  int lastScene = 0;
  if (pGroup != NULL) {
    lastScene = pGroup->getLastCalledScene();
  } else if (pZone != NULL) {
    lastScene = pZone->getGroup(0)->getLastCalledScene();
  } else {
    // should never reach here because ok, would be false
    assert(false);
  }
  JSONWriter json;
  json.add("scene", lastScene);
  return json.successJSON();
}

std::string ZoneRequestHandler::getName(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
  JSONWriter json;
  json.add("name", pZone->getName());
  return json.successJSON();
}

std::string ZoneRequestHandler::setName(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
  if (_request.hasParameter("newName")) {
    if (pZone->getID() == 0) {
      return JSONWriter::failure("changing name for zone 0 is not allowed");
    }
    std::string newName = _request.getParameter("newName");
    newName = escapeHTML(newName);
    pZone->setName(newName);
  } else {
    return JSONWriter::failure("missing parameter 'newName'");
  }
  return JSONWriter::success();
}

std::string ZoneRequestHandler::sceneSetName(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
  StringConverter st("UTF-8", "UTF-8");

  if (pGroup == NULL) {
    return JSONWriter::failure("Need group to work");
  }
  int sceneNumber = strToIntDef(_request.getParameter("sceneNumber"), -1);
  if (sceneNumber == -1) {
    return JSONWriter::failure("Need valid parameter 'sceneNumber'");
  }
  if (!SceneHelper::isInRange(sceneNumber, pGroup->getZoneID())) {
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
}

std::string ZoneRequestHandler::sceneGetName(boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
  if (pGroup == NULL) {
    return JSONWriter::failure("Need group to work");
  }
  int sceneNumber = strToIntDef(_request.getParameter("sceneNumber"), -1);
  if (sceneNumber == -1) {
    return JSONWriter::failure("Need valid parameter 'sceneNumber'");
  }
  if (!SceneHelper::isInRange(sceneNumber, pGroup->getZoneID())) {
    return JSONWriter::failure("Parameter 'sceneNumber' out of bounds ('" + intToString(sceneNumber) + "')");
  }

  std::string sceneName = pGroup->getSceneName(sceneNumber);
  JSONWriter json;
  json.add("name", sceneName);
  return json.successJSON();
}

std::string ZoneRequestHandler::getReachableScenes(
    boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
  if (pGroup == NULL) {
    return JSONWriter::failure("Need group to work");
  }

  // for all selected groups try to find all reachable scenes and their names inside group
  Set devicesInGroup = pGroup->getDevices();
  std::vector<int> groupReachableScenes;

  for (int iDevice = 0; iDevice < devicesInGroup.length(); iDevice++) {
    auto device = devicesInGroup.get(iDevice).getDevice();

    // check only devices with buttons that are assigned to this exact group
    // TODO(soon): if device is removed from cluster the ButtonActiveGroup is set to 255 and not to button group,
    // current workaround is that we use device activeGroup if the buttonActiveGroup is not set
    if ((device->getButtonInputCount() > 0) &&
        ((device->getButtonActiveGroup() == pGroup->getID()) ||
            ((device->getButtonActiveGroup() == 255) && (device->getActiveGroup() == pGroup->getID())))) {
      const auto& deviceReachableScenes = SceneHelper::getReachableScenes(device->getButtonID(), pGroup->getID());
      std::copy(deviceReachableScenes.begin(), deviceReachableScenes.end(), std::back_inserter(groupReachableScenes));
    }
  }

  // sort the scenes by value and remove duplicates
  std::sort(groupReachableScenes.begin(), groupReachableScenes.end());
  groupReachableScenes.resize(std::distance(
      groupReachableScenes.begin(), std::unique(groupReachableScenes.begin(), groupReachableScenes.end())));

  // filter the scenes so only available scenes in this group remain
  const auto& availableScenes = pGroup->getAvailableScenes();
  std::vector<int> filteredGroupScenes;

  std::set_intersection(groupReachableScenes.begin(), groupReachableScenes.end(), availableScenes.begin(),
      availableScenes.end(), std::back_inserter(filteredGroupScenes));

  JSONWriter json;
  json.startArray("reachableScenes");
  foreach (auto&& sceneNr, filteredGroupScenes) { json.add(sceneNr); }
  json.endArray();

  json.startArray("userSceneNames");
  // add the resulting names to the output
  foreach (auto&& sceneNr, filteredGroupScenes) {
    // get the user defined scene name
    auto sceneName = pGroup->getSceneName(sceneNr);
    if (!sceneName.empty()) {
      json.startObject();
      json.add("sceneNr", sceneNr);
      json.add("sceneName", sceneName);
      json.endObject();
    }
  }
  json.endArray();

  return json.successJSON();
}

std::string ZoneRequestHandler::getTemperatureControlStatus(
        boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
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
  json.add("ControlDSUID", hProp.m_HeatingControlDSUID);

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
}

std::string ZoneRequestHandler::getTemperatureControlConfig(
        boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
  if (pZone->getID() == 0) {
    return JSONWriter::failure("Zone id 0 is invalid");
  }

  JSONWriter json;
  ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();

  if (hProp.m_HeatingControlDSUID == DSUID_NULL) {
    return JSONWriter::failure("Not a heating control device");
  } else {
    json.add("IsConfigured", true);
  }

  json.add("ControlDSUID", hProp.m_HeatingControlDSUID);
  json.add("ControlMode", hProp.m_HeatingControlMode);
  json.add("EmergencyValue", hProp.m_EmergencyValue - 100);
  switch (hProp.m_HeatingControlMode) {
    case HeatingControlModeIDOff:
      break;
    case HeatingControlModeIDPID:
      json.add("CtrlKp", (double)hProp.m_Kp * 0.025);
      json.add("CtrlTs", hProp.m_Ts);
      json.add("CtrlTi", hProp.m_Ti);
      json.add("CtrlKd", hProp.m_Kd);
      json.add("CtrlImin", (double)hProp.m_Imin * 0.025);
      json.add("CtrlImax", (double)hProp.m_Imax * 0.025);
      json.add("CtrlYmin", hProp.m_Ymin - 100);
      json.add("CtrlYmax", hProp.m_Ymax - 100);
      json.add("CtrlAntiWindUp", (hProp.m_AntiWindUp > 0));
      json.add("CtrlKeepFloorWarm", (hProp.m_KeepFloorWarm > 0));
      break;
    case HeatingControlModeIDZoneFollower:
      json.add("ReferenceZone", hProp.m_HeatingMasterZone);
      json.add("CtrlOffset", hProp.m_CtrlOffset);
      break;
    case HeatingControlModeIDManual:
      json.add("ManualValue", hProp.m_ManualValue - 100);
      break;
    case HeatingControlModeIDFixed:
      break;
  }
  return json.successJSON();
}

std::string ZoneRequestHandler::setTemperatureControlConfig(
        boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
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
    hConfig = pZone->getHeatingControlMode();
  }

  if (_request.hasParameter("ControlMode")) {
    (void)_request.getParameter("ControlMode", hConfig.ControllerMode);
  }
  if (_request.hasParameter("ReferenceZone")) {
    (void)_request.getParameter("ReferenceZone", hConfig.SourceZoneId);
  }
  if (_request.hasParameter("CtrlOffset")) {
    int offset;
    (void)_request.getParameter("CtrlOffset", offset);
    hConfig.Offset = offset;
  }
  if (_request.hasParameter("EmergencyValue")) {
    (void)_request.getParameter("EmergencyValue", hConfig.EmergencyValue);
    hConfig.EmergencyValue += 100;
  }
  if (_request.hasParameter("ManualValue")) {
    (void)_request.getParameter("ManualValue", hConfig.ManualValue);
    hConfig.ManualValue += 100;
  }
  if (_request.hasParameter("CtrlYmin")) {
    (void)_request.getParameter("CtrlYmin", hConfig.Ymin);
    hConfig.Ymin += 100;
  }
  if (_request.hasParameter("CtrlYmax")) {
    (void)_request.getParameter("CtrlYmax", hConfig.Ymax);
    hConfig.Ymax += 100;
  }
  double tempDouble = 0;
  if (_request.getParameter("CtrlKp", tempDouble)) {
    hConfig.Kp = (uint16_t)tempDouble * 40;
  }
  (void)_request.getParameter("CtrlTi", hConfig.Ti);
  (void)_request.getParameter("CtrlTs", hConfig.Ts);
  (void)_request.getParameter("CtrlKd", hConfig.Kd);
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
}

std::string ZoneRequestHandler::getTemperatureControlValues(
        boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
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
      json.add("Off", sensorValueToDouble(SensorType::RoomTemperatureSetpoint, hOpValues.OpMode0));
      json.add("Comfort", sensorValueToDouble(SensorType::RoomTemperatureSetpoint, hOpValues.OpMode1));
      json.add("Economy", sensorValueToDouble(SensorType::RoomTemperatureSetpoint, hOpValues.OpMode2));
      json.add("NotUsed", sensorValueToDouble(SensorType::RoomTemperatureSetpoint, hOpValues.OpMode3));
      json.add("Night", sensorValueToDouble(SensorType::RoomTemperatureSetpoint, hOpValues.OpMode4));
      json.add("Holiday", sensorValueToDouble(SensorType::RoomTemperatureSetpoint, hOpValues.OpMode5));
      json.add("Cooling", sensorValueToDouble(SensorType::RoomTemperatureSetpoint, hOpValues.OpMode6));
      json.add("CoolingOff", sensorValueToDouble(SensorType::RoomTemperatureSetpoint, hOpValues.OpMode7));
      break;
    case HeatingControlModeIDZoneFollower:
      break;
    case HeatingControlModeIDFixed:
      json.add("Off", sensorValueToDouble(SensorType::RoomTemperatureControlVariable, hOpValues.OpMode0));
      json.add("Comfort", sensorValueToDouble(SensorType::RoomTemperatureControlVariable, hOpValues.OpMode1));
      json.add("Economy", sensorValueToDouble(SensorType::RoomTemperatureControlVariable, hOpValues.OpMode2));
      json.add("NotUsed", sensorValueToDouble(SensorType::RoomTemperatureControlVariable, hOpValues.OpMode3));
      json.add("Night", sensorValueToDouble(SensorType::RoomTemperatureControlVariable, hOpValues.OpMode4));
      json.add("Holiday", sensorValueToDouble(SensorType::RoomTemperatureControlVariable, hOpValues.OpMode5));
      json.add("Cooling", sensorValueToDouble(SensorType::RoomTemperatureControlVariable, hOpValues.OpMode6));
      json.add("CoolingOff", sensorValueToDouble(SensorType::RoomTemperatureControlVariable, hOpValues.OpMode7));
      break;
  }
  return json.successJSON();
}

std::string ZoneRequestHandler::setTemperatureControlValues(
        boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
  if (pZone->getID() == 0) {
    return JSONWriter::failure("Zone id 0 is invalid");
  }

  JSONWriter json;
  ZoneHeatingProperties_t hProp = pZone->getHeatingProperties();
  ZoneHeatingOperationModeSpec_t hOpValues;
  SensorType SensorConversion;
  int iValue;
  double fValue;

  if (hProp.m_HeatingControlMode == HeatingControlModeIDPID) {
    SensorConversion = SensorType::RoomTemperatureSetpoint;
  } else if (hProp.m_HeatingControlMode == HeatingControlModeIDFixed) {
    SensorConversion = SensorType::RoomTemperatureControlVariable;
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
      hOpValues.OpMode0 = doubleToSensorValue(SensorConversion, iValue);
    } catch (std::invalid_argument& e) {
      try {
        fValue = strToDouble(_request.getParameter("Off"));
        hOpValues.OpMode0 = doubleToSensorValue(SensorConversion, fValue);
      } catch (std::invalid_argument& e) {
      }
    }
  }
  if (_request.hasParameter("Comfort")) {
    try {
      iValue = strToUInt(_request.getParameter("Comfort"));
      hOpValues.OpMode1 = doubleToSensorValue(SensorConversion, iValue);
    } catch (std::invalid_argument& e) {
      try {
        fValue = strToDouble(_request.getParameter("Comfort"));
        hOpValues.OpMode1 = doubleToSensorValue(SensorConversion, fValue);
      } catch (std::invalid_argument& e) {
      }
    }
  }
  if (_request.hasParameter("Economy")) {
    try {
      iValue = strToUInt(_request.getParameter("Economy"));
      hOpValues.OpMode2 = doubleToSensorValue(SensorConversion, iValue);
    } catch (std::invalid_argument& e) {
      try {
        fValue = strToDouble(_request.getParameter("Economy"));
        hOpValues.OpMode2 = doubleToSensorValue(SensorConversion, fValue);
      } catch (std::invalid_argument& e) {
      }
    }
  }
  if (_request.hasParameter("NotUsed")) {
    try {
      iValue = strToUInt(_request.getParameter("NotUsed"));
      hOpValues.OpMode3 = doubleToSensorValue(SensorConversion, iValue);
    } catch (std::invalid_argument& e) {
      try {
        fValue = strToDouble(_request.getParameter("NotUsed"));
        hOpValues.OpMode3 = doubleToSensorValue(SensorConversion, fValue);
      } catch (std::invalid_argument& e) {
      }
    }
  }
  if (_request.hasParameter("Night")) {
    try {
      iValue = strToUInt(_request.getParameter("Night"));
      hOpValues.OpMode4 = doubleToSensorValue(SensorConversion, iValue);
    } catch (std::invalid_argument& e) {
      try {
        fValue = strToDouble(_request.getParameter("Night"));
        hOpValues.OpMode4 = doubleToSensorValue(SensorConversion, fValue);
      } catch (std::invalid_argument& e) {
      }
    }
  }
  if (_request.hasParameter("Holiday")) {
    try {
      iValue = strToUInt(_request.getParameter("Holiday"));
      hOpValues.OpMode5 = doubleToSensorValue(SensorConversion, iValue);
    } catch (std::invalid_argument& e) {
      try {
        fValue = strToDouble(_request.getParameter("Holiday"));
        hOpValues.OpMode5 = doubleToSensorValue(SensorConversion, fValue);
      } catch (std::invalid_argument& e) {
      }
    }
  }
  if (_request.hasParameter("Cooling")) {
    try {
      iValue = strToUInt(_request.getParameter("Cooling"));
      hOpValues.OpMode6 = doubleToSensorValue(SensorConversion, iValue);
    } catch (std::invalid_argument& e) {
      try {
        fValue = strToDouble(_request.getParameter("Cooling"));
        hOpValues.OpMode6 = doubleToSensorValue(SensorConversion, fValue);
      } catch (std::invalid_argument& e) {
      }
    }
  }
  if (_request.hasParameter("CoolingOff")) {
    try {
      iValue = strToUInt(_request.getParameter("CoolingOff"));
      hOpValues.OpMode7 = doubleToSensorValue(SensorConversion, iValue);
    } catch (std::invalid_argument& e) {
      try {
        fValue = strToDouble(_request.getParameter("CoolingOff"));
        hOpValues.OpMode7 = doubleToSensorValue(SensorConversion, fValue);
      } catch (std::invalid_argument& e) {
      }
    }
  }

  m_Apartment.getBusInterface()->getStructureModifyingBusInterface()->setZoneHeatingOperationModes(
          hProp.m_HeatingControlDSUID, pZone->getID(), hOpValues);

  return json.successJSON();
}

std::string ZoneRequestHandler::setTemperatureControlState(
        boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
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
    } catch (std::invalid_argument& e) {
      return JSONWriter::failure("Invalid mode for the control state");
    }
  }
  hState.State = (uint8_t)value;

  m_Apartment.getBusInterface()->getStructureModifyingBusInterface()->setZoneHeatingState(
          hProp.m_HeatingControlDSUID, pZone->getID(), hState);

  return JSONWriter::success();
}

std::string ZoneRequestHandler::getTemperatureControlInternals(
        boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
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
  json.add("ControlDSUID", hProp.m_HeatingControlDSUID);
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

  json.add("CtrlTRecent", sensorValueToDouble(SensorType::TemperatureIndoors, hInternals.Trecent));
  json.add("CtrlTReference", sensorValueToDouble(SensorType::RoomTemperatureSetpoint, hInternals.Treference));
  json.add("CtrlTError", (double)hInternals.TError * 0.025);
  json.add("CtrlTErrorPrev", (double)hInternals.TErrorPrev * 0.025);
  json.add("CtrlIntegral", (double)hInternals.Integral * 0.025);
  json.add("CtrlYp", (double)hInternals.Yp * 0.01);
  json.add("CtrlYi", (double)hInternals.Yi * 0.01);
  json.add("CtrlYd", (double)hInternals.Yd * 0.01);
  json.add("CtrlY", sensorValueToDouble(SensorType::RoomTemperatureControlVariable, hInternals.Y));
  json.add("CtrlAntiWindUp", hInternals.AntiWindUp);

  return json.successJSON();
}

std::string ZoneRequestHandler::setStatusField(
        boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& request) {
  DS_REQUIRE(pGroup != 0, "Missing required groupID parameter.");
  pGroup->setStatusField(request.getRequiredParameter("field"), request.getRequiredParameter("value"));
  return JSONWriter::success();
}


std::string ZoneRequestHandler::setSensorSource(
        boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
  if (pZone->getID() == 0) {
    return JSONWriter::failure("Not allowed to assign sensor for zone 0");
  }
  std::string dsuidStr = _request.getParameter("dsuid");
  if (dsuidStr.empty()) {
    return JSONWriter::failure("Missing parameter 'dsuid'");
  }
  dsuid_t dsuid = str2dsuid(dsuidStr);

  int intType = strToIntDef(_request.getParameter("sensorType"), -1);
  if (intType < 0) {
    return JSONWriter::failure("Missing or invalid parameter 'sensorType'");
  }
  boost::shared_ptr<Device> dev = m_Apartment.getDeviceByDSID(dsuid);

  SensorType type = static_cast<SensorType>(intType);

  switch (type) {
    case SensorType::BrightnessOutdoors:
    case SensorType::TemperatureOutdoors:
    case SensorType::HumidityOutdoors:
    case SensorType::WindSpeed:
    case SensorType::WindDirection:
    case SensorType::GustSpeed:
    case SensorType::GustDirection:
    case SensorType::Precipitation:
    case SensorType::AirPressure:
      pZone = m_Apartment.getZone(0);
      break;
    case SensorType::ActivePower:
    case SensorType::OutputCurrent:
    case SensorType::ElectricMeter:
    case SensorType::OutputCurrent16A:
    case SensorType::ActivePowerVA:
    case SensorType::TemperatureIndoors:
    case SensorType::BrightnessIndoors:
    case SensorType::HumidityIndoors:
    case SensorType::CO2Concentration:
    case SensorType::COConcentration:
    case SensorType::SoundPressureLevel:
    case SensorType::RoomTemperatureSetpoint:
    case SensorType::RoomTemperatureControlVariable:
      // all valid sensor types do not need any processing
      break;
    case SensorType::Reserved1:
    case SensorType::Reserved2:
    case SensorType::NotUsed:
    case SensorType::UnknownType:
    default:
      return JSONWriter::failure("Invalid parameter 'sensorType': " + intToString(static_cast<int>(type)));
      break;
  }

  StructureManipulator manipulator(*m_pStructureBusInterface, *m_pStructureQueryBusInterface, m_Apartment);
  manipulator.setZoneSensor(*pZone, type, dev);
  return JSONWriter::success();
}

std::string ZoneRequestHandler::clearSensorSource(
        boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
  if (pZone->getID() == 0) {
    return JSONWriter::success();
  }
  int intType = strToIntDef(_request.getParameter("sensorType"), -1);
  if (intType < 0) {
    return JSONWriter::failure("Missing or invalid parameter 'sensorType'");
  }
  SensorType sensorType = static_cast<SensorType>(intType);
  StructureManipulator manipulator(*m_pStructureBusInterface, *m_pStructureQueryBusInterface, m_Apartment);
  manipulator.resetZoneSensor(*pZone, sensorType);
  return JSONWriter::success();
}

std::string ZoneRequestHandler::getAssignedSensors(
        boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
  JSONWriter json;
  json.startArray("sensors");
  foreach (auto&& devSensor, pZone->getAssignedSensors()) {
    json.startObject();
    json.add("sensorType", devSensor.m_sensorType);
    json.add("dsuid", devSensor.m_DSUID);
    json.endObject();
  }
  json.endArray();
  return json.successJSON();
}

std::string ZoneRequestHandler::getSensorValues(
        boost::shared_ptr<Zone> pZone, boost::shared_ptr<Group> pGroup, const RestfulRequest& _request) {
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
}

WebServerResponse ZoneRequestHandler::jsonHandleRequest(
        const RestfulRequest& _request, boost::shared_ptr<Session> _session, const struct mg_connection* _connection) {
  StringConverter st("UTF-8", "UTF-8");
  bool ok = true;
  std::string errorMessage;
  std::string zoneName = _request.getParameter("name");
  std::string zoneIDString = _request.getParameter("id");
  boost::shared_ptr<Zone> pZone;
  if (!zoneIDString.empty()) {
    int zoneID = strToIntDef(zoneIDString, -1);
    if (zoneID != -1) {
      try {
        pZone = m_Apartment.getZone(zoneID);
      } catch (std::runtime_error& e) {
        ok = false;
        errorMessage = "Could not find zone with id '" + zoneIDString + "'";
      }
    } else {
      ok = false;
      errorMessage = "Could not parse id '" + zoneIDString + "'";
    }
  } else if (!zoneName.empty()) {
    try {
      pZone = m_Apartment.getZone(zoneName);
    } catch (std::runtime_error& e) {
      ok = false;
      errorMessage = "Could not find zone named '" + zoneName + "'";
    }
  } else {
    ok = false;
    errorMessage = "Need parameter name or id to identify zone";
  }
  if (ok) {
    boost::shared_ptr<Group> pGroup;
    std::string groupName = _request.getParameter("groupName");
    std::string groupIDString = _request.getParameter("groupID");
    if (!groupName.empty()) {
      try {
        pGroup = pZone->getGroup(groupName);
        if (pGroup == NULL) {
          // TODO: this might better be done by the zone
          throw std::runtime_error("dummy");
        }
      } catch (std::runtime_error& e) {
        errorMessage = "Could not find group with name '" + groupName + "'";
        ok = false;
      }
    } else if (!groupIDString.empty()) {
      try {
        int groupID = strToIntDef(groupIDString, -1);
        if (groupID != -1) {
          pGroup = pZone->getGroup(groupID);
          if (pGroup == NULL) {
            // TODO: this might better be done by the zone
            throw std::runtime_error("dummy");
          }
        } else {
          errorMessage = "Could not parse group id '" + groupIDString + "'";
          ok = false;
        }
      } catch (std::runtime_error& e) {
        errorMessage = "Could not find group with ID '" + groupIDString + "'";
        ok = false;
      }
    }

    if (ok) {
      if (isDeviceInterfaceCall(_request)) {
        boost::shared_ptr<IDeviceInterface> interface;
        if (pGroup != NULL) {
          interface = pGroup;
        }
        if (ok) {
          if (interface == NULL) {
            interface = pZone;
          }
          return handleDeviceInterfaceRequest(_request, interface, _session);
        }
      } else if (_request.getMethod() == "getLastCalledScene") {
        return getLastCalledScene(pZone, pGroup, _request);
      } else if (_request.getMethod() == "getName") {
        return getName(pZone, pGroup, _request);
      } else if (_request.getMethod() == "setName") {
        return setName(pZone, pGroup, _request);
      } else if (_request.getMethod() == "sceneSetName") {
        return sceneSetName(pZone, pGroup, _request);
      } else if (_request.getMethod() == "sceneGetName") {
        return sceneGetName(pZone, pGroup, _request);
      } else if (_request.getMethod() == "getReachableScenes") {
        return getReachableScenes(pZone, pGroup, _request);
      } else if (_request.getMethod() == "getTemperatureControlStatus") {
        return getTemperatureControlStatus(pZone, pGroup, _request);
      } else if (_request.getMethod() == "getTemperatureControlConfig") {
        return getTemperatureControlConfig(pZone, pGroup, _request);
      } else if (_request.getMethod() == "setTemperatureControlConfig") {
        return setTemperatureControlConfig(pZone, pGroup, _request);
      } else if (_request.getMethod() == "getTemperatureControlValues") {
        return getTemperatureControlValues(pZone, pGroup, _request);
      } else if (_request.getMethod() == "setTemperatureControlValues") {
        return setTemperatureControlValues(pZone, pGroup, _request);
      } else if (_request.getMethod() == "setTemperatureControlState") {
        return setTemperatureControlState(pZone, pGroup, _request);
      } else if (_request.getMethod() == "getTemperatureControlInternals") {
        return getTemperatureControlInternals(pZone, pGroup, _request);
      } else if (_request.getMethod() == "setStatusField") {
        return setStatusField(pZone, pGroup, _request);
      } else if (_request.getMethod() == "setSensorSource") {
        return setSensorSource(pZone, pGroup, _request);
      } else if (_request.getMethod() == "clearSensorSource") {
        return clearSensorSource(pZone, pGroup, _request);
      } else if (_request.getMethod() == "getAssignedSensors") {
        return getAssignedSensors(pZone, pGroup, _request);
      } else if (_request.getMethod() == "getSensorValues") {
        return getSensorValues(pZone, pGroup, _request);
      } else {
        throw std::runtime_error("Unhandled function");
      }
    }
  }
  if (!ok) {
    return JSONWriter::failure(errorMessage);
  } else {
    return JSONWriter::success(); // TODO: check this, we shouldn't get here
  }
} // handleRequest

} // namespace dss
