/*
   Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

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

#include "action-execute.h"

#include <boost/make_shared.hpp>

#include "event/event_create.h"
#include "messages/vdc-messages.pb.h"
#include "model/apartment.h"
#include "model/group.h"
#include "model/state.h"
#include "model/zone.h"
#include "protobufjson.h"
#include "sceneaccess.h"
#include "util.h"
#include "systemcondition.h"

// durations to wait after each action (in milliseconds)
#define ACTION_DURATION_ZONE_SCENE      500
#define ACTION_DURATION_ZONE_UNDO_SCENE 1000
#define ACTION_DURATION_DEVICE_SCENE    1000
#define ACTION_DURATION_DEVICE_VALUE    1000
#define ACTION_DURATION_DEVICE_BLINK    1000
#define ACTION_DURATION_ZONE_BLINK      500
#define ACTION_DURATION_CUSTOM_EVENT    100
#define ACTION_DURATION_URL             100
#define ACTION_DURATION_STATE_CHANGE    100

namespace dss {

ActionExecute::ActionExecute(const Properties &properties) : m_properties(properties) {}

std::string ActionExecute::getActionName(PropertyNodePtr _actionNode) {
  std::string action_name;
  PropertyNode *parent1 = _actionNode->getParentNode();
  if (parent1 != NULL) {
    PropertyNode *parent2 = parent1->getParentNode();
    if (parent2 != NULL) {
      PropertyNodePtr name = parent2->getPropertyByName("name");
      if (name != NULL) {
        action_name = name->getAsString();
      }
    }
  }
  return action_name;
}

void ActionExecute::executeZoneScene(PropertyNodePtr _actionNode) {
  try {
    int zoneId;
    int groupId;
    int sceneId;
    bool forceFlag = false;
    SceneAccessCategory sceneAccess = SAC_UNKNOWN;

    PropertyNodePtr oZoneNode = _actionNode->getPropertyByName("zone");
    if (oZoneNode == NULL) {
      Logger::getInstance()->log("ActionExecute:: "
                                 "executeZoneScene - missing zone parameter", lsError);
      return;
    } else {
      zoneId = oZoneNode->getIntegerValue();
    }

    PropertyNodePtr oGroupNode = _actionNode->getPropertyByName("group");
    if (oGroupNode == NULL) {
      Logger::getInstance()->log("ActionExecute:: "
                                 "executeZoneScene - missing group parameter", lsError);
      return;
    } else {
      groupId = oGroupNode->getIntegerValue();
    }

    PropertyNodePtr oSceneNode = _actionNode->getPropertyByName("scene");
    if (oSceneNode == NULL) {
      Logger::getInstance()->log("ActionExecute::"
                                 "executeZoneScene: missing scene parameter", lsError);
      return;
    } else {
      sceneId = oSceneNode->getIntegerValue();
    }

    PropertyNodePtr oForceNode = _actionNode->getPropertyByName("force");
    if (oForceNode != NULL) {
      forceFlag = oForceNode->getBoolValue();
    }

    PropertyNodePtr oCategoryNode = _actionNode->getPropertyByName("category");
    if (oCategoryNode != NULL) {
      sceneAccess = SceneAccess::stringToCategory(oCategoryNode->getStringValue());
    }

    boost::shared_ptr<Zone> zone;
    if (DSS::hasInstance()) {
      zone = DSS::getInstance()->getApartment().getZone(zoneId);
      boost::shared_ptr<Group> group = zone->getGroup(groupId);
      group->callScene(coJSScripting, sceneAccess, sceneId, "", forceFlag);
    }
  } catch(SceneAccessException& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeZoneScene: execution not allowed: " +
                               std::string(e.what()));
    if (DSS::hasInstance()) {
      boost::shared_ptr<Event> pEvent;
      pEvent = createActionDenied("zone-scene",
                                  getActionName(_actionNode),
                                  m_properties.get("source-name", ""),
                                  e.what());

      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
  } catch (std::runtime_error& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeZoneScene: could not call scene on zone " +
                               std::string(e.what()));
  }
}

void ActionExecute::executeZoneUndoScene(PropertyNodePtr _actionNode) {
  try {
    int zoneId;
    int groupId;
    int sceneId;
    SceneAccessCategory sceneAccess = SAC_UNKNOWN;

    PropertyNodePtr oZoneNode = _actionNode->getPropertyByName("zone");
    if (oZoneNode == NULL) {
      Logger::getInstance()->log("ActionExecute:: "
                                 "executeZoneUndoScene - missing zone parameter", lsError);
      return;
    } else {
      zoneId = oZoneNode->getIntegerValue();
    }

    PropertyNodePtr oGroupNode = _actionNode->getPropertyByName("group");
    if (oGroupNode == NULL) {
      Logger::getInstance()->log("ActionExecute:: "
                                 "executeZoneUndoScene - missing group parameter", lsError);
      return;
    } else {
      groupId = oGroupNode->getIntegerValue();
    }

    PropertyNodePtr oSceneNode = _actionNode->getPropertyByName("scene");
    if (oSceneNode == NULL) {
      Logger::getInstance()->log("ActionExecute::"
                                 "executeZoneUndoScene: missing scene parameter", lsError);
      return;
    } else {
      sceneId = oSceneNode->getIntegerValue();
    }

    PropertyNodePtr oCategoryNode = _actionNode->getPropertyByName("category");
    if (oCategoryNode != NULL) {
      sceneAccess = SceneAccess::stringToCategory(oCategoryNode->getStringValue());
    }

    boost::shared_ptr<Zone> zone;
    if (DSS::hasInstance()) {
      zone = DSS::getInstance()->getApartment().getZone(zoneId);
      boost::shared_ptr<Group> group = zone->getGroup(groupId);
      group->undoScene(coJSScripting, sceneAccess, sceneId, "");
    }
  } catch(SceneAccessException& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeZoneUndoScene: execution not allowed: " +
                               std::string(e.what()));
    if (DSS::hasInstance()) {
      boost::shared_ptr<Event> pEvent;
      pEvent = createActionDenied("zone-undo-scene",
                                  getActionName(_actionNode),
                                  m_properties.get("source-name", ""),
                                  e.what());
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
  } catch (std::runtime_error& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeZoneUndoScene: could not undo scene on zone " +
                               std::string(e.what()));
  }
}

boost::shared_ptr<Device> ActionExecute::getDeviceFromNode(PropertyNodePtr _actionNode) {
  boost::shared_ptr<Device> device;
  PropertyNodePtr oDeviceNode = _actionNode->getPropertyByName("dsuid");
  if (oDeviceNode == NULL) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeDeviceScene: could call scene on device - missing dsuid",
                               lsError);
    return device;
  }

  std::string dsuidStr = oDeviceNode->getStringValue();
  if (dsuidStr.empty()) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeDeviceScene: could call scene on device - empty dsuid",
                               lsError);
    return device;
  }

  dsuid_t deviceDSID = str2dsuid(dsuidStr);

  if (DSS::hasInstance()) {
    device = DSS::getInstance()->getApartment().getDeviceByDSID(deviceDSID);
  }
  return device;
}

void ActionExecute::executeDeviceScene(PropertyNodePtr _actionNode) {
  try {
    int sceneId;
    bool forceFlag = false;
    SceneAccessCategory sceneAccess = SAC_UNKNOWN;

    PropertyNodePtr oSceneNode = _actionNode->getPropertyByName("scene");
    if (oSceneNode == NULL) {
      Logger::getInstance()->log("ActionExecute::"
                                 "executeDeviceScene: missing scene parameter", lsError);
      return;
    } else {
      sceneId = oSceneNode->getIntegerValue();
    }

    PropertyNodePtr oForceNode = _actionNode->getPropertyByName("force");
    if (oForceNode != NULL) {
      forceFlag = oForceNode->getBoolValue();
    }

    PropertyNodePtr oCategoryNode = _actionNode->getPropertyByName("category");
    if (oCategoryNode != NULL) {
      sceneAccess = SceneAccess::stringToCategory(oCategoryNode->getStringValue());
    }

    boost::shared_ptr<Device> target = getDeviceFromNode(_actionNode);
    if (target == NULL) {
      Logger::getInstance()->log("ActionExecute::"
                                 "executeDeviceScene: could not call scene on device - device "
                                 "was not found", lsError);
      return;
    }

    target->callScene(coJSScripting, sceneAccess, sceneId, "", forceFlag);

  } catch(SceneAccessException& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeDeviceScene: execution not allowed: " +
                               std::string(e.what()));
    if (DSS::hasInstance()) {
      boost::shared_ptr<Event> pEvent;
      pEvent = createActionDenied("device-scene",
                                  getActionName(_actionNode),
                                  m_properties.get("source-name", ""),
                                  e.what());
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
  } catch (std::runtime_error& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeDeviceScene: could not call scene on zone " +
                               std::string(e.what()));
  }
}

void ActionExecute::executeDeviceChannelValue(PropertyNodePtr _actionNode) {
  try {
    boost::shared_ptr<Device> target = getDeviceFromNode(_actionNode);
    if (target == NULL) {
      Logger::getInstance()->log("ActionExecute::"
                                 "executeDeviceChannelValue: could not set value on device - device "
                                 "was not found", lsError);
      return;
    }

    struct channel {
      int id;
      int index;
      int value;
    };
    std::vector<channel> vChannels;

    int i = 0;
    while (true) {
      auto oChannelIdNode = _actionNode->getPropertyByName(std::string("channelid-") + intToString(i));
      auto oChannelValueNode = _actionNode->getPropertyByName(std::string("channelvalue-") + intToString(i));
      if ((oChannelIdNode == NULL) || (oChannelValueNode == NULL)) {
        break;
      }
      i++;
      int channelId = oChannelIdNode->getIntegerValue();
      int channelIndex = target->getOutputChannelIndex(channelId);
      vChannels.push_back(channel{channelId, channelIndex, oChannelValueNode->getIntegerValue()});
    }

    auto numNonApplyChannels = static_cast <int>(vChannels.size()) - 1;

    if (numNonApplyChannels < 0) {
      Logger::getInstance()->log("ActionExecute::"
                                 "executeDeviceChannelValue: no channel settings found", lsWarning);
      return;
    }

    for (int i = 0; i < numNonApplyChannels; ++i) {
      auto ch = vChannels[i];
      target->setDeviceOutputChannelValue(ch.index, getOutputChannelSize(ch.id), ch.value, false);
    }
    auto ch = vChannels[numNonApplyChannels];
    target->setDeviceOutputChannelValue(ch.index, getOutputChannelSize(ch.id), ch.value, true);

  } catch(SceneAccessException& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeDeviceChannelValue: execution not allowed: " +
                               std::string(e.what()));
    std::string action_name = getActionName(_actionNode);
    boost::shared_ptr<Event> pEvent;
    pEvent.reset(new Event(EventName::ExecutionDenied));
    pEvent->setProperty("action-type", "device-moc-value");
    pEvent->setProperty("action-name", action_name);
    pEvent->setProperty("source-name", m_properties.get("source-name", ""));
    pEvent->setProperty("reason", std::string(e.what()));
    if (DSS::hasInstance()) {
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
  } catch (std::runtime_error& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeDeviceChannelValue: could not set value on device " +
                               std::string(e.what()));
  }
}

void ActionExecute::executeDeviceValue(PropertyNodePtr _actionNode) {
  try {
    SceneAccessCategory sceneAccess = SAC_UNKNOWN;

    PropertyNodePtr oValueNode = _actionNode->getPropertyByName("value");
    if (oValueNode == NULL) {
      Logger::getInstance()->log("ActionExecute::"
                                 "executeDeviceValue: could not set value on device - missing "
                                 "value", lsError);
      return;
    }

    boost::shared_ptr<Device> target = getDeviceFromNode(_actionNode);
    if (target == NULL) {
      Logger::getInstance()->log("ActionExecute::"
                                 "executeDeviceValue: could not set value on device - device "
                                 "was not found", lsError);
      return;
    }

    PropertyNodePtr oCategoryNode = _actionNode->getPropertyByName("category");
    if (oCategoryNode != NULL) {
      sceneAccess = SceneAccess::stringToCategory(oCategoryNode->getStringValue());
    }

    target->setValue(coJSScripting,
                     sceneAccess,
                     oValueNode->getIntegerValue(), "");

  } catch(SceneAccessException& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeDeviceValue: execution not allowed: " +
                               std::string(e.what()));
    std::string action_name = getActionName(_actionNode);
    boost::shared_ptr<Event> pEvent;
    pEvent.reset(new Event(EventName::ExecutionDenied));
    pEvent->setProperty("action-type", "device-value");
    pEvent->setProperty("action-name", action_name);
    pEvent->setProperty("source-name", m_properties.get("source-name", ""));
    pEvent->setProperty("reason", std::string(e.what()));
    if (DSS::hasInstance()) {
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
  } catch (std::runtime_error& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeDeviceValue: could not set value on device " +
                               std::string(e.what()));
  }
}

void ActionExecute::executeDeviceBlink(PropertyNodePtr _actionNode) {
  try {
    SceneAccessCategory sceneAccess = SAC_UNKNOWN;

    boost::shared_ptr<Device> target = getDeviceFromNode(_actionNode);
    if (target == NULL) {
      Logger::getInstance()->log("ActionExecute::"
                                 "executeDeviceBlink: could not blink device - device "
                                 "was not found", lsError);
      return;
    }

    PropertyNodePtr oCategoryNode = _actionNode->getPropertyByName("category");
    if (oCategoryNode != NULL) {
      sceneAccess = SceneAccess::stringToCategory(oCategoryNode->getStringValue());
    }

    target->callScene(coJSScripting, sceneAccess, SceneImpulse, "", true);

  } catch(SceneAccessException& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeDeviceBlink: execution not allowed: " +
                               std::string(e.what()));
    std::string action_name = getActionName(_actionNode);
    boost::shared_ptr<Event> pEvent;
    pEvent.reset(new Event(EventName::ExecutionDenied));
    pEvent->setProperty("action-type", "device-blink");
    pEvent->setProperty("action-name", action_name);
    pEvent->setProperty("source-name", m_properties.get("source-name", ""));
    pEvent->setProperty("reason", std::string(e.what()));
    if (DSS::hasInstance()) {
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
  } catch (std::runtime_error& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeDeviceBlink: could not blink device " +
                               std::string(e.what()));
  }
}

void ActionExecute::executeDeviceAction(PropertyNodePtr _actionNode) {
  try {
    std::string id;
    PropertyNodePtr idNode = _actionNode->getPropertyByName("id");
    if (idNode == NULL) {
      Logger::getInstance()->log("ActionExecute::"
                                 "executeDeviceAction: missing parameter id", lsError);
      return;
    } else {
      id = idNode->getStringValue();
    }

    boost::shared_ptr<Device> target = getDeviceFromNode(_actionNode);
    if (target == NULL) {
      Logger::getInstance()->log("ActionExecute::"
                                 "executeDeviceScene: could not call scene on device - device "
                                 "was not found", lsError);
      return;
    }

    PropertyNodePtr paramsNode = _actionNode->getPropertyByName("params");
    vdcapi::PropertyElement parsedParamsElement;
    const vdcapi::PropertyElement* paramsElement = &vdcapi::PropertyElement::default_instance();
    if (paramsNode && !paramsNode->getStringValue().empty()) {
      parsedParamsElement = ProtobufToJSon::jsonToElement(paramsNode->getStringValue());
      paramsElement = &parsedParamsElement;
    }

    target->callAction(id, *paramsElement);

  } catch(SceneAccessException& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeDeviceAction: execution not allowed: " +
                               std::string(e.what()));
    if (DSS::hasInstance()) {
      boost::shared_ptr<Event> pEvent;
      pEvent = createActionDenied("device-action",
                                  getActionName(_actionNode),
                                  m_properties.get("source-name", ""),
                                  e.what());
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
  } catch (std::runtime_error& e) {
    Logger::getInstance()->log("ActionExecute::executeDeviceAction failed: "
                               + std::string(e.what()));
  }
}

void ActionExecute::executeZoneBlink(PropertyNodePtr _actionNode) {
  try {
    int zoneId;
    int groupId;
    SceneAccessCategory sceneAccess = SAC_UNKNOWN;

    PropertyNodePtr oZoneNode = _actionNode->getPropertyByName("zone");
    if (oZoneNode == NULL) {
      Logger::getInstance()->log("ActionExecute::"
                                 "executeZoneBlink - missing zone parameter", lsError);
      return;
    } else {
      zoneId = oZoneNode->getIntegerValue();
    }

    PropertyNodePtr oGroupNode = _actionNode->getPropertyByName("group");
    if (oGroupNode == NULL) {
      Logger::getInstance()->log("ActionExecute::"
                                 "executeZoneBlink - missing group parameter", lsError);
      return;
    } else {
      groupId = oGroupNode->getIntegerValue();
    }

    PropertyNodePtr oCategoryNode = _actionNode->getPropertyByName("category");
    if (oCategoryNode != NULL) {
      sceneAccess = SceneAccess::stringToCategory(oCategoryNode->getStringValue());
    }

    if (DSS::hasInstance()) {
      boost::shared_ptr<Zone> zone;
      zone = DSS::getInstance()->getApartment().getZone(zoneId);
      boost::shared_ptr<Group> group = zone->getGroup(groupId);
      group->callScene(coJSScripting, sceneAccess, SceneImpulse, "", true);
    }
  } catch(SceneAccessException& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeZoneBlink: execution not allowed: " +
                               std::string(e.what()));
    std::string action_name = getActionName(_actionNode);
    boost::shared_ptr<Event> pEvent;
    pEvent.reset(new Event(EventName::ExecutionDenied));
    pEvent->setProperty("action-type", "zone-blink");
    pEvent->setProperty("action-name", action_name);
    pEvent->setProperty("source-name", m_properties.get("source-name", ""));
    pEvent->setProperty("reason", std::string(e.what()));
    if (DSS::hasInstance()) {
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }
  } catch (std::runtime_error& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeZoneBlink: could not blink zone " +
                               std::string(e.what()));
  }
}

void ActionExecute::executeCustomEvent(PropertyNodePtr _actionNode) {
  PropertyNodePtr oEventNode = _actionNode->getPropertyByName("event");
  if (oEventNode == NULL) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeCustomEvent - missing event parameter", lsError);
    return;
  }

  boost::shared_ptr<Event> evt = boost::make_shared<Event>("highlevelevent");
  evt->setProperty("id", oEventNode->getStringValue());
  evt->setProperty("source-name", getActionName(_actionNode));
  if (DSS::hasInstance()) {
    DSS::getInstance()->getEventQueue().pushEvent(evt);
  }
}

void ActionExecute::executeURL(PropertyNodePtr _actionNode) {
  PropertyNodePtr oUrlNode =  _actionNode->getPropertyByName("url");
  if (oUrlNode == NULL) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeURL: missing url parameter", lsError);
    return;
  }

  std::string oUrl = unescapeHTML(oUrlNode->getAsString());
  if (oUrl.empty()) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeURL: empty url parameter", lsError);
    return;
  }

  Logger::getInstance()->log("ActionExecute::"
                             "executeURL: " + oUrl);

  boost::shared_ptr<HttpClient> http = boost::make_shared<HttpClient>();
  long code = http->get(oUrl, NULL, true); // do not check certificates
  std::ostringstream out;
  out << code;

  Logger::getInstance()->log("ActionExecute::"
                             "executeURL: request to " + oUrl + " returned HTTP code " +
                             out.str());
}

void ActionExecute::executeStateChange(PropertyNodePtr _actionNode) {
  PropertyNodePtr oStateNode = _actionNode->getPropertyByName("statename");
  if (oStateNode == NULL) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeStateChange - missing statename parameter", lsError);
    return;
  }

  boost::shared_ptr<State> pState;
  try {
    pState = DSS::getInstance()->getApartment().getNonScriptState(oStateNode->getStringValue());
  } catch(ItemNotFoundException& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeStateChange - state '" + oStateNode->getStringValue() + "' does not exist", lsError);
    return;
  }

  if (pState->getType() == StateType_Device) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeStateChange - cannot modify states of type \'device\'", lsError);
    return;
  }

  if (pState->getType() == StateType_Circuit) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeStateChange - cannot modify dsm power states", lsError);
    return;
  }

  PropertyNodePtr oValueNode = _actionNode->getPropertyByName("value");
  PropertyNodePtr oSValueNode = _actionNode->getPropertyByName("state");
  if (oValueNode == NULL && oSValueNode == NULL) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeStateChange: missing value or state parameter", lsError);
    return;
  }

  if (oValueNode && oValueNode->getValueType() == vTypeInteger) {
    pState->setState(coJSScripting, oValueNode->getIntegerValue());
  } else if (oSValueNode && oSValueNode->getValueType() == vTypeString) {
    pState->setState(coJSScripting, oSValueNode->getStringValue());
  } else {
    Logger::getInstance()->log("ActionExecute::"
                               "executeStateChange: wrong data type for value or state parameter", lsError);
  }
}

void ActionExecute::executeAddonStateChange(PropertyNodePtr _actionNode) {

  PropertyNodePtr oScriptNode = _actionNode->getPropertyByName("addon-id");
  if (oScriptNode == NULL) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeAddonStateChange - missing addon-id parameter", lsError);
    return;
  }

  PropertyNodePtr oStateNode = _actionNode->getPropertyByName("statename");
  if (oStateNode == NULL) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeAddonStateChange - missing statename parameter", lsError);
    return;
  }

  boost::shared_ptr<State> pState;
  try {
    pState = DSS::getInstance()->getApartment().getState(StateType_Script,
                                                         oScriptNode->getStringValue(), oStateNode->getStringValue());
  } catch(ItemNotFoundException& e) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeAddonStateChange - state '" + oStateNode->getStringValue() + "' does not exist", lsError);
    return;
  }

  PropertyNodePtr oValueNode = _actionNode->getPropertyByName("value");
  PropertyNodePtr oSValueNode = _actionNode->getPropertyByName("state");
  if (oValueNode == NULL && oSValueNode == NULL) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeAddonStateChange: missing value or state parameter", lsError);
    return;
  }

  if (oValueNode && oValueNode->getValueType() == vTypeInteger) {
    pState->setState(coJSScripting, oValueNode->getIntegerValue());
  } else if (oSValueNode && oSValueNode->getValueType() == vTypeString) {
    pState->setState(coJSScripting, oSValueNode->getStringValue());
  } else {
    Logger::getInstance()->log("ActionExecute::"
                               "executeAddonStateChange: wrong data type for value or state parameter", lsError);
  }
}

void ActionExecute::executeHeatingMode(PropertyNodePtr _actionNode) {
  PropertyNodePtr oZoneNode = _actionNode->getPropertyByName("zone");
  if (oZoneNode == NULL) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeAddonStateChange - missing zone parameter", lsError);
    return;
  }

  PropertyNodePtr oModeNode = _actionNode->getPropertyByName("mode");
  if (oModeNode == NULL) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeAddonStateChange - missing mode parameter", lsError);
    return;
  }

  PropertyNodePtr oResetNode = _actionNode->getPropertyByName("reset");
  bool reset = false;
  if (oResetNode != NULL) {
    reset = oResetNode->getBoolValue();
  }

  boost::shared_ptr<Event> evt = boost::make_shared<Event>("heating-controller.operation-mode");
  evt->setProperty("actions", reset ? "resetOperationMode" : "setOperationMode");
  evt->setProperty("zoneID", oZoneNode->getAsString());
  evt->setProperty("operationMode", oModeNode->getAsString());
  if (DSS::hasInstance()) {
    DSS::getInstance()->getEventQueue().pushEvent(evt);
  }
}

unsigned int ActionExecute::executeOne(PropertyNodePtr _actionNode) {
  Logger::getInstance()->log("ActionExecute::"
                             "executeOne: " + _actionNode->getDisplayName(), lsDebug);

  PropertyNodePtr oTypeNode = _actionNode->getPropertyByName("type");
  if (oTypeNode != NULL) {
    std::string sActionType = oTypeNode->getStringValue();
    if (!sActionType.empty()) {
      if (sActionType == "zone-scene") {
        executeZoneScene(_actionNode);
        return ACTION_DURATION_ZONE_SCENE;
      } else if (sActionType == "undo-zone-scene") {
        executeZoneUndoScene(_actionNode);
        return ACTION_DURATION_ZONE_UNDO_SCENE;
      } else if (sActionType == "device-scene") {
        executeDeviceScene(_actionNode);
        return ACTION_DURATION_DEVICE_SCENE;
      } else if (sActionType == "device-value") {
        executeDeviceValue(_actionNode);
        return ACTION_DURATION_DEVICE_VALUE;
      } else if (sActionType == "device-moc-value") {
        executeDeviceChannelValue(_actionNode);
        return ACTION_DURATION_DEVICE_VALUE;
      } else if (sActionType == "device-blink") {
        executeDeviceBlink(_actionNode);
        return ACTION_DURATION_DEVICE_BLINK;
      } else if (sActionType == "device-action") {
        executeDeviceAction(_actionNode);
        return ACTION_DURATION_DEVICE_BLINK;
      } else if (sActionType == "zone-blink") {
        executeZoneBlink(_actionNode);
        return ACTION_DURATION_ZONE_BLINK;
      } else if (sActionType == "custom-event") {
        executeCustomEvent(_actionNode);
        return ACTION_DURATION_CUSTOM_EVENT;
      } else if (sActionType == "url") {
        executeURL(_actionNode);
        return ACTION_DURATION_URL;
      } else if (sActionType == "change-state") {
        executeStateChange(_actionNode);
        return ACTION_DURATION_STATE_CHANGE;
      } else if (sActionType == "change-addon-state") {
        executeAddonStateChange(_actionNode);
        return ACTION_DURATION_STATE_CHANGE;
      } else if (sActionType == "heating-mode") {
        executeHeatingMode(_actionNode);
        return ACTION_DURATION_STATE_CHANGE;
      }

    } else {
      Logger::getInstance()->log("ActionExecute::"
                                 "type is not available", lsError);
    }
  } else {
    Logger::getInstance()->log("ActionExecute::"
                               "type node is not available", lsError);

  }

  return 0;
}

void ActionExecute::executeStep(std::vector<PropertyNodePtr> _actionNodes) {
  for (size_t i = 0; i < _actionNodes.size(); i++) {
    PropertyNodePtr oActionNode = _actionNodes.at(i);
    unsigned int lWaitingTime = 100;
    if (oActionNode != NULL) {
      lWaitingTime = executeOne(oActionNode);
    } else {
      Logger::getInstance()->log("ActionExecute::"
                                 "action node in executeStep is not available", lsError);
    }
    if (lWaitingTime > 0) {
      sleepMS(lWaitingTime);
    }
  }
}

void ActionExecute::execute(std::string _path) {
  if (_path.empty()) {
    return;
  }
  if (!DSS::hasInstance()) {
    return;
  }
  PropertyNodePtr oBaseActionNode = DSS::getInstance()->getPropertySystem().getProperty(_path + "/actions");
  if (oBaseActionNode == NULL) {
    Logger::getInstance()->log("ActionExecute: no "
                               "actionNodes in path " + _path, lsError);
    return;
  }
  if (oBaseActionNode->getChildCount() == 0) {
    Logger::getInstance()->log("EventInterpreterPluginActionExecute: no "
                               "actionSubnodes in path " + _path, lsError);
    return;
  }

  std::vector<int> oDelay;
  std::vector<int> temp;
  temp.push_back(0);
  for (int i = 0; i < oBaseActionNode->getChildCount(); i++) {
    PropertyNodePtr cn = oBaseActionNode->getChild(i)->getPropertyByName("delay");
    if (cn == NULL) {
      continue;
    }
    try {
      temp.push_back(cn->getIntegerValue());
    } catch (PropertyTypeMismatch& e) {
      Logger::getInstance()->log("EventInterpreterPluginActionExecute: wrong "
                                 "parameter type for delay, path " + _path, lsError);
    }
  }
  sort(temp.begin(), temp.end());
  temp.erase(unique(temp.begin(), temp.end()), temp.end());
  oDelay.swap(temp);

  if (oDelay.size() > 1) {
    Logger::getInstance()->log("ActionExecute: delay "
                               "parameter present, execution will be fragmented", lsDebug);

    for (size_t s = 0; s < oDelay.size(); s++) {
      boost::shared_ptr<Event> evt = boost::make_shared<Event>("action_execute");
      evt->applyProperties(m_properties);
      evt->setProperty("path", _path);
      evt->setProperty("delay", intToString(oDelay.at(s)));
      evt->setProperty(EventProperty::Time, "+" + intToString(oDelay.at(s)));
      if (DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(evt);
      } else {
        return;
      }
    }
  } else {
    if (m_properties.has("ignoreConditions")) {
      Logger::getInstance()->log("ActionExecute: "
                                 "condition check disabled in path" + _path);
    }
    else if (!checkSystemCondition(_path)) {
      Logger::getInstance()->log("ActionExecute: "
                                 "condition check failed in path " + _path);
      return;
    }
    std::vector<PropertyNodePtr> oArrayActions;
    for (int i = 0; i < oBaseActionNode->getChildCount(); i++) {
      oArrayActions.push_back(oBaseActionNode->getChild(i));
    }
    executeStep(oArrayActions);
    if (DSS::hasInstance()) {
      DSS::getInstance()->getPropertySystem().setStringValue(_path +
                                                             "/lastExecuted",
                                                             DateTime());
    }
  }
}

std::vector<PropertyNodePtr> ActionExecute::filterActionsWithDelay(PropertyNodePtr _actionNode, int _delayValue) {
  std::vector<PropertyNodePtr> oResultArray;
  for (int i = 0; i < _actionNode->getChildCount(); i++) {
    PropertyNodePtr delayParent = _actionNode->getChild(i);
    if (delayParent != NULL) {
      PropertyNodePtr delayNode = delayParent->getPropertyByName("delay");
      if (delayNode != NULL) {
        try {
          int delayNodeValue = delayNode->getIntegerValue();
          if (delayNodeValue == _delayValue) {
            oResultArray.push_back(delayParent);
          }
        } catch (PropertyTypeMismatch& e) {}
      }
    }
  }
  return oResultArray;
}

void ActionExecute::executeWithDelay(std::string _path,
                                     std::string _delay) {
  if (!DSS::hasInstance()) {
    return;
  }
  PropertyNodePtr oBaseActionNode =
    DSS::getInstance()->getPropertySystem().getProperty(_path + "/actions");
  if (oBaseActionNode == NULL) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeWithDelay: no actionNodes in path " + _path, lsError);
    return;
  }
  if (oBaseActionNode->getChildCount() == 0) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeWithDelay: no actionSubnodes in path " + _path, lsError);
    return;
  }

  if (m_properties.has("ignoreConditions")) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeWithDelay: condition check disabled in path" + _path);
  }
  else if (!checkSystemCondition(_path)) {
    Logger::getInstance()->log("ActionExecute::"
                               "executeWithDelay: condition check failed in path " + _path,
                               lsError);
    return;
  }

  std::vector<PropertyNodePtr> delayedActions =
    filterActionsWithDelay(oBaseActionNode, strToInt(_delay));
  if (delayedActions.size() > 0) {
    executeStep(delayedActions);
  }
}

} // namespace dss
