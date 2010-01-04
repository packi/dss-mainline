/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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

#include "set.h"

#include <stdexcept>
// hash_map,map used by SetSplitter
#include <map>
#ifndef WIN32 
#include <ext/hash_map>
#else
#include <hash_map>
#endif
#ifndef WIN32
using namespace __gnu_cxx;
#else
using namespace stdext;
#endif

#include "core/logger.h"
#include "core/foreach.h"
#include "core/ds485const.h"

#include "device.h"
#include "group.h"
#include "apartment.h"
#include "zone.h"
#include "modulator.h"

namespace dss {

    //================================================== Set

  Set::Set() {
  } // ctor

  Set::Set(Device& _device) {
    m_ContainedDevices.push_back(DeviceReference(_device, &_device.getApartment()));
  } // ctor(Device)

  Set::Set(DeviceVector _devices) {
    m_ContainedDevices = _devices;
  } // ctor(DeviceVector)

  Set::Set(const Set& _copy) {
    m_ContainedDevices = _copy.m_ContainedDevices;
  }

  void Set::nextScene() {
    throw std::runtime_error("Not yet implemented");
  } // nextScene

  void Set::previousScene() {
    throw std::runtime_error("Not yet implemented");
  } // previousScene

  void Set::perform(IDeviceAction& _deviceAction) {
    for(DeviceIterator iDevice = m_ContainedDevices.begin(); iDevice != m_ContainedDevices.end(); ++iDevice) {
      _deviceAction.perform(iDevice->getDevice());
    }
  } // perform

  Set Set::getSubset(const IDeviceSelector& _selector) const {
    Set result;
    foreach(DeviceReference iDevice, m_ContainedDevices) {
      if(_selector.selectDevice(iDevice.getDevice())) {
        result.addDevice(iDevice);
      }
    }
    return result;
  } // getSubset

  class ByGroupSelector : public IDeviceSelector {
  private:
    const int m_GroupNr;
  public:
    ByGroupSelector(const int _groupNr)
    : m_GroupNr(_groupNr)
    {}

    virtual ~ByGroupSelector() {}

    virtual bool selectDevice(const Device& _device) const {
      return _device.isInGroup(m_GroupNr);
    }
  };

  Set Set::getByGroup(int _groupNr) const {
    if(_groupNr != GroupIDBroadcast) {
      return getSubset(ByGroupSelector(_groupNr));
    } else {
      return *this;
    }
  } // getByGroup(id)

  Set Set::getByGroup(const Group& _group) const {
    return getByGroup(_group.getID());
  } // getByGroup(ref)

  Set Set::getByGroup(const std::string& _name) const {
    Set result;
    if(isEmpty()) {
      return result;
    } else {
      Group& g = get(0).getDevice().getApartment().getGroup(_name);
      return getByGroup(g.getID());
    }
  } // getByGroup(name)

  Set Set::getByZone(int _zoneID) const {
    if(_zoneID != 0) {
      Set result;
      foreach(const DeviceReference& dev, m_ContainedDevices) {
        if(dev.getDevice().getZoneID() == _zoneID) {
          result.addDevice(dev);
        }
      }
      return result;
    } else {
      // return a copy of this set if the broadcast zone was requested
      return *this;
    }
  } // getByZone(id)

  Set Set::getByZone(const std::string& _zoneName) const {
    Set result;
    if(isEmpty()) {
      return result;
    } else {
      Zone& zone = get(0).getDevice().getApartment().getZone(_zoneName);
      return getByZone(zone.getID());
    }
  } // getByZone(name)

  Set Set::getByModulator(const int _modulatorID) const {
    Set result;
    foreach(const DeviceReference& dev, m_ContainedDevices) {
      if(dev.getDevice().getModulatorID() == _modulatorID) {
        result.addDevice(dev);
      }
    }
    return result;
  } // getByModulator

  Set Set::getByModulator(const Modulator& _modulator) const {
    return getByModulator(_modulator.getBusID());
  } // getByModulator

