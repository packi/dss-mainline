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

#include "dsdevicebusinterface.h"

#include "dsbusinterface.h"

#include "core/dsidhelper.h"
#include "core/logger.h"

#include "core/model/device.h"

namespace dss {

  //================================================== DSDeviceBusInterface

  uint16_t DSDeviceBusInterface::deviceGetParameterValue(devid_t _id, const dss_dsid_t& _dsMeterDSID, int _paramID) {
    // TODO: libdsm
    Logger::getInstance()->log("deviceGetParameterValue: not implemented yet", lsInfo);
    return 0;
  } // deviceGetParameterValue

  void DSDeviceBusInterface::setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size) {
    // TODO: libdsm
    Logger::getInstance()->log("Not implemented yet: setValueDevice()", lsInfo);
  } // setValueDevice

  int DSDeviceBusInterface::getSensorValue(const Device& _device, const int _sensorID) {
    // TODO: libdsm
    Logger::getInstance()->log("getSensorValue(): not implemented yet", lsInfo);
    return 0;
  } // getSensorValue

  void DSDeviceBusInterface::lockOrUnlockDevice(const Device& _device, const bool _lock) {
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    int ret = DeviceProperties_set_locked_flag(m_DSMApiHandle, dsmDSID, _device.getShortAddress(), _lock);
    DSBusInterface::checkResultCode(ret);
  } // lockOrUnlockDevice

} // namespace dss
