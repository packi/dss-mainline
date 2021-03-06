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

#ifndef DSSTRUCTUREQUERYBUSINTERFACE_H_
#define DSSTRUCTUREQUERYBUSINTERFACE_H_

#include "src/businterface.h"

#include "dsbusinterfaceobj.h"

namespace dss {

  class DSStructureQueryBusInterface : public DSBusInterfaceObj,
                                       public StructureQueryBusInterface {
  public:
    DSStructureQueryBusInterface() : DSBusInterfaceObj() {}

    virtual std::vector<DSMeterSpec_t> getBusMembers();
    virtual DSMeterSpec_t getDSMeterSpec(const dsuid_t& _dsMeterID);
    virtual std::vector<int> getZones(const dsuid_t& _dsMeterID);
    virtual std::vector<DeviceSpec_t> getDevicesInZone(const dsuid_t& _dsMeterID, const int _zoneID, bool complete = true);
    virtual std::vector<DeviceSpec_t> getInactiveDevicesInZone(const dsuid_t& _dsMeterID, const int _zoneID);
    virtual std::vector<GroupSpec_t> getGroups(const dsuid_t& _dsMeterID, const int _zoneID);
    virtual std::vector<ClusterSpec_t> getClusters(const dsuid_t& _dsMeterID);
    virtual std::vector<CircuitPowerStateSpec_t> getPowerStates(const dsuid_t& _dsMeterID);
    virtual std::vector<std::pair<int, int> > getLastCalledScenes(const dsuid_t& _dsMeterID, const int _zoneID);
    std::bitset<7> getZoneStates(const dsuid_t& _dsMeterID, const int _zoneID);
    virtual bool getEnergyBorder(const dsuid_t& _dsMeterID, int& _lower, int& _upper);
    virtual DeviceSpec_t deviceGetSpec(devid_t _id, dsuid_t _dsMeterID);
    virtual std::string getSceneName(dsuid_t _dsMeterID, boost::shared_ptr< dss::Group > _group, const uint8_t _sceneNumber);
    virtual DSMeterHash_t getDSMeterHash(const dsuid_t& _dsMeterID);
    virtual void getDSMeterState(const dsuid_t& _dsMeterID, uint8_t* state);
    virtual ZoneHeatingConfigSpec_t getZoneHeatingConfig(const dsuid_t& _dsMeterID, const uint16_t _ZoneID);
    virtual ZoneHeatingInternalsSpec_t getZoneHeatingInternals(const dsuid_t& _dsMeterID, const uint16_t _ZoneID);
    virtual ZoneHeatingStateSpec_t getZoneHeatingState(const dsuid_t& _dsMeterID, const uint16_t _ZoneID);
    virtual ZoneHeatingOperationModeSpec_t getZoneHeatingOperationModes(const dsuid_t& _dsMeterID, const uint16_t _ZoneID);
    virtual dsuid_t getZoneSensor(const dsuid_t& _meterDSUID, const uint16_t _zoneID, SensorType _sensorType);
    virtual void getZoneSensorValue(const dsuid_t& _meterDSUID, const uint16_t _zoneID, SensorType _sensorType, uint16_t *_sensorValue, uint32_t *_sensorAge);
    virtual int getDevicesCountInZone(const dsuid_t& _dsMeterID, const int _zoneID, bool _onlyActive = false);
    virtual void protobufMessageRequest(const dsuid_t _dSMdSUID, const uint16_t _request_size, const uint8_t *_request, uint16_t *_response_size, uint8_t *_response);
  private:
    int getGroupCount(const dsuid_t& _dsMeterID, const int _zoneID);
    std::vector<int> makeDeviceGroups(const uint8_t *bitfield, int bits, const DeviceSpec_t& spec);
    void checkDeviceActiveDefaultGroup(const DeviceSpec_t& spec);
    void updateButtonGroupFromMeter(dsuid_t _dsMeterID, DeviceSpec_t& _spec);
    void updateBinaryInputTableFromMeter(dsuid_t _dsMeterID, DeviceSpec_t& _spec);
    void updateSensorInputTableFromMeter(dsuid_t _dsMeterID, DeviceSpec_t& _spec);
    void updateOutputChannelTableFromMeter(dsuid_t _dsMeterID, DeviceSpec_t& _spec);
  }; // DSStructureQueryBusInterface

} // namespace dss

#endif
