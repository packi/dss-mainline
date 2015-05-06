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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "system_states.h"

#include "base.h"
#include "dss.h"
#include "event/event_fields.h"
#include "foreach.h"
#include "logger.h"
#include "model/zone.h"
#include "model/group.h"
#include "model/device.h"
#include "model/apartment.h"
#include "model/state.h"
#include "model/modelconst.h"
#include "propertysystem.h"
#include "security/security.h"

namespace dss {

namespace SystemStateName {
  const std::string Sun = "sun";
  const std::string Frost = "frost";
  const std::string HeatingMode = "heating_mode";
}

EventInterpreterPluginSystemState::EventInterpreterPluginSystemState(EventInterpreter* _pInterpreter)
: EventInterpreterPlugin("system_state", _pInterpreter)
{ }

EventInterpreterPluginSystemState::~EventInterpreterPluginSystemState()
{ }

void EventInterpreterPluginSystemState::handleEvent(Event& _event, const EventSubscription& _subscription) {
  Logger::getInstance()->log("EventInterpreterPluginSystemState::"
          "handleEvent: processing event \'" + _event.getName() + "\'",
          lsDebug);

  boost::shared_ptr<SystemState> state = boost::make_shared<SystemState>();

  if (!state->setup(_event)) {
    Logger::getInstance()->log("EventInterpreterPluginSystemState::"
            "handleEvent: could not setup event data!");
    return;
  }

  addEvent(state);
}


SystemState::SystemState() : SystemEvent(), m_evtRaiseLocation(erlState),
  m_apartment(DSS::getInstance()->getApartment()) {
}

SystemState::~SystemState() {
}

std::string SystemState::formatZoneName(const std::string &_name, int _zoneId) {
  // set state in the zone the dsuid is logically attached
  return "zone." + intToString(_zoneId) + "." + _name;
}

std::string SystemState::formatGroupName(const std::string &_name, int _groupId) {
  assert(_groupId >= GroupIDAppUserMin);
  return "zone.0.group." + intToString(_groupId) + "." + _name;;
}

/* TODO why was formatGroupName insufficient */
std::string SystemState::formatGroupName2(const std::string &_name, int _groupId) {
  assert(_groupId >= GroupIDAppUserMin);
  return _name + ".group." + intToString(_groupId);
}

boost::shared_ptr<State> SystemState::registerState(std::string _name,
                                                    bool _persistent) {
  boost::shared_ptr<State> state =
    m_apartment.allocateState(StateType_Service, _name, "system_state");
  state->setPersistence(_persistent);
  return state;
}

boost::shared_ptr<State> SystemState::getOrRegisterState(std::string _name) {
  try {
    return m_apartment.getState(StateType_Service, _name);
  } catch (ItemNotFoundException &ex) {
    return registerState(_name, true);
  }
}

bool SystemState::lookupState(boost::shared_ptr<State> &_state,
                              const std::string &_name) {
  try {
    _state = m_apartment.getState(StateType_Service, _name);
    assert(_state != NULL);
    return true;
  } catch (ItemNotFoundException &ex) {
    return false;
  }
}

void SystemState::callScene(int _zoneId, int _groupId, int _sceneId,
                            callOrigin_t _origin)
{
  try {
    boost::shared_ptr<Zone> z = m_apartment.getZone(_zoneId);
    boost::shared_ptr<Group> g = z->getGroup(_groupId);
    g->callScene(_origin, SAC_MANUAL, _sceneId, "", false);
  } catch (ItemNotFoundException &ex) {}
}

void SystemState::undoScene(int _zoneId, int _groupId, int _sceneId,
                            callOrigin_t _origin)
{
  try {
    boost::shared_ptr<Zone> z = m_apartment.getZone(_zoneId);
    boost::shared_ptr<Group> g = z->getGroup(_groupId);
    g->undoScene(_origin, SAC_MANUAL, _sceneId, "");
  } catch (ItemNotFoundException &ex) {}
}

void SystemState::bootstrap() {
  boost::shared_ptr<State> state;

  state = registerState("presence", true);
  // Default presence status is "present" - set default before loading
  // old status from persistent storage
  state->setState(coSystem, State_Active);

  State::ValueRange_t presenceValues;
  presenceValues.push_back("unknown");
  presenceValues.push_back("present");
  presenceValues.push_back("absent");
  presenceValues.push_back("invalid");
  state->setValueRange(presenceValues);

  state = registerState("hibernation", true);

  State::ValueRange_t sleepmodeValues;
  sleepmodeValues.push_back("unknown");
  sleepmodeValues.push_back("awake");
  sleepmodeValues.push_back("sleeping");
  sleepmodeValues.push_back("invalid");
  state->setValueRange(sleepmodeValues);

  registerState("alarm", true);
  registerState("alarm2", true);
  registerState("alarm3", true);
  registerState("alarm4", true);
  registerState("panic", true);
  registerState("fire", true);
  registerState("wind", true);
  registerState("rain", true);

  registerState(SystemStateName::Sun, true);
  registerState(SystemStateName::Frost, true);
  state = registerState(SystemStateName::HeatingMode, true);
  State::ValueRange_t heatingModeValues;
  heatingModeValues.push_back("off");
  heatingModeValues.push_back("heating");
  heatingModeValues.push_back("cooling");
  heatingModeValues.push_back("auto");
  state->setValueRange(heatingModeValues);
}

void SystemState::startup() {
  bool absent = false;
  bool panic = false;
  bool alarm = false;
  bool sleeping = false;

  foreach (boost::shared_ptr<Device> device, m_apartment.getDevicesVector()) {
    if (device == NULL) {
      continue;
    }

    foreach (boost::shared_ptr<DeviceBinaryInput_t> input,
             device->getBinaryInputs()) {
      // motion
      if ((input->m_inputType == BinaryInputIDMovement) ||
          (input->m_inputType == BinaryInputIDMovementInDarkness)) {
        std::string stateName;
        if (input->m_targetGroupId >= GroupIDAppUserMin) {
          stateName = formatGroupName("motion", input->m_targetGroupId);
        } else {
          stateName = formatZoneName("motion", device->getZoneID());
        }
        getOrRegisterState(stateName);
      }

      // presence
      if ((input->m_inputType == BinaryInputIDPresence) ||
          (input->m_inputType == BinaryInputIDPresenceInDarkness)) {
        std::string stateName;
        if (input->m_targetGroupId >= GroupIDAppUserMin) {
          stateName = formatGroupName("presence", input->m_targetGroupId);
        } else {
          stateName = formatZoneName("presence", device->getZoneID());
        }
        getOrRegisterState(stateName);
      }

      // wind monitor
      if (input->m_inputType == BinaryInputIDWindDetector) {
        if (input->m_targetGroupId >= GroupIDAppUserMin) {
          getOrRegisterState(formatGroupName2("wind", input->m_targetGroupId));
        }
      }

      // rain monitor
      if (input->m_inputType == BinaryInputIDRainDetector) {
        if (input->m_targetGroupId >= GroupIDAppUserMin) {
          getOrRegisterState(formatGroupName2("rain", input->m_targetGroupId));
        }
      }
    } // per device binary inputs for loop
  } // devices for loop

  foreach (boost::shared_ptr<Zone> zone, m_apartment.getZones()) {
    if ((zone == NULL) || (!zone->isPresent())) {
      continue;
    }

    foreach (boost::shared_ptr<Group> group, zone->getGroups()) {
      if (isAppUserGroup(group->getID())) {
        if (group->getStandardGroupID() == GroupIDGray) {
          registerState(formatGroupName2("wind", group->getID()), true);
        }
        continue;
      }

      if (group->getLastCalledScene() == SceneAbsent) {
        absent = true;
      }

      if (group->getLastCalledScene() == ScenePanic) {
        panic = true;
      }

      if (group->getLastCalledScene() == SceneAlarm) {
        alarm = true;
      }

      if (group->getLastCalledScene() == SceneSleeping) {
        sleeping = true;
      }
    } // groups for loop
  } // zones for loop

  boost::shared_ptr<State> state;
  if (lookupState(state, "presence")) {
    if ((absent == true) && (state->getState() == State_Inactive)) {
      state->setState(coJSScripting, "absent");
    } else if (absent == false) {
      state->setState(coJSScripting, "present");
    }
  }

  if (lookupState(state, "hibernation")) {
    if ((sleeping == true) && (state->getState() == State_Inactive)) {
      state->setState(coJSScripting, "sleeping");
    } else if (sleeping == false) {
      state->setState(coJSScripting, "awake");
    }
  } // hibernation state

  if (lookupState(state, "panic")) {
    if ((panic == true) && (state->getState() == State_Inactive)) {
      state->setState(coJSScripting, State_Active);
    } else if (panic == false) {
      state->setState(coJSScripting, State_Inactive);
    }
  } // panic state

  if (lookupState(state, "alarm")) {
    if ((alarm == true) && (state->getState() == State_Inactive)) {
      state->setState(coJSScripting, State_Active);
    } else if (alarm == false) {
      state->setState(coJSScripting, State_Inactive);
    }
  } // alarm state

  // clear fire alarm after 6h
  #define CLEAR_ALARM_URLENCODED_JSON "%7B%20%22name%22%3A%22FireAutoClear%22%2C%20%22id%22%3A%20%22system_state_fire_alarm_reset%22%2C%22triggers%22%3A%5B%7B%20%22type%22%3A%22state-change%22%2C%20%22name%22%3A%22fire%22%2C%20%22state%22%3A%22active%22%7D%5D%2C%22delay%22%3A21600%2C%22actions%22%3A%5B%7B%20%22type%22%3A%22undo-zone-scene%22%2C%20%22zone%22%3A0%2C%20%22group%22%3A0%2C%20%22scene%22%3A76%2C%20%22force%22%3A%22false%22%2C%20%22delay%22%3A0%20%7D%5D%2C%22conditions%22%3A%7B%20%22enabled%22%3Anull%2C%22weekdays%22%3Anull%2C%22timeframe%22%3Anull%2C%22zoneState%22%3Anull%2C%22systemState%22%3A%5B%7B%22name%22%3A%22fire%22%2C%22value%22%3A%221%22%7D%5D%2C%22addonState%22%3Anull%7D%2C%22scope%22%3A%22system_state.auto_cleanup%22%7D"
/*
  var t0 = 6 * 60 * 60;
  var sr = '{ "name":"FireAutoClear", "id": "system_state_fire_alarm_reset",\
    "triggers":[{ "type":"state-change", "name":"fire", "state":"active"}],\
    "delay":' + t0 + ',\
    "actions":[{ "type":"undo-zone-scene", "zone":0, "group":0, "scene":76, "force":"false", "delay":0 }],\
    "conditions":{ "enabled":null,"weekdays":null,"timeframe":null,"zoneState":null,\
      "systemState":[{"name":"fire","value":"1"}],\
      "addonState":null},\
    "scope":"system_state.auto_cleanup"\
    }';
*/
  boost::shared_ptr<Event> event = boost::make_shared<Event>("system-addon-scene-responder.config");
  event->setProperty("actions", "save");
  event->setProperty("value", CLEAR_ALARM_URLENCODED_JSON);
  DSS::getInstance()->getEventQueue().pushEvent(event);
}

std::string SystemState::getData(int *zoneId, int *groupId, int *sceneId,
                                 callOrigin_t *callOrigin) {
  std::string originDSUID;

  *callOrigin = coUnknown;
  if (m_properties.has(ef_callOrigin)) {
      *callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ef_callOrigin), 0);
  }

  if (!zoneId || !groupId || !sceneId) {
    return originDSUID;
  }

  *zoneId = -1;
  *groupId = -1;
  *sceneId = -1;

  if (m_properties.has("sceneID")) {
      *sceneId = strToIntDef(m_properties.get("sceneID"), -1);
  }

  if (m_properties.has(ef_originDSUID)) {
    originDSUID = m_properties.get(ef_originDSUID);
  }

  if (((m_evtRaiseLocation == erlGroup) ||
       (m_evtRaiseLocation == erlApartment)) && (m_raisedAtGroup != NULL)) {
      *zoneId = m_raisedAtGroup->getZoneID();
      *groupId = m_raisedAtGroup->getID();
  } else if ((m_evtRaiseLocation == erlState) && (m_raisedAtState != NULL)) {
    if (m_raisedAtState->getType() == StateType_Device) {
      boost::shared_ptr<Device> device = m_raisedAtState->getProviderDevice();
      *zoneId = device->getZoneID();
      originDSUID = dsuid2str(device->getDSID());
    } else if (m_raisedAtState->getType() == StateType_Group) {
      boost::shared_ptr<Group> group = m_raisedAtState->getProviderGroup();
      *zoneId = group->getZoneID();
      *groupId = group->getID();
    }
  } else if ((m_evtRaiseLocation == erlDevice) &&
             (m_raisedAtDevice != NULL)) {
    originDSUID = dsuid2str(m_raisedAtDevice->getDSID());
  }

  return originDSUID;
}

