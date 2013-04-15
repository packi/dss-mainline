/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#include "config.h"

#include "eventinterpretersystemplugins.h"
#include "logger.h"
#include "base.h"
#include "dss.h"
#include "model/set.h"
#include "model/zone.h"
#include "model/group.h"
#include "model/device.h"
#include "model/apartment.h"
#include "model/state.h"
#include "propertysystem.h"
#include "systemcondition.h"
#include "security/security.h"

#ifdef HAVE_CURL
#include "url.h"
#endif

#include <set>

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

  SystemEventActionExecute::SystemEventActionExecute() : SystemEvent() {
  }

  SystemEventActionExecute::~SystemEventActionExecute() {
  }

  void SystemEventActionExecute::executeZoneScene(PropertyNodePtr _actionNode) {
    try {
      int zoneId;
      int groupId;
      int sceneId;
      bool forceFlag = false;
      SceneAccessCategory sceneAccess = SAC_UNKNOWN;

      PropertyNodePtr oZoneNode = _actionNode->getPropertyByName("zone");
      if (oZoneNode == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute:: "
                "executeZoneScene - missing zone parameter", lsError);
        return;
      } else {
        zoneId = oZoneNode->getIntegerValue();
      }

      PropertyNodePtr oGroupNode = _actionNode->getPropertyByName("group");
      if (oGroupNode == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute:: "
                "executeZoneScene - missing group parameter", lsError);
        return;
      } else {
        groupId = oGroupNode->getIntegerValue();
      }

      PropertyNodePtr oSceneNode = _actionNode->getPropertyByName("scene");
      if (oSceneNode == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute::"
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
        group->callScene(coJSScripting, sceneAccess, sceneId, forceFlag);
      }
    } catch(SceneAccessException& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
                    "executeZoneScene: execution not allowed: " +
                    std::string(e.what()));
      std::string action_name = getActionName(_actionNode);
      boost::shared_ptr<Event> pEvent;
      pEvent.reset(new Event("executionDenied"));
      pEvent->setProperty("action-type", "zone-scene");
      pEvent->setProperty("action-name", action_name);
      pEvent->setProperty("source-name", m_properties.get("source-name", ""));
      pEvent->setProperty("reason", std::string(e.what()));
      if (DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(pEvent);
      }
    } catch (std::runtime_error& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeZoneScene: could not call scene on zone " +
              std::string(e.what()));
    }
  }

  void SystemEventActionExecute::executeZoneUndoScene(PropertyNodePtr _actionNode) {
    try {
      int zoneId;
      int groupId;
      int sceneId;
      SceneAccessCategory sceneAccess = SAC_UNKNOWN;

      PropertyNodePtr oZoneNode = _actionNode->getPropertyByName("zone");
      if (oZoneNode == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute:: "
                "executeZoneUndoScene - missing zone parameter", lsError);
        return;
      } else {
        zoneId = oZoneNode->getIntegerValue();
      }

      PropertyNodePtr oGroupNode = _actionNode->getPropertyByName("group");
      if (oGroupNode == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute:: "
                "executeZoneUndoScene - missing group parameter", lsError);
        return;
      } else {
        groupId = oGroupNode->getIntegerValue();
      }

      PropertyNodePtr oSceneNode = _actionNode->getPropertyByName("scene");
      if (oSceneNode == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute::"
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
        group->undoScene(coJSScripting, sceneAccess, sceneId);
      }
    } catch(SceneAccessException& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
                    "executeZoneUndoScene: execution not allowed: " +
                    std::string(e.what()));
      std::string action_name = getActionName(_actionNode);
      boost::shared_ptr<Event> pEvent;
      pEvent.reset(new Event("executionDenied"));
      pEvent->setProperty("action-type", "zone-undo-scene");
      pEvent->setProperty("action-name", action_name);
      pEvent->setProperty("source-name", m_properties.get("source-name", ""));
      pEvent->setProperty("reason", std::string(e.what()));
      if (DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(pEvent);
      }
    } catch (std::runtime_error& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeZoneUndoScene: could not undo scene on zone " +
              std::string(e.what()));
    }
  }

  boost::shared_ptr<Device> SystemEventActionExecute::getDeviceFromNode(PropertyNodePtr _actionNode) {
      boost::shared_ptr<Device> device;
      PropertyNodePtr oDeviceNode = _actionNode->getPropertyByName("dsid");
      if (oDeviceNode == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute::"
              "executeDeviceScene: could call scene on device - missing dsid",
              lsError);
        return device;
      }

      std::string dsidStr = oDeviceNode->getStringValue();
      if (dsidStr.empty()) {
        Logger::getInstance()->log("SystemEventActionExecute::"
              "executeDeviceScene: could call scene on device - empty dsid",
              lsError);
        return device;
      }

      dss_dsid_t deviceDSID = dss_dsid_t::fromString(dsidStr);

      if (DSS::hasInstance()) {
        device = DSS::getInstance()->getApartment().getDeviceByDSID(deviceDSID);
      }
      return device;
  }

  void SystemEventActionExecute::executeDeviceScene(PropertyNodePtr _actionNode) {
    try {
      int sceneId;
      bool forceFlag = false;
      SceneAccessCategory sceneAccess = SAC_UNKNOWN;

      PropertyNodePtr oSceneNode = _actionNode->getPropertyByName("scene");
      if (oSceneNode == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute::"
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
        Logger::getInstance()->log("SystemEventActionExecute::"
                 "executeDeviceScene: could not call scene on device - device "
                 "was not found", lsError);
        return;
      }

      target->callScene(coJSScripting, sceneAccess, sceneId, forceFlag);

    } catch(SceneAccessException& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
                    "executeDeviceScene: execution not allowed: " +
                    std::string(e.what()));
      std::string action_name = getActionName(_actionNode);
      boost::shared_ptr<Event> pEvent;
      pEvent.reset(new Event("executionDenied"));
      pEvent->setProperty("action-type", "device-scene");
      pEvent->setProperty("action-name", action_name);
      pEvent->setProperty("source-name", m_properties.get("source-name", ""));
      pEvent->setProperty("reason", std::string(e.what()));
      if (DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(pEvent);
      }
    } catch (std::runtime_error& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeDeviceScene: could not call scene on zone " +
              std::string(e.what()));
    }
  }

  void SystemEventActionExecute::executeDeviceValue(PropertyNodePtr _actionNode) {
    try {
      SceneAccessCategory sceneAccess = SAC_UNKNOWN;

      PropertyNodePtr oValueNode = _actionNode->getPropertyByName("value");
      if (oValueNode == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute::"
              "executeDeviceValue: could not set value on device - missing "
              "value", lsError);
        return;
      }

      boost::shared_ptr<Device> target = getDeviceFromNode(_actionNode);
      if (target == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute::"
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
                       oValueNode->getIntegerValue());

    } catch(SceneAccessException& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
                    "executeDeviceValue: execution not allowed: " +
                    std::string(e.what()));
      std::string action_name = getActionName(_actionNode);
      boost::shared_ptr<Event> pEvent;
      pEvent.reset(new Event("executionDenied"));
      pEvent->setProperty("action-type", "device-value");
      pEvent->setProperty("action-name", action_name);
      pEvent->setProperty("source-name", m_properties.get("source-name", ""));
      pEvent->setProperty("reason", std::string(e.what()));
      if (DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(pEvent);
      }
    } catch (std::runtime_error& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeDeviceValue: could not set value on device " +
              std::string(e.what()));
    }
  }

  void SystemEventActionExecute::executeDeviceBlink(PropertyNodePtr _actionNode) {
    try {
      SceneAccessCategory sceneAccess = SAC_UNKNOWN;

      boost::shared_ptr<Device> target = getDeviceFromNode(_actionNode);
      if (target == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute::"
                 "executeDeviceBlink: could not blink device - device "
                 "was not found", lsError);
        return;
      }

      PropertyNodePtr oCategoryNode = _actionNode->getPropertyByName("category");
      if (oCategoryNode != NULL) {
        sceneAccess = SceneAccess::stringToCategory(oCategoryNode->getStringValue());
      }

      target->blink(coJSScripting, sceneAccess);

    } catch(SceneAccessException& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
                    "executeDeviceBlink: execution not allowed: " +
                    std::string(e.what()));
      std::string action_name = getActionName(_actionNode);
      boost::shared_ptr<Event> pEvent;
      pEvent.reset(new Event("executionDenied"));
      pEvent->setProperty("action-type", "device-blink");
      pEvent->setProperty("action-name", action_name);
      pEvent->setProperty("source-name", m_properties.get("source-name", ""));
      pEvent->setProperty("reason", std::string(e.what()));
      if (DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(pEvent);
      }
    } catch (std::runtime_error& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeDeviceBlink: could not blink device " +
              std::string(e.what()));
    }
  }

  void SystemEventActionExecute::executeZoneBlink(PropertyNodePtr _actionNode) {
    try {
      int zoneId;
      int groupId;
      SceneAccessCategory sceneAccess = SAC_UNKNOWN;

      PropertyNodePtr oZoneNode = _actionNode->getPropertyByName("zone");
      if (oZoneNode == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute::"
                "executeZoneBlink - missing zone parameter", lsError);
        return;
      } else {
        zoneId = oZoneNode->getIntegerValue();
      }

      PropertyNodePtr oGroupNode = _actionNode->getPropertyByName("group");
      if (oGroupNode == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute::"
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
        group->blink(coJSScripting, sceneAccess);
      }
    } catch(SceneAccessException& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
                    "executeZoneBlink: execution not allowed: " +
                    std::string(e.what()));
      std::string action_name = getActionName(_actionNode);
      boost::shared_ptr<Event> pEvent;
      pEvent.reset(new Event("executionDenied"));
      pEvent->setProperty("action-type", "zone-blink");
      pEvent->setProperty("action-name", action_name);
      pEvent->setProperty("source-name", m_properties.get("source-name", ""));
      pEvent->setProperty("reason", std::string(e.what()));
      if (DSS::hasInstance()) {
        DSS::getInstance()->getEventQueue().pushEvent(pEvent);
      }
    } catch (std::runtime_error& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeZoneBlink: could not blink zone " +
              std::string(e.what()));
    }
  }

  void SystemEventActionExecute::executeCustomEvent(PropertyNodePtr _actionNode) {
    PropertyNodePtr oEventNode = _actionNode->getPropertyByName("event");
    if (oEventNode == NULL) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeCustomEvent - missing event parameter", lsError);
      return;
    }

    boost::shared_ptr<Event> evt(new Event("highlevelevent"));
    evt->setProperty("id", oEventNode->getStringValue());
    evt->setProperty("source-name", getActionName(_actionNode));
    if (DSS::hasInstance()) {
      DSS::getInstance()->getEventQueue().pushEvent(evt);
    }
  }

  void SystemEventActionExecute::executeURL(PropertyNodePtr _actionNode) {
#ifndef HAVE_CURL
    Logger::getInstance()->log("SystemEventActionExecute:"
            "executeURL: dSS was compiled without cURL support, "
            "ignoring request", lsWarning);
#else
    PropertyNodePtr oUrlNode =  _actionNode->getPropertyByName("url");
    if (oUrlNode == NULL) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeURL: missing url parameter", lsError);
      return;
    }

    std::string oUrl = oUrlNode->getAsString();
    if (oUrl.empty()) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeURL: empty url parameter", lsError);
      return;
    }

    Logger::getInstance()->log("SystemEventActionExecute::"
            "executeURL: " + oUrl);

    boost::shared_ptr<URL> url(new URL());
    long code = url->request(oUrl);
    std::ostringstream out;
    out << code;

    Logger::getInstance()->log("SystemEventActionExecute::"
            "executeURL: request to " + oUrl + " returned HTTP code " +
            out.str());
