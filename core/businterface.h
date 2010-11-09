/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#ifndef BUSINTERFACE_H_
#define BUSINTERFACE_H_

#include "ds485types.h"

#include "base.h"

#include <string>
#include <vector>
#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>

namespace dss {

  class Device;
  class AddressableModelItem;
  class BusInterface;

  typedef boost::tuple<dss_dsid_t, int, int, int, int, std::string> DSMeterSpec_t; // bus-id, arm-sw-version, dsp-sw-version, hw-version, api version, name
  typedef boost::tuple<int, int, int, int> DeviceSpec_t; // function id, product id, revision, bus address

  class DeviceBusInterface {
  public:
    //------------------------------------------------ Device manipulation
    virtual uint16_t deviceGetParameterValue(devid_t _id, const dss_dsid_t& _dsMeterID, int _paramID) = 0;

    virtual void setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size) = 0;
    virtual int getSensorValue(const Device& _device, const int _sensorID) = 0;
    /** Tells the dSM to lock the device if \a _lock is true. */
    virtual void lockOrUnlockDevice(const Device& _device, const bool _lock) = 0;

    virtual ~DeviceBusInterface() {}; // please the compiler (virtual dtor)
  }; // DeviceBusInterface

  class StructureQueryBusInterface {
  public:
    /** Returns an std::vector containing the dsMeter-spec of all dsMeters present. */
    virtual std::vector<DSMeterSpec_t> getDSMeters() = 0;

    /** Returns the dsMeter-spec for a dsMeter */
    virtual DSMeterSpec_t getDSMeterSpec(const dss_dsid_t& _dsMeterID) = 0;

    /** Returns a std::vector conatining the zone-ids of the specified dsMeter */
    virtual std::vector<int> getZones(const dss_dsid_t& _dsMeterID) = 0;
    /** Returns the bus-ids of the devices present in the given zone of the specified dsMeter */
    virtual std::vector<int> getDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) = 0;

    /** Returns the a std::vector containing the group-ids of the given zone on the specified dsMeter */
    virtual std::vector<int> getGroups(const dss_dsid_t& _dsMeterID, const int _zoneID) = 0;

    virtual std::vector<int> getGroupsOfDevice(const dss_dsid_t& _dsMeterID, const int _deviceID) = 0;

    /** Returns the DSID of a given device */
    virtual dss_dsid_t getDSIDOfDevice(const dss_dsid_t& _dsMeterID, const int _deviceID) = 0;

    virtual int getLastCalledScene(const dss_dsid_t& _dsMeterID, const int _zoneID, const int _groupID) = 0;
    virtual bool getEnergyBorder(const dss_dsid_t& _dsMeterID, int& _lower, int& _upper) = 0;

    /** Returns the function, product and revision id of the device. */
    virtual DeviceSpec_t deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID) = 0;

    virtual ~StructureQueryBusInterface() {}; // please the compiler (virtual dtor)
    virtual bool isLocked(boost::shared_ptr<const Device> _device) = 0;
  }; // StructureQueryBusInterface

  class StructureModifyingBusInterface {
  public:
    /** Adds the given device to the specified zone. */
    virtual void setZoneID(const dss_dsid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID) = 0;

    /** Creates a new Zone on the given dsMeter */
    virtual void createZone(const dss_dsid_t& _dsMeterID, const int _zoneID) = 0;

    /** Removes the zone \a _zoneID on the dsMeter \a _dsMeterID */
    virtual void removeZone(const dss_dsid_t& _dsMeterID, const int _zoneID) = 0;

    /** Adds a device to a given group */
    virtual void addToGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID) = 0;
    /** Removes a device from a given group */
    virtual void removeFromGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID) = 0;

    /** Removes all inactive (!isPresent) devices from the dSM */
    virtual void removeInactiveDevices(const dss_dsid_t& _dsMeterID) = 0;

    virtual ~StructureModifyingBusInterface() {}; // please the compiler (virtual dtor)
  }; // StructureModifyingBusInterface

  class ActionRequestInterface {
  public:
    virtual void callScene(AddressableModelItem *pTarget, const uint16_t scene) = 0;
    virtual void saveScene(AddressableModelItem *pTarget, const uint16_t scene) = 0;
    virtual void undoScene(AddressableModelItem *pTarget) = 0;
    virtual void blink(AddressableModelItem *pTarget) = 0;
    virtual void setValue(AddressableModelItem *pTarget, const double _value) = 0;
  }; // ActionRequestInterface


  class MeteringBusInterface {
  public:
    /** Returns the current power-consumption in mW */
    virtual unsigned long getPowerConsumption(const dss_dsid_t& _dsMeterID) = 0;
    /** Sends a message to all devices to report their power consumption */
    virtual void requestPowerConsumption() = 0;

    /** Returns the meter value in Wh */
    virtual unsigned long getEnergyMeterValue(const dss_dsid_t& _dsMeterID) = 0;
    /** Sends a message to all devices to report their energy value */
    virtual void requestEnergyMeterValue() = 0;

    virtual ~MeteringBusInterface() {}; // please the compiler (virtual dtor)
  }; // MeteringBusInterface

  class BusEventSink {
  public:
    virtual void onGroupCallScene(BusInterface* _source,
                                  const dss_dsid_t& _dsMeterID,
                                  const int _zoneID,
                                  const int _groupID,
                                  const int _sceneID) = 0;
  };

  /** Interface to be implemented by any bus interface provider */
  class BusInterface {
  public:
    virtual ~BusInterface() {};

    virtual DeviceBusInterface* getDeviceBusInterface() = 0;
    virtual StructureQueryBusInterface* getStructureQueryBusInterface() = 0;
    virtual MeteringBusInterface* getMeteringBusInterface() = 0;
    virtual StructureModifyingBusInterface* getStructureModifyingBusInterface() = 0;
    virtual ActionRequestInterface* getActionRequestInterface() = 0;

    virtual void setBusEventSink(BusEventSink* _eventSink) = 0;
  };

  class BusApiError : public DSSException {
  public:
    BusApiError(const std::string& _what)
    : DSSException(_what) {}
  }; // BusApiError

}
#endif /* BUSINTERFACE_H_ */
