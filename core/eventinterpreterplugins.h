/*
 * eventinterpreterplugins.h
 *
 *  Created on: Nov 10, 2008
 *      Author: patrick
 */

#ifndef EVENTINTERPRETERPLUGINS_H_
#define EVENTINTERPRETERPLUGINS_H_

#include "event.h"
#include "DS485Interface.h"

namespace dss {
  class EventInterpreterPluginRaiseEvent : public EventInterpreterPlugin {
  public:
    EventInterpreterPluginRaiseEvent(EventInterpreter* _pInterpreter);
    virtual ~EventInterpreterPluginRaiseEvent();

    virtual void HandleEvent(Event& _event, const EventSubscription& _subscription);
  }; // EventInterpreterPluginRaiseEvent

  class EventInterpreterPluginJavascript : public EventInterpreterPlugin {
  }; // EventInterpreterPluginJavascript

  class EventInterpreterPluginDS485 : public EventInterpreterPlugin {
  private:
    DS485Interface* m_pInterface;
    string GetParameter(XMLNodeList& _nodes, const string& _parameterName);
  public:
    EventInterpreterPluginDS485(DS485Interface* _pInterface, EventInterpreter* _pInterpreter);

    virtual SubscriptionOptions* CreateOptionsFromXML(XMLNodeList& _nodes);

    virtual void HandleEvent(Event& _event, const EventSubscription& _subscription);
  }; // EventInterpreterPluginDS485

} // namespace dss

#endif /* EVENTINTERPRETERPLUGINS_H_ */
