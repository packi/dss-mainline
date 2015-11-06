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
#include "model/device.h"
#include "model/modelconst.h"
#include "model/devicereference.h"

namespace dss {

  static bool isWindowHandle(PropertyNodePtr deviceNode)
  {
    if (!deviceNode) {
      return false;
    }

    PropertyNodePtr dsuidNode = deviceNode->getPropertyByName("dSUID");
    if (!dsuidNode) {
      return false;
    }

    const std::string dsuid_str =dsuidNode->getAsString();
    dsuid_t deviceDsuid  = str2dsuid(dsuid_str);
    Apartment& apartment = DSS::getInstance()->getApartment();
    const DeviceReference devRef(deviceDsuid, &apartment);

    boost::shared_ptr<const Device> device = devRef.getDevice();
    if (!device) {
      return false;
    }

    return device->hasBinaryInputType(BinaryInputIDWindowTilt);
  }

  static bool checkWindowHandleCondition (PropertyNodePtr base, PropertyNodePtr oSystemStates, int targetValue) {
    if (!base) {
      Logger::getInstance()->log("deviceNode not valid:", lsError);
      return false;
    }

    PropertyNodePtr deviceNode = base->getChild(0);
    std::string baseName = deviceNode->getPropertyByName("dSUID")->getAsString();
    std::string device1 = "dev." + baseName + ".0";
    std::string device2 = "dev." + baseName + ".1";

    // get value 1 and value 2 for window handle
    int actualValue1 = 0;
    int actualValue2 = 0;
    for (int i = 0; i < oSystemStates->getChildCount(); ++i) {
      PropertyNodePtr nameNode =
          oSystemStates->getChild(i)->getPropertyByName("name");

      PropertyNodePtr valueNode =
          oSystemStates->getChild(i)->getPropertyByName("value");

      if ((nameNode == NULL) || (valueNode == NULL)) {
        Logger::getInstance()->log("checkSystemCondition: can not check"
              " condition, missing name or value node!", lsError);
        continue;
      }
      if (device1 == nameNode->getAsString()) {
        actualValue1 = valueNode->getIntegerValue();
      }
      if (device2 == nameNode->getAsString()) {
        actualValue2 = valueNode->getIntegerValue();
      }
      if ((actualValue1 != 0) && (actualValue2 != 0)) {
        break;
      }
    }

    PropertyNodePtr indexNode  = base->getPropertyByName("inputIndex");
    int index = indexNode->getIntegerValue();

    if ((index == 0) && (targetValue == 2)) { // closed state
      return ((actualValue1 == 2) && (actualValue2 == 2));
    } else if ((index == 1) && (targetValue == 2)) { // open state
      return ((actualValue1 == 1) && (actualValue2 == 2));
    } else if ((index == 1) && (targetValue == 1)) { // tilt state
      return ((actualValue1 == 1) && (actualValue2 == 1));
    }
    Logger::getInstance()->log("checkWindowHandleCondition: can not check."
      " Pattern of index and value not supported!", lsWarning);
    return false;
  }

  static bool checkValueCondition(std::string& sValue, PropertyNodePtr valueNode, std::string& sName) {
    if (sValue != valueNode->getAsString()) {
      Logger::getInstance()->log("checkValueCondition: " +
        sName + " failed: value is " + valueNode->getAsString() +
        ", requested is " + sValue, lsDebug);
      return false;
    }
    return true;
  }

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
              PropertyNodePtr base = oSystemStates->getChild(i)->getPropertyByName("device");
              if (isWindowHandle(base->getChild(0)))
              {
                if (!checkWindowHandleCondition(base, oSystemStates, oStateNode->getChild(j)->getIntegerValue())) {
                  return false;
                }
              } else {
                if (!checkValueCondition(sValue, valueNode, sName)) {
                  return false;
                }
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

      PropertyNodePtr oDateNode = oBaseConditionNode->getPropertyByName("date");
      if (oDateNode != NULL) {
        bool fMatch = false;
        for (int i = 0; i < oDateNode->getChildCount(); i++) {
          PropertyNodePtr dateStartNode = oDateNode->getChild(i)->getPropertyByName("start");
          PropertyNodePtr dateEndNode = oDateNode->getChild(i)->getPropertyByName("end");
          PropertyNodePtr dateRruleNode = oDateNode->getChild(i)->getPropertyByName("rrule");
          if (dateStartNode != NULL && dateEndNode != NULL && dateEndNode != NULL) {
            DateTime nTime = DateTime();
            ICalEvent icalEvent(dateRruleNode->getAsString(), dateStartNode->getAsString(), dateEndNode->getAsString());
            if (icalEvent.isDateInside(nTime)) {
              fMatch = true;
              break;
            }
          }
        }
        if (!fMatch) {
          return false;
        }
      }

    } // (oBaseConditionNode != NULL)
    return true;
  } // checkSystemCondition

}; // namespace

