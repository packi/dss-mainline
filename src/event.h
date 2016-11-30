/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Michael Troß, aizo GmbH <michael.tross@aizo.com>

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

#ifndef EVENT_H_
#define EVENT_H_

#include "base.h"
#include "datetools.h"
#include "thread.h"
#include "syncevent.h"
#include "subsystem.h"
#include "propertysystem.h"
#include "model/modelmaintenance.h"

#include <string>
#include <deque>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/atomic.hpp>

namespace dss {

  //================================================== Constants

  namespace EventName {
    extern const std::string CallScene;
    extern const std::string CallSceneBus;
    extern const std::string DeviceSensorValue;
    extern const std::string DeviceSensorEvent;
    extern const std::string DeviceStatus;
    extern const std::string DeviceInvalidSensor;
    extern const std::string DeviceBinaryInputEvent;
    extern const std::string DeviceStateEvent;
    extern const std::string DeviceEventEvent;
    extern const std::string DeviceActionEvent;
    extern const std::string DeviceCustomActionChangedEvent;
    extern const std::string DeviceButtonClick;
    extern const std::string IdentifyBlink;
    extern const std::string ExecutionDenied;
    extern const std::string Running;
    extern const std::string ModelReady;
    extern const std::string UndoScene;
    extern const std::string ZoneSensorValue;
    extern const std::string ZoneSensorError;
    extern const std::string StateChange;
    extern const std::string AddonStateChange;
    extern const std::string HeatingEnabled;
    extern const std::string HeatingSystemCapability;
    extern const std::string HeatingControllerSetup;
    extern const std::string HeatingControllerValue;
    extern const std::string HeatingControllerValueDsHub;
    extern const std::string HeatingControllerState;
    extern const std::string HighLevelEvent;
    extern const std::string OldStateChange;
    extern const std::string AddonToCloud;
    extern const std::string HeatingValveProtection;
    extern const std::string DeviceHeatingTypeChanged;
    extern const std::string UpdateAutoselect;
    extern const std::string LogFileData;
    extern const std::string DebugMonitorUpdate;
    extern const std::string Sunshine;
    extern const std::string FrostProtection;
    extern const std::string HeatingModeSwitch;
    extern const std::string BuildingService;
    extern const std::string OperationLock;
    extern const std::string ClusterConfigLock;
    extern const std::string CheckSensorValues;
    extern const std::string DeviceEvent;
    extern const std::string CheckHeatingGroups;
    extern const std::string Signal;
    extern const std::string WebSessionCleanup;
    extern const std::string SendMail;
    extern const std::string ButtonClickBus;
    extern const std::string DSMeterReady;
    extern const std::string ExecutionDeniedDigestCheck;
    extern const std::string DevicesFirstSeen;
    extern const std::string DatabaseImported;
    extern const std::string ButtonDeviceAction;
    extern const std::string SceneNameChanged;
    extern const std::string ReexportTimings;
    extern const std::string AutoClusterUpdate;
  }

  namespace EventProperty {
    extern const char* Name;
    extern const char* Location;
    extern const char* Context;
    extern const char* Time;
    extern const char* ICalStartTime;
    extern const char* ICalRRule;
    extern const char* Unique;
  }

  //================================================== Forward declarations


  class EventInterpreter;
  class ScheduledEvent;
  class EventRunner;
  class Zone;
  class Group;
  class DeviceReference;
  class State;
  class Apartment;

  //================================================== Class definitions

  //-------------------------------------------------- Event

  typedef enum {
    erlGroup,
    erlDevice,
    erlApartment,
    erlState
  } EventRaiseLocation;

  class Event : public boost::enable_shared_from_this<Event> {
  private:
    std::string m_Name;
    std::string m_Location;
    bool m_LocationSet;
    std::string m_Context;
    bool m_ContextSet;
    std::string m_Time;
    bool m_TimeSet;
    EventRaiseLocation m_RaiseLocation;
    boost::shared_ptr<Group> m_RaisedAtGroup;
    boost::shared_ptr<State> m_RaisedAtState;
    boost::shared_ptr<DeviceReference> m_RaisedAtDevice;

    Properties m_Properties;
    DateTime m_timestamp;
  private:
    void reset();
  public:
    Event(const std::string& _name, boost::shared_ptr<Group> _context);
    Event(const std::string& _name, boost::shared_ptr<State> _state);
    Event(const std::string& _name, boost::shared_ptr<DeviceReference> _ref);
    Event(const std::string& _name);
    Event();

