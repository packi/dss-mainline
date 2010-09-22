/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#ifndef EVENT_H_
#define EVENT_H_

#include "base.h"
#include "datetools.h"
#include "thread.h"
#include "syncevent.h"
#include "subsystem.h"

#include <string>
#include <deque>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/thread/mutex.hpp>

namespace Poco {
  namespace XML {
    class Node;
  }
}

namespace dss {

  //================================================== Forward declarations

  class EventInterpreter;
  class ScheduledEvent;
  class EventRunner;
  class Zone;
  class DeviceReference;

  //================================================== Class definitions

  //-------------------------------------------------- Event

  typedef enum {
    erlZone,
    erlDevice,
    erlApartment
  } EventRaiseLocation;

  class Event {
  private:
    std::string m_Name;
    std::string m_Location;
    bool m_LocationSet;
    std::string m_Context;
    bool m_ContextSet;
    std::string m_Time;
    bool m_TimeSet;
    EventRaiseLocation m_RaiseLocation;
    Zone* m_RaisedAtZone;
    DeviceReference* m_RaisedAtDevice;

    Properties m_Properties;
  private:
    void reset();
  public:
    Event(const std::string& _name, Zone* _context);
    Event(const std::string& _name, DeviceReference* _ref);
    Event(const std::string& _name);
    Event();

    ~Event();

    const std::string& getName() const { return m_Name; }

    const std::string& getPropertyByName(const std::string& _name) const;
    bool hasPropertySet(const std::string& _name) const;
    void unsetProperty(const std::string& _name);
    bool setProperty(const std::string& _name, const std::string& _value);

    void setLocation(const std::string& _value) { m_Location = _value; m_LocationSet = true; }
    void setContext(const std::string& _value) { m_Context = _value; m_ContextSet = true; }
    void setTime(const std::string& _value) { m_Time = _value; m_TimeSet = true; }

    const Zone& getRaisedAtZone() const;
    const DeviceReference& getRaisedAtDevice() const { return *m_RaisedAtDevice; }

    const Properties& getProperties() const { return m_Properties; }
    void setProperties(const Properties& _value) { m_Properties = _value; }
    void applyProperties(const Properties& _others);
    /** Checks whether _other and this are the same regarding uniqueness */
    bool isReplacementFor(const Event& _other);
  }; // Event


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

    void loadParameterFromXML(Poco::XML::Node* _node);

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

    virtual bool matches(const Event& _event) = 0;
  }; // EventPropertyFilter


  //-------------------------------------------------- EventPropertyMatchFilter

  class EventPropertyMatchFilter : public EventPropertyFilter {
  private:
    std::string m_Value;
  public:
    EventPropertyMatchFilter(const std::string& _propertyName, const std::string& _value)
    : EventPropertyFilter(_propertyName), m_Value(_value) {}

    virtual ~EventPropertyMatchFilter() {}

    virtual bool matches(const Event& _event);
  }; // EventPropertyMatchFilter


  //-------------------------------------------------- EventPropertyExistsFilter

  class EventPropertyExistsFilter : public EventPropertyFilter {
  public:
    EventPropertyExistsFilter(const std::string& _propertyName);
    virtual ~EventPropertyExistsFilter() {};

    virtual bool matches(const Event& _event);
  }; // EventPropertyExistsFilter


  //-------------------------------------------------- EventPropertyMissingFilter

  class EventPropertyMissingFilter : public EventPropertyFilter {
  public:
    EventPropertyMissingFilter(const std::string& _propertyName);
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

    void addPropertyFilter(EventPropertyFilter* _pPropertyFilter);

    void setFilterOption(const EventPropertyFilterOption _value) { m_FilterOption = _value; }
    bool matches(Event& _event);
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
    virtual void handleEvent(Event& _event, const EventSubscription& _subscription) = 0;

    virtual boost::shared_ptr<SubscriptionOptions> createOptionsFromXML(Poco::XML::Node* _node);
  }; // EventInterpreterPlugin


  //-------------------------------------------------- EventQueue

  class EventQueue {
  private:
    std::deque< boost::shared_ptr<Event> > m_EventQueue;
    SyncEvent m_EntryInQueueEvt;
    boost::mutex m_QueueMutex;

    EventRunner* m_EventRunner;
    boost::shared_ptr<Schedule> scheduleFromEvent(boost::shared_ptr<Event> _event);
    const int m_EventTimeoutMS;
  public:
    EventQueue(const int _eventTimeoutMS = 1000);
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
    boost::mutex m_EventsMutex;
    bool m_ShutdownFlag;
  public:
    EventRunner();

    void addEvent(ScheduledEvent* _scheduledEvent);

    bool raisePendingEvents(DateTime& _from, int _deltaSeconds);

    int getSize() const;
    const ScheduledEvent& getEvent(const int _idx) const;
    void removeEvent(const int _idx);

    void run();
    bool runOnce();
    void shutdown();

    void setEventQueue(EventQueue* _value) { m_EventQueue = _value; }
  }; // EventRunner


  //-------------------------------------------------- EventInterpreter

  class EventInterpreter : public Subsystem,
                           public Thread {
  private:
    typedef std::vector< boost::shared_ptr<EventSubscription> > SubscriptionVector;
    SubscriptionVector m_Subscriptions;
    boost::mutex m_SubscriptionsMutex;
    std::vector<EventInterpreterPlugin*> m_Plugins;
    EventQueue* m_Queue;
    EventRunner* m_EventRunner;
    int m_EventsProcessed;
  private:
    void loadSubscription(Poco::XML::Node* _node);
    void loadFilter(Poco::XML::Node* _node, EventSubscription& _subscription);
    void loadPropertyFilter(Poco::XML::Node* _pNode, EventSubscription& _subscription);
    boost::shared_ptr<EventSubscription> subscriptionByID(const std::string& _name);
  protected:
    virtual void doStart();
  public:
    EventInterpreter(DSS* _pDSS);
    virtual ~EventInterpreter();

    virtual void initialize();

    virtual void execute();

    void addPlugin(EventInterpreterPlugin* _plugin);
    EventInterpreterPlugin* getPluginByName(const std::string& _name);

    void subscribe(boost::shared_ptr<EventSubscription> _subscription);
    void unsubscribe(const std::string& _subscriptionID);

    void loadFromXML(const std::string& _fileName);
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
  public:
    ScheduledEvent(boost::shared_ptr<Event> _pEvt, boost::shared_ptr<Schedule> _pSchedule)
    : m_Event(_pEvt), m_Schedule(_pSchedule) {};

    /** Returns the event that will be raised */
    boost::shared_ptr<Event> getEvent() { return m_Event; }
    const boost::shared_ptr<Event> getEvent() const { return m_Event; }
    /** Returns the associated Schedule */
    Schedule& getSchedule() const { return *m_Schedule; }
    /** Returns the name of this ScheduledEvent */
    const std::string& getName() const { return m_Name; }
    /** Sets the name of this ScheduledEvent */
    void setName(const std::string& _value) { m_Name = _value; }
  }; // ScheduledEvent


  //================================================== Constants

  extern const char* EventPropertyName;
  extern const char* EventPropertyLocation;
  extern const char* EventPropertyContext;
  extern const char* EventPropertyTime;
  extern const char* EventPropertyICalStartTime;
  extern const char* EventPropertyICalRRule;

} // namespace dss

#endif /* EVENT_H_ */
