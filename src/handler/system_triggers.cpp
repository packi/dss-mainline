/*
 *  Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 */

#include "system_triggers.h"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "event/event_create.h"
#include "event/event_fields.h"
#include "model/group.h"
#include "security/security.h"
#include "systemcondition.h"

namespace dss {
  const std::string ptn_triggers = "triggers";
  const std::string  ptn_type = "type";

  const std::string ptn_damping = "damping";
  const std::string  ptn_damp_interval = "interval";
  const std::string  ptn_damp_rewind = "rewindTimer";
  const std::string  ptn_damp_start_ts = "lastTrigger";

  const std::string ptn_action_lag = "lag";
  const std::string  ptn_action_delay = "delay";
  const std::string  ptn_action_reschedule = "reschedule";
  const std::string  ptn_action_ts = "last_event_scheduled_at";
  const std::string  ptn_action_eventid = "eventid";

  SystemTrigger::SystemTrigger(const Event &event)
    : SystemEvent(event),
      m_evtSrcIsGroup(false),
      m_evtSrcIsDevice(false),
      m_evtSrcZone(0),
      m_evtSrcGroup(0)
  {
    EventRaiseLocation raiseLocation = event.getRaiseLocation();
    if ((raiseLocation == erlGroup) || (raiseLocation == erlApartment)) {
      auto group = event.getRaisedAtGroup(DSS::getInstance()->getApartment());
      m_evtSrcIsGroup = (raiseLocation == erlGroup);
      m_evtSrcIsDevice = false;
      m_evtSrcZone = group->getZoneID();
      m_evtSrcGroup = group->getID();
    } else if (raiseLocation == erlDevice) {
      boost::shared_ptr<const DeviceReference> device =
          event.getRaisedAtDevice();
      m_evtSrcIsGroup = false;
      m_evtSrcIsDevice = true;
      m_evtSrcZone = device->getDevice()->getZoneID();
      m_evtSrcDSID = device->getDSID();
    } else if (raiseLocation == erlState) {
      m_evtSrcIsGroup = false;
      m_evtSrcIsDevice = false;
    }
  }

  SystemTrigger::~SystemTrigger() = default;

  bool SystemTrigger::checkSceneZone(PropertyNodePtr _triggerProp) {
    if (!((m_evtName == EventName::CallScene) ||
          (m_evtName == EventName::CallSceneBus))) {
      return false;
    }

    if (!m_evtSrcIsGroup) {
      return false;
    }

    int scene = strToIntDef(m_properties.get(ef_sceneID), -1);
    dsuid_t originDSUID;
    if (m_properties.has(ef_originDSUID)) {
      originDSUID = str2dsuid(m_properties.get(ef_originDSUID));
    }
    bool forced = false;
    if (m_properties.has(ef_forced)) {
      std::string sForced = m_properties.get(ef_forced);
      forced = (sForced == "true");
    }

    int iZone;
    try {
      PropertyNodePtr triggerZone = _triggerProp->getPropertyByName(ef_zone);
      if (triggerZone) {
        iZone = triggerZone->getIntegerValue();
      } else {
        return false;
      }
      if (iZone >= 0 && iZone != m_evtSrcZone) {
        return false;
      }
    } catch (PropertyTypeMismatch& e){
      return false;
    }

    int iGroup;
    try {
      PropertyNodePtr triggerGroup = _triggerProp->getPropertyByName(ef_group);
      if (triggerGroup) {
        iGroup = triggerGroup->getIntegerValue();
      } else {
        return false;
      }
      if (iGroup >= 0 && iGroup != m_evtSrcGroup) {
        return false;
      }
    } catch (PropertyTypeMismatch& e){
      return false;
    }

    int iScene;
    try {
      PropertyNodePtr triggerScene = _triggerProp->getPropertyByName(ef_scene);
      if (triggerScene) {
        iScene = triggerScene->getIntegerValue();
      } else {
        return false;
      }
      if (iScene != scene) {
          return false;
      }
    } catch (PropertyTypeMismatch& e){
      return false;
    }

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName(ef_dsuid);
    std::string iDevice;
    if (triggerDSID != NULL) {
      iDevice = triggerDSID->getAsString();
      if (!iDevice.empty() && (iDevice != "-1") &&
         (iDevice != dsuid2str(originDSUID))) {
        return false;
      }
    }

    PropertyNodePtr forcedFlag = _triggerProp->getPropertyByName(ef_forced);
    if (forcedFlag) {
      try {
        bool iForced = forcedFlag->getBoolValue();
        if (forced == true && iForced == false) {
          return false;
        }
        if (forced == false && iForced == true) {
          return false;
        }
      } catch (PropertyTypeMismatch& e){
        return false;
      }
    }

    std::string bus = ((m_evtName == EventName::CallSceneBus) ? "Bus" : "");
    Logger::getInstance()->log("SystemTrigger::"
            "checkSceneZone: *** Match: CallScene" +
            bus +
            " Zone: " + intToString(iZone) + ", Group: " + intToString(iGroup) + ", Scene: " +
            intToString(iScene) + ", Origin: " + iDevice);

    return true;
  }

