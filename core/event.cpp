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
#include "dss.h"

#include <set>
#include <iostream>

using std::set;
using std::cout;

namespace dss {

  //================================================== Event

  Event::Event(const string& _name)
  : m_Name(_name),
    m_RaiseLocation(erlApartment)
  {
    Reset();
  } // ctor

  Event::Event(const string& _name, Zone* _zone)
  : m_Name(_name),
    m_RaiseLocation(erlZone),
    m_RaisedAtZone(_zone)
  {
    Reset();
  } // ctor

  void Event::Reset() {
    m_LocationSet = false;
    m_ContextSet = false;
    m_TimeSet = false;
  } // Reset

  const string& Event::GetPropertyByName(const string& _name) const {
    if(_name == EventPropertyName) {
      return m_Name;
    } else if(_name == EventPropertyLocation) {
      return m_Location;
    } else if(_name == EventPropertyContext) {
      return m_Context;
    } else if(_name == EventPropertyTime) {
      return m_Time;
    }
    return m_Properties.Get(_name, "");
  } // GetPropertyByName

  bool Event::HasPropertySet(const string& _name) const {
    if(_name == EventPropertyName) {
      return true;
    } else if(_name == EventPropertyLocation) {
      return m_LocationSet;
    } else if(_name == EventPropertyContext) {
      return m_ContextSet;
    } else if(_name == EventPropertyTime) {
      return m_TimeSet;
    }
    return m_Properties.Has(_name);
  } // HasPropertySet

  void Event::UnsetProperty(const string& _name) {
    if(_name == EventPropertyLocation) {
      m_LocationSet = false;
      m_Location = "";
    } else if(_name == EventPropertyContext) {
      m_ContextSet = false;
      m_Context = "";
    } else if(_name == EventPropertyTime) {
      m_TimeSet = false;
      m_Time = "";
    } else {
      m_Properties.Unset(_name);
    }
  } // UnsetProperty

  bool Event::SetProperty(const string& _name, const string& _value) {
    if(!_name.empty()) {
      m_Properties.Set(_name, _value);
      return true;
    }
    return false;
  }

  const Zone& Event::GetRaisedAtZone() const {
    if(m_RaiseLocation == erlZone) {
      return *m_RaisedAtZone;
    } else if(m_RaiseLocation == erlDevice) {
      const Device& dev = m_RaisedAtDevice->GetDevice();
      return dev.GetApartment().GetZone(dev.GetZoneID());
    } else {
      // TODO: We should really try to get the apartment from elsewhere...
      return DSS::GetInstance()->GetApartment().GetZone(0);
    }
  } // GetRaisedAtZone

  //================================================== EventInterpreter

/*

   EventInterpreter::EventInterpreter(EventQueue* _queue)
  : m_Queue(_queue),
    m_EventsProcessed(0)
  { } // ctor(EventQueue)
*/
  EventInterpreter::EventInterpreter(DSS* _pDSS)
  : Subsystem(_pDSS, "EventInterpreter"),
    Thread("EventInterpreter"),
    m_Queue(NULL),
    m_EventRunner(NULL),
    m_EventsProcessed(0)
  { } // ctor()

  EventInterpreter::~EventInterpreter() {
    ScrubVector(m_Plugins);
  } // dtor

  void EventInterpreter::Start() {
    Subsystem::Start();
    Run();
  }

  void EventInterpreter::AddPlugin(EventInterpreterPlugin* _plugin) {
    m_Plugins.push_back(_plugin);
  } // AddPlugin

