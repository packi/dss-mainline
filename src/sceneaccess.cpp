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
#include "model/group.h"
#include "model/state.h"

#include <boost/shared_ptr.hpp>

namespace dss {

bool SceneAccess::checkAccess(const AddressableModelItem *_pTarget, const SceneAccessCategory _category)
{
  Apartment& apartment = DSS::getInstance()->getApartment();
  PropertyNodePtr property = apartment.getPropertySystem()->getRootNode();
  {
    /*
     * Fire: Prohibit all automatic actions.
     */
    boost::shared_ptr<State> fire = apartment.getState("fire");
    if (fire && (fire->getState() == State_Active)) {
      switch (_category) {
      case SAC_MANUAL:
        break;
      default:
        throw SceneAccessException("Execution blocked: fire is active");
      }
    }
  }

  {
    /*
     * Wind: Prohibit automatic actions in group shade
     */
    boost::shared_ptr<State> wind = apartment.getState("wind");
    if (wind && (wind->getState() == State_Active)) {
      const Group* pGroup = dynamic_cast<const Group*>(_pTarget);
      if (pGroup != NULL) {
        if (DEVICE_CLASS_GR == pGroup->getID()) {
          switch (_category) {
          case SAC_MANUAL:
            break;
          default:
            throw SceneAccessException("Execution blocked: wind is active");
          }
        }
      }
      const Device* pDevice = dynamic_cast<const Device*>(_pTarget);
      if (pDevice != NULL) {
        if (pDevice->getGroupBitmask().test(DEVICE_CLASS_GR)) {
          switch (_category) {
          case SAC_MANUAL:
            break;
          default:
            throw SceneAccessException("Execution blocked: wind is active");
          }
        }
      }
    }
  }

  {
    /*
     * Partial Wind (in user groups): Prevent automatic actions in relevant user group.
     */
    for (int i = 16; i < 24; ++i) {
      boost::shared_ptr<Group> gr = apartment.getGroup(i);
      if (gr && gr->isValid() && gr->getStandardGroupID() == DEVICE_CLASS_GR) {
        boost::shared_ptr<State> wind = apartment.getState("wind.group" + intToString(i));
        if (wind != NULL && (wind->getState() == State_Active)) {
          const Group* pGroup = dynamic_cast<const Group*>(_pTarget);
          if (pGroup != NULL) {
            if (pGroup->getID() == i) {
              switch (_category) {
              case SAC_MANUAL:
                break;
              default:
                throw SceneAccessException("Execution blocked: wind is active");
              }
            }
          }
          const Device* pDevice = dynamic_cast<const Device*>(_pTarget);
          if (pDevice != NULL) {
            if (pDevice->getGroupBitmask().test(i)) {
              switch (_category) {
              case SAC_MANUAL:
                break;
              default:
                throw SceneAccessException("Execution blocked: wind is active");
              }
            }
          }
        }
      }
    }
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
}

}
