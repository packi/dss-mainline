/*
 * eventinterpreterplugins.cpp
 *
 *  Created on: Nov 10, 2008
 *      Author: patrick
 */


#include "eventinterpreterplugins.h"

#include "logger.h"

namespace dss {


  //================================================== EventInterpreterPluginRaiseEvent

  EventInterpreterPluginRaiseEvent::EventInterpreterPluginRaiseEvent(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("raise_event", _pInterpreter)
  { } // ctor

  EventInterpreterPluginRaiseEvent::~EventInterpreterPluginRaiseEvent()
  { } // dtor

  void EventInterpreterPluginRaiseEvent::HandleEvent(Event& _event, const EventSubscription& _subscription) {
    Event* newEvent = new Event(_subscription.GetOptions().GetParameter("event_name"));
    if(_subscription.GetOptions().HasParameter("time")) {
      string timeParam = _subscription.GetOptions().GetParameter("time");
      if(timeParam.size() > 0) {
        Logger::GetInstance()->Log("RaiseEvent: Event has time");
        newEvent->SetTime(timeParam);
      }
    }
    GetEventInterpreter().GetQueue().PushEvent(newEvent);
  } // HandleEvent

} // namespace dss
