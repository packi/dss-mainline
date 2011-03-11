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
#include "core/model/group.h"

#include "dsbusinterface.h"

namespace dss {

  //================================================== DSStructureQueryBusInterface

  std::vector<DSMeterSpec_t> DSStructureQueryBusInterface::getDSMeters() {
    std::vector<DSMeterSpec_t> result;
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      return result;
    }

    dsid_t device_list[63]; // TODO: constant for 63?
    int ret = DsmApiGetBusMembers(m_DSMApiHandle, device_list, 63);
    lock.unlock();
    if(ret < 0) {
      // DsmApiGetBusMembers returns >= 0 on success
      DSBusInterface::checkResultCode(ret);
    }

    for(int i = 0; i < ret; ++i) {
      // don't include ourself
      if(!DsmApiIsdSM(device_list[i])) {
        continue;
      }
      dss_dsid_t dsid;
      dsid_helper::toDssDsid(device_list[i], dsid);
      result.push_back(getDSMeterSpec(dsid));
    }
    return result;
  } // getDSMeters

  DSMeterSpec_t DSStructureQueryBusInterface::getDSMeterSpec(const dss_dsid_t& _dsMeterID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    DSMeterSpec_t result;
    result.DSID = _dsMeterID;
    dsid_t dsid;
    uint8_t nameBuf[NAME_LEN];
    dsid_helper::toDsmapiDsid(_dsMeterID, dsid);
    int ret = dSMInfo(m_DSMApiHandle, dsid, &result.HardwareVersion,
                      &result.SoftwareRevisionARM, &result.SoftwareRevisionDSP,
                      &result.APIVersion, NULL, nameBuf);
    DSBusInterface::checkResultCode(ret);

    char nameStr[NAME_LEN];
    memcpy(nameStr, nameBuf, NAME_LEN);
    result.Name = nameStr;
    return result;
  } // getDSMeterSpec

  int DSStructureQueryBusInterface::getGroupCount(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
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
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::vector<int> result;

    int numGroups = getGroupCount(_dsMeterID, _zoneID);
    Logger::getInstance()->log(std::string("DSMeter has ") + intToString(numGroups) + " groups");

    dsid_t dsid;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsid);
    uint8_t groupId;
    uint8_t groupTargetId;
    for(int iGroup = 0; iGroup < numGroups; iGroup++) {
      int ret = ZoneGroupInfo_by_index(m_DSMApiHandle, dsid, _zoneID, iGroup, &groupId, &groupTargetId, NULL, NULL);
      DSBusInterface::checkResultCode(ret);

      result.push_back(groupId);
    }
    return result;
  } // getGroups

  std::vector<int> DSStructureQueryBusInterface::getZones(const dss_dsid_t& _dsMeterID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
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

  // TODO: make this a private method of DSStructureQueryBusInterface
  //       as soon as dsm-api-const.h has a header guard in testing branch
  std::vector<int> extractGroupIDs(uint8_t _groups[GROUPS_LEN]) {
    std::vector<int> result;
    for(int iByte = 0; iByte < GROUPS_LEN; iByte++) {
      uint8_t byte = _groups[iByte];
      for(int iBit = 0; iBit < 8; iBit++) {
        if(byte & (1 << iBit)) {
          int groupID = (iByte * 8 + iBit) + 1;
          if(groupID <= GroupIDMax) {
            result.push_back(groupID);
          } else {
            Logger::getInstance()->log("Group ID out of bound (" +
                                       intToString(groupID) + ")", lsWarning);
          }
        }
      }
    }
    return result;
  } // extractGroupIDs

  int DSStructureQueryBusInterface::getDevicesCountInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
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

  std::vector<DeviceSpec_t> DSStructureQueryBusInterface::getDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::vector<DeviceSpec_t> result;

    int numDevices = getDevicesCountInZone(_dsMeterID, _zoneID);
    Logger::getInstance()->log(std::string("Found ") + intToString(numDevices) + " devices in zone.");
    dsid_t dsid;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsid);
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      DeviceSpec_t spec;
      uint8_t locked;
      uint8_t outputHasLoad;
      uint8_t groups[GROUPS_LEN];
      int ret = DeviceInfo_by_index_only_active(m_DSMApiHandle, dsid, _zoneID, iDevice,
                                                &spec.ShortAddress, &spec.VendorID, &spec.ProductID, &spec.FunctionID,
                                                &spec.Version,
                                                NULL, NULL, NULL, &locked, &outputHasLoad, groups, NULL,
                                                NULL, NULL, &spec.SerialNumber);
      DSBusInterface::checkResultCode(ret);
      spec.Locked = (locked != 0);
      spec.OutputHasLoad = (outputHasLoad != 0);
      spec.Groups = extractGroupIDs(groups);
      dsid_t dsid;
      ret = DsmApiExpandDeviceDSID(spec.VendorID, spec.SerialNumber, &dsid);
      DSBusInterface::checkResultCode(ret);
      dsid_helper::toDssDsid(dsid, spec.DSID);

      result.push_back(spec);
    }
    return result;
  } // getDevicesInZone

  DeviceSpec_t DSStructureQueryBusInterface::deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    DeviceSpec_t result;
    uint8_t locked;
    uint8_t outputHasLoad;
    dsid_t dsmDSID;
    uint8_t groups[GROUPS_LEN];
    dsid_helper::toDsmapiDsid(_dsMeterID, dsmDSID);
    int ret = DeviceInfo_by_device_id(m_DSMApiHandle, dsmDSID, _id,
                                      &result.ShortAddress, &result.VendorID, &result.ProductID, &result.FunctionID,
                                      &result.Version,
                                      NULL, NULL, NULL, &locked, &outputHasLoad, groups, NULL,
                                      NULL, NULL, &result.SerialNumber);
    DSBusInterface::checkResultCode(ret);
    result.Locked = (locked != 0);
    result.OutputHasLoad = (outputHasLoad != 0);
    result.Groups = extractGroupIDs(groups);
    dsid_t dsid;
    ret = DsmApiExpandDeviceDSID(result.VendorID, result.SerialNumber, &dsid);
    DSBusInterface::checkResultCode(ret);
    dsid_helper::toDssDsid(dsid, result.DSID);

    return result;
  } // deviceGetSpec

  int DSStructureQueryBusInterface::getLastCalledScene(const dss_dsid_t& _dsMeterID, const int _zoneID, const int _groupID) {
    // TODO: libdsm-api
    return SceneOff;
  } // getLastCalledScene

  bool DSStructureQueryBusInterface::getEnergyBorder(const dss_dsid_t& _dsMeterID, int& _lower, int& _upper) {
    // TODO: libdsm
    Logger::getInstance()->log("getEnergyBorder(): not implemented yet", lsInfo);
    return false;
  } // getEnergyBorder

  std::string DSStructureQueryBusInterface::getSceneName(dss_dsid_t _dsMeterID, boost::shared_ptr<Group> _group, const uint8_t _sceneNumber) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsmDSID);
    uint8_t name[20];
    int ret = ZoneGroupSceneInfo(m_DSMApiHandle, dsmDSID, _group->getZoneID(), _group->getID(), _sceneNumber, name);
    DSBusInterface::checkResultCode(ret);
    return std::string(reinterpret_cast<char*>(&name));
  }

} // namespace dss