  Set Set::getByFunctionID(const int _functionID) const {
    Set result;
    foreach(const DeviceReference& dev, m_ContainedDevices) {
      if(dev.getDevice().getFunctionID() == _functionID) {
        result.addDevice(dev);
      }
    }
    return result;
  } // getByFunctionID

  Set Set::getByPresence(const bool _presence) const {
    Set result;
    foreach(const DeviceReference& dev, m_ContainedDevices) {
      if(dev.getDevice().isPresent()== _presence) {
        result.addDevice(dev);
      }
    }
    return result;
  } // getByPresence

  class ByNameSelector : public IDeviceSelector {
  private:
    const std::string m_Name;
  public:
    ByNameSelector(const std::string& _name) : m_Name(_name) {};
    virtual ~ByNameSelector() {};

    virtual bool selectDevice(const Device& _device) const {
      return _device.getName() == m_Name;
    }
  };

  DeviceReference Set::getByName(const std::string& _name) const {
    Set resultSet = getSubset(ByNameSelector(_name));
    if(resultSet.length() == 0) {
      throw ItemNotFoundException(_name);
    }
    return resultSet.m_ContainedDevices.front();
  } // getByName


  class ByIDSelector : public IDeviceSelector {
  private:
    const devid_t m_ID;
  public:
    ByIDSelector(const devid_t _id) : m_ID(_id) {}
    virtual ~ByIDSelector() {};

    virtual bool selectDevice(const Device& _device) const {
      return _device.getShortAddress() == m_ID;
    }
  };

  DeviceReference Set::getByBusID(const devid_t _id) const {
    Set resultSet = getSubset(ByIDSelector(_id));
    if(resultSet.length() == 0) {
      throw ItemNotFoundException(std::string("with busid ") + intToString(_id));
    }
    return resultSet.m_ContainedDevices.front();
  } // getByBusID

  class ByDSIDSelector : public IDeviceSelector {
  private:
    const dsid_t m_ID;
  public:
    ByDSIDSelector(const dsid_t _id) : m_ID(_id) {}
    virtual ~ByDSIDSelector() {};

    virtual bool selectDevice(const Device& _device) const {
      return _device.getDSID() == m_ID;
    }
  };

  DeviceReference Set::getByDSID(const dsid_t _dsid) const {
    Set resultSet = getSubset(ByDSIDSelector(_dsid));
    if(resultSet.length() == 0) {
      throw ItemNotFoundException("with dsid " + _dsid.toString());
    }
    return resultSet.m_ContainedDevices.front();
  } // getByDSID

  int Set::length() const {
    return m_ContainedDevices.size();
  } // length

  bool Set::isEmpty() const {
    return length() == 0;
  } // isEmpty

  Set Set::combine(Set& _other) const {
    Set resultSet(_other);
    foreach(const DeviceReference& iDevice, m_ContainedDevices) {
      if(!resultSet.contains(iDevice)) {
        resultSet.addDevice(iDevice);
      }
    }
    return resultSet;
  } // combine

  Set Set::remove(const Set& _other) const {
    Set resultSet(*this);
    foreach(const DeviceReference& iDevice, _other.m_ContainedDevices) {
      resultSet.removeDevice(iDevice);
    }
    return resultSet;
  } // remove

  bool Set::contains(const DeviceReference& _device) const {
    DeviceVector::const_iterator pos = find(m_ContainedDevices.begin(), m_ContainedDevices.end(), _device);
    return pos != m_ContainedDevices.end();
  } // contains

  bool Set::contains(const Device& _device) const {
    return contains(DeviceReference(_device, &_device.getApartment()));
  } // contains

  void Set::addDevice(const DeviceReference& _device) {
    if(!contains(_device)) {
      m_ContainedDevices.push_back(_device);
    }
  } // addDevice

  void Set::addDevice(const Device& _device) {
    addDevice(DeviceReference(_device, &_device.getApartment()));
  } // addDevice

