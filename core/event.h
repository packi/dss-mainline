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

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

using std::string;
using std::queue;

namespace dss {

  //================================================== Forward declarations

  class EventInterpreter;
  class ScheduledEvent;
  class EventRunner;

  //================================================== Class definitions

  //-------------------------------------------------- Event

  typedef enum {
    erlZone,
    erlDevice,
    erlApartment
  } EventRaiseLocation;

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

    Properties m_Properties;
  private:
    void reset();
  public:
    Event(const string& _name, Zone* _context);
    Event(const string& _name, DeviceReference* _ref);
    Event(const string& _name);

    const string& getName() const { return m_Name; }

    const string& getPropertyByName(const string& _name) const;
    bool hasPropertySet(const string& _name) const;
    void unsetProperty(const string& _name);
    bool setProperty(const string& _name, const string& _value);

    void setLocation(const string& _value) { m_Location = _value; m_LocationSet = true; }
    void setContext(const string& _value) { m_Context = _value; m_ContextSet = true; }
    void setTime(const string& _value) { m_Time = _value; m_TimeSet = true; }

    const Zone& getRaisedAtZone() const;
    const DeviceReference& getRaisedAtDevice() const { return *m_RaisedAtDevice; }

    const Properties& getProperties() const { return m_Properties; }
    void setProperties(const Properties& _value) { m_Properties = _value; }
  }; // Event


  //-------------------------------------------------- SubscriptionOptions

  class SubscriptionOptions {
  private:
    Properties m_Parameters;
  public:
    SubscriptionOptions();
    virtual ~SubscriptionOptions();

    const string& getParameter(const string& _name) const;
    void setParameter(const string& _name, const string& _value);
    bool hasParameter(const string& _name) const;

    void loadParameterFromXML(XMLNode& _node);
  }; // SubscriptionOptions


  //-------------------------------------------------- EventPropertyFilter

  class EventPropertyFilter {
  private:
    string m_PropertyName;
  public:
    EventPropertyFilter(const string& _propertyName);
    virtual ~EventPropertyFilter() {}

    const string& getPropertyName() const { return m_PropertyName; }

    virtual bool matches(const Event& _event) = 0;
  }; // EventPropertyFilter


  //-------------------------------------------------- EventPropertyMatchFilter

  class EventPropertyMatchFilter : public EventPropertyFilter {
  private:
    string m_Value;
  public:
    EventPropertyMatchFilter(const string& _propertyName, const string& _value)
    : EventPropertyFilter(_propertyName), m_Value(_value) {}

    virtual ~EventPropertyMatchFilter() {}

    virtual bool matches(const Event& _event);
  }; // EventPropertyMatchFilter


  //-------------------------------------------------- EventPropertyExistsFilter

  class EventPropertyExistsFilter : public EventPropertyFilter {
  public:
    EventPropertyExistsFilter(const string& _propertyName);
    virtual ~EventPropertyExistsFilter() {};

    virtual bool matches(const Event& _event);
  }; // EventPropertyExistsFilter


  //-------------------------------------------------- EventPropertyMissingFilter

  class EventPropertyMissingFilter : public EventPropertyFilter {
  public:
    EventPropertyMissingFilter(const string& _propertyName);
    virtual ~EventPropertyMissingFilter() {};

    virtual bool matches(const Event& _event);
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
    string m_EventName;
    string m_HandlerName;
    string m_ID;
    boost::shared_ptr<SubscriptionOptions> m_SubscriptionOptions;
    boost::ptr_vector<EventPropertyFilter> m_Filter;
    EventPropertyFilterOption m_FilterOption;
  protected:
    void initialize();
  public:
    EventSubscription(const string& _eventName, const string& _handlerName, boost::shared_ptr<SubscriptionOptions> _options);
    EventSubscription(const string& _eventName, const string& _handlerName, const string& _id, boost::shared_ptr<SubscriptionOptions> _options);

    ~EventSubscription();

    void setEventName(const string& _value) { m_EventName = _value; }
    const string& getEventName() const { return m_EventName; }
    void setHandlerName(const string& _value) { m_HandlerName = _value; }
    const string& getHandlerName() const { return m_HandlerName; }
    const string& getID() const { return m_ID; }

    SubscriptionOptions& getOptions() { return *m_SubscriptionOptions.get(); }
    const SubscriptionOptions& getOptions() const { return *m_SubscriptionOptions.get(); }

    void addPropertyFilter(EventPropertyFilter* _pPropertyFilter);

    void setFilterOption(const EventPropertyFilterOption _value) { m_FilterOption = _value; }
    bool matches(Event& _event);
  }; // EventSubscription


  //-------------------------------------------------- EventInterpreterPlugin

  class EventInterpreterPlugin {
  private:
    string m_Name;
    EventInterpreter* const m_pInterpreter;
  protected:
    EventInterpreter& getEventInterpreter() { return *m_pInterpreter; }
  public:
    EventInterpreterPlugin(const string& _name, EventInterpreter* _interpreter);
    virtual ~EventInterpreterPlugin() {}

    const string& getName() const { return m_Name; }
    virtual void handleEvent(Event& _event, const EventSubscription& _subscription) = 0;

    virtual SubscriptionOptions* createOptionsFromXML(XMLNodeList& _nodes);
  }; // EventInterpreterPlugin


  //-------------------------------------------------- EventQueue

  class EventQueue {
  private:
    queue< boost::shared_ptr<Event> > m_EventQueue;
    SyncEvent m_EntryInQueueEvt;
    Mutex m_QueueMutex;

    EventRunner* m_EventRunner;
  public:
    EventQueue();
    void pushEvent(boost::shared_ptr<Event> _event);
    boost::shared_ptr<Event> popEvent();
    bool waitForEvent();

    void shutdown();

    void setEventRunner(EventRunner* _value) { m_EventRunner = _value; }
  };


  //-------------------------------------------------- EventRunner

  class EventRunner {
  private:
    boost::ptr_vector<ScheduledEvent> m_ScheduledEvents;

    DateTime getNextOccurence();
    DateTime m_WakeTime;
    SyncEvent m_NewItem;
    EventQueue* m_EventQueue;
  public:
    EventRunner();

    void addEvent(ScheduledEvent* _scheduledEvent);

    bool raisePendingEvents(DateTime& _from, int _deltaSeconds);

    int getSize() const;
    const ScheduledEvent& getEvent(const int _idx) const;
    void removeEvent(const int _idx);

    void run();
    bool runOnce();

    void setEventQueue(EventQueue* _value) { m_EventQueue = _value; }
  }; // EventRunner


  //-------------------------------------------------- EventInterpreter

  class EventInterpreter : public Subsystem,
                           public Thread {
  private:
    vector< boost::shared_ptr<EventSubscription> > m_Subscriptions;
    vector<EventInterpreterPlugin*> m_Plugins;
    EventQueue* m_Queue;
    EventRunner* m_EventRunner;
    int m_EventsProcessed;
  private:
    void loadSubscription(XMLNode& _node);
    void loadFilter(XMLNode& _node, EventSubscription& _subscription);
    EventInterpreterPlugin* getPluginByName(const string& _name);
  protected:
    virtual void doStart();
  public:
    EventInterpreter(DSS* _pDSS);
    virtual ~EventInterpreter();

    virtual void initialize();

    virtual void execute();

    void addPlugin(EventInterpreterPlugin* _plugin);

    void subscribe(boost::shared_ptr<EventSubscription> _subscription);
    void unsubscribe(const string& _subscriptionID);

    void loadFromXML(const string& _fileName);

    int getEventsProcessed() const { return m_EventsProcessed; }
    EventQueue& getQueue() { return *m_Queue; }
    void setEventQueue(EventQueue* _queue) { m_Queue = _queue; }
    EventRunner& getEventRunner() { return *m_EventRunner; }
    void setEventRunner(EventRunner* _runner) { m_EventRunner = _runner; }
    int getNumberOfSubscriptions() { return m_Subscriptions.size(); }
  }; // EventInterpreter


  //-------------------------------------------------- ScheduledEvent

  /** Combines an Event with a Schedule
    * These events get raised according to their Schedule
   */
  class ScheduledEvent {
  private:
    boost::shared_ptr<Event> m_Event;
    Schedule* m_Schedule;
    string m_Name;
    bool m_OwnsEvent;
  public:
    ScheduledEvent(boost::shared_ptr<Event> _pEvt, Schedule* _pSchedule)
    : m_Event(_pEvt), m_Schedule(_pSchedule), m_OwnsEvent(true) {};
    ~ScheduledEvent();

    /** Returns the event that will be raised */
    boost::shared_ptr<Event> getEvent() { return m_Event; };
    /** Returns the associated Schedule */
    Schedule& getSchedule() const { return *m_Schedule; };
    /** Returns the name of this ScheduledEvent */
    const string& getName() const { return m_Name; };
    /** Sets the name of this ScheduledEvent */
    void setName(const string& _value) { m_Name = _value; };

    void setOwnsEvent(const bool _value) { m_OwnsEvent = _value; }
  };


  //================================================== Constants

  extern const char* EventPropertyName;
  extern const char* EventPropertyLocation;
  extern const char* EventPropertyContext;
  extern const char* EventPropertyTime;

} // namespace dss

#endif /* EVENT_H_ */
