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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "dsstructurequerybusinterface.h"

#include <digitalSTROM/dsm-api-v2/dsm-api-const.h>
#include <digitalSTROM/ds485-client-interface.h>

#include "src/logger.h"

#include "src/model/modelconst.h"
#include "src/model/device.h"
#include "src/model/group.h"
#include "src/model/modulator.h"
#include "src/model/zone.h"
#include "src/util.h"

#include "dsbusinterface.h"

namespace dss {

  //================================================== DSStructureQueryBusInterface

  std::vector<DSMeterSpec_t> DSStructureQueryBusInterface::getBusMembers() {
    std::vector<DSMeterSpec_t> result;

    static const int MAX_DEVICE = 63;
    dsuid_t device_list[MAX_DEVICE];
    int deviceCount = 0;

    { // scoped lock.
      boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
      if(m_DSMApiHandle == NULL) {
        return result;
      }
      deviceCount = DsmApiGetBusMembers(m_DSMApiHandle, device_list, MAX_DEVICE);
      if (deviceCount < 0) {
        // DsmApiGetBusMembers:
        //    result >= 0 number of devices.
        //    result < 0 -> error code.
        DSBusInterface::checkResultCode(deviceCount);
      }
    } // end scoped lock.

    for (int i = 0; i < deviceCount; ++i) {
      try {
        result.push_back(getDSMeterSpec(device_list[i]));
      } catch (BusApiError& err) {
        if (err.error == ERROR_RESPONSE_TIMEOUT) {
          Logger::getInstance()->log("DSStructureQueryBusInterface::"
                                     "getDSMeters: ignore response timeout of " +
                                     dsuid2str(device_list[i]), lsDebug);
          continue;
        }
        throw; // rethrow exception
      }
    }
    return result;
  } // getDSMeters

  DSMeterSpec_t DSStructureQueryBusInterface::getDSMeterSpec(const dsuid_t& _dsMeterID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if (m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }

    DSMeterSpec_t result;
    uint8_t nameBuf[NAME_LEN];
    uint8_t flags;
    uint8_t devType;
    int ret;

    result.APIVersion = 0;
    result.HardwareVersion = 0;
    result.SoftwareRevisionARM = 0;
    result.SoftwareRevisionDSP = 0;
    result.ApartmentState = 0;
    result.DeviceType = BusMember_Unknown;
    result.DSID = _dsMeterID;

    try {
      ret = BusMember_get_type(m_DSMApiHandle, _dsMeterID, &devType);
      DSBusInterface::checkResultCode(ret);
    } catch (BusApiError& err) {
      Logger::getInstance()->log("DSStructureQueryBusInterface::"
                                 "getDSMeterSpec: BusMember_get_type: Bus api error: " +
                                 std::string(err.what()), lsWarning);
      return result; // do not rethrow exception
    }
    result.DeviceType = static_cast<BusMemberDevice_t>(devType);

    if (!busMemberIsDSMeter(result.DeviceType)) {
      Logger::getInstance()->log("DSStructureQueryBusInterface::"
                                 "getDSMeterSpec: ignore bus member " + dsuid2str(_dsMeterID) +
                                 " with bus member type " + intToString(devType), lsInfo);
      return result;
    }

    try {
      ret = dSMInfo(m_DSMApiHandle, _dsMeterID, &result.HardwareVersion,
                    &result.SoftwareRevisionARM, &result.SoftwareRevisionDSP,
                    &result.APIVersion, NULL, nameBuf);
      DSBusInterface::checkResultCode(ret);

    } catch (BusApiError& err) {
      Logger::getInstance()->log("DSStructureQueryBusInterface::"
                                 "getDSMeterSpec: dSMInfo: Bus api error: " +
                                 std::string(err.what()), lsWarning);
      throw; // rethrow exception
    }

    char nameStr[NAME_LEN];
    memcpy(nameStr, nameBuf, NAME_LEN);
    result.Name = nameStr;

    try {
      ret = dSMProperties_get_flags(m_DSMApiHandle, _dsMeterID, &flags);
      DSBusInterface::checkResultCode(ret);
      result.flags = std::bitset<8>(flags);
    } catch (BusApiError& err) {
      Logger::getInstance()->log("DSStructureQueryBusInterface::"
                                 "getDSMeterSpec: dSMProperties_get_flags: Bus api error: " +
                                 std::string(err.what()), lsWarning);
      throw; // rethrow exception
    }

    try {
      ret = dSMProperties_get_apartment_state(m_DSMApiHandle, _dsMeterID,
                                              &result.ApartmentState);
      DSBusInterface::checkResultCode(ret);
    } catch (BusApiError& err) {
      Logger::getInstance()->log("DSStructureQueryBusInterface::"
                                 "getDSMeterSpec: dSMProperties_get_apartment_state: Bus api error: " +
                                 std::string(err.what()), lsWarning);
      throw; // rethrow exception
    }
    return result;
  } // getDSMeterSpec