void SystemState::callscene() {
  int zoneId = -1;
  int groupId = -1;
  int sceneId = -1;
  callOrigin_t callOrigin = coUnknown;

  getData(&zoneId, &groupId, &sceneId, &callOrigin);

  if (callOrigin == coSystem) {
      // ignore scene calls originated by server generated system level events
      //  e.g. scene calls issued by state changes
      return;
  }

  boost::shared_ptr<State> state;

  if (groupId == 0) {
    if ((groupId == 0) && (sceneId == SceneAbsent)) {
      if (lookupState(state, "presence")) {
        state->setState(coSystem, "absent");
      }
      // #2561: auto-clear panic and fire
      if (lookupState(state, "panic")) {
        if (state->getState() == State_Active) {
          undoScene(0, 0, ScenePanic, coSystem);
        }
      }
      if (lookupState(state, "fire")) {
        if (state->getState() == State_Active) {
          undoScene(0, 0, SceneFire, coSystem);
        }
      }
    } else if ((groupId == 0) && (sceneId == ScenePresent)) {
      if (lookupState(state, "presence")) {
        state->setState(coSystem, "present");
      }
    } else if ((groupId == 0) && (sceneId == SceneSleeping)) {
      if (lookupState(state, "hibernation")) {
        state->setState(coSystem, "sleeping");
      }
    } else if ((groupId == 0) && (sceneId == SceneWakeUp)) {
      if (lookupState(state, "hibernation")) {
        state->setState(coSystem, "awake");
      }
    } else if ((groupId == 0) && (sceneId == ScenePanic)) {
      if (lookupState(state, "panic")) {
        state->setState(coSystem, State_Active);
      }
    } else if ((groupId == 0) && (sceneId == SceneFire)) {
      if (lookupState(state, "fire")) {
        state->setState(coSystem, State_Active);
      }
    } else if ((groupId == 0) && (sceneId == SceneAlarm)) {
      if (lookupState(state, "alarm")) {
        state->setState(coSystem, State_Active);
      }
    } else if ((groupId == 0) && (sceneId == SceneAlarm2)) {
      if (lookupState(state, "alarm2")) {
        state->setState(coSystem, State_Active);
      }
    } else if ((groupId == 0) && (sceneId == SceneAlarm3)) {
      if (lookupState(state, "alarm3")) {
        state->setState(coSystem, State_Active);
      }
    } else if ((groupId == 0) && (sceneId == SceneAlarm4)) {
      if (lookupState(state, "alarm4")) {
        state->setState(coSystem, State_Active);
      }
    } else if (sceneId == SceneWindActive) {
      state = getOrRegisterState("wind");
      state->setState(coSystem, State_Active);
    } else if (sceneId == SceneWindInactive) {
      state = getOrRegisterState("wind");
      state->setState(coSystem, State_Inactive);
    } else if (sceneId == SceneRainActive) {
      state = getOrRegisterState("rain");
      state->setState(coSystem, State_Active);
    } else if (sceneId == SceneRainInactive) {
      state = getOrRegisterState("rain");
      state->setState(coSystem, State_Inactive);
      for (size_t grp = GroupIDAppUserMin; grp <= GroupIDAppUserMax; grp++) {
        if (lookupState(state, formatGroupName2("rain", groupId))) {
          state->setState(coSystem, State_Inactive);
        }
      }
    }
  } else if (isAppUserGroup(groupId)) {
    if (sceneId == SceneWindActive) {
      if (groupId == 0) {
        state = getOrRegisterState("wind");
      } else if (isAppUserGroup(groupId)) {
        state = getOrRegisterState(formatGroupName2("wind", groupId));
      }
      state->setState(coSystem, State_Active);
    } else if (sceneId == SceneWindInactive) {
      if (groupId == 0) {
        state = getOrRegisterState("wind");
      } else if (isAppUserGroup(groupId)) {
        state = getOrRegisterState(formatGroupName2("wind", groupId));
      }
      state->setState(coSystem, State_Inactive);
    } else if (sceneId == SceneRainActive) {
      if (groupId == 0) {
        state = getOrRegisterState("rain");
      } else if (isAppUserGroup(groupId)) {
        state = getOrRegisterState(formatGroupName2("rain", groupId));
      }
      state->setState(coSystem, State_Active);
    } else if (sceneId == SceneRainInactive) {
      if (groupId == 0) {
        state = getOrRegisterState("rain");
        state->setState(coSystem, State_Inactive);
        for (size_t grp = GroupIDAppUserMin; grp <= GroupIDAppUserMax; grp++) {
          if (lookupState(state, formatGroupName2("rain", groupId))) {
            state->setState(coSystem, State_Inactive);
          }
        }
      } else if (isAppUserGroup(groupId)) {
        state = getOrRegisterState(formatGroupName2("rain", groupId));
        state->setState(coSystem, State_Inactive);
      }
    }
  }
}

