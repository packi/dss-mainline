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

#include "src/dsidhelper.h"

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

    if (IsBroadcastId(meterDSID)) {
      DSBusInterface::checkBroadcastResultCode(ret);
    } else {
      DSBusInterface::checkResultCode(ret);
    }
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

    int ret = CircuitRemoveDevice_by_id(m_DSMApiHandle, meterDSID, _deviceID);
    DSBusInterface::checkResultCode(ret);
  } // removeDeviceFromDSMeter

  void DSStructureModifyingBusInterface::removeDeviceFromDSMeters(const dss_dsid_t& _deviceDSID)
  {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t broadcastDSID;
    SetBroadcastId(broadcastDSID);
    int ret = CircuitRemoveDevice_by_dsid(m_DSMApiHandle, broadcastDSID, _deviceDSID.lower);
    DSBusInterface::checkBroadcastResultCode(ret);
  } // removeDeviceFromDSMeters

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

  void DSStructureModifyingBusInterface::createGroup(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID, const std::string& _name) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if (m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = ZoneGroupModify_add(m_DSMApiHandle, m_BroadcastDSID, _zoneID, _groupID, _standardGroupID);
    DSBusInterface::checkBroadcastResultCode(ret);
    uint8_t grName[NAME_LEN];
    strncpy((char *) grName, _name.c_str(), NAME_LEN);
    ret = ZoneGroupProperties_set_name(m_DSMApiHandle, m_BroadcastDSID, _zoneID, _groupID, grName);
    DSBusInterface::checkBroadcastResultCode(ret);
  } // createGroup

  void DSStructureModifyingBusInterface::groupSetStandardID(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    int ret;
    if (m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    ret = ZoneGroupProperties_set_state_machine(m_DSMApiHandle, m_BroadcastDSID, _zoneID, _groupID, _standardGroupID);
    DSBusInterface::checkBroadcastResultCode(ret);
  } // groupSetStandardID

  void DSStructureModifyingBusInterface::groupSetName(uint16_t _zoneID, uint8_t _groupID, const std::string& _name) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    int ret;
    if (m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    uint8_t grName[NAME_LEN];
    strncpy((char *) grName, _name.c_str(), NAME_LEN);
    ret = ZoneGroupProperties_set_name(m_DSMApiHandle, m_BroadcastDSID, _zoneID, _groupID, grName);
    DSBusInterface::checkBroadcastResultCode(ret);
  } // groupSetName

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
    DSBusInterface::checkResultCode(ret);
  } // setButtonSetsLocalPriority

  void DSStructureModifyingBusInterface::setButtonCallsPresent(const dss_dsid_t& _dsMeterID, const devid_t _deviceID, bool _callsPresent) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);
    int ret = DeviceProperties_set_button_set_no_coming_home_call(m_DSMApiHandle, meterDSID, _deviceID, _callsPresent ? 0 : 1);
    DSBusInterface::checkResultCode(ret);
  } // setButtonCallsPresent

} // namespace dss