  void Set::removeDevice(const DeviceReference& _device) {
    DeviceVector::iterator pos = find(m_ContainedDevices.begin(), m_ContainedDevices.end(), _device);
    if(pos != m_ContainedDevices.end()) {
      m_ContainedDevices.erase(pos);
    }
  } // removeDevice

  void Set::removeDevice(const Device& _device) {
    removeDevice(DeviceReference(_device, &_device.getApartment()));
  } // removeDevice

  const DeviceReference& Set::get(int _index) const {
    return m_ContainedDevices.at(_index);
  } // get

  const DeviceReference& Set::operator[](const int _index) const {
    return get(_index);
  } // operator[]

  DeviceReference& Set::get(int _index) {
    return m_ContainedDevices.at(_index);
  } // get

  DeviceReference& Set::operator[](const int _index) {
    return get(_index);
  } // operator[]

  unsigned long Set::getPowerConsumption() {
    unsigned long result = 0;
    foreach(DeviceReference& iDevice, m_ContainedDevices) {
      result += iDevice.getPowerConsumption();
    }
    return result;
  } // getPowerConsumption

  std::ostream& operator<<(std::ostream& out, const Device& _dt) {
    out << "Device ID " << _dt.getShortAddress();
    if(!_dt.getName().empty()) {
      out << " name: " << _dt.getName();
    }
    return out;
  } // operator<<

  class SetSplitter {
  public:
    typedef hash_map<const Zone*, std::pair< std::vector<Group*>, Set> > FittingResult;
    typedef std::vector<AddressableModelItem*> ModelItemVector;

    static std::vector<AddressableModelItem*> splitUp(Set& _set) {
      std::vector<AddressableModelItem*> result;
      if(_set.length() == 1) {
        log("Optimization: Set contains only one device");
        result.push_back(&_set.get(0).getDevice());
      } else if(_set.length() > 0) {
        Apartment& apt = _set.get(0).getDevice().getApartment();
        if(_set.length() == apt.getDevices().length()) {
          log("Optimization: Set contains all devices of apartment");
          result.push_back(apt.getZone(0).getGroup(0));
        } else {
          result = bestFit(_set);
        }

      }
      return result;
    }
  private:
    typedef std::pair<std::vector<Group*>, Set> FittingResultPerModulator;
    typedef std::map<const Zone*, Set> HashMapZoneSet;
    static const bool OptimizerDebug = false;

    static ModelItemVector bestFit(const Set& _set) {
      ModelItemVector result;
      HashMapZoneSet zoneToSet = splitByZone(_set);

      std::back_insert_iterator<ModelItemVector> insertionIterator(result);
      for(HashMapZoneSet::iterator it = zoneToSet.begin(); it != zoneToSet.end(); ++it) {
        ModelItemVector resultPart = bestFit(*(it->first), it->second);
        std::copy(resultPart.begin(), resultPart.end(), insertionIterator);
      }

      return result;
    } // bestFit(const Set&)

    /** Returns a hash map that contains all zones and the devices in that zone in a set */
    static HashMapZoneSet splitByZone(const Set& _set) {
      HashMapZoneSet result;
      for(int iDevice = 0; iDevice < _set.length(); iDevice++) {
        const Device& dev = _set.get(iDevice).getDevice();
        Zone& zone = dev.getApartment().getZone(dev.getZoneID());
        result[&zone].addDevice(dev);
      }
      return result;
    } // splitByZone

