/*
    Copyright (c) 2009,2010 digitalSTROM.org, Zurich, Switzerland

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

#ifndef _DSBUSINTERFACE_H_INCLUDED
#define _DSBUSINTERFACE_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "src/ds485types.h"

#include "src/businterface.h"
#include "src/subsystem.h"

#include <digitalSTROM/dsm-api-v2/dsm-api.h>

#include <boost/shared_ptr.hpp>

namespace dss {

  class ModelEvent;
  class ModelMaintenance;
  class DSActionRequest;
  class DSDeviceBusInterface;
  class DSMeteringBusInterface;
  class DSStructureQueryBusInterface;
  class DSStructureModifyingBusInterface;
  class User;

  class DSBusInterface : public ThreadedSubsystem,
                         public BusInterface {
  private:
    boost::shared_ptr<DSActionRequest> m_pActionRequestInterface;
    boost::shared_ptr<DSDeviceBusInterface> m_pDeviceBusInterface;
    boost::shared_ptr<DSMeteringBusInterface> m_pMeteringBusInterface;
    boost::shared_ptr<DSStructureQueryBusInterface> m_pStructureQueryBusInterface;
    boost::shared_ptr<DSStructureModifyingBusInterface> m_pStructureModifyingBusInterface;
    User* m_SystemUser;

    ModelMaintenance* m_pModelMaintenance;

    DsmApiHandle_t m_dsmApiHandle;
    bool m_dsmApiReady;
    std::string m_connectionURI;
    BusEventSink* m_pBusEventSink;

    void loginFromCallback();

    void busReady();
    virtual void execute();
    void connectToDS485D();

    static void busChangeCallback(void* _userData, dsuid_t *_id, int _flag);
    void handleBusChange(dsuid_t *_id, int _flag);

    static void linkStateCallback(void* _userData, bool _link);
    void handleLinkState(bool _link);

    static void eventDeviceAccessibilityOffCallback(uint8_t _errorCode,
                                                    void* _userData,
                                                    dsuid_t _sourceDSMeterID,
                                                   dsuid_t _destinationDSMeterID,
                                                    uint16_t _deviceID,
                                                    uint16_t _zoneID,
                                                    uint16_t _vendorID,
                                                    uint16_t _productID,
                                                    uint16_t _functionID,
                                                    uint16_t _version,
                                                    dsuid_t _deviceDSID);
    void eventDeviceAccessibilityOff(uint8_t _errorCode, dsuid_t _dsMeterID,
                                     uint16_t _deviceID, uint16_t _zoneID,
                                     uint16_t _vendorID, dsuid_t _deviceDSID);
    static void eventDeviceAccessibilityOnCallback(uint8_t _errorCode,
                                                   void* _userData,
                                                   dsuid_t _sourceDSMeterID,
                                                   dsuid_t _destinationDSMeterID,
                                                   uint16_t _deviceID,
                                                   uint16_t _zoneID,
                                                   uint16_t _vendorID,
                                                   uint16_t _productID,
                                                   uint16_t _functionID,
                                                   uint16_t _version,
                                                   dsuid_t _deviceDSID);
    void eventDeviceAccessibilityOn(uint8_t _errorCode, dsuid_t _dsMeterID,
                                    uint16_t _deviceID, uint16_t _zoneID,
                                    uint16_t _vendorID, dsuid_t _deviceDSID);
    static void eventDevicePresenceCallback(uint8_t _errorCode, void* _userData,
                                            dsuid_t _sourceDSMeterID,
                                            dsuid_t _destinationDSMeterID,
                                            uint16_t _deviceID, uint8_t _present);
    void eventDevicePresence(uint8_t _errorCode, dsuid_t _dsMeterID, uint16_t _deviceID,
                             uint8_t _present);
    static void eventDataModelChangedCallback(uint8_t _errorCode, 
                                              void* _userData, 
                                              dsuid_t _sourceID, 
                                              dsuid_t _destinationID, 
                                              uint16_t _shortAddress);
    void eventDataModelChanged(dsuid_t _dsMeterID, uint16_t _shortAddress);
    static void deviceConfigSetCallback(uint8_t _errorCode, void* _userData, 
                                        dsuid_t _sourceID, dsuid_t _destinationID, 
                                        uint16_t _deviceID, uint8_t _configClass, 
                                        uint8_t _configIndex, uint8_t _value);
    void deviceConfigSet(dsuid_t _dsMeterID, uint16_t _deviceID, 
                         uint8_t _configClass, uint8_t _configIndex, 
                         uint8_t _value);
    void handleBusCallScene(uint8_t _errorCode, dsuid_t _sourceID, uint16_t _zoneID,
                            uint8_t _groupID, uint16_t _originDeviceId, uint8_t _sceneID, bool _forced);
    static void handleBusCallSceneCallback(uint8_t _errorCode, void *_userData, dsuid_t _sourceID,
                                           dsuid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                           uint16_t _originDeviceId, uint8_t _sceneID);
    static void handleBusCallSceneForcedCallback(uint8_t _errorCode, void *_userData, dsuid_t _sourceID,
                                               dsuid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                               uint16_t _originDeviceId, uint8_t _sceneID);

    void handleBusUndoScene(uint8_t _errorCode, dsuid_t _sourceID,
                            uint16_t _zoneID, uint8_t _groupID, uint16_t _originDeviceId,
                            uint8_t _sceneID, bool _explicit);
    static void handleBusUndoSceneCallback(uint8_t _errorCode, void *_userData, dsuid_t _sourceID,
                                           dsuid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                           uint16_t _originDeviceId);
    static void handleBusUndoSceneNumberCallback(uint8_t _errorCode, void *_userData, dsuid_t _sourceID,
                                                 dsuid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                                 uint16_t _originDeviceId, uint8_t _sceneID);

    void handleBusBlink(uint8_t _errorCode, dsuid_t _sourceID, uint16_t _zoneID,
                        uint8_t _groupID, uint16_t _originDeviceId);
    static void handleBusBlinkCallback(uint8_t _errorCode, void *_userData, dsuid_t _sourceID,
                                       dsuid_t _targetID, uint16_t _zoneID, uint8_t _groupID,
                                       uint16_t _originDeviceId);

    void handleDeviceLocalAction(dsuid_t _dsMeterID, uint16_t _deviceID, uint8_t _state);
    static void handleDeviceLocalActionCallback(uint8_t _errorCode, void* _userData,
                                        dsuid_t _sourceID, dsuid_t _destinationID,
                                        uint16_t _deviceID, uint16_t _zoneID,
                                        uint8_t _state);

    void handleDeviceAction(dsuid_t _dsMeterID, uint16_t _deviceID, uint8_t _buttonNr, uint8_t _clickType);
    static void handleDeviceActionCallback(uint8_t _errorCode, void* _userData,
                                        dsuid_t _sourceID, dsuid_t _destinationID,
                                        uint16_t _deviceID, uint16_t _zoneID, uint8_t _groupID,
                                        uint8_t _buttonNr, uint8_t _clickType);

    static void handleDirectDeviceActionCallback(uint8_t _errorCode, void* _userData,
                                                 dsuid_t _sourceID, dsuid_t _destinationID,
                                                 uint16_t _deviceID, uint16_t _zoneID, uint8_t _groupID,
                                                 uint16_t _actionID);
    void handleDirectDeviceAction(dsuid_t _dsMeterID, uint16_t _deviceID, uint16_t _actionID);

    void handleDeviceCallScene(dsuid_t _destinationID, uint16_t _deviceID, uint8_t _sceneID, bool _forced);
    static void handleDeviceCallSceneCallback(uint8_t _errorCode, void* _userData,
                                       dsuid_t _sourceID, dsuid_t _destinationID,
                                       uint16_t _deviceID, uint8_t _sceneID);
    static void handleDeviceCallSceneForcedCallback(uint8_t _errorCode, void* _userData,
                                           dsuid_t _sourceID, dsuid_t _destinationID,
                                           uint16_t _deviceID, uint8_t _sceneID);

    void handleDeviceBlink(dsuid_t _destinationID, uint16_t _deviceID);
    static void handleDeviceBlinkCallback(uint8_t _errorCode, void* _userData,
                                          dsuid_t _sourceID, dsuid_t _destinationID,
                                          uint16_t _deviceID);

    void handleDeviceSetName(dsuid_t _destinationID, uint16_t _deviceID, 
                             std::string _name);
    static void handleDeviceSetNameCallback(uint8_t _errorCode, void* _userData, 
                                            dsuid_t _sourceID, 
                                            dsuid_t _destinationID, 
                                            uint16_t _deviceID, 
                                            const uint8_t* _name);

    void handleDsmSetName(dsuid_t _destinationID, std::string _name);
    static void handleDsmSetNameCallback(uint8_t _errorCode,
                                         void *_userData, dsuid_t _sourceID,
                                         dsuid_t _destinationID,
                                         const uint8_t *_name);
    
    void handleCircuitEnergyData(uint8_t _errorCode,
                                 dsuid_t _sourceId, dsuid_t _destinationId,
                                 uint32_t _powerW, uint32_t _energyWh);
    static void handleCircuitEnergyDataCallback(uint8_t _errorCode,
                                                void* _userData,
                                                dsuid_t _sourceId,
                                                dsuid_t _destinationId,
                                                uint32_t _powerW,
                                                uint32_t _energyWs);
    void handleSensorEvent(uint8_t _errorCode,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _deviceID, uint8_t _eventIndex);
    static void handleSensorEventCallback(uint8_t _errorCode, void* _userData,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _deviceID, uint8_t _eventIndex);
    void handleBinaryInputEvent(uint8_t _errorCode,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _deviceID, uint8_t _eventIndex, uint8_t _eventType, uint8_t _state);
    static void handleBinaryInputEventCallback(uint8_t _errorCode, void* _userData,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _deviceID, uint8_t _eventIndex, uint8_t _eventType, uint8_t _state);
    void handleSensorValueEvent(uint8_t _errorCode,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _deviceID, uint8_t _sensorIndex, uint16_t _sensorValue);
    static void handleSensorValueCallback(uint8_t _errorCode, void* _userData,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _deviceID, uint8_t _sensorIndex, uint16_t _sensorValue);
    void handleZoneSensorValueEvent(uint8_t _errorCode,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _ZoneId, uint8_t _GroupId, dsuid_t _dSUID,
        uint8_t _SensorType, uint16_t _Value, uint8_t _Precision);
    static void handleZoneSensorValueCallback(uint8_t _errorCode, void* _userData,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _ZoneId, uint8_t _GroupId, dsuid_t _dSUID,
        uint8_t _SensorType, uint16_t _Value, uint8_t _Precision);

    void handleHeatingControllerConfig(uint8_t _errorCode,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _ZoneId, uint8_t ControllerMode,
        uint16_t _Kp, uint8_t _Ts, uint16_t _Ti, uint16_t _Kd,
        uint16_t _Imin, uint16_t _Imax, uint8_t _Ymin, uint8_t _Ymax,
        uint8_t _AntiWindUp, uint8_t _KeepFloorWarm, uint16_t _SourceZoneId,
        uint16_t _Offset, uint8_t _ManualValue, uint8_t _EmergencyValue);
    static void handleHeatingControllerConfigCallback(uint8_t _errorCode, void *_userData,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _ZoneId, uint8_t _ControllerMode,
        uint16_t _Kp, uint8_t _Ts, uint16_t _Ti, uint16_t _Kd,
        uint16_t _Imin, uint16_t _Imax, uint8_t _Ymin, uint8_t _Ymax,
        uint8_t _AntiWindUp, uint8_t _KeepFloorWarm, uint16_t _SourceZoneId,
        uint16_t _Offset, uint8_t _ManualValue, uint8_t _EmergencyValue);
    void handleHeatingControllerOperationModes(uint8_t _errorCode,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _ZoneId,
        uint16_t _OperationMode0, uint16_t _OperationMode1, uint16_t _OperationMode2, uint16_t _OperationMode3,
        uint16_t _OperationMode4, uint16_t _OperationMode5, uint16_t _OperationMode6, uint16_t _OperationMode7,
        uint16_t _OperationMode8, uint16_t _OperationMode9, uint16_t _OperationModeA, uint16_t _OperationModeB,
        uint16_t _OperationModeC, uint16_t _OperationModeD, uint16_t _OperationModeE, uint16_t _OperationModeF);
    static void handleHeatingControllerOperationModesCallback(uint8_t _errorCode, void *_userData,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _ZoneId,
        uint16_t _OperationMode0, uint16_t _OperationMode1, uint16_t _OperationMode2, uint16_t _OperationMode3,
        uint16_t _OperationMode4, uint16_t _OperationMode5, uint16_t _OperationMode6, uint16_t _OperationMode7,
        uint16_t _OperationMode8, uint16_t _OperationMode9, uint16_t _OperationModeA, uint16_t _OperationModeB,
        uint16_t _OperationModeC, uint16_t _OperationModeD, uint16_t _OperationModeE, uint16_t _OperationModeF);
    void handleHeatingControllerState(uint8_t _errorCode,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _ZoneId, uint8_t _State);
    static void handleHeatingControllerStateCallback(uint8_t _errorCode, void *_userData,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _ZoneId, uint8_t _State);
    void handleHeatingControllerStateEvent(uint8_t _errorCode,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _ZoneId, uint8_t _State);
    static void handleHeatingControllerStateEventCallback(uint8_t _errorCode, void *_userData,
        dsuid_t _sourceID, dsuid_t _destinationID,
        uint16_t _ZoneId, uint8_t _State);

    void handleDsmSetFlags(dsuid_t _destinationID, std::bitset<8> _flags);
    static void handleDsmSetFlagsCallback(uint8_t _errorCode,
                                         void *_userData, dsuid_t _sourceID,
                                         dsuid_t _destinationID,
                                         uint8_t flags);

    void handleClusterSetConfigrationLock(uint8_t _clusterID,
                                          uint8_t _configurationLock);
    static void handleClusterSetConfigrationLockCallback(uint8_t _errorCode,
                                                         void *_userData,
                                                         dsuid_t _sourceID,
                                                         dsuid_t _destinationID,
                                                         uint8_t _clusterID,
                                                         uint8_t _lock);
    void handleClusterSetSceneLock(uint8_t _clusterID,
                                   const uint8_t _lockedScenes[]);
    static void handleClusterSetSceneLockCallback(uint8_t _errorCode,
                                                  void *_userData,
                                                  dsuid_t _sourceID,
                                                  dsuid_t _destinationID,
                                                  uint8_t _clusterID,
                                                  const uint8_t _lockedScenes[]);
    void handleActionRequestExtraCmd(const dsuid_t &_sourceId,
                                     const dsuid_t &_destinationId,
                                     uint16_t _zoneId,
                                     uint8_t _groupId,
                                     uint16_t _originDeviceId,
                                     uint8_t _command);
    static void handleActionRequestExtraCmdCallback(uint8_t _error_code,
                                                    void *_arg,
                                                    dsuid_t _sourceId,
                                                    dsuid_t _destinationId,
                                                    uint16_t _zoneId,
                                                    uint8_t _groupId,
                                                    uint16_t _originDeviceId,
                                                    uint8_t _command);

    void handleGenericEvent(uint8_t _errorCode, dsuid_t _sourceID, dsuid_t _destinationID,
                            uint16_t _EventType, uint8_t _PayloadLength, const uint8_t _Payload[]);
    static void handleGenericEventCallback(uint8_t _errorCode, void *_userData, dsuid_t _sourceID,
                                           dsuid_t _destinationID, uint16_t _EventType,
                                           uint8_t _PayloadLength, const uint8_t _Payload[]);

  protected:
    virtual void doStart();
  public:
    DSBusInterface(DSS* _pDSS, ModelMaintenance* _pModelMaintenance);
    virtual ~DSBusInterface() {};

    virtual void shutdown();

    virtual DeviceBusInterface* getDeviceBusInterface();
    virtual StructureQueryBusInterface* getStructureQueryBusInterface();
    virtual MeteringBusInterface* getMeteringBusInterface();
    virtual StructureModifyingBusInterface* getStructureModifyingBusInterface();
    virtual ActionRequestInterface* getActionRequestInterface();

    virtual void setBusEventSink(BusEventSink* _eventSink);
    static void checkResultCode(const int _resultCode);
    static void checkBroadcastResultCode(const int _resultCode);

    virtual void initialize();

    virtual const std::string getConnectionURI() { return m_connectionURI; }

  }; // DSBusInterface

} // namespace dss

#endif
