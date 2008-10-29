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

#include <string>
#include <queue>

using std::string;
using std::queue;

namespace dss {

  class Event {
  private:
    string m_Name;
  public:
    Event(const string& _name);
  }; // Event


  class SubscriptionOptions {
  private:
    HashMapConstStringString m_Parameters;
  public:
    SubscriptionOptions();
    virtual ~SubscriptionOptions();
  }; // SubscriptionOptions

  class EventSubscription {
  private:
    string m_EventName;
    string m_HandlerName;
    string m_ID;
    SubscriptionOptions* m_SubscriptionOptions;
  public:
    EventSubscription(const string& _eventName, const string _handlerName, SubscriptionOptions* _options);
    EventSubscription(const string& _eventName, const string _handlerName, const string& _id, SubscriptionOptions* _options);


    void SetEventName(const string& _value) { m_EventName = _value; }
    const string& GetEventName() const { return m_EventName; }
    void SetHandlerName(const string& _value) { m_HandlerName = _value; }
    const string& GetHandlerName() const { return m_HandlerName; }
    const string& GetID() const { return m_ID; }
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
  public:
    void PushEvent(Event* _event);
    Event* PopEvent();
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
