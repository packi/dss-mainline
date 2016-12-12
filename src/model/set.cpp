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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "set.h"

#include <stdexcept>

#include "src/util.h"
#include "src/logger.h"
#include "src/foreach.h"
#include "src/model/modelconst.h"

#include "device.h"
#include "group.h"
#include "apartment.h"
#include "zone.h"
#include "modulator.h"
#include "setsplitter.h"

namespace dss {

    //================================================== Set

  Set::Set() {
  } // ctor

  Set::Set(boost::shared_ptr<const Device> _device) {
    m_ContainedDevices.push_back(DeviceReference(_device, &_device->getApartment()));
  } // ctor(Device)

  Set::Set(std::vector<DeviceReference> _devices) {
    m_ContainedDevices = _devices;
  }

  Set::Set(const Set& _copy) {
    m_ContainedDevices = _copy.m_ContainedDevices;
  }

  void Set::nextScene(const callOrigin_t _origin, const SceneAccessCategory _category) {
    throw std::runtime_error("Not yet implemented");
  } // nextScene

  void Set::previousScene(const callOrigin_t _origin, const SceneAccessCategory _category) {
    throw std::runtime_error("Not yet implemented");
  } // previousScene

  void Set::perform(IDeviceAction& _deviceAction) {
    for(auto iDevice = m_ContainedDevices.begin(); iDevice != m_ContainedDevices.end(); ++iDevice) {
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

    virtual bool selectDevice(boost::shared_ptr<const Device> _device) const {
      return _device->isInGroup(m_GroupNr);
    }
  };

  Set Set::getByGroup(int _groupNr) const {
    if(_groupNr != GroupIDBroadcast) {
      return getSubset(ByGroupSelector(_groupNr));
    } else {
      return *this;
    }
  } // getByGroup(id)

  Set Set::getByGroup(boost::shared_ptr<const Group> _group) const {
    return getByGroup(_group->getID());
  } // getByGroup(ref)

  Set Set::getByGroup(const std::string& _name) const {
    Set result;
    if(isEmpty()) {
      return result;
    } else {
      boost::shared_ptr<Group> g = get(0).getDevice()->getApartment().getGroup(_name);
      return getByGroup(g->getID());
    }
  } // getByGroup(name)

  class ByColorSelector : public IDeviceSelector {
  private:
    DeviceClasses_t m_color;
  public:
    ByColorSelector(const DeviceClasses_t _color) : m_color(_color) {}
    virtual ~ByColorSelector() {};

    virtual bool selectDevice(boost::shared_ptr<const Device> _device) const {
      return (_device->getDeviceClass() == m_color);
    }
  };

  Set Set::getByColor(DeviceClasses_t _color) const {
    return getSubset(ByColorSelector(_color));
  }

  Set Set::getByZone(int _zoneID) const {
    if(_zoneID != 0) {
      Set result;
      foreach(const DeviceReference& dev, m_ContainedDevices) {
        if(dev.getDevice()->getZoneID() == _zoneID) {
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
      boost::shared_ptr<Zone> zone = get(0).getDevice()->getApartment().getZone(_zoneName);
      return getByZone(zone->getID());
    }
  } // getByZone(name)

  Set Set::getByDSMeter(boost::shared_ptr<const DSMeter> _dsMeter) const {
    return getByDSMeter(_dsMeter->getDSID());
  } // getByDSMeter

  Set Set::getByDSMeter(const dsuid_t& _dsMeterDSID) const {
    Set result;
    foreach(const DeviceReference& dev, m_ContainedDevices) {
      if (dev.getDevice()->getDSMeterDSID() == _dsMeterDSID) {
        result.addDevice(dev);
      }
    }
    return result;
  } // getByDSMeter

  Set Set::getByLastKnownDSMeter(const dsuid_t& _dsMeterDSID) const {
    Set result;
    foreach(const DeviceReference& dev, m_ContainedDevices) {
      if (dev.getDevice()->getLastKnownDSMeterDSID() == _dsMeterDSID) {
        result.addDevice(dev);
      }
    }
    return result;
  } // getByLastKnownDSMeter

  Set Set::getByFunctionID(const int _functionID) const {
    Set result;
    foreach(const DeviceReference& dev, m_ContainedDevices) {
      if(dev.getDevice()->getFunctionID() == _functionID) {
        result.addDevice(dev);
      }
    }
    return result;
  } // getByFunctionID

  Set Set::getByPresence(const bool _presence) const {
    Set result;
    foreach(const DeviceReference& dev, m_ContainedDevices) {
      if(dev.getDevice()->isPresent()== _presence) {
        result.addDevice(dev);
      }
    }
    return result;
  } // getByPresence

  Set Set::getByTag(const std::string& _tagName) const {
    Set result;
    foreach(const DeviceReference& dev, m_ContainedDevices) {
      if(dev.getDevice()->hasTag(_tagName)) {
        result.addDevice(dev);
      }
    }
    return result;
  } // getByTag

  class ByNameSelector : public IDeviceSelector {
  private:
    const std::string m_Name;
  public:
    ByNameSelector(const std::string& _name) : m_Name(_name) {};
    virtual ~ByNameSelector() {};

    virtual bool selectDevice(boost::shared_ptr<const Device> _device) const {
      return _device->getName() == m_Name;
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
    const dsuid_t& m_DSMeterID;
  public:
    ByIDSelector(const devid_t _id, const dsuid_t& _dsMeterID)
    : m_ID(_id), m_DSMeterID(_dsMeterID)
    {}
    virtual ~ByIDSelector() {};

    virtual bool selectDevice(boost::shared_ptr<const Device> _device) const {
      return ((_device->getShortAddress() == m_ID) &&
              (_device->getDSMeterDSID() == m_DSMeterID));
    }
  };

  DeviceReference Set::getByBusID(const devid_t _id, const dsuid_t& _dsid) const {
    Set resultSet = getSubset(ByIDSelector(_id, _dsid));
    if(resultSet.length() == 0) {
      throw ItemNotFoundException(std::string("with busid ") + intToString(_id));
    }
    return resultSet.m_ContainedDevices.front();
  } // getByBusID

  DeviceReference Set::getByBusID(const devid_t _busid, boost::shared_ptr<const DSMeter> _meter) const {
    return getByBusID(_busid, _meter->getDSID());
  } // getByBusID

  class ByDSIDSelector : public IDeviceSelector {
  private:
    const dsuid_t m_ID;
  public:
    ByDSIDSelector(const dsuid_t _id) : m_ID(_id) {}
    virtual ~ByDSIDSelector() {};

    virtual bool selectDevice(boost::shared_ptr<const Device> _device) const {
      return (_device->getDSID() == m_ID);
    }
  };

  DeviceReference Set::getByDSID(const dsuid_t _dsid) const {
    Set resultSet = getSubset(ByDSIDSelector(_dsid));
    if(resultSet.length() == 0) {
      throw ItemNotFoundException("with dsid " + dsuid2str(_dsid));
    }
    return resultSet.m_ContainedDevices.front();
  } // getByDSID

  class BySerialSelector : public IDeviceSelector {
  private:
    const uint32_t m_serial;
  public:
    BySerialSelector(const uint32_t _serial) : m_serial(_serial) {}
    virtual ~BySerialSelector() {};

    virtual bool selectDevice(boost::shared_ptr<const Device> _device) const {
      dsuid_t dsuid = _device->getDSID();
      if (!_device->isVdcDevice()) {
        uint32_t serialNumber;
        if (dsuid_get_serial_number(&dsuid, &serialNumber) == 0) {
          return (serialNumber == m_serial);
        }
      }
      return false;
    }
  };

  DeviceReference Set::getBySerial(const uint32_t _serial) const {
    Set resultSet = getSubset(BySerialSelector(_serial));
    if(resultSet.length() == 0) {
      throw ItemNotFoundException("with serial " + uintToString(_serial, true));
    }
    return resultSet.m_ContainedDevices.front();
  } // getBySerial

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
    auto pos = find(m_ContainedDevices.begin(), m_ContainedDevices.end(), _device);
    return pos != m_ContainedDevices.end();
  } // contains

  bool Set::contains(boost::shared_ptr<const Device> _device) const {
    return contains(DeviceReference(_device, &_device->getApartment()));
  } // contains

  void Set::addDevice(const DeviceReference& _device) {
    if(!contains(_device)) {
      m_ContainedDevices.push_back(_device);
    }
  } // addDevice

  void Set::addDevice(boost::shared_ptr<const Device> _device) {
    addDevice(DeviceReference(_device, &_device->getApartment()));
  } // addDevice

  void Set::removeDevice(const DeviceReference& _device) {
    auto pos = find(m_ContainedDevices.begin(), m_ContainedDevices.end(), _device);
    if(pos != m_ContainedDevices.end()) {
      m_ContainedDevices.erase(pos);
    }
  } // removeDevice

  void Set::removeDevice(boost::shared_ptr<const Device> _device) {
    removeDevice(DeviceReference(_device, &_device->getApartment()));
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

  class BySensorSelector : public IDeviceSelector {
  private:
    SensorType m_sensorType;
  public:
    BySensorSelector(SensorType _type) : m_sensorType(_type) {}
    virtual ~BySensorSelector() {}

    virtual bool selectDevice(boost::shared_ptr<const Device> _device) const {
      try {
        _device->getSensorByType(m_sensorType);
        return true;
      } catch (ItemNotFoundException &ex) {}

      return false;
    }
  };

  Set Set::getBySensorType(SensorType _type) const {
    return getSubset(BySensorSelector(_type));
  }

  std::vector<boost::shared_ptr<AddressableModelItem> > Set::splitIntoAddressableItems() {
    return SetSplitter::splitUp(*this);
  } // splitIntoAddressableItems

} // namespace dss
