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

#ifndef MODELMAINTENANCE_H_
#define MODELMAINTENANCE_H_

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include "dssfwd.h"
#include "src/subsystem.h"
#include "src/thread.h"
#include "src/model/modelevent.h"
#include "src/taskprocessor.h"
#include "device.h"
#include "../webservice_connection.h"

namespace dss {
  class Apartment;
  class Event;
  class Metering;
  class StructureQueryBusInterface;

  class ModelDeferredEvent {
  public:
    static const int kModelSceneTimeout = 2;
  private:
    dsuid_t m_Source;
    time_t m_Timestamp;
    bool m_IsCalled;
  public:
    /** Constructs a ModelDeferredEvent with timestamp */
    ModelDeferredEvent(dsuid_t _source) : m_Source(_source), m_IsCalled(false)
    {
      setTimestamp();
    }
    virtual ~ModelDeferredEvent() {}

    dsuid_t getSource() { return m_Source; }
    bool isOriginMyself() { return (m_Source == DSUID_NULL); }
    bool isDue() { time_t now = time(NULL); return (now - m_Timestamp >= kModelSceneTimeout); }
    bool isCalled() { return m_IsCalled; }

    void setTimestamp() { m_Timestamp = time(NULL); }
    void clearTimestamp() { m_Timestamp = 0; }
    void setCalled() { m_IsCalled = true; }
  };

  class ModelDeferredSceneEvent : public ModelDeferredEvent {
  private:
    int m_ZoneID;
    int m_GroupID;
    int m_OriginDeviceID;
    int m_SceneID;
    callOrigin_t m_Origin;
    bool m_forcedFlag;
    std::string m_OriginToken;
  public:
    /** Constructs a ModelDeferredSceneEvent with timestamp */
    ModelDeferredSceneEvent(dsuid_t _source, int _zoneID, int _groupID, int _originDeviceID, int _sceneID, callOrigin_t _origin, bool _forced, std::string _token) :
      ModelDeferredEvent(_source), m_ZoneID(_zoneID), m_GroupID(_groupID), m_OriginDeviceID(_originDeviceID),
      m_SceneID(_sceneID), m_Origin(_origin), m_forcedFlag(_forced), m_OriginToken(_token)
    {}
    virtual ~ModelDeferredSceneEvent() {}

    int getSceneID() { return m_SceneID; }
    int getGroupID() { return m_GroupID; }
    int getZoneID() { return m_ZoneID; }
    int getOriginDeviceID() { return m_OriginDeviceID; }
    bool getForcedFlag() { return m_forcedFlag; }
    callOrigin_t getCallOrigin() { return m_Origin; }
    void setScene(int _sceneID) { m_SceneID = _sceneID; setTimestamp(); }
    std::string getOriginToken() { return m_OriginToken; }
  };

  class ModelDeferredButtonEvent : public ModelDeferredEvent {
  private:
    int m_DeviceID;
    int m_ButtonIndex;
    int m_ClickType;
    int m_HoldTime;
  public:
    /** Constructs a ModelDeferredButtonEvent with timestamp */
    ModelDeferredButtonEvent(dsuid_t _source, int _deviceID, int _buttonIndex, int _clickType) :
      ModelDeferredEvent(_source),
      m_DeviceID(_deviceID), m_ButtonIndex(_buttonIndex), m_ClickType(_clickType), m_HoldTime(0)
    {}
    virtual ~ModelDeferredButtonEvent() {}

    int getDeviceID() { return m_DeviceID; }
    int getButtonIndex() { return m_ButtonIndex; }
    int getClickType() { return m_ClickType; }
    int getRepeatCount() { return m_HoldTime; }
    void setClickType(int _clickType) { m_ClickType = _clickType; setTimestamp(); }
    void incRepeatCount() { m_HoldTime ++; }
  };


  //----------------------------------------------------------------------------
  class MeterMaintenance: public Thread {
    __DECL_LOG_CHANNEL__
  public:
    MeterMaintenance(DSS* _pDSS, const std::string& _name);
    virtual void shutdown() { Thread::terminate(); }
    bool isRunning() { return Thread::isRunning(); }
    void triggerMeterSynchronization();
    /** Starts the event-processing */
    virtual void execute();