    /** Tries to find a large group containing all devices and a list of devices to
        address all items in the set*/
    static ModelItemVector bestFit(const Zone& _zone, const Set& _set) {
      Set workingCopy = _set;

      ModelItemVector result;

      if(OptimizerDebug) {
        Logger::getInstance()->log("Finding fit for zone " + intToString(_zone.getID()));
      }

      if(_zone.getDevices().length() == _set.length()) {
        Logger::getInstance()->log(std::string("Optimization: Set contains all devices of zone ") + intToString(_zone.getID()));
        result.push_back(findGroupContainingAllDevices(_set, _zone));
      } else {
        std::vector<Group*> unsuitableGroups;
        Set workingCopy = _set;

        while(!workingCopy.isEmpty()) {
          DeviceReference& ref = workingCopy.get(0);
          workingCopy.removeDevice(ref);

          if(OptimizerDebug) {
            Logger::getInstance()->log("Working with device " + ref.getDSID().toString());
          }

          bool foundGroup = false;
          for(int iGroup = 0; iGroup < ref.getDevice().getGroupsCount(); iGroup++) {
            Group& g = ref.getDevice().getGroupByIndex(iGroup);

            if(OptimizerDebug) {
              Logger::getInstance()->log("  Checking Group " + intToString(g.getID()));
            }
            // continue if already found unsuitable
            if(find(unsuitableGroups.begin(), unsuitableGroups.end(), &g) != unsuitableGroups.end()) {
              if(OptimizerDebug) {
                Logger::getInstance()->log("  Group discarded before, continuing search");
              }
              continue;
            }

            // see if we've got a fit
            bool groupFits = true;
            Set devicesInGroup = _zone.getDevices().getByGroup(g);
            if(OptimizerDebug) {
              Logger::getInstance()->log("    Group has " + intToString(devicesInGroup.length()) + " devices");
            }
            for(int iDevice = 0; iDevice < devicesInGroup.length(); iDevice++) {
              if(!_set.contains(devicesInGroup.get(iDevice))) {
                unsuitableGroups.push_back(&g);
                groupFits = false;
                if(OptimizerDebug) {
                  Logger::getInstance()->log("    Original set does _not_ contain device " + devicesInGroup.get(iDevice).getDevice().getDSID().toString());
                }
                break;
              }
              if(OptimizerDebug) {
                Logger::getInstance()->log("    Original set contains device " + devicesInGroup.get(iDevice).getDevice().getDSID().toString());
              }
            }
            if(groupFits) {
              if(OptimizerDebug) {
                Logger::getInstance()->log("  Found a fit " + intToString(g.getID()));
              }
              foundGroup = true;
              result.push_back(&g);
              if(OptimizerDebug) {
                Logger::getInstance()->log("  Removing devices from working copy");
              }
              while(!devicesInGroup.isEmpty()) {
                workingCopy.removeDevice(devicesInGroup.get(0));
                devicesInGroup.removeDevice(devicesInGroup.get(0));
              }
              if(OptimizerDebug) {
                Logger::getInstance()->log("  Done. (Removing devices from working copy)");
              }
              break;
            }
          }

          // if no fitting group found
          if(!foundGroup) {
            result.push_back(&ref.getDevice());
          }
        }
      }
      return result;
    }

    static Group* findGroupContainingAllDevices(const Set& _set, const Zone& _zone) {
      std::bitset<63> possibleGroups;
      possibleGroups.set();
      for(int iDevice = 0; iDevice < _set.length(); iDevice++) {
        possibleGroups &= _set[iDevice].getDevice().getGroupBitmask();
      }
      if(possibleGroups.any()) {
        for(unsigned int iGroup = 0; iGroup < possibleGroups.size(); iGroup++) {
          if(possibleGroups.test(iGroup)) {
            Logger::getInstance()->log("Sending the command to group " + intToString(iGroup + 1));
            return _zone.getGroup(iGroup + 1);
          }
        }
        assert(false); // can't come here or we've detected a bug in std::bitvector.any()
      } else {
        Logger::getInstance()->log("Sending the command to broadcast group");
        return _zone.getGroup(GroupIDBroadcast);
      }
    }
  private:
    static void log(const std::string& _line) {
      Logger::getInstance()->log(_line);
    }
  };

  std::vector<AddressableModelItem*> Set::splitIntoAddressableItems() {
    return SetSplitter::splitUp(*this);
  } // splitIntoAddressableItems

} // namespace dss