  bool SystemTrigger::checkUndoSceneZone(PropertyNodePtr _triggerProp) {
    if (m_evtName != EventName::UndoScene) {
      return false;
    }

    if (!m_evtSrcIsGroup) {
      return false;
    }

    int scene = strToIntDef(m_properties.get(ef_sceneID), -1);
    dsuid_t originDSUID;
    if (m_properties.has(ef_originDSUID)) {
      originDSUID = str2dsuid(m_properties.get(ef_originDSUID));
    }

    PropertyNodePtr triggerZone = _triggerProp->getPropertyByName(ef_zone);
    if (triggerZone == NULL) {
      return false;
    }

    int iZone = -1;
    try {
      PropertyNodePtr triggerZone = _triggerProp->getPropertyByName(ef_zone);
      if (triggerZone) {
        iZone = triggerZone->getIntegerValue();
      }
      if (iZone >= 0 && iZone != m_evtSrcZone) {
        return false;
      }
    } catch (PropertyTypeMismatch& e){
      return false;
    }

    int iGroup = -1;
    try {
      PropertyNodePtr triggerGroup = _triggerProp->getPropertyByName(ef_group);
      if (triggerGroup) {
        iGroup = triggerGroup->getIntegerValue();
      }
      if (iGroup >= 0 && iGroup != m_evtSrcGroup) {
        return false;
      }
    } catch (PropertyTypeMismatch& e){
      return false;
    }

    int iScene = -1;
    try {
      PropertyNodePtr triggerScene = _triggerProp->getPropertyByName(ef_scene);
      if (triggerScene) {
        iScene = triggerScene->getIntegerValue();
      }
      if (iScene >= 0 && iScene != scene) {
          return false;
      }
    } catch (PropertyTypeMismatch& e){
      return false;
    }

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName(ef_dsuid);
    std::string iDevice;
    if (triggerDSID != NULL) {
      iDevice = triggerDSID->getAsString();
      if (!iDevice.empty() && (iDevice != "-1") &&
         (iDevice != dsuid2str(originDSUID))) {
        return false;
      }
    }

    Logger::getInstance()->log("SystemTrigger::"
            "checkUndoSceneZone: *** Match: UndoScene Zone: " + intToString(iZone) +
            ", Group: " + intToString(iGroup) + ", Scene: " +
            intToString(iScene) + ", Origin: " + iDevice);

    return true;
  }

  bool SystemTrigger::checkDeviceScene(PropertyNodePtr _triggerProp) {
    if (m_evtName != EventName::CallScene) {
      return false;
    }

    if (!m_evtSrcIsDevice) {
      return false;
    }

    dsuid_t dsuid = m_evtSrcDSID;
    int scene = strToIntDef(m_properties.get(ef_sceneID), -1);

    if (dsuid == DSUID_NULL) {
      return false;
    }

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName(ef_dsuid);
    if (triggerDSID == NULL) {
      return false;
    }

    PropertyNodePtr triggerSCENE = _triggerProp->getPropertyByName(ef_scene);
    if (triggerSCENE == NULL) {
      return false;
    }

    std::string sDSID = triggerDSID->getStringValue();

    int iScene;
    try {
      iScene = triggerSCENE->getIntegerValue();
      if (iScene != scene) {
          return false;
      }
    } catch (PropertyTypeMismatch& e){
      return false;
    }
    if ((sDSID == "-1") || (sDSID == dsuid2str(dsuid))) {
      if ((iScene == scene) || (iScene == -1)) {
        Logger::getInstance()->log("SystemTrigger::"
                "checkDeviceScene: Match: DeviceScene dSID:" + sDSID +
                ", Scene: " + intToString(iScene));
        return true;
      }
    }

    return false;
  }

