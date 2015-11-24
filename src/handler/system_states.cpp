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
#include "model/cluster.h"
#include "model/device.h"
#include "model/apartment.h"
#include "model/state.h"
#include "model/modelconst.h"
#include "propertysystem.h"
#include "security/security.h"

namespace dss {

namespace StateName {
  const std::string Alarm = "alarm";
  const std::string Alarm2 = "alarm2";
  const std::string Alarm3 = "alarm3";
  const std::string Alarm4 = "alarm4";
  const std::string BuildingService = "building_service";
  const std::string Fire = "fire";
  const std::string Frost = "frost";
  const std::string HeatingModeControl = "heating_water_system";
  const std::string Hibernation = "hibernation";
  const std::string Panic = "panic";
  const std::string Motion = "motion";
  const std::string OperationLock = "operation_lock";
  const std::string Presence = "presence";
  const std::string Rain = "rain";
  const std::string Wind = "wind";
  const std::string HeatingSystem = "heating_system";
  const std::string HeatingSystemMode = "heating_system_mode";
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
  return "cluster." + intToString(_groupId) + "." + _name;
}

std::string SystemState::formatAppartmentStateName(const std::string &_name, int _groupId) {
  assert(_groupId >= GroupIDAppUserMin);
  return _name + ".group" + intToString(_groupId);
}

boost::shared_ptr<State> SystemState::registerState(std::string _name, bool _persistent) {
  boost::shared_ptr<State> state = boost::make_shared<State> (StateType_Service, _name, "");
  m_apartment.allocateState(state);
  state->setPersistence(_persistent);
  return state;
}

boost::shared_ptr<State> SystemState::getOrRegisterState(std::string _name) {
  boost::shared_ptr<State> state;
  if (lookupState(state, _name)) {
    return state;
  }
  return registerState(_name, true);
}

boost::shared_ptr<State> SystemState::loadPersistentState(eStateType _type, std::string _name) {
  boost::shared_ptr<State> state = boost::make_shared <State> (_type, _name, "");
  if (state->hasPersistentData()) {
    state->setPersistence(true);
    m_apartment.allocateState(state);
  } else {
    state.reset();
  }
  return state;
}

bool SystemState::lookupState(boost::shared_ptr<State> &_state,
                              const std::string &_name) {
  try {
    _state = m_apartment.getNonScriptState(_name);
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

  state = registerState(StateName::Presence, true);
  // Default presence status is "present" - set default before loading
  // old status from persistent storage
  state->setState(coSystem, State_Active);

  State::ValueRange_t presenceValues;
  presenceValues.push_back("unknown");
  presenceValues.push_back("present");
  presenceValues.push_back("absent");
  presenceValues.push_back("invalid");
  state->setValueRange(presenceValues);

  state = registerState(StateName::Hibernation, true);

  State::ValueRange_t sleepmodeValues;
  sleepmodeValues.push_back("unknown");
  sleepmodeValues.push_back("awake");
  sleepmodeValues.push_back("sleeping");
  sleepmodeValues.push_back("invalid");
  state->setValueRange(sleepmodeValues);

  registerState(StateName::Alarm, true);
  registerState(StateName::Alarm2, true);
  registerState(StateName::Alarm3, true);
  registerState(StateName::Alarm4, true);
  registerState(StateName::Panic, true);
  registerState(StateName::Fire, true);
  registerState(StateName::Wind, true);
  registerState(StateName::Rain, true);
  registerState(StateName::Frost, true);
  registerState(StateName::HeatingSystem, true);
  registerState(StateName::HeatingSystemMode, true);

  state = registerState(StateName::HeatingModeControl, true);
  State::ValueRange_t heatingModeValues;
  heatingModeValues.push_back("off");
  heatingModeValues.push_back("heating");
  heatingModeValues.push_back("cooling");
  heatingModeValues.push_back("auto");
  state->setValueRange(heatingModeValues);
  if (!state->hasPersistentData()) {
    state->setState(coSystemStartup, "heating");
  }
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
          stateName = formatGroupName(StateName::Motion, input->m_targetGroupId);
        } else {
          stateName = formatZoneName(StateName::Motion, device->getZoneID());
        }
        getOrRegisterState(stateName);
      }