    ~Event();

    const std::string& getName() const { return m_Name; }

    const std::string& getPropertyByName(const std::string& _name) const;
    bool hasPropertySet(const std::string& _name) const;
    void unsetProperty(const std::string& _name);
    bool setProperty(const std::string& _name, const std::string& _value);
    bool setProperty(const std::string& name, SensorType value);

    template<typename T>
    T getPropertyByName(const std::string &name) const;

    void setLocation(const std::string& _value) { m_Location = _value; m_LocationSet = true; }
    void setContext(const std::string& _value) { m_Context = _value; m_ContextSet = true; }
    void setTime(const std::string& _value) { m_Time = _value; m_TimeSet = true; }

    boost::shared_ptr<const Group> getRaisedAtGroup(Apartment& _apartment) const;
    boost::shared_ptr<const Group> getRaisedAtGroup() const;
    boost::shared_ptr<const DeviceReference> getRaisedAtDevice() const { return m_RaisedAtDevice; }
    boost::shared_ptr<const State> getRaisedAtState() const { return m_RaisedAtState; }
    EventRaiseLocation getRaiseLocation() { return m_RaiseLocation; }

    const Properties& getProperties() const { return m_Properties; }
    void setProperties(const Properties& _value) { m_Properties = _value; }
    void applyProperties(const Properties& _others);
    /** Checks whether _other and this are the same regarding uniqueness */
    bool isReplacementFor(const Event& _other);
    boost::shared_ptr<Event> getptr() {
      return shared_from_this();
    };

    DateTime getTimestamp() const { return m_timestamp; }
    std::string toString() const;
  }; // Event


  template <>
  SensorType Event::getPropertyByName<SensorType>(const std::string& value) const;

  //-------------------------------------------------- Events

  class ModelChangedEvent {
  public:
    static const char *Apartment;
    static const char *TimedEvent;
    static const char *UserDefinedAction;

    /* has to be different events, for 'Unique' to work */
    static boost::shared_ptr<Event> createApartmentChanged();
    static boost::shared_ptr<Event> createTimedEventChanged();
    static boost::shared_ptr<Event> createUdaChanged();
  private:
    static boost::shared_ptr<Event> createEvent(const char *desc);
  };

  //-------------------------------------------------- SubscriptionOptions

  class SubscriptionOptions {
  private:
    Properties m_Parameters;
  public:
    SubscriptionOptions();
    virtual ~SubscriptionOptions();

    const std::string& getParameter(const std::string& _name) const;
    void setParameter(const std::string& _name, const std::string& _value);
    bool hasParameter(const std::string& _name) const;

    void loadParameterFromProperty(PropertyNodePtr _node);

    const Properties& getParameters() const { return m_Parameters; }
  }; // SubscriptionOptions


  //-------------------------------------------------- EventPropertyFilter

  class EventPropertyFilter {
  private:
    std::string m_PropertyName;
  public:
    EventPropertyFilter(const std::string& _propertyName);
    virtual ~EventPropertyFilter() {}

    const std::string& getPropertyName() const { return m_PropertyName; }

    virtual bool matches(boost::shared_ptr<Event> _event) = 0;
  }; // EventPropertyFilter


  //-------------------------------------------------- EventPropertyMatchFilter

  class EventPropertyMatchFilter : public EventPropertyFilter {
  private:
    std::string m_Value;
  public:
    EventPropertyMatchFilter(const std::string& _propertyName, const std::string& _value)
    : EventPropertyFilter(_propertyName), m_Value(_value) {}
    virtual ~EventPropertyMatchFilter() {}

    const std::string& getValue() const { return m_Value; }

    virtual bool matches(boost::shared_ptr<Event> _event);
  }; // EventPropertyMatchFilter


  //-------------------------------------------------- EventPropertyExistsFilter

  class EventPropertyExistsFilter : public EventPropertyFilter {
  public:
    EventPropertyExistsFilter(const std::string& _propertyName);
    virtual ~EventPropertyExistsFilter() {};

    virtual bool matches(boost::shared_ptr<Event> _event);
  }; // EventPropertyExistsFilter


  //-------------------------------------------------- EventPropertyMissingFilter