  bool SystemTrigger::checkDeviceSensor(PropertyNodePtr _triggerProp) {
    if (m_evtName != "deviceSensorEvent") {
      return false;
    }

    dsuid_t dsuid = m_evtSrcDSID;
    std::string eventName = m_properties.get(ef_sensorEvent);
    std::string eventIndex = m_properties.get(ef_sensorIndex);

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName(ef_dsuid);
    if (triggerDSID == NULL) {
      return false;
    }

    std::string sDSID = triggerDSID->getStringValue();
    if (!((sDSID == "-1") || (sDSID == dsuid2str(dsuid)))) {
      return false;
    }

    PropertyNodePtr triggerEventId = _triggerProp->getPropertyByName(ef_eventid);
    PropertyNodePtr triggerName = _triggerProp->getPropertyByName(ef_evt);

    if (triggerEventId == NULL) {
      if (triggerName == NULL) {
        return false;
      }
      std::string iName = triggerName->getStringValue();
      if ((iName == eventName) || (iName == "-1")) {
        Logger::getInstance()->log("SystemTrigger::"
                "checkDeviceSensor:: Match: SensorEvent dSID: " + sDSID +
                " EventName: " + iName);
        return true;
      }
      return false;
    }

    std::string iEventId = triggerEventId->getAsString();
    if ((iEventId == eventIndex) || (iEventId == "-1")) {
      Logger::getInstance()->log("SystemTrigger::"
              "checkDeviceSensor:: Match: SensorEvent dSID: " + sDSID +
              " EventId: " + iEventId);
      return true;
    }

    return false;
  }

  bool SystemTrigger::checkDeviceBinaryInput(PropertyNodePtr _triggerProp) {
    if (m_evtName != EventName::DeviceBinaryInputEvent) {
      return false;
    }

    dsuid_t dsuid = m_evtSrcDSID;
    std::string eventIndex = m_properties.get("inputIndex");
    std::string eventType = m_properties.get("inputType");
    std::string eventState = m_properties.get("inputState");

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName("dsuid");
    PropertyNodePtr triggerIndex = _triggerProp->getPropertyByName("index");
    PropertyNodePtr triggerType = _triggerProp->getPropertyByName("stype");
    PropertyNodePtr triggerState = _triggerProp->getPropertyByName("state");

    // need either: dsuid + index + state or stype + state (+ dsuid)

    if ((triggerDSID == NULL) && (triggerType == NULL)) {
      return false;
    }
    if (triggerState == NULL) {
      return false;
    }

    std::string sDSID;
    std::string sIndex;
    std::string sType;
    std::string sState = triggerState->getAsString();

    if (triggerDSID) {
      sDSID = triggerDSID->getStringValue();
      if (!((sDSID == "-1") || (sDSID == dsuid2str(dsuid)))) {
        return false;
      }
      if (triggerIndex) {
        if (triggerType && (triggerType->getAsString() != eventType)) {
          return false;
        }
        sIndex = triggerIndex->getAsString();
        if (sIndex == eventIndex && sState == eventState) {
          Logger::getInstance()->log("SystemTrigger::"
              "checkDeviceBinaryInput:: Match: BinaryInput dSID: " + sDSID +
              " Index: " + sIndex + ", State: " + sState);
          return true;
        }
      }
    }

    if (triggerType) {
      sType = triggerType->getAsString();
      if (sType == eventType && sState == eventState) {
        Logger::getInstance()->log("SystemTrigger::"
            "checkDeviceBinaryInput:: Match: BinaryInput dSID: " + sDSID +
            " Type: " + sType + ", State: " + sState);
        return true;
      }
    }

    return false;
  }

  bool SystemTrigger::checkDevice(PropertyNodePtr _triggerProp) {
    if (m_evtName != EventName::DeviceButtonClick) {
      return false;
    }

    dsuid_t dsuid = m_evtSrcDSID;
    std::string clickType = m_properties.get("clickType");
    std::string buttonIndex = m_properties.get("buttonIndex");

    if (dsuid == DSUID_NULL) {
      return false;
    }

    if (clickType.empty() || (clickType == "-1")) {
      return false;
    }

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName("dsuid");
    if (triggerDSID == NULL) {
      return false;
    }

    PropertyNodePtr triggerMSG =  _triggerProp->getPropertyByName("msg");
    if (triggerMSG == NULL) {
      return false;
    }

    std::string iButtonIndexID = "-1";

    PropertyNodePtr triggerIndex = _triggerProp->getPropertyByName("buttonIndex");
    if (triggerIndex != NULL) {
      iButtonIndexID = triggerIndex->getAsString();
    }

    std::string sDSID = triggerDSID->getAsString();
    std::string iMSG = triggerMSG->getAsString();
    if ((sDSID == "-1") || (sDSID == dsuid2str(dsuid))) {
      if ((iMSG == clickType) || (iMSG == "-1")) {
        if ((buttonIndex == iButtonIndexID) || (iButtonIndexID == "-1")) {
          Logger::getInstance()->log("SystemTrigger::"
                  "checkDevice:: Match: ButtonClick dSID: " + sDSID +
                  ", Klick: " + iMSG + ", ButtonIndex: " + iButtonIndexID);
          return true;
        }
      }
    }

    return false;
  }

