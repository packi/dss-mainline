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

#ifndef DSDEVICEBUSINTERFACE_H_
#define DSDEVICEBUSINTERFACE_H_

#include "core/businterface.h"

#include "dsbusinterfaceobj.h"

namespace dss {

  class DSDeviceBusInterface : public DSBusInterfaceObj,
                               public DeviceBusInterface {
  public:
    DSDeviceBusInterface()
    : DSBusInterfaceObj()
    { }

    virtual uint16_t deviceGetParameterValue(devid_t _id, const dss_dsid_t& _dsMeterID, int _paramID) ;
    virtual void setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size) ;
    virtual int getSensorValue(const Device& _device, const int _sensorID);
    virtual void lockOrUnlockDevice(const Device& _device, const bool _lock);
  }; // DSDeviceBusInterface


} // namespace dss

#endif
