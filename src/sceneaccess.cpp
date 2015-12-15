/*
    Copyright (c) 2013 digitalSTROM.org, Zurich, Switzerland

    Author: Christian Hitz, aizo AG <christian.hitz@aizo.com>

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

#include "sceneaccess.h"
#include "stdio.h"
#include "string.h"

#include "src/base.h"
#include "src/logger.h"
#include "propertysystem.h"
#include "dss.h"
#include "model/device.h"
#include "model/apartment.h"
#include "model/addressablemodelitem.h"
#include "model/cluster.h"
#include "model/group.h"
#include "model/set.h"
#include "model/state.h"
#include "model/modelconst.h"

#include <boost/shared_ptr.hpp>

namespace dss {

bool SceneAccess::checkAccess(const AddressableModelItem *_pTarget, const SceneAccessCategory _category)
{
  if (!DSS::hasInstance()) {
    return true;
  }

  // "manual" scene calls are never blocked
  if (_category == SAC_MANUAL) {
    return true;
  }

  Apartment& apartment = DSS::getInstance()->getApartment();
  PropertyNodePtr property = apartment.getPropertySystem()->getRootNode();

  {
    /*
     * Fire & Wind: Prohibit automatic actions in group shade
     *
     * Behavior changed 2015/12/10:
     * Fire should not block all groups, but only shades, see #11755
     */
    boost::shared_ptr<State> fire = apartment.getNonScriptState("fire");
    boost::shared_ptr<State> wind = apartment.getNonScriptState("wind");
    bool protectionActive =
        (wind && (wind->getState() == State_Active)) ||
        (fire && (fire->getState() == State_Active));

    if (protectionActive) {
      const Group* pGroup = dynamic_cast<const Group*>(_pTarget);
      if (pGroup != NULL) {
        if (DEVICE_CLASS_GR == pGroup->getStandardGroupID()) {
          throw SceneAccessException("Execution blocked: wind is active");
        }
      }
      const Device* pDevice = dynamic_cast<const Device*>(_pTarget);
      if (pDevice != NULL) {
        if (pDevice->getGroupBitmask().test(DEVICE_CLASS_GR-1)) {
          throw SceneAccessException("Execution blocked: wind is active");
        }
      }
    }
  }

  {
    /*
     * Partial Wind (in user groups): Prevent automatic actions in relevant user group.
     */
    boost::shared_ptr<Cluster> gr;
    for (int i = GroupIDAppUserMin; i <= GroupIDAppUserMax; ++i) {
      try {
        gr = apartment.getCluster(i);
      } catch (ItemNotFoundException& e) {
        continue;
      }
      if (gr && gr->isValid() && gr->getStandardGroupID() == DEVICE_CLASS_GR) {
        boost::shared_ptr<State> wind;
        try {
          wind = apartment.getNonScriptState("wind.group" + intToString(i));
        } catch (ItemNotFoundException& e) {
          continue;
        }
        if (wind != NULL && (wind->getState() == State_Active)) {
          const Group* pGroup = dynamic_cast<const Group*>(_pTarget);
          if (pGroup != NULL) {
            // addressing the same cluster with an active wind alarm
            if (pGroup->getID() == i) {
              throw SceneAccessException("Execution blocked: wind is active");
            }
            // addressing group 2 in a single zone (or all zones) with devices
            // that are member of a cluster with an active wind alarm
            if (pGroup->getID() == DEVICE_CLASS_GR) {
              Set devsInZones = pGroup->getDevices().getByGroup(i);
              if (devsInZones.length() > 0) {
                throw SceneAccessException("Execution blocked: wind is active");
              }
            }
          }
          const Device* pDevice = dynamic_cast<const Device*>(_pTarget);
          if (pDevice != NULL) {
            if (pDevice->getGroupBitmask().test(i-1)) {
              throw SceneAccessException("Execution blocked: wind is active");
            }
          }
        }
      }
    }
  }

  return true;
}

bool SceneAccess::checkStates(const AddressableModelItem *_pTarget, int _sceneNr)
{
  if (!DSS::hasInstance()) {
    return true;
  }

  std::string stateName;
  switch (_sceneNr) {
    case SceneWindActive: stateName = "wind"; break;
    case SceneRainActive: stateName = "rain"; break;
    case SceneHailActive: stateName = "hail"; break;
    case SceneFire: stateName = "fire"; break;
    case SceneAlarm: stateName = "alarm"; break;
    case SceneAlarm2: stateName = "alarm2"; break;
    case SceneAlarm3: stateName = "alarm3"; break;
    case SceneAlarm4: stateName = "alarm4"; break;
    default: return true;
  }

  Apartment& apartment = DSS::getInstance()->getApartment();
  boost::shared_ptr<State> s = apartment.getNonScriptState(stateName);
  if (s && (s->getState() == State_Active)) {
    std::string errorMessage("Execution blocked: state " + stateName + " is already active");
    throw SceneAccessException(errorMessage);
  }

  return true;
}

SceneAccessCategory SceneAccess::stringToCategory(const std::string& _categoryString)
{
  if (_categoryString == "manual") {
    return SAC_MANUAL;
  } else if (_categoryString == "timer") {
    return SAC_TIMER;
  } else if (_categoryString == "algorithm") {
    return SAC_ALGORITHM;
  }
  return SAC_UNKNOWN;
}

std::string SceneAccess::categoryToString(const SceneAccessCategory _category)
{
  switch (_category) {
  case SAC_MANUAL:
    return "manual";
  case SAC_TIMER:
    return "timer";
  case SAC_ALGORITHM:
    return "algorithm";
  case SAC_UNKNOWN:
    return "unknown";
  }
  return "invalid";
}

}
