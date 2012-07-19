/*
    Copyright (c) 2009,2011 digitalSTROM.org, Zurich, Switzerland

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
#include <boost/shared_ptr.hpp>

namespace dss {

  class Device;
  class AddressableModelItem;
  class BusInterface;
  class Group;

  typedef struct {
    dss_dsid_t DSID;
    uint32_t SoftwareRevisionARM;
    uint32_t SoftwareRevisionDSP;
    uint32_t HardwareVersion;
    uint16_t APIVersion;
    std::string Name;
  } DSMeterSpec_t;

  typedef struct {
    devid_t ShortAddress;
    uint16_t FunctionID;
    uint16_t ProductID;
    uint16_t VendorID;
    uint16_t Version;
    uint32_t SerialNumber;
    bool Locked;
    uint8_t ActiveState;
    uint8_t OutputMode;
    uint8_t LTMode;
    std::vector<int> Groups;
    dss_dsid_t DSID;
    uint8_t ButtonID;
    uint8_t GroupMembership;
    uint8_t ActiveGroup;
    bool SetsLocalPriority;
    std::string Name;
    uint16_t ZoneID;
  } DeviceSpec_t;

  typedef struct {
    uint32_t Hash;
    uint32_t ModificationCount;
  } DSMeterHash_t;

  class DeviceBusInterface {
  public:
    //------------------------------------------------ Device manipulation
    virtual uint8_t getDeviceConfig(const Device& _device,
                                    uint8_t _configClass,
                                    uint8_t _configIndex) = 0;

    virtual uint16_t getDeviceConfigWord(const Device& _device,
                                       uint8_t _configClass,
                                       uint8_t _configIndex) = 0;

    virtual void setDeviceConfig(const Device& _device, uint8_t _configClass,
                                 uint8_t _configIndex, uint8_t _value) = 0;

    /** Enable or disable programming mode */
    virtual void setDeviceProgMode(const Device& _device, uint8_t modeId) = 0;

    /** Tests transmission quality to a device, where the first returned
      value is the DownstreamQuality and the second value the UpstreamQuality */
    virtual std::pair<uint8_t, uint16_t> getTransmissionQuality(const Device& _device) = 0;

    /** Set generic device output value */
    virtual void setValue(const Device& _device, uint8_t _value) = 0;

    /** Queries sensor value & type from a device */
    virtual uint32_t getSensorValue(const Device& _device, const int _sensorIndex) = 0;
    virtual uint8_t getSensorType(const Device& _device, const int _sensorIndex) = 0;

    /** add device to a user group */
    virtual void addGroup(const Device& _device, const int _groupId) = 0;

    /** remove device from a user group */
    virtual void removeGroup(const Device& _device, const int _groupId) = 0;

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
    virtual std::vector<DeviceSpec_t> getDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) = 0;
    virtual std::vector<DeviceSpec_t> getInactiveDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) = 0;

    /** Returns the a std::vector containing the group-ids of the given zone on the specified dsMeter */
    virtual std::vector<int> getGroups(const dss_dsid_t& _dsMeterID, const int _zoneID) = 0;

    virtual std::vector<std::pair<int, int> > getLastCalledScenes(const dss_dsid_t& _dsMeterID, const int _zoneID) = 0;
    virtual bool getEnergyBorder(const dss_dsid_t& _dsMeterID, int& _lower, int& _upper) = 0;

    /** Returns the function, product and revision id of the device. */
    virtual DeviceSpec_t deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID) = 0;

    virtual ~StructureQueryBusInterface() {}; // please the compiler (virtual dtor)
    virtual std::string getSceneName(dss_dsid_t _dsMeterID, boost::shared_ptr<Group> _group, const uint8_t _sceneNumber) = 0;

    /** returns the hash over the dSMeter's datamodel */
    virtual DSMeterHash_t getDSMeterHash(const dss_dsid_t& _dsMeterID) = 0;
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

    /** Removes the device from the dSM data model **/
    virtual void removeDeviceFromDSMeter(const dss_dsid_t& _dsMeterID, const int _deviceID) = 0;
    /** Sets the name of a scene */
    virtual void sceneSetName(uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneNumber, const std::string& _name) = 0;
    virtual void deviceSetName(dss_dsid_t _meterDSID, devid_t _deviceID, const std::string& _name) = 0;
    virtual void meterSetName(dss_dsid_t _meterDSID, const std::string& _name) = 0;

    virtual void createGroup(uint16_t _zoneID, uint8_t _groupID) = 0;
    virtual void removeGroup(uint16_t _zoneID, uint8_t _groupID) = 0;

    /** Send sensoric data downstream to devices */
    virtual void sensorPush(uint16_t _zoneID, dss_dsid_t _sourceID, uint8_t _sensorType, uint16_t _sensorValue) = 0;

    virtual void setButtonSetsLocalPriority(const dss_dsid_t& _dsMeterID, const devid_t _deviceID, bool _setsPriority) = 0;

    virtual ~StructureModifyingBusInterface() {}; // please the compiler (virtual dtor)
  }; // StructureModifyingBusInterface

  class ActionRequestInterface {
  public:
    virtual void callScene(AddressableModelItem *pTarget, const uint16_t _origin, const uint16_t scene, const bool _force) = 0;
    virtual void saveScene(AddressableModelItem *pTarget, const uint16_t _origin, const uint16_t scene) = 0;
    virtual void undoScene(AddressableModelItem *pTarget, const uint16_t _origin, const uint16_t scene) = 0;
    virtual void undoSceneLast(AddressableModelItem *pTarget, const uint16_t _origin) = 0;
    virtual void blink(AddressableModelItem *pTarget, const uint16_t _origin) = 0;
    virtual void setValue(AddressableModelItem *pTarget, const uint16_t _origin, const uint8_t _value) = 0;
  }; // ActionRequestInterface


  class MeteringBusInterface {
  public:
    /** Sends a message to all devices to report their meter-data */
    virtual void requestMeterData() = 0;

    /** Returns the current power-consumption in mW */
    virtual unsigned long getPowerConsumption(const dss_dsid_t& _dsMeterID) = 0;

    /** Returns the meter value in Wh */
    virtual unsigned long getEnergyMeterValue(const dss_dsid_t& _dsMeterID) = 0;

    virtual ~MeteringBusInterface() {}; // please the compiler (virtual dtor)
  }; // MeteringBusInterface

  class BusEventSink {
  public:
    virtual void onGroupCallScene(BusInterface* _source,
                                  const dss_dsid_t& _dsMeterID,
                                  const int _zoneID,
                                  const int _groupID,
                                  const int _originDeviceId,
                                  const int _sceneID,
                                  const bool _force) = 0;
    virtual void onGroupUndoScene(BusInterface* _source,
                                  const dss_dsid_t& _dsMeterID,
                                  const int _zoneID,
                                  const int _groupID,
                                  const int _originDeviceId,
                                  const int _sceneID,
                                  const bool _explicit) = 0;
    virtual void onDeviceCallScene(BusInterface* _source,
                                  const dss_dsid_t& _dsMeterID,
                                  const int _deviceID,
                                  const int _originDeviceId,
                                  const int _sceneID,
                                  const bool _force) = 0;
    virtual void onDeviceUndoScene(BusInterface* _source,
                                  const dss_dsid_t& _dsMeterID,
                                  const int _deviceID,
                                  const int _originDeviceId,
                                  const int _sceneID,
                                  const bool _explicit) = 0;
    virtual void onMeteringEvent(BusInterface* _source,
                                 const dss_dsid_t& _dsMeterID,
                                 const int _powerW,
                                 const int _energyWs) = 0;
    virtual ~BusEventSink() {};
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
