/*
 * eventinterpreterplugins.h
 *
 *  Created on: Nov 10, 2008
 *      Author: patrick
 */

#ifndef EVENTINTERPRETERPLUGINS_H_
#define EVENTINTERPRETERPLUGINS_H_

#include "event.h"

namespace dss {
  class EventInterpreterPluginRaiseEvent : public EventInterpreterPlugin {
  public:
    EventInterpreterPluginRaiseEvent(EventInterpreter* _pInterpreter);
    ~EventInterpreterPluginRaiseEvent();

    virtual void HandleEvent(Event& _event, const EventSubscription& _subscription);
  };

  class EventInterpreterPluginJavascript : public EventInterpreterPlugin {
  };
} // namespace dss

#endif /* EVENTINTERPRETERPLUGINS_H_ */