  int DSStructureQueryBusInterface::getGroupCount(const dsuid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    uint16_t zoneId;
    uint8_t virtualZoneId, numberOfGroups;
    uint8_t name[NAME_LEN];

    int ret = ZoneInfo_by_id(m_DSMApiHandle, _dsMeterID, _zoneID, &zoneId, &virtualZoneId, &numberOfGroups, name);
    DSBusInterface::checkResultCode(ret);

    // TODO: libdsm:
    // assert 0 <= numberOfGroups < GroupIDStandardMax

    return numberOfGroups;
  } // getGroupCount

  std::vector<GroupSpec_t> DSStructureQueryBusInterface::getGroups(const dsuid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::vector<GroupSpec_t> gresult;

    int numGroups = getGroupCount(_dsMeterID, _zoneID);
    Logger::getInstance()->log(std::string("DSMeter has ") + intToString(numGroups) + " groups");

    for (int iGroup = 0; iGroup < numGroups; iGroup++) {
      GroupSpec_t result;
      uint8_t nameBuf[NAME_LEN];

      int ret = ZoneGroupInfo_by_index(m_DSMApiHandle, _dsMeterID, _zoneID, iGroup,
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

  std::vector<ClusterSpec_t> DSStructureQueryBusInterface::getClusters(const dsuid_t& _dsMeterID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if (m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::vector<ClusterSpec_t> gresult;

    for (int iCluster = GroupIDAppUserMin; iCluster <= GroupIDAppUserMax; iCluster++) {
      ClusterSpec_t result;
      uint8_t nameBuf[NAME_LEN];
      uint8_t sceneLock[16];
      uint8_t configurationLock = 0;
      uint8_t canHaveStateMachine = 0;

      int ret = ClusterInfo_by_id(m_DSMApiHandle, _dsMeterID, iCluster,
          &result.StandardGroupID, &canHaveStateMachine, &result.NumberOfDevices, nameBuf,
          NULL, NULL, &configurationLock, sceneLock, &result.location, &result.floor, &result.protectionClass);
      DSBusInterface::checkResultCode(ret);

      result.GroupID = iCluster;
      char nameStr[NAME_LEN];
      memcpy(nameStr, nameBuf, NAME_LEN);
      result.Name = nameStr;
      result.canHaveStateMachine = (canHaveStateMachine == 1);
      result.configurationLocked = (configurationLock == 1);
      result.lockedScenes = parseBitfield(sceneLock, sizeof(sceneLock) * 8);

      gresult.push_back(result);
    }
    return gresult;
  } // getGroups

  std::vector<int> DSStructureQueryBusInterface::getZones(const dsuid_t& _dsMeterID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::vector<int> result;

    uint8_t numZones;

    int ret = ZoneCount(m_DSMApiHandle, _dsMeterID, &numZones);
    DSBusInterface::checkResultCode(ret);
    Logger::getInstance()->log(std::string("DSMeter has ") + intToString(numZones) + " zones");

    uint16_t zoneId;
    for(int iZone = 0; iZone < numZones; iZone++) {
      ret = ZoneInfo_by_index(m_DSMApiHandle, _dsMeterID, iZone,
                              &zoneId, NULL, NULL, NULL);
      DSBusInterface::checkResultCode(ret);

      result.push_back(zoneId);
    }
    return result;
  } // getZones

  int DSStructureQueryBusInterface::getDevicesCountInZone(const dsuid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    uint16_t numberOfDevices;
    int ret = ZoneDeviceCount_all(m_DSMApiHandle, _dsMeterID, _zoneID, &numberOfDevices);
    DSBusInterface::checkResultCode(ret);

    return numberOfDevices;
  } // getDevicesCountInZone

  void DSStructureQueryBusInterface::updateButtonGroupFromMeter(dsuid_t _dsMeterID, DeviceSpec_t& _spec) {
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
                                       &flags.flags, NULL, NULL);
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
                                   dsuid2str(_spec.DSID) +
                                   " has no buttons", lsInfo);
      } else {
        Logger::getInstance()->log("Error reading DeviceButtonInfo: " +
                                   std::string(e.what()), lsWarning);
      }
    }
  } // updateButtonGroupFromMeter

