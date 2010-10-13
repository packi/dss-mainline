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

#include "dsstructurequerybusinterface.h"

#include <digitalSTROM/dsm-api-v2/dsm-api-const.h>

#include "core/logger.h"
#include "core/dsidhelper.h"

#include "core/model/modelconst.h"
#include "core/model/device.h"

#include "dsbusinterface.h"

namespace dss {

  //================================================== DSStructureQueryBusInterface

  std::vector<DSMeterSpec_t> DSStructureQueryBusInterface::getDSMeters() {
    std::vector<DSMeterSpec_t> result;
    dsid_t ownDSID;

    // TODO: we could cache our own DSID
    int ret = DsmApiGetOwnDSID(m_DSMApiHandle, &ownDSID);
    DSBusInterface::checkResultCode(ret);

    dsid_t device_list[63]; // TODO: constant for 63?
    ret = DsmApiGetBusMembers(m_DSMApiHandle, device_list, 63);
    if(ret < 0) {
      // DsmApiGetBusMembers returns >= 0 on success
      DSBusInterface::checkResultCode(ret);
    }

    for(int i = 0; i < ret; ++i) {
      // don't include ourself
      if(IsEqualId(device_list[i], ownDSID)) {
        continue;
      }
      DSMeterSpec_t spec = getDSMeterSpec(device_list[i]);
      result.push_back(spec);
    }
    return result;
  } // getDSMeters

  DSMeterSpec_t DSStructureQueryBusInterface::getDSMeterSpec(const dsid_t& _dsMeterID) {
    uint32_t hwVersion;
    uint32_t swVersion;
    uint16_t apiVersion;
    uint8_t dsidBuf[DSID_LEN];
    uint8_t nameBuf[NAME_LEN];
    int ret = dSMInfo(m_DSMApiHandle, _dsMeterID, &hwVersion, &swVersion, &apiVersion, dsidBuf, nameBuf);
    DSBusInterface::checkResultCode(ret);

    char nameStr[NAME_LEN];
    memcpy(nameStr, nameBuf, NAME_LEN);
    DSMeterSpec_t spec(_dsMeterID, swVersion, hwVersion, apiVersion, nameStr);
    return spec;
  } // getDSMeterSpec

  int DSStructureQueryBusInterface::getGroupCount(const dsid_t& _dsMeterID, const int _zoneID) {
    uint16_t zoneId;
    uint8_t virtualZoneId, numberOfGroups;
    uint8_t name[NAME_LEN];

    int ret = ZoneInfo_by_id(m_DSMApiHandle, _dsMeterID, _zoneID, &zoneId, &virtualZoneId, &numberOfGroups, name);
    DSBusInterface::checkResultCode(ret);

    // TODO: libdsm:
    // assert 0 <= numberOfGroups < GroupIDStandardMax

    return numberOfGroups;
  } // getGroupCount

  std::vector<int> DSStructureQueryBusInterface::getGroups(const dsid_t& _dsMeterID, const int _zoneID) {

    std::vector<int> result;

    int numGroups = getGroupCount(_dsMeterID, _zoneID);
    Logger::getInstance()->log(std::string("DSMeter has ") + intToString(numGroups) + " groups");

    uint8_t groupId;
    for(int iGroup = 0; iGroup < numGroups; iGroup++) {
      int ret = ZoneGroupInfo_by_index(m_DSMApiHandle, _dsMeterID, _zoneID, iGroup, &groupId, NULL, NULL);
      DSBusInterface::checkResultCode(ret);

      result.push_back(groupId);
    }
    return result;
  } // getGroups

  std::vector<int> DSStructureQueryBusInterface::getGroupsOfDevice(const dsid_t& _dsMeterID, const int _deviceID) {
    uint8_t groups[GROUPS_LEN];
    int ret = DeviceInfo_by_device_id(m_DSMApiHandle, _dsMeterID, _deviceID, NULL, NULL, NULL, NULL,
                                      NULL, NULL, NULL, NULL, groups, NULL, NULL, NULL);
    DSBusInterface::checkResultCode(ret);
    std::vector<int> result;
    for(int iByte = 0; iByte < GROUPS_LEN; iByte++) {
      uint8_t byte = groups[iByte];
      for(int iBit = 0; iBit < 8; iBit++) {
        if(byte & (1 << iBit)) {
          result.push_back((iByte * 8 + iBit) + 1);
        }
      }
    }
    return result;
  } // getGroupsOfDevice