void SystemState::undoscene() {
  int zoneId = -1;
  int groupId = -1;
  int sceneId = -1;
  callOrigin_t callOrigin = coUnknown;
  std::string originDSUID;
  boost::shared_ptr<State> state;

  originDSUID = getData(&zoneId, &groupId, &sceneId, &callOrigin);
  dsuid_t dsuid = str2dsuid(originDSUID);

  if (groupId != 0) {
    // panic/fire/alarm apply to broadcast group-0
    // no undoscene for wind/rain
    return;
  }

  if ((groupId == 0) && (sceneId == ScenePanic)) {
    if (lookupState(state, "panic")) {
      state->setState(coSystem, State_Inactive);
    }

    // #2561: auto-reset fire if panic was reset by a button
    if (lookupState(state, "fire")) {
      if ((dsuid != DSUID_NULL) && (callOrigin == coDsmApi) &&
          (state->getState() == State_Active)) {
        undoScene(0, 0, SceneFire, coSystem);
      } // valid dSID && dSM API origin && state == active
    }
  } else if ((groupId == 0) && (sceneId == SceneFire)) {
    if (lookupState(state, "fire")) {
      state->setState(coSystem, State_Inactive);
    }

    // #2561: auto-reset panic if fire was reset by a button
    if (lookupState(state, "panic")) {
      if ((dsuid != DSUID_NULL) && (callOrigin == coDsmApi)
          && (state->getState() == State_Active)) {
        undoScene(0, 0, ScenePanic, coSystem);
      } // valid dSID && dSM API origin && state == active
    }
  } else if ((groupId == 0) && (sceneId == SceneAlarm)) {
    if (lookupState(state, "alarm")) {
      state->setState(coSystem, State_Inactive);
    }
  } else if ((groupId == 0) && (sceneId == SceneAlarm2)) {
    if (lookupState(state, "alarm2")) {
      state->setState(coSystem, State_Inactive);
    }
  } else if ((groupId == 0) && (sceneId == SceneAlarm3)) {
    if (lookupState(state, "alarm3")) {
      state->setState(coSystem, State_Inactive);
    }
  } else if ((groupId == 0) && (sceneId == SceneAlarm4)) {
    if (lookupState(state, "alarm4")) {
      state->setState(coSystem, State_Inactive);
    }
  }
}

