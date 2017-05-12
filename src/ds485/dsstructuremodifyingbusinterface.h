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

#ifndef DSSTRUCTUREMODIFYINGBUSINTERFACE_H_
#define DSSTRUCTUREMODIFYINGBUSINTERFACE_H_

#include "src/businterface.h"
#include "model/modelmaintenance.h"
#include "dsbusinterfaceobj.h"

namespace dss {

  class DSStructureModifyingBusInterface : public DSBusInterfaceObj,
                                           public StructureModifyingBusInterface {
  public:
    DSStructureModifyingBusInterface()
    : DSBusInterfaceObj(),
      m_pModelMaintenance(NULL)
    {
    }

    void setZoneID(const dsuid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID) DS_OVERRIDE;
    void createZone(const dsuid_t& _dsMeterID, const int _zoneID) DS_OVERRIDE;
    void removeZone(const dsuid_t& _dsMeterID, const int _zoneID) DS_OVERRIDE;

    void addToGroup(const dsuid_t& _dsMeterID, const int _groupID, const int _deviceID) DS_OVERRIDE;
    void removeFromGroup(const dsuid_t& _dsMeterID, const int _groupID, const int _deviceID) DS_OVERRIDE;
    void removeDeviceFromDSMeter(const dsuid_t& _dsMeterID, const int _deviceID) DS_OVERRIDE;
    void removeDeviceFromDSMeters(const dsuid_t& _DeviceDSID) DS_OVERRIDE;

    void sceneSetName(uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneNumber, const std::string& _name) DS_OVERRIDE;
    void deviceSetName(dsuid_t _meterDSID, devid_t _deviceID, const std::string& _name) DS_OVERRIDE;
    void meterSetName(dsuid_t _meterDSID, const std::string& _name) DS_OVERRIDE;

    void createGroup(uint16_t _zoneID, uint8_t _groupID, ApplicationType applicationType, uint32_t applicationConfig,
        const std::string& _name) DS_OVERRIDE;
    void removeGroup(uint16_t _zoneID, uint8_t _groupID) DS_OVERRIDE;
    void groupSetApplication(
        uint16_t _zoneID, uint8_t _groupID, ApplicationType applicationType, uint32_t applicationConfig) DS_OVERRIDE;
    void groupSetName(uint16_t _zoneID, uint8_t _groupID, const std::string& _name) DS_OVERRIDE;

    void createCluster(uint8_t _groupID, ApplicationType applicationType, uint32_t applicationConfig,
        const std::string& _name) DS_OVERRIDE;
    void removeCluster(uint8_t _clusterID) DS_OVERRIDE;
    void clusterSetName(uint8_t _clusterID, const std::string& _name) DS_OVERRIDE;
    void clusterSetApplication(
        uint8_t _clusterID, ApplicationType applicationType, uint32_t applicationConfig) DS_OVERRIDE;
    void clusterSetProperties(
        uint8_t _clusterID, uint16_t _location, uint16_t _floor, uint16_t _protectionClass) DS_OVERRIDE;
    void clusterSetLockedScenes(uint8_t _clusterID, const std::vector<int> _lockedScenes) DS_OVERRIDE;
    void clusterSetConfigurationLock(uint8_t _clusterID, bool _lock) DS_OVERRIDE;

    void setButtonSetsLocalPriority(const dsuid_t& _dsMeterID, const devid_t _deviceID, bool _setsPriority) DS_OVERRIDE;
    void setButtonCallsPresent(const dsuid_t& _dsMeterID, const devid_t _deviceID, bool _callsPresent) DS_OVERRIDE;

    void setZoneHeatingConfig(
        const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingConfigSpec_t _spec) DS_OVERRIDE;
    void setZoneHeatingOperationModes(
        const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingOperationModeSpec_t _spec) DS_OVERRIDE;
    void setZoneSensor(const uint16_t _zoneID, SensorType sensorType, const dsuid_t& sensorDSUID) DS_OVERRIDE;
    void resetZoneSensor(const uint16_t _zoneID, SensorType sensorType) DS_OVERRIDE;

    void setCircuitPowerStateConfig(
        const dsuid_t& _dsMeterID, const int _index, const int _setThreshold, const int _resetThreshold) DS_OVERRIDE;

    void setModelMaintenace(ModelMaintenance* _modelMaintenance);

    void setProperty(const dsuid_t& _meter,
        const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement>& properties) DS_OVERRIDE;
    vdcapi::Message getProperty(const dsuid_t& _meter,
        const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement>& query) DS_OVERRIDE;

  private:
    ModelMaintenance* m_pModelMaintenance;
  }; // DSStructureModifyingBusInterface

} // namespace dss

#endif
