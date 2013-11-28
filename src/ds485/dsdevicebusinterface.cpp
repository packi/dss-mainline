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

#include <bitset>

#include "dsdevicebusinterface.h"

#include "dsbusinterface.h"

#include "src/dsidhelper.h"
#include "src/logger.h"

#include "src/model/device.h"
#include "src/model/modelevent.h"
#include "src/model/modelmaintenance.h"
#include "dss.h"

namespace dss {

  //================================================== DSDeviceBusInterface

  // default timeout for synchronous calls to dsm api
  static const int kDSM_API_TIMEOUT = 8;

  uint8_t DSDeviceBusInterface::getDeviceConfig(const Device& _device,
                                                uint8_t _configClass,
                                                uint8_t _configIndex) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
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
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
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
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
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

  void DSDeviceBusInterface::setDeviceButtonActiveGroup(const Device& _device,
                                                        uint8_t _groupID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    int ret = DeviceProperties_set_button_active_group(m_DSMApiHandle, dsmDSID,
                                          _device.getShortAddress(), _groupID);
    DSBusInterface::checkResultCode(ret);
  } // setDeviceButtonActiveGroup

  void DSDeviceBusInterface::setDeviceProgMode(const Device& _device,
                                                     uint8_t _modeId) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    int ret;
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    if(_modeId) {
      ret = DeviceActionRequest_action_programming_mode_on(m_DSMApiHandle,
          dsmDSID, _device.getShortAddress());
    } else {
      ret = DeviceActionRequest_action_programming_mode_off(m_DSMApiHandle,
          dsmDSID, _device.getShortAddress());
    }
    DSBusInterface::checkResultCode(ret);
  } // setDeviceProgMode

  void DSDeviceBusInterface::setValue(const Device& _device,
                                            uint8_t _value) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    int ret = DeviceActionRequest_action_set_outval(m_DSMApiHandle, dsmDSID,
                                                    _device.getShortAddress(),
                                                    _value);
    DSBusInterface::checkResultCode(ret);
  } // setValue

  uint32_t DSDeviceBusInterface::getSensorValue(const Device& _device,
                                                     const int _sensorIndex) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);
    uint16_t retVal;

    int ret = DeviceSensor_get_value_sync(m_DSMApiHandle, dsmDSID,
        _device.getShortAddress(), _sensorIndex,
        kDSM_API_TIMEOUT, &retVal);

    DSBusInterface::checkResultCode(ret);
    _device.setSensorValue(_sensorIndex, retVal);
    return retVal;
  } // getSensorValue

  uint8_t DSDeviceBusInterface::getSensorType(const Device& _device,
                                              const int _sensorIndex) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);
    uint8_t present, type, last;

    int ret = DeviceSensor_get_type_sync(m_DSMApiHandle, dsmDSID,
        _device.getShortAddress(), _sensorIndex,
        kDSM_API_TIMEOUT, &present, &type, &last);

    DSBusInterface::checkResultCode(ret);
    return type;
  } // getSensorType

  void DSDeviceBusInterface::addGroup(const Device& _device, const int _groupID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    int ret = DeviceGroupMembershipModify_add(m_DSMApiHandle, dsmDSID,
                                              _device.getShortAddress(),
                                              _groupID);
    if(ret == ERROR_WRONG_PARAMETER) {
      Logger::getInstance()->log("addGroup: dsm-api returned wrong parameter", lsWarning);
    } else {
      DSBusInterface::checkResultCode(ret);
    }
  } // addGroup

  void DSDeviceBusInterface::removeGroup(const Device& _device, const int _groupID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    int ret = DeviceGroupMembershipModify_remove(m_DSMApiHandle, dsmDSID,
                                                 _device.getShortAddress(),
                                                 _groupID);
    if(ret == ERROR_WRONG_PARAMETER) {
      Logger::getInstance()->log("addGroup: dsm-api returned wrong parameter", lsWarning);
    } else {
      DSBusInterface::checkResultCode(ret);
    }
  } // removeGroup

  void DSDeviceBusInterface::lockOrUnlockDevice(const Device& _device, const bool _lock) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device.getDSMeterDSID(), dsmDSID);

    int ret = DeviceProperties_set_locked_flag(m_DSMApiHandle, dsmDSID, _device.getShortAddress(), _lock);
    DSBusInterface::checkResultCode(ret);
  } // lockOrUnlockDevice

  std::pair<uint8_t, uint16_t> DSDeviceBusInterface::getTransmissionQuality(const Device& _device) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
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

  DSDeviceBusInterface::OEMDataReader::OEMDataReader(const std::string& _busConnection)
    : m_busConnection(_busConnection)
    , m_dsmApiHandle(NULL)
    , m_deviceAdress(0)
    , m_dsmId()
    , m_revisionID(0)
  {
  }

  DSDeviceBusInterface::OEMDataReader::~OEMDataReader()
  {
    if (m_dsmApiHandle != NULL) {
      DsmApiClose(m_dsmApiHandle);
      DsmApiCleanup(m_dsmApiHandle);
      m_dsmApiHandle = NULL;
    }
  }

  uint16_t DSDeviceBusInterface::OEMDataReader::getDeviceConfigWord(const dsid_t& _dsm,
                                                                            dev_t _device,
                                                                            uint8_t _configClass,
                                                                            uint8_t _configIndex) const
  {
    if (m_dsmApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    uint16_t retVal;

    int ret = DeviceConfig_get_sync_16(m_dsmApiHandle, _dsm,
                                           _device,
                                           _configClass,
                                           _configIndex,
                                           30,
                                           &retVal);
    DSBusInterface::checkResultCode(ret);

    return retVal;
  }

  uint8_t DSDeviceBusInterface::OEMDataReader::getDeviceConfig(const dsid_t& _dsm,
                                                                      dev_t _device,
                                                                      uint8_t _configClass,
                                                                      uint8_t _configIndex) const
  {
    if (m_dsmApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    uint8_t retVal;

    int ret = DeviceConfig_get_sync_8(m_dsmApiHandle, _dsm,
                                      _device,
                                      _configClass,
                                      _configIndex,
                                      30,
                                      &retVal);
    DSBusInterface::checkResultCode(ret);

    return retVal;
  }

  void DSDeviceBusInterface::OEMDataReader::run()
  {
    uint64_t ean = 0;
    uint16_t serialNumber = 0;
    uint8_t partNumber = 0;
    bool isIndependent = false;
    bool isConfigLocked = false;
    DeviceOEMState_t state = DEVICE_OEM_UNKOWN;
    DeviceOEMInetState_t deviceInetState = DEVICE_OEM_EAN_NO_EAN_CONFIGURED;
    dsid_t dsmId;
    dsid_helper::toDsmapiDsid(m_dsmId, dsmId);

    m_dsmApiHandle = DsmApiInitialize();
    if (!m_dsmApiHandle) {
      throw std::runtime_error("OEMDataReader: Unable to get dsmapi handle");
    }

    int result = DsmApiOpen(m_dsmApiHandle, m_busConnection.c_str(), 0);
    if (result < 0) {
      throw std::runtime_error(std::string("OEMDataReader: Unable to open connection to: ") + m_busConnection);
    }

    try {
      // check if EAN is programmed: Bank 3: 0x2e-0x2f 0x0000 < x < 0xffff
      uint16_t result = getDeviceConfigWord(dsmId, m_deviceAdress, 3, 0x2e);
      deviceInetState = (DeviceOEMInetState_t)(result >> 12);

      if (deviceInetState == DEVICE_OEM_EAN_NO_EAN_CONFIGURED) {
        // no EAN programmed
        state = DEVICE_OEM_NONE;
      } else {
        // EAN programmed
        ean |= ((long long unsigned int)(result & 0xFFF) << 32);

        result = getDeviceConfigWord(dsmId, m_deviceAdress, 3, 0x2a);
        ean |= result;

        result = getDeviceConfigWord(dsmId, m_deviceAdress, 3, 0x2c);
        ean |= ((long long unsigned int)result << 16);

        serialNumber = getDeviceConfigWord(dsmId, m_deviceAdress, 1, 0x1c);

        partNumber = getDeviceConfig(dsmId, m_deviceAdress, 1, 0x1e);
        isIndependent = (partNumber & 0x80);
        partNumber &= 0x7F;

        state = DEVICE_OEM_VALID;
      }

      if (m_revisionID >= 0x357) {
        if (std::bitset<8>(
                    getDeviceConfig(dsmId, m_deviceAdress, 3, 0x1f)).test(0)) {
          isConfigLocked = true;
        }
      }

    } catch (BusApiError& er) {
      // Bus error
      Logger::getInstance()->log(std::string("OEMDataReader::run: bus error: ") + er.what(), lsWarning);
      return;
    }

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceEANReady,
                                                m_dsmId);
    pEvent->addParameter(m_deviceAdress);
    pEvent->addParameter(state);
    pEvent->addParameter(deviceInetState);
    pEvent->addParameter(ean >> 32);
    pEvent->addParameter(ean & 0xFFFFFFFF);
    pEvent->addParameter(serialNumber);
    pEvent->addParameter(partNumber);
    pEvent->addParameter(isIndependent);
    pEvent->addParameter(isConfigLocked);
    if(DSS::hasInstance()) {
      DSS::getInstance()->getModelMaintenance().addModelEvent(pEvent);
    }
  }

  void DSDeviceBusInterface::OEMDataReader::setup(boost::shared_ptr<Device> _device)
  {
    m_deviceAdress = _device->getShortAddress();
    m_dsmId = _device->getDSMeterDSID();
    m_revisionID = _device->getRevisionID();
  }

} // namespace dss