void SystemState::stateBinaryInputGeneric(State &_state,
                                          int targetGroupType,
                                          int targetGroupId) {
  try {
    std::string stateName = _state.getName();
    PropertyNodePtr pNode =
        DSS::getInstance()->getPropertySystem().getProperty(
            "/scripts/system_state/" + stateName + "." +
            intToString(targetGroupType) + "." +
            intToString(targetGroupId));
    if (pNode == NULL) {
      pNode = DSS::getInstance()->getPropertySystem().createProperty(
            "/scripts/system_state/" + stateName + "." +
            intToString(targetGroupType) + "." +
            intToString(targetGroupId));
      pNode->setIntegerValue(0);
    }

    if (m_properties.has("value")) {
      std::string val = m_properties.get("value");
      int iVal = strToIntDef(val, -1);
      if (iVal == 1) {
        pNode->setIntegerValue(pNode->getIntegerValue() + 1);
        _state.setState(coSystemBinaryInput, State_Active);
      } else if (iVal == 2) {
        if (pNode->getIntegerValue() > 0) {
          pNode->setIntegerValue(pNode->getIntegerValue() - 1);
        }
        if (pNode->getIntegerValue() == 0) {
          _state.setState(coSystemBinaryInput, State_Inactive);
        }
      }
    } // m_properties.has("value")
  } catch (ItemNotFoundException &ex) {}
}