  private:
    void synchronizeMeters();
    int getNumValiddSMeters() const;
    void readOutPendingMeter();
    void dsMeterReady(const dsuid_t& _dsMeterBusID);
    void setApartmentState();
    void autoAssignSensors();
    void synchronizeZoneSensorAssignment();
    void raiseEvent(const boost::shared_ptr<Event> &event);
    void setupInitializedState();
    void monitorInitialization();
    int getdSMBusMemberCount();

  private:
    DSS* m_pDSS;
    Apartment* m_pApartment;
    StructureModifyingBusInterface* m_pModifyingBusInterface;
    StructureQueryBusInterface* m_pQueryBusInterface;
    bool m_IsInitializing;
    boost::mutex m_syncMutex;
    bool m_triggerSynchronize;
    int m_retryCount;
  };
  //----------------------------------------------------------------------------

  class ModelMaintenance : public ThreadedSubsystem {
  public:

    class OEMWebQuery : public Task {
    public:
      class OEMWebQueryCallback : public URLRequestCallback {
      public:
        OEMWebQueryCallback(dsuid_t _deviceDSUID, DeviceOEMState_t _oldState);
        virtual ~OEMWebQueryCallback() {}
        virtual void result(long code, const std::string &result);
      private:
        dsuid_t m_deviceDSUID;
        DeviceOEMState_t m_oldOEMState;
        std::string m_configLink;
      };

      OEMWebQuery(boost::shared_ptr<Device> _device, DeviceOEMState_t _oldState);
      virtual ~OEMWebQuery() {}
      virtual void run();

    private:
      dsuid_t m_deviceDSUID;
      std::string m_EAN;
      uint16_t m_partNumber;
      uint16_t m_serialNumber;
      bool m_queryConfigLink;
      DeviceOEMState_t m_oldOEMState;
    };

    class VdcDataQuery : public Task {
    public:
      VdcDataQuery(boost::shared_ptr<Device> _device);
      virtual ~VdcDataQuery() {}
      virtual void run();

    private:
      boost::shared_ptr<Device> m_Device;
    };

    class DatabaseDownload : public Task {
    public:
      DatabaseDownload(std::string _script_id, std::string _url);
      virtual ~DatabaseDownload() {}
      virtual void run();

    private:
      std::string m_scriptId;
      std::string m_url;
    };

    class WebSocketEvent : public Task {
    public:
      WebSocketEvent(boost::shared_ptr<Event> _event);
      virtual ~WebSocketEvent() {}
      virtual void run();

    private:
      boost::shared_ptr<Event> m_event;
    };

    ModelMaintenance(DSS* _pDSS, const int _eventTimeoutMS = 1000);
    virtual ~ModelMaintenance() {}

    virtual void shutdown();

    virtual void initialize();

    /** Adds a model event to the queue.
     * The ownership of the event will reside with the Apartment. ModelEvents arriving while initializing will be discarded.
     */
    void addModelEvent(ModelEvent* _pEvent);
    void publishWebSocketEvent(boost::shared_ptr<Event> _event);

    /** Starts the event-processing */
    virtual void execute();

    void onGroupCallScene(dsuid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const int _sceneID, const callOrigin_t _origin, const bool _forced, std::string _token);
    void onGroupUndoScene(dsuid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const int _sceneID, const callOrigin_t _origin, const std::string _token);

    void onDeviceNameChanged(dsuid_t _meterID, const devid_t _deviceID,
                             const std::string& _name);
    void onDsmNameChanged(dsuid_t _meterID, const std::string& _name);
    void onDsmFlagsChanged(dsuid_t _meterID, const std::bitset<8> _flags);

    bool isInitializing() const { return m_IsInitializing; }
    void setApartment(Apartment* _value);
    void setMetering(Metering* _value);
    void setStructureQueryBusInterface(StructureQueryBusInterface* _value);
    void setStructureModifyingBusInterface(StructureModifyingBusInterface* _value);

