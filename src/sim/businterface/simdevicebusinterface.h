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

#ifndef SIMDEVICEBUSINTERFACE_H_
#define SIMDEVICEBUSINTERFACE_H_

#include "src/businterface.h"

namespace dss {

  class DSSim;

  class SimDeviceBusInterface : public DeviceBusInterface {
  public:
    SimDeviceBusInterface(boost::shared_ptr<DSSim> _pSimulation)
    : m_pSimulation(_pSimulation)
    { }
    virtual uint8_t getDeviceConfig(const Device& _device,
                                    uint8_t _configClass,
                                    uint8_t _configIndex);

    virtual uint16_t getDeviceConfigWord(const Device& _device,
                                       uint8_t _configClass,
                                       uint8_t _configIndex);

    virtual void setDeviceConfig(const Device& _device, uint8_t _configClass,
                                 uint8_t _configIndex, uint8_t _value);

    virtual void setDeviceProgMode(const Device& _device, uint8_t _modeId);

    virtual void setValue(const Device& _device, uint8_t _value);

    virtual void addGroup(const Device& _device, const int _groupId);
    virtual void removeGroup(const Device& _device, const int _groupId);

    /** Queries sensor value and type from a device */
    virtual uint32_t getSensorValue(const Device& _device, const int _sensorIndex);
    virtual uint8_t getSensorType(const Device& _device, const int _sensorIndex);

    /** Tells the dSM to lock the device if \a _lock is true. */
    virtual void lockOrUnlockDevice(const Device& _device, const bool _lock);

    virtual std::pair<uint8_t, uint16_t> getTransmissionQuality(const Device& _device);
  private:
    boost::shared_ptr<DSSim> m_pSimulation;
  }; // SimDeviceBusInterface

}

#endif