/**
1 Präsenz
2 Helligkeit (Raum)
3 Präsenz bei Dunkelheit
4 Dämmerung (Außen)
5 Bewegung
6 Bewegung bei Dunkelheit
7 Rauchmelder
8 Windwächter
9 Regenwächter
10 Sonneneinstrahlung
11 Raumthermostat
*/

void SystemState::stateBinaryinput() {
  if (m_raisedAtState == NULL) {
    return;
  }
  boost::shared_ptr<Device> pDev = m_raisedAtState->getProviderDevice();

  if (!m_properties.has("statename")) {
    return;
  }

  std::string statename = m_properties.get("statename");
  if (statename.empty()) {
    return;
  }

  PropertyNodePtr stateNode =
        DSS::getInstance()->getPropertySystem().getProperty(
                                                "/usr/states/" + statename);
  if (stateNode == NULL) {
    return;
  }

  PropertyNodePtr iiNode = stateNode->getProperty("device/inputIndex");
  if (iiNode == NULL) {
    return;
  }

  uint8_t inputIndex = (uint8_t)iiNode->getIntegerValue();
  const boost::shared_ptr<DeviceBinaryInput_t> devInput = pDev->getBinaryInput(inputIndex);

  if (devInput->m_inputId != 15) {
    return;
  }

  // motion
  if ((devInput->m_inputType == BinaryInputIDMovement) ||
      (devInput->m_inputType == BinaryInputIDMovementInDarkness)) {
    if (devInput->m_targetGroupId >= GroupIDAppUserMin) {
      statename = formatGroupName("motion", devInput->m_targetGroupId);
    } else {
      statename = formatZoneName("motion", pDev->getZoneID());
    }
    boost::shared_ptr<State> state = getOrRegisterState(statename);
    stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
                            devInput->m_targetGroupId);
  }

  // presence
  if ((devInput->m_inputType == BinaryInputIDPresence) ||
      (devInput->m_inputType == BinaryInputIDPresenceInDarkness)) {
    if (devInput->m_targetGroupId >= GroupIDAppUserMin) {
      statename = formatGroupName("presence", devInput->m_targetGroupId);
    } else {
      statename = formatZoneName("presence", pDev->getZoneID());
    }
    boost::shared_ptr<State> state = getOrRegisterState(statename);
    stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
                            devInput->m_targetGroupId);
  }

  // smoke detector
  if (devInput->m_inputType == BinaryInputIDSmokeDetector) {
    boost::shared_ptr<State> state;
    if (lookupState(state, "fire")) {
      if (m_properties.has("value")) {
        std::string val = m_properties.get("value");
        int iVal = strToIntDef(val, -1);
        if (iVal == 1) {
          state->setState(coSystemBinaryInput, State_Active);
        }
      }
    }
  }

  // wind monitor
  if (devInput->m_inputType == BinaryInputIDWindDetector) {
    boost::shared_ptr<State> state;
    // create state for a user group if it does not exist (new group?)
    if (devInput->m_targetGroupId >= GroupIDAppUserMin) {
      statename = formatGroupName2("wind", devInput->m_targetGroupId);
      state = getOrRegisterState(statename);
      stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
                              devInput->m_targetGroupId);
    } else if (lookupState(state, "wind")) {
      stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
                              devInput->m_targetGroupId);
    }
  }

  // rain monitor
  if (devInput->m_inputType == BinaryInputIDRainDetector) {
    boost::shared_ptr<State> state;
    // create state for a user group if it does not exist (new group?)
    if (devInput->m_targetGroupId >= GroupIDAppUserMin) {
      statename = formatGroupName2("rain", devInput->m_targetGroupId);
      state = getOrRegisterState(statename);
      stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
                              devInput->m_targetGroupId);
    } else if (lookupState(state, "rain")) {
      stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
                              devInput->m_targetGroupId);
    }
  }

  // zone thermostat
  if (devInput->m_inputType == BinaryInputIDRoomThermostat) {
    if (m_properties.has("value")) {
      std::string val = m_properties.get("value");
      int iVal = strToIntDef(val, -1);
      int sceneID = SceneOff;
      if (iVal == 1) {
        sceneID = Scene1;
      }
      callScene(pDev->getZoneID(), GroupIDHeating, sceneID, coSystemBinaryInput);
    }
  }
}