    TaskProcessor& getTaskProcessor() { return m_taskProcessor; }
    TaskProcessor& getTaskProcessorMaySleep() { return m_taskProcessorMaySleep; }

    void scheduleDeviceReadout(const dsuid_t& _dSMeterID,
                               boost::shared_ptr<Task> task);
    /**
     * When the UI triggers some changes, it typically re-reads the structure
     * immediately to update its view. If we answer that request too early,
     * e.g. before automatic clusters have been regrouped, the UI will not see
     * the effect of its changes.u
     * This call will scan the event queue for dirty events, as these mark
     * the end of a set of changes. It will then block until that event has
     * been processed.
     * TODO the apartment model might still be inconsistent, since new changes
     * might be scheduled, from another UI, app or ds485 event.
     */
    bool pendingChangesBarrier(int waitSeconds = 60);
  protected:
    virtual void doStart();
    bool handleModelEvents(); //< access from unit test
    unsigned indexOfNextSyncState(); //< access form unit test
  private:
    bool handleDeferredModelEvents();
    void handleDeferredModelStateChanges(callOrigin_t _origin, int _zoneID, int _groupID, int _sceneID);
    void discoverDS485Devices();
    void readConfiguration();
    void writeConfiguration();
    void scheduleConfigWrite();
    void delayedConfigWrite();

    void raiseEvent(const boost::shared_ptr<Event> &event);
    void notifyModelConsistent(); //< notifies pendingChangesBarrier

    void onDeviceCallScene(const dsuid_t& _dsMeterID, const int _deviceID, const int _originDeviceID, const int _sceneID, const callOrigin_t _origin, const bool _forced, const std::string _token);
    void onDeviceBlink(const dsuid_t& _dsMeterID, const int _deviceID, const int _originDeviceID, const callOrigin_t _origin, const std::string _token);
    void onDeviceActionEvent(const dsuid_t& _dsMeterID, const int _deviceID, const int _buttonNr, const int _clickType);
    void onDeviceDirectActionEvent(const dsuid_t& _dsMeterID, const int _deviceID, const int _actionID);

    void onGroupCallSceneFiltered(dsuid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const int _sceneID, const callOrigin_t _origin, const bool _forced, const std::string _token);
    void onDeviceActionFiltered(dsuid_t _source, const int _deviceID, const int _buttonNr, const int _clickType);
    void onGroupBlink(dsuid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const callOrigin_t _origin, const std::string _token);

    void onAddDevice(const dsuid_t& _dsMeterID, const int _zoneID, const int _devID);
    void onRemoveDevice(const dsuid_t& _dsMeterID, const int _zoneID, const int _devID);
    void onJoinedDSMeter(const dsuid_t& _dsMeterID);
    void onLostDSMeter(const dsuid_t& _dsMeterID);
    void onDeviceConfigChanged(const dsuid_t& _dsMeterID, int _deviceID,
                               int _configClass, int _configIndex, int _value);
    void rescanDevice(const dsuid_t& _dsMeterID, const int _deviceID);
    void onSensorEvent(dsuid_t _meterID, const devid_t _deviceID, const int& _eventIndex);
    void onBinaryInputEvent(dsuid_t _meterID, const devid_t _deviceID, const int& _eventIndex, const int& _eventType, const int& _state);
    void onSensorValue(dsuid_t _meterID, const devid_t _deviceID, const int& _sensorIndex, const int& _sensorValue);
    void onCircuitPowerStateChange(dsuid_t _meterID, const int& _baseIndex, const int& _stateMask, const int& _stateValue);
    void onZoneSensorValue(const dsuid_t &_meterID, const dsuid_t& _sourceDevice, const int& _zoneID, const int& _groupID, const int& _sensorType, const int& _sensorValue, const int& _precision);
    void onEANReady(dsuid_t _dsMeterID, const devid_t _deviceID,
                      const DeviceOEMState_t _state, const DeviceOEMInetState_t _iNetState,
                      const unsigned long long _eanNumber,
                      const int _serialNumber, const int _partNumber,
                      const bool _isIndependent, const bool _isConfigLocked,
                      const int _pairedDevices, const bool _isVisible);
    void onOEMDataReady(dsuid_t _deviceID,
                           const DeviceOEMState_t _state, const std::string& _productName,
                           const std::string& _iconPath, const std::string& _productURL,
                           const std::string& _defaultName,
                           const std::string& _configLink);
    void onDeviceOEMDataUpdateProductInfoState(dsuid_t _deviceID,
                                               const DeviceOEMState_t _state);