  bool SystemTrigger::checkDirectDeviceAction(PropertyNodePtr _triggerProp) {
    if (m_evtName != EventName::ButtonDeviceAction) {
      return false;
    }

    dsuid_t dsuid = m_evtSrcDSID;
    std::string action = m_properties.get("actionID");

    if (dsuid == DSUID_NULL) {
      return false;
    }

    if (action.empty() || (action == "-1")) {
      return false;
    }

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName("dsuid");
    if (triggerDSID == NULL) {
      return false;
    }

    PropertyNodePtr actionNode =  _triggerProp->getPropertyByName("action");
    if (actionNode == NULL) {
      return false;
    }
    std::string sDSID = triggerDSID->getAsString();
    std::string sAction = actionNode->getAsString();
    if ((sDSID == "-1") || (sDSID == dsuid2str(dsuid))) {
      if ((sAction == action) || (sAction == "-1")) {
        Logger::getInstance()->log("SystemTrigger::"
                "checkDevice:: Match: ButtonClick dSID: " + sDSID +
                ", action: " + sAction);
          return true;
      }
    }

    return false;
  }

  bool SystemTrigger::checkDeviceNamedAction(PropertyNodePtr _triggerProp) {
    if (m_evtName != EventName::DeviceActionEvent) {
      return false;
    }

    dsuid_t dsuid = m_evtSrcDSID;
    std::string action = m_properties.get("actionId");

    if (dsuid == DSUID_NULL) {
      return false;
    }

    if (action.empty() || (action == "-1")) {
      return false;
    }

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName("dsuid");
    if (triggerDSID == NULL) {
      return false;
    }

    PropertyNodePtr idNode =  _triggerProp->getPropertyByName("id");
    if (idNode == NULL) {
      return false;
    }
    std::string sDSID = triggerDSID->getAsString();
    std::string sAction = idNode->getAsString();
    if ((sDSID == "-1") || (sDSID == dsuid2str(dsuid))) {
      if ((sAction == action) || (sAction == "-1")) {
        Logger::getInstance()->log("SystemTrigger::"
                "checkDeviceNamedAction:: Match: dSID: " + sDSID +
                ", id: " + sAction);
          return true;
      }
    }

    return false;
  }

  bool SystemTrigger::checkDeviceNamedEvent(PropertyNodePtr _triggerProp) {
    if (m_evtName != EventName::DeviceEventEvent) {
      return false;
    }

    dsuid_t dsuid = m_evtSrcDSID;
    std::string evtName = m_properties.get("eventId");

    if (dsuid == DSUID_NULL) {
      return false;
    }

    if (evtName.empty() || (evtName == "-1")) {
      return false;
    }

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName("dsuid");
    if (triggerDSID == NULL) {
      return false;
    }

    PropertyNodePtr idNode =  _triggerProp->getPropertyByName("id");
    if (idNode == NULL) {
      return false;
    }
    std::string sDSID = triggerDSID->getAsString();
    std::string sEvent = idNode->getAsString();
    if ((sDSID == "-1") || (sDSID == dsuid2str(dsuid))) {
      if ((sEvent == evtName) || (sEvent == "-1")) {
        Logger::getInstance()->log("SystemTrigger::"
                "checkDeviceNamedEvent:: Match: dSID: " + sDSID +
                ", id: " + sEvent);
          return true;
      }
    }

    return false;
  }

  bool SystemTrigger::checkHighlevel(PropertyNodePtr _triggerProp) {
      if (m_evtName != "highlevelevent") {
        return false;
      }

      if (!m_properties.has("id")) {
        return false;
      }

      std::string id = m_properties.get("id");
      if (id.empty()) {
        return false;
      }

      PropertyNodePtr triggerEvent = _triggerProp->getPropertyByName("event");
      if (triggerEvent == NULL) {
        return false;
      }

      std::string sEvent = triggerEvent->getAsString();
      if (sEvent == id) {
        Logger::getInstance()->log("SystemTrigger::"
                "checkHighlevel Match: HighLevelEvent EventID:" + id);
        return true;
      }

    return false;
  }

