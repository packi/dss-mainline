/*
 * eventinterpreterplugins.h
 *
 *  Created on: Nov 10, 2008
 *      Author: patrick
 */

#ifndef EVENTINTERPRETERPLUGINS_H_
#define EVENTINTERPRETERPLUGINS_H_

#include "event.h"
#include "jshandler.h"

namespace dss {
  class EventInterpreterPluginRaiseEvent : public EventInterpreterPlugin {
  public:
    EventInterpreterPluginRaiseEvent(EventInterpreter* _pInterpreter);
    virtual ~EventInterpreterPluginRaiseEvent();

    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  }; // EventInterpreterPluginRaiseEvent

  class EventInterpreterPluginJavascript : public EventInterpreterPlugin {
  private:
    ScriptEnvironment m_Environment;
  public:
    EventInterpreterPluginJavascript(EventInterpreter* _pInterpreter);

    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  }; // EventInterpreterPluginJavascript

  class DS485Interface;

  class EventInterpreterPluginDS485 : public EventInterpreterPlugin {
  private:
    DS485Interface* m_pInterface;
    string getParameter(XMLNodeList& _nodes, const string& _parameterName);
  public:
    EventInterpreterPluginDS485(DS485Interface* _pInterface, EventInterpreter* _pInterpreter);

    virtual SubscriptionOptions* createOptionsFromXML(XMLNodeList& _nodes);

    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  }; // EventInterpreterPluginDS485

} // namespace dss

#endif /* EVENTINTERPRETERPLUGINS_H_ */
