/*
 * eventinterpreterplugins.cpp
 *
 *  Created on: Nov 10, 2008
 *      Author: patrick
 */


#include "eventinterpreterplugins.h"

namespace dss {


  //================================================== EventInterpreterPluginRaiseEvent

  EventInterpreterPluginRaiseEvent::EventInterpreterPluginRaiseEvent(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("raise_event", _pInterpreter)
  { } // ctor

  EventInterpreterPluginRaiseEvent::~EventInterpreterPluginRaiseEvent()
  { } // dtor

  void EventInterpreterPluginRaiseEvent::HandleEvent(Event& _event, const EventSubscription& _subscription) {
    Event* newEvent = new Event(_subscription.GetOptions().GetParameter("event_name"));
    GetEventInterpreter().GetQueue().PushEvent(newEvent);
  } // HandleEvent

} // namespace dss