      // presence
      if ((input->m_inputType == BinaryInputIDPresence) ||
          (input->m_inputType == BinaryInputIDPresenceInDarkness)) {
        std::string stateName;
        if (input->m_targetGroupId >= GroupIDAppUserMin) {
          stateName = formatGroupName(StateName::Presence, input->m_targetGroupId);
        } else {
          stateName = formatZoneName(StateName::Presence, device->getZoneID());
        }
        getOrRegisterState(stateName);
      }

      // wind monitor
      if (input->m_inputType == BinaryInputIDWindDetector) {
        if (input->m_targetGroupId >= GroupIDAppUserMin) {
          getOrRegisterState(formatAppartmentStateName(StateName::Wind, input->m_targetGroupId));
        }
      }

      // rain monitor
      if (input->m_inputType == BinaryInputIDRainDetector) {
        if (input->m_targetGroupId >= GroupIDAppUserMin) {
          getOrRegisterState(formatAppartmentStateName(StateName::Rain, input->m_targetGroupId));
        }
      }

      // frost detector
      if (input->m_inputType == BinaryInputIDFrostDetector) {
        if (input->m_targetGroupId >= GroupIDAppUserMin) {
          getOrRegisterState(formatAppartmentStateName(StateName::Frost, input->m_targetGroupId));
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
          registerState(formatAppartmentStateName(StateName::Wind, group->getID()), true);
        }
        continue;
      }