  bool SystemTrigger::checkState(PropertyNodePtr _triggerProp) {
    if (m_evtName != EventName::AddonStateChange && m_evtName != EventName::StateChange) {
      return false;
    }

    std::string name = m_properties.get("statename");
    std::string state = m_properties.get("state");
    std::string value = m_properties.get("value");
    std::string oldvalue = m_properties.get("oldvalue");

    PropertyNodePtr triggerName = _triggerProp->getPropertyByName("name");
    PropertyNodePtr triggerState = _triggerProp->getPropertyByName("state");
    PropertyNodePtr triggerValue = _triggerProp->getPropertyByName("value");
    PropertyNodePtr triggerOldvalue = _triggerProp->getPropertyByName("oldvalue");

    if (m_evtName == EventName::AddonStateChange) {
      std::string scriptID = m_properties.get("scriptID");
      PropertyNodePtr triggerScriptID = _triggerProp->getPropertyByName("addon-id");
      if (triggerScriptID == NULL) {
        return false;
      }
      std::string sScriptID = triggerScriptID->getAsString();
      if (sScriptID != scriptID) {
        return false;
      }
    }

    if (triggerName == NULL) {
      return false;
    }
    if (triggerState == NULL && triggerValue == NULL) {
      return false;
    }

    std::string sName = triggerName->getAsString();
    if (sName != name) {
      return false;
    }

    std::string sState;
    std::string sValue;
    std::string sOldvalue;

    if (triggerState) {
      sState = triggerState->getAsString();
    }
    if (triggerValue) {
      sValue = triggerValue->getAsString();
    }
    if (triggerOldvalue) {
      sOldvalue = triggerOldvalue->getAsString();
    }

    if (sState == state || sValue == value) {
      if (sOldvalue.empty() || sOldvalue == oldvalue) {
        Logger::getInstance()->log("SystemTrigger::"
            "checkState Match: State:" + name + ", Value: " + state + "/" + value);
        return true;
      }
    }

    return false;
  }

  bool SystemTrigger::checkSensorValue(PropertyNodePtr _triggerProp) {
    if (m_evtName != EventName::DeviceSensorValue && m_evtName != EventName::ZoneSensorValue) {
      return false;
    }

    if (m_evtName == EventName::DeviceSensorValue) {
      PropertyNodePtr triggerDevice = _triggerProp->getPropertyByName("dsuid");
      std::string triggerDSID = triggerDevice ? triggerDevice->getAsString() : "";
      dsuid_t dsuid = m_evtSrcDSID;
      std::string eventDSID = dsuid2str(dsuid);
      if (triggerDSID != eventDSID) {
        Logger::getInstance()->log("SystemTrigger::checkSensor:: type: " + m_evtName +
                ", dsuid: " + dsuid2str(m_evtSrcDSID) + ", trigger dsuid " + triggerDSID, lsWarning);
        return false;
      }
    } else if (m_evtName == EventName::ZoneSensorValue) {
      PropertyNodePtr triggerZone = _triggerProp->getPropertyByName("zone");
      std::string zone = triggerZone ? triggerZone->getAsString() : "";
      if (strToInt(zone) != m_evtSrcZone) {
        Logger::getInstance()->log("SystemTrigger::checkSensor:: type: " + m_evtName +
                ", zone: " + intToString(m_evtSrcZone) + ", trigger zone " + zone, lsWarning);
        return false;
      }
    } else {
      return false;
    }

    std::string sensorType = m_properties.get("sensorType");
    PropertyNodePtr triggerSensorType = _triggerProp->getPropertyByName("sensor");
    std::string sSensorType = triggerSensorType ? triggerSensorType->getAsString() : "";
    if (sSensorType != sensorType) {
      Logger::getInstance()->log("SystemTrigger::checkSensor:: type: " + m_evtName +
              ", sensor: " + sensorType + ", trigger sensor " + sSensorType, lsWarning);
      return false;
    }

    PropertyNodePtr triggerValue = _triggerProp->getPropertyByName("value");
    PropertyNodePtr triggerOperator = _triggerProp->getPropertyByName("operator");

    if (!triggerValue || !triggerOperator) {
      Logger::getInstance()->log("SystemTrigger::checkSensor:: value or operation node missing",
                                 lsError);
      return false;
    }

    std::string sensorValueFloat = m_properties.get("sensorValueFloat");
    double eventValueFloat = strToDouble(sensorValueFloat);

    std::string sValue = triggerValue->getAsString();
    double triggerValueFloat = strToDouble(sValue);
    std::string sOperator = triggerOperator->getAsString();

    Logger::getInstance()->log("SystemTrigger::checkSensor:: value: " + sValue +
            ", event value: " + sensorValueFloat + ", operator " + sOperator);

    if (sOperator == "greater") {
      if (eventValueFloat > triggerValueFloat) {
        Logger::getInstance()->log("SystemTrigger::checkSensor:: Match: value: " + sValue +
                ", event value: " + sensorValueFloat);
        return true;
      }
    } else if (sOperator == "lower") {
      if (eventValueFloat < triggerValueFloat) {
        Logger::getInstance()->log("SystemTrigger::checkSensor:: Match: value: " + sValue +
                ", event value: " + sensorValueFloat);
        return true;
      }
    }

    return false;
  }

