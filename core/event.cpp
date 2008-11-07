/*
 * event.cpp
 *
 *  Created on: Oct 24, 2008
 *      Author: patrick
 */

#include "event.h"

#include "base.h"

namespace dss {

  //================================================== Event

  Event::Event(const string& _name)
  : m_Name(_name)
  {} // ctor

  string Event::GetPropertyByName(const string& _name) const {
    if(_name == EventPropertyName) {
      return m_Name;
    } else if(_name == EventPropertyLocation) {
      return m_Location;
    } else if(_name == EventPropertyContext) {
      return m_Context;
    }
    return string();
  }

  bool Event::HasPropertySet(const string& _name) const {
    if(_name == EventPropertyName) {
      return true;
    } else if(_name == EventPropertyLocation) {
      return m_LocationSet;
    } else if(_name == EventPropertyContext) {
      return m_ContextSet;
    }
    return false;
  }


  //================================================== EventInterpreter

  EventInterpreter::EventInterpreter(EventQueue* _queue)
  : m_Queue(_queue)
  { } // ctor

  EventInterpreter::~EventInterpreter() {
    ScrubVector(m_Plugins);
    ScrubVector(m_Subscriptions);
  } // dtor

  void EventInterpreter::AddPlugin(EventInterpreterPlugin* _plugin) {
    m_Plugins.push_back(_plugin);
  } // AddPlugin

  void EventInterpreter::Execute() {
    Event* toProcess = NULL;
    while(!m_Terminated && m_Queue->WaitForEvent()) {
      toProcess = m_Queue->PopEvent();
      vector<string> handlersToCall;
      for(vector<EventSubscription*>::iterator ipSubscription = m_Subscriptions.begin(), e = m_Subscriptions.end();
          ipSubscription != e; ++ipSubscription)
      {
        if((*ipSubscription)->Matches(*toProcess)) {
          handlersToCall.push_back((*ipSubscription)->GetHandlerName());
        }
      }
      delete toProcess;
    }
  } // Execute

  void EventInterpreter::Subscribe(EventSubscription* _subscription) {
    m_Subscriptions.push_back(_subscription);
  } // Subscribe

  void EventInterpreter::Unsubscribe(const string& _subscriptionID) {
    for(vector<EventSubscription*>::iterator ipSubscription = m_Subscriptions.begin(), e = m_Subscriptions.end();
        ipSubscription != e; ++ipSubscription)
    {
      if((*ipSubscription)->GetID() == _subscriptionID) {
        m_Subscriptions.erase(ipSubscription);
        break;
      }
    }
  } // Unsubscribe


  //================================================== EventQueue

  void EventQueue::PushEvent(Event* _event) {
    m_QueueMutex.Lock();
    m_EventQueue.push(_event);
    m_QueueMutex.Unlock();
    m_EntryInQueueEvt.Signal();
  } // PushEvent

  Event* EventQueue::PopEvent() {
    m_QueueMutex.Lock();
    Event* result = m_EventQueue.front();
    m_EventQueue.pop();
    m_QueueMutex.Unlock();
    return result;
  } // PopEvent

  bool EventQueue::WaitForEvent() {
    if(m_EventQueue.empty()) {
      m_EntryInQueueEvt.WaitFor();
    }
    return !m_EventQueue.empty();
  }

  //================================================== EventSubscription

  EventSubscription::EventSubscription(const string& _eventName, const string _handlerName, SubscriptionOptions* _options)
  : m_EventName(_eventName),
    m_HandlerName(_handlerName),
    m_ID(_eventName + _handlerName),
    m_SubscriptionOptions(_options)
  {
  } // ctor

  EventSubscription::EventSubscription(const string& _eventName, const string _handlerName, const string& _id, SubscriptionOptions* _options)
  : m_EventName(_eventName),
    m_HandlerName(_handlerName),
    m_ID(_id),
    m_SubscriptionOptions(_options)
  {
  } // ctor(with id)

  void EventSubscription::AddPropertyFilter(EventPropertyFilter* _pPropertyFilter) {
    m_Filter.push_back(_pPropertyFilter);
  }

  bool EventSubscription::Matches(Event& _event) {
    for(vector<EventPropertyFilter*>::iterator ipFilter = m_Filter.begin(), e = m_Filter.end();
        ipFilter != e; ++ipFilter)
    {
      if((*ipFilter)->Matches(_event)) {
        if(m_FilterOption == foMatchOne) {
          return true;
        } else if(m_FilterOption == foMatchNone) {
          return false;
        }
      } else if(m_FilterOption == foMatchAll) {
        return false;
      }
    }
    return true;
  } // Matches


  //================================================== EventPropertyMatchFilter

  bool EventPropertyMatchFilter::Matches(const Event& _event) {
    if(_event.HasPropertySet(GetPropertyName())) {
      return _event.GetPropertyByName(GetPropertyName()) == m_Value;
    }
    return false;
  } // Matches

  //================================================== EventPropertyExistsFilter

  bool EventPropertyExistsFilter::Matches(const Event& _event) {
    return _event.HasPropertySet(GetPropertyName());
  } // Matches

  //================================================= EventPropertyMissingFilter

  bool EventPropertyMissingFilter::Matches(const Event& _event) {
    return !_event.HasPropertySet(GetPropertyName());
  }

  //================================================== External consts

  const char* EventPropertyName = "name";
  const char* EventPropertyLocation = "location";
  const char* EventPropertyContext = "context";

} // namespace dss