  void DSStructureQueryBusInterface::updateBinaryInputTableFromMeter(dsuid_t _dsMeterID, DeviceSpec_t& _spec) {
    _spec.binaryInputsValid = false;
    if ((((_spec.FunctionID >> 6) & 0x3F) == 0x04) && ((_spec.FunctionID & 0x0008) > 0)) {
      try {
        _spec.binaryInputs.clear();

        uint8_t numBinaryinputs = 0;
        int ret = DeviceBinaryInput_get_count(m_DSMApiHandle, _dsMeterID, _spec.ShortAddress, &numBinaryinputs);
        if(ret == ERROR_WRONG_MSGID || ret == ERROR_WRONG_MODIFIER) {
          Logger::getInstance()->log("Unsupported message-id DeviceBinaryInput_get_count", lsWarning);
          return;
        } else {
          DSBusInterface::checkResultCode(ret);
        }

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
        _spec.binaryInputsValid = true;
      } catch(BusApiError& e) {
        Logger::getInstance()->log("Error reading DeviceBinaryInput: " +
        std::string(e.what()), lsWarning);
      }
    }
  } // updateBinaryInputTableFromMeter

  void DSStructureQueryBusInterface::updateSensorInputTableFromMeter(dsuid_t _dsMeterID, DeviceSpec_t& _spec) {
    _spec.sensorInputsValid = false;
    if (true || ((((_spec.FunctionID >> 6) & 0x3F) == 0x04) && ((_spec.FunctionID & 0x0008) > 0))) {
      try {
        _spec.sensorInputs.clear();

        uint8_t numSensors = 0;
        int ret = DeviceSensor_get_count(m_DSMApiHandle, _dsMeterID, _spec.ShortAddress, &numSensors);
        if(ret == ERROR_WRONG_MSGID || ret == ERROR_WRONG_MODIFIER) {
          Logger::getInstance()->log("Unsupported message-id DeviceSensor_get_count", lsWarning);
          return;
        } else {
          DSBusInterface::checkResultCode(ret);
        }

        for (int i = 0; i < numSensors; i++) {
          uint8_t SensorType;
          uint32_t SensorPollInterval;
          uint8_t SensorBroadcastFlag;
          uint8_t SensorConversionFlag;
          ret = DeviceSensor_get_by_index(m_DSMApiHandle, _dsMeterID, _spec.ShortAddress, i,
                                               &SensorType, &SensorPollInterval,
                                               &SensorBroadcastFlag, &SensorConversionFlag);
          DSBusInterface::checkResultCode(ret);
          DeviceSensorSpec_t sensorInput;
          sensorInput.SensorType = SensorType;
          sensorInput.SensorPollInterval = SensorPollInterval;
          sensorInput.SensorBroadcastFlag = SensorBroadcastFlag;
          sensorInput.SensorConversionFlag = SensorConversionFlag;
          _spec.sensorInputs.push_back(sensorInput);
        }
        _spec.sensorInputsValid = true;
      } catch(BusApiError& e) {
        Logger::getInstance()->log("Error reading DeviceSensor: " +
        std::string(e.what()), lsWarning);
      }
    }
  } // updateBinaryInputTableFromMeter

