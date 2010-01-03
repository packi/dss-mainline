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

  class Apartment;
  
  class EventInterpreterPluginRaiseEvent : public EventInterpreterPlugin {
  private:
    void applyOptionsWithSuffix(const SubscriptionOptions& _options, const std::string& _suffix, boost::shared_ptr<Event> _event);
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
    Apartment& m_Apartment;
    std::string getParameter(Poco::XML::Node* _node, const std::string& _parameterName);
  public:
    EventInterpreterPluginDS485(Apartment& _apartment, DS485Interface* _pInterface, EventInterpreter* _pInterpreter);

    virtual SubscriptionOptions* createOptionsFromXML(Poco::XML::Node* _node);

    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  }; // EventInterpreterPluginDS485


  //-------------------------------------------------- Event Relay

  class EventInterpreterInternalRelay;

  class EventRelayTarget {
  public:
    EventRelayTarget(EventInterpreterInternalRelay& _relay)
    : m_Relay(_relay)
    { } // ctor
    virtual ~EventRelayTarget();

    virtual void handleEvent(Event& _event, const EventSubscription& _subscription) = 0;
    virtual void subscribeTo(boost::shared_ptr<EventSubscription> _pSubscription);
    virtual void unsubscribeFrom(const std::string& _subscriptionID);
  private:
    EventInterpreterInternalRelay& m_Relay;
    std::vector<std::string> m_SubscriptionIDs;
  }; // EventRelayTarget

  class EventInterpreterInternalRelay : public EventInterpreterPlugin {
  public:
    EventInterpreterInternalRelay(EventInterpreter* _pInterpreter);
    virtual ~EventInterpreterInternalRelay();

    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);

    static const char* getPluginName() { return "internal_relay"; }
  protected:
    friend class EventRelayTarget;
    void registerSubscription(EventRelayTarget* _pTarget, const std::string& _subscriptionID);
    void removeSubscription(const std::string& _subscriptionID);
  private:
    HASH_NAMESPACE::hash_map<const std::string, EventRelayTarget*> m_IDTargetMap;
  }; // EventInterpreterInternalRelay

  class EventInterpreterPluginEmail : public EventInterpreterPlugin {
   private:
   public:
       EventInterpreterPluginEmail(EventInterpreter* _pInterpreter);

     virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
   }; // EventInterpreterPluginEmail

} // namespace dss

#endif /* EVENTINTERPRETERPLUGINS_H_ */
