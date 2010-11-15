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
    if(m_DSMApiHandle == NULL) {
      return result;
    }
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
      dss_dsid_t dsid;
      dsid_helper::toDssDsid(device_list[i], dsid);
      DSMeterSpec_t spec = getDSMeterSpec(dsid);
      result.push_back(spec);
    }
    return result;
  } // getDSMeters

  DSMeterSpec_t DSStructureQueryBusInterface::getDSMeterSpec(const dss_dsid_t& _dsMeterID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    uint32_t hwVersion;
    uint32_t armSwVersion;
    uint32_t dspSwVersion;
    uint16_t apiVersion;
    uint8_t dsidBuf[DSID_LEN];
    uint8_t nameBuf[NAME_LEN];
    dsid_t dsid;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsid);
    int ret = dSMInfo(m_DSMApiHandle, dsid, &hwVersion, &armSwVersion, &dspSwVersion, &apiVersion, dsidBuf, nameBuf);
    DSBusInterface::checkResultCode(ret);

    char nameStr[NAME_LEN];
    memcpy(nameStr, nameBuf, NAME_LEN);
    DSMeterSpec_t spec(_dsMeterID, armSwVersion, dspSwVersion, hwVersion, apiVersion, nameStr);
    return spec;
  } // getDSMeterSpec

  int DSStructureQueryBusInterface::getGroupCount(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    uint16_t zoneId;
    uint8_t virtualZoneId, numberOfGroups;
    uint8_t name[NAME_LEN];

    dsid_t dsid;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsid);
    int ret = ZoneInfo_by_id(m_DSMApiHandle, dsid, _zoneID, &zoneId, &virtualZoneId, &numberOfGroups, name);
    DSBusInterface::checkResultCode(ret);

    // TODO: libdsm:
    // assert 0 <= numberOfGroups < GroupIDStandardMax

    return numberOfGroups;
  } // getGroupCount

  std::vector<int> DSStructureQueryBusInterface::getGroups(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::vector<int> result;

    int numGroups = getGroupCount(_dsMeterID, _zoneID);
    Logger::getInstance()->log(std::string("DSMeter has ") + intToString(numGroups) + " groups");

    dsid_t dsid;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsid);
    uint8_t groupId;
    for(int iGroup = 0; iGroup < numGroups; iGroup++) {
      int ret = ZoneGroupInfo_by_index(m_DSMApiHandle, dsid, _zoneID, iGroup, &groupId, NULL, NULL);
      DSBusInterface::checkResultCode(ret);

      result.push_back(groupId);
    }
    return result;
  } // getGroups

  std::vector<int> DSStructureQueryBusInterface::getGroupsOfDevice(const dss_dsid_t& _dsMeterID, const int _deviceID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    uint8_t groups[GROUPS_LEN];
    dsid_t dsid;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsid);
    int ret = DeviceInfo_by_device_id(m_DSMApiHandle, dsid, _deviceID, NULL, NULL, NULL, NULL,
                                      NULL, NULL, NULL, NULL, groups, NULL, NULL, NULL);
    DSBusInterface::checkResultCode(ret);
    std::vector<int> result;
    for(int iByte = 0; iByte < GROUPS_LEN; iByte++) {
      uint8_t byte = groups[iByte];
      for(int iBit = 0; iBit < 8; iBit++) {
        if(byte & (1 << iBit)) {
          int groupID = (iByte * 8 + iBit) + 1;
          if(groupID <= GroupIDMax) {
            result.push_back(groupID);
          } else {
            Logger::getInstance()->log("Group ID out of bound (" + intToString(groupID) +
                                       ") on dSMeter: " + _dsMeterID.toString() +
                                       " deviceID " +intToString(_deviceID), lsWarning);
          }
        }
      }
    }
    return result;
  } // getGroupsOfDevice

  std::vector<int> DSStructureQueryBusInterface::getZones(const dss_dsid_t& _dsMeterID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::vector<int> result;

    uint8_t numZones;
    dsid_t dsid;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsid);

    int ret = ZoneCount(m_DSMApiHandle, dsid, &numZones);
    DSBusInterface::checkResultCode(ret);
    Logger::getInstance()->log(std::string("DSMeter has ") + intToString(numZones) + " zones");

    uint16_t zoneId;
    for(int iZone = 0; iZone < numZones; iZone++) {
      ret = ZoneInfo_by_index(m_DSMApiHandle, dsid, iZone,
                                       &zoneId, NULL, NULL, NULL);
      DSBusInterface::checkResultCode(ret);

      result.push_back(zoneId);
    }
    return result;
  } // getZones

  int DSStructureQueryBusInterface::getDevicesCountInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    uint16_t numberOfDevices;
    dsid_t dsid;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsid);
    int ret = ZoneDeviceCount_only_active(m_DSMApiHandle, dsid, _zoneID, &numberOfDevices);
    DSBusInterface::checkResultCode(ret);

    return numberOfDevices;
  } // getDevicesCountInZone

  std::vector<int> DSStructureQueryBusInterface::getDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::vector<int> result;

    int numDevices = getDevicesCountInZone(_dsMeterID, _zoneID);
    Logger::getInstance()->log(std::string("Found ") + intToString(numDevices) + " devices in zone.");
    dsid_t dsid;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsid);
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      uint16_t deviceId;
      int ret = DeviceInfo_by_index_only_active(m_DSMApiHandle, dsid, _zoneID, iDevice, &deviceId,
                                    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
      DSBusInterface::checkResultCode(ret);
      result.push_back(deviceId);
    }
    return result;
  } // getDevicesInZone

  dss_dsid_t DSStructureQueryBusInterface::getDSIDOfDevice(const dss_dsid_t& _dsMeterID, const int _deviceID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t dsid;
    dsid_t meterDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, meterDSID);

    int ret = DeviceInfo_by_device_id(m_DSMApiHandle, meterDSID, _deviceID, NULL, NULL, NULL, NULL,
                                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, dsid.id);
    DSBusInterface::checkResultCode(ret);

    dss_dsid_t dss_dsid;
    dsid_helper::toDssDsid(dsid, dss_dsid);
    return dss_dsid;
  } // getDSIDOfDevice

  int DSStructureQueryBusInterface::getLastCalledScene(const dss_dsid_t& _dsMeterID, const int _zoneID, const int _groupID) {
    // TODO: libdsm-api
    return SceneOff;
  } // getLastCalledScene

  bool DSStructureQueryBusInterface::getEnergyBorder(const dss_dsid_t& _dsMeterID, int& _lower, int& _upper) {
    // TODO: libdsm
    Logger::getInstance()->log("getEnergyBorder(): not implemented yet", lsInfo);
    return false;
  } // getEnergyBorder

  DeviceSpec_t DSStructureQueryBusInterface::deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID) {
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
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
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
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