  bool SystemTrigger::checkEvent(PropertyNodePtr _triggerProp) {

    PropertyNodePtr triggerName = _triggerProp->getPropertyByName("name");

    if (triggerName == NULL) {
      //trigger structure is incomplete
      return false;
    }
    std::string sName = triggerName->getAsString();
    if (sName != m_evtName) {
      // names do not match
      return false;
    }

    PropertyNodePtr triggerParameters = _triggerProp->getPropertyByName("parameter");
    if (triggerParameters != NULL) {
      for (int i = 0; i < triggerParameters->getChildCount(); ++i) {
        PropertyNodePtr triggerParameter = triggerParameters->getChild(i);
        if (!m_properties.has(triggerParameter->getName())) {
          // if the event is missing a parameter given in the trigger structure --> no match
          return false;
        }

        std::string eventValue = m_properties.get(triggerParameter->getName());
        std::string triggerValue = triggerParameter->getAsString();
        if (eventValue != triggerValue) {
          // given parameters must match
          return false;
        }
      }
    }

    Logger::getInstance()->log("SystemTrigger::checkEvent:: Match event: " + sName);
    return true;
  }

  bool SystemTrigger::checkTrigger(PropertyNodePtr _triggerProp) {
    PropertyNodePtr appTrigger = _triggerProp->getPropertyByName(ptn_triggers);
    if (appTrigger == NULL) {
      return false;
    }

    for (int i = 0; i < appTrigger->getChildCount(); i++) {
      if (checkTriggerNode(appTrigger->getChild(i))) {
        return true;
      }
    }

    // no trigger matched
    return false;
  }

  bool SystemTrigger::checkTriggerNode(PropertyNodePtr triggerProp)
  {
    if (triggerProp == NULL) {
      return false;
    }

    PropertyNodePtr triggerType = triggerProp->getPropertyByName(ptn_type);
    if (triggerType == NULL) {
      return false;
    }

    std::string triggerValue = triggerType->getAsString();

    if (m_evtName == EventName::CallScene) {
      if (triggerValue == "zone-scene") {
        if (checkSceneZone(triggerProp)) {
          return true;
        }
      } else if (triggerValue == "device-scene") {
        if (checkDeviceScene(triggerProp)) {
          return true;
        }
      }

    } else if (m_evtName == EventName::CallSceneBus) {
      if (triggerValue == "bus-zone-scene") {
        if (checkSceneZone(triggerProp)) {
          return true;
        }
      }

    } else if (m_evtName == EventName::UndoScene) {
      if (triggerValue == "undo-zone-scene") {
        if (checkUndoSceneZone(triggerProp)) {
          return true;
        }
      }

    } else if (m_evtName == EventName::DeviceButtonClick) {
      if (triggerValue == "device-msg") {
        if (checkDevice(triggerProp)) {
          return true;
        }
      }

    } else if (m_evtName == EventName::ButtonDeviceAction) {
      if (triggerValue == "device-action") {
        if (checkDirectDeviceAction(triggerProp)) {
          return true;
        }
      }

    } else if (m_evtName == "deviceSensorEvent") {
      if (triggerValue == "device-sensor") {
        if (checkDeviceSensor(triggerProp)) {
          return true;
        }
      }

    } else if (m_evtName == "deviceSensorValue") {
      if (triggerValue == "device-sensor-value") {
        if (checkSensorValue(triggerProp)) {
          return true;
        }
      }

    } else if (m_evtName == "zoneSensorValue") {
      if (triggerValue == "zone-sensor-value") {
        if (checkSensorValue(triggerProp)) {
          return true;
        }
      }

    } else if (m_evtName == EventName::DeviceBinaryInputEvent) {
      if (triggerValue == "device-binary-input") {
        if (checkDeviceBinaryInput(triggerProp)) {
          return true;
        }
      }

    } else if (m_evtName == EventName::DeviceActionEvent) {
      if (triggerValue == "device-named-action") {
        if (checkDeviceNamedAction(triggerProp)) {
          return true;
        }
      }

    } else if (m_evtName == EventName::DeviceEventEvent) {
      if (triggerValue == "device-named-event") {
        if (checkDeviceNamedEvent(triggerProp)) {
          return true;
        }
      }

    } else if (m_evtName == "highlevelevent") {
      if (triggerValue == "custom-event") {
        if (checkHighlevel(triggerProp)) {
          return true;
        }
      }

    } else if (m_evtName == EventName::StateChange) {
      if (triggerValue == "state-change") {
        if (checkState(triggerProp)) {
          return true;
        }
      }
    } else if (m_evtName == EventName::AddonStateChange) {
      if (triggerValue == "addon-state-change") {
        if (checkState(triggerProp)) {
          return true;
        }
      }
    } else {
      if (triggerValue == "event") {
        if (checkEvent(triggerProp)) {
          return true;
        }
      }
    }

    // no trigger matched
    return false;
  }

