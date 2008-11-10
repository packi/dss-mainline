/*
 * event.cpp
 *
 *  Created on: Oct 24, 2008
 *      Author: patrick
 */

#include "event.h"

#include "base.h"
#include "logger.h"
#include "xmlwrapper.h"

#include <set>
#include <iostream>

using std::set;
using std::cout;

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
  : m_Queue(_queue),
    m_EventsProcessed(0)
  { } // ctor

  EventInterpreter::~EventInterpreter() {
    ScrubVector(m_Plugins);
    ScrubVector(m_Subscriptions);
  } // dtor

  void EventInterpreter::AddPlugin(EventInterpreterPlugin* _plugin) {
    m_Plugins.push_back(_plugin);
  } // AddPlugin

  void EventInterpreter::Execute() {
    while(!m_Terminated && m_Queue->WaitForEvent()) {
      Event* toProcess = m_Queue->PopEvent();
      if(toProcess != NULL) {

        Logger::GetInstance()->Log(string("EventInterpreter: Got event from queue: '") + toProcess->GetName() + "'");

        for(vector<EventSubscription*>::iterator ipSubscription = m_Subscriptions.begin(), e = m_Subscriptions.end();
            ipSubscription != e; ++ipSubscription)
        {
          if((*ipSubscription)->Matches(*toProcess)) {
            bool called = false;
            Logger::GetInstance()->Log(string("EventInterpreter: Subscription'") + (*ipSubscription)->GetID() + "' matches event");

            EventInterpreterPlugin* plugin = GetPluginByName((*ipSubscription)->GetHandlerName());
            if(plugin != NULL) {
              Logger::GetInstance()->Log(string("EventInterpreter: Found handler '") + plugin->GetName() + "' calling...");
              plugin->HandleEvent(*toProcess, **ipSubscription);
              called = true;
              Logger::GetInstance()->Log("EventInterpreter: called.");
              break;
            }
            if(!called) {
              Logger::GetInstance()->Log(string("EventInterpreter: Could not find handler '") + (*ipSubscription)->GetHandlerName());
            }

          }
        }

        m_EventsProcessed++;
        Logger::GetInstance()->Log(string("EventInterpreter: Done processing event '") + toProcess->GetName() + "'");
        delete toProcess;
      }
    }
  } // Execute

  EventInterpreterPlugin* EventInterpreter::GetPluginByName(const string& _name) {
    for(vector<EventInterpreterPlugin*>::iterator ipPlugin = m_Plugins.begin(), e = m_Plugins.end();
        ipPlugin != e; ++ipPlugin)
    {
      if((*ipPlugin)->GetName() == _name) {
        return *ipPlugin;
      }
    }
    return NULL;
  } // GetPluginByName

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

  void EventInterpreter::LoadFromXML(const string& _fileName) {
    const int apartmentConfigVersion = 1;
    Logger::GetInstance()->Log(string("EventInterpreter: Loading subscriptions from '") + _fileName + "'");

    XMLDocumentFileReader reader(_fileName);

    XMLNode rootNode = reader.GetDocument().GetRootNode();
    if(rootNode.GetName() == "subscriptions") {
      if(StrToIntDef(rootNode.GetAttributes()["version"], -1) == apartmentConfigVersion) {
        XMLNodeList nodes = rootNode.GetChildren();
        for(XMLNodeList::iterator iNode = nodes.begin(); iNode != nodes.end(); ++iNode) {
          string nodeName = iNode->GetName();
          if(nodeName == "subscription") {
            LoadSubscription(*iNode);
          }
        }
      }
    }
  } // LoadFromXML

  void EventInterpreter::LoadSubscription(XMLNode& _node) {
    string evtName = _node.GetAttributes()["event-name"];
    string handlerName = _node.GetAttributes()["handler-name"];

    if(evtName.size() == 0) {
      Logger::GetInstance()->Log("EventInterpreter::LoadSubscription: empty event-name, skipping this subscription", lsWarning);
      return;
    }

    if(handlerName.size() == 0) {
      Logger::GetInstance()->Log("EventInterpreter::LoadSubscription: empty handler-name, skipping this subscription", lsWarning);
      return;
    }

    SubscriptionOptions* opts = NULL;

    EventInterpreterPlugin* plugin = GetPluginByName(handlerName);
    if(plugin == NULL) {
      Logger::GetInstance()->Log(string("EventInterpreter::LoadSubscription: could not find plugin for handler-name '") + handlerName + "'");
      Logger::GetInstance()->Log(       "EventInterpreter::LoadSubscription: Still generating a subscription but w/o inner parameter");
    } else {
      opts = plugin->CreateOptionsFromXML(_node.GetChildren());
    }

    EventSubscription* subscription = new EventSubscription(evtName, handlerName, opts);
    Subscribe(subscription);
  } // LoadSubsription


  //================================================== EventQueue

  void EventQueue::PushEvent(Event* _event) {
    Logger::GetInstance()->Log(string("EventQueue: New event '") + _event->GetName() + "' in queue...");
    m_QueueMutex.Lock();
    m_EventQueue.push(_event);
    m_QueueMutex.Unlock();
    m_EntryInQueueEvt.Signal();
  } // PushEvent

  Event* EventQueue::PopEvent() {
    m_QueueMutex.Lock();
    Event* result = NULL;
    if(!m_EventQueue.empty()) {
      result = m_EventQueue.front();
    }
    m_EventQueue.pop();
    m_QueueMutex.Unlock();
    return result;
  } // PopEvent

  bool EventQueue::WaitForEvent() {
    if(m_EventQueue.empty()) {
      m_EntryInQueueEvt.WaitFor();
    }
    return !m_EventQueue.empty();
  } // WaitForEvent

  void EventQueue::Shutdown() {
    m_EntryInQueueEvt.Broadcast();
  } // Shutdown


  //================================================== EventSubscription

  EventSubscription::EventSubscription(const string& _eventName, const string& _handlerName, SubscriptionOptions* _options)
  : m_EventName(_eventName),
    m_HandlerName(_handlerName),
    m_ID(_eventName + _handlerName),
    m_SubscriptionOptions(_options)
  {
    Initialize();
  } // ctor

  EventSubscription::EventSubscription(const string& _eventName, const string& _handlerName, const string& _id, SubscriptionOptions* _options)
  : m_EventName(_eventName),
    m_HandlerName(_handlerName),
    m_ID(_id),
    m_SubscriptionOptions(_options)
  {
    Initialize();
  } // ctor(with id)

  EventSubscription::~EventSubscription() {
    delete m_SubscriptionOptions;
  } // dtor

  void EventSubscription::Initialize() {
    m_FilterOption = foMatchAll;
    AddPropertyFilter(new EventPropertyMatchFilter(EventPropertyName, m_EventName));
  } // Initialize

  void EventSubscription::AddPropertyFilter(EventPropertyFilter* _pPropertyFilter) {
    m_Filter.push_back(_pPropertyFilter);
  } // AddPropertyFilter

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


  //================================================== SubscriptionOptions

  SubscriptionOptions::SubscriptionOptions()
  { } // ctor

  SubscriptionOptions::~SubscriptionOptions()
  { } // dtor

  const string& SubscriptionOptions::GetParameter(const string& _name) const {
    HashMapConstStringString::const_iterator it = m_Parameters.find(_name);
    if(it != m_Parameters.end()) {
      return it->second;
    }

    throw runtime_error(string("no value for parameter found: ") + _name);
  } // GetParameter

  void SubscriptionOptions::SetParameter(const string& _name, const string& _value) {
    m_Parameters[_name] = _value;
  } // SetParameter


  //================================================== EventInterpreterPlugin

  EventInterpreterPlugin::EventInterpreterPlugin(const string& _name, EventInterpreter* _interpreter)
  : m_Name(_name),
    m_pInterpreter(_interpreter)
  { } // ctor

  SubscriptionOptions* EventInterpreterPlugin::CreateOptionsFromXML(XMLNodeList& _node) {
    return NULL;
  }

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