  int DSStructureQueryBusInterface::getZoneCount(const dsid_t& _dsMeterID) {
    uint8_t zoneCount;
    int ret = ZoneCount(m_DSMApiHandle, _dsMeterID, &zoneCount);
    DSBusInterface::checkResultCode(ret);
    return zoneCount;
  } // getZoneCount

  std::vector<int> DSStructureQueryBusInterface::getZones(const dsid_t& _dsMeterID) {
    std::vector<int> result;

    int numZones = getZoneCount(_dsMeterID);
    Logger::getInstance()->log(std::string("DSMeter has ") + intToString(numZones) + " zones");

    uint16_t zoneId;
    for(int iZone = 0; iZone < numZones; iZone++) {
      int ret = ZoneInfo_by_index(m_DSMApiHandle, _dsMeterID, iZone,
                                       &zoneId, NULL, NULL, NULL);
      DSBusInterface::checkResultCode(ret);

      result.push_back(zoneId);
    }
    return result;
  } // getZones

  int DSStructureQueryBusInterface::getDevicesCountInZone(const dsid_t& _dsMeterID, const int _zoneID) {
    uint16_t numberOfDevices;
    int ret = ZoneDeviceCount_all(m_DSMApiHandle, _dsMeterID, _zoneID, &numberOfDevices);
    DSBusInterface::checkResultCode(ret);

    return numberOfDevices;
  } // getDevicesCountInZone

  std::vector<int> DSStructureQueryBusInterface::getDevicesInZone(const dsid_t& _dsMeterID, const int _zoneID) {
    std::vector<int> result;

    int numDevices = getDevicesCountInZone(_dsMeterID, _zoneID);
    Logger::getInstance()->log(std::string("Found ") + intToString(numDevices) + " devices in zone.");
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      uint16_t deviceId;
      int ret = DeviceInfo_by_index(m_DSMApiHandle, _dsMeterID, _zoneID, iDevice, &deviceId,
                                    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      DSBusInterface::checkResultCode(ret);
      result.push_back(deviceId);
    }
    return result;
  } // getDevicesInZone

  dss_dsid_t DSStructureQueryBusInterface::getDSIDOfDevice(const dsid_t& _dsMeterID, const int _deviceID) {
    dsid_t dsid;
    int ret = DeviceInfo_by_device_id(m_DSMApiHandle, _dsMeterID, _deviceID, NULL, NULL, NULL, NULL,
                                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, dsid.id);
    DSBusInterface::checkResultCode(ret);

    dss_dsid_t dss_dsid;
    dsid_helper::toDssDsid(dsid, dss_dsid);
    return dss_dsid;
  } // getDSIDOfDevice

  int DSStructureQueryBusInterface::getLastCalledScene(const int _dsMeterID, const int _zoneID, const int _groupID) {
    // TODO: libdsm-api
    return SceneOff;
  } // getLastCalledScene

  bool DSStructureQueryBusInterface::getEnergyBorder(const int _dsMeterID, int& _lower, int& _upper) {
    // TODO: libdsm
    Logger::getInstance()->log("getEnergyBorder(): not implemented yet", lsInfo);
    return false;
  } // getEnergyBorder


  DeviceSpec_t DSStructureQueryBusInterface::deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID) {

    uint16_t functionId, productId, version;
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsmDSID);
    int ret = DeviceInfo_by_device_id(m_DSMApiHandle, dsmDSID, _id,
                                      NULL, &functionId, &productId, &version,
                                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    DSBusInterface::checkResultCode(ret);

    DeviceSpec_t spec(functionId, productId, version, _id);
    return spec;
  } // deviceGetSpec

  bool DSStructureQueryBusInterface::isLocked(boost::shared_ptr<const Device> _device) {

    uint8_t locked;
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_device->getDSMeterDSID(), dsmDSID);
    int ret = DeviceInfo_by_device_id(m_DSMApiHandle, dsmDSID, _device->getShortAddress(),
                                      NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                      &locked, NULL, NULL, NULL, NULL);
    DSBusInterface::checkResultCode(ret);
    return locked;
  } // isLocked

} // namespace dss
