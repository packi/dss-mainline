/*
 * event.h
 *
 *  Created on: Oct 24, 2008
 *      Author: patrick
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "model.h"
#include "thread.h"
#include "syncevent.h"
#include "mutex.h"
#include "xmlwrapper.h"

#include <string>
#include <queue>

using std::string;
using std::queue;

namespace dss {

  //================================================== Forward declarations

  class EventInterpreter;
  class ScheduledEvent;
  class EventRunner;

  //================================================== Class definitions

  //-------------------------------------------------- Event

  typedef enum EventRaiseLocation {
    erlZone,
    erlDevice,
    erlApartment
  };

  class Event {
  private:
    string m_Name;
    string m_Location;
    bool m_LocationSet;
    string m_Context;
    bool m_ContextSet;
    string m_Time;
    bool m_TimeSet;
    EventRaiseLocation m_RaiseLocation;
    Zone* m_RaisedAtZone;
    DeviceReference* m_RaisedAtDevice;
  public:
    Event(const string& _name, Zone* _context);
    Event(const string& _name, DeviceReference* _ref);
    Event(const string& _name);

    const string& GetName() const { return m_Name; }

    string GetPropertyByName(const string& _name) const;
    bool HasPropertySet(const string& _name) const;
    bool UnsetProperty(const string& _name);

    void SetLocation(const string& _value) { m_Location = _value; m_LocationSet = true; }
    void SetContext(const string& _value) { m_Context = _value; m_ContextSet = true; }
    void SetTime(const string& _value) { m_Time = _value; m_TimeSet = true; }

    const Zone& GetRaisedAtZone() const;
    const DeviceReference& GetRaisedAtDevice() const { return *m_RaisedAtDevice; }
  }; // Event


  //-------------------------------------------------- SubscriptionOptions

  class SubscriptionOptions {
  private:
    HashMapConstStringString m_Parameters;
  public:
    SubscriptionOptions();
    virtual ~SubscriptionOptions();

    const string& GetParameter(const string& _name) const;
    void SetParameter(const string& _name, const string& _value);
    bool HasParameter(const string& _name) const;

    void LoadParameterFromXML(XMLNode& _node);
  }; // SubscriptionOptions


  //-------------------------------------------------- EventPropertyFilter

  class EventPropertyFilter {
  private:
    string m_PropertyName;
  public:
    EventPropertyFilter(const string& _propertyName)
    : m_PropertyName(_propertyName) {}
    virtual ~EventPropertyFilter() {}

    const string& GetPropertyName() const { return m_PropertyName; }

    virtual bool Matches(const Event& _event) = 0;
  }; // EventPropertyFilter


  //-------------------------------------------------- EventPropertyMatchFilter

  class EventPropertyMatchFilter : public EventPropertyFilter {
  private:
    string m_Value;
  public:
    EventPropertyMatchFilter(const string& _propertyName, const string& _value)
    : EventPropertyFilter(_propertyName), m_Value(_value) {}

    virtual ~EventPropertyMatchFilter() {}

    virtual bool Matches(const Event& _event);
  }; // EventPropertyMatchFilter


  //-------------------------------------------------- EventPropertyExistsFilter

  class EventPropertyExistsFilter : public EventPropertyFilter {
  public:
    virtual ~EventPropertyExistsFilter() {}
    virtual bool Matches(const Event& _event);
  }; // EventPropertyExistsFilter


  //-------------------------------------------------- EventPropertyMissingFilter

  class EventPropertyMissingFilter : public EventPropertyFilter {
  public:
    virtual ~EventPropertyMissingFilter() {}
    virtual bool Matches(const Event& _event);
  }; // EventPropertyMissingFilter


  //-------------------------------------------------- EventSubscription

  class EventSubscription {
  private:
    string m_EventName;
    string m_HandlerName;
    string m_ID;
    SubscriptionOptions* m_SubscriptionOptions;
    vector<EventPropertyFilter*> m_Filter;
    typedef enum {
      foMatchNone,
      foMatchAll,
      foMatchOne
    } EventPropertyFilterOption;
    EventPropertyFilterOption m_FilterOption;
  protected:
    void Initialize();
  public:
    EventSubscription(const string& _eventName, const string& _handlerName, SubscriptionOptions* _options);
    EventSubscription(const string& _eventName, const string& _handlerName, const string& _id, SubscriptionOptions* _options);

    ~EventSubscription();

    void SetEventName(const string& _value) { m_EventName = _value; }
    const string& GetEventName() const { return m_EventName; }
    void SetHandlerName(const string& _value) { m_HandlerName = _value; }
    const string& GetHandlerName() const { return m_HandlerName; }
    const string& GetID() const { return m_ID; }

    SubscriptionOptions& GetOptions() { return *m_SubscriptionOptions; }
    const SubscriptionOptions& GetOptions() const { return *m_SubscriptionOptions; }

    void AddPropertyFilter(EventPropertyFilter* _pPropertyFilter);

    void SetFilterOption(const EventPropertyFilterOption _value) { m_FilterOption = _value; }
    bool Matches(Event& _event);
  }; // EventSubscription


  //-------------------------------------------------- EventInterpreterPlugin

  class EventInterpreterPlugin {
  private:
    string m_Name;
    EventInterpreter* const m_pInterpreter;
  protected:
    EventInterpreter& GetEventInterpreter() { return *m_pInterpreter; }
  public:
    EventInterpreterPlugin(const string& _name, EventInterpreter* _interpreter);
    virtual ~EventInterpreterPlugin() {}

    const string& GetName() const { return m_Name; }
    virtual void HandleEvent(Event& _event, const EventSubscription& _subscription) = 0;

    virtual SubscriptionOptions* CreateOptionsFromXML(XMLNodeList& _nodes);
  }; // EventInterpreterPlugin


  //-------------------------------------------------- EventQueue

  class EventQueue {
  private:
    queue<Event*> m_EventQueue;
    SyncEvent m_EntryInQueueEvt;
    Mutex m_QueueMutex;

    EventRunner* m_EventRunner;
  public:
    EventQueue();
    void PushEvent(Event* _event);
    Event* PopEvent();
    bool WaitForEvent();

    void Shutdown();

    void SetEventRunner(EventRunner* _value) { m_EventRunner = _value; }
  };


  //-------------------------------------------------- EventRunner

  class EventRunner {
  private:
    boost::ptr_vector<ScheduledEvent> m_ScheduledEvents;

    DateTime GetNextOccurence();
    DateTime m_WakeTime;
    SyncEvent m_NewItem;
    EventQueue* m_EventQueue;
  public:
    EventRunner();

    void AddEvent(ScheduledEvent* _scheduledEvent);

    bool RaisePendingEvents(DateTime& _from, int _deltaSeconds);

    int GetSize() const;
    const ScheduledEvent& GetEvent(const int _idx) const;
    void RemoveEvent(const int _idx);

    void Run();
    bool RunOnce();

    void SetEventQueue(EventQueue* _value) { m_EventQueue = _value; }
  }; // EventRunner


  //-------------------------------------------------- EventInterpreter

  class EventInterpreter : public Thread {
  private:
    vector<EventSubscription*> m_Subscriptions;
    vector<EventInterpreterPlugin*> m_Plugins;
    EventQueue* m_Queue;
    EventRunner* m_EventRunner;
    int m_EventsProcessed;
  private:
    void LoadSubscription(XMLNode& _node);
    EventInterpreterPlugin* GetPluginByName(const string& _name);
  public:
    EventInterpreter(EventQueue* _queue);
    EventInterpreter();
    virtual ~EventInterpreter();

    void AddPlugin(EventInterpreterPlugin* _plugin);

    void Subscribe(EventSubscription* _subscription);
    void Unsubscribe(const string& _subscriptionID);

    virtual void Execute();

    void LoadFromXML(const string& _fileName);

    int GetEventsProcessed() const { return m_EventsProcessed; }
    EventQueue& GetQueue() { return *m_Queue; }
    void SetEventQueue(EventQueue* _queue) { m_Queue = _queue; }
    EventRunner& GetEventRunner() { return *m_EventRunner; }
    void SetEventRunner(EventRunner* _runner) { m_EventRunner = _runner; }
    int GetNumberOfSubscriptions() { return m_Subscriptions.size(); }
  }; // EventInterpreter


  //-------------------------------------------------- ScheduledEvent

  /** Combines an Event with a Schedule
    * These events get raised according to their Schedule
   */
  class ScheduledEvent {
  private:
    Event* m_Event;
    Schedule* m_Schedule;
    string m_Name;
    bool m_OwnsEvent;
  public:
    ScheduledEvent(Event* _pEvt, Schedule* _pSchedule)
    : m_Event(_pEvt), m_Schedule(_pSchedule), m_OwnsEvent(true) {};
    ~ScheduledEvent();

    /** Returns the event that will be raised */
    Event* GetEvent() { return m_Event; };
    /** Returns the associated Schedule */
    Schedule& GetSchedule() const { return *m_Schedule; };
    /** Returns the name of this ScheduledEvent */
    const string& GetName() const { return m_Name; };
    /** Sets the name of this ScheduledEvent */
    void SetName(const string& _value) { m_Name = _value; };

    void SetOwnsEvent(const bool _value) { m_OwnsEvent = _value; }
  };


  //================================================== Constants

  extern const char* EventPropertyName;
  extern const char* EventPropertyLocation;
  extern const char* EventPropertyContext;
  extern const char* EventPropertyTime;

} // namespace dss

#endif /* EVENT_H_ */
