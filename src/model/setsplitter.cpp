/*
 * setsplitter.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: pkochanowski
 */

#include "setsplitter.h"

namespace dss {

  const bool SetSplitter::OptimizerDebug = false;

  SetSplitter::ModelItemVector SetSplitter::splitUp(Set& _set) {
    ModelItemVector result;
    if (_set.length() > 0) {
      Apartment& apt = _set.get(0).getDevice()->getApartment();
      if (_set.length() == apt.getDevices().length()) {
        log("Optimization: Set contains all devices of apartment");
        result.push_back(apt.getZone(0)->getGroup(0));
      } else {
        result = bestFit(_set);
      }

    }
    return result;
  }

  SetSplitter::ModelItemVector SetSplitter::bestFit(const Set& _set) {
    ModelItemVector result;
    HashMapZoneSet zoneToSet = splitByZone(_set);

    std::back_insert_iterator<ModelItemVector> insertionIterator(result);
    for (HashMapZoneSet::iterator it = zoneToSet.begin(); it != zoneToSet.end(); ++it) {
      ModelItemVector resultPart = bestFit(*(it->first), it->second);
      std::copy(resultPart.begin(), resultPart.end(), insertionIterator);
    }

    if (result.size() > 1) {
      ModelItemVector partZoneZero = bestFit(*_set.get(0).getDevice()->getApartment().getZone(0), _set);
      if (partZoneZero.size() < result.size()) {
        result = partZoneZero;
      }
    }

    return result;
  } // bestFit(const Set&)

  /** Returns a hash map that contains all zones and the devices in that zone in a set */
  SetSplitter::HashMapZoneSet SetSplitter::splitByZone(const Set& _set) {
    HashMapZoneSet result;
    for (int iDevice = 0; iDevice < _set.length(); iDevice++) {
      boost::shared_ptr<const Device> dev = _set.get(iDevice).getDevice();
      boost::shared_ptr<Zone> zone = dev->getApartment().getZone(dev->getZoneID());
      result[zone].addDevice(dev);
    }
    return result;
  } // splitByZone

  /** Tries to find a large group containing all devices and a list of devices to
   address all items in the set*/
  SetSplitter::ModelItemVector SetSplitter::bestFit(const Zone& _zone, const Set& _set) {
    Set workingCopy = _set;

    ModelItemVector result;

    if (OptimizerDebug) {
      Logger::getInstance()->log("Finding fit for zone " + intToString(_zone.getID()));
    }

    if (_zone.getDevices().length() == _set.length()) {
      Logger::getInstance()->log(
          std::string("Optimization: Set contains all devices of zone ") + intToString(_zone.getID()));
      result.push_back(findGroupContainingAllDevices(_set, _zone));
    } else {
      std::vector<boost::shared_ptr<Group> > unsuitableGroups;
      Set workingCopy = _set;

      while (!workingCopy.isEmpty()) {
        DeviceReference ref = workingCopy.get(0);
        workingCopy.removeDevice(ref);

        if (OptimizerDebug) {
          Logger::getInstance()->log(
              "Working with device " + dsuid2str(ref.getDSID()) + " groups: "
                  + intToString(ref.getDevice()->getGroupsCount()));
        }

        bool foundGroup = false;
        for (int iGroup = 0; iGroup < ref.getDevice()->getGroupsCount(); iGroup++) {
          boost::shared_ptr<Group> g = ref.getDevice()->getGroupByIndex(iGroup);

          boost::shared_ptr<Group> groupOfZone = _zone.getGroup(g->getID());
          if (groupOfZone != NULL) {
            g = groupOfZone;
          }

          if (OptimizerDebug) {
            Logger::getInstance()->log("  Checking Group " + intToString(g->getID()));
          }
          // continue if already found unsuitable
          if (find(unsuitableGroups.begin(), unsuitableGroups.end(), g) != unsuitableGroups.end()) {
            if (OptimizerDebug) {
              Logger::getInstance()->log("  Group discarded before, continuing search");
            }
            continue;
          }

          // see if we've got a fit
          bool groupFits = true;
          Set devicesInGroup = _zone.getDevices().getByGroup(g);
          if (OptimizerDebug) {
            Logger::getInstance()->log("    Group has " + intToString(devicesInGroup.length()) + " devices");
          }
          for (int iDevice = 0; iDevice < devicesInGroup.length(); iDevice++) {
            if (!_set.contains(devicesInGroup.get(iDevice))) {
              unsuitableGroups.push_back(g);
              groupFits = false;
              if (OptimizerDebug) {
                Logger::getInstance()->log(
                    "    Original set does _not_ contain device "
                        + dsuid2str(devicesInGroup.get(iDevice).getDevice()->getDSID()));
              }
              break;
            }
            if (OptimizerDebug) {
              Logger::getInstance()->log(
                  "    Original set contains device " + dsuid2str(devicesInGroup.get(iDevice).getDevice()->getDSID()));
            }
          }
          if (groupFits) {
            if (OptimizerDebug) {
              Logger::getInstance()->log("  Found a fit " + intToString(g->getID()));
            }
            foundGroup = true;
            result.push_back(g);
            if (OptimizerDebug) {
              Logger::getInstance()->log("  Removing devices from working copy");
            }
            while (!devicesInGroup.isEmpty()) {
              workingCopy.removeDevice(devicesInGroup.get(0));
              devicesInGroup.removeDevice(devicesInGroup.get(0));
            }
            if (OptimizerDebug) {
              Logger::getInstance()->log("  Done. (Removing devices from working copy)");
            }
            break;
          }
        }

        // if no fitting group found
        if (!foundGroup) {
          result.push_back(ref.getDevice());
        }
      }
    }
    return result;
  }

  boost::shared_ptr<Group> SetSplitter::findGroupContainingAllDevices(const Set& _set, const Zone& _zone) {
    std::vector<int> intersection;

    // if there are any devices try to find common group
    if (_set.length() > 0) {
      // start with group list from first device
      intersection = _set[0].getDevice()->getGroupIds();
      std::sort(intersection.begin(), intersection.end());

      // search all other devices
      for (int iDevice = 1; iDevice < _set.length(); iDevice++) {

        std::vector<int> deviceGroups = _set[iDevice].getDevice()->getGroupIds();
        std::sort(deviceGroups.begin(), deviceGroups.end());

        // find all groups that are in common with current intersection
        std::vector<int> tmpVector;
        std::set_intersection(intersection.begin(), intersection.end(), deviceGroups.begin(), deviceGroups.end(),
            std::back_inserter(tmpVector));

        intersection = std::move(tmpVector);

        // if we do not have common groups we will not find any
        if (intersection.size() == 0) {
          break;
        }
      }
    }

    // if we found at least one group that is common to all devices return it, otherwise just use broadcast group
    if (intersection.size() > 0) {
      Logger::getInstance()->log("Sending the command to group " + intToString(intersection.front()));
      return _zone.getGroup(intersection.front());
    } else {
      Logger::getInstance()->log("Sending the command to broadcast group");
      return _zone.getGroup(GroupIDBroadcast);
    }
  }
}