  /**
   * damping() - decide if trigger shall be damped or an event emitted
   * @_triggerParamNode complete trigger parameter node
   * @return true if event shall be damped, false if no damping is applied
   */
  bool SystemTrigger::damping(PropertyNodePtr _triggerParamNode) {
    if (_triggerParamNode == NULL || !_triggerParamNode->getProperty(ptn_damping)) {
      return false;
    }

    PropertyNodePtr dampNode = _triggerParamNode ->getProperty(ptn_damping);
    if (!dampNode->getProperty(ptn_damp_interval)) {
      // no delay specified, nothing to do
      return false;
    }

    if (!dampNode->getProperty(ptn_damp_start_ts)) {
      // first trigger ever, no rate-limit possible
      PropertyNodePtr tmp = dampNode->createProperty(ptn_damp_start_ts);
      tmp->setStringValue(DateTime().toISO8601());
      return false;
    }

    // delay in seconds
    int interval = dampNode->getProperty(ptn_damp_interval)->getIntegerValue();
    if (interval < 0) {
      Logger::getInstance()->log("trigger::damping: invalid interval " +
                                 intToString(interval), lsWarning);
      return false;
    }

    PropertyNodePtr lastTsNode = dampNode->getProperty(ptn_damp_start_ts);
    DateTime lastTS = DateTime::parseISO8601(lastTsNode->getAsString());

    PropertyNodePtr rewindNode = dampNode->getProperty(ptn_damp_rewind);
    if (rewindNode && rewindNode->getBoolValue()) {
      // extend rate-limit interval
      lastTsNode->setStringValue(DateTime().toISO8601());
    }

    if (DateTime().difference(lastTS) < interval) {
      // really damp
      Logger::getInstance()->log("trigger:rate-limit", lsInfo);
      return true;
    }

    // rate-limit interval expired, start new interval
    lastTsNode->setStringValue(DateTime().toISO8601());
    return false;
  }

  /**
   * reschedule_action() - reschedule event if trigger fired again (optional)
   * @_triggerParamNode complete trigger node with all parameters
   * @return true if event was reschedule
   */
  bool SystemTrigger::rescheduleAction(PropertyNodePtr _triggerNode, PropertyNodePtr _triggerParamNode) {

    PropertyNodePtr lagNode = _triggerParamNode->getProperty(ptn_action_lag);

    if (lagNode == NULL) {
      return false;
    }

    if (!lagNode->getProperty(ptn_action_reschedule) ||
        !lagNode->getProperty(ptn_action_reschedule)->getBoolValue() ||
        !lagNode->getProperty(ptn_action_delay) ||
        !lagNode->getProperty(ptn_action_eventid)) {
      // rescheduling not enabled, or missing delay
      return false;
    }

    int delay = lagNode->getProperty(ptn_action_delay)->getIntegerValue();

    PropertyNodePtr lastTsNode = lagNode->getProperty(ptn_action_ts);
    DateTime lastTS = DateTime::parseISO8601(lastTsNode->getAsString());

    if (DateTime().difference(lastTS) > delay) {
      // assume action was executed, or let it execute
      // event queue has no interface to find out
      return false;
    }

    EventRunner &runner(DSS::getInstance()->getEventRunner());
    runner.removeEvent(lagNode->getProperty(ptn_action_eventid)->getStringValue());

    // relayTrigger will do the same, nevermind:
    lastTsNode->setStringValue(DateTime().toISO8601());
    relayTrigger(_triggerNode, _triggerParamNode);

    return true;
  }

