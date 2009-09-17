/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#ifndef EVENTINTERPRETERPLUGINS_H_
#define EVENTINTERPRETERPLUGINS_H_

#include "event.h"

#include <boost/shared_ptr.hpp>

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
    std::vector<boost::shared_ptr<ScriptContext> > m_KeptContexts;
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
