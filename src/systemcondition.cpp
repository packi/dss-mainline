/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

    Based on system_conditions.js, authors:
            Roman Köhler <roman.koehler@aizo.com>
            Michael Troß <michael.tross@aizo.com>

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

#include "systemcondition.h"
#include "dss.h"
#include "propertysystem.h"
#include "logger.h"
#include "datetools.h"
#include "base.h"

namespace dss {

  bool checkTimeCondition(PropertyNodePtr timeStartNode, PropertyNodePtr timeEndNode, int secNow) {
    int startSinceMidnight = 0;
    int endSinceMidnight = 24 * 60 * 60;

    // TODO use DateTools instead of ad-hoc parsing

    if (timeStartNode != NULL) {
      std::string sTime = timeStartNode->getAsString();
      std::vector<std::string> sTimeArr = splitString(sTime, ':');
      if (sTimeArr.size() == 3) {
        startSinceMidnight =
            (strToInt(sTimeArr.at(0)) * 60 * 60) +
            (strToInt(sTimeArr.at(1)) * 60) +
            strToInt(sTimeArr.at(2));
      }
    }
    if (timeEndNode != NULL) {
      std::string eTime = timeEndNode->getAsString();
      std::vector<std::string> eTimeArr = splitString(eTime, ':');
      if (eTimeArr.size() == 3) {
        endSinceMidnight =
            (strToInt(eTimeArr.at(0)) * 60 * 60) +
            (strToInt(eTimeArr.at(1)) * 60) +
            strToInt(eTimeArr.at(2));
      }
    }
    Logger::getInstance()->log("checkTimeCondition: "
        "start: " + intToString(startSinceMidnight) + ", "
        "end: " + intToString(endSinceMidnight) + ", "
        "now: " + intToString(secNow), lsDebug);

    if (startSinceMidnight < endSinceMidnight) {
      // 1. 10:00:00 - 12:00:00
      if ((secNow < startSinceMidnight) || (secNow > endSinceMidnight)) {
        return false;
      }
    } else {
      // 2. 22:00:00 - 04:00:00
      if ((secNow < startSinceMidnight) && (secNow > endSinceMidnight)) {
        return false;
      }
    }
    return true;
  } // checkTimeCondition

