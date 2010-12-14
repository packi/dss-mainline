/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

  // default timeout for synchronous calls to dsm api
  static const int kDSM_API_TIMEOUT = 5;

  uint8_t DSDeviceBusInterface::getDeviceConfig(const Device& _device,
                                                uint8_t _configClass,
                                                uint8_t _configIndex) {
    if(m_DSMApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    uint8_t retVal;

    int ret = DeviceConfig_get_sync_8(m_DSMApiHandle, dsmDSID,
                                      _device.getShortAddress(),
                                      _configClass,
                                      _configIndex,
                                      kDSM_API_TIMEOUT,
                                      &retVal);
    DSBusInterface::checkResultCode(ret);

    return retVal;
  } // getDeviceConfig

  uint16_t DSDeviceBusInterface::getDeviceConfigWord(const Device& _device,
                                                 uint8_t _configClass,
                                                 uint8_t _configIndex) {
    if(m_DSMApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    uint16_t retVal;

    int ret = DeviceConfig_get_sync_16(m_DSMApiHandle, dsmDSID,
                                       _device.getShortAddress(),
                                       _configClass,
                                       _configIndex,
                                       kDSM_API_TIMEOUT,
                                       &retVal);
    DSBusInterface::checkResultCode(ret);

    return retVal;
  } // getDeviceConfigWord


  void DSDeviceBusInterface::setDeviceConfig(const Device& _device,
                                             uint8_t _configClass,
                                             uint8_t _configIndex,
                                             uint8_t _value) {
    if(m_DSMApiHandle == NULL) {
      return;
    }

    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    int ret = DeviceConfig_set(m_DSMApiHandle, dsmDSID,
                               _device.getShortAddress(), _configClass,
                               _configIndex, _value);
    DSBusInterface::checkResultCode(ret);
  } // setDeviceConfig

  void DSDeviceBusInterface::setOutputValue(const Device& _device,
                                            uint8_t _value) {
    if(m_DSMApiHandle == NULL) {
      return;
    }

    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    int ret = DeviceActionRequest_action_set_outval(m_DSMApiHandle, dsmDSID,
                                                    _device.getShortAddress(),
                                                    _value);
    DSBusInterface::checkResultCode(ret);
  } // setOutputValue


  int DSDeviceBusInterface::getSensorValue(const Device& _device, const int _sensorID) {
    // TODO: libdsm
    Logger::getInstance()->log("getSensorValue(): not implemented yet", lsInfo);
    return 0;
  } // getSensorValue

  void DSDeviceBusInterface::lockOrUnlockDevice(const Device& _device, const bool _lock) {
    if(m_DSMApiHandle == NULL) {
      return;
    }
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    int ret = DeviceProperties_set_locked_flag(m_DSMApiHandle, dsmDSID, _device.getShortAddress(), _lock);
    DSBusInterface::checkResultCode(ret);
  } // lockOrUnlockDevice

  void DSDeviceBusInterface::setOutputLoad(const Device& _device, const bool _load) {
    if(m_DSMApiHandle == NULL) {
      return;
    }
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    int ret = DeviceProperties_set_output_load(m_DSMApiHandle, dsmDSID, _device.getShortAddress(), _load);
    DSBusInterface::checkResultCode(ret);
  } // setOutputLoad

  std::pair<uint8_t, uint16_t> DSDeviceBusInterface::getTransmissionQuality(const Device& _device) {
    if(m_DSMApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    uint8_t downstream;
    uint16_t upstream;

    int ret = TestTransmissionQuality_get_sync(m_DSMApiHandle, dsmDSID,
                                               _device.getShortAddress(),
                                               kDSM_API_TIMEOUT,
                                               &downstream, &upstream);
    DSBusInterface::checkResultCode(ret);
    return std::make_pair(downstream, upstream);
  }
} // namespace dss
