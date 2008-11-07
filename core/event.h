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

#include <string>
#include <queue>

using std::string;
using std::queue;

namespace dss {

  extern const char* EventPropertyName;
  extern const char* EventPropertyLocation;
  extern const char* EventPropertyContext;

  class Event {
  private:
    string m_Name;
    string m_Location;
    bool m_LocationSet;
    string m_Context;
    bool m_ContextSet;
  public:
    Event(const string& _name);

    string GetPropertyByName(const string& _name) const;
    bool HasPropertySet(const string& _name) const;

    void SetLocation(const string& _value) { m_Location = _value; m_LocationSet = true; }
    void SetContext(const string& _value) { m_Context = _value; m_ContextSet = true; }
  }; // Event


  class SubscriptionOptions {
  private:
    HashMapConstStringString m_Parameters;
  public:
    SubscriptionOptions();
    virtual ~SubscriptionOptions();
  }; // SubscriptionOptions

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

  class EventPropertyMatchFilter : public EventPropertyFilter {
  private:
    string m_Value;
  public:
    EventPropertyMatchFilter(const string& _propertyName, const string& _value)
    : EventPropertyFilter(_propertyName), m_Value(_value) {}

    virtual ~EventPropertyMatchFilter() {}

    virtual bool Matches(const Event& _event);
  }; // EventPropertyMatchFilter

  class EventPropertyExistsFilter : public EventPropertyFilter {
  public:
    virtual ~EventPropertyExistsFilter() {}
    virtual bool Matches(const Event& _event);
  }; // EventPropertyExistsFilter

  class EventPropertyMissingFilter : public EventPropertyFilter {
  public:
    virtual ~EventPropertyMissingFilter() {}
    virtual bool Matches(const Event& _event);
  }; // EventPropertyMissingFilter

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
  public:
    EventSubscription(const string& _eventName, const string _handlerName, SubscriptionOptions* _options);
    EventSubscription(const string& _eventName, const string _handlerName, const string& _id, SubscriptionOptions* _options);


    void SetEventName(const string& _value) { m_EventName = _value; }
    const string& GetEventName() const { return m_EventName; }
    void SetHandlerName(const string& _value) { m_HandlerName = _value; }
    const string& GetHandlerName() const { return m_HandlerName; }
    const string& GetID() const { return m_ID; }

    void AddPropertyFilter(EventPropertyFilter* _pPropertyFilter);

    void SetFilterOption(const EventPropertyFilterOption _value) { m_FilterOption = _value; }
    bool Matches(Event& _event);
  }; // EventSubscription

  class EventInterpreterPlugin {
  private:
    string m_Name;
  public:
    EventInterpreterPlugin(const string& _name);

    const string& GetName() const { return m_Name; }
    void HandleEvent(Event& _event);
  }; // EventInterpreterPlugin

  class EventQueue {
  private:
    queue<Event*> m_EventQueue;
    SyncEvent m_EntryInQueueEvt;
    Mutex m_QueueMutex;
  public:
    void PushEvent(Event* _event);
    Event* PopEvent();
    bool WaitForEvent();
  };

  class EventInterpreter : public Thread {
  private:
    vector<EventSubscription*> m_Subscriptions;
    vector<EventInterpreterPlugin*> m_Plugins;
    EventQueue* m_Queue;
  public:
    EventInterpreter(EventQueue* _queue);
    virtual ~EventInterpreter();

    void AddPlugin(EventInterpreterPlugin* _plugin);

    void Subscribe(EventSubscription* _subscription);
    void Unsubscribe(const string& _subscriptionID);

    virtual void Execute();
  }; // EventInterpreter

  /** Combines an Event with a Schedule
    * These events get raised according to their Schedule
   */
  class ScheduledEvent {
  private:
    boost::shared_ptr<Event> m_Event;
    boost::shared_ptr<Schedule> m_Schedule;
    string m_Name;
  public:
    ScheduledEvent(boost::shared_ptr<Event> _evt, boost::shared_ptr<Schedule> _schedule)
    : m_Event(_evt), m_Schedule(_schedule) {};

    /** Returns the event that will be raised */
    Event& GetEvent() const { return *m_Event; };
    /** Returns the associated Schedule */
    Schedule& GetSchedule() const { return *m_Schedule; };
    /** Returns the name of this ScheduledEvent */
    const string& GetName() const { return m_Name; };
    /** Sets the name of this ScheduledEvent */
    void SetName(const string& _value) { m_Name = _value; };
  };

} // namespace dss

#endif /* EVENT_H_ */
