/*
    Copyright (c) 2009,2010 digitalSTROM.org, Zurich, Switzerland

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

#ifndef EVENTINTERPRETERPLUGINS_H_
#define EVENTINTERPRETERPLUGINS_H_

#include "event.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "src/scripting/jshandler.h"

namespace dss {

  class Apartment;
  class InternalEventRelayTarget;

  class EventInterpreterPluginRaiseEvent : public EventInterpreterPlugin {
  private:
    void applyOptionsWithSuffix(boost::shared_ptr<const SubscriptionOptions> _options, const std::string& _suffix, boost::shared_ptr<Event> _event, bool _onlyOverride);
  public:
    EventInterpreterPluginRaiseEvent(EventInterpreter* _pInterpreter);
    virtual ~EventInterpreterPluginRaiseEvent();

    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  }; // EventInterpreterPluginRaiseEvent

  class ScriptContextWrapper;
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;

  class EventInterpreterPluginJavascript : public EventInterpreterPlugin {
  private:
    boost::shared_ptr<ScriptEnvironment> m_pEnvironment;
    boost::weak_ptr<ScriptContextWrapper> m_WrapperInAction;
    std::vector<boost::shared_ptr<ScriptContextWrapper> > m_WrappedContexts;
    boost::shared_ptr<InternalEventRelayTarget> m_pRelayTarget;
    static const std::string kCleanupScriptsEventName;
    PropertyNodePtr m_pScriptRootNode;
  private:
    void initializeEnvironment();
    void setupCleanupEvent();
    void cleanupTerminatedScripts(Event& _event, const EventSubscription& _subscription);
    void sendCleanupEvent();
  public:
    EventInterpreterPluginJavascript(EventInterpreter* _pInterpreter);
    ~EventInterpreterPluginJavascript();

    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);

    boost::shared_ptr<ScriptContextWrapper> getContextWrapperForContext(ScriptContext* _pContext);
  }; // EventInterpreterPluginJavascript

  class BusInterface;

  class EventInterpreterPluginDS485 : public EventInterpreterPlugin {
  private:
    BusInterface* m_pInterface;
    Apartment& m_Apartment;
    std::string getParameter(Poco::XML::Node* _node, const std::string& _parameterName);
  public:
    EventInterpreterPluginDS485(Apartment& _apartment, BusInterface* _pInterface, EventInterpreter* _pInterpreter);

    virtual boost::shared_ptr<SubscriptionOptions> createOptionsFromXML(Poco::XML::Node* _node);

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
  protected:
    EventInterpreterInternalRelay& getRelay() { return m_Relay; }
  private:
    EventInterpreterInternalRelay& m_Relay;
    typedef std::vector<boost::shared_ptr<EventSubscription> > SubscriptionPtrList;
    SubscriptionPtrList m_Subscriptions;
  }; // EventRelayTarget

  class EventInterpreterInternalRelay : public EventInterpreterPlugin {
  public:
    EventInterpreterInternalRelay(EventInterpreter* _pInterpreter);
    virtual ~EventInterpreterInternalRelay();

    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);

    static const char* getPluginName() { return "internal_relay"; }
    EventInterpreter& getEventInterpreter() { return EventInterpreterPlugin::getEventInterpreter(); }
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
   } __attribute__ ((deprecated)); // EventInterpreterPluginEmail

  class EventInterpreterPluginSendmail : public EventInterpreterPlugin {
   private:
   public:
       EventInterpreterPluginSendmail(EventInterpreter* _pInterpreter);

     virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
   }; // EventInterpreterPluginEmail

} // namespace dss

#endif /* EVENTINTERPRETERPLUGINS_H_ */