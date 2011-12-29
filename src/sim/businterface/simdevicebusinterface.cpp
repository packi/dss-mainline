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

#include "src/sim/dssim.h"
#include "src/sim/dsmetersim.h"
#include "src/model/device.h"

namespace dss {

  //================================================== SimDeviceBusInterface

  uint8_t SimDeviceBusInterface::getDeviceConfig(const Device& _device,
                                                 const uint8_t _configClass,
                                                 const uint8_t _configIndex) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_device.getDSMeterDSID());
    if(pMeter != NULL) {
      DSIDInterface* pDevice = pMeter->getSimulatedDevice(_device.getShortAddress());
      if(pDevice != NULL) {
        return pDevice->getDeviceConfig(_configClass, _configIndex);
      }
    }
    return 0xFF;
  } // deviceGetParameterValue

  uint16_t SimDeviceBusInterface::getDeviceConfigWord(const Device& _device,
                                                   const uint8_t _configClass,
                                                   const uint8_t _configIndex) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_device.getDSMeterDSID());
    if(pMeter != NULL) {
      DSIDInterface* pDevice = pMeter->getSimulatedDevice(_device.getShortAddress());
      if(pDevice != NULL) {
        return pDevice->getDeviceConfigWord(_configClass, _configIndex);
      }
    }
    return 0xFFFF;
  } // deviceGetParameterValue

  void SimDeviceBusInterface::setDeviceConfig(const Device& _device,
                                              const uint8_t _configClass,
                                              const uint8_t _configIndex,
                                              const uint8_t _value) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_device.getDSMeterDSID());
    if(pMeter != NULL) {
      DSIDInterface* pDevice = pMeter->getSimulatedDevice(_device.getShortAddress());
      if(pDevice != NULL) {
        pDevice->setDeviceConfig(_configClass, _configIndex, _value);
      }
    }
  } // setDeviceConfig

  void SimDeviceBusInterface::setDeviceProgMode(const Device& _device,
                                                const uint8_t _modeId) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_device.getDSMeterDSID());
    if(pMeter != NULL) {
      DSIDInterface* pDevice = pMeter->getSimulatedDevice(_device.getShortAddress());
      if(pDevice != NULL) {
        pDevice->setDeviceProgMode(_modeId);
      }
    }
  } // setDeviceProgMode

  std::pair<uint8_t, uint16_t> SimDeviceBusInterface::getTransmissionQuality(const Device& _device) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_device.getDSMeterDSID());
    if(pMeter != NULL) {
      DSIDInterface* pDevice = pMeter->getSimulatedDevice(_device.getShortAddress());
      if(pDevice != NULL) {
        return pDevice->getTransmissionQuality();
      }
    }
    return std::make_pair(0, 0);
  } // getTransmissionQuality

  void SimDeviceBusInterface::setValue(const Device& _device,
                                             const uint8_t _value) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_device.getDSMeterDSID());
    if(pMeter != NULL) {
      DSIDInterface* pDevice = pMeter->getSimulatedDevice(_device.getShortAddress());
      if(pDevice != NULL) {
        pDevice->setValue(_value);
      }
    }
  } // setValue

  void SimDeviceBusInterface::addGroup(const Device& _device, const int _groupId) {
  } // addGroup

  void SimDeviceBusInterface::removeGroup(const Device& _device, const int _groupId) {
  } // removeGroup

  uint32_t SimDeviceBusInterface::getSensorValue(const Device& _device, const int _sensorIndex) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_device.getDSMeterDSID());
    if(pMeter != NULL) {
      DSIDInterface* pDevice = pMeter->getSimulatedDevice(_device.getShortAddress());
      if(pDevice != NULL) {
        return pDevice->getDeviceSensorValue(_sensorIndex);
      }
    }
    return 0;
  } // getSensorValue

  uint8_t SimDeviceBusInterface::getSensorType(const Device& _device, const int _sensorIndex) {
    boost::shared_ptr<DSMeterSim> pMeter = m_pSimulation->getDSMeter(_device.getDSMeterDSID());
    if(pMeter != NULL) {
      DSIDInterface* pDevice = pMeter->getSimulatedDevice(_device.getShortAddress());
      if(pDevice != NULL) {
        return pDevice->getDeviceSensorType(_sensorIndex);
      }
    }
    return 255;
  } // getSensorType

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
