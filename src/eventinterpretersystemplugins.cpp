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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include <signal.h>
#include <set>

#include "base.h"
#include "dss.h"
#include "event/event_create.h"
#include "event/event_fields.h"
#include "eventinterpretersystemplugins.h"
#include "http_client.h"
#include "internaleventrelaytarget.h"
#include "logger.h"
#include "model/set.h"
#include "model/zone.h"
#include "model/group.h"
#include "model/cluster.h"
#include "model/device.h"
#include "model/apartment.h"
#include "model/state.h"
#include "model/modelconst.h"
#include "model/scenehelper.h"
#include "propertysystem.h"
#include "security/security.h"
#include "structuremanipulator.h"
#include "systemcondition.h"
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

  const int SceneTableHeating_length = 6;
  const char *SceneTableHeating[] =
  {
    "Off",
    "Comfort",
    "Economy",
    "NotUsed",
    "Night",
    "Holiday",
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
      if (DSS::hasInstance()) {
        boost::shared_ptr<Event> pEvent;
        pEvent = createActionDenied("zone-scene",
                                    getActionName(_actionNode),
                                    m_properties.get("source-name", ""),
                                    e.what());

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
      if (DSS::hasInstance()) {
        boost::shared_ptr<Event> pEvent;
        pEvent = createActionDenied("zone-undo-scene",
                                    getActionName(_actionNode),
                                    m_properties.get("source-name", ""),
                                    e.what());
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
      PropertyNodePtr oDeviceNode = _actionNode->getPropertyByName("dsuid");
      if (oDeviceNode == NULL) {
        Logger::getInstance()->log("SystemEventActionExecute::"
              "executeDeviceScene: could call scene on device - missing dsuid",
              lsError);
        return device;
      }

      std::string dsuidStr = oDeviceNode->getStringValue();
      if (dsuidStr.empty()) {
        Logger::getInstance()->log("SystemEventActionExecute::"
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
      if (DSS::hasInstance()) {
        boost::shared_ptr<Event> pEvent;
        pEvent = createActionDenied("device-scene",
                                    getActionName(_actionNode),
                                    m_properties.get("source-name", ""),
                                    e.what());
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
      pEvent.reset(new Event(EventName::ExecutionDenied));
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
      pEvent.reset(new Event(EventName::ExecutionDenied));
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
      pEvent.reset(new Event(EventName::ExecutionDenied));
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

    boost::shared_ptr<Event> evt = boost::make_shared<Event>("highlevelevent");
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

    boost::shared_ptr<HttpClient> url = boost::make_shared<HttpClient>();
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

  void SystemEventActionExecute::executeHeatingMode(PropertyNodePtr _actionNode) {
    PropertyNodePtr oZoneNode = _actionNode->getPropertyByName("zone");
    if (oZoneNode == NULL) {
      Logger::getInstance()->log("SystemEventActionExecute::"
              "executeAddonStateChange - missing zone parameter", lsError);
      return;
    }

    PropertyNodePtr oModeNode = _actionNode->getPropertyByName("mode");
    if (oModeNode == NULL) {
      Logger::getInstance()->log("SystemEventActionExecute::"
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
        } else if (sActionType == "heating-mode") {
          executeHeatingMode(_actionNode);
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

  bool SystemTrigger::checkTrigger(std::string _path) {
    if (!DSS::hasInstance()) {
      return false;
    }
    PropertyNodePtr appProperty =
      DSS::getInstance()->getPropertySystem().getProperty(_path);
    if (appProperty == false) {
      return false;
    }

    PropertyNodePtr appTrigger = appProperty->getPropertyByName(ef_triggers);
    if (appTrigger == NULL) {
      return false;
    }

    for (int i = 0; i < appTrigger->getChildCount(); i++) {
      PropertyNodePtr triggerProp = appTrigger->getChild(i);
      if (triggerProp == NULL) {
        continue;
      }

      PropertyNodePtr triggerType = triggerProp->getPropertyByName(ef_type);
      if (triggerType == NULL) {
        continue;
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

    int i;
    PropertyNodePtr triggerPathNode;

    try {
      for (i = 0; i < triggerProperty->getChildCount(); i++) {
        PropertyNodePtr triggerNode = triggerProperty->getChild(i);
        if (triggerNode == NULL) {
          continue;
        }
        triggerPathNode = triggerNode->getPropertyByName("triggerPath");
        if (triggerPathNode == NULL) {
          continue;
        }
        std::string sTriggerPath = triggerPathNode->getStringValue();
        if (checkTrigger(sTriggerPath) && checkSystemCondition(sTriggerPath)) {
          relayTrigger(triggerNode);
        }
      } // for loop
    } catch (PropertyTypeMismatch& e) {
      Logger::getInstance()->log("SystemTrigger::run: error parsing trigger at " +
          triggerPathNode->getDisplayName(), lsInfo);
    } catch (std::runtime_error& e) {
      Logger::getInstance()->log("SystemTrigger::run: runtime error at " +
          triggerPathNode->getDisplayName(), lsInfo);
    }
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

    boost::shared_ptr<SystemEventActionExecute> action = boost::make_shared<SystemEventActionExecute>();
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

    boost::shared_ptr<SystemTrigger> trigger = boost::make_shared<SystemTrigger>();

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


    boost::shared_ptr<SystemTrigger> trigger = boost::make_shared<SystemTrigger>();
    if (trigger->setup(_event) == false) {
      Logger::getInstance()->log("EventInterpreterPluginHighlevelEvent::"
              "handleEvent: could not setup event data for SystemTrigger!");
      return;
    }

    boost::shared_ptr<SystemEventHighlevel> hl = boost::make_shared<SystemEventHighlevel>();
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

    boost::shared_ptr<SystemEventLog> log = boost::make_shared<SystemEventLog>();

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

  std::string SystemEventLog::getSceneName(int _sceneID, int _groupID) {
    std::string sceneName = "Unknown";

    if (_groupID == GroupIDControlTemperature) {
      if (_sceneID >= 0 && _sceneID < SceneTableHeating_length) {
        sceneName = SceneTableHeating[_sceneID];
      }
    } else {
      if ((_sceneID >= 0) && (_sceneID < 64) && (_sceneID < SceneTable0_length)) {
        sceneName = SceneTable0[_sceneID];
      }
      if ((_sceneID >= 64) && ((_sceneID - 64) < SceneTable1_length)) {
        sceneName = SceneTable1[_sceneID - 64];
      }
    }

    sceneName += ";" + intToString(_sceneID);

    return sceneName;
  }

  std::string SystemEventLog::getDeviceName(std::string _origin_dsuid) {
    std::string devName = "Unknown";

    if (!_origin_dsuid.empty()) {
      try {
        boost::shared_ptr<Device> device =
            DSS::getInstance()->getApartment().getDeviceByDSID(
                                str2dsuid(_origin_dsuid));
        if (device && (!device->getName().empty())) {
            devName = device->getName();
        }
      } catch (ItemNotFoundException &ex) {};
    }

    devName += ";" + _origin_dsuid;
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
    std::string sceneName = getSceneName(_scene_id, _group->getID());

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";Last Scene;" + sceneName + ';' + zoneName + ';' +
                  groupName + ";;;");
  }

  void SystemEventLog::logZoneGroupScene(boost::shared_ptr<ScriptLogger> _logger,
                                         boost::shared_ptr<Zone> _zone,
                                         int _group_id, int _scene_id,
                                         bool _is_forced,
                                         std::string _origin_dsuid,
                                         callOrigin_t _call_origin,
                                         std::string _origin_token) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_zone->getGroup(_group_id));
    std::string sceneName = getSceneName(_scene_id, _group_id);
    std::string devName = getDeviceName(_origin_dsuid);
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
                                        std::string _origin_dsuid,
                                        callOrigin_t _call_origin) {
    std::string devName = getDeviceName(_origin_dsuid);
    std::string sceneName = getSceneName(_scene_id, -1);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;CallOrigin;originToken');
    _logger->logln(";Device;" + sceneName + ";;;;;" + devName + ";" +
                   getCallOrigin(_call_origin) + ";");
  }

  void SystemEventLog::logDeviceScene(boost::shared_ptr<ScriptLogger> _logger,
                                      boost::shared_ptr<const Device> _device,
                                      boost::shared_ptr<Zone> _zone,
                                      int _scene_id, bool _is_forced,
                                      std::string _origin_dsuid,
                                      callOrigin_t _call_origin,
                                      std::string _token) {
    std::string zoneName = getZoneName(_zone);
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());
    std::string origName = getDeviceName(_origin_dsuid);
    std::string sceneName = getSceneName(_scene_id, -1);

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
                                        std::string _origin_dsuid,
                                        callOrigin_t _call_origin,
                                        std::string _origin_token) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_zone->getGroup(_group_id));
    std::string devName = getDeviceName(_origin_dsuid);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;CallOrigin;originToken');
    _logger->logln(";Blink;;" + zoneName + ";" + groupName + ";" + devName + 
                   ";;" + getCallOrigin(_call_origin) + ";" +_origin_token);
  }

  void SystemEventLog::logDeviceBlink(boost::shared_ptr<ScriptLogger> _logger,
                                      boost::shared_ptr<const Device> _device,
                                      boost::shared_ptr<Zone> _zone,
                                      std::string _origin_dsuid,
                                      callOrigin_t _call_origin,
                                      std::string _origin_token) {
    std::string zoneName = getZoneName(_zone);
    std::string devName = _device->getName() + ";" +
                          dsuid2str(_device->getDSID());
    std::string origName = getDeviceName(_origin_dsuid);
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Device;Device-ID;Origin;Origin-ID;CallOrigin;originToken');
    _logger->logln(";DeviceBlink;;" + zoneName + ";" + devName + ";;" +
                   origName + ";" + getCallOrigin(_call_origin) + ";" + _origin_token);
  }

  void SystemEventLog::logZoneGroupUndo(boost::shared_ptr<ScriptLogger> _logger,
                                        boost::shared_ptr<Zone> _zone,
                                        int _group_id, int _scene_id,
                                        std::string _origin_dsuid,
                                        callOrigin_t _call_origin,
                                        std::string _origin_token) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_zone->getGroup(_group_id));
    std::string devName = getDeviceName(_origin_dsuid);
    std::string sceneName = getSceneName(_scene_id, _group_id);

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

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";SensorValue;" + SceneHelper::sensorName(sensorType) +
        " [" + intToString(sensorType) + '/' + sensorIndex + "];" +
        sensorValueFloat + " [" + sensorValue + "];" +
        zoneName + ";;;" + devName + ";");
  }

  void SystemEventLog::logZoneSensorValue(
      boost::shared_ptr<ScriptLogger> _logger,
      boost::shared_ptr<Zone> _zone,
      int _groupId) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_zone->getGroup(_groupId));
    uint8_t sensorType = strToInt(m_properties.get("sensorType"));
    std::string sensorValue = m_properties.get("sensorValue");
    std::string sensorValueFloat = m_properties.get("sensorValueFloat");

    std::string typeName = SceneHelper::sensorName(sensorType);
    std::string origName = getDeviceName(m_properties.get("originDSID"));

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";ZoneSensorValue;" +
        typeName + " [" + intToString(sensorType) + "];" +
        sensorValueFloat + " [" + sensorValue + "];" +
        zoneName + ";" + groupName + ";" + origName + ";");
    }

    void SystemEventLog::logStateChange(
      boost::shared_ptr<ScriptLogger> _logger,
      boost::shared_ptr<const State> _st,
      const std::string& _statename, const std::string& _state,
      const std::string& _value, const std::string& _origin_device_id) {

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken'');

    if (m_raisedAtState->getType() == StateType_Service) {
      std::string origName = getDeviceName(_origin_device_id);
      _logger->logln(";StateApartment;" + _statename + ";" + _value + ";" + _state  + ";;;;;" + origName + ";");

    } else if (m_raisedAtState->getType() == StateType_Device) {
      boost::shared_ptr<Device> device = m_raisedAtState->getProviderDevice();
      std::string devName = device->getName() + ";" + dsuid2str(device->getDSID());
      _logger->logln(";StateDevice;" + _statename + ";" + _value + ";" + _state + ";;;;" + devName + ";");

    } else if (m_raisedAtState->getType() == StateType_Group) {
      boost::shared_ptr<Group> group = m_raisedAtState->getProviderGroup();
      int groupID = group->getID();
      int zoneID = group->getZoneID();
      boost::shared_ptr<Zone> zone = DSS::getInstance()->getApartment().getZone(zoneID);
      std::string groupName = getGroupName(zone->getGroup(groupID));
      std::string zoneName = getZoneName(zone);
      _logger->logln(";StateGroup;" + _statename + ";" + _value + ";" + _state + ";" +
          zoneName + ";" + groupName + ";;");

    } else if (m_raisedAtState->getType() == StateType_SensorDevice) {
      boost::shared_ptr<Device> device = m_raisedAtState->getProviderDevice();
      std::string devName = device->getName() + ";" + dsuid2str(device->getDSID());
      _logger->logln(";StateSensorDevice;" + _statename + ";" + _value + ";" + _state + ";;;;" + devName + ";");

    } else if (m_raisedAtState->getType() == StateType_SensorZone) {
      boost::shared_ptr<Group> group = m_raisedAtState->getProviderGroup();
      int groupID = group->getID();
      int zoneID = group->getZoneID();
      boost::shared_ptr<Zone> zone = DSS::getInstance()->getApartment().getZone(zoneID);
      std::string groupName = getGroupName(zone->getGroup(groupID));
      std::string zoneName = getZoneName(zone);
      _logger->logln(";StateSensorGroup;" + _statename + ";" + _value + ";" + _state + ";" +
          zoneName + ";" + groupName + ";;");

    } else {
      std::string origName = getDeviceName(_origin_device_id);
      _logger->logln(";StateScript;" + _statename + ";" + _value + ";" + _state + ";;;;;" + origName + ";");
    }
  }

  void SystemEventLog::logSunshine(boost::shared_ptr<ScriptLogger> _logger,
                                   std::string _value,
                                   std::string _direction,
                                   std::string _originDeviceID) {
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";Sunshine;" + _value + ";;" + _direction + ";;;;" + _originDeviceID + ";");
  }

  void SystemEventLog::logFrostProtection(boost::shared_ptr<ScriptLogger> _logger,
                                          std::string _value,
                                          std::string _originDeviceID) {
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";FrostProtection;" + _value + ";;;;;;" + _originDeviceID + ";");
  }

  void SystemEventLog::logHeatingModeSwitch(boost::shared_ptr<ScriptLogger> _logger,
                                          std::string _value,
                                          std::string _originDeviceID) {
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";HeatingModeSwitch;" + _value + ";;;;;;" + _originDeviceID + ";");
  }

  void SystemEventLog::logBuildingService(boost::shared_ptr<ScriptLogger> _logger,
                                          std::string _value,
                                          std::string _originDeviceID) {
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";BuildingService;" + _value + ";;;;;;" + _originDeviceID + ";");
  }

  void SystemEventLog::logExecutionDenied(boost::shared_ptr<ScriptLogger> _logger,
                                          std::string _action, std::string _reason) {
    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";ExecutionDenied;" + _action + ";" + _reason + ";;;;;;");
  }

  void SystemEventLog::logOperationLock(boost::shared_ptr<ScriptLogger> _logger,
                                        boost::shared_ptr<Zone> _zone,
                                        int _groupId,
                                        int _lock,
                                        callOrigin_t _call_origin) {
    std::string zoneName = getZoneName(_zone);
    std::string groupName = getGroupName(_zone->getGroup(_groupId));
    std::string origName = getCallOrigin(_call_origin);

    //l.logln('Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken');
    _logger->logln(";OperationLock;" +
        intToString(_lock) + ";;" +
        zoneName + ";" + groupName + ";" + origName + ";");
  }

  void SystemEventLog::model_ready() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-event.log", NULL));

    logger->logln("Time;Event;Action;Action-ID/Button Index;Zone;Zone-ID;Group;Group-ID;Origin;Origin-ID;originToken");

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
            logLastScene(logger, zones.at(z), groups.at(g), scene_id);
          }
        }
      }
    }
  }

  void SystemEventLog::callScene() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-event.log", NULL));
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

    std::string originDSUID;
    if (m_properties.has(ef_originDSUID)) {
      originDSUID = m_properties.get(ef_originDSUID);
    }

    callOrigin_t callOrigin = coUnknown;
    if (m_properties.has(ef_callOrigin)) {
      callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ef_callOrigin), 0);
    }

    if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
      // ZoneGroup Action Request
      zoneId = m_raisedAtGroup->getZoneID();
      int groupId = m_raisedAtGroup->getID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);

        logZoneGroupScene(logger, zone, groupId, sceneId, isForced,
                          originDSUID, callOrigin, token);
      } catch (ItemNotFoundException &ex) {}
    } else if ((m_evtRaiseLocation == erlDevice) &&
               (m_raisedAtDevice != NULL)) {
      zoneId = m_raisedAtDevice->getDevice()->getZoneID();

      if (callOrigin == coUnknown) {
        // DeviceLocal Action Event
        logDeviceLocalScene(logger, sceneId,
                        dsuid2str(m_raisedAtDevice->getDevice()->getDSID()),
                        callOrigin);
      } else {
        // Device Action Request
        try {
          boost::shared_ptr<Zone> zone =
              DSS::getInstance()->getApartment().getZone(zoneId);
          logDeviceScene(logger, m_raisedAtDevice->getDevice(), zone, sceneId,
                         isForced, originDSUID, callOrigin, token);
        } catch (ItemNotFoundException &ex) {}
      }
    }
  }

  void SystemEventLog::blink() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-event.log", NULL));
    int zoneId = -1;
    std::string token;
    if (m_properties.has("originToken")) {
      token = m_properties.get("originToken");
    }

    std::string originDSUID;
    if (m_properties.has(ef_originDSUID)) {
      originDSUID = m_properties.get(ef_originDSUID);
    }

    callOrigin_t callOrigin = coUnknown;
    if (m_properties.has(ef_callOrigin)) {
      callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ef_callOrigin), 0);
    }

    if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
      zoneId = m_raisedAtGroup->getZoneID();
      int groupId = m_raisedAtGroup->getID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);
        logZoneGroupBlink(logger, zone, groupId, originDSUID, callOrigin, token);
      } catch (ItemNotFoundException &ex) {}
    } else if ((m_evtRaiseLocation == erlDevice) &&
               (m_raisedAtDevice != NULL)) {
      zoneId = m_raisedAtDevice->getDevice()->getZoneID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);
        logDeviceBlink(logger, m_raisedAtDevice->getDevice(), zone,
                       originDSUID, callOrigin, token);
      } catch (ItemNotFoundException &ex) {}
    }
  }

  void SystemEventLog::undoScene() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-event.log", NULL));
    int sceneId = -1;
    if (m_properties.has("sceneID")) {
        sceneId = strToIntDef(m_properties.get("sceneID"), -1);
    }

    std::string token;
    if (m_properties.has("originToken")) {
      token = m_properties.get("originToken");
    }

    std::string originDSUID;
    if (m_properties.has(ef_originDSUID)) {
      originDSUID = m_properties.get(ef_originDSUID);
    }

    callOrigin_t callOrigin = coUnknown;
    if (m_properties.has(ef_callOrigin)) {
      callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ef_callOrigin), 0);
    }

    if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
      int groupId = m_raisedAtGroup->getID();
      int zoneId = m_raisedAtGroup->getZoneID();
      try {
        boost::shared_ptr<Zone> zone =
                            DSS::getInstance()->getApartment().getZone(zoneId);
        logZoneGroupUndo(logger, zone, groupId, sceneId, originDSUID,
                         callOrigin, token);
      } catch (ItemNotFoundException &ex) {}
    }
  }

  void SystemEventLog::buttonClick() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-event.log", NULL));
    if (m_raisedAtDevice != NULL) {
      logDeviceButtonClick(logger, m_raisedAtDevice->getDevice());
    }
  }

  void SystemEventLog::deviceBinaryInputEvent() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-event.log", NULL));
    if (m_raisedAtDevice != NULL) {
      logDeviceBinaryInput(logger, m_raisedAtDevice->getDevice());
    }
  }

  void SystemEventLog::deviceSensorEvent() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-event.log", NULL));
    if (m_raisedAtDevice != NULL) {
      logDeviceSensorEvent(logger, m_raisedAtDevice->getDevice());
    }
  }

  void SystemEventLog::deviceSensorValue() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-sensor.log", NULL));
    if (m_raisedAtDevice != NULL) {
      int zoneId = m_raisedAtDevice->getDevice()->getZoneID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);
        logDeviceSensorValue(logger, m_raisedAtDevice->getDevice(), zone);
      } catch (ItemNotFoundException &ex) {}
    }
  }

  void SystemEventLog::zoneSensorValue() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-sensor.log", NULL));
    if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
      // Zone Sensor Value
      int zoneId = m_raisedAtGroup->getZoneID();
      int groupId = m_raisedAtGroup->getID();
      try {
        boost::shared_ptr<Zone> zone =
            DSS::getInstance()->getApartment().getZone(zoneId);
        logZoneSensorValue(logger, zone, groupId);
      } catch (ItemNotFoundException &ex) {}
    }
  }

  void SystemEventLog::stateChange() {
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
    std::string originDSUID;
    if (m_properties.has("originDSUID")) {
      originDSUID = m_properties.get("originDSUID");
    }
    if ((m_evtRaiseLocation == erlState) && (m_raisedAtState != NULL)) {
      boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
          "system-event.log", NULL));
      try {
        logStateChange(logger, m_raisedAtState, statename, state, value, originDSUID);
      } catch (std::exception &ex) {}

      std::string name = statename.substr(0, statename.find("."));
      std::string location = statename.substr(statename.find(".") + 1);
      if (name == "wind" ||
          name == "rain" ||
          name == "hail" ||
          name == "panic" ||
          name == "fire" ||
          name == "alarm" ||
          name == "alarm2" ||
          name == "alarm3" ||
          name == "alarm4") {
        logger.reset(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
            "system-protection.log", NULL));
        try {
          name[0] = toupper(name[0]);
          if (!location.empty()) {
            std::string groupName("group");
            size_t groupPos = location.find(groupName);
            if (std::string::npos != groupPos) {
              groupPos += groupName.length();
              int clusterID = strToInt(location.substr(groupPos));
              boost::shared_ptr<Cluster> cluster = DSS::getInstance()->getApartment().getCluster(clusterID);
              std::string clusterName = cluster->getName();
              if (clusterName.empty()) {
                clusterName = "Cluster #" + intToString(clusterID);
              }
              logger->logln(name + " is " + state + " in " + clusterName);
            } else {
              logger->logln(name + " is " + state);
            }
          } else {
            logger->logln(name + " is " + state);
          }
        } catch (std::exception &ex) {}
      }
    }
  }

  void SystemEventLog::sunshine() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-event.log", NULL));
    std::string value;
    if (m_properties.has("value")) {
      value = m_properties.get("value");
    }
    std::string direction;
    if (m_properties.has("direction")) {
      direction = m_properties.get("direction");
    }
    std::string originDeviceID;
    if (m_properties.has("originDeviceID")) {
      originDeviceID = m_properties.get("originDeviceID");
    }
    try {
      logSunshine(logger, value, direction, originDeviceID);
    } catch (std::exception &ex) {}

    logger.reset(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-protection.log", NULL));
    try {
      int v = strToInt(value);
      std::string valueString;
      switch(v) {
        case 1: valueString = "active"; break;
        case 2: valueString = "inactive"; break;
        default: valueString = "unknown"; break;
      }
      CardinalDirection_t dir;
      parseCardinalDirection(direction, &dir);
      logger->logln("Sun protection from " + toUIString(dir) + " is " + valueString);
    } catch (std::exception &ex) {}
  }

  void SystemEventLog::frostProtection() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-event.log", NULL));
    std::string value;
    if (m_properties.has("value")) {
      value = m_properties.get("value");
    }
    std::string originDeviceID;
    if (m_properties.has("originDeviceID")) {
      originDeviceID = m_properties.get("originDeviceID");
    }
    try {
      logFrostProtection(logger, value, originDeviceID);
    } catch (ItemNotFoundException &ex) {} catch (std::exception &ex) {}

    logger.reset(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-protection.log", NULL));
    try {
      int v = strToInt(value);
      std::string valueString;
      switch(v) {
        case 1: valueString = "active"; break;
        case 2: valueString = "inactive"; break;
        default: valueString = "unknown"; break;
      }
      logger->logln("Frost protection is " + valueString);
    } catch (std::exception &ex) {}
  }

  void SystemEventLog::heatingModeSwitch() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-event.log", NULL));
    std::string value;
    if (m_properties.has("value")) {
      value = m_properties.get("value");
    }
    std::string originDeviceID;
    if (m_properties.has("originDeviceID")) {
      originDeviceID = m_properties.get("originDeviceID");
    }
    try {
      logHeatingModeSwitch(logger, value, originDeviceID);
    } catch (std::exception &ex) {}
  }

  void SystemEventLog::buildingService() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-event.log", NULL));
    std::string value;
    if (m_properties.has("value")) {
      value = m_properties.get("value");
    }
    std::string originDeviceID;
    if (m_properties.has("originDeviceID")) {
      originDeviceID = m_properties.get("originDeviceID");
    }
    try {
      logBuildingService(logger, value, originDeviceID);
    } catch (std::exception &ex) {}
    
    logger.reset(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-protection.log", NULL));
    try {
      int v = strToInt(value);
      std::string valueString;
      switch(v) {
        case 1: valueString = "active"; break;
        case 2: valueString = "inactive"; break;
        default: valueString = "unknown"; break;
      }
      logger->logln("Service protection is " + valueString);
    } catch (std::exception &ex) {}
  }

  void SystemEventLog::executionDenied() {
    boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-event.log", NULL));
    std::string action;
    std::string reason;
    if (m_properties.has("source-name")) {
      action = m_properties.get("source-name");
    }
    if (action.empty()) {
      if (m_properties.has("action-name")) {
        action = m_properties.get("action-name");
      }
    }
    if (m_properties.has("reason")) {
      reason = m_properties.get("reason");
    }

    try {
      logExecutionDenied(logger, action, reason);
    } catch (ItemNotFoundException &ex) {} catch (std::exception &ex) {}

    logger.reset(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
        "system-protection.log", NULL));
    try {
      logger->logln("Timer or automatic activity \"" + action + "\": " + reason);
    } catch (std::exception &ex) {}
  }

  void SystemEventLog::operationLock(const std::string& _evtName) {
    int lockStatus = -1;
    if (m_properties.has("lock")) {
      lockStatus = strToInt(m_properties.get("lock"));
    }

    callOrigin_t callOrigin = coUnknown;
    if (m_properties.has(ef_callOrigin)) {
      callOrigin = (callOrigin_t)strToIntDef(m_properties.get(ef_callOrigin), 0);
    }

    if ((m_evtRaiseLocation == erlGroup) && (m_raisedAtGroup != NULL)) {
      boost::shared_ptr<ScriptLogger> logger(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
          "system-event.log", NULL));
      int groupId, zoneId;
      try {
        groupId = m_raisedAtGroup->getID();
        zoneId = m_raisedAtGroup->getZoneID();
        boost::shared_ptr<Zone> zone = DSS::getInstance()->getApartment().getZone(zoneId);
        logOperationLock(logger, zone, groupId, lockStatus, callOrigin);
      } catch (ItemNotFoundException &ex) {} catch (std::exception &ex) {}

      logger.reset(new ScriptLogger(DSS::getInstance()->getJSLogDirectory(),
          "system-protection.log", NULL));
      try {
        boost::shared_ptr<Group> group = DSS::getInstance()->getApartment().getZone(zoneId)->getGroup(groupId);
        std::string lockString;
        switch (lockStatus) {
          case 0: lockString = "inactive"; break;
          case 1: lockString = "active"; break;
          default: lockString = "unknown"; break;
        }
        std::string gName = group->getName();
        if (gName.empty()) {
          gName = std::string("Cluster #") + intToString(groupId);
        }
        if (_evtName == EventName::OperationLock) {
          logger->logln("Operation lock " + lockString + " in " + gName);
        } else if (_evtName == EventName::ClusterConfigLock) {
          logger->logln("Configuration lock " + lockString + " in " + gName + " (from " + getCallOrigin(callOrigin) + ")");
        }
      } catch (ItemNotFoundException &ex) {} catch (std::exception &ex) {}
    }
  }

  void SystemEventLog::run() {
    if (DSS::hasInstance()) {
      DSS::getInstance()->getSecurity().loginAsSystemUser(
        "SystemEventLog needs system rights");
    } else {
      return;
    }

    if (m_evtName == EventName::ModelReady) {
      model_ready();
    } else if (m_evtName == EventName::CallScene) {
      callScene();
    } else if (m_evtName == EventName::IdentifyBlink) {
      blink();
    } else if (m_evtName == EventName::UndoScene) {
      undoScene();
    } else if (m_evtName == EventName::DeviceButtonClick) {
      buttonClick();
    } else if (m_evtName == EventName::DeviceBinaryInputEvent) {
      deviceBinaryInputEvent();
    } else if (m_evtName == EventName::DeviceSensorEvent) {
      deviceSensorEvent();
    } else if (m_evtName == EventName::StateChange) {
      stateChange();
    } else if (m_evtName == EventName::DeviceSensorValue) {
      deviceSensorValue();
    } else if (m_evtName == EventName::ZoneSensorValue) {
      zoneSensorValue();
    } else if (m_evtName == EventName::Sunshine) {
      sunshine();
    } else if (m_evtName == EventName::FrostProtection) {
      frostProtection();
    } else if (m_evtName == EventName::HeatingModeSwitch) {
      heatingModeSwitch();
    } else if (m_evtName == EventName::BuildingService) {
      buildingService();
    } else if (m_evtName == EventName::ExecutionDenied) {
      executionDenied();
    } else if (m_evtName == EventName::OperationLock || m_evtName == EventName::ClusterConfigLock) {
      operationLock(m_evtName);
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

} // namespace dss