void SystemState::stateApartment() {
  if (!m_properties.has("statename")) {
    return;
  }

  std::string statename = m_properties.get("statename");
  if (statename.empty()) {
    return;
  }

  int zoneId;
  int groupId;
  int sceneId;
  callOrigin_t callOrigin = coUnknown;

  getData(&zoneId, &groupId, &sceneId, &callOrigin);

  groupId = 0;
  size_t groupName = statename.find(".group");
  if (groupName != std::string::npos) {
    std::string id = statename.substr(groupName + 6);
    groupId = strToIntDef(id, 0);
  }

  if (callOrigin == coSystem) {
    // ignore state change originated by server generated system level events,
    // e.g. scene calls issued by state changes, to avoid loops
    return;
  }

  int iVal = -1;
  if (m_properties.has("value")) {
    std::string val = m_properties.get("value");
    iVal = strToIntDef(val, -1);
  }

  if (statename == "fire") {
    if (iVal == 1) {
      callScene(0, 0, SceneFire, coSystem);
    }
  }

  if (statename.substr(0, 4) == "rain") {
    if (iVal == 1) {
      callScene(0, groupId, SceneRainActive, coSystem);
    } else if (iVal == 2) {
      callScene(0, groupId, SceneRainInactive, coSystem);
    }
  }

  if (statename.substr(0, 4) == "wind") {
    if (iVal == 1) {
      callScene(0, groupId, SceneWindActive, coSystem);
    } else if (iVal == 2) {
      callScene(0, groupId, SceneWindInactive, coSystem);
    }
  }
}