  class EventPropertyMissingFilter : public EventPropertyFilter {
  public:
    EventPropertyMissingFilter(const std::string& _propertyName);
    virtual ~EventPropertyMissingFilter() {};

    virtual bool matches(boost::shared_ptr<Event> _event);
  }; // EventPropertyMissingFilter


  //-------------------------------------------------- EventSubscription

  class EventSubscription {
  public:
    typedef enum {
      foMatchNone,
      foMatchAll,
      foMatchOne
    } EventPropertyFilterOption;
  private:
    std::string m_EventName;
    std::string m_HandlerName;
    std::string m_ID;
    boost::shared_ptr<SubscriptionOptions> m_SubscriptionOptions;
    boost::ptr_vector<EventPropertyFilter> m_Filter;
    EventPropertyFilterOption m_FilterOption;
  protected:
    void initialize();
  public:
    EventSubscription(const std::string& _eventName, const std::string& _handlerName, EventInterpreter& _interpreter, boost::shared_ptr<SubscriptionOptions> _options);

    ~EventSubscription();

    void setEventName(const std::string& _value) { m_EventName = _value; }
    const std::string& getEventName() const { return m_EventName; }
    void setHandlerName(const std::string& _value) { m_HandlerName = _value; }
    const std::string& getHandlerName() const { return m_HandlerName; }
    const std::string& getID() const { return m_ID; }

    boost::shared_ptr<SubscriptionOptions> getOptions() { return m_SubscriptionOptions; }
    boost::shared_ptr<const SubscriptionOptions> getOptions() const { return m_SubscriptionOptions; }

    const EventPropertyFilterOption getFilterMode() { return m_FilterOption; }
    EventPropertyFilter** const getFilter() { return m_Filter.c_array(); }

    void addPropertyFilter(EventPropertyFilter* _pPropertyFilter);

    void setFilterOption(const EventPropertyFilterOption _value) { m_FilterOption = _value; }
    bool matches(boost::shared_ptr<Event> _event);
  }; // EventSubscription


  //-------------------------------------------------- EventInterpreterPlugin

  class EventInterpreterPlugin {
  private:
    std::string m_Name;
    EventInterpreter* const m_pInterpreter;
  protected:
    EventInterpreter& getEventInterpreter() { return *m_pInterpreter; }
  public:
    EventInterpreterPlugin(const std::string& _name, EventInterpreter* _interpreter);
    virtual ~EventInterpreterPlugin() {}

    const std::string& getName() const { return m_Name; }
    /**
     * Subscribe to given events.
     * Fallback when subscribtions.xml makes no sense
     */
    virtual void subscribe() {};
    virtual void handleEvent(Event& _event, const EventSubscription& _subscription) = 0;

    void log(const std::string& _message, aLogSeverity _severity = lsDebug);
  }; // EventInterpreterPlugin


  //-------------------------------------------------- EventQueue

  class EventQueue {
  private:
    std::deque< boost::shared_ptr<Event> > m_EventQueue;
    SyncEvent m_EntryInQueueEvt;
    boost::mutex m_QueueMutex;

    Subsystem* m_Subsystem;
    EventRunner* m_EventRunner;
    boost::shared_ptr<Schedule> scheduleFromEvent(boost::shared_ptr<Event> _event);
    const int m_EventTimeoutMS;

    boost::atomic<unsigned long int> m_ScheduledEventCounter;

  public:
    EventQueue(Subsystem* _subsystem, const int _eventTimeoutMS = 1000);
    void pushEvent(boost::shared_ptr<Event> _event);
    std::string pushTimedEvent(boost::shared_ptr<Event> _event);
    boost::shared_ptr<Event> popEvent();
    bool waitForEvent();

    void shutdown();
    void setEventRunner(EventRunner* _value) { m_EventRunner = _value; }
    void log(const std::string& _message, aLogSeverity _severity = lsDebug);
  };


  //-------------------------------------------------- EventRunner

  class EventRunner : public PropertyListener {
  private:
    boost::ptr_vector<ScheduledEvent> m_ScheduledEvents;
    typedef boost::ptr_vector<ScheduledEvent> m_ScheduledEvents_t;
    DateTime m_WakeTime;
    SyncEvent m_NewItem;
    Subsystem* m_Subsystem;
    EventQueue* m_EventQueue;
    mutable boost::mutex m_EventsMutex;
    bool m_ShutdownFlag;
    PropertyNodePtr m_MonitorNode;
  public:
    EventRunner(Subsystem* _subsystem, PropertyNodePtr _monitorNode = PropertyNodePtr());