  void EventInterpreter::Execute() {
    if(m_Queue == NULL) {
      Logger::GetInstance()->Log("EventInterpreter: No queue set. Can't work like that... exiting...", lsFatal);
      return;
    }

    if(m_EventRunner == NULL) {
      Logger::GetInstance()->Log("EventInterpreter: No runner set. exiting...", lsFatal);
      return;
    }
    while(!m_Terminated) {
      if(m_Queue->WaitForEvent()) {
        boost::shared_ptr<Event> toProcess = m_Queue->PopEvent();
        if(toProcess.get() != NULL) {

          Logger::GetInstance()->Log(string("EventInterpreter: Got event from queue: '") + toProcess->GetName() + "'");

          for(vector< boost::shared_ptr<EventSubscription> >::iterator ipSubscription = m_Subscriptions.begin(), e = m_Subscriptions.end();
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
               // break;
              }
              if(!called) {
                Logger::GetInstance()->Log(string("EventInterpreter: Could not find handler '") + (*ipSubscription)->GetHandlerName());
              }

            }
          }

          m_EventsProcessed++;
          Logger::GetInstance()->Log(string("EventInterpreter: Done processing event '") + toProcess->GetName() + "'");
        }
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

  void EventInterpreter::Subscribe(boost::shared_ptr<EventSubscription> _subscription) {
    m_Subscriptions.push_back(_subscription);
  } // Subscribe

  void EventInterpreter::Unsubscribe(const string& _subscriptionID) {
    for(vector< boost::shared_ptr<EventSubscription> >::iterator ipSubscription = m_Subscriptions.begin(), e = m_Subscriptions.end();
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

    if(FileExists(_fileName)) {
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
    }
  } // LoadFromXML

  void EventInterpreter::LoadFilter(XMLNode& _node, EventSubscription& _subscription) {
    string matchType = _node.GetAttributes()["match"];
    if(matchType == "all") {
      _subscription.SetFilterOption(EventSubscription::foMatchAll);
    } else if(matchType == "none") {
      _subscription.SetFilterOption(EventSubscription::foMatchNone);
    } else if(matchType == "one") {
      _subscription.SetFilterOption(EventSubscription::foMatchOne);
    } else {
      Logger::GetInstance()->Log(string("EventInterpreter::LoadFilter: Could not determine the match-type (\"") + matchType + "\", reverting to 'all'", lsError);
    }
    XMLNodeList nodes = _node.GetChildren();
    for(XMLNodeList::iterator iNode = nodes.begin(); iNode != nodes.end(); ++iNode) {
      string nodeName = iNode->GetName();
      if(nodeName == "property-filter") {
        EventPropertyFilter* filter = NULL;
        string filterType = iNode->GetAttributes()["type"];
        string propertyName = iNode->GetAttributes()["property"];
        if(filterType.empty() || propertyName.empty()) {
          Logger::GetInstance()->Log("EventInterpreter::LoadFilter: Missing type and/or property-name", lsFatal);
        } else {
          if(filterType == "exists") {
            filter = new EventPropertyExistsFilter(propertyName);
          } else if(filterType == "missing") {
            filter = new EventPropertyMissingFilter(propertyName);
          } else if(filterType == "matches") {
            string matchValue = iNode->GetAttributes()["value"];
            filter = new EventPropertyMatchFilter(propertyName, matchValue);
          } else {
            Logger::GetInstance()->Log("Unknown property-filter type", lsError);
          }
        }
        if(filter != NULL) {
          _subscription.AddPropertyFilter(filter);
        }
      }
    }
  } // LoadFilter

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

    boost::shared_ptr<SubscriptionOptions> opts;
    bool hadOpts = false;

    EventInterpreterPlugin* plugin = GetPluginByName(handlerName);
    if(plugin == NULL) {
      Logger::GetInstance()->Log(string("EventInterpreter::LoadSubscription: could not find plugin for handler-name '") + handlerName + "'", lsWarning);
      Logger::GetInstance()->Log(       "EventInterpreter::LoadSubscription: Still generating a subscription but w/o inner parameter", lsWarning);
    } else {
      opts.reset(plugin->CreateOptionsFromXML(_node.GetChildren()));
      hadOpts = true;
    }
    try {
      XMLNode& paramNode = _node.GetChildByName("parameter");
      if(opts.get() == NULL) {
        opts.reset(new SubscriptionOptions());
      }
      opts->LoadParameterFromXML(paramNode);
    } catch(runtime_error& e) {
      // only delete options created in the try-part...
      if(!hadOpts) {
        opts.reset();
      }
    }

    boost::shared_ptr<EventSubscription> subscription(new EventSubscription(evtName, handlerName, opts));
    try {
      XMLNode& filterNode = _node.GetChildByName("filter");
      LoadFilter(filterNode, *subscription);
    } catch(runtime_error& e) {
    }

    Subscribe(subscription);
  } // LoadSubsription


  //================================================== EventQueue

  EventQueue::EventQueue()
  : m_EventRunner(NULL)
  { } // ctor

  void EventQueue::PushEvent(boost::shared_ptr<Event> _event) {
    Logger::GetInstance()->Log(string("EventQueue: New event '") + _event->GetName() + "' in queue...");
    if(_event->HasPropertySet(EventPropertyTime)) {
      DateTime when;
      bool validDate = false;
      string timeStr = _event->GetPropertyByName(EventPropertyTime);
      if(timeStr.size() >= 2) {
        // relative time
        if(timeStr[0] == '+') {
          string timeOffset = timeStr.substr(1, string::npos);
          int offset = StrToIntDef(timeOffset, -1);
          if(offset >= 0) {
            when = when.AddSeconds(offset);
            validDate = true;
          } else {
            Logger::GetInstance()->Log(string("EventQueue::PushEvent: Could not parse offset or offset is below zero '") + timeOffset + "'", lsError);
          }
        } else {
          try {
            when = DateTime::FromISO(timeStr);
            validDate = true;
          } catch(runtime_error& e) {
            Logger::GetInstance()->Log(string("EventQueue::PushEvent: Invalid time specified '") + timeStr + "' error: " + e.what(), lsError);
          }
        }
      }
      if(validDate) {
        Logger::GetInstance()->Log(string("EventQueue::PushEvent: Event has a valid time, rescheduling at ") + (string)when);
        Schedule* sched = new StaticSchedule(when);
        ScheduledEvent* scheduledEvent = new ScheduledEvent(_event, sched);
        scheduledEvent->SetOwnsEvent(false);
        m_EventRunner->AddEvent(scheduledEvent);
      } else {
        Logger::GetInstance()->Log("EventQueue::PushEvent: Dropping event with invalid time", lsError);
      }
    } else {
      m_QueueMutex.Lock();
      m_EventQueue.push(_event);
      m_QueueMutex.Unlock();
      m_EntryInQueueEvt.Signal();
    }
  } // PushEvent

  boost::shared_ptr<Event> EventQueue::PopEvent() {
    m_QueueMutex.Lock();
    boost::shared_ptr<Event> result;
    if(!m_EventQueue.empty()) {
      result = m_EventQueue.front();
      m_EventQueue.pop();
    }
    m_QueueMutex.Unlock();
    return result;
  } // PopEvent

  bool EventQueue::WaitForEvent() {
    if(m_EventQueue.empty()) {
      m_EntryInQueueEvt.WaitFor(1000);
    }
    return !m_EventQueue.empty();
  } // WaitForEvent

  void EventQueue::Shutdown() {
    m_EntryInQueueEvt.Broadcast();
  } // Shutdown


  //================================================== EventRunner

  const bool DebugEventRunner = true;

  EventRunner::EventRunner()
  : m_EventQueue(NULL)
  { } // ctor

  int EventRunner::GetSize() const {
    return m_ScheduledEvents.size();
  } // GetSize

  const ScheduledEvent& EventRunner::GetEvent(const int _idx) const {
    return m_ScheduledEvents.at(_idx);
  } // GetEvent

  void EventRunner::RemoveEvent(const int _idx) {
    boost::ptr_vector<ScheduledEvent>::iterator it = m_ScheduledEvents.begin();
    advance(it, _idx);
    m_ScheduledEvents.erase(it);
  } // RemoveEvent

  void EventRunner::AddEvent(ScheduledEvent* _scheduledEvent) {
    m_ScheduledEvents.push_back(_scheduledEvent);
    m_NewItem.Signal();
  } // AddEvent

  DateTime EventRunner::GetNextOccurence() {
    DateTime now;
    DateTime result = now.AddYear(10);
    if(DebugEventRunner) {
      Logger::GetInstance()->Log("EventRunner: *********");
      Logger::GetInstance()->Log("number in queue: " + IntToString(GetSize()));
    }
    for(boost::ptr_vector<ScheduledEvent>::iterator ipSchedEvt = m_ScheduledEvents.begin(), e = m_ScheduledEvents.end();
        ipSchedEvt != e; ++ipSchedEvt)
    {
      DateTime next = ipSchedEvt->GetSchedule().GetNextOccurence(now);
      if(DebugEventRunner) {
        Logger::GetInstance()->Log(string("next:   ") + (string)next);
        Logger::GetInstance()->Log(string("result: ") + (string)result);
      }
      result = min(result, next);
      if(DebugEventRunner) {
        Logger::GetInstance()->Log(string("chosen: ") + (string)result);
      }
    }
    return result;
  } // GetNextOccurence

  void EventRunner::Run() {
    while(true) {
      RunOnce();
    }
  } // Run

  bool EventRunner::RunOnce() {
    if(m_ScheduledEvents.empty()) {
      m_NewItem.WaitFor(1000);
      return false;
    } else {
      DateTime now;
      m_WakeTime = GetNextOccurence();
      int sleepSeconds = m_WakeTime.Difference(now);

      // Prevent loops when a cycle takes less than 1s
      if(sleepSeconds == 0) {
        m_NewItem.WaitFor(1000);
        return false;
      }

      if(!m_NewItem.WaitFor(sleepSeconds * 1000)) {
        return RaisePendingEvents(m_WakeTime, 2);
      }
    }
    return false;
  }

  bool EventRunner::RaisePendingEvents(DateTime& _from, int _deltaSeconds) {
    bool result = false;
    DateTime virtualNow = _from.AddSeconds(-_deltaSeconds/2);
    if(DebugEventRunner) {
      cout << "vNow:    " << virtualNow << endl;
    }
    for(boost::ptr_vector<ScheduledEvent>::iterator ipSchedEvt = m_ScheduledEvents.begin(), e = m_ScheduledEvents.end();
        ipSchedEvt != e; ++ipSchedEvt)
    {
      DateTime nextOccurence = ipSchedEvt->GetSchedule().GetNextOccurence(virtualNow);
      if(DebugEventRunner) {
        cout << "nextOcc: " << nextOccurence << endl;
        cout << "diff:    " << nextOccurence.Difference(virtualNow) << endl;
      }
      if(abs(nextOccurence.Difference(virtualNow)) <= _deltaSeconds/2) {
        result = true;
        if(m_EventQueue != NULL) {
          boost::shared_ptr<Event> evt = ipSchedEvt->GetEvent();
          if(evt->HasPropertySet(EventPropertyTime)) {
            evt->UnsetProperty(EventPropertyTime);
          }
          m_EventQueue->PushEvent(evt);
        } else {
          Logger::GetInstance()->Log("EventRunner: Cannot push event back to queue because the Queue is NULL", lsFatal);
        }
      }
    }
    return result;
  } // RaisePendingEvents


  //================================================== ScheduledEvent

  ScheduledEvent::~ScheduledEvent() {
    delete m_Schedule;
    m_Schedule = NULL;
  } // dtor


  //================================================== EventSubscription

  EventSubscription::EventSubscription(const string& _eventName, const string& _handlerName, boost::shared_ptr<SubscriptionOptions> _options)
  : m_EventName(_eventName),
    m_HandlerName(_handlerName),
    m_ID(_eventName + _handlerName),
    m_SubscriptionOptions(_options)
  {
    Initialize();
  } // ctor

  EventSubscription::EventSubscription(const string& _eventName, const string& _handlerName, const string& _id, boost::shared_ptr<SubscriptionOptions> _options)
  : m_EventName(_eventName),
    m_HandlerName(_handlerName),
    m_ID(_id),
    m_SubscriptionOptions(_options)
  {
    Initialize();
  } // ctor(with id)

  EventSubscription::~EventSubscription() {
  } // dtor

  void EventSubscription::Initialize() {
    m_FilterOption = foMatchAll;
    AddPropertyFilter(new EventPropertyMatchFilter(EventPropertyName, m_EventName));
  } // Initialize

  void EventSubscription::AddPropertyFilter(EventPropertyFilter* _pPropertyFilter) {
    m_Filter.push_back(_pPropertyFilter);
  } // AddPropertyFilter

  bool EventSubscription::Matches(Event& _event) {
    for(boost::ptr_vector<EventPropertyFilter>::iterator ipFilter = m_Filter.begin(), e = m_Filter.end();
        ipFilter != e; ++ipFilter)
    {
      if(ipFilter->Matches(_event)) {
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
    return m_Parameters.Get(_name);
  } // GetParameter

  bool SubscriptionOptions::HasParameter(const string& _name) const {
    return m_Parameters.Has(_name);
  } // HasParameter

  void SubscriptionOptions::SetParameter(const string& _name, const string& _value) {
    m_Parameters.Set(_name, _value);
  } // SetParameter

  void SubscriptionOptions::LoadParameterFromXML(XMLNode& _node) {
    XMLNodeList nodes = _node.GetChildren();
    for(XMLNodeList::iterator iNode = nodes.begin(); iNode != nodes.end(); ++iNode) {
      string nodeName = iNode->GetName();
      if(nodeName == "parameter") {
        string value;
        string name;
        if(iNode->GetChildren().size() > 0) {
          value = iNode->GetChildren()[0].GetContent();
        }
        name = iNode->GetAttributes()["name"];
        if(name.size() > 0) {
          SetParameter(name, value);
        }
      }
    }
  } // LoadParameterFromXML


  //================================================== EventInterpreterPlugin

  EventInterpreterPlugin::EventInterpreterPlugin(const string& _name, EventInterpreter* _interpreter)
  : m_Name(_name),
    m_pInterpreter(_interpreter)
  { } // ctor

  SubscriptionOptions* EventInterpreterPlugin::CreateOptionsFromXML(XMLNodeList& _nodes) {
    return NULL;
  } // CreateOptionsFromXML

  //================================================== EventPropertyFilter

  EventPropertyFilter::EventPropertyFilter(const string& _propertyName)
  : m_PropertyName(_propertyName)
  { } // ctor


  //================================================== EventPropertyMatchFilter

  bool EventPropertyMatchFilter::Matches(const Event& _event) {
    if(_event.HasPropertySet(GetPropertyName())) {
      return _event.GetPropertyByName(GetPropertyName()) == m_Value;
    }
    return false;
  } // Matches


  //================================================== EventPropertyExistsFilter

  EventPropertyExistsFilter::EventPropertyExistsFilter(const string& _propertyName)
  : EventPropertyFilter(_propertyName)
  { } // ctor

  bool EventPropertyExistsFilter::Matches(const Event& _event) {
    return _event.HasPropertySet(GetPropertyName());
  } // Matches


  //================================================= EventPropertyMissingFilter

  EventPropertyMissingFilter::EventPropertyMissingFilter(const string& _propertyName)
  : EventPropertyFilter(_propertyName)
  { } // ctor

  bool EventPropertyMissingFilter::Matches(const Event& _event) {
    return !_event.HasPropertySet(GetPropertyName());
  } // Matches


  //================================================== External consts

  const char* EventPropertyName = "name";
  const char* EventPropertyLocation = "location";
  const char* EventPropertyContext = "context";
  const char* EventPropertyTime = "time";

} // namespace dss