      switch(group->getLastCalledScene()) {
      case SceneAbsent:
        absent = true;
        break;
      case ScenePanic:
        panic = true;
        break;
      case SceneAlarm:
        alarm = true;
        break;
      case SceneSleeping:
        sleeping = true;
        break;
      }
    } // groups for loop
  } // zones for loop

  // Restore apartment states from "lastCalledScene" ...
  boost::shared_ptr<State> state;
  if (lookupState(state, StateName::Presence)) {
    if ((absent == true) && (state->getState() == State_Inactive)) {
      state->setState(coJSScripting, "absent");
    } else if (absent == false) {
      state->setState(coJSScripting, "present");
    }
  }

  if (lookupState(state, StateName::Hibernation)) {
    if ((sleeping == true) && (state->getState() == State_Inactive)) {
      state->setState(coJSScripting, "sleeping");
    } else if (sleeping == false) {
      state->setState(coJSScripting, "awake");
    }
  } // hibernation state

  if (lookupState(state, StateName::Panic)) {
    if ((panic == true) && (state->getState() == State_Inactive)) {
      state->setState(coJSScripting, State_Active);
    } else if (panic == false) {
      state->setState(coJSScripting, State_Inactive);
    }
  } // panic state

  if (lookupState(state, StateName::Alarm)) {
    if ((alarm == true) && (state->getState() == State_Inactive)) {
      state->setState(coJSScripting, State_Active);
    } else if (alarm == false) {
      state->setState(coJSScripting, State_Inactive);
    }
  } // alarm state

  // Restore states if they have been registered before, that reads: if a persistent data file exists
  foreach (boost::shared_ptr<Cluster> cluster, m_apartment.getClusters()) {
    if (cluster->getStandardGroupID() == GroupIDGray) {
      loadPersistentState(StateType_Service, formatAppartmentStateName(StateName::Wind, cluster->getID()));
    }
    loadPersistentState(StateType_Group, formatAppartmentStateName(StateName::OperationLock, cluster->getID()));
  }
  loadPersistentState(StateType_Service, StateName::BuildingService);

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
    switch (sceneId) {
    case SceneAbsent:
      if (lookupState(state, StateName::Presence)) {
        state->setState(coSystem, "absent");
      }
      // #2561: auto-clear panic and fire
      if (lookupState(state, StateName::Panic)) {
        if (state->getState() == State_Active) {
          undoScene(0, 0, ScenePanic, coSystem);
        }
      }
      if (lookupState(state, StateName::Fire)) {
        if (state->getState() == State_Active) {
          undoScene(0, 0, SceneFire, coSystem);
        }
      }
      break;
    case ScenePresent:
      if (lookupState(state, StateName::Presence)) {
        state->setState(coSystem, "present");
      }
      break;
    case  SceneSleeping:
      if (lookupState(state, StateName::Hibernation)) {
        state->setState(coSystem, "sleeping");
      }
      break;
    case SceneWakeUp:
      if (lookupState(state, StateName::Hibernation)) {
        state->setState(coSystem, "awake");
      }
      break;
    case ScenePanic:
      if (lookupState(state, StateName::Panic)) {
        state->setState(coSystem, State_Active);
      }
      break;
    case SceneFire:
      if (lookupState(state, StateName::Fire)) {
        state->setState(coSystem, State_Active);
      }
      break;
    case SceneAlarm:
      if (lookupState(state, StateName::Alarm)) {
        state->setState(coSystem, State_Active);
      }
      break;
    case SceneAlarm2:
      if (lookupState(state, StateName::Alarm2)) {
        state->setState(coSystem, State_Active);
      }
      break;
    case SceneAlarm3:
      if (lookupState(state, StateName::Alarm3)) {
        state->setState(coSystem, State_Active);
      }
      break;
    case SceneAlarm4:
      if (lookupState(state, StateName::Alarm4)) {
        state->setState(coSystem, State_Active);
      }
      break;
    case SceneWindActive:
      state = getOrRegisterState(StateName::Wind);
      state->setState(coSystem, State_Active);
      break;
    case SceneWindInactive:
      state = getOrRegisterState(StateName::Wind);
      state->setState(coSystem, State_Inactive);
      break;
    case SceneRainActive:
      state = getOrRegisterState(StateName::Rain);
      state->setState(coSystem, State_Active);
      break;
    case SceneRainInactive:
      state = getOrRegisterState(StateName::Rain);
      state->setState(coSystem, State_Inactive);
      // cluster sensor and the global rain sensor could contradict each other,
      // and we are not resolving that conflict. Currently we support either
      // global or cluster-local states, but not both at the same time.
      // In the presence of cluster rain states inactive-rain on group0 serves
      // as a broadcast command only
      for (int grp = GroupIDAppUserMin; grp <= GroupIDAppUserMax; grp++) {
        if (lookupState(state, formatAppartmentStateName(StateName::Rain, grp))) {
          state->setState(coSystem, State_Inactive);
        }
      }
      break;
    }
  } else if (isAppUserGroup(groupId)) {
    /*
     * wind/rain can be apartment wide or only to one facade
     */
    switch (sceneId) {
    case SceneWindActive:
      state = getOrRegisterState(formatAppartmentStateName(StateName::Wind, groupId));
      state->setState(coSystem, State_Active);
      break;
    case SceneWindInactive:
      state = getOrRegisterState(formatAppartmentStateName(StateName::Wind, groupId));
      state->setState(coSystem, State_Inactive);
      break;
    case SceneRainActive:
      state = getOrRegisterState(formatAppartmentStateName(StateName::Rain, groupId));
      state->setState(coSystem, State_Active);
      break;
    case SceneRainInactive:
      state = getOrRegisterState(formatAppartmentStateName(StateName::Rain, groupId));
      state->setState(coSystem, State_Inactive);
      break;
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

  assert(groupId == 0);
  switch (sceneId) {
  case ScenePanic:
    if (lookupState(state, StateName::Panic)) {
      state->setState(coSystem, State_Inactive);
    }

    // #2561: auto-reset fire if panic was reset by a button
    if (lookupState(state, StateName::Fire)) {
      if ((dsuid != DSUID_NULL) && (callOrigin == coDsmApi) &&
          (state->getState() == State_Active)) {
        undoScene(0, 0, SceneFire, coSystem);
      } // valid dSID && dSM API origin && state == active
    }
    break;
  case SceneFire:
    if (lookupState(state, StateName::Fire)) {
      state->setState(coSystem, State_Inactive);
    }

    // #2561: auto-reset panic if fire was reset by a button
    if (lookupState(state, StateName::Panic)) {
      if ((dsuid != DSUID_NULL) && (callOrigin == coDsmApi)
          && (state->getState() == State_Active)) {
        undoScene(0, 0, ScenePanic, coSystem);
      } // valid dSID && dSM API origin && state == active
    }
    break;
  case SceneAlarm:
    if (lookupState(state, StateName::Alarm)) {
      state->setState(coSystem, State_Inactive);
    }
    break;
  case SceneAlarm2:
    if (lookupState(state, StateName::Alarm2)) {
      state->setState(coSystem, State_Inactive);
    }
    break;
  case SceneAlarm3:
    if (lookupState(state, StateName::Alarm3)) {
      state->setState(coSystem, State_Inactive);
    }
    break;
  case SceneAlarm4:
    if (lookupState(state, StateName::Alarm4)) {
      state->setState(coSystem, State_Inactive);
    }
    break;
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
      boost::shared_ptr<Device> pDev = m_raisedAtState->getProviderDevice();
      std::string val = m_properties.get("value");
      int iVal = strToIntDef(val, -1);
      if (iVal == 1) {
        pNode->setIntegerValue(pNode->getIntegerValue() + 1);
        _state.setState(coSystemBinaryInput, State_Active);
        _state.setOriginDeviceDSUID(pDev->getDSID());
      } else if (iVal == 2) {
        if (pNode->getIntegerValue() > 0) {
          pNode->setIntegerValue(pNode->getIntegerValue() - 1);
        }
        if (pNode->getIntegerValue() == 0) {
          _state.setState(coSystemBinaryInput, State_Inactive);
        }
        _state.setOriginDeviceDSUID(pDev->getDSID());
      }
    } // m_properties.has("value")
  } catch (ItemNotFoundException &ex) {}
}

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
      statename = formatGroupName(StateName::Motion, devInput->m_targetGroupId);
    } else {
      statename = formatZoneName(StateName::Motion, pDev->getZoneID());
    }
    boost::shared_ptr<State> state = getOrRegisterState(statename);
    stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
                            devInput->m_targetGroupId);
  }

  // presence
  if ((devInput->m_inputType == BinaryInputIDPresence) ||
      (devInput->m_inputType == BinaryInputIDPresenceInDarkness)) {
    if (devInput->m_targetGroupId >= GroupIDAppUserMin) {
      statename = formatGroupName(StateName::Presence, devInput->m_targetGroupId);
    } else {
      statename = formatZoneName(StateName::Presence, pDev->getZoneID());
    }
    boost::shared_ptr<State> state = getOrRegisterState(statename);
    stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
                            devInput->m_targetGroupId);
  }

  // smoke detector
  if (devInput->m_inputType == BinaryInputIDSmokeDetector) {
    boost::shared_ptr<State> state;
    if (lookupState(state, StateName::Fire)) {
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
      statename = formatAppartmentStateName(StateName::Wind, devInput->m_targetGroupId);
      state = getOrRegisterState(statename);
      stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
                              devInput->m_targetGroupId);
    } else if (lookupState(state, StateName::Wind)) {
      stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
                              devInput->m_targetGroupId);
    }
  }

  // rain monitor
  if (devInput->m_inputType == BinaryInputIDRainDetector) {
    boost::shared_ptr<State> state;
    // create state for a user group if it does not exist (new group?)
    if (devInput->m_targetGroupId >= GroupIDAppUserMin) {
      statename = formatAppartmentStateName(StateName::Rain, devInput->m_targetGroupId);
      state = getOrRegisterState(statename);
      stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
                              devInput->m_targetGroupId);
    } else if (lookupState(state, StateName::Rain)) {
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

  // frost detector
  if (devInput->m_inputType == BinaryInputIDFrostDetector) {
    boost::shared_ptr<State> state;
    if (lookupState(state, StateName::Frost)) {
      stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
          devInput->m_targetGroupId);
    }
  }

  // heating (on/off)
  if (devInput->m_inputType == BinaryInputIDHeatingSystem) {
    boost::shared_ptr<State> state;
    if (lookupState(state, StateName::HeatingSystem)) {
      stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
          devInput->m_targetGroupId);
    }
  }

  // heating mode: hot/cold
  if (devInput->m_inputType == BinaryInputIDHeatingSystemMode) {
    boost::shared_ptr<State> state;
    if (lookupState(state, StateName::HeatingSystemMode)) {
      stateBinaryInputGeneric(*state, devInput->m_targetGroupType,
          devInput->m_targetGroupId);
    }
  }

  // evaluate heating mode
  if ((devInput->m_inputType == BinaryInputIDHeatingSystem) ||
      (devInput->m_inputType == BinaryInputIDHeatingSystemMode)) {

    boost::shared_ptr<State> heating;
    boost::shared_ptr<State> heating_mode;

    if (lookupState(heating, StateName::HeatingSystem) &&
        lookupState(heating_mode, StateName::HeatingSystemMode))
    {
      std::string value; // value: {Off=0, Heat=1, Cold=2, Auto=3}
      if (heating->getState() == State_Inactive) {
        value = "0";
      } else if (heating->getState() == State_Active) {
        if (heating_mode->getState() == State_Active) {
          value = "1";
        } else if (heating_mode->getState() == State_Inactive) {
          value = "2";
        }
      }

      if (!value.empty()) {
        boost::shared_ptr<Event> event;
        event = boost::make_shared<Event>(EventName::HeatingModeSwitch);
        event->setProperty("value", value);
        event->setProperty(ef_callOrigin, intToString(coSystemBinaryInput));
        DSS::getInstance()->getEventQueue().pushEvent(event);
      }
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

  if (statename == StateName::Fire) {
    if (iVal == 1) {
      callScene(0, 0, SceneFire, coSystem);
    }
  }

  if (statename.substr(0, 4) == StateName::Rain) {
    if (iVal == 1) {
      callScene(0, groupId, SceneRainActive, coSystem);
    } else if (iVal == 2) {
      callScene(0, groupId, SceneRainInactive, coSystem);
    }
  }

  if (statename.substr(0, 4) == StateName::Wind) {
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
    } else if (m_evtName == EventName::ModelReady) {
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

    } else if (m_evtName == EventName::Sunshine) {
      std::string value = m_properties.get("value");
      std::string direction = m_properties.get("direction");

      // TODO: define algorithm for system states based on sun{shine,protection} inputs

    } else if (m_evtName == EventName::FrostProtection) {
      unsigned int value = strToInt(m_properties.get("value"));
      assert(value == State_Active || value == State_Inactive);
      getOrRegisterState(StateName::Frost)->setState(coDsmApi, value);

    } else if (m_evtName == EventName::HeatingModeSwitch) {
      boost::shared_ptr<State> state;
      state = getOrRegisterState(StateName::HeatingModeControl);
      unsigned int value = strToUInt(m_properties.get("value"));
      assert(value < state->getValueRangeSize());

      callOrigin_t origin = coDsmApi;

      if (m_properties.has("callOrigin")) {
        std::string s = m_properties.get("callOrigin");
        if (!s.empty()) {
          origin = (callOrigin_t)strToInt(s);
        }
      }
      state->setState(origin, value);

    } else if (m_evtName == EventName::BuildingService) {
      unsigned int value = strToInt(m_properties.get("value"));
      assert(value == State_Active || value == State_Inactive);
      getOrRegisterState(StateName::BuildingService)->setState(coDsmApi, value);

    }
  } catch(ItemNotFoundException& ex) {
    Logger::getInstance()->log("SystemState::run: item not found data model error", lsInfo);
  }
}

} // namespace dss