  void DSStructureQueryBusInterface::updateOutputChannelTableFromMeter(dsuid_t _dsMeterID, DeviceSpec_t& _spec) {
    _spec.outputChannelsValid = false;
    if (true || ((((_spec.FunctionID >> 6) & 0x3F) == 0x04) && ((_spec.FunctionID & 0x0008) > 0))) {
      try {
        _spec.outputChannels.clear();

        uint8_t numChannels = 0;
        int ret = DeviceOPCTable_get_count(m_DSMApiHandle, _dsMeterID, _spec.ShortAddress, &numChannels);
        if(ret == ERROR_WRONG_MSGID || ret == ERROR_WRONG_MODIFIER) {
          Logger::getInstance()->log("Unsupported message-id DeviceOPCTable_get_count", lsWarning);
          return;
        } else {
          DSBusInterface::checkResultCode(ret);
        }

        for (int i = 0; i < numChannels; i++) {
          uint8_t outputChannel;
          ret = DeviceOPCTable_get_by_index(m_DSMApiHandle, _dsMeterID, _spec.ShortAddress, i,
                                            &outputChannel);
          DSBusInterface::checkResultCode(ret);
          _spec.outputChannels.push_back(outputChannel);
        }
        _spec.outputChannelsValid = true;
      } catch(BusApiError& e) {
        Logger::getInstance()->log("Error reading DeviceOPCTable: " +
        std::string(e.what()), lsWarning);
      }
    }
  } // updateOutputChannelTableFromMeter

  std::vector<DeviceSpec_t> DSStructureQueryBusInterface::getDevicesInZone(const dsuid_t& _dsMeterID, const int _zoneID, bool complete) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::vector<DeviceSpec_t> result;

