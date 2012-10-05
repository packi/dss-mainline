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
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/filesystem.hpp>

#include "src/subsystem.h"
#include "src/thread.h"
#include "src/syncevent.h"
#include "src/model/modelevent.h"
#include "src/taskprocessor.h"
#include "device.h"

namespace dss {
  class Apartment;
  class Event;
  class Metering;
  class StructureQueryBusInterface;

  class ModelDeferredEvent {
  public:
    static const int kModelSceneTimeout = 2;
  private:
    dss_dsid_t m_Source;
    time_t m_Timestamp;
    bool m_IsCalled;
  public:
    /** Constructs a ModelDeferredEvent with timestamp */
    ModelDeferredEvent(dss_dsid_t _source) : m_Source(_source), m_IsCalled(false)
    {
      setTimestamp();
    }
    virtual ~ModelDeferredEvent() {}

    dss_dsid_t getSource() { return m_Source; }
    bool isOriginMyself() { return m_Source.lower == 0 && m_Source.upper == 0; }
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
    bool m_forcedFlag;
  public:
    /** Constructs a ModelDeferredSceneEvent with timestamp */
    ModelDeferredSceneEvent(dss_dsid_t _source, int _zoneID, int _groupID, int _originDeviceID, int _sceneID, bool _forced) :
      ModelDeferredEvent(_source), m_ZoneID(_zoneID), m_GroupID(_groupID), m_OriginDeviceID(_originDeviceID),
      m_SceneID(_sceneID), m_forcedFlag(_forced)
    {}
    virtual ~ModelDeferredSceneEvent() {}

    int getSceneID() { return m_SceneID; }
    int getGroupID() { return m_GroupID; }
    int getZoneID() { return m_ZoneID; }
    int getOriginDeviceID() { return m_OriginDeviceID; }
    bool getForcedFlag() { return m_forcedFlag; }
    void setScene(int _sceneID) { m_SceneID = _sceneID; setTimestamp(); }
  };

  class ModelDeferredButtonEvent : public ModelDeferredEvent {
  private:
    int m_DeviceID;
    int m_ButtonIndex;
    int m_ClickType;
    int m_HoldTime;
  public:
    /** Constructs a ModelDeferredButtonEvent with timestamp */
    ModelDeferredButtonEvent(dss_dsid_t _source, int _deviceID, int _buttonIndex, int _clickType) :
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

  class ModelMaintenance : public ThreadedSubsystem {
  public:
    ModelMaintenance(DSS* _pDSS, const int _eventTimeoutMS = 1000);
    virtual ~ModelMaintenance() {}

    virtual void initialize();

    /** Adds a model event to the queue.
     * The ownership of the event will reside with the Apartment. ModelEvents arriving while initializing will be discarded.
     */
    void addModelEvent(ModelEvent* _pEvent);

    /** Starts the event-processing */
    virtual void execute();

    void onGroupCallScene(dss_dsid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const int _sceneID, const bool _forced);
    void onGroupUndoScene(dss_dsid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const int _sceneID);

    void onDeviceNameChanged(dss_dsid_t _meterID, const devid_t _deviceID, 
                             const std::string& _name);
    void onDsmNameChanged(dss_dsid_t _meterID, const std::string& _name);

    bool isInitializing() const { return m_IsInitializing; }
    void setApartment(Apartment* _value);
    void setMetering(Metering* _value);
    void setStructureQueryBusInterface(StructureQueryBusInterface* _value);

    boost::shared_ptr<TaskProcessor> getTaskProcessor() const
        { return m_taskProcessor; }
  protected:
    virtual void doStart();
  private:
    bool handleModelEvents();
    bool handleDeferredModelEvents();
    void eraseModelEventsFromQueue(ModelEvent::EventType _type);
    void dsMeterReady(const dss_dsid_t& _dsMeterBusID);
    void discoverDS485Devices();
    void readOutPendingMeter();

    void readConfiguration();
    void writeConfiguration();

    void raiseEvent(boost::shared_ptr<Event> _pEvent);

    void onDeviceCallScene(const dss_dsid_t& _dsMeterID, const int _deviceID, const int _originDeviceID, const int _sceneID, const bool _forced);
    void onDeviceActionEvent(const dss_dsid_t& _dsMeterID, const int _deviceID, const int _buttonNr, const int _clickType);

    void onGroupCallSceneFiltered(dss_dsid_t _source, const int _zoneID, const int _groupID, const int _originDeviceID, const int _sceneID, const bool _forced);
    void onDeviceActionFiltered(dss_dsid_t _source, const int _deviceID, const int _buttonNr, const int _clickType);

    void onAddDevice(const dss::dss_dsid_t& _dsMeterID, const int _zoneID, const int _devID);
    void onRemoveDevice(const dss_dsid_t& _dsMeterID, const int _zoneID, const int _devID);
    void onLostDSMeter(const dss_dsid_t& _dsMeterID);
    void onDeviceConfigChanged(const dss_dsid_t& _dsMeterID, int _deviceID, 
                               int _configClass, int _configIndex, int _value);
    void rescanDevice(const dss_dsid_t& _dsMeterID, const int _deviceID);
    void onSensorEvent(dss_dsid_t _meterID, const devid_t _deviceID, const int& _eventIndex);
    void onSensorValue(dss_dsid_t _meterID, const devid_t _deviceID, const int& _sensorIndex, const int& _sensorValue);
    void onEANReady(dss_dsid_t _dsMeterID, const devid_t _deviceID,
                      const DeviceOEMState_t& _state, const unsigned long long& _eanNumber,
                      const int& _serialNumber, const int& _partNumber);
    void onOEMDataReady(dss_dsid_t _dsMeterID, const devid_t _deviceID,
                           const DeviceOEMState_t _state, const std::string& _productName,
                           const std::string& _iconPath, const std::string& _productURL);
  private:
    bool m_IsInitializing;
    bool m_IsDirty;

    boost::ptr_vector<ModelEvent> m_ModelEvents;
    Mutex m_ModelEventsMutex;
    SyncEvent m_NewModelEvent;
    Apartment* m_pApartment;
    Metering* m_pMetering;
    const int m_EventTimeoutMS;
    StructureQueryBusInterface* m_pStructureQueryBusInterface;

    std::list<boost::shared_ptr<ModelDeferredEvent> > m_DeferredEvents;

    void checkConfigFile(boost::filesystem::path _filename);

    boost::shared_ptr<TaskProcessor> m_taskProcessor;

    class OEMWebQuery : public Task {
    public:
      OEMWebQuery(boost::shared_ptr<Device> _device);
      virtual ~OEMWebQuery() {}
      virtual void run();
    private:
      std::string m_EAN;
      uint16_t m_partNumber;
      devid_t m_deviceAdress;
    };
  }; // ModelMaintenance

}

#endif /* MODELMAINTENANCE_H_ */
