/*
    Copyright (c) 2009,2010,2012 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Michael Troß, aizo GmbH <michael.tross@aizo.com>

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
#include "webservice_api.h"

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
    __DECL_LOG_CHANNEL__
  private:
    boost::shared_ptr<ScriptEnvironment> m_pEnvironment;
    boost::shared_ptr<InternalEventRelayTarget> m_pRelayTarget;
    static const std::string kCleanupScriptsEventName;
    PropertyNodePtr m_pScriptRootNode;

    boost::weak_ptr<ScriptContextWrapper> m_WrapperInAction;
    std::vector<boost::shared_ptr<ScriptContextWrapper> > m_WrappedContexts;
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
    HASH_MAP<std::string, EventRelayTarget*> m_IDTargetMap;
  }; // EventInterpreterInternalRelay

  class EventInterpreterPluginSendmail : public EventInterpreterPlugin {
   private:
     static void* run(void* arg);
     pthread_t m_thread;
     pthread_mutex_t m_Mutex;
     pthread_cond_t m_Condition;
     std::deque<std::string> m_MailFiles;
     std::string m_mailq_dir;
     static bool m_active;
   public:
     EventInterpreterPluginSendmail(EventInterpreter* _pInterpreter);
     virtual ~EventInterpreterPluginSendmail();
     virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
   }; // EventInterpreterPluginSendmail

  class EventInterpreterPluginExecutionDeniedDigest : public EventInterpreterPlugin {
  private:
    std::ostringstream m_digest;
    DateTime m_timestamp;
    boost::mutex m_digest_mutex;
    bool m_check_scheduled;

  public:
    EventInterpreterPluginExecutionDeniedDigest(EventInterpreter* _pInterpreter);
    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };

  class EventInterpreterPluginApartmentChange : public EventInterpreterPlugin {
  public:
    EventInterpreterPluginApartmentChange(EventInterpreter* _pInterpreter);
    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };

  class EventInterpreterWebserviceGetWeatherInformationDone : public URLRequestCallback {
  public:
    virtual ~EventInterpreterWebserviceGetWeatherInformationDone() {}
    virtual void result(long code, const std::string &res);
  };

  class EventInterpreterWebservicePlugin : public EventInterpreterPlugin,
                                           private PropertyListener {
  private:
    __DECL_LOG_CHANNEL__
    virtual void propertyChanged(PropertyNodePtr _caller,
                                 PropertyNodePtr _changedNode);
  public:
    static std::string ConnectionKeepAlive;
    EventInterpreterWebservicePlugin(EventInterpreter* _pInterpreter);
    virtual ~EventInterpreterWebservicePlugin();
    virtual void subscribe();
    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  private:
    /** do not call DSS::getInstance() in destructor */
    PropertyNodePtr websvcEnabledNode;
    EventInterpreterWebserviceGetWeatherInformationDone m_cbW;
  };

  class EventInterpreterSensorMonitorPlugin : public EventInterpreterPlugin {
    private:
      __DECL_LOG_CHANNEL__
    public:
      EventInterpreterSensorMonitorPlugin(EventInterpreter* _pInterpreter);
      virtual ~EventInterpreterSensorMonitorPlugin() {};
      virtual void subscribe();
      virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };

  class EventInterpreterStateSensorPlugin : public EventInterpreterPlugin {
    private:
      __DECL_LOG_CHANNEL__
    public:
      EventInterpreterStateSensorPlugin(EventInterpreter* _pInterpreter);
      virtual ~EventInterpreterStateSensorPlugin() {};
      virtual void subscribe();
      virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };

  class EventInterpreterHeatingMonitorPlugin : public EventInterpreterPlugin {
    private:
      __DECL_LOG_CHANNEL__
    public:
      EventInterpreterHeatingMonitorPlugin(EventInterpreter* _pInterpreter);
      virtual ~EventInterpreterHeatingMonitorPlugin() {};
      virtual void subscribe();
      virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };

  class EventInterpreterHeatingValveProtectionPlugin : public EventInterpreterPlugin {
    private:
      __DECL_LOG_CHANNEL__
    public:
      EventInterpreterHeatingValveProtectionPlugin(EventInterpreter* _pInterpreter);
      virtual ~EventInterpreterHeatingValveProtectionPlugin() {};
      virtual void subscribe();
      virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };

  class EventInterpreterDebugMonitorPlugin : public EventInterpreterPlugin {
    private:
      __DECL_LOG_CHANNEL__
    public:
      EventInterpreterDebugMonitorPlugin(EventInterpreter* _pInterpreter);
      virtual ~EventInterpreterDebugMonitorPlugin() {};
      virtual void subscribe();
      virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };

  class EventInterpreterDatabaseUpdatePlugin : public EventInterpreterPlugin {
    public:
      EventInterpreterDatabaseUpdatePlugin(EventInterpreter* _pInterpreter);
      virtual ~EventInterpreterDatabaseUpdatePlugin() {};
      virtual void subscribe();
      virtual void handleEvent(Event& _event, const EventSubscription& _subscription);
  };


} // namespace dss

#endif /* EVENTINTERPRETERPLUGINS_H_ */