    int numDevices = getDevicesCountInZone(_dsMeterID, _zoneID);
    Logger::getInstance()->log(std::string("Found ") + intToString(numDevices) + " devices in zone.");
    for(int iDevice = 0; iDevice < numDevices; iDevice++) {
      DeviceSpec_t spec = {};
      uint8_t locked;
      uint8_t groups[GROUPS_LEN];
      uint8_t name[NAME_LEN];
      int ret = DeviceInfo_by_index(m_DSMApiHandle, _dsMeterID, _zoneID, iDevice,
          &spec.ShortAddress, &spec.VendorID, &spec.ProductID, &spec.FunctionID,
          &spec.Version, &spec.ZoneID, &spec.ActiveState, &locked, &spec.OutputMode,
          &spec.LTMode, groups, name, &spec.DSID);
      DSBusInterface::checkResultCode(ret);
      spec.Locked = (locked != 0);
      spec.Groups = parseBitfield(groups, sizeof(groups) * 8);
      spec.Name = std::string(reinterpret_cast<char*>(name));

      if (complete) {
        updateButtonGroupFromMeter(_dsMeterID, spec);
        updateBinaryInputTableFromMeter(_dsMeterID, spec);
        updateSensorInputTableFromMeter(_dsMeterID, spec);
        updateOutputChannelTableFromMeter(_dsMeterID, spec);
      }

      result.push_back(spec);
    }
    return result;
  } // getDevicesInZone

  std::vector<DeviceSpec_t> DSStructureQueryBusInterface::getInactiveDevicesInZone(const dsuid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    std::vector<DeviceSpec_t> result;
    uint16_t numberOfDevices;
    int ret = ZoneDeviceCount_only_inactive(m_DSMApiHandle, _dsMeterID, _zoneID, &numberOfDevices);
    DSBusInterface::checkResultCode(ret);

    for(int iDevice = 0; iDevice < numberOfDevices; iDevice++) {
      DeviceSpec_t spec = {};
      uint8_t locked;
      uint8_t groups[GROUPS_LEN];
      uint8_t name[NAME_LEN];
      int ret = DeviceInfo_by_index_only_inactive(m_DSMApiHandle, _dsMeterID, _zoneID, iDevice,
          &spec.ShortAddress, &spec.VendorID, &spec.ProductID, &spec.FunctionID,
          &spec.Version, &spec.ZoneID, &spec.ActiveState, &locked, &spec.OutputMode,
          &spec.LTMode, groups, name, &spec.DSID);
      DSBusInterface::checkResultCode(ret);
      spec.Locked = (locked != 0);
      spec.Groups = parseBitfield(groups, sizeof(groups) * 8);
      spec.Name = std::string(reinterpret_cast<char*>(name));

      updateButtonGroupFromMeter(_dsMeterID, spec);
      updateBinaryInputTableFromMeter(_dsMeterID, spec);
      updateSensorInputTableFromMeter(_dsMeterID, spec);
      updateOutputChannelTableFromMeter(_dsMeterID, spec);

      result.push_back(spec);
    }
    return result;
  } // getInactiveDevicesInZone

  DeviceSpec_t DSStructureQueryBusInterface::deviceGetSpec(devid_t _id, dsuid_t _dsMeterID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    DeviceSpec_t result = {};
    uint8_t locked;
    uint8_t groups[GROUPS_LEN];
    uint8_t name[NAME_LEN];
    int ret = DeviceInfo_by_device_id(m_DSMApiHandle, _dsMeterID, _id,
        &result.ShortAddress, &result.VendorID, &result.ProductID, &result.FunctionID,
        &result.Version, &result.ZoneID, &result.ActiveState, &locked, &result.OutputMode,
        &result.LTMode, groups, name, &result.DSID);
    DSBusInterface::checkResultCode(ret);
    if (_id != result.ShortAddress) {
      throw BusApiError("DeviceInfo returned answer from a different device (" + intToString(_id) + " != " + intToString(result.ShortAddress) + ")");
    }
    result.Locked = (locked != 0);
    result.Groups = parseBitfield(groups, sizeof(groups) * 8);
    result.Name = std::string(reinterpret_cast<char*>(name));

    updateButtonGroupFromMeter(_dsMeterID, result);
    updateBinaryInputTableFromMeter(_dsMeterID, result);
    updateSensorInputTableFromMeter(_dsMeterID, result);
    updateOutputChannelTableFromMeter(_dsMeterID, result);

    return result;
  } // deviceGetSpec

  std::vector<std::pair<int, int> > DSStructureQueryBusInterface::getLastCalledScenes(const dsuid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }

    uint8_t groups[7];
    uint8_t scenes[7];
    uint8_t hsize;
    int ret = ZoneProperties_get_scene_history(m_DSMApiHandle, _dsMeterID, (uint16_t) _zoneID, &hsize,
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

  std::bitset<7> DSStructureQueryBusInterface::getZoneStates(const dsuid_t& _dsMeterID, const int _zoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }

    uint16_t room;

    // not adding the area stuf
    int ret = ZoneProperties_get_room_states(m_DSMApiHandle, _dsMeterID,
                (uint16_t) _zoneID, &room, NULL, NULL, NULL, NULL);
    if (ret == ERROR_WRONG_MODIFIER) {
      return std::bitset<7>(0);
    }
    DSBusInterface::checkResultCode(ret);
    return std::bitset<7>(room);
  }

  bool DSStructureQueryBusInterface::getEnergyBorder(const dsuid_t& _dsMeterID, int& _lower, int& _upper) {
    // TODO: libdsm
    Logger::getInstance()->log("getEnergyBorder(): not implemented yet", lsInfo);
    return false;
  } // getEnergyBorder

  std::string DSStructureQueryBusInterface::getSceneName(dsuid_t _dsMeterID, boost::shared_ptr<Group> _group, const uint8_t _sceneNumber) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    uint8_t name[20];
    int ret = ZoneGroupSceneInfo(m_DSMApiHandle, _dsMeterID, _group->getZoneID(), _group->getID(), _sceneNumber, name);
    DSBusInterface::checkResultCode(ret);
    return std::string(reinterpret_cast<char*>(&name));
  }

  DSMeterHash_t DSStructureQueryBusInterface::getDSMeterHash(const dsuid_t& _dsMeterID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    DSMeterHash_t result;
    int ret = dSMConfig_get_hash(m_DSMApiHandle, _dsMeterID, &result.Hash, &result.ModificationCount, &result.EventCount);
    DSBusInterface::checkResultCode(ret);

    return result;
  } // getDSMeterHash

  ZoneHeatingConfigSpec_t DSStructureQueryBusInterface::getZoneHeatingConfig(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    ZoneHeatingConfigSpec_t result;
    int ret = ControllerHeating_get_config(m_DSMApiHandle, _dsMeterID, _ZoneID,
        &result.ControllerMode, (uint16_t*)&result.Kp, &result.Ts, &result.Ti, &result.Kd,
        (uint16_t*)&result.Imin, (uint16_t*)&result.Imax, &result.Ymin, &result.Ymax,
        &result.AntiWindUp, &result.KeepFloorWarm, &result.SourceZoneId,
        (uint16_t*)&result.Offset, &result.ManualValue, &result.EmergencyValue);

    if (result.EmergencyValue == 0) {
      result.EmergencyValue = 100;
    }
    DSBusInterface::checkResultCode(ret);
    return result;
  } // getZoneHeatingConfig

  ZoneHeatingStateSpec_t DSStructureQueryBusInterface::getZoneHeatingState(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    ZoneHeatingStateSpec_t result;
    int ret = ControllerHeating_get_state(m_DSMApiHandle, _dsMeterID, _ZoneID,
        &result.State);
    DSBusInterface::checkResultCode(ret);
    return result;
  } // getZoneHeatingState

  ZoneHeatingInternalsSpec_t DSStructureQueryBusInterface::getZoneHeatingInternals(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    ZoneHeatingInternalsSpec_t result;
    int ret = ControllerHeating_get_internals(m_DSMApiHandle, _dsMeterID, _ZoneID,
        &result.Trecent, &result.Treference, (uint16_t*)&result.TError, (uint16_t*)&result.TErrorPrev,
        (uint32_t*)&result.Integral, (uint32_t*)&result.Yp, (uint32_t*)&result.Yi, (uint32_t*)&result.Yd,
        &result.Y, &result.AntiWindUp);
    DSBusInterface::checkResultCode(ret);
    return result;
  } // getZoneHeatingInternals

  ZoneHeatingOperationModeSpec_t DSStructureQueryBusInterface::getZoneHeatingOperationModes(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    ZoneHeatingOperationModeSpec_t result;
    int ret = ControllerHeating_get_operation_modes(m_DSMApiHandle, _dsMeterID, _ZoneID,
        &result.OpMode0, &result.OpMode1, &result.OpMode2, &result.OpMode3,
        &result.OpMode4, &result.OpMode5, &result.OpMode6, &result.OpMode7,
        &result.OpMode8, &result.OpMode9, &result.OpModeA, &result.OpModeB,
        &result.OpModeC, &result.OpModeD, &result.OpModeE, &result.OpModeF);
    DSBusInterface::checkResultCode(ret);
    return result;
  } // getZoneHeatingOperationModes

  dsuid_t DSStructureQueryBusInterface::getZoneSensor(
                                                const dsuid_t& _meterDSUID,
                                                const uint16_t _zoneID,
                                                const uint8_t _sensorType) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }

    dsuid_t sensorDSUID;
    int ret = ZoneProperties_get_zone_sensor(m_DSMApiHandle, _meterDSUID,
                                             _zoneID, _sensorType,
                                             &sensorDSUID);
    DSBusInterface::checkResultCode(ret);
    return sensorDSUID;
  }

  void DSStructureQueryBusInterface::getZoneSensorValue(
      const dsuid_t& _meterDSUID,
      const uint16_t _zoneID,
      const uint8_t _sensorType,
      uint16_t *_sensorValue,
      uint32_t *_sensorAge) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }

    int ret = ZoneProperties_get_zone_sensor_value(m_DSMApiHandle, _meterDSUID,
        _zoneID, _sensorType, _sensorValue, _sensorAge);
    DSBusInterface::checkResultCode(ret);
  }

  void DSStructureQueryBusInterface::protobufMessageRequest(const dsuid_t _dSMdSUID,
                                              const uint16_t _request_size,
                                              const uint8_t *_request,
                                              uint16_t *_response_size,
                                              uint8_t *_response) {
    boost::recursive_mutex::scoped_lock lock(m_DSMApiHandleMutex);
    if(m_DSMApiHandle == NULL) {
      throw BusApiError("Bus not ready");
    }
    int ret = UserProtobufMessageRequest(m_DSMApiHandle, _dSMdSUID,
                                         _request_size, _request,
                                         _response_size, _response);
    DSBusInterface::checkResultCode(ret);
  }

} // namespace dss
