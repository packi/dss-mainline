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

#include "src/logger.h"
#include "src/dsidhelper.h"

#include "src/model/modelconst.h"
#include "src/model/device.h"
#include "src/model/group.h"

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

  std::vector<GroupSpec_t> DSStructureQueryBusInterface::getGroups(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::vector<GroupSpec_t> gresult;
    dsid_t dsid;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsid);

    int numGroups = getGroupCount(_dsMeterID, _zoneID);
    Logger::getInstance()->log(std::string("DSMeter has ") + intToString(numGroups) + " groups");

    for (int iGroup = 0; iGroup < numGroups; iGroup++) {
      GroupSpec_t result;
      uint8_t nameBuf[NAME_LEN];

      int ret = ZoneGroupInfo_by_index(m_DSMApiHandle, dsid, _zoneID, iGroup,
          &result.GroupID, &result.StandardGroupID, &result.NumberOfDevices, nameBuf
#if DSM_API_VERSION >= 0x106
          , NULL, NULL
#endif
          );
      DSBusInterface::checkResultCode(ret);

      char nameStr[NAME_LEN];
      memcpy(nameStr, nameBuf, NAME_LEN);
      result.Name = nameStr;

      gresult.push_back(result);
    }
    return gresult;
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
          int groupID = (iByte * 8 + iBit);
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
    int ret = ZoneDeviceCount_all(m_DSMApiHandle, dsid, _zoneID, &numberOfDevices);
    DSBusInterface::checkResultCode(ret);

    return numberOfDevices;
  } // getDevicesCountInZone

  void DSStructureQueryBusInterface::updateButtonGroupFromMeter(dsid_t _dsMeterID, DeviceSpec_t& _spec) {
    int ret = -1;
    try {
      union {
        uint8_t flags;
        struct {
          uint8_t setLocalPriority : 1;
          uint8_t callsNoPresent : 1;
          uint8_t reserved : 6;
        };
      } flags;

      ret = DeviceButtonInfo_by_device(m_DSMApiHandle, _dsMeterID, _spec.ShortAddress, &_spec.ButtonID,
                                       &_spec.GroupMembership, &_spec.ActiveGroup,
                                       &flags.flags);
      DSBusInterface::checkResultCode(ret);
      _spec.SetsLocalPriority = (flags.setLocalPriority == 1);
      _spec.CallsPresent = (flags.callsNoPresent == 0);
    } catch(BusApiError& e) {
      _spec.ButtonID = 0xff;
      _spec.ActiveGroup = 0xff;
      _spec.GroupMembership = 0xff;
      _spec.SetsLocalPriority = false;
      _spec.CallsPresent = true;
      if (ret == ERROR_WRONG_MSGID || ret == ERROR_WRONG_MODIFIER) {
        Logger::getInstance()->log("Unsupported message-id DeviceButtonInfo", lsWarning);
      } else if (ret == ERROR_WRONG_PARAMETER) {
        Logger::getInstance()->log("DeviceButtonInfo: Device: " +
                                   _spec.DSID.toString() +
                                   " has no buttons", lsInfo);
      } else {
        Logger::getInstance()->log("Error reading DeviceButtonInfo: " +
                                   std::string(e.what()), lsWarning);
      }
    }
  } // updateButtonGroupFromMeter

  void DSStructureQueryBusInterface::updateBinaryInputTableFromMeter(dsid_t _dsMeterID, DeviceSpec_t& _spec) {
    if ((((_spec.FunctionID >> 6) & 0x3F) == 0x04) && ((_spec.FunctionID & 0x0008) > 0)) {
      try {
        uint8_t numBinaryinputs = 0;
        int ret = DeviceBinaryInput_get_count(m_DSMApiHandle, _dsMeterID, _spec.ShortAddress, &numBinaryinputs);
        if(ret == ERROR_WRONG_MSGID || ret == ERROR_WRONG_MODIFIER) {
          Logger::getInstance()->log("Unsupported message-id DeviceBinaryInput_get_count", lsWarning);
          return;
        } else {
          DSBusInterface::checkResultCode(ret);
        }
        _spec.binaryInputs.clear();
        for (int i = 0; i < numBinaryinputs; i++) {
          uint8_t TargetGroupType;
          uint8_t TargetGroup;
          uint8_t InputType;
          uint8_t InputID;
          ret = DeviceBinaryInput_get_by_index(m_DSMApiHandle, _dsMeterID, _spec.ShortAddress, i,
                                               &TargetGroupType, &TargetGroup,
                                               &InputType, &InputID);
          DSBusInterface::checkResultCode(ret);
          DeviceBinaryInputSpec_t binaryInput;
          binaryInput.TargetGroupType = TargetGroupType;
          binaryInput.TargetGroup = TargetGroup;
          binaryInput.InputType = InputType;
          binaryInput.InputID = InputID;
          _spec.binaryInputs.push_back(binaryInput);
        }
      } catch(BusApiError& e) {
        Logger::getInstance()->log("Error reading DeviceBinaryInput: " +
        std::string(e.what()), lsWarning);
      }
    }
  } // updateBinaryInputTableFromMeter

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
      uint8_t groups[GROUPS_LEN];
      uint8_t name[NAME_LEN];
      int ret = DeviceInfo_by_index(m_DSMApiHandle, dsid, _zoneID, iDevice,
          &spec.ShortAddress, &spec.VendorID, &spec.ProductID, &spec.FunctionID,
          &spec.Version, &spec.ZoneID, &spec.ActiveState, &locked, &spec.OutputMode,
          &spec.LTMode, groups, name, &spec.SerialNumber);
      DSBusInterface::checkResultCode(ret);
      spec.Locked = (locked != 0);
      spec.Groups = extractGroupIDs(groups);
      spec.Name = std::string(reinterpret_cast<char*>(name));

      dsid_t devdsid;
      try {
        ret = DsmApiGetDeviceDSID(spec.VendorID, spec.ProductID, spec.FunctionID >> 12, spec.Version, spec.SerialNumber, &devdsid);
        DSBusInterface::checkResultCode(ret);
      } catch(BusApiError& e) {
        Logger::getInstance()->log("Error converting DSID for device with serialNumber " +
            intToString(spec.SerialNumber, true), lsWarning);
      }
      dsid_helper::toDssDsid(devdsid, spec.DSID);

      updateButtonGroupFromMeter(dsid, spec);
      updateBinaryInputTableFromMeter(dsid, spec);

      result.push_back(spec);
    }
    return result;
  } // getDevicesInZone

  std::vector<DeviceSpec_t> DSStructureQueryBusInterface::getInactiveDevicesInZone(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::vector<DeviceSpec_t> result;
    uint16_t numberOfDevices;
    dsid_t dsid;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsid);
    int ret = ZoneDeviceCount_only_inactive(m_DSMApiHandle, dsid, _zoneID, &numberOfDevices);
    DSBusInterface::checkResultCode(ret);

    for(int iDevice = 0; iDevice < numberOfDevices; iDevice++) {
      DeviceSpec_t spec;
      uint8_t locked;
      uint8_t groups[GROUPS_LEN];
      uint8_t name[NAME_LEN];
      int ret = DeviceInfo_by_index_only_inactive(m_DSMApiHandle, dsid, _zoneID, iDevice,
          &spec.ShortAddress, &spec.VendorID, &spec.ProductID, &spec.FunctionID,
          &spec.Version, &spec.ZoneID, &spec.ActiveState, &locked, &spec.OutputMode,
          &spec.LTMode, groups, name, &spec.SerialNumber);
      DSBusInterface::checkResultCode(ret);
      spec.Locked = (locked != 0);
      spec.Groups = extractGroupIDs(groups);
      spec.Name = std::string(reinterpret_cast<char*>(name));
      dsid_t devdsid;
      try {
        ret = DsmApiGetDeviceDSID(spec.VendorID, spec.ProductID, spec.FunctionID >> 12, spec.Version, spec.SerialNumber, &devdsid);
        DSBusInterface::checkResultCode(ret);
      } catch(BusApiError& e) {
        Logger::getInstance()->log("Error converting DSID for device with serialNumber " +
            intToString(spec.SerialNumber, true), lsWarning);
      }
      dsid_helper::toDssDsid(devdsid, spec.DSID);

      updateButtonGroupFromMeter(dsid, spec);
      updateBinaryInputTableFromMeter(dsid, spec);

      result.push_back(spec);
    }
    return result;
  } // getInactiveDevicesInZone

  DeviceSpec_t DSStructureQueryBusInterface::deviceGetSpec(devid_t _id, dss_dsid_t _dsMeterID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    DeviceSpec_t result;
    uint8_t locked;
    dsid_t dsmDSID;
    uint8_t groups[GROUPS_LEN];
    uint8_t name[NAME_LEN];
    dsid_helper::toDsmapiDsid(_dsMeterID, dsmDSID);
    int ret = DeviceInfo_by_device_id(m_DSMApiHandle, dsmDSID, _id,
        &result.ShortAddress, &result.VendorID, &result.ProductID, &result.FunctionID,
        &result.Version, &result.ZoneID, &result.ActiveState, &locked, &result.OutputMode,
        &result.LTMode, groups, name, &result.SerialNumber);
    DSBusInterface::checkResultCode(ret);
    result.Locked = (locked != 0);
    result.Groups = extractGroupIDs(groups);
    result.Name = std::string(reinterpret_cast<char*>(name));

    dsid_t devdsid;
    try {
      ret = DsmApiGetDeviceDSID(result.VendorID, result.ProductID, result.FunctionID >> 12, result.Version, result.SerialNumber, &devdsid);
      DSBusInterface::checkResultCode(ret);
    } catch(BusApiError& e) {
      Logger::getInstance()->log("Error converting DSID for device with serialNumber " +
          intToString(result.SerialNumber, true), lsWarning);
    }
    dsid_helper::toDssDsid(devdsid, result.DSID);

    updateButtonGroupFromMeter(dsmDSID, result);
    updateBinaryInputTableFromMeter(dsmDSID, result);

    return result;
  } // deviceGetSpec

  std::vector<std::pair<int, int> > DSStructureQueryBusInterface::getLastCalledScenes(const dss_dsid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }

    dsid_t dsmDSID;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsmDSID);

    uint8_t groups[7];
    uint8_t scenes[7];
    uint8_t hsize;
    int ret = ZoneProperties_get_scene_history(m_DSMApiHandle, dsmDSID, (uint16_t) _zoneID, &hsize,
        &groups[0], &scenes[0],
        &groups[1], &scenes[1],
        &groups[2], &scenes[2],
        &groups[3], &scenes[3],
        &groups[4], &scenes[4],
        &groups[5], &scenes[5],
        &groups[6], &scenes[6]
        );
    DSBusInterface::checkResultCode(ret);

    std::vector<std::pair<int,int> > result;
    for (int i = hsize - 1; i >= 0; i--) {
      result.push_back(std::make_pair(groups[i], scenes[i]));
    }
    return result;
  } // getLastCalledScenes

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

  DSMeterHash_t DSStructureQueryBusInterface::getDSMeterHash(const dss_dsid_t& _dsMeterID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    DSMeterHash_t result;
    dsid_t dsid;
    dsid_helper::toDsmapiDsid(_dsMeterID, dsid);
    int ret = dSMConfig_get_hash(m_DSMApiHandle, dsid, &result.Hash, &result.ModificationCount, &result.EventCount);
    DSBusInterface::checkResultCode(ret);

    return result;
  } // getDSMeterHash

} // namespace dss
