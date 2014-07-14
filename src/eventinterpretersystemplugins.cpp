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

#include <signal.h>
#include <set>

#include "eventinterpretersystemplugins.h"
#include "internaleventrelaytarget.h"
#include "logger.h"
#include "base.h"
#include "dss.h"
#include "http_client.h"
#include "model/set.h"
#include "model/zone.h"
#include "model/group.h"
#include "model/device.h"
#include "model/apartment.h"
#include "model/state.h"
#include "model/modelconst.h"
#include "model/scenehelper.h"
#include "propertysystem.h"
#include "systemcondition.h"
#include "security/security.h"
#include "src/dsidhelper.h"
#include "util.h"

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

  const int SceneTable0_length = 56;
  const char *SceneTable0[] =
  {
    "Off",
    "Off-Area1",
    "Off-Area2",
    "Off-Area3",
    "Off-Area4",
    "Stimmung1",
    "On-Area1",
    "On-Area2",
    "On-Area3",
    "On-Area4",
    "Dimm-Area",
    "Dec",
    "Inc",
    "Min",
    "Max",
    "Stop",
    "N/A",
    "Stimmung2",
    "Stimmung3",
    "Stimmung4",
    "Stimmung12",
    "Stimmung13",
    "Stimmung14",
    "Stimmung22",
    "Stimmung23",
    "Stimmung24",
    "Stimmung32",
    "Stimmung33",
    "Stimmung34",
    "Stimmung42",
    "Stimmung43",
    "Stimmung44",
    "Off-Stimmung10",
    "On-Stimmung11",
    "Off-Stimmung20",
    "On-Stimmung21",
    "Off-Stimmung30",
    "On-Stimmung31",
    "Off-Stimmung40",
    "On-Stimmung41",
    "Off-Automatic",
    "N/A",
    "Dec-Area1",
    "Inc-Area1",
    "Dec-Area2",
    "Inc-Area2",
    "Dec-Area3",
    "Inc-Area3",
    "Dec-Area4",
    "Inc-Area4",
    "Off-Device",
    "On-Device",
    "Stop-Area1",
    "Stop-Area2",
    "Stop-Area3",
    "Stop-Area4"
  };

  const int SceneTable1_length = 28;
  const char *SceneTable1[] =
  {
    "N/A",
    "Panic",
    "Overload",
    "Zone-Standby",
    "Zone-Deep-Off",
    "Sleeping",
    "Wake-Up",
    "Present",
    "Absent",
    "Bell",
    "Alarm",
    "Zone-Activate",
    "Fire",
    "Smoke",
    "Water",
    "Gas",
    "Bell2",
    "Bell3",
    "Bell4",
    "Alarm2",
    "Alarm3",
    "Alarm4",
    "Wind",
    "WindN",
    "Rain",
    "RainN",
    "Hail",
    "HailN"
  };

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
        group->callScene(coJSScripting, sceneAccess, sceneId, "", forceFlag);
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
        group->undoScene(coJSScripting, sceneAccess, sceneId, "");
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

      dsuid_t deviceDSID = str2dsuid(dsidStr);

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

      target->callScene(coJSScripting, sceneAccess, sceneId, "", forceFlag);

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
                       oValueNode->getIntegerValue(), "");

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

      target->blink(coJSScripting, sceneAccess, "");

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
        group->blink(coJSScripting, sceneAccess, "");
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
    PropertyNodePtr oUrlNode =  _actionNode->getPropertyByName("url");
    if (oUrlNode == NULL) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeURL: missing url parameter", lsError);
      return;
    }

    std::string oUrl = unescapeHTML(oUrlNode->getAsString());
    if (oUrl.empty()) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeURL: empty url parameter", lsError);
      return;
    }

    Logger::getInstance()->log("SystemEventActionExecute::"
            "executeURL: " + oUrl);

    boost::shared_ptr<HttpClient> url(new HttpClient());
    long code = url->request(oUrl, GET, NULL);
    std::ostringstream out;
    out << code;

    Logger::getInstance()->log("SystemEventActionExecute::"
            "executeURL: request to " + oUrl + " returned HTTP code " +
            out.str());
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
      pState = DSS::getInstance()->getApartment().getNonScriptState(oStateNode->getStringValue());
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

  void SystemEventActionExecute::executeAddonStateChange(PropertyNodePtr _actionNode) {

    PropertyNodePtr oScriptNode = _actionNode->getPropertyByName("addon-id");
    if (oScriptNode == NULL) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeAddonStateChange - missing addon-id parameter", lsError);
      return;
    }

    PropertyNodePtr oStateNode = _actionNode->getPropertyByName("statename");
    if (oStateNode == NULL) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeAddonStateChange - missing statename parameter", lsError);
      return;
    }

    boost::shared_ptr<State> pState;
    try {
      pState = DSS::getInstance()->getApartment().getState(StateType_Script,
        oScriptNode->getStringValue(), oStateNode->getStringValue());
    } catch(ItemNotFoundException& e) {
      Logger::getInstance()->log("SystemEventActionExecute::"
          "executeAddonStateChange - state '" + oStateNode->getStringValue() + "' does not exist", lsError);
      return;
    }

    PropertyNodePtr oValueNode = _actionNode->getPropertyByName("value");
    PropertyNodePtr oSValueNode = _actionNode->getPropertyByName("state");
    if (oValueNode == NULL && oSValueNode == NULL) {
      Logger::getInstance()->log("SystemEventActionExecute::"
          "executeAddonStateChange: missing value or state parameter", lsError);
      return;
    }

    if (oValueNode && oValueNode->getValueType() == vTypeInteger) {
      pState->setState(coJSScripting, oValueNode->getIntegerValue());
    } else if (oSValueNode && oSValueNode->getValueType() == vTypeString) {
      pState->setState(coJSScripting, oSValueNode->getStringValue());
    } else {
      Logger::getInstance()->log("SystemEventActionExecute::"
          "executeAddonStateChange: wrong data type for value or state parameter", lsError);
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
        } else if (sActionType == "change-addon-state") {
          executeAddonStateChange(_actionNode);
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
        evt->setProperty(EventProperty::Time, "+" + intToString(oDelay.at(s)));
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

  static const std::string ss_callOrigin = "callOrigin";
  static const std::string ss_originDSID = "originDSID";
  static const std::string ss_zone = "zone";
  static const std::string ss_group = "group";
  static const std::string ss_scene = "scene";
  static const std::string ss_sceneID = "sceneID";
  static const std::string ss_dsid = "dsid";
  static const std::string ss_forced = "forced";
  static const std::string ss_sensorEvent = "sensorEvent";
  static const std::string ss_sensorIndex = "sensorIndex";
  static const std::string ss_eventid = "eventid";
  static const std::string ss_evt = "evt";
  static const std::string ss_triggers = "triggers";
  static const std::string ss_type = "type";

  SystemTrigger::SystemTrigger()
    : SystemEvent(),
      m_evtSrcIsGroup(false),
      m_evtSrcIsDevice(false),
      m_evtSrcZone(0),
      m_evtSrcGroup(0)
  {
  }

  SystemTrigger::~SystemTrigger() {
  }

  bool SystemTrigger::checkSceneZone(PropertyNodePtr _triggerProp) {
    if (!((m_evtName == "callScene") || (m_evtName == "callSceneBus"))) {
      return false;
    }

    if (!m_evtSrcIsGroup) {
      return false;
    }

    int scene = strToIntDef(m_properties.get(ss_sceneID), -1);
    dsuid_t originDSID;
    if (m_properties.has(ss_originDSID)) {
      originDSID = str2dsuid(m_properties.get(ss_originDSID));
    }
    bool forced;
    if (m_properties.has(ss_forced)) {
      std::string sForced = m_properties.get(ss_forced);
      forced = (sForced == "true");
    }

    PropertyNodePtr triggerZone = _triggerProp->getPropertyByName(ss_zone);
    if (triggerZone == NULL) {
      return false;
    }

    int iZone;
    try {
      iZone = triggerZone->getIntegerValue();
      if (iZone != m_evtSrcZone) {
        return false;
      }
    } catch (PropertyTypeMismatch& e){
      return false;
    }

    PropertyNodePtr triggerGroup = _triggerProp->getPropertyByName(ss_group);
    if (triggerGroup == NULL) {
      return false;
    }

    int iGroup;
    try {
      iGroup = triggerGroup->getIntegerValue();
      if (iGroup != m_evtSrcGroup) {
        return false;
      }
    } catch (PropertyTypeMismatch& e){
      return false;
    }

    PropertyNodePtr triggerScene = _triggerProp->getPropertyByName(ss_scene);
    if (triggerScene == NULL) {
      return false;
    }

    int iScene;
    try {
      iScene = triggerScene->getIntegerValue();
      if (iScene != scene) {
          return false;
      }
    } catch (PropertyTypeMismatch& e){
      return false;
    }
    
    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName(ss_dsid);
    std::string iDevice;
    if (triggerDSID != NULL) {
      iDevice = triggerDSID->getAsString();
      if (!iDevice.empty() && (iDevice != "-1") &&
         (iDevice != dsuid2str(originDSID))) {
        return false;
      }
    }

    PropertyNodePtr forcedFlag = _triggerProp->getPropertyByName(ss_forced);
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

    std::string bus = ((m_evtName == "callSceneBus") ? "Bus" : "");
    Logger::getInstance()->log("SystemTrigger::"
            "checkSceneZone: *** Match: CallScene" +
            bus +
            " Zone: " + intToString(iZone) + ", Group: " + intToString(iGroup) + ", Scene: " +
            intToString(iScene) + ", Origin: " + iDevice);

    return true;
  }

  bool SystemTrigger::checkUndoSceneZone(PropertyNodePtr _triggerProp) {
    if (m_evtName != "undoScene") {
      return false;
    }

    if (!m_evtSrcIsGroup) {
      return false;
    }

    int scene = strToIntDef(m_properties.get(ss_sceneID), -1);
    dsuid_t originDSID;
    if (m_properties.has(ss_originDSID)) {
      originDSID = str2dsuid(m_properties.get(ss_originDSID));
    }

    PropertyNodePtr triggerZone = _triggerProp->getPropertyByName(ss_zone);
    if (triggerZone == NULL) {
      return false;
    }

    int iZone;
    try {
      iZone = triggerZone->getIntegerValue();
      if (iZone != m_evtSrcZone) {
        return false;
      }
    } catch (PropertyTypeMismatch& e){
      return false;
    }

    PropertyNodePtr triggerGroup = _triggerProp->getPropertyByName(ss_group);
    if (triggerGroup == NULL) {
      return false;
    }

    int iGroup;
    try {
      iGroup = triggerGroup->getIntegerValue();
      if (iGroup != m_evtSrcGroup) {
        return false;
      }
    } catch (PropertyTypeMismatch& e){
      return false;
    }

    PropertyNodePtr triggerScene = _triggerProp->getPropertyByName(ss_scene);
    if (triggerScene == NULL) {
      return false;
    }

    int iScene;
    try {
      iScene = triggerScene->getIntegerValue();
      if (iScene != scene) {
          return false;
      }
    } catch (PropertyTypeMismatch& e){
      return false;
    }

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName(ss_dsid);
    std::string iDevice;
    if (triggerDSID != NULL) {
      iDevice = triggerDSID->getAsString();
      if (!iDevice.empty() && (iDevice != "-1") &&
         (iDevice != dsuid2str(originDSID))) {
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
    if (m_evtName != "callScene") {
      return false;
    }

    if (!m_evtSrcIsDevice) {
      return false;
    }

    dsuid_t dsid = m_evtSrcDSID;
    int scene = strToIntDef(m_properties.get(ss_sceneID), -1);

    if (IsNullDsuid(dsid)) {
      return false;
    }

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName(ss_dsid);
    if (triggerDSID == NULL) {
      return false;
    }

    PropertyNodePtr triggerSCENE = _triggerProp->getPropertyByName(ss_scene);
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
    if ((sDSID == "-1") || (sDSID == dsuid2str(dsid))) {
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
    
    dsuid_t dsid = m_evtSrcDSID;
    std::string eventName = m_properties.get(ss_sensorEvent);
    std::string eventIndex = m_properties.get(ss_sensorIndex);

    PropertyNodePtr triggerDSID = _triggerProp->getPropertyByName(ss_dsid);
    if (triggerDSID == NULL) {
      return false;
    }

    std::string sDSID = triggerDSID->getStringValue();
    if (!((sDSID == "-1") || (sDSID == dsuid2str(dsid)))) {
      return false;
    }

    PropertyNodePtr triggerEventId = _triggerProp->getPropertyByName(ss_eventid);
    PropertyNodePtr triggerName = _triggerProp->getPropertyByName(ss_evt);

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

    dsuid_t dsid = m_evtSrcDSID;
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
      if (!((sDSID == "-1") || (sDSID == dsuid2str(dsid)))) {
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

    dsuid_t dsid = m_evtSrcDSID;
    std::string clickType = m_properties.get("clickType");
    std::string buttonIndex = m_properties.get("buttonIndex");

    if (IsNullDsuid(dsid)) {
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
    if ((sDSID == "-1") || (sDSID == dsuid2str(dsid))) {
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
    if ((m_evtName != "addonStateChange") && (m_evtName != "stateChange")) {
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

    if (m_evtName == "addonStateChange") {
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

  bool SystemTrigger::checkTrigger(std::string _path) {
    if (!DSS::hasInstance()) {
      return false;
    }
    PropertyNodePtr appProperty =
      DSS::getInstance()->getPropertySystem().getProperty(_path);
    if (appProperty == false) {
      return false;
    }

    PropertyNodePtr appTrigger = appProperty->getPropertyByName(ss_triggers);
    if (appTrigger == NULL) {
      return false;
    }

    for (int i = 0; i < appTrigger->getChildCount(); i++) {
      PropertyNodePtr triggerProp = appTrigger->getChild(i);
      if (triggerProp == NULL) {
        continue;
      }

      PropertyNodePtr triggerType = triggerProp->getPropertyByName(ss_type);
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

      } else if (m_evtName == "callSceneBus") {
        if (triggerValue == "bus-zone-scene") {
          if (checkSceneZone(triggerProp)) {
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
      } else if (m_evtName == "addonStateChange") {
        if (triggerValue == "addon-state-change") {
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

    trigger->run();
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

  EventInterpreterPluginSystemEventLog::EventInterpreterPluginSystemEventLog(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("system_event_log", _pInterpreter)
  { }

  EventInterpreterPluginSystemEventLog::~EventInterpreterPluginSystemEventLog()
  { }

  void EventInterpreterPluginSystemEventLog::handleEvent(Event& _event, const EventSubscription& _subscription) {
    Logger::getInstance()->log("EventInterpreterPluginSystemEventLog::"
            "handleEvent: processing event \'" + _event.getName() + "\'",
            lsDebug);

    boost::shared_ptr<SystemEventLog> log(new SystemEventLog());

    if (!log->setup(_event)) {
      Logger::getInstance()->log("EventInterpreterPluginSystemEventLog::"
              "handleEvent: could not setup event data!");
      return;
    }

    addEvent(log);
  }


  SystemEventLog::SystemEventLog() : SystemEvent(), m_evtRaiseLocation(erlApartment) {
  }

  SystemEventLog::~SystemEventLog() {
  }

  std::string SystemEventLog::getZoneName(boost::shared_ptr<Zone> _zone) {
    std::string zName = _zone->getName();
    if (_zone->getID() == 0) {
      zName = "Broadcast";
    }
    return zName + ";" + intToString(_zone->getID());
  }

  std::string SystemEventLog::getGroupName(boost::shared_ptr<Group> _group) {
    std::string gName = _group->getName();
    if (gName.empty()) {
      gName = "Unknown";
    }

    return gName + ";" + intToString(_group->getID());
  }

  std::string SystemEventLog::getSceneName(int _scene_id) {
    std::string sceneName;

    if ((_scene_id >= 0) && (_scene_id < 64) &&
        (_scene_id < SceneTable0_length)) {
      sceneName = SceneTable0[_scene_id];
    }
    if ((_scene_id >= 64) && ((_scene_id - 64) < SceneTable1_length)) {
      sceneName = SceneTable1[_scene_id - 64];
    }

    sceneName += ";" + intToString(_scene_id);

    return sceneName;
  }

  std::string SystemEventLog::getDeviceName(std::string _origin_dsid) {
    std::string devName = "Unknown";

    if (!_origin_dsid.empty()) {
      try {
        boost::shared_ptr<Device> device =
            DSS::getInstance()->getApartment().getDeviceByDSID(
                                str2dsuid(_origin_dsid));
        if (device && (!device->getName().empty())) {
            devName = device->getName();
        }
      } catch (ItemNotFoundException &ex) {};
    }

    devName += ";" + _origin_dsid;
    return devName;
  }

  std::string SystemEventLog::getCallOrigin(callOrigin_t _call_origin) {
    switch (_call_origin) {
      case coJSScripting:
        return "Scripting";
      case coJSON:
        return "JSON";
      case coSubscription:
        return "Bus-Handler";
      case coTest:
        return "Test";
      case coSystem:
        return "System";
      case coSystemBinaryInput:
        return "System-Binary-Input";
      case coDsmApi:
        return "dSM-API";
      case coUnknown:
        return "(unspecified)";
      default:
        return intToString(_call_origin);
    }
  }

  void SystemEventLog::logLastScene(boost::shared_ptr<ScriptLogger> _logger,
                                    boost::shared_ptr<Zone> _zone,
                                    boost::shared_ptr<Group> _group,
                                    int _scene_id) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_group);
    std::string sceneName = getSceneName(_scene_id);

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";Last Scene;" + sceneName + ';' + zoneName + ';' +
                  groupName + ";;;");
  }

  void SystemEventLog::logZoneGroupScene(boost::shared_ptr<ScriptLogger> _logger,
                                         boost::shared_ptr<Zone> _zone,
                                         int _group_id, int _scene_id,
                                         bool _is_forced,
                                         std::string _origin_dsid,
                                         callOrigin_t _call_origin,
                                         std::string _origin_token) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_zone->getGroup(_group_id));
    std::string sceneName = getSceneName(_scene_id);
    std::string devName = getDeviceName(_origin_dsid);
    if (_is_forced) {
      //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;CallOrigin;originToken');
      _logger->logln(";CallSceneForced;" + sceneName + ";" + zoneName + ";" +
                     groupName + ";" + devName + ";" +
                     getCallOrigin(_call_origin) + ";" + _origin_token);
    } else {
      //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;CallOrigin;originToken');
      _logger->logln(";CallScene;" + sceneName + ";" + zoneName + ";" +
                     groupName + ";" + devName +";" +
                     getCallOrigin(_call_origin) + ";" + _origin_token);
    }
  }

  void SystemEventLog::logDeviceLocalScene(
                                        boost::shared_ptr<ScriptLogger> _logger,
                                        int _scene_id,
                                        std::string _origin_dsid,
                                        callOrigin_t _call_origin) {
    std::string devName = getDeviceName(_origin_dsid);
    std::string sceneName = getSceneName(_scene_id);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;CallOrigin;originToken');
    _logger->logln(";Device;" + sceneName + ";;;;;" + devName + ";" +
                   getCallOrigin(_call_origin) + ";");
  }

  void SystemEventLog::logDeviceScene(boost::shared_ptr<ScriptLogger> _logger,
                                      boost::shared_ptr<const Device> _device,
                                      boost::shared_ptr<Zone> _zone,
                                      int _scene_id, bool _is_forced,
                                      std::string _origin_dsid,
                                      callOrigin_t _call_origin,
                                      std::string _token) {
    std::string zoneName = getZoneName(_zone);
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());
    std::string origName = getDeviceName(_origin_dsid);
    std::string sceneName = getSceneName(_scene_id);

    if (_is_forced) {
      //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Device;Device-ID;Origin;Origin-ID;CallOrigin;originToken');
      _logger->logln(";DeviceSceneForced;" + sceneName + ";" + zoneName + ";" +
                     devName + ";" + origName + ";" +
                     getCallOrigin(_call_origin) + ";" + _token);
    } else {
      //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Device;Device-ID;Origin;Origin-ID;CallOrigin;originToken');
      _logger->logln(";DeviceScene;" + sceneName + ";" + zoneName + ";" +
                     devName + ";" + origName + ";" +
                     getCallOrigin(_call_origin) + ";" + _token);
    }
  }

  void SystemEventLog::logZoneGroupBlink(
                                        boost::shared_ptr<ScriptLogger> _logger,
                                        boost::shared_ptr<Zone> _zone,
                                        int _group_id,
                                        std::string _origin_dsid,
                                        callOrigin_t _call_origin,
                                        std::string _origin_token) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_zone->getGroup(_group_id));
    std::string devName = getDeviceName(_origin_dsid);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;CallOrigin;originToken');
    _logger->logln(";Blink;;" + zoneName + ";" + groupName + ";" + devName + 
                   ";;" + getCallOrigin(_call_origin) + ";" +_origin_token);
  }

  void SystemEventLog::logDeviceBlink(boost::shared_ptr<ScriptLogger> _logger,
                                      boost::shared_ptr<const Device> _device,
                                      boost::shared_ptr<Zone> _zone,
                                      std::string _origin_dsid,
                                      callOrigin_t _call_origin,
                                      std::string _origin_token) {
    std::string zoneName = getZoneName(_zone);
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());
    std::string origName = getDeviceName(_origin_dsid);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Device;Device-ID;Origin;Origin-ID;CallOrigin;originToken');
    _logger->logln(";DeviceBlink;;" + zoneName + ";" + devName + ";;" +
                   origName + ";" + getCallOrigin(_call_origin) + ";" + _origin_token);
  }

  void SystemEventLog::logZoneGroupUndo(boost::shared_ptr<ScriptLogger> _logger,
                                        boost::shared_ptr<Zone> _zone,
                                        int _group_id, int _scene_id,
                                        std::string _origin_dsid,
                                        callOrigin_t _call_origin,
                                        std::string _origin_token) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_zone->getGroup(_group_id));
    std::string devName = getDeviceName(_origin_dsid);
    std::string sceneName = getSceneName(_scene_id);

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";UndoScene;" + sceneName + ";" + zoneName + ";" +
                   groupName + ";" + devName + ";" +
                   getCallOrigin(_call_origin) + ";" + _origin_token);
  }

  void SystemEventLog::logDeviceButtonClick(
                                boost::shared_ptr<ScriptLogger> _logger,
                                boost::shared_ptr<const Device> _device) {
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());
    std::string keynum;
    if (m_properties.has("buttonIndex")) {
      keynum = m_properties.get("buttonIndex");
    }

    std::string clicknum;
    if (m_properties.has("clickType")) {
      clicknum = m_properties.get("clickType");
    }

    std::string holdcount;
    if (m_properties.has("holdCount")) {
      holdcount = m_properties.get("holdCount");
    }

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    if (holdcount.empty()) {
      _logger->logln(";ButtonClick;Type " + clicknum + ";" + keynum +
                     ";;;;;" + devName + ";");
    } else {
      _logger->logln(";ButtonClick;Hold " + holdcount + ';' + keynum +
                     ";;;;;" + devName + ";");
    }
  }

  void SystemEventLog::logDeviceBinaryInput(
                                boost::shared_ptr<ScriptLogger> _logger,
                                boost::shared_ptr<const Device> _device) {
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());

    std::string index;
    if (m_properties.has("inputIndex")) {
      index = m_properties.get("inputIndex");
    }

    std::string state;
    if (m_properties.has("inputState")) {
      state = m_properties.get("inputState");
    }

    std::string type;
    if (m_properties.has("inputType")) {
      type = m_properties.get("inputType");
    }

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";BinaryInput;State " + state + ";" + index + ";;;;;" +
                   devName + ";");
  }

  void SystemEventLog::logDeviceSensorEvent(
                                    boost::shared_ptr<ScriptLogger> _logger,
                                    boost::shared_ptr<const Device> _device) {
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());

    std::string sensorEvent;
    if (m_properties.has("sensorEvent")) {
      sensorEvent = m_properties.get("sensorEvent");
    }

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";EventTable;" + sensorEvent + ";;;;;;" + devName + ";");
  }

  void SystemEventLog::logDeviceSensorValue(
                                    boost::shared_ptr<ScriptLogger> _logger,
                                    boost::shared_ptr<const Device> _device,
                                    boost::shared_ptr<Zone> _zone) {
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());
    std::string zoneName = getZoneName(_zone);

    std::string sensorIndex;
    if (m_properties.has("sensorIndex")) {
      sensorIndex = m_properties.get("sensorIndex");
    }

    uint8_t sensorType = 255;
    if (m_properties.has("sensorType")) {
      sensorType = strToInt(m_properties.get("sensorType"));
    } else {
      try {
        boost::shared_ptr<DeviceSensor_t> pSensor = _device->getSensor(strToInt(sensorIndex));
        sensorType = pSensor->m_sensorType;
      } catch (ItemNotFoundException& ex) {}
    }

    std::string sensorValue;
    if (m_properties.has("sensorValue")) {
      sensorValue = m_properties.get("sensorValue");
    }

    std::string sensorValueFloat;
    if (m_properties.has("sensorValueFloat")) {
      sensorValueFloat = m_properties.get("sensorValueFloat");
    }

    std::string typeName;
    SceneHelper::sensorName(sensorType, typeName);

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";SensorValue;" +
        typeName + " [" + intToString(sensorType) + '/' + sensorIndex + "];" +
        sensorValueFloat + " [" + sensorValue + "];" +
        zoneName + ";;;" + devName + ";");
  }

  void SystemEventLog::logStateChangeScript(
                                    boost::shared_ptr<ScriptLogger> _logger,
                                    std::string _statename, std::string _state,
                                    std::string _value,
                                    std::string _origin_dsid) {

    std::string origName = getDeviceName(_origin_dsid);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";StateAddonScript;" + _statename + ";" + _value + ";" +
                   _state + ";;;;;" + origName + ";");
  }

  void SystemEventLog::logStateChangeApartment(
                                    boost::shared_ptr<ScriptLogger> _logger,
                                    std::string _statename, std::string _state,
                                    std::string _value,
                                    std::string _origin_dsid) {

    std::string origName = getDeviceName(_origin_dsid);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken'');
    _logger->logln(";StateApartment;" + _statename + ";" + _value + ";" +
                   _state  + ";;;;;" + origName + ";");
  }

  void SystemEventLog::logStateChangeDevice(
                                    boost::shared_ptr<ScriptLogger> _logger,
                                    std::string _statename, std::string _state,
                                    std::string _value,
                                    boost::shared_ptr<const Device> _device) {

    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";StateDevice;" + _statename + ";" + _value + ";" + _state +
                   ";;;;" + devName + ";");
  }

  void SystemEventLog::logStateChangeGroup(
                                    boost::shared_ptr<ScriptLogger> _logger,
                                    std::string _statename, std::string _state,
                                    std::string _value,
                                    int _group_id, int _zone_id) {
    try {
      boost::shared_ptr<Zone> zone =
              DSS::getInstance()->getApartment().getZone(_zone_id);

      std::string groupName = getGroupName(zone->getGroup(_group_id));
      std::string zoneName = getZoneName(zone);

      //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
      _logger->logln(";StateGroup;" + _statename + ";" + _value + ";" + _state +
                     ";" + zoneName + ";" + intToString(_zone_id) + ';' +
                     groupName + ";;");
    } catch (ItemNotFoundException &ex) {}
  }

  void SystemEventLog::model_ready(boost::shared_ptr<ScriptLogger> _logger) {
    _logger->logln("Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken");

    std::vector<boost::shared_ptr<Zone> > zones = DSS::getInstance()->getApartment().getZones();
    for (size_t z = 0; z < zones.size(); z++) {
      if (!zones.at(z)->isPresent()) {
        continue;
      }

      std::vector<boost::shared_ptr<Group> > groups = zones.at(z)->getGroups();
      for (size_t g = 0; g < groups.size(); g++) {
        int group_id = groups.at(g)->getID();
        if (((group_id > 0) && (group_id < GroupIDStandardMax)) ||
            (group_id == GroupIDControlTemperature)) {
          int scene_id = groups.at(g)->getLastCalledScene();
          if (scene_id >= 0) {
            logLastScene(_logger, zones.at(z), groups.at(g), scene_id);
          }
        }
      }
    }
  }

  void SystemEventLog::callScene(boost::shared_ptr<ScriptLogger> _logger) {
    int sceneId = -1;
    if (m_properties.has("sceneID")) {
        sceneId = strToIntDef(m_properties.get("sceneID"), -1);
    }

    bool isForced = false;
    if (m_properties.has("forced")) {
        if (m_properties.get("forced") == "true") {
          isForced = true;
        }
    }
    int zoneId = -1;
    std::string token;
    if (m_properties.has("originToken")) {
      token = m_properties.get("originToken");
    }

    std::string originDSID;
    if (m_properties.has(ss_originDSID)) {
      originDSID = m_properties.get(ss_originDSID);
    }

    callOrigin_t callOrigin = coUnknown;
    if (m_properties.has(ss_callOrigin)) {
      callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ss_callOrigin), 0);
    }

    if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
      // ZoneGroup Action Request
      zoneId = m_raisedAtGroup->getZoneID();
      int groupId = m_raisedAtGroup->getID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);

        logZoneGroupScene(_logger, zone, groupId, sceneId, isForced,
                          originDSID, callOrigin, token);
      } catch (ItemNotFoundException &ex) {}
    } else if ((m_evtRaiseLocation == erlDevice) &&
               (m_raisedAtDevice != NULL)) {
      zoneId = m_raisedAtDevice->getDevice()->getZoneID();

      if (callOrigin == coUnknown) {
        // DeviceLocal Action Event
        logDeviceLocalScene(_logger, sceneId,
                        dsuid2str(m_raisedAtDevice->getDevice()->getDSID()),
                        callOrigin);
      } else {
        // Device Action Request
        try {
          boost::shared_ptr<Zone> zone =
              DSS::getInstance()->getApartment().getZone(zoneId);
          logDeviceScene(_logger, m_raisedAtDevice->getDevice(), zone, sceneId,
                         isForced, originDSID, callOrigin, token);
        } catch (ItemNotFoundException &ex) {}
      }
    }
  }

  void SystemEventLog::blink(boost::shared_ptr<ScriptLogger> _logger) {
    int zoneId = -1;
    std::string token;
    if (m_properties.has("originToken")) {
      token = m_properties.get("originToken");
    }

    std::string originDSID;
    if (m_properties.has(ss_originDSID)) {
      originDSID = m_properties.get(ss_originDSID);
    }

    callOrigin_t callOrigin;
    if (m_properties.has(ss_callOrigin)) {
      callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ss_callOrigin), 0);
    }

    if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
      zoneId = m_raisedAtGroup->getZoneID();
      int groupId = m_raisedAtGroup->getID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);
        logZoneGroupBlink(_logger, zone, groupId, originDSID, callOrigin, token);
      } catch (ItemNotFoundException &ex) {}
    } else if ((m_evtRaiseLocation == erlDevice) &&
               (m_raisedAtDevice != NULL)) {
      zoneId = m_raisedAtDevice->getDevice()->getZoneID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);
        logDeviceBlink(_logger, m_raisedAtDevice->getDevice(), zone,
                       originDSID, callOrigin, token);
      } catch (ItemNotFoundException &ex) {}
    }
  }

  void SystemEventLog::undoScene(boost::shared_ptr<ScriptLogger> _logger) {
    int sceneId = -1;
    if (m_properties.has("sceneID")) {
        sceneId = strToIntDef(m_properties.get("sceneID"), -1);
    }

    std::string token;
    if (m_properties.has("originToken")) {
      token = m_properties.get("originToken");
    }

    std::string originDSID;
    if (m_properties.has(ss_originDSID)) {
      originDSID = m_properties.get(ss_originDSID);
    }

    callOrigin_t callOrigin = coUnknown;
    if (m_properties.has(ss_callOrigin)) {
      callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ss_callOrigin), 0);
    }

    if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
      int groupId = m_raisedAtGroup->getID();
      int zoneId = m_raisedAtGroup->getZoneID();
      try {
        boost::shared_ptr<Zone> zone =
                            DSS::getInstance()->getApartment().getZone(zoneId);
        logZoneGroupUndo(_logger, zone, groupId, sceneId, originDSID,
                         callOrigin, token);
      } catch (ItemNotFoundException &ex) {}
    }
  }

  void SystemEventLog::buttonClick(boost::shared_ptr<ScriptLogger> _logger) {
    if (m_raisedAtDevice != NULL) {
      logDeviceButtonClick(_logger, m_raisedAtDevice->getDevice());
    }
  }

  void SystemEventLog::deviceBinaryInputEvent(
                                    boost::shared_ptr<ScriptLogger> _logger) {
    if (m_raisedAtDevice != NULL) {
      logDeviceBinaryInput(_logger, m_raisedAtDevice->getDevice());
    }
  }

  void SystemEventLog::deviceSensorEvent(
                                    boost::shared_ptr<ScriptLogger> _logger) {
    if (m_raisedAtDevice != NULL) {
      logDeviceSensorEvent(_logger, m_raisedAtDevice->getDevice());
    }
  }

  void SystemEventLog::deviceSensorValue(
                                    boost::shared_ptr<ScriptLogger> _logger) {
    if (m_raisedAtDevice != NULL) {
      int zoneId = m_raisedAtDevice->getDevice()->getZoneID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);
        logDeviceSensorValue(_logger, m_raisedAtDevice->getDevice(), zone);
      } catch (ItemNotFoundException &ex) {}
    }
  }

  void SystemEventLog::stateChange(boost::shared_ptr<ScriptLogger> _logger) {
    std::string statename;
    if (m_properties.has("statename")) {
      statename = m_properties.get("statename");
    }

    std::string state;
    if (m_properties.has("state")) {
      state = m_properties.get("state");
    }

    std::string value;
    if (m_properties.has("value")) {
      value = m_properties.get("value");
    }

    std::string originDSID;
    if (m_properties.has("originDSID")) {
      originDSID = m_properties.get("originDSID");
    }

    if ((m_evtRaiseLocation == erlState) && (m_raisedAtState != NULL)) {
      if (m_raisedAtState->getType() == StateType_Script) {
        logStateChangeScript(_logger, statename, state, value, originDSID);
      } else if (m_raisedAtState->getType() == StateType_Service) {
        logStateChangeApartment(_logger, statename, state, value,
                                originDSID);
      } else if (m_raisedAtState->getType() == StateType_Device) {
        boost::shared_ptr<Device> device = m_raisedAtState->getProviderDevice();
        logStateChangeDevice(_logger, statename, state, value, device);
      } else if (m_raisedAtState->getType() == StateType_Group) {
        boost::shared_ptr<Group> group = m_raisedAtState->getProviderGroup();
        int groupID = group->getID();
        int zoneID = group->getZoneID();
        logStateChangeGroup(_logger, statename, state, value, groupID, zoneID);
      }
    }
  }

  void SystemEventLog::run() {
    if (DSS::hasInstance()) {
      DSS::getInstance()->getSecurity().loginAsSystemUser(
        "SystemEventLog needs system rights");
    } else {
      return;
    }

    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(
                                DSS::getInstance()->getJSLogDirectory(),
                                "system-event.log", NULL));

    if (logger == NULL) {
      Logger::getInstance()->log("SystemEventLog::run(): could not init "
                                 "logger!");
      return;
    }

    if (m_evtName == "model_ready") {
      model_ready(logger);
    } else if (m_evtName == "callScene") {
      callScene(logger);
    } else if (m_evtName == "blink") {
      blink(logger);
    } else if (m_evtName == "undoScene") {
      undoScene(logger);
    } else if (m_evtName == "buttonClick") {
      buttonClick(logger);
    } else if (m_evtName == "deviceBinaryInputEvent") {
      deviceBinaryInputEvent(logger);
    } else if (m_evtName == "deviceSensorEvent") {
      deviceSensorEvent(logger);
    } else if (m_evtName == "stateChange") {
      stateChange(logger);
    } else if (m_evtName == "deviceSensorValue") {
      logger.reset(new ScriptLogger(
          DSS::getInstance()->getJSLogDirectory(), "system-sensor.log", NULL));
      if (logger == NULL) {
        Logger::getInstance()->log("SystemEventLog::run(): could not init logger!");
        return;
      }
      deviceSensorValue(logger);
    }
  }

  bool SystemEventLog::setup(Event& _event) {
    m_evtName = _event.getName();
    m_evtRaiseLocation = _event.getRaiseLocation();
    m_raisedAtGroup = _event.getRaisedAtGroup(DSS::getInstance()->getApartment());
    m_raisedAtDevice = _event.getRaisedAtDevice();
    m_raisedAtState = _event.getRaisedAtState();
    return SystemEvent::setup(_event);
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

    boost::shared_ptr<SystemState> state(new SystemState());

    if (!state->setup(_event)) {
      Logger::getInstance()->log("EventInterpreterPluginSystemState::"
              "handleEvent: could not setup event data!");
      return;
    }

    addEvent(state);
  }


  SystemState::SystemState() : SystemEvent(), m_evtRaiseLocation(erlState) {
  }

  SystemState::~SystemState() {
  }

  boost::shared_ptr<State>  SystemState::registerState(std::string _name,
                                                       bool _persistent) {
    boost::shared_ptr<State> state =
            DSS::getInstance()->getApartment().allocateState(
                    StateType_Service, _name, "system_state");
    state->setPersistence(_persistent);
    return state;
  }

  boost::shared_ptr<State> SystemState::getOrRegisterState(std::string _name) {
    try {
      return DSS::getInstance()->getApartment().getState(
                StateType_Service, _name);
    } catch (ItemNotFoundException &ex) {
      return registerState(_name, true);
    }
  }

  void SystemState::bootstrap() {
    boost::shared_ptr<State> state;

    state = DSS::getInstance()->getApartment().allocateState(
        StateType_Service, "presence", "system_state");

    // Default presence status is "present" - set default before loading
    // old status from persistent storage
    state->setState(coSystem, State_Active);
    state->setPersistence(true);

    std::list<std::string> presenceValues;
    presenceValues.push_back("unknown");
    presenceValues.push_back("present");
    presenceValues.push_back("absent");
    presenceValues.push_back("invalid");
    state->setValueRange(presenceValues);

    state = DSS::getInstance()->getApartment().allocateState(
        StateType_Service, "hibernation", "system_state");
    state->setPersistence(true);
    std::list<std::string> sleepmodeValues;
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
  }

  void SystemState::startup() {
    bool absent = false;
    bool panic = false;
    bool alarm = false;
    bool sleeping = false;

    std::vector<boost::shared_ptr<Device> > devices =
            DSS::getInstance()->getApartment().getDevicesVector();

    for (size_t i = 0; i < devices.size(); i++) {
      boost::shared_ptr<Device> device = devices.at(i);
      if (device == NULL) {
        continue;
      }

      const std::vector<boost::shared_ptr<DeviceBinaryInput_t> > bInputs =
          device->getBinaryInputs();
      for (size_t j = 0; j < bInputs.size(); j++) {
        boost::shared_ptr<DeviceBinaryInput_t> input = bInputs.at(j);

        // motion
        if ((input->m_inputType == BinaryInputIDMovement) ||
            (input->m_inputType == BinaryInputIDMovementInDarkness)) {
          std::string stateName;
          if (input->m_targetGroupId >= 16) {
            stateName = "zone.0.group." + intToString(input->m_targetGroupId) +
                        ".motion";
          } else {
            stateName = "zone." + intToString(device->getZoneID()) + ".motion";
          }
          getOrRegisterState(stateName);
        }

        // presence
        if ((input->m_inputType == BinaryInputIDPresence) ||
            (input->m_inputType == BinaryInputIDPresenceInDarkness)) {
          std::string stateName;
          if (input->m_targetGroupId >= 16) {
            stateName = "zone.0.group." + intToString(input->m_targetGroupId) +
                        ".presence";
          } else {
            stateName = "zone." + intToString(device->getZoneID()) +
                        ".presence";
          }
          getOrRegisterState(stateName);
        }

        // wind monitor
        if (input->m_inputType == BinaryInputIDWindDetector) {
          std::string stateName = "wind";
          if (input->m_targetGroupId >= 16) {
            stateName = stateName + ".group" +
                        intToString(input->m_targetGroupId);
            getOrRegisterState(stateName);
          }
        }

        // rain monitor
        if (input->m_inputType == BinaryInputIDRainDetector) {
          std::string stateName = "rain";
          if (input->m_targetGroupId >= 16) {
            stateName = stateName + ".group" +
                        intToString(input->m_targetGroupId);
            getOrRegisterState(stateName);
          }
        }
      } // per device binary inputs for loop
    } // devices for loop

    std::vector<boost::shared_ptr<Zone> > zones =
        DSS::getInstance()->getApartment().getZones();
    for (size_t z = 0; z < zones.size(); z++) {
      boost::shared_ptr<Zone> zone = zones.at(z);
      if ((zone == NULL) || (!zone->isPresent())) {
        continue;
      }

      std::vector<boost::shared_ptr<Group> > groups = zone->getGroups();
      for (size_t g = 0; g < groups.size(); g++) {
        boost::shared_ptr<Group> group = groups.at(g);
        if ((group->getID() >= 16) && (group->getID() <= 23)) {
            if (group->getStandardGroupID() == 2) {
              registerState("wind.group" + intToString(group->getID()), true);
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
    try {
      state = DSS::getInstance()->getApartment().getState(StateType_Service, "presence");
      if (state != NULL) {
        if ((absent == true) && (state->getState() == State_Inactive)) {
          state->setState(coJSScripting, "absent");
        } else if (absent == false) {
          state->setState(coJSScripting, "present");
        }
      } // presence state
    } catch (ItemNotFoundException &ex) {}

    try {
      state = DSS::getInstance()->getApartment().getState(StateType_Service, "hibernation");
      if (state != NULL) {
        if ((sleeping == true) && (state->getState() == State_Inactive)) {
          state->setState(coJSScripting, "sleeping");
        } else if (absent == false) {
          state->setState(coJSScripting, "awake");
        }
      } // hibernation state
    } catch (ItemNotFoundException &ex) {}

    try {
      state = DSS::getInstance()->getApartment().getState(StateType_Service, "panic");
      if (state != NULL) {
        if ((panic == true) && (state->getState() == State_Inactive)) {
          state->setState(coJSScripting, State_Active);
        } else if (panic == false) {
          state->setState(coJSScripting, State_Inactive);
        }
      } // panic state
    } catch (ItemNotFoundException &ex) {}

    try {
      state = DSS::getInstance()->getApartment().getState(StateType_Service, "alarm");
      if (state != NULL) {
        if ((alarm == true) && (state->getState() == State_Inactive)) {
          state->setState(coJSScripting, State_Active);
        } else if (alarm == false) {
          state->setState(coJSScripting, State_Inactive);
        }
      } // alarm state
    } catch (ItemNotFoundException &ex) {}

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
    boost::shared_ptr<Event> event(new Event("system-addon-scene-responder.config"));
    event->setProperty("actions", "save");
    event->setProperty("value", CLEAR_ALARM_URLENCODED_JSON);
    DSS::getInstance()->getEventQueue().pushEvent(event);
  }

    std::string SystemState::getData(int *zoneId, int *groupId, int *sceneId, callOrigin_t *callOrigin) {
    std::string originDSID;

    *callOrigin = coUnknown;
    if (m_properties.has(ss_callOrigin)) {
        *callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ss_callOrigin), 0);
    }

    if (!zoneId || !groupId || !sceneId) {
      return originDSID;
    }

    *zoneId = -1;
    *groupId = -1;
    *sceneId = -1;

    if (m_properties.has("sceneID")) {
        *sceneId = strToIntDef(m_properties.get("sceneID"), -1);
    }

    if (m_properties.has(ss_originDSID)) {
      originDSID = m_properties.get(ss_originDSID);
    }

    if (((m_evtRaiseLocation == erlGroup) ||
         (m_evtRaiseLocation == erlApartment)) && (m_raisedAtGroup != NULL)) {
        *zoneId = m_raisedAtGroup->getZoneID();
        *groupId = m_raisedAtGroup->getID();
    } else if ((m_evtRaiseLocation == erlState) && (m_raisedAtState != NULL)) {
      if (m_raisedAtState->getType() == StateType_Device) {
        boost::shared_ptr<Device> device = m_raisedAtState->getProviderDevice();
        *zoneId = device->getZoneID();
        originDSID = dsuid2str(device->getDSID());
      } else if (m_raisedAtState->getType() == StateType_Group) {
        boost::shared_ptr<Group> group = m_raisedAtState->getProviderGroup();
        *zoneId = group->getZoneID();
        *groupId = group->getID();
      }
    } else if ((m_evtRaiseLocation == erlDevice) &&
               (m_raisedAtDevice != NULL)) {
      originDSID = dsuid2str(m_raisedAtDevice->getDSID());
    }

    return originDSID;
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
    if ((groupId == 0) && (sceneId == SceneAbsent)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "presence");
        state->setState(coSystem, "absent");
      } catch (ItemNotFoundException &ex) {}
      try {
        // #2561: auto-clear panic and fire
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "panic");
        if (state != NULL) {
          if (state->getState() == State_Active) {
            boost::shared_ptr<Zone> z =
                DSS::getInstance()->getApartment().getZone(0);
            if (z != NULL) {
              boost::shared_ptr<Group> g = z->getGroup(0);
              g->undoScene(coSystem, SAC_MANUAL, ScenePanic, "");
            }
          }
        }
      } catch (ItemNotFoundException &ex) {}
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "fire");
        if (state != NULL) {
          if (state->getState() == State_Active) {
            boost::shared_ptr<Zone> z =
                DSS::getInstance()->getApartment().getZone(0);
            if (z != NULL) {
              boost::shared_ptr<Group> g = z->getGroup(0);
              g->undoScene(coSystem, SAC_MANUAL, SceneFire, "");
            }
          }
        }
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == ScenePresent)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "presence");
        state->setState(coSystem, "present");
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == SceneSleeping)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "hibernation");
        state->setState(coSystem, "sleeping");
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == SceneWakeUp)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "hibernation");
        state->setState(coSystem, "awake");
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == ScenePanic)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "panic");
        state->setState(coSystem, State_Active);
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == SceneFire)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "fire");
        state->setState(coSystem, State_Active);
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == SceneAlarm)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "alarm");
        state->setState(coSystem, State_Active);
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == SceneAlarm2)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "alarm2");
        state->setState(coSystem, State_Active);
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == SceneAlarm3)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "alarm3");
        state->setState(coSystem, State_Active);
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == SceneAlarm4)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "alarm4");
        state->setState(coSystem, State_Active);
      } catch (ItemNotFoundException &ex) {}
    } else if (sceneId == SceneWindActive) {
      if (groupId == 0) {
        state = getOrRegisterState("wind");
      } else if ((groupId >= 16) && (groupId <= 23)) {
        state = getOrRegisterState("wind.group" + intToString(groupId));
      }
      state->setState(coSystem, State_Active);
    } else if (sceneId == SceneWindInactive) {
      if (groupId == 0) {
        state = getOrRegisterState("wind");
      } else if ((groupId >= 16) && (groupId <= 23)) {
        state = getOrRegisterState("wind.group" + intToString(groupId));
      }
      state->setState(coSystem, State_Inactive);
    } else if (sceneId == SceneRainActive) {
      if (groupId == 0) {
        state = getOrRegisterState("rain");
      } else if ((groupId >= 16) && (groupId <= 23)) {
        state = getOrRegisterState("rain.group" + intToString(groupId));
      }
      state->setState(coSystem, State_Active);
    } else if (sceneId == SceneRainInactive) {
      if (groupId == 0) {
        state = getOrRegisterState("rain");
        state->setState(coSystem, State_Inactive);
        for (size_t grp = 16; grp <= 23; grp++) {
          try {
            state = DSS::getInstance()->getApartment().getState(
                                    StateType_Service,
                                    "rain.group" + intToString(groupId));
            state->setState(coSystem, State_Inactive);
          } catch (ItemNotFoundException &ex) {
          }
        }
      } else if ((groupId >= 16) && (groupId <= 23)) {
        state = getOrRegisterState("rain.group" + intToString(groupId));
        state->setState(coSystem, State_Inactive);
      }
    }
  }

  void SystemState::undoscene() {
    int zoneId = -1;
    int groupId = -1;
    int sceneId = -1;
    callOrigin_t callOrigin = coUnknown;
    std::string originDSID;
    boost::shared_ptr<State> state;

    originDSID = getData(&zoneId, &groupId, &sceneId, &callOrigin);

    dsuid_t dsuid = str2dsuid(originDSID);
    if ((groupId == 0) && (sceneId == ScenePanic)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "panic");
        state->setState(coSystem, State_Inactive);

        // #2561: auto-reset fire if panic was reset by a button
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "fire");
        if (!IsNullDsuid(dsuid) && (callOrigin == coDsmApi) &&
            (state->getState() == State_Active)) {
          boost::shared_ptr<Zone> z = DSS::getInstance()->getApartment().getZone(0);
          if (z != NULL) {
            boost::shared_ptr<Group> g = z->getGroup(0);
            g->undoScene(coSystem, SAC_MANUAL, SceneFire, "");
          }
        } // valid dSID && dSM API origin && state == active
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == SceneFire)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "fire");
        state->setState(coSystem, State_Inactive);

        // #2561: auto-reset panic if fire was reset by a button
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "panic");
        if (!IsNullDsuid(dsuid) && (callOrigin == coDsmApi)
            && (state->getState() == State_Active)) {
          boost::shared_ptr<Zone> z = DSS::getInstance()->getApartment().getZone(0);
          if (z != NULL) {
            boost::shared_ptr<Group> g = z->getGroup(0);
            g->undoScene(coSystem, SAC_MANUAL, ScenePanic, "");
          }
        } // valid dSID && dSM API origin && state == active
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == SceneAlarm)) {
      try {
        state =  DSS::getInstance()->getApartment().getState(StateType_Service, "alarm");
        state->setState(coSystem, State_Inactive);
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == SceneAlarm2)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "alarm2");
        state->setState(coSystem, State_Inactive);
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == SceneAlarm3)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "alarm3");
        state->setState(coSystem, State_Inactive);
      } catch (ItemNotFoundException &ex) {}
    } else if ((groupId == 0) && (sceneId == SceneAlarm4)) {
      try {
        state = DSS::getInstance()->getApartment().getState(StateType_Service, "alarm4");
        state->setState(coSystem, State_Inactive);
      } catch (ItemNotFoundException &ex) {}
    }
  }

  void SystemState::stateBinaryInputGeneric(std::string stateName,
                                            int targetGroupType,
                                            int targetGroupId) {
      try {
        boost::shared_ptr<State> state =
            DSS::getInstance()->getApartment().getState(StateType_Service,
                                                        stateName);

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
            state->setState(coSystemBinaryInput, State_Active);
          } else if (iVal == 2) {
            if (pNode->getIntegerValue() > 0) {
              pNode->setIntegerValue(pNode->getIntegerValue() - 1);
            }
            if (pNode->getIntegerValue() == 0) {
              state->setState(coSystemBinaryInput, State_Inactive);
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
      if (devInput->m_targetGroupId >= 16) {
        // create state for a user group if it does not exist (new group?)
        statename = "zone.0.group." + intToString(devInput->m_targetGroupId) + ".motion";
      } else {
        // set motion state in the zone the dsid is logical attached to
        statename = "zone." + intToString(pDev->getZoneID()) + ".motion";
      }
      getOrRegisterState(statename);
      stateBinaryInputGeneric(statename, devInput->m_targetGroupType,
                              devInput->m_targetGroupId);
    }

    // presence
    if ((devInput->m_inputType == BinaryInputIDPresence) ||
        (devInput->m_inputType == BinaryInputIDPresenceInDarkness)) {
      if (devInput->m_targetGroupId >= 16) {
        // create state for a user group if it does not exist (new group?)
        statename = "zone.0.group." + intToString(devInput->m_targetGroupId) + ".presence";
      } else {
        // set presence state in the zone the dsid is logical attached to
        statename = "zone." + intToString(pDev->getZoneID()) + ".presence";
      }
      getOrRegisterState(statename);
      stateBinaryInputGeneric(statename, devInput->m_targetGroupType,
                              devInput->m_targetGroupId);
    }

    // smoke detector
    if (devInput->m_inputType == BinaryInputIDSmokeDetector) {
      try {
        boost::shared_ptr<State> state =
            DSS::getInstance()->getApartment().getState(StateType_Service, "fire");
        if (m_properties.has("value")) {
          std::string val = m_properties.get("value");
          int iVal = strToIntDef(val, -1);
          if (iVal == 1) {
            state->setState(coSystemBinaryInput, State_Active);
          }
        }
      } catch (ItemNotFoundException &ex) {}
    }

    // wind monitor
    if (devInput->m_inputType == BinaryInputIDWindDetector) {
      statename = "wind";
      // create state for a user group if it does not exist (new group?)
      if (devInput->m_targetGroupId >= 16) {
        statename = statename + ".group" + intToString(devInput->m_targetGroupId);
        getOrRegisterState(statename);
      }
      stateBinaryInputGeneric(statename, devInput->m_targetGroupType,
                              devInput->m_targetGroupId);
    }

    // rain monitor
    if (devInput->m_inputType == BinaryInputIDRainDetector) {
      statename = "rain";
      // create state for a user group if it does not exist (new group?)
      if (devInput->m_targetGroupId >= 16) {
        statename = statename + ".group" + intToString(devInput->m_targetGroupId);
        getOrRegisterState(statename);
      }
      stateBinaryInputGeneric(statename, devInput->m_targetGroupType,
                              devInput->m_targetGroupId);
    }

    // zone thermostat
    if (devInput->m_inputType == BinaryInputIDRoomThermostat) {
      int zone = pDev->getZoneID();
      boost::shared_ptr<Zone> pZone = DSS::getInstance()->getApartment().getZone(zone);
      boost::shared_ptr<Group> pGroup = pZone->getGroup(GroupIDHeating);
      if (m_properties.has("value")) {
        std::string val = m_properties.get("value");
        int iVal = strToIntDef(val, -1);
        int sceneID = SceneOff;
        if (iVal == 1) {
          sceneID = Scene1;
        }
        pGroup->callScene(coSystemBinaryInput, SAC_MANUAL, sceneID, "", false);
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
    boost::shared_ptr<Zone> z = DSS::getInstance()->getApartment().getZone(0);

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
        boost::shared_ptr<Group> g = z->getGroup(0);
        if (g != NULL) {
          g->callScene(coSystem, SAC_MANUAL, SceneFire, "", false);
        }
      }
    }

    if (statename.substr(0, 4) == "rain") {
      boost::shared_ptr<Group> g = z->getGroup(groupId);
      if (g != NULL) {
        if (iVal == 1) {
          g->callScene(coSystem, SAC_MANUAL, SceneRainActive, "", false);
        } else if (iVal == 2) {
          g->callScene(coSystem, SAC_MANUAL, SceneRainInactive, "", false);
        }
      }
    }

    if (statename.substr(0, 4) == "wind") {
      boost::shared_ptr<Group> g = z->getGroup(groupId);
      if (g != NULL) {
        if (iVal == 1) {
          g->callScene(coSystem, SAC_MANUAL, SceneWindActive, "", false);
        } else if (iVal == 2) {
          g->callScene(coSystem, SAC_MANUAL, SceneWindInactive, "", false);
        }
      }
    }
  }

  bool SystemState::setup(Event& _event) {
    m_evtName = _event.getName();
    m_evtRaiseLocation = _event.getRaiseLocation();
    m_raisedAtGroup = _event.getRaisedAtGroup(DSS::getInstance()->getApartment());
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
      if (m_evtName == "running") {
        bootstrap();
      } else if (m_evtName == "model_ready") {
        startup();
      } else if (m_evtName == "callScene") {
        if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
          callscene();
        }
      } else if (m_evtName == "undoScene") {
        undoscene();
      } else if (m_evtName == "stateChange") {
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
