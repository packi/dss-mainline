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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <bitset>

#include "dsdevicebusinterface.h"

#include "dsbusinterface.h"
#include "src/logger.h"
#include "src/model/device.h"
#include "src/model/modelevent.h"
#include "src/model/modelmaintenance.h"
#include "src/model/modulator.h"
#include "src/model/apartment.h"
#include "src/model/modelconst.h"
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

    uint8_t retVal;

    int ret = DeviceConfig_get_sync_8(m_DSMApiHandle, _device.getDSMeterDSID(),
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

    uint16_t retVal;

    int ret = DeviceConfig_get_sync_16(m_DSMApiHandle, _device.getDSMeterDSID(),
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

    boost::shared_ptr<DSMeter> pMeter = DSS::getInstance()->getApartment()
                                        .getDSMeterByDSID(_device.getDSMeterDSID());

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    int ret;
    if (pMeter->getApiVersion() >= 0x301) {
      ret = DeviceConfig_set_sync(m_DSMApiHandle, _device.getDSMeterDSID(),
                                  _device.getShortAddress(), _configClass,
                                  _configIndex, _value, 30);
    } else {
      ret = DeviceConfig_set(m_DSMApiHandle, _device.getDSMeterDSID(),
                             _device.getShortAddress(), _configClass,
                             _configIndex, _value);
    }
    DSBusInterface::checkResultCode(ret);
  } // setDeviceConfig

  void DSDeviceBusInterface::setDeviceButtonActiveGroup(const Device& _device,
                                                        uint8_t _groupID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    int ret = DeviceProperties_set_button_active_group(m_DSMApiHandle,
                                          _device.getDSMeterDSID(),
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

    if(_modeId) {
      ret = DeviceActionRequest_action_programming_mode_on(m_DSMApiHandle,
                _device.getDSMeterDSID(), _device.getShortAddress());
    } else {
      ret = DeviceActionRequest_action_programming_mode_off(m_DSMApiHandle,
          _device.getDSMeterDSID(), _device.getShortAddress());
    }
    DSBusInterface::checkResultCode(ret);
  } // setDeviceProgMode

  void DSDeviceBusInterface::setValue(const Device& _device,
                                            uint8_t _value) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    int ret = DeviceActionRequest_action_set_outval(m_DSMApiHandle,
                                                    _device.getDSMeterDSID(),
                                                    _device.getShortAddress(),
                                                    _value);
    DSBusInterface::checkResultCode(ret);
  } // setValue

  uint32_t DSDeviceBusInterface::getSensorValue(const Device& _device,
                                                     const int _sensorIndex) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    uint16_t retVal;

    int ret = DeviceSensor_get_value_sync(m_DSMApiHandle,
                                          _device.getDSMeterDSID(),
                                          _device.getShortAddress(),
                                          _sensorIndex, kDSM_API_TIMEOUT,
                                          &retVal);
    DSBusInterface::checkResultCode(ret);

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceSensorValue, _device.getDSMeterDSID());
    pEvent->addParameter(_device.getShortAddress());
    pEvent->addParameter(_sensorIndex);
    pEvent->addParameter(retVal);
    if (DSS::hasInstance()) {
      DSS::getInstance()->getModelMaintenance().addModelEvent(pEvent);
    } else {
      delete pEvent;
    }

    return retVal;
  } // getSensorValue

  void DSDeviceBusInterface::addGroup(const Device& _device, const int _groupID) {

    boost::shared_ptr<DSMeter> pMeter = DSS::getInstance()->getApartment()
                                        .getDSMeterByDSID(_device.getDSMeterDSID());

    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }
    int ret = 0;
    if (pMeter->getApiVersion() >= 0x301) {
      ret = DeviceGroupMembershipModify_add_sync(m_DSMApiHandle,
                                                 _device.getDSMeterDSID(),
                                                 _device.getShortAddress(),
                                                 _groupID, 30);
    } else {
      ret = DeviceGroupMembershipModify_add(m_DSMApiHandle,
                                            _device.getDSMeterDSID(),
                                            _device.getShortAddress(),
                                            _groupID);
    }
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

    int ret = DeviceGroupMembershipModify_remove(m_DSMApiHandle,
                                                 _device.getDSMeterDSID(),
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

    int ret = DeviceProperties_set_locked_flag(m_DSMApiHandle,
                                               _device.getDSMeterDSID(),
                                               _device.getShortAddress(),
                                               _lock);
    DSBusInterface::checkResultCode(ret);
  } // lockOrUnlockDevice

  std::pair<uint8_t, uint16_t> DSDeviceBusInterface::getTransmissionQuality(const Device& _device) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    uint8_t downstream;
    uint16_t upstream;

    int ret = TestTransmissionQuality_get_sync(m_DSMApiHandle,
                                               _device.getDSMeterDSID(),
                                               _device.getShortAddress(),
                                               kDSM_API_TIMEOUT,
                                               &downstream, &upstream);
    DSBusInterface::checkResultCode(ret);
    return std::make_pair(downstream, upstream);
  }

  void DSDeviceBusInterface::increaseDeviceOutputChannelValue(
                                                        const Device& _device,
                                                        uint8_t _channel) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    int ret = DeviceActionRequest_action_opc_inc(m_DSMApiHandle,
                                                 _device.getDSMeterDSID(),
                                                 _device.getShortAddress(),
                                                 _channel);
    DSBusInterface::checkResultCode(ret);
  } // increaseDeviceOutputChannelValue

  void DSDeviceBusInterface::decreaseDeviceOutputChannelValue(const Device& _device,
                                                        uint8_t _channel) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    int ret = DeviceActionRequest_action_opc_dec(m_DSMApiHandle,
                                                 _device.getDSMeterDSID(),
                                                 _device.getShortAddress(),
                                                 _channel);
    DSBusInterface::checkResultCode(ret);
  } // decreaseDeviceOutputChannelValue

  void DSDeviceBusInterface::stopDeviceOutputChannelValue(const Device& _device,
                                                    uint8_t _channel) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    int ret = DeviceActionRequest_action_opc_stop(m_DSMApiHandle,
                                                  _device.getDSMeterDSID(),
                                                  _device.getShortAddress(),
                                                  _channel);
    DSBusInterface::checkResultCode(ret);
  } // stopDeviceOutputChannelValue

  uint16_t DSDeviceBusInterface::getDeviceOutputChannelValue(
                                                        const Device& _device,
                                                        uint8_t _channel) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");;
    }

    uint16_t out = 0;

    int ret = DeviceOPCConfig_get_current_sync(m_DSMApiHandle,
                                               _device.getDSMeterDSID(),
                                               _device.getShortAddress(),
                                               _channel, 2*kDSM_API_TIMEOUT,
                                               &out);
    DSBusInterface::checkResultCode(ret);
    return out;
  } // getDeviceOutputChannelValue

  void DSDeviceBusInterface::setDeviceOutputChannelValue(const Device& _device,
                                   uint8_t _channel, uint8_t _size,
                                   uint16_t _value, bool _applyNow) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    int ret;

    if (_applyNow) {
      ret = DeviceOPCConfig_set_current_and_apply(m_DSMApiHandle,
                                                  _device.getDSMeterDSID(),
                                                  _device.getShortAddress(),
                                                  _channel, _size, _value);
    } else {
      ret = DeviceOPCConfig_set_current(m_DSMApiHandle,
                                        _device.getDSMeterDSID(),
                                        _device.getShortAddress(),
                                        _channel, _size, _value);
    }
    DSBusInterface::checkResultCode(ret);
  } // setDeviceOutputChannelValue

  uint16_t DSDeviceBusInterface::getDeviceOutputChannelSceneValue(
                                                        const Device& _device,
                                                        uint8_t _channel,
                                                        uint8_t _scene) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    uint16_t out = 0;

    int ret = DeviceOPCConfig_get_scene_sync(m_DSMApiHandle,
                                             _device.getDSMeterDSID(),
                                             _device.getShortAddress(),
                                             _channel, _scene,
                                             2*kDSM_API_TIMEOUT, &out);
    DSBusInterface::checkResultCode(ret);
    return out;
  } // getDeviceOutputChannelSceneValue

  void DSDeviceBusInterface::setDeviceOutputChannelSceneValue(
                                                        const Device& _device,
                                                        uint8_t _channel,
                                                        uint8_t _size,
                                                        uint8_t _scene,
                                                        uint16_t _value) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    int ret = DeviceOPCConfig_set_scene(m_DSMApiHandle,
                                        _device.getDSMeterDSID(),
                                        _device.getShortAddress(),
                                        _channel, _size, _scene, _value);
    DSBusInterface::checkResultCode(ret);
  } // setDeviceOutputChannelSceneValue

  uint16_t DSDeviceBusInterface::getDeviceOutputChannelSceneConfig(
                                                        const Device& _device,
                                                        uint8_t _scene) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    uint16_t out = 0;

    int ret = DeviceOPCConfig_get_scene_config_sync(m_DSMApiHandle,
                                                    _device.getDSMeterDSID(),
                                                    _device.getShortAddress(),
                                                    _scene,
                                                    2*kDSM_API_TIMEOUT, &out);
    DSBusInterface::checkResultCode(ret);
    return out;
  } // getDeviceOutputChannelSceneConfig

  void DSDeviceBusInterface::setDeviceOutputChannelSceneConfig(
                                                        const Device& _device,
                                                        uint8_t _scene,
                                                        uint16_t _value) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    int ret = DeviceOPCConfig_set_scene_config(m_DSMApiHandle,
                                               _device.getDSMeterDSID(),
                                               _device.getShortAddress(),
                                               _scene, _value);
    DSBusInterface::checkResultCode(ret);
  } // setDeviceOutputChannelSceneConfig

  void DSDeviceBusInterface::setDeviceOutputChannelDontCareFlags(
                                                        const Device& _device,
                                                        uint8_t _scene,
                                                        uint16_t _value) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return;
    }

    int ret = DeviceOPCConfig_set_dc_flags(m_DSMApiHandle,
                                           _device.getDSMeterDSID(),
                                           _device.getShortAddress(),
                                           _scene, _value);
    DSBusInterface::checkResultCode(ret);
  }

  uint16_t DSDeviceBusInterface::getDeviceOutputChannelDontCareFlags(
                                                        const Device& _device,
                                                        uint8_t _scene) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    uint16_t out = 0;

    int ret = DeviceOPCConfig_get_dc_flags_sync(m_DSMApiHandle,
                                                _device.getDSMeterDSID(),
                                                _device.getShortAddress(),
                                                _scene,
                                                2*kDSM_API_TIMEOUT, &out);
    DSBusInterface::checkResultCode(ret);
    return out;
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

  uint16_t DSDeviceBusInterface::OEMDataReader::getDeviceConfigWord(uint8_t _configClass, uint8_t _configIndex) const
  {
    if (m_dsmApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    // wait until dsm has finished device registrations
    while (m_dsm && m_dsm->isPresent() && m_dsm->getState()) {
      sleep(5);
    }
    if ((m_dsm == NULL) || !m_dsm->isPresent()) {
      throw std::runtime_error("OEMDataReader: dsm is not present");
    }

    uint16_t retVal;
    int ret = DeviceConfig_get_sync_16(m_dsmApiHandle, m_dsmId, m_deviceAdress, _configClass, _configIndex, 15, &retVal);
    if (ret == ERROR_SYNC_RESPONSE_TIMEOUT || ret == ERROR_TIMEOUT) {
      ret = DeviceConfig_get_sync_16(m_dsmApiHandle, m_dsmId, m_deviceAdress, _configClass, _configIndex, 15, &retVal);
    }
    DSBusInterface::checkResultCode(ret);

    return retVal;
  }

  uint8_t DSDeviceBusInterface::OEMDataReader::getDeviceConfig(uint8_t _configClass, uint8_t _configIndex) const
  {
    if (m_dsmApiHandle == NULL) {
      throw std::runtime_error("Invalid libdsm api handle");
    }

    // wait until dsm has finished device registrations
    while (m_dsm && m_dsm->isPresent() && m_dsm->getState()) {
      sleep(5);
    }
    if ((m_dsm == NULL) || !m_dsm->isPresent()) {
      throw std::runtime_error("OEMDataReader: dsm is not present");
    }

    uint8_t retVal;
    int ret = DeviceConfig_get_sync_8(m_dsmApiHandle, m_dsmId, m_deviceAdress, _configClass, _configIndex, 15, &retVal);
    if (ret == ERROR_SYNC_RESPONSE_TIMEOUT || ret == ERROR_TIMEOUT) {
      ret = DeviceConfig_get_sync_8(m_dsmApiHandle, m_dsmId, m_deviceAdress, _configClass, _configIndex, 15, &retVal);
    }
    DSBusInterface::checkResultCode(ret);

    return retVal;
  }

  void DSDeviceBusInterface::OEMDataReader::run()
  {
    uint64_t ean = 0;
    uint16_t serialNumber = 0;
    uint8_t partNumber = 0;
    bool isVisible = false;
    bool isIndependent = false;
    bool isConfigLocked = false;
    uint8_t pairedDevices = 0;
    DeviceOEMState_t state = DEVICE_OEM_UNKOWN;
    DeviceOEMInetState_t deviceInetState = DEVICE_OEM_EAN_NO_EAN_CONFIGURED;

    m_dsmApiHandle = DsmApiInitialize();
    if (!m_dsmApiHandle) {
      throw std::runtime_error("OEMDataReader: Unable to get dsmapi handle");
    }

    int result = DsmApiOpen(m_dsmApiHandle, m_busConnection.c_str(), 0);
    if (result < 0) {
      throw std::runtime_error(std::string("OEMDataReader: Unable to open connection to: ") + m_busConnection);
    }

    try {
      // read device-active data and EAN part number
      uint16_t result0;
      result0 = getDeviceConfigWord(1, 0x1e);
      partNumber = result0 & 0x7F;
      isIndependent = (result0 & 0x80);
      uint8_t upper = result0 >> 8;
      isVisible = !(upper & (1 << 4));
      pairedDevices = (upper & 0x0f);

      // optimize readout time: avoid switching banks
      serialNumber = getDeviceConfigWord(1, 0x1c);

      // check if EAN is programmed: Bank 3: 0x2e-0x2f 0x0000 < x < 0xffff
      uint16_t result;
      result = getDeviceConfigWord(3, 0x2e);
      deviceInetState = (DeviceOEMInetState_t)(result >> 12);

      if (deviceInetState == DEVICE_OEM_EAN_NO_EAN_CONFIGURED) {
        // no EAN programmed
        state = DEVICE_OEM_NONE;
      } else {
        // EAN programmed
        ean |= ((long long unsigned int)(result & 0xFFF) << 32);

        result = getDeviceConfigWord(3, 0x2a);
        ean |= result;

        result = getDeviceConfigWord(3, 0x2c);
        ean |= ((long long unsigned int)result << 16);

        state = DEVICE_OEM_VALID;
      }

      if (m_revisionID >= TBVersion_OemConfigLock) {
        uint8_t deviceActive;
        deviceActive = getDeviceConfig(3, 0x1f);
        if (std::bitset<8>(deviceActive).test(0)) {
          isConfigLocked = true;
        }
      }

    } catch (BusApiError& er) {
      // Bus error
      Logger::getInstance()->log(std::string("OEMDataReader::run: bus error: ") + er.what() +
          " reading from dSM " + dsuid2str(m_dsmId) +
          " DeviceId " + intToString(m_deviceAdress), lsWarning);
    }

    ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceEANReady, m_dsmId);
    pEvent->addParameter(m_deviceAdress);
    pEvent->addParameter(state);
    pEvent->addParameter(deviceInetState);
    pEvent->addParameter(ean >> 32);
    pEvent->addParameter(ean & 0xFFFFFFFF);
    pEvent->addParameter(serialNumber);
    pEvent->addParameter(partNumber);
    pEvent->addParameter(isIndependent);
    pEvent->addParameter(isConfigLocked);
    pEvent->addParameter(pairedDevices);
    pEvent->addParameter(isVisible);
    if (DSS::hasInstance()) {
      DSS::getInstance()->getModelMaintenance().addModelEvent(pEvent);
    } else {
      delete pEvent;
    }
  }

  void DSDeviceBusInterface::OEMDataReader::setup(boost::shared_ptr<Device> _device)
  {
    m_deviceAdress = _device->getShortAddress();
    m_dsmId = _device->getDSMeterDSID();
    m_revisionID = _device->getRevisionID();
    if (DSS::hasInstance()) {
      m_dsm = DSS::getInstance()->getApartment().getDSMeterByDSID(m_dsmId);
    }
  }

  DSDeviceBusInterface::TNYConfigReader::TNYConfigReader(const std::string& _busConnection) : OEMDataReader(_busConnection)
  { }

  DSDeviceBusInterface::TNYConfigReader::~TNYConfigReader()
  { }

  void DSDeviceBusInterface::TNYConfigReader::run()
  {
    bool isVisible = false;
    uint8_t pairedDevices = 0;

    m_dsmApiHandle = DsmApiInitialize();
    if (!m_dsmApiHandle) {
      throw std::runtime_error("TNYConfigReader: Unable to get dsmapi handle");
    }

    int result = DsmApiOpen(m_dsmApiHandle, m_busConnection.c_str(), 0);
    if (result < 0) {
      throw std::runtime_error(std::string("TNYConfigReader: Unable to open connection to: ") + m_busConnection);
    }

    try {
      // read device-active data and pairing data
      uint8_t cfg;
      cfg = getDeviceConfig(1, CfgFunction_DeviceActive);
      isVisible = !(cfg & (1 << 4));
      pairedDevices = (cfg & 0x0f);

      ModelEvent* pEvent = new ModelEventWithDSID(ModelEvent::etDeviceDataReady, m_dsmId);
      pEvent->addParameter(m_deviceAdress);
      pEvent->addParameter(pairedDevices);
      pEvent->addParameter(isVisible);
      if (DSS::hasInstance()) {
        DSS::getInstance()->getModelMaintenance().addModelEvent(pEvent);
      } else {
        delete pEvent;
      }

    } catch (BusApiError& er) {
      // Bus error
      Logger::getInstance()->log(std::string("TNYConfigReader::run: bus error: ") + er.what() +
          " reading from dSM " + dsuid2str(m_dsmId) +
          " DeviceId " + intToString(m_deviceAdress), lsWarning);
    }
  }

} // namespace dss