  bool checkSystemCondition(std::string _path) {
    PropertyNodePtr oBaseConditionNode =
        DSS::getInstance()->getPropertySystem().getProperty(
                _path + "/conditions");
    if (oBaseConditionNode != NULL) {
      PropertyNodePtr oEnabledNode = oBaseConditionNode->getPropertyByName("enabled");
      if (oEnabledNode != NULL) {
        if (oEnabledNode->getBoolValue() == false) {
          return false;
        }
      }

      // all system states must match
      PropertyNodePtr oStateNode = oBaseConditionNode->getPropertyByName("states");
      PropertyNodePtr oSystemStates =
          DSS::getInstance()->getPropertySystem().getProperty("/usr/states");

      if ((oStateNode != NULL) && (oSystemStates != NULL)) {

        for (int j = 0; j < oStateNode->getChildCount(); j++) {
          bool fFound = false;
          std::string sName = oStateNode->getChild(j)->getName();
          std::string sValue = oStateNode->getChild(j)->getAsString();
          for (int i = 0; i < oSystemStates->getChildCount(); i++) {
            PropertyNodePtr nameNode =
                oSystemStates->getChild(i)->getPropertyByName("name");
            PropertyNodePtr valueNode =
                oSystemStates->getChild(i)->getPropertyByName("value");
            if ((nameNode == NULL) || (valueNode == NULL)) {
              Logger::getInstance()->log("checkSystemCondition: can not check"
                    " condition, missing name or value node!", lsError);
              continue;
            }
            // search for a requested state
            if (sName == nameNode->getAsString()) {
              fFound = true;
              // state found ...
              if (sValue != valueNode->getAsString()) {
                Logger::getInstance()->log("checkSystemCondition: " +
                    sName + " failed: value is " + valueNode->getAsString() +
                    ", requested is " + sValue, lsDebug);
                return false;
              }
              break;
            }
          } // oSystemStates loop
          // state was requested as active - but not found in /usr/states
          if (fFound == false) {
            Logger::getInstance()->log("checkSystemCondition: " +
              sName + " requested but not found in system states!", lsError);
            return false;
          }
        } // oStateNode loop
      } // nodes NULL check

      PropertyNodePtr oAddonStateNode = oBaseConditionNode->getPropertyByName("addon-states");

      if ((oAddonStateNode != NULL)) {
        for (int j = 0; j < oAddonStateNode->getChildCount(); j++) {
          //assuming condi = /addon-states/<scriptID>/myFire=true
          std::string sAddonID = oAddonStateNode->getChild(j)->getName();
          PropertyNodePtr oAddonStateSubnode=oAddonStateNode->getChild(j);
          // searching for a specific addon subpath in /usr/addon-states/<scriptid>
          PropertyNodePtr oSystemAddonStates =
              DSS::getInstance()->getPropertySystem().getProperty("/usr/addon-states/" + sAddonID);

          if (oSystemAddonStates != NULL) {
            for (int k = 0; k < oAddonStateSubnode->getChildCount(); k++) {
              bool fFound = false;
              std::string sName = oAddonStateSubnode->getChild(k)->getName();
              std::string sValue = oAddonStateSubnode->getChild(k)->getAsString();
              for (int i = 0; i < oSystemAddonStates->getChildCount(); i++) {
                PropertyNodePtr nameNode =
                    oSystemAddonStates->getChild(i)->getPropertyByName("name");

                PropertyNodePtr valueNode =
                    oSystemAddonStates->getChild(i)->getPropertyByName("value");

                if ((nameNode == NULL) || (valueNode == NULL)) {
                  Logger::getInstance()->log("checkSystemAddonCondition: can not check"
                          " addon condition, missing name or value node!", lsError);
                  continue;
                }

                // search for a requested state
                if (sName == nameNode->getAsString()) {
                  fFound = true;
                  // state found ...
                  if (sValue != valueNode->getAsString()) {
                    Logger::getInstance()->log("checkSystemAddonCondition: " +
                            sName + " failed: value is " + valueNode->getAsString() +
                            ", requested is " + sValue, lsDebug);
                    return false;
                  }
                  break;
                }
              }
              // state was requested as active - but not found in /usr/states

              if (fFound == false) {
                Logger::getInstance()->log("checkSystemCondition: " +
                        sName + " requested but not found in system states!", lsError);
                return false;
              }
            }
          } else {
              // addon states for required addon generally not found
              Logger::getInstance()->log("checkSystemCondition: " +
                      sAddonID + " addon states for required addon generally not found!", lsError);
              return false;
          }
        } // oSystemAddonStates loop
      } // nodes NULL check

      // at least one of the zone states must match
      PropertyNodePtr oZoneNode =
          oBaseConditionNode->getPropertyByName("zone-states");
      if (oZoneNode != NULL) {
        bool fMatch = false;
        for (int i = 0; i < oZoneNode->getChildCount(); i++) {
          PropertyNodePtr testZoneIdNode =
              oZoneNode->getChild(i)->getPropertyByName("zone");
          PropertyNodePtr testGroupIdNode =
              oZoneNode->getChild(i)->getPropertyByName("group");
          PropertyNodePtr testSceneIdNode =
              oZoneNode->getChild(i)->getPropertyByName("scene");

          if ((testZoneIdNode == NULL) || (testGroupIdNode == NULL)) {
            Logger::getInstance()->log("checkSystemCondition: can not check "
                "condition, zone or group id node!", lsError);
            return false;
          }

          if (testSceneIdNode != NULL) {
            PropertyNodePtr groupNode =
                DSS::getInstance()->getPropertySystem().getProperty(
                      "/apartment/zones/zone" +
                      testZoneIdNode->getAsString() + "/groups/group" +
                      testGroupIdNode->getAsString());

            if (groupNode != NULL) {
              PropertyNodePtr lastCalledScene =
                groupNode->getPropertyByName("lastCalledScene");
              if (lastCalledScene != NULL) {
                if (lastCalledScene->getAsString() ==
                        testSceneIdNode->getAsString()) {
                    fMatch = true;
                    break;
                }
              }
            }
          } // testSceneIdNode != NULL
        }
        if (!fMatch) {
          return false;
        }
      } // oZoneNode != NULL

      PropertyNodePtr oWeekdayNode =
          oBaseConditionNode->getPropertyByName("weekdays");
      if (oWeekdayNode != NULL) {
        std::string sWeekdayString = oWeekdayNode->getAsString();
        if (!sWeekdayString.empty()) {
          DateTime oNow = DateTime();
          std::vector<std::string> oWeekDayArray =
              splitString(sWeekdayString, ',');
          bool fFound = false;
          for (size_t i = 0; i < oWeekDayArray.size(); i++) {
            if (strToInt(oWeekDayArray.at(i)) == oNow.getWeekday()) {
              fFound = true;
              break;
            }
          }
          if (fFound == false) {
            return false;
          }
        }
      } // oWeekdayNode != NULL

      PropertyNodePtr timeStartNode = oBaseConditionNode->getPropertyByName("time-start");
      PropertyNodePtr timeEndNode = oBaseConditionNode->getPropertyByName("time-end");
      if (timeStartNode != NULL || timeEndNode != NULL) {
        DateTime nTime = DateTime();
        int secNow = (nTime.getHour() * 60 + nTime.getMinute()) * 60 +
          nTime.getSecond();
        if (!checkTimeCondition(timeStartNode, timeEndNode, secNow)) {
          return false;
        }
      }

    } // (oBaseConditionNode != NULL)
    return true;
  } // checkSystemCondition

}; // namespace