    void addEvent(ScheduledEvent* _scheduledEvent);

    size_t getSize() const;
    const ScheduledEvent& getEvent(const std::string& _eventID) const;

    bool raisePendingEvents();

    void removeEvent(const std::string& _eventID);
    void removeEventByName(const std::string& _eventName);

    void run();
    void shutdown();
    void log(const std::string& _message, aLogSeverity _severity = lsDebug);

    void setEventQueue(EventQueue* _value) { m_EventQueue = _value; }
  protected:
    virtual void propertyRemoved(PropertyNodePtr _parent,
                                 PropertyNodePtr _child);
    void removeEventInternal(const std::string& _eventID);
  }; // EventRunner


  //-------------------------------------------------- EventInterpreter

  class EventInterpreter : public Subsystem,
                           public Thread {
  private:
    typedef std::vector< boost::shared_ptr<EventSubscription> > SubscriptionVector;
    SubscriptionVector m_Subscriptions;
    boost::mutex m_SubscriptionsMutex;
    bool m_SubscriptionsMutex_locked;
    std::vector<EventInterpreterPlugin*> m_Plugins;
    EventQueue* m_Queue;
    EventRunner* m_EventRunner;
    int m_EventsProcessed;
  private:
    void loadSubscription(PropertyNodePtr _node);
    void loadState(PropertyNodePtr _node);
    void loadFilter(PropertyNodePtr _node, boost::shared_ptr<EventSubscription> _subscription);
    boost::shared_ptr<EventSubscription> subscriptionByID(const std::string& _name);
  protected:
    virtual void doStart();
  public:
    EventInterpreter(DSS* _pDSS);
    virtual ~EventInterpreter();

    virtual void initialize();

    virtual void execute();
    void executePendingEvent();

    EventInterpreter& addPlugin(EventInterpreterPlugin* _plugin);
    EventInterpreterPlugin* getPluginByName(const std::string& _name);

    void subscribe(boost::shared_ptr<EventSubscription> _subscription);
    void unsubscribe(const std::string& _subscriptionID);
    void unsubscribe(boost::shared_ptr<EventSubscription> _subscription);

    void loadSubscriptionsFromProperty(PropertyNodePtr _node);
    void loadStatesFromProperty(PropertyNodePtr _node);

    std::string uniqueSubscriptionID(const std::string& _proposal);

    int getEventsProcessed() const { return m_EventsProcessed; }
    EventQueue& getQueue() { return *m_Queue; }
    void setEventQueue(EventQueue* _queue) { m_Queue = _queue; }
    EventRunner& getEventRunner() { return *m_EventRunner; }
    void setEventRunner(EventRunner* _runner) { m_EventRunner = _runner; }
    int getNumberOfSubscriptions() const { return m_Subscriptions.size(); }
    SubscriptionVector getSubscriptions() const { return m_Subscriptions; }
  }; // EventInterpreter


  //-------------------------------------------------- ScheduledEvent

  /** Combines an Event with a Schedule
    * These events get raised according to their Schedule
   */
  class ScheduledEvent {
  private:
    boost::shared_ptr<Event> m_Event;
    boost::shared_ptr<Schedule> m_Schedule;
    std::string m_Name;
    std::string m_EventID;
  public:
    ScheduledEvent(boost::shared_ptr<Event> _pEvt,
                   boost::shared_ptr<Schedule> _pSchedule,
                   unsigned long int _counterID);

    /** Returns the event that will be raised */
    boost::shared_ptr<Event> getEvent() { return m_Event; }
    const boost::shared_ptr<Event> getEvent() const { return m_Event; }
    /** Returns the associated Schedule */
    Schedule& getSchedule() const { return *m_Schedule; }
    /** Returns the name of this ScheduledEvent */
    const std::string& getName() const { return m_Name; }
    /** Sets the name of this ScheduledEvent */
    void setName(const std::string& _value) { m_Name = _value; }
    /** Returns the event ID */
    const std::string& getID() const { return m_EventID; }
  }; // ScheduledEvent


} // namespace dss

#endif /* EVENT_H_ */
