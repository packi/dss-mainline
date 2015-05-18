/*
 *  Copyright (c) 2014 digitalSTROM.org, Zurich, Switzerland
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __DS485_DUMMY_INTERFACES__
#define __DS485_DUMMY_INTERFACES__

#include "dss.h"

namespace dss {

class DummyStructureModifyingInterface : public StructureModifyingBusInterface {
public:
  virtual void setZoneID(const dsuid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID) {
  }
  virtual void createZone(const dsuid_t& _dsMeterID, const int _zoneID) {
  }
  virtual void removeZone(const dsuid_t& _dsMeterID, const int _zoneID) {
  }
  virtual void addToGroup(const dsuid_t& _dsMeterID, const int _groupID, const int _deviceID) {
  }
  virtual void removeFromGroup(const dsuid_t& _dsMeterID, const int _groupID, const int _deviceID) {
  }
  virtual void removeInactiveDevices(const dsuid_t& _dsMeterID) {
  }
  virtual void removeDeviceFromDSMeter(const dsuid_t& _dsMeterID, const int _deviceID) {
  }
  virtual void removeDeviceFromDSMeters(const dsuid_t& _deviceDSID) {
  }
  virtual void sceneSetName(uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneNumber, const std::string& _name) {
  }
  virtual void deviceSetName(dsuid_t _meterDSID, devid_t _deviceID, const std::string& _name) {
  }
  virtual void meterSetName(dsuid_t _meterDSID, const std::string& _name) {
  }
  virtual void createGroup(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID, const std::string& _name) {
  }
  virtual void removeGroup(uint16_t _zoneID, uint8_t _groupID) {
  }
  virtual void groupSetName(uint16_t _zoneID, uint8_t _groupID, const std::string& _name) {
  }
  virtual void groupSetStandardID(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID) {
  }
  virtual void clusterSetName(uint8_t _clusterID, const std::string& _name) {}
  virtual void clusterSetStandardID(uint8_t _clusterID, uint8_t _standardGroupID) {}
  virtual void clusterSetProperties(uint8_t _clusterID, uint16_t _location, uint16_t _floor, uint16_t _protectionClass) {}
  virtual void clusterSetLockedScenes(uint8_t _clusterID, const std::vector<int> _lockedScenes) {}
  virtual void sensorPush(uint16_t _zoneID, uint8_t groupID, dsuid_t _sourceID, uint8_t _sensorType, uint16_t _sensorValue) {
  }
  virtual void setButtonSetsLocalPriority(const dsuid_t& _dsMeterID, const devid_t _deviceID, bool _setsPriority) {
  }
  virtual void setButtonCallsPresent(const dsuid_t& _dsMeterID, const devid_t _deviceID, bool _callsPresent) {
  }
  virtual void setZoneHeatingConfig(const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingConfigSpec_t _spec) {}
  virtual void setZoneHeatingState(const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingStateSpec_t _spec) {}
  virtual void setZoneHeatingOperationModes(const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingOperationModeSpec_t _spec) {}
  virtual void setZoneSensor(const uint16_t _zoneID, const uint8_t _sensorType, const dsuid_t& _sensorDSUID) {}
  virtual void resetZoneSensor(const uint16_t _zoneID, const uint8_t _sensorType) {}
};

class DummyStructureQueryBusInterface: public StructureQueryBusInterface {
public:
  virtual std::vector<DSMeterSpec_t> getBusMembers() {
    return std::vector<DSMeterSpec_t>();
  }
  virtual DSMeterSpec_t getDSMeterSpec(const dsuid_t& _dsMeterID) {
    return DSMeterSpec_t();
  }
  virtual std::vector<int> getZones(const dsuid_t& _dsMeterID) {
    return std::vector<int>();
  }
  virtual std::vector<DeviceSpec_t> getDevicesInZone(const dsuid_t& _dsMeterID, const int _zoneID, bool complete = true) {
    return std::vector<DeviceSpec_t>();
  }
  virtual std::vector<DeviceSpec_t> getInactiveDevicesInZone(const dsuid_t& _dsMeterID, const int _zoneID) {
    return std::vector<DeviceSpec_t>();
  }
  virtual std::vector<GroupSpec_t> getGroups(const dsuid_t& _dsMeterID, const int _zoneID) {
    return std::vector<GroupSpec_t>();
  }
  virtual std::vector<ClusterSpec_t> getClusters(const dsuid_t& _dsMeterID) {
    return std::vector<ClusterSpec_t>();
  }
  virtual std::vector<std::pair<int,int> > getLastCalledScenes(const dsuid_t& _dsMeterID, const int _zoneID) {
    return std::vector<std::pair<int,int> >();
  }
  virtual std::bitset<7> getZoneStates(const dsuid_t& _dsMeterID, const int _zoneID) {
      return std::bitset<7>(0);
  }

  virtual bool getEnergyBorder(const dsuid_t& _dsMeterID, int& _lower, int& _upper) {
    _lower = 0;
    _upper = 0;
    return false;
  }
  virtual DeviceSpec_t deviceGetSpec(devid_t _id, dsuid_t _dsMeterID) {
    return DeviceSpec_t();
  }
  virtual std::string getSceneName(dsuid_t _dsMeterID, boost::shared_ptr<Group> _group, const uint8_t _sceneNumber) {
    return "";
  }
  virtual DSMeterHash_t getDSMeterHash(const dsuid_t& _dsMeterID) {
    return DSMeterHash_t();
  }
  virtual ZoneHeatingConfigSpec_t getZoneHeatingConfig(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) {
    return ZoneHeatingConfigSpec_t();
  }
  virtual ZoneHeatingInternalsSpec_t getZoneHeatingInternals(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) {
    return ZoneHeatingInternalsSpec_t();
  }
  virtual ZoneHeatingStateSpec_t getZoneHeatingState(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) {
    return ZoneHeatingStateSpec_t();
  }
  virtual ZoneHeatingOperationModeSpec_t getZoneHeatingOperationModes(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) {
    return ZoneHeatingOperationModeSpec_t();
  }
  virtual dsuid_t getZoneSensor(const dsuid_t& _meterDSUID, const uint16_t _zoneID, const uint8_t _sensorType) {
    return DSUID_NULL;
  }
  virtual void getZoneSensorValue(const dsuid_t& _meterDSUID, const uint16_t _zoneID, const uint8_t _sensorType, uint16_t *_sensorValue, uint32_t *_sensorAge) {
  }
  virtual void protobufMessageRequest(const dsuid_t _dSMdSUID, const uint16_t _request_size, const uint8_t *_request, uint16_t *_response_size, uint8_t *_response) {
  }
}; // DummyStructureQueryBusInterface

class DummyBusInterface : public BusInterface {
public:
  DummyBusInterface(StructureModifyingBusInterface* _pStructureModifier,
                    StructureQueryBusInterface* _pStructureQuery,
                    ActionRequestInterface* _pActionRequest)
  : m_pStructureModifier(_pStructureModifier),
    m_pStructureQuery(_pStructureQuery),
    m_pActionRequest(_pActionRequest)
  { }

  virtual DeviceBusInterface* getDeviceBusInterface() {
    return NULL;
  }
  virtual StructureQueryBusInterface* getStructureQueryBusInterface() {
    return m_pStructureQuery;
  }
  virtual MeteringBusInterface* getMeteringBusInterface() {
    return NULL;
  }
  virtual StructureModifyingBusInterface* getStructureModifyingBusInterface() {
    return m_pStructureModifier;
  }
  virtual ActionRequestInterface* getActionRequestInterface() {
    return m_pActionRequest;
  }

  virtual void setBusEventSink(BusEventSink* _eventSink) {
  }
private:
  StructureModifyingBusInterface* m_pStructureModifier;
  StructureQueryBusInterface* m_pStructureQuery;
  ActionRequestInterface* m_pActionRequest;
};

class DummyActionRequestInterface : public ActionRequestInterface {
public:
  virtual void callScene(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint16_t scene, const std::string _token, const bool _force) {
    Group* pGroup = dynamic_cast<Group*>(pTarget);
    Device* pDevice = dynamic_cast<Device*>(pTarget);
    m_Log += "callScene(";
    if(pGroup != NULL) {
      m_Log += intToString(pGroup->getZoneID()) + "," + intToString(pGroup->getID());
    } else {
      m_Log += intToString(pDevice->getShortAddress());
    }
    m_Log += "," + intToString(scene) + ")";
  }
  virtual void callSceneMin(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint16_t _scene, const std::string _token) {
  }
  virtual void saveScene(AddressableModelItem *pTarget, const callOrigin_t _origin, const uint16_t scene, const std::string _token) {
  }
  virtual void undoScene(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint16_t scene, const std::string _token) {
  }
  virtual void undoSceneLast(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token) {
  }
  virtual void blink(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token) {
  }
  virtual void setValue(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _value, const std::string _token) {
  }
  virtual void increaseOutputChannelValue(AddressableModelItem*, const callOrigin_t _origin, SceneAccessCategory, uint8_t, std::string) {
  }
  virtual void decreaseOutputChannelValue(AddressableModelItem*, const callOrigin_t _origin, SceneAccessCategory, uint8_t, std::string) {
  }
  virtual void stopOutputChannelValue(AddressableModelItem*, const callOrigin_t _origin, SceneAccessCategory, uint8_t, std::string) {
  }
  virtual void pushSensor(AddressableModelItem*, const callOrigin_t _origin, const SceneAccessCategory _category, dsuid_t _sourceID, uint8_t _sensorType, double _sensorValueFloat, const std::string _token) {
  }

  virtual bool isOperationLock(const dsuid_t &_dSM, int _clusterId) {
    return false;
  }

  virtual ~DummyActionRequestInterface(){};

  std::string getLog() {
    return m_Log;
  }

  void clearLog() {
    m_Log.clear();
  }
private:
  std::string m_Log;
};


}
#endif
