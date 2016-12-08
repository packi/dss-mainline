/*
    Copyright (c) 2009,2011 digitalSTROM.org, Zurich, Switzerland

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

#ifndef BUSINTERFACE_H_
#define BUSINTERFACE_H_

#include "businterface.h"

#include <string>
#include <vector>
#include <bitset>
#include <boost/shared_ptr.hpp>
#include <limits.h>

#include "base.h"
#include "ds485types.h"
#include "sceneaccess.h"
#include "model/deviceinterface.h"

namespace google {
  namespace protobuf {
    template <typename T>
    class RepeatedPtrField;
  }
}
namespace vdcapi {
  class PropertyElement;
  class Message;
}

namespace dss {

  class Device;
  class AddressableModelItem;
  class BusInterface;
  class Group;

  typedef struct {
    dsuid_t DSID;
    BusMemberDevice_t DeviceType;
    uint32_t SoftwareRevisionARM;
    uint32_t SoftwareRevisionDSP;
    uint32_t HardwareVersion;
    uint16_t APIVersion;
    std::string Name;
    std::bitset<8> flags;
    uint8_t ApartmentState;
  } DSMeterSpec_t;

  typedef struct {
    uint8_t GroupID;
    uint8_t StandardGroupID;
    uint16_t NumberOfDevices;
    std::string Name;
    uint32_t configuration;
  } GroupSpec_t;

  typedef struct {
    uint8_t GroupID;
    uint8_t StandardGroupID;
    bool canHaveStateMachine;
    uint16_t NumberOfDevices;
    std::string Name;
    uint16_t protectionClass;
    uint16_t location;
    uint16_t floor;
    bool configurationLocked;
    std::vector<int> lockedScenes;
    uint32_t configuration;
  } ClusterSpec_t;

  typedef struct {
    GroupType TargetGroupType;
    uint8_t TargetGroup;
    BinaryInputType InputType;
    BinaryInputId InputID;
  } DeviceBinaryInputSpec_t;

  typedef struct {
    std::string Name;
    std::vector<std::string> Values;
  } DeviceStateSpec_t;

  typedef struct {
    uint8_t Index;
    uint8_t State;
    uint32_t ActiveThreshold;
    uint32_t InactiveThreshold;
  } CircuitPowerStateSpec_t;

  typedef struct {
    SensorType sensorType;
    uint32_t SensorPollInterval;
    uint8_t SensorBroadcastFlag;
    uint8_t SensorConversionFlag;
  } DeviceSensorSpec_t;

  typedef struct {
    devid_t ShortAddress;
    uint16_t FunctionID;
    uint16_t ProductID;
    uint16_t VendorID;
    uint16_t Version;
    bool Locked;
    uint8_t ActiveState;
    uint8_t OutputMode;
    uint8_t LTMode;
    std::vector<int> Groups;
    dsuid_t DSID;
    uint8_t ButtonID;
    uint8_t GroupMembership;
    uint8_t ActiveGroup;
    bool SetsLocalPriority;
    bool CallsPresent;
    std::string Name;
    uint16_t ZoneID;
    std::vector<DeviceBinaryInputSpec_t> binaryInputs;
    bool binaryInputsValid;
    std::vector<DeviceSensorSpec_t> sensorInputs;
    bool sensorInputsValid;
    std::vector<int> outputChannels;
    bool outputChannelsValid;
    uint8_t deviceActiveGroup;
    uint8_t deviceDefaultGroup;
  } DeviceSpec_t;

  typedef struct {
    uint32_t Hash;
    uint32_t ModificationCount;
    uint32_t EventCount;
  } DSMeterHash_t;

  typedef struct {
    uint8_t ControllerMode;
    uint16_t Kp;
    uint8_t Ts;
    uint16_t Ti;
    uint16_t Kd;
    int16_t Imin;
    int16_t Imax;
    uint8_t Ymin;
    uint8_t Ymax;
    uint8_t AntiWindUp;
    uint8_t KeepFloorWarm;
    uint16_t SourceZoneId;
    int16_t Offset;
    uint8_t EmergencyValue;
    uint8_t ManualValue;
  } ZoneHeatingConfigSpec_t;

  typedef struct {
    uint16_t Trecent;
    uint16_t Treference;
    int16_t TError;
    int16_t TErrorPrev;
    int32_t Integral;
    int32_t Yp;
    int32_t Yi;
    int32_t Yd;
    uint8_t Y;
    uint8_t AntiWindUp;
  } ZoneHeatingInternalsSpec_t;

  typedef struct {
    uint8_t State;
  } ZoneHeatingStateSpec_t;

  typedef struct {
    uint16_t OpMode0;
    uint16_t OpMode1;
    uint16_t OpMode2;
    uint16_t OpMode3;
    uint16_t OpMode4;
    uint16_t OpMode5;
    uint16_t OpMode6;
    uint16_t OpMode7;
    uint16_t OpMode8;
    uint16_t OpMode9;
    uint16_t OpModeA;
    uint16_t OpModeB;
    uint16_t OpModeC;
    uint16_t OpModeD;
    uint16_t OpModeE;
    uint16_t OpModeF;
  } ZoneHeatingOperationModeSpec_t;

  class DeviceBusInterface {
  public:
    //------------------------------------------------ Device manipulation
    virtual uint8_t getDeviceConfig(const Device& _device,
                                    uint8_t _configClass,
                                    uint8_t _configIndex) = 0;

    virtual uint16_t getDeviceConfigWord(const Device& _device,
                                       uint8_t _configClass,
                                       uint8_t _configIndex) = 0;

    virtual void setDeviceConfig(const Device& _device, uint8_t _configClass,
                                 uint8_t _configIndex, uint8_t _value) = 0;

    /** Set the active group for the button */
    virtual void setDeviceButtonActiveGroup(const Device& _device,
                                            uint8_t _groupID) = 0;

    /** Enable or disable programming mode */
    virtual void setDeviceProgMode(const Device& _device, uint8_t modeId) = 0;

    /** Increase Increase value of selected output channel. */
    virtual void increaseDeviceOutputChannelValue(const Device& _device,
                                            uint8_t _channel) = 0;

    /** Decrease value of selected output channel. */
    virtual void decreaseDeviceOutputChannelValue(const Device& _device,
                                            uint8_t _channel) = 0;

    /** Stop dimming value of selected output channel. */
    virtual void stopDeviceOutputChannelValue(const Device& _device,
                                        uint8_t _channel) = 0;

    virtual uint16_t getDeviceOutputChannelValue(const Device& _device,
                                                 uint8_t _channel) = 0;

    virtual void setDeviceOutputChannelValue(const Device& _device,
                                             uint8_t _channel, uint8_t _size,
                                             uint16_t _value,
                                             bool _applyNow = true) = 0;

    virtual uint16_t getDeviceOutputChannelSceneValue(const Device& _device,
                                                      uint8_t _channel,
                                                      uint8_t _scene) = 0;

    virtual void setDeviceOutputChannelSceneValue(const Device& _device,
                                                  uint8_t _channel,
                                                  uint8_t _size, uint8_t _scene,
                                                  uint16_t _value) = 0;

    virtual uint16_t getDeviceOutputChannelSceneConfig(const Device& _device,
                                                       uint8_t _scene) = 0;

    virtual void setDeviceOutputChannelSceneConfig(const Device& _device,
                                                   uint8_t _scene,
                                                   uint16_t _value) = 0;
    virtual void setDeviceOutputChannelDontCareFlags(const Device& _device,
                                                     uint8_t _scene,
                                                     uint16_t _value) = 0;
    virtual uint16_t getDeviceOutputChannelDontCareFlags(const Device& _device,
                                                         uint8_t _scene) = 0;

    /** Tests transmission quality to a device, where the first returned
      value is the DownstreamQuality and the second value the UpstreamQuality */
    virtual std::pair<uint8_t, uint16_t> getTransmissionQuality(const Device& _device) = 0;

    /** Set generic device output value */
    virtual void setValue(const Device& _device, uint8_t _value) = 0;

    /** Queries sensor value & type from a device */
    virtual uint32_t getSensorValue(const Device& _device, const int _sensorIndex) = 0;

    /** add device to a user group */
    virtual void addGroup(const Device& _device, const int _groupId) = 0;

    /** remove device from a user group */
    virtual void removeGroup(const Device& _device, const int _groupId) = 0;

    /** Tells the dSM to lock the device if \a _lock is true. */
    virtual void lockOrUnlockDevice(const Device& _device, const bool _lock) = 0;

    /** Invokes generic request on vdc device and waits for reply. */
    virtual void genericRequest(const Device& _device, const std::string& methodName,
                                const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& params) = 0;

    virtual void setProperty(const Device& _device,
                                    const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& properties) = 0;
    virtual vdcapi::Message getProperty(const Device& _device,
                                    const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& query) = 0;

    virtual ~DeviceBusInterface() {}; // please the compiler (virtual dtor)
  }; // DeviceBusInterface

  class StructureQueryBusInterface {
  public:
    /** Returns an std::vector containing the dsMeter-spec of all dsMeters present. */
    virtual std::vector<DSMeterSpec_t> getBusMembers() = 0;

    /** Returns the dsMeter-spec for a dsMeter */
    virtual DSMeterSpec_t getDSMeterSpec(const dsuid_t& _dsMeterID) = 0;

    /** Returns a std::vector conatining the zone-ids of the specified dsMeter */
    virtual std::vector<int> getZones(const dsuid_t& _dsMeterID) = 0;
    /** Returns the bus-ids of the devices present in the given zone of the specified dsMeter */
    virtual std::vector<DeviceSpec_t> getDevicesInZone(const dsuid_t& _dsMeterID, const int _zoneID, bool complete = true) = 0;
    virtual std::vector<DeviceSpec_t> getInactiveDevicesInZone(const dsuid_t& _dsMeterID, const int _zoneID) = 0;

    /** Returns the a std::vector containing the group-ids of the given zone on the specified dsMeter */
    virtual std::vector<GroupSpec_t> getGroups(const dsuid_t& _dsMeterID, const int _zoneID) = 0;
    virtual std::vector<ClusterSpec_t> getClusters(const dsuid_t& _dsMeterID) = 0;
    virtual std::vector<CircuitPowerStateSpec_t> getPowerStates(const dsuid_t& _dsMeterID) = 0;

    virtual std::vector<std::pair<int, int> > getLastCalledScenes(const dsuid_t& _dsMeterID, const int _zoneID) = 0;
    virtual std::bitset<7> getZoneStates(const dsuid_t& _dsMeterID, const int _zoneID) = 0;
    virtual bool getEnergyBorder(const dsuid_t& _dsMeterID, int& _lower, int& _upper) = 0;

    /** Returns the function, product and revision id of the device. */
    virtual DeviceSpec_t deviceGetSpec(devid_t _id, dsuid_t _dsMeterID) = 0;

    virtual ~StructureQueryBusInterface() {}; // please the compiler (virtual dtor)
    virtual std::string getSceneName(dsuid_t _dsMeterID, boost::shared_ptr<Group> _group, const uint8_t _sceneNumber) = 0;

    /** returns the hash over the dSMeter's datamodel */
    virtual DSMeterHash_t getDSMeterHash(const dsuid_t& _dsMeterID) = 0;
    virtual void getDSMeterState(const dsuid_t& _dsMeterID, uint8_t *state) = 0;

    /** return heating controller data model */
    virtual ZoneHeatingConfigSpec_t getZoneHeatingConfig(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) = 0;
    virtual ZoneHeatingInternalsSpec_t getZoneHeatingInternals(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) = 0;
    virtual ZoneHeatingStateSpec_t getZoneHeatingState(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) = 0;
    virtual ZoneHeatingOperationModeSpec_t getZoneHeatingOperationModes(const dsuid_t& _dsMeterID, const uint16_t _ZoneID) = 0;
    virtual dsuid_t getZoneSensor(const dsuid_t& _meterDSUID, const uint16_t _zoneID, SensorType _sensorType) = 0;
    virtual void getZoneSensorValue(const dsuid_t& _meterDSUID, const uint16_t _zoneID, SensorType _sensorType, uint16_t *SensorValue, uint32_t *SensorAge) = 0;
    virtual int getDevicesCountInZone(const dsuid_t& _dsMeterID, const int _zoneID, bool _onlyActive = false) = 0;
    /** vdsm property queries */
    virtual void protobufMessageRequest(const dsuid_t _dSMdSUID, const uint16_t _request_size, const uint8_t *_request, uint16_t *_response_size, uint8_t *_response) = 0;
  }; // StructureQueryBusInterface

  class StructureModifyingBusInterface {
  public:
    /** Adds the given device to the specified zone. */
    virtual void setZoneID(const dsuid_t& _dsMeterID, const devid_t _deviceID, const int _zoneID) = 0;

    /** Creates a new Zone on the given dsMeter */
    virtual void createZone(const dsuid_t& _dsMeterID, const int _zoneID) = 0;

    /** Removes the zone \a _zoneID on the dsMeter \a _dsMeterID */
    virtual void removeZone(const dsuid_t& _dsMeterID, const int _zoneID) = 0;

    /** Adds a device to a given group */
    virtual void addToGroup(const dsuid_t& _dsMeterID, const int _groupID, const int _deviceID) = 0;
    /** Removes a device from a given group */
    virtual void removeFromGroup(const dsuid_t& _dsMeterID, const int _groupID, const int _deviceID) = 0;

    /** Removes the device from the dSM data model **/
    virtual void removeDeviceFromDSMeter(const dsuid_t& _dsMeterID, const int _deviceID) = 0;
    virtual void removeDeviceFromDSMeters(const dsuid_t& _DeviceDSID) = 0;

    /** Sets the name of a scene */
    virtual void sceneSetName(uint16_t _zoneID, uint8_t _groupID, uint8_t _sceneNumber, const std::string& _name) = 0;
    virtual void deviceSetName(dsuid_t _meterDSID, devid_t _deviceID, const std::string& _name) = 0;
    virtual void meterSetName(dsuid_t _meterDSID, const std::string& _name) = 0;

    /** Create and manage user groups */
    virtual void createGroup(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID, const std::string& _name) = 0;
    virtual void removeGroup(uint16_t _zoneID, uint8_t _groupID) = 0;
    virtual void groupSetStandardID(uint16_t _zoneID, uint8_t _groupID, uint8_t _standardGroupID) = 0;
    virtual void groupSetName(uint16_t _zoneID, uint8_t _groupID, const std::string& _name) = 0;
    virtual void groupSetConfiguration(uint16_t _zoneID, uint8_t _groupID, uint8_t _groupConfiguration) = 0;

    virtual void createCluster(uint8_t _groupID, uint8_t _standardGroupID, const std::string& _name) = 0;
    virtual void removeCluster(uint8_t _clusterID) = 0;
    virtual void clusterSetName(uint8_t _clusterID, const std::string& _name) = 0;
    virtual void clusterSetStandardID(uint8_t _clusterID, uint8_t _standardGroupID) = 0;
    virtual void clusterSetProperties(uint8_t _clusterID, uint16_t _location, uint16_t _floor, uint16_t _protectionClass) = 0;
    virtual void clusterSetLockedScenes(uint8_t _clusterID, const std::vector<int> _lockedScenes) = 0;
    virtual void clusterSetConfigurationLock(uint8_t _clusterID, bool _lock) = 0;
    virtual void clusterSetConfiguration(uint8_t _clusterID, uint8_t _clusterConfiguration) = 0;
    
    virtual void setButtonSetsLocalPriority(const dsuid_t& _dsMeterID, const devid_t _deviceID, bool _setsPriority) = 0;
    virtual void setButtonCallsPresent(const dsuid_t& _dsMeterID, const devid_t _deviceID, bool _callsPresent) = 0;

    /** Create and manage heating controller */
    virtual void synchronizeZoneHeatingConfig(const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingConfigSpec_t _spec) = 0;
    virtual void setZoneHeatingConfig(const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingConfigSpec_t _spec) = 0;
    virtual void setZoneHeatingState(const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingStateSpec_t _spec) = 0;
    virtual void setZoneHeatingOperationModes(const dsuid_t& _dsMeterID, const uint16_t _ZoneID, const ZoneHeatingOperationModeSpec_t _spec) = 0;
    virtual void setZoneSensor(const uint16_t _zoneID, SensorType _sensorType, const dsuid_t& _sensorDSUID) = 0;
    virtual void resetZoneSensor(const uint16_t _zoneID, SensorType _sensorType) = 0;

    virtual void setCircuitPowerStateConfig(const dsuid_t& _dsMeterID, const int _index, const int _setThreshold, const int _resetThreshold) = 0;

    virtual void setProperty(const dsuid_t& _meter, const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& properties) = 0;
    virtual vdcapi::Message getProperty(const dsuid_t& _meter, const ::google::protobuf::RepeatedPtrField< ::vdcapi::PropertyElement >& query) = 0;

    virtual ~StructureModifyingBusInterface() {}; // please the compiler (virtual dtor)
  }; // StructureModifyingBusInterface

  class ActionRequestInterface {
  public:
    virtual ~ActionRequestInterface() {}; // please the compiler (virtual dtor)
    virtual void callScene(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint16_t scene, const std::string _token, const bool _force) = 0;
    virtual void callSceneMin(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint16_t _scene, const std::string _token) = 0;
    virtual void saveScene(AddressableModelItem *pTarget, const callOrigin_t _origin, const uint16_t scene, const std::string _token) = 0;
    virtual void undoScene(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint16_t scene, const std::string _token) = 0;
    virtual void undoSceneLast(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token) = 0;
    virtual void blink(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const std::string _token) = 0;
    virtual void setValue(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, const uint8_t _value, const std::string _token) = 0;
    virtual void increaseOutputChannelValue(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, uint8_t _channel, const std::string _token) = 0;
    virtual void decreaseOutputChannelValue(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, uint8_t _channel, const std::string _token) = 0;
    virtual void stopOutputChannelValue(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, uint8_t _channel, const std::string _token) = 0;
    virtual void pushSensor(AddressableModelItem *pTarget, const callOrigin_t _origin, const SceneAccessCategory _category, dsuid_t _sourceID, SensorType _sensorType, double _sensorValueFloat, const std::string _token) = 0;
    //< @ret true if locked
    virtual bool isOperationLock(const dsuid_t &_dSM, int _clusterId) = 0;
  }; // ActionRequestInterface

  class MeteringBusInterface {
  public:
    /** Sends a message to all devices to report their meter-data */
    virtual void requestMeterData() = 0;

    /** Returns the current power-consumption in mW */
    virtual unsigned long getPowerConsumption(const dsuid_t& _dsMeterID) = 0;

    /** Returns the meter value in Wh */
    virtual unsigned long getEnergyMeterValue(const dsuid_t& _dsMeterID) = 0;

    virtual ~MeteringBusInterface() {}; // please the compiler (virtual dtor)
  }; // MeteringBusInterface

  class BusEventSink {
  public:
    virtual void onGroupCallScene(BusInterface* _source,
                                  const dsuid_t& _dsMeterID,
                                  const int _zoneID,
                                  const int _groupID,
                                  const int _originDeviceId,
                                  const SceneAccessCategory _category,
                                  const int _sceneID,
                                  const callOrigin_t _origin,
                                  const std::string _token,
                                  const bool _force) = 0;
    virtual void onGroupUndoScene(BusInterface* _source,
                                  const dsuid_t& _dsMeterID,
                                  const int _zoneID,
                                  const int _groupID,
                                  const int _originDeviceId,
                                  const SceneAccessCategory _category,
                                  const int _sceneID,
                                  const bool _explicit,
                                  const callOrigin_t _origin,
                                  const std::string _token) = 0;
    virtual void onGroupBlink(BusInterface* _source,
                              const dsuid_t& _dsMeterID,
                              const int _zoneID,
                              const int _groupID,
                              const int _originDeviceId,
                              const SceneAccessCategory _category,
                              const callOrigin_t _origin,
                              const std::string _token) = 0;
    virtual void onDeviceCallScene(BusInterface* _source,
                                  const dsuid_t& _dsMeterID,
                                  const int _deviceID,
                                  const int _originDeviceId,
                                  const SceneAccessCategory _category,
                                  const int _sceneID,
                                  const callOrigin_t _origin,
                                  const std::string _token,
                                  const bool _force) = 0;
    virtual void onDeviceBlink(BusInterface* _source,
                               const dsuid_t& _dsMeterID,
                               const int _deviceID,
                               const int _originDeviceId,
                               const SceneAccessCategory _category,
                               const callOrigin_t _origin,
                               const std::string _token) = 0;
    virtual void onDeviceUndoScene(BusInterface* _source,
                                  const dsuid_t& _dsMeterID,
                                  const int _deviceID,
                                  const int _originDeviceId,
                                  const SceneAccessCategory _category,
                                  const int _sceneID,
                                  const bool _explicit,
                                  const callOrigin_t _origin,
                                  const std::string _token) = 0;
    virtual void onMeteringEvent(BusInterface* _source,
                                 const dsuid_t& _dsMeterID,
                                 const unsigned int _powerW,
                                 const unsigned int _energyWs) = 0;
    virtual void onZoneSensorValue(BusInterface* _source,
                                   const dsuid_t& _dsMeterID,
                                   const dsuid_t& _sourceDevice,
                                   const int& _zoneID,
                                   const int& _groupID,
                                   SensorType _sensorType,
                                   const int& _sensorValue,
                                   const int& _precision,
                                   const SceneAccessCategory _category,
                                   const callOrigin_t _origin) = 0;
    virtual ~BusEventSink() {};
  };

  /** Interface to be implemented by any bus interface provider */
  class BusInterface {
  public:
    virtual ~BusInterface() {};

    virtual DeviceBusInterface* getDeviceBusInterface() = 0;
    virtual StructureQueryBusInterface* getStructureQueryBusInterface() = 0;
    virtual MeteringBusInterface* getMeteringBusInterface() = 0;
    virtual StructureModifyingBusInterface* getStructureModifyingBusInterface() = 0;
    virtual ActionRequestInterface* getActionRequestInterface() = 0;

    virtual void setBusEventSink(BusEventSink* _eventSink) = 0;
    virtual const std::string getConnectionURI() { return ""; }
  };

  class BusApiError : public DSSException {
  public:
    BusApiError(const std::string& _what, int _error=INT_MIN)
    : DSSException(_what)
    , error(_error) {}
    int error;
  }; // BusApiError

}
#endif /* BUSINTERFACE_H_ */