  void SystemTrigger::relayTrigger(PropertyNodePtr _relay, PropertyNodePtr _triggerParamNode) {

    PropertyNodePtr triggerPath = _relay->getPropertyByName("triggerPath");
    PropertyNodePtr relayedEventName =
        _relay->getPropertyByName("relayedEventName");

    if ((triggerPath == NULL) || (relayedEventName == NULL)) {
      Logger::getInstance()->log("SystemTrigger::"
              "relayTrigger: Missing trigger properties path or event name");
      return;
    }

    std::string sTriggerPath = triggerPath->getAsString();
    std::string sRelayedEventName = relayedEventName->getAsString();
    std::string additionalParameter;

    PropertyNodePtr aParam =
        _relay->getPropertyByName("additionalRelayingParameter");
    if (aParam != NULL) {
      additionalParameter = aParam->getAsString();
    }

    boost::shared_ptr<Event> evt = boost::make_shared<Event>(sRelayedEventName);
    evt->applyProperties(m_properties);
    evt->setProperty("path", sTriggerPath);

    Logger::getInstance()->log("SystemTrigger::"
            "relayTrigger: received additional parameters: " +
            additionalParameter);

    // add additional parameters
    std::vector<std::string> params = dss::splitString(additionalParameter, ';');
    for (size_t i = 0; i < params.size(); i++) {
      std::pair<std::string, std::string> kv = splitIntoKeyValue(params.at(i));
      if (kv.first.empty() || kv.second.empty()) {
        continue;
      }
      Logger::getInstance()->log("SystemTrigger::"
              "relayTrigger: adding event parameters [" + kv.first + "=" +
              kv.second + "]");
      evt->setProperty(kv.first, kv.second);
    }

    if (!DSS::hasInstance()) {
      // some unit tests exit here
      return;
    }

    Logger::getInstance()->log("SystemTrigger::relayTrigger: relaying event \'" + evt->getName() + "\'");

    PropertyNodePtr lagNode = _triggerParamNode->getProperty(ptn_action_lag);
    if (!lagNode || !lagNode->getProperty(ptn_action_delay) ||
        (lagNode->getProperty(ptn_action_delay)->getIntegerValue() == 0)) {
        // no lag configured, immediately execute
        DSS::getInstance()->getEventQueue().pushEvent(evt);
        return;
    }

    if (lagNode->getProperty(ptn_action_delay)->getIntegerValue() < 0) {
      Logger::getInstance()->log("SystemTrigger::relayTrigger: invalid lag paramter" +
                                 lagNode->getProperty(ptn_action_delay)->getAsString(),
                                 lsWarning);
      return;
    }

    Logger::getInstance()->log("SystemTrigger::relayTrigger: action lag", lsWarning);

    evt->setProperty("time", "+" + lagNode->getProperty(ptn_action_delay)->getAsString());

    std::string id = DSS::getInstance()->getEventQueue().pushTimedEvent(evt);
    if (id.empty()) {
      // failed to schedule the event, invalid lag parameter
      Logger::getInstance()->log("SystemTrigger::relayTrigger: dropping event after failure to queue it", lsWarning);
      return;
    }

    lagNode->createProperty(ptn_action_eventid)->setStringValue(id);
    lagNode->createProperty(ptn_action_ts)->setStringValue(DateTime().toISO8601());
  }

  void SystemTrigger::run() {
    if (!DSS::hasInstance()) {
      return;
    }

    DSS::getInstance()->getSecurity().loginAsSystemUser(
        "EventInterpreterPluginSystemTrigger needs system rights");

    PropertySystem &propSystem(DSS::getInstance()->getPropertySystem());

    PropertyNodePtr triggerProperty = propSystem.getProperty("/usr/triggers");
    if (triggerProperty == NULL) {
      return;
    }

    PropertyNodePtr triggerPathNode;
    std::string sTriggerPath;
    try {
      for (int i = 0; i < triggerProperty->getChildCount(); i++) {
        PropertyNodePtr triggerNode = triggerProperty->getChild(i);
        if (triggerNode == NULL) {
          continue;
        }
        triggerPathNode = triggerNode->getPropertyByName("triggerPath");
        if (triggerPathNode == NULL) {
          continue;
        }

        PropertyNodePtr triggerParamNode =
          propSystem.getProperty(triggerPathNode->getStringValue());
        if (triggerParamNode == NULL) {
          continue;
        }

        sTriggerPath = triggerPathNode->getStringValue();

        if (checkTrigger(triggerParamNode) && checkSystemCondition(sTriggerPath)) {


          PropertyNodePtr lagNode = triggerParamNode->getProperty(ptn_action_lag);
          PropertyNodePtr dampNode = triggerParamNode->getProperty(ptn_damping);

          if ((lagNode && lagNode->getProperty(ptn_action_lag)) &&
              (dampNode && dampNode->getProperty(ptn_damp_interval))) {
            int dampInterval = dampNode->getProperty(ptn_damp_interval)->getIntegerValue();
            int actionLag = lagNode->getProperty(ptn_action_lag)->getIntegerValue();

            if (actionLag > dampInterval) {
              // otherwise new events are scheduled before intial event fired
              // we could end up rescheduling multiple events
              Logger::getInstance()->log("Action lag should not exceed damping interval",
                                         lsWarning);
            }
          }

          if (rescheduleAction(triggerNode, triggerParamNode)) {
            // no new actions spawned while rescheduling a pending event
            continue;
          }

          if (damping(triggerParamNode)) {
            // trigger is rate-limited, suppress spawning new event
            continue;
          }


          relayTrigger(triggerNode, triggerParamNode);
        }
      } // for loop
    } catch (PropertyTypeMismatch& e) {
      Logger::getInstance()->log("SystemTrigger::run: error parsing trigger at " +
          sTriggerPath + ": " + e.what(), lsInfo);
    } catch (std::runtime_error& e) {
      Logger::getInstance()->log("SystemTrigger::run: runtime error at " +
          sTriggerPath + ": " + e.what(), lsInfo);
    }
  }
}
