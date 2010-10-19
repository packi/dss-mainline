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

#include "simdevicebusinterface.h"

#include "core/sim/dssim.h"
#include "core/sim/dsmetersim.h"
#include "core/model/device.h"

namespace dss {

  //================================================== SimDeviceBusInterface

  uint16_t SimDeviceBusInterface::deviceGetParameterValue(devid_t _id, const dss_dsid_t& _dsMeterID, int _paramID) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_dsMeterID);
    if(pMeter != NULL) {
      DSIDInterface* pDevice = pMeter->getSimulatedDevice(_id);
      if(pDevice != NULL) {
        return pDevice->getValue(_paramID);
      }
    }
    return 0xFFFF;
  } // deviceGetParameterValue

  void SimDeviceBusInterface::setValueDevice(const Device& _device, const uint16_t _value, const uint16_t _parameterID, const int _size) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_device.getDSMeterDSID());
    if(pMeter != NULL) {
      DSIDInterface* pDevice = pMeter->getSimulatedDevice(_device.getShortAddress());
      if(pDevice != NULL) {
        pDevice->setValue(_value, _parameterID);
      }
    }
  } // setValueDevice

  int SimDeviceBusInterface::getSensorValue(const Device& _device, const int _sensorID) {
    return 0;
  } // getSensorValue

  void SimDeviceBusInterface::lockOrUnlockDevice(const Device& _device, const bool _lock) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_device.getDSMeterDSID());
    if(pMeter != NULL) {
      DSIDInterface* pDevice = pMeter->getSimulatedDevice(_device.getShortAddress());
      if(pDevice != NULL) {
        pDevice->setIsLocked(_lock);
      }
    }
  } // lockOrUnlockDevice

} // namespace dss
