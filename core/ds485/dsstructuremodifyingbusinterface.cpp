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

#include "dsstructuremodifyingbusinterface.h"

#include "dsbusinterface.h"

#include "core/dsidhelper.h"

namespace dss {

  //================================================== DSStructureModifyingBusInterface

  void DSStructureModifyingBusInterface::setZoneID(const dss_dsid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    int ret = DeviceProperties_set_zone(m_DSMApiHandle, meterDSID, _deviceID, _zoneID);
    DSBusInterface::checkResultCode(ret);
  } // setZoneID

  void DSStructureModifyingBusInterface::createZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    int ret = ZoneModify_add(m_DSMApiHandle, meterDSID, _zoneID);
    DSBusInterface::checkResultCode(ret);
  } // createZone

  void DSStructureModifyingBusInterface::removeZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    int ret = ZoneModify_remove(m_DSMApiHandle, meterDSID, _zoneID);
    DSBusInterface::checkResultCode(ret);
  } // removeZone

  void DSStructureModifyingBusInterface::addToGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    int ret = DeviceGroupMembershipModify_add(m_DSMApiHandle, meterDSID, _deviceID, _groupID);
    DSBusInterface::checkResultCode(ret);
  } // addToGroup

  void DSStructureModifyingBusInterface::removeFromGroup(const dss_dsid_t& _dsMeterID, const int _groupID, const int _deviceID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    int ret = DeviceGroupMembershipModify_remove(m_DSMApiHandle, meterDSID, _deviceID, _groupID);
    DSBusInterface::checkResultCode(ret);
  } // removeFromGroup

  void DSStructureModifyingBusInterface::removeDeviceFromDSMeter(const dss_dsid_t& _dsMeterID, const int _deviceID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    if (IsNullId(meterDSID)) {
      return;
    }

    int ret = CircuitRemoveDevice(m_DSMApiHandle, meterDSID, _deviceID);
    DSBusInterface::checkResultCode(ret);
  } // removeDeviceFromDSMeter

  void DSStructureModifyingBusInterface::sceneSetName(uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneNumber, const std::string& _name) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::string nameStr = truncateUTF8String(_name, 19);
    uint8_t name[20];
    strncpy(reinterpret_cast<char*>(name), nameStr.c_str(), 20);
    int ret = ZoneGroupSceneProperties_set_name(m_DSMApiHandle, m_BroadcastDSID, _zoneID, _groupID, _sceneNumber, name);
    DSBusInterface::checkBroadcastResultCode(ret);
  } // sceneSetName
  
  void DSStructureModifyingBusInterface::deviceSetName(dss_dsid_t _meterDSID, devid_t _deviceID, const std::string& _name) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::string nameStr = truncateUTF8String(_name, 19);
    uint8_t name[20];
    strncpy(reinterpret_cast<char*>(name), nameStr.c_str(), 20);
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_meterDSID, meterDSID);
    int ret = DeviceProperties_set_name(m_DSMApiHandle, meterDSID, _deviceID, name);
    DSBusInterface::checkResultCode(ret);
  } // deviceSetName

  void DSStructureModifyingBusInterface::meterSetName(dss_dsid_t _meterDSID, const std::string& _name) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::string nameStr = truncateUTF8String(_name, 19);
    uint8_t name[20];
    strncpy(reinterpret_cast<char*>(name), nameStr.c_str(), 20);
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_meterDSID, meterDSID);
    int ret = dSMProperties_set_name(m_DSMApiHandle, meterDSID, name);
    DSBusInterface::checkResultCode(ret);
  } // meterSetName

  void DSStructureModifyingBusInterface::createGroup(uint16_t _zoneID, uint8_t _groupID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = ZoneGroupModify_add(m_DSMApiHandle, m_BroadcastDSID, _zoneID, _groupID, 0);
    DSBusInterface::checkBroadcastResultCode(ret);
  } // createGroup

  void DSStructureModifyingBusInterface::removeGroup(uint16_t _zoneID, uint8_t _groupID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = ZoneGroupModify_remove(m_DSMApiHandle, m_BroadcastDSID, _zoneID, _groupID);
    DSBusInterface::checkBroadcastResultCode(ret);
  } // removeGroup

  void DSStructureModifyingBusInterface::sensorPush(uint16_t _zoneID, dss_dsid_t _sourceID, uint8_t _sensorType, uint16_t _sensorValue) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = ZoneSensorPush(m_DSMApiHandle, m_BroadcastDSID, _zoneID, _sourceID.lower, _sensorType, _sensorValue, 0);
    DSBusInterface::checkBroadcastResultCode(ret);
  } // sensorPush

  void DSStructureModifyingBusInterface::setButtonSetsLocalPriority(const dss_dsid_t& _dsMeterID, const devid_t _deviceID, bool _setsPriority) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    int ret = DeviceProperties_set_button_set_local_priority(m_DSMApiHandle, meterDSID, _deviceID, _setsPriority ? 1 : 0);
    DSBusInterface::checkBroadcastResultCode(ret);
  } // setButtonSetsLocalPriority

} // namespace dss