bool SystemState::setup(Event& _event) {
  m_evtName = _event.getName();
  m_evtRaiseLocation = _event.getRaiseLocation();
  m_raisedAtGroup = _event.getRaisedAtGroup(m_apartment);
  m_raisedAtDevice = _event.getRaisedAtDevice();
  m_raisedAtState = _event.getRaisedAtState();
  return SystemEvent::setup(_event);
}

void SystemState::run() {
  if (DSS::hasInstance()) {
    DSS::getInstance()->getSecurity().loginAsSystemUser(
      "SystemState needs system rights");
  } else {
    return;
  }

  try {
    if (m_evtName == EventName::Running) {
      bootstrap();
    } else if (m_evtName == "model_ready") {
      startup();
    } else if (m_evtName == EventName::CallScene) {
      if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
        callscene();
      }
    } else if (m_evtName == EventName::UndoScene) {
      undoscene();
    } else if (m_evtName == EventName::StateChange) {
      if (m_evtRaiseLocation) {

      }
      if (m_raisedAtState->getType() == StateType_Device) {
        stateBinaryinput();
      } else if (m_raisedAtState->getType() == StateType_Service) {
        stateApartment();
      }
    }
  } catch(ItemNotFoundException& ex) {
    Logger::getInstance()->log("SystemState::run: item not found data model error", lsInfo);
  }
}

} // namespace dss
