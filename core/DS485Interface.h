/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#ifndef DS485INTERFACE_H_
#define DS485INTERFACE_H_

#include "ds485types.h"
#include "unix/ds485.h"
#include "model.h"
#include "base.h"

#include <string>
#include <vector>
#include <boost/tuple/tuple.hpp>

namespace dss {

	typedef boost::tuple<int, int, int, std::string, int> ModulatorSpec_t; // bus-id, sw-version, hw-version, name, device-id

  /** Interface to be implemented by any implementation of the DS485 interface */
  class DS485Interface {
  public:
    virtual ~DS485Interface() {};

    /** Returns true when the interface is ready to transmit user generated DS485Packets */
    virtual bool isReady() = 0;

    virtual void sendFrame(DS485CommandFrame& _frame) = 0;

    //------------------------------------------------ Specialized Commands (system)
    /** Returns an std::vector containing the modulator-spec of all modulators present. */
    virtual std::vector<ModulatorSpec_t> getModulators() = 0;

    /** Returns the modulator-spec for a modulator */
    virtual ModulatorSpec_t getModulatorSpec(const int _modulatorID) = 0;

    /** Returns a std::vector conatining the zone-ids of the specified modulator */
    virtual std::vector<int> getZones(const int _modulatorID) = 0;
    /** Returns the count of the zones of the specified modulator */
    virtual int getZoneCount(const int _modulatorID) = 0;
    /** Returns the bus-ids of the devices present in the given zone of the specified modulator */
    virtual std::vector<int> getDevicesInZone(const int _modulatorID, const int _zoneID) = 0;
    /** Returns the count of devices present in the given zone of the specified modulator */
    virtual int getDevicesCountInZone(const int _modulatorID, const int _zoneID) = 0;

    /** Adds the given device to the specified zone. */
    virtual void setZoneID(const int _modulatorID, const devid_t _deviceID, const int _zoneID) = 0;

    /** Creates a new Zone on the given modulator */
    virtual void createZone(const int _modulatorID, const int _zoneID) = 0;

    /** Removes the zone \a _zoneID on the modulator \a _modulatorID */
    virtual void removeZone(const int _modulatorID, const int _zoneID) = 0;

    /** Returns the count of groups present in the given zone of the specifid modulator */
    virtual int getGroupCount(const int _modulatorID, const int _zoneID) = 0;
    /** Returns the a std::vector containing the group-ids of the given zone on the specified modulator */
    virtual std::vector<int> getGroups(const int _modulatorID, const int _zoneID) = 0;
    /** Returns the count of devices present in the given group */
    virtual int getDevicesInGroupCount(const int _modulatorID, const int _zoneID, const int _groupID) = 0;
    /** Returns a std::vector containing the bus-ids of the devices present in the given group */
    virtual std::vector<int> getDevicesInGroup(const int _modulatorID, const int _zoneID, const int _groupID) = 0;

    virtual std::vector<int> getGroupsOfDevice(const int _modulatorID, const int _deviceID) = 0;

    /** Adds a device to a given group */
    virtual void addToGroup(const int _modulatorID, const int _groupID, const int _deviceID) = 0;
    /** Removes a device from a given group */
    virtual void removeFromGroup(const int _modulatorID, const int _groupID, const int _deviceID) = 0;

    /** Adds a user group */
    virtual int addUserGroup(const int _modulatorID) = 0;
    /** Removes a user group */
    virtual void removeUserGroup(const int _modulatorID, const int _groupID) = 0;

    /** Returns the DSID of a given device */
    virtual dsid_t getDSIDOfDevice(const int _modulatorID, const int _deviceID) = 0;
    /** Returns the DSID of a given modulator */
    virtual dsid_t getDSIDOfModulator(const int _modulatorID) = 0;

    virtual int getLastCalledScene(const int _modulatorID, const int _zoneID, const int _groupID) = 0;

    //------------------------------------------------ Metering

    /** Returns the current power-consumption in mW */
    virtual unsigned long getPowerConsumption(const int _modulatorID) = 0;

    /** Returns the meter value in Wh */
    virtual unsigned long getEnergyMeterValue(const int _modulatorID) = 0;

    virtual bool getEnergyBorder(const int _modulatorID, int& _lower, int& _upper) = 0;

    //------------------------------------------------ UDI
    //virtual void dSLinkConfigWrite(devid_t _devAdr, uint8_t _index, uint8_t _value) = 0;
    //virtual bool dSLinkConfigRead(devid_t _devAdr, uint8_t _index, uint8_t& value) = 0;
    virtual uint8_t dSLinkSend(const int _modulatorID, devid_t _devAdr, uint8_t _value, uint8_t _flags) = 0;

    //------------------------------------------------ Device manipulation
    virtual std::vector<int> sendCommand(DS485Command _cmd, const Set& _set, int _param = -1) = 0;
    virtual std::vector<int> sendCommand(DS485Command _cmd, const Device& _device, int _param = -1) = 0;
    virtual std::vector<int> sendCommand(DS485Command _cmd, devid_t _id, uint8_t _modulatorID, int _param = -1) = 0;
    virtual std::vector<int> sendCommand(DS485Command _cmd, const Zone& _zone, Group& _group, int _param = -1) = 0;
    virtual std::vector<int> sendCommand(DS485Command _cmd, const Zone& _zone, uint8_t _groupID, int _param = -1) = 0;
    
    virtual void setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size) = 0;
    virtual int getSensorValue(const Device& _device, const int _sensorID) = 0;
  };

  class DS485ApiError : public DSSException {
  public:
    DS485ApiError(const std::string& _what)
    : DSSException(_what) {}
  }; // DS485ApiError

}
#endif /* DS485INTERFACE_H_ */