    void onHeatingControllerConfig(dsuid_t _dsMeterID, const int _ZoneID, boost::shared_ptr<void> _spec);
    void onHeatingControllerValues(dsuid_t _dsMeterID, const int _ZoneID, boost::shared_ptr<void> _spec);
    void onHeatingControllerState(dsuid_t _dsMeterID, const int _ZoneID, const int _State);

    void onClusterConfigLock(const int _clusterID, const bool _configurationLock);
    void onClusterLockedScenes(const int _clusterID, const std::vector<int> &_lockedScenes);

    void onGenericEvent(const GenericEventType_t _eventType, const boost::shared_ptr<GenericEventPayload_t> &_pPayload, const callOrigin_t _origin);
    void onOperationLock(int _zoneID, int _groupID, bool _lock, callOrigin_t _callOrigin);
    void onAutoClusterMaintenance(dsuid_t _deviceID);
    void onAutoClusterCleanup();
    void onMeterReady();
    void onDeviceDataReady(dsuid_t _meterID, const devid_t _deviceID,
                        const int& _pairedDevices, const bool& _visible);
    void onDsmStateChange(dsuid_t _meterID, const uint8_t& _state);
    void onVdceEvent(const VdceModelEvent& event);

    void setupWebUpdateEvent();
    void updateWebData(Event& _event, const EventSubscription& _subscription);
    void sendWebUpdateEvent(int _interval = 86400);
    void pollSensors(boost::shared_ptr<DeviceReference> pDevRef);
    void synchronizeHeatingAssignment(const dsuid_t& _dSMeterID);

  protected:
    bool m_IsInitializing; //< allow to clear from unit test
    unsigned m_processedEvents; //< actually dequeued, increased before processing

  private:
    bool m_IsDirty;
    bool m_pendingSaveRequest;
    DateTime m_pendingSaveRequestTS;
    unsigned m_suppressSaveRequestNotify;
    boost::mutex m_SaveRequestMutex;

    typedef boost::ptr_deque<ModelEvent> m_ModelEvents_t;
    boost::ptr_deque<ModelEvent> m_ModelEvents;
    boost::mutex m_ModelEventsMutex;
    boost::condition_variable m_NewModelEvent;

    Apartment* m_pApartment;
    Metering* m_pMetering;
    const boost::chrono::milliseconds m_EventTimeoutMS;
    StructureQueryBusInterface* m_pStructureQueryBusInterface;
    StructureModifyingBusInterface* m_pStructureModifyingBusInterface;

    typedef std::vector<boost::shared_ptr<ModelDeferredEvent> > m_DeferredEvents_t;
    std::vector<boost::shared_ptr<ModelDeferredEvent> > m_DeferredEvents;

    void checkConfigFile(boost::filesystem::path _filename);

    TaskProcessor m_taskProcessor;
    TaskProcessor m_taskProcessorMaySleep;

    static const std::string kWebUpdateEventName;
    boost::shared_ptr<InternalEventRelayTarget> m_pRelayTarget;

    boost::mutex m_rvMut;
    boost::condition_variable m_rvCond;

    boost::shared_ptr<MeterMaintenance> m_pMeterMaintenance;

    boost::mutex m_readoutTasksMutex;
    std::vector<std::pair<dsuid_t, boost::shared_ptr<Task> > > m_deviceReadoutTasks;

  }; // ModelMaintenance

}

#endif /* MODELMAINTENANCE_H_ */