#endif
  }

  void SystemEventActionExecute::executeStateChange(PropertyNodePtr _actionNode) {
    PropertyNodePtr oStateNode = _actionNode->getPropertyByName("statename");
    if (oStateNode == NULL) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeStateChange - missing statename parameter", lsError);
      return;
    }

    boost::shared_ptr<State> pState;
    try {
      pState = DSS::getInstance()->getApartment().getState(oStateNode->getStringValue());
    } catch(ItemNotFoundException& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
          "executeStateChange - state '" + oStateNode->getStringValue() + "' does not exist", lsError);
      return;
    }

    if (pState->getType() == StateType_Device) {
      Logger::getInstance()->log("SystemEventActionExecute::"
          "executeStateChange - cannot modify states of type \'device\'", lsError);
      return;
    }

    PropertyNodePtr oValueNode = _actionNode->getPropertyByName("value");
    PropertyNodePtr oSValueNode = _actionNode->getPropertyByName("state");
    if (oValueNode == NULL && oSValueNode == NULL) {
      Logger::getInstance()->log("SystemEventActionExecute::"
          "executeStateChange: missing value or state parameter", lsError);
      return;
    }

    if (oValueNode && oValueNode->getValueType() == vTypeInteger) {
      pState->setState(coJSScripting, oValueNode->getIntegerValue());
    } else if (oSValueNode && oSValueNode->getValueType() == vTypeString) {
      pState->setState(coJSScripting, oSValueNode->getStringValue());
    } else {
      Logger::getInstance()->log("SystemEventActionExecute::"
          "executeStateChange: wrong data type for value or state parameter", lsError);
    }
  }

  unsigned int SystemEventActionExecute::executeOne(
                                                PropertyNodePtr _actionNode) {
    Logger::getInstance()->log("SystemEventActionExecute::"
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
        } else if (sActionType == "device-blink") {
          executeDeviceBlink(_actionNode);
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
        }
      } else {
        Logger::getInstance()->log("SystemEventActionExecute::"
                "type is not available", lsError);
      }
    } else {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "type node is not available", lsError);

    }

    return 0;
  }

  void SystemEventActionExecute::executeStep(std::vector<PropertyNodePtr> _actionNodes) {
      for (size_t i = 0; i < _actionNodes.size(); i++) {
        PropertyNodePtr oActionNode = _actionNodes.at(i);
          unsigned int lWaitingTime = 100;
          if (oActionNode != NULL) {
            lWaitingTime = executeOne(oActionNode);
          } else {
            Logger::getInstance()->log("SystemEventActionExecute::"
                 "action node in executeStep is not available", lsError);
          }
          if (lWaitingTime > 0) {
            sleepMS(lWaitingTime);
          }
      }
  }

  void SystemEventActionExecute::execute(std::string _path) {
    if (_path.empty()) {
      return;
    }
    if (!DSS::hasInstance()) {
      return;
    }
    PropertyNodePtr oBaseActionNode = DSS::getInstance()->getPropertySystem().getProperty(_path + "/actions");
    if (oBaseActionNode == NULL) {
      Logger::getInstance()->log("SystemEventActionExecute: no "
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
      Logger::getInstance()->log("SystemEventActionExecute: delay "
              "parameter present, execution will be fragmented", lsDebug);

      for (size_t s = 0; s < oDelay.size(); s++) {
        boost::shared_ptr<Event> evt(new Event("action_execute"));
        evt->applyProperties(m_properties);
        evt->setProperty("path", _path);
        evt->setProperty("delay", intToString(oDelay.at(s)));
        evt->setProperty(EventPropertyTime, "+" + intToString(oDelay.at(s)));
        if (DSS::hasInstance()) {
          DSS::getInstance()->getEventQueue().pushEvent(evt);
        } else {
          return;
        }
      }
    } else {
      if (m_properties.has("ignoreConditions")) {
        Logger::getInstance()->log("SystemEventActionExecute: "
            "condition check disabled in path" + _path);
      }
      else if (!checkSystemCondition(_path)) {
        Logger::getInstance()->log("SystemEventActionExecute: "
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

  std::vector<PropertyNodePtr> SystemEventActionExecute::filterActionsWithDelay(PropertyNodePtr _actionNode, int _delayValue) {
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

  void SystemEventActionExecute::executeWithDelay(std::string _path,
                                                           std::string _delay) {
    if (!DSS::hasInstance()) {
      return;
    }
    PropertyNodePtr oBaseActionNode =
        DSS::getInstance()->getPropertySystem().getProperty(_path + "/actions");
    if (oBaseActionNode == NULL) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeWithDelay: no actionNodes in path " + _path, lsError);
      return;
    }
    if (oBaseActionNode->getChildCount() == 0) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeWithDelay: no actionSubnodes in path " + _path, lsError);
      return;
    }

    if (m_properties.has("ignoreConditions")) {
      Logger::getInstance()->log("SystemEventActionExecute::"
          "executeWithDelay: condition check disabled in path" + _path);
    }
    else if (!checkSystemCondition(_path)) {
      Logger::getInstance()->log("SystemEventActionExecute::"
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

  void SystemEventActionExecute::run() {
    if (DSS::hasInstance()) {
      DSS::getInstance()->getSecurity().loginAsSystemUser(
        "EventInterpreterPluginActionExecute needs system rights");
    } else {
      return;
    }

    if (!m_delay.empty()) {
      executeWithDelay(m_path, m_delay);
    } else {
      execute(m_path);
    }
  }

  bool SystemEventActionExecute::setup(Event& _event) {
    SystemEvent::setup(_event);

    if (_event.hasPropertySet("path")) {
      m_path = _event.getPropertyByName("path");
      if (_event.hasPropertySet("delay")) {
        m_delay = _event.getPropertyByName("delay");
      }
      return true;
    } else {
        Logger::getInstance()->log("SystemEventActionExecute::setup: "
            "missing property \'path\' in event " + _event.getName());
    }

    return false;
  }

  SystemEvent::SystemEvent() : Task() {
  }
  SystemEvent::~SystemEvent() {
  }

  std::string SystemEvent::getActionName(PropertyNodePtr _actionNode) {
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

  SystemEventHighlevel::SystemEventHighlevel() : SystemEventActionExecute() {
  }

  SystemEventHighlevel::~SystemEventHighlevel() {
  }

  bool SystemEventHighlevel::setup(Event& _event) {
    return SystemEvent::setup(_event);
  }

  void SystemEventHighlevel::run() {
    if (!m_properties.has("id")) {
      return;
    }

    std::string id = m_properties.get("id");
    if (id.empty()) {
      return;
    }

    if (!DSS::hasInstance()) {
      return;
    }
    // check for user defined actions
    PropertyNodePtr baseNode =
        DSS::getInstance()->getPropertySystem().getProperty("/usr/events");
    if (baseNode == NULL) {
      // ok if no hle's found
      return;
    }

    if (baseNode->getChildCount() == 0) {
      return;
    }
    for (int i = 0; i < baseNode->getChildCount(); i++) {
      PropertyNodePtr parent = baseNode->getChild(i);
      if (parent != NULL) {
        PropertyNodePtr idNode = parent->getPropertyByName("id");
        if ((idNode != NULL) &&
            (idNode->getStringValue() == id)) {
          std::string fullName = parent->getDisplayName();
          PropertyNode* nextNode = parent->getParentNode();
          while(nextNode != NULL) {
            if(nextNode->getParentNode() != NULL) {
              fullName = nextNode->getDisplayName() + "/" + fullName;
            } else {
              fullName = "/" + fullName;
            }
            nextNode = nextNode->getParentNode();
          }
          execute(fullName);
          return;
        }
      }
    } // for loop
    Logger::getInstance()->log("SystemEventHighlevel::"
            "run: event for id " + id + " not found", lsError);

  }

  SystemTrigger::SystemTrigger() : SystemEvent(),
      m_evtSrcIsGroup(false), m_evtSrcIsDevice(false) {
  }

  SystemTrigger::~SystemTrigger() {
  }

  bool SystemTrigger::checkSceneZone(PropertyNodePtr _triggerProp) {
    if (m_evtName != "callScene") {
      return false;
    }

    if (!m_evtSrcIsGroup) {
      return false;
    }

    std::string zone = intToString(m_evtSrcZone);
    std::string group = intToString(m_evtSrcGroup);
    std::string scene = m_properties.get("sceneID");
    dss_dsid_t originDevice;
    if (m_properties.has("originDeviceID")) {
      originDevice = dss_dsid_t::fromString(m_properties.get("originDeviceID"));
    }
    std::string forced;
    if (m_properties.has("forced")) {
      forced = m_properties.get("forced");
    }

    if (zone.empty() && group.empty() && scene.empty()) {
      return false;
    }

    PropertyNodePtr triggerZone = _triggerProp->getPropertyByName("zone");
    if (triggerZone == NULL) {
      return false;
    }

    std::string iZone = triggerZone->getAsString();
    if ((strToIntDef(iZone, -1) >= 0) && (iZone != zone)) {
      return false;
    }

    PropertyNodePtr triggerGroup = _triggerProp->getPropertyByName("group");
    if (triggerGroup == NULL) {
      return false;
    }

    std::string iGroup = triggerGroup->getAsString();
    if ((strToIntDef(iGroup, -1) >= 0) && (iGroup != group)) {
      return false;
    }

    PropertyNodePtr triggerScene = _triggerProp->getPropertyByName("scene");
    if (triggerScene == NULL) {
      return false;
    }
    
    std::string iScene = triggerScene->getAsString();
    if ((strToIntDef(iScene, -1) >= 0) && (iScene != scene)) {
        return false;
    }
    
    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName("dsid");
    std::string iDevice;
    if (triggerDSID != NULL) {
      iDevice = triggerDSID->getAsString();
      if (!iDevice.empty() && (iDevice != "-1") &&
         (iDevice != originDevice.toString())) {
        return false;
      }
    }

    PropertyNodePtr forcedFlag = _triggerProp->getPropertyByName("forced");
    if (forcedFlag) {
      std::string iForced = forcedFlag->getAsString();
      if (!forced.empty() && (forced.compare(iForced) != 0)) {
        return false;
      }
    }

    Logger::getInstance()->log("SystemTrigger::"
            "checkSceneZone: *** Match: CallScene Zone: " + iZone +
            ", Group: " + iGroup + ", Scene: " +
            iScene + ", Origin: " + iDevice);

    return true;
  }

  bool SystemTrigger::checkUndoSceneZone(PropertyNodePtr _triggerProp) {
    if (m_evtName != "undoScene") {
      return false;
    }

    if (!m_evtSrcIsGroup) {
      return false;
    }

    std::string zone = intToString(m_evtSrcZone);
    std::string group = intToString(m_evtSrcGroup);
    std::string scene = m_properties.get("sceneID");
    dss_dsid_t originDevice;
    if (m_properties.has("originDeviceID")) {
      originDevice = dss_dsid_t::fromString(m_properties.get("originDeviceID"));
    }

    if (zone.empty() && group.empty() && scene.empty()) {
      return false;
    }

    PropertyNodePtr triggerZone = _triggerProp->getPropertyByName("zone");
    if (triggerZone == NULL) {
      return false;
    }

    std::string iZone = triggerZone->getAsString();
    if ((strToIntDef(iZone, -1) >= 0) && (iZone != zone)) {
      return false;
    }

    PropertyNodePtr triggerGroup = _triggerProp->getPropertyByName("group");
    if (triggerGroup == NULL) {
      return false;
    }

    std::string iGroup = triggerGroup->getAsString();
    if ((strToIntDef(iGroup, -1) >= 0) && (iGroup != group)) {
      return false;
    }

    PropertyNodePtr triggerScene = _triggerProp->getPropertyByName("scene");
    if (triggerScene == NULL) {
      return false;
    }

    std::string iScene = triggerScene->getAsString();
    if ((strToIntDef(iScene, -1) >= 0) && (iScene != scene)) {
        return false;
    }

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName("dsid");
    std::string iDevice;
    if (triggerDSID != NULL) {
      iDevice = triggerDSID->getAsString();
      if (!iDevice.empty() && (iDevice != "-1") &&
         (iDevice != originDevice.toString())) {
        return false;
      }
    }

    Logger::getInstance()->log("SystemTrigger::"
            "checkUndoSceneZone: *** Match: UndoScene Zone: " + iZone +
            ", Group: " + iGroup + ", Scene: " +
            iScene + ", Origin: " + iDevice);

    return true;
  }

  bool SystemTrigger::checkDeviceScene(PropertyNodePtr _triggerProp) {
    if (m_evtName != "callScene") {
      return false;
    }

    if (!m_evtSrcIsDevice) {
      return false;
    }

    dss_dsid_t dsid = m_evtSrcDSID;
    std::string scene = m_properties.get("sceneID");;

    if ((scene.empty()) || (dsid == NullDSID)) {
      return false;
    }

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName("dsid");
    if (triggerDSID == NULL) {
      return false;
    }

    PropertyNodePtr triggerSCENE = _triggerProp->getPropertyByName("scene");
    if (triggerSCENE == NULL) {
      return false;
    }

    std::string sDSID = triggerDSID->getStringValue();
    std::string iSCENE = triggerSCENE->getAsString();
    if ((sDSID == "-1") || (sDSID == dsid.toString())) {
      if ((iSCENE == scene) || (iSCENE == "-1")) {
        Logger::getInstance()->log("SystemTrigger::"
                "checkDeviceScene: Match: DeviceScene dSID:" + sDSID +
                ", Scene: " + iSCENE);
        return true;
      }
    }

    return false;
  }


  bool SystemTrigger::checkDeviceSensor(PropertyNodePtr _triggerProp) {
    if (m_evtName != "deviceSensorEvent") {
      return false;
    }
    
    dss_dsid_t dsid = m_evtSrcDSID;
    std::string eventName = m_properties.get("sensorEvent");;
    std::string eventIndex = m_properties.get("sensorIndex");

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName("dsid");
    if (triggerDSID == NULL) {
      return false;
    }

    std::string sDSID = triggerDSID->getStringValue();
    if (!((sDSID == "-1") || (sDSID == dsid.toString()))) {
      return false;
    }

    PropertyNodePtr triggerEventId = _triggerProp->getPropertyByName("eventid");
    PropertyNodePtr triggerName = _triggerProp->getPropertyByName("evt");

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
    if (m_evtName != "deviceBinaryInputEvent") {
      return false;
    }

    dss_dsid_t dsid = m_evtSrcDSID;
    std::string eventIndex = m_properties.get("inputIndex");
    std::string eventType = m_properties.get("inputType");
    std::string eventState = m_properties.get("inputState");

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName("dsid");
    PropertyNodePtr triggerIndex = _triggerProp->getPropertyByName("index");
    PropertyNodePtr triggerType = _triggerProp->getPropertyByName("stype");
    PropertyNodePtr triggerState = _triggerProp->getPropertyByName("state");

    // need either: dsid + index + state or stype + state (+ dsid)

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
      if (!((sDSID == "-1") || (sDSID == dsid.toString()))) {
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
    if (m_evtName != "buttonClick") {
      return false;
    }

    dss_dsid_t dsid = m_evtSrcDSID;
    std::string clickType = m_properties.get("clickType");
    std::string buttonIndex = m_properties.get("buttonIndex");

    if (dsid == NullDSID) {
      return false;
    }

    if (clickType.empty() || (clickType == "-1")) {
      return false;
    }

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName("dsid");
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
    if ((sDSID == "-1") || (sDSID == dsid.toString())) {
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
    if (m_evtName != "stateChange") {
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

  bool SystemTrigger::checkTrigger(std::string _path) {
    if (!DSS::hasInstance()) {
      return false;
    }
    PropertyNodePtr appProperty =
      DSS::getInstance()->getPropertySystem().getProperty(_path);
    if (appProperty == false) {
      return false;
    }

    PropertyNodePtr appTrigger = appProperty->getPropertyByName("triggers");
    if (appTrigger == NULL) {
      return false;
    }

    for (int i = 0; i < appTrigger->getChildCount(); i++) {
      PropertyNodePtr triggerProp = appTrigger->getChild(i);
      if (triggerProp == NULL) {
        continue;
      }

      PropertyNodePtr triggerType = triggerProp->getPropertyByName("type");
      if (triggerType == NULL) {
        continue;
      }

      std::string triggerValue = triggerType->getAsString();

      if (m_evtName == "callScene") {
        if (triggerValue == "zone-scene") {
          if (checkSceneZone(triggerProp)) {
            return true;
          }
        } else if (triggerValue == "device-scene") {
          if (checkDeviceScene(triggerProp)) {
            return true;
          }
        }

      } else if (m_evtName == "undoScene") {
        if (triggerValue == "undo-zone-scene") {
          if (checkUndoSceneZone(triggerProp)) {
            return true;
          }
        }

      } else if (m_evtName == "buttonClick") {
        if (triggerValue == "device-msg") {
          if (checkDevice(triggerProp)) {
            return true;
          }
        }

      } else if (m_evtName == "deviceSensorEvent") {
        if (triggerValue == "device-sensor") {
          if (checkDeviceSensor(triggerProp)) {
            return true;
          }
        }

      } else if (m_evtName == "deviceBinaryInputEvent") {
        if (triggerValue == "device-binary-input") {
          if (checkDeviceBinaryInput(triggerProp)) {
            return true;
          }
        }

      } else if (m_evtName == "highlevelevent") {
        if (triggerValue == "custom-event") {
          if (checkHighlevel(triggerProp)) {
            return true;
          }
        }

      } else if (m_evtName == "stateChange") {
        if (triggerValue == "state-change") {
          if (checkState(triggerProp)) {
            return true;
          }
        }

      }

    } // for loop
    return false;
  }

  void SystemTrigger::relayTrigger(PropertyNodePtr _relay) {
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

    boost::shared_ptr<Event> evt(new Event(sRelayedEventName));
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

    if (DSS::hasInstance()) {
      Logger::getInstance()->log("SystemTrigger::"
              "relayTrigger: relaying event \'" + evt->getName() + "\'");

      DSS::getInstance()->getEventQueue().pushEvent(evt);
    }
  }

  void SystemTrigger::run() {
    if (!DSS::hasInstance()) {
      return;
    }

    DSS::getInstance()->getSecurity().loginAsSystemUser(
        "EventInterpreterPluginSystemTrigger needs system rights");

    PropertyNodePtr triggerProperty =
        DSS::getInstance()->getPropertySystem().getProperty("/usr/triggers");
    if (triggerProperty == NULL) {
      return;
    }
    for (int i = 0; i < triggerProperty->getChildCount(); i++) {
      PropertyNodePtr triggerNode = triggerProperty->getChild(i);
      if (triggerNode == NULL) {
        continue;
      }
      PropertyNodePtr triggerPathNode =
          triggerNode->getPropertyByName("triggerPath");
      if (triggerPathNode == NULL) {
        continue;
      }
      std::string sTriggerPath = triggerPathNode->getStringValue();
      if (checkTrigger(sTriggerPath) && checkSystemCondition(sTriggerPath)) {
        relayTrigger(triggerNode);
      }
    } // for loop
  }

  bool SystemEvent::setup(Event& _event) {
    m_properties = _event.getProperties();
    return true;
  }

  bool SystemTrigger::setup(Event& _event) {
    SystemEvent::setup(_event);
    m_evtName = _event.getName();
    EventRaiseLocation raiseLocation = _event.getRaiseLocation();
    if((raiseLocation == erlGroup) || (raiseLocation == erlApartment)) {
      if (!DSS::hasInstance()) {
        return false;
      }
      boost::shared_ptr<const Group> group =
          _event.getRaisedAtGroup(DSS::getInstance()->getApartment());

      m_evtSrcIsGroup = (raiseLocation == erlGroup);
      m_evtSrcIsDevice = false;
      m_evtSrcZone = group->getZoneID();
      m_evtSrcGroup = group->getID();
    } else if (raiseLocation == erlDevice) {
      boost::shared_ptr<const DeviceReference> device =
          _event.getRaisedAtDevice();
      m_evtSrcIsGroup = false;
      m_evtSrcIsDevice = true;
      m_evtSrcZone = device->getDevice()->getZoneID();
      m_evtSrcDSID = device->getDSID();
    } else if (raiseLocation == erlState) {
      m_evtSrcIsGroup = false;
      m_evtSrcIsDevice = false;
    }

    return true;
  }

  EventInterpreterPluginActionExecute::EventInterpreterPluginActionExecute(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("action_execute", _pInterpreter)
  { }

  EventInterpreterPluginActionExecute::~EventInterpreterPluginActionExecute()
  { }

  void EventInterpreterPluginActionExecute::handleEvent(Event& _event, const EventSubscription& _subscription) {
    Logger::getInstance()->log("EventInterpreterPluginActionExecute::"
            "handleEvent: processing event " + _event.getName());

    boost::shared_ptr<SystemEventActionExecute> action(new SystemEventActionExecute());
    if (action->setup(_event) == false) {
      Logger::getInstance()->log("EventInterpreterPluginActionExecute::"
              "handleEvent: missing path property, ignoring event");
      return;
    }

    addEvent(action);
  }


  EventInterpreterPluginSystemTrigger::EventInterpreterPluginSystemTrigger(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("system_trigger", _pInterpreter)
  { }

  EventInterpreterPluginSystemTrigger::~EventInterpreterPluginSystemTrigger()
  { }

  void EventInterpreterPluginSystemTrigger::handleEvent(Event& _event, const EventSubscription& _subscription) {
    Logger::getInstance()->log("EventInterpreterPluginSystemTrigger::"
            "handleEvent: processing event \'" + _event.getName() + "\'",
            lsDebug);

    boost::shared_ptr<SystemTrigger> trigger(new SystemTrigger());

    if (!trigger->setup(_event)) {
      Logger::getInstance()->log("EventInterpreterPluginSystemTrigger::"
              "handleEvent: could not setup event data!");
      return;
    }

    addEvent(trigger);
  }

  EventInterpreterPluginHighlevelEvent::EventInterpreterPluginHighlevelEvent(EventInterpreter* _pInterpreter) : 
      EventInterpreterPlugin("highlevelevent", _pInterpreter) {
  }

  EventInterpreterPluginHighlevelEvent::~EventInterpreterPluginHighlevelEvent()
  { }

  void EventInterpreterPluginHighlevelEvent::handleEvent(Event& _event, const EventSubscription& _subscription) {
    Logger::getInstance()->log("EventInterpreterPluginHighlevelEvent::"
            "handleEvent " + _event.getName(), lsDebug);


    boost::shared_ptr<SystemTrigger> trigger(new SystemTrigger());
    if (trigger->setup(_event) == false) {
      Logger::getInstance()->log("EventInterpreterPluginHighlevelEvent::"
              "handleEvent: could not setup event data for SystemTrigger!");
      return;
    }

    boost::shared_ptr<SystemEventHighlevel> hl(new SystemEventHighlevel());
    if (hl->setup(_event) == false) {
      Logger::getInstance()->log("EventInterpreterPluginHighlevelEvent::"
        "handleEvent: could not setup event data for SystemEventHighlevel!");
      return;
    }

    addEvent(trigger);
    addEvent(hl);
  }


} // namespace dss
