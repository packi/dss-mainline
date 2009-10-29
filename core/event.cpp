/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland
    Copyright (c) 2008 Patrick Staehlin <me@packi.ch>

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

#include "event.h"

#include "base.h"
#include "logger.h"
#include "xmlwrapper.h"
#include "dss.h"
#include "propertysystem.h"

#include "foreach.h"

#include <set>
#include <iostream>

using std::set;
using std::cout;

namespace dss {

  //================================================== Event

  Event::Event(const string& _name)
  : m_Name(_name),
    m_RaiseLocation(erlApartment),
    m_RaisedAtZone(NULL),
    m_RaisedAtDevice(NULL)
  {
    reset();
  } // ctor

  Event::Event(const string& _name, Zone* _zone)
  : m_Name(_name),
    m_RaiseLocation(erlZone),
    m_RaisedAtZone(_zone),
    m_RaisedAtDevice(NULL)
  {
    reset();
  } // ctor

  Event::Event(const string& _name, DeviceReference* _reference)
  : m_Name(_name),
    m_RaiseLocation(erlDevice),
    m_RaisedAtZone(NULL),
    m_RaisedAtDevice(NULL)
  {
    m_RaisedAtDevice = new DeviceReference(*_reference);
    reset();
  }

  Event::Event()
  : m_Name("unnamed_event")
  {
    reset();
  }

  Event::~Event() {
    delete m_RaisedAtDevice;
    m_RaisedAtDevice = NULL;
  }

  void Event::reset() {
    m_LocationSet = false;
    m_ContextSet = false;
    m_TimeSet = false;
  } // reset

  const string& Event::getPropertyByName(const string& _name) const {
    if(_name == EventPropertyName) {
      return m_Name;
    } else if(_name == EventPropertyLocation) {
      return m_Location;
    } else if(_name == EventPropertyContext) {
      return m_Context;
    } else if(_name == EventPropertyTime) {
      return m_Time;
    }
    return m_Properties.get(_name, "");
  } // getPropertyByName

  bool Event::hasPropertySet(const string& _name) const {
    if(_name == EventPropertyName) {
      return true;
    } else if(_name == EventPropertyLocation) {
      return m_LocationSet;
    } else if(_name == EventPropertyContext) {
      return m_ContextSet;
    } else if(_name == EventPropertyTime) {
      return m_TimeSet;
    }
    return m_Properties.has(_name);
  } // hasPropertySet

  void Event::unsetProperty(const string& _name) {
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
      m_Properties.unset(_name);
    }
  } // unsetProperty

  bool Event::setProperty(const string& _name, const string& _value) {
    if(!_name.empty()) {
      if(_name == EventPropertyLocation) {
        m_LocationSet = true;
        m_Location = _value;
      } else if(_name == EventPropertyContext) {
        m_ContextSet = true;
        m_Context = _value;
      } else if(_name == EventPropertyTime) {
        m_TimeSet = true;
        m_Time = _value;
      } else {
        m_Properties.set(_name, _value);
      }
      return true;
    }
    return false;
  }

  const Zone& Event::getRaisedAtZone() const {
    if(m_RaiseLocation == erlZone) {
      return *m_RaisedAtZone;
    } else if(m_RaiseLocation == erlDevice) {
      const Device& dev = m_RaisedAtDevice->getDevice();
      return dev.getApartment().getZone(dev.getZoneID());
    } else {
      // TODO: We should really try to get the apartment from elsewhere...
      return DSS::getInstance()->getApartment().getZone(0);
    }
  } // getRaisedAtZone

  //================================================== EventInterpreter

  EventInterpreter::EventInterpreter(DSS* _pDSS)
  : Subsystem(_pDSS, "EventInterpreter"),
    Thread("EventInterpreter"),
    m_Queue(NULL),
    m_EventRunner(NULL),
    m_EventsProcessed(0)
  {
    if(DSS::hasInstance()) {
      getDSS().getPropertySystem().createProperty(getPropertyBasePath() + "eventsProcessed")->linkToProxy(PropertyProxyPointer<int>(&m_EventsProcessed));
    }
  } // ctor()

  EventInterpreter::~EventInterpreter() {
    scrubVector(m_Plugins);
  } // dtor

  void EventInterpreter::doStart() {
    run();
  } // doStart

  void EventInterpreter::initialize() {
    Subsystem::initialize();
    if(DSS::hasInstance()) {
      getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "subscriptionfile", getDSS().getDataDirectory() + "subscriptions.xml", true, false);
    }
  } // initialize

  void EventInterpreter::addPlugin(EventInterpreterPlugin* _plugin) {
    m_Plugins.push_back(_plugin);
  } // addPlugin

  void EventInterpreter::execute() {
    if(DSS::hasInstance()) {
      loadFromXML(getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "subscriptionfile"));
    }

    if(m_Queue == NULL) {
      Logger::getInstance()->log("EventInterpreter: No queue set. Can't work like that... exiting...", lsFatal);
      return;
    }

    if(m_EventRunner == NULL) {
      Logger::getInstance()->log("EventInterpreter: No runner set. exiting...", lsFatal);
      return;
    }
    while(!m_Terminated) {
      if(m_Queue->waitForEvent()) {
        boost::shared_ptr<Event> toProcess = m_Queue->popEvent();
        if(toProcess.get() != NULL) {

          Logger::getInstance()->log(string("EventInterpreter: Got event from queue: '") + toProcess->getName() + "'", lsInfo);
          for(HashMapConstStringString::const_iterator iParam = toProcess->getProperties().getContainer().begin(), e = toProcess->getProperties().getContainer().end();
              iParam != e; ++iParam)
          {
            Logger::getInstance()->log("EventInterpreter:  Parameter '" + iParam->first + "' = '" + iParam->second + "'");
          }

          for(vector< boost::shared_ptr<EventSubscription> >::iterator ipSubscription = m_Subscriptions.begin(), e = m_Subscriptions.end();
              ipSubscription != e; ++ipSubscription)
          {
            if((*ipSubscription)->matches(*toProcess)) {
              bool called = false;
              Logger::getInstance()->log(string("EventInterpreter: Subscription '") + (*ipSubscription)->getID() + "' matches event");

              EventInterpreterPlugin* plugin = getPluginByName((*ipSubscription)->getHandlerName());
              if(plugin != NULL) {
                Logger::getInstance()->log(string("EventInterpreter: Found handler '") + plugin->getName() + "' calling...");
                plugin->handleEvent(*toProcess, **ipSubscription);
                called = true;
                Logger::getInstance()->log("EventInterpreter: called.");
              }
              if(!called) {
                Logger::getInstance()->log(string("EventInterpreter: Could not find handler '") + (*ipSubscription)->getHandlerName(), lsInfo);
              }

            } else {
              Logger::getInstance()->log(string("EventInterpreter: No match on subscription '") + (*ipSubscription)->getID() + "'");
            }
          }

          m_EventsProcessed++;
          Logger::getInstance()->log(string("EventInterpreter: Done processing event '") + toProcess->getName() + "'", lsInfo);
        }
      }
    }
  } // execute

  EventInterpreterPlugin* EventInterpreter::getPluginByName(const string& _name) {
    for(vector<EventInterpreterPlugin*>::iterator ipPlugin = m_Plugins.begin(), e = m_Plugins.end();
        ipPlugin != e; ++ipPlugin)
    {
      if((*ipPlugin)->getName() == _name) {
        return *ipPlugin;
      }
    }
    return NULL;
  } // getPluginByName

  void EventInterpreter::subscribe(boost::shared_ptr<EventSubscription> _subscription) {
    assert(_subscription != NULL);
    assert(subscriptionByID(_subscription->getID()) == NULL);
    m_Subscriptions.push_back(_subscription);
  } // subscribe

  void EventInterpreter::unsubscribe(const string& _subscriptionID) {
    for(vector< boost::shared_ptr<EventSubscription> >::iterator ipSubscription = m_Subscriptions.begin(), e = m_Subscriptions.end();
        ipSubscription != e; ++ipSubscription)
    {
      if((*ipSubscription)->getID() == _subscriptionID) {
        m_Subscriptions.erase(ipSubscription);
        break;
      }
    }
  } // unsubscribe

  boost::shared_ptr<EventSubscription> EventInterpreter::subscriptionByID(const std::string& _subscriptionID) {
    boost::shared_ptr<EventSubscription> result;
    for(vector< boost::shared_ptr<EventSubscription> >::iterator ipSubscription = m_Subscriptions.begin(), e = m_Subscriptions.end();
        ipSubscription != e; ++ipSubscription)
    {
      if((*ipSubscription)->getID() == _subscriptionID) {
        result = *ipSubscription;
        break;
      }
    }
    return result;
  } // subscriptionByID

  std::string EventInterpreter::uniqueSubscriptionID(const std::string& _proposal) {
    std::string result = _proposal;
    int index = 1;
    while(subscriptionByID(result) != NULL) {
      result = _proposal + intToString(index++);
    }
    return result;
  } // uniqueSubscriptionID

  void EventInterpreter::loadFromXML(const string& _fileName) {
    const int apartmentConfigVersion = 1;
    Logger::getInstance()->log(string("EventInterpreter: Loading subscriptions from '") + _fileName + "'");

    if(fileExists(_fileName)) {
      XMLDocumentFileReader reader(_fileName);

      XMLNode rootNode = reader.getDocument().getRootNode();
      if(rootNode.getName() == "subscriptions") {
        if(strToIntDef(rootNode.getAttributes()["version"], -1) == apartmentConfigVersion) {
          XMLNodeList nodes = rootNode.getChildren();
          for(XMLNodeList::iterator iNode = nodes.begin(); iNode != nodes.end(); ++iNode) {
            string nodeName = iNode->getName();
            if(nodeName == "subscription") {
              loadSubscription(*iNode);
            }
          }
        }
      }
    }
  } // loadFromXML

  void EventInterpreter::loadFilter(XMLNode& _node, EventSubscription& _subscription) {
    string matchType = _node.getAttributes()["match"];
    if(matchType == "all") {
      _subscription.setFilterOption(EventSubscription::foMatchAll);
    } else if(matchType == "none") {
      _subscription.setFilterOption(EventSubscription::foMatchNone);
    } else if(matchType == "one") {
      _subscription.setFilterOption(EventSubscription::foMatchOne);
    } else {
      Logger::getInstance()->log(string("EventInterpreter::loadFilter: Could not determine the match-type (\"") + matchType + "\", reverting to 'all'", lsError);
    }
    XMLNodeList nodes = _node.getChildren();
    for(XMLNodeList::iterator iNode = nodes.begin(); iNode != nodes.end(); ++iNode) {
      string nodeName = iNode->getName();
      if(nodeName == "property-filter") {
        EventPropertyFilter* filter = NULL;
        string filterType = iNode->getAttributes()["type"];
        string propertyName = iNode->getAttributes()["property"];
        if(filterType.empty() || propertyName.empty()) {
          Logger::getInstance()->log("EventInterpreter::loadFilter: Missing type and/or property-name", lsFatal);
        } else {
          if(filterType == "exists") {
            filter = new EventPropertyExistsFilter(propertyName);
          } else if(filterType == "missing") {
            filter = new EventPropertyMissingFilter(propertyName);
          } else if(filterType == "matches") {
            string matchValue = iNode->getAttributes()["value"];
            filter = new EventPropertyMatchFilter(propertyName, matchValue);
          } else {
            Logger::getInstance()->log("Unknown property-filter type", lsError);
          }
        }
        if(filter != NULL) {
          _subscription.addPropertyFilter(filter);
        }
      }
    }
  } // loadFilter

  void EventInterpreter::loadSubscription(XMLNode& _node) {
    string evtName = _node.getAttributes()["event-name"];
    string handlerName = _node.getAttributes()["handler-name"];

    if(evtName.size() == 0) {
      Logger::getInstance()->log("EventInterpreter::loadSubscription: empty event-name, skipping this subscription", lsWarning);
      return;
    }

    if(handlerName.size() == 0) {
      Logger::getInstance()->log("EventInterpreter::loadSubscription: empty handler-name, skipping this subscription", lsWarning);
      return;
    }

    boost::shared_ptr<SubscriptionOptions> opts;
    bool hadOpts = false;

    EventInterpreterPlugin* plugin = getPluginByName(handlerName);
    if(plugin == NULL) {
      Logger::getInstance()->log(string("EventInterpreter::loadSubscription: could not find plugin for handler-name '") + handlerName + "'", lsWarning);
      Logger::getInstance()->log(       "EventInterpreter::loadSubscription: Still generating a subscription but w/o inner parameter", lsWarning);
    } else {
      opts.reset(plugin->createOptionsFromXML(_node.getChildren()));
      hadOpts = true;
    }
    try {
      XMLNode& paramNode = _node.getChildByName("parameter");
      if(opts == NULL) {
        opts.reset(new SubscriptionOptions());
      }
      opts->loadParameterFromXML(paramNode);
    } catch(std::runtime_error& e) {
      // only delete options created in the try-part...
      if(!hadOpts) {
        opts.reset();
      }
    }

    boost::shared_ptr<EventSubscription> subscription(new EventSubscription(evtName, handlerName, *this, opts));
    try {
      XMLNode& filterNode = _node.getChildByName("filter");
      loadFilter(filterNode, *subscription);
    } catch(std::runtime_error& e) {
    }

    subscribe(subscription);
  } // loadSubsription


  //================================================== EventQueue

  EventQueue::EventQueue()
  : m_EventRunner(NULL)
  { } // ctor

  void EventQueue::pushEvent(boost::shared_ptr<Event> _event) {
    Logger::getInstance()->log(string("EventQueue: New event '") + _event->getName() + "' in queue...", lsInfo);
    if(_event->hasPropertySet(EventPropertyTime)) {
      DateTime when;
      bool validDate = false;
      string timeStr = _event->getPropertyByName(EventPropertyTime);
      if(timeStr.size() >= 2) {
        // relative time
        if(timeStr[0] == '+') {
          string timeOffset = timeStr.substr(1, string::npos);
          int offset = strToIntDef(timeOffset, -1);
          if(offset >= 0) {
            when = when.addSeconds(offset);
            validDate = true;
          } else {
            Logger::getInstance()->log(string("EventQueue::pushEvent: Could not parse offset or offset is below zero: '") + timeOffset + "'", lsError);
          }
        } else {
          try {
            when = DateTime::fromISO(timeStr);
            validDate = true;
          } catch(std::runtime_error& e) {
            Logger::getInstance()->log(string("EventQueue::pushEvent: Invalid time specified '") + timeStr + "' error: " + e.what(), lsError);
          }
        }
      }
      if(validDate) {
        Logger::getInstance()->log(string("EventQueue::pushEvent: Event has a valid time, rescheduling at ") + (string)when, lsInfo);
        boost::shared_ptr<Schedule> sched(new StaticSchedule(when));
        ScheduledEvent* scheduledEvent = new ScheduledEvent(_event, sched);
        m_EventRunner->addEvent(scheduledEvent);
      } else {
        Logger::getInstance()->log("EventQueue::pushEvent: Dropping event with invalid time", lsError);
      }
    } else {
      m_QueueMutex.lock();
      m_EventQueue.push(_event);
      m_QueueMutex.unlock();
      m_EntryInQueueEvt.signal();
    }
  } // pushEvent

  boost::shared_ptr<Event> EventQueue::popEvent() {
    m_QueueMutex.lock();
    boost::shared_ptr<Event> result;
    if(!m_EventQueue.empty()) {
      result = m_EventQueue.front();
      m_EventQueue.pop();
    }
    m_QueueMutex.unlock();
    return result;
  } // popEvent

  bool EventQueue::waitForEvent() {
    if(m_EventQueue.empty()) {
      m_EntryInQueueEvt.waitFor(1000);
    }
    return !m_EventQueue.empty();
  } // waitForEvent

  void EventQueue::shutdown() {
    m_EntryInQueueEvt.broadcast();
  } // shutdown


  //================================================== EventRunner

  const bool DebugEventRunner = true;

  EventRunner::EventRunner()
  : m_EventQueue(NULL)
  { } // ctor

  int EventRunner::getSize() const {
    return m_ScheduledEvents.size();
  } // getSize

  const ScheduledEvent& EventRunner::getEvent(const int _idx) const {
    return m_ScheduledEvents.at(_idx);
  } // getEvent

  void EventRunner::removeEvent(const int _idx) {
    m_EventsMutex.lock();
    boost::ptr_vector<ScheduledEvent>::iterator it = m_ScheduledEvents.begin();
    advance(it, _idx);
    m_ScheduledEvents.erase(it);
    m_EventsMutex.unlock();
  } // removeEvent

  void EventRunner::addEvent(ScheduledEvent* _scheduledEvent) {
    m_EventsMutex.lock();
    m_ScheduledEvents.push_back(_scheduledEvent);
    m_EventsMutex.unlock();
    m_NewItem.signal();
  } // addEvent

  DateTime EventRunner::getNextOccurence() {
    DateTime now;
    DateTime result = now.addYear(10);
    if(DebugEventRunner) {
      Logger::getInstance()->log("EventRunner: *********");
      Logger::getInstance()->log("number in queue: " + intToString(getSize()));
    }

    m_EventsMutex.lock();
    for(boost::ptr_vector<ScheduledEvent>::iterator ipSchedEvt = m_ScheduledEvents.begin(), e = m_ScheduledEvents.end();
        ipSchedEvt != e; )
    {
      DateTime next = ipSchedEvt->getSchedule().getNextOccurence(now);
      if(DebugEventRunner) {
        Logger::getInstance()->log(string("next:   ") + (string)next);
        Logger::getInstance()->log(string("result: ") + (string)result);
      }
      if(next == DateTime::NullDate) {
        Logger::getInstance()->log("EventRunner: Removing event");
        m_ScheduledEvents.erase(ipSchedEvt++);
        continue;
      }
      result = std::min(result, next);
      if(DebugEventRunner) {
        Logger::getInstance()->log(string("chosen: ") + (string)result);
      }
      ++ipSchedEvt;
    }
    m_EventsMutex.unlock();
    return result;
  } // getNextOccurence

  void EventRunner::run() {
    while(true) {
      runOnce();
    }
  } // run

  bool EventRunner::runOnce() {
    if(m_ScheduledEvents.empty()) {
      m_NewItem.waitFor(1000);
      return false;
    } else {
      DateTime now;
      m_WakeTime = getNextOccurence();
      int sleepSeconds = m_WakeTime.difference(now);

      // Prevent loops when a cycle takes less than 1s
      if(sleepSeconds <= 0) {
        m_NewItem.waitFor(1000);
        return false;
      }

      if(!m_NewItem.waitFor(sleepSeconds * 1000)) {
        return raisePendingEvents(m_WakeTime, 2);
      }
    }
    return false;
  } // runOnce

  bool EventRunner::raisePendingEvents(DateTime& _from, int _deltaSeconds) {
    bool result = false;
    DateTime virtualNow = _from.addSeconds(-_deltaSeconds/2);
    if(DebugEventRunner) {
      cout << "vNow:    " << virtualNow << std::endl;
    }

    m_EventsMutex.lock();
    for(boost::ptr_vector<ScheduledEvent>::iterator ipSchedEvt = m_ScheduledEvents.begin(), e = m_ScheduledEvents.end();
        ipSchedEvt != e; ++ipSchedEvt)
    {
      DateTime nextOccurence = ipSchedEvt->getSchedule().getNextOccurence(virtualNow);
      if(DebugEventRunner) {
        cout << "nextOcc: " << nextOccurence << std::endl;
        cout << "diff:    " << nextOccurence.difference(virtualNow) << std::endl;
      }
      if(abs(nextOccurence.difference(virtualNow)) <= _deltaSeconds/2) {
        result = true;
        if(m_EventQueue != NULL) {
          boost::shared_ptr<Event> evt = ipSchedEvt->getEvent();
          if(evt->hasPropertySet(EventPropertyTime)) {
            evt->unsetProperty(EventPropertyTime);
          }
          m_EventQueue->pushEvent(evt);
        } else {
          Logger::getInstance()->log("EventRunner: Cannot push event back to queue because the Queue is NULL", lsFatal);
        }
      }
    }
    m_EventsMutex.unlock();
    return result;
  } // raisePendingEvents


  //================================================== EventSubscription

  EventSubscription::EventSubscription(const string& _eventName, const string& _handlerName, EventInterpreter& _interpreter, boost::shared_ptr<SubscriptionOptions> _options)
  : m_EventName(_eventName),
    m_HandlerName(_handlerName),
    m_SubscriptionOptions(_options)
  {
    m_ID = _interpreter.uniqueSubscriptionID(_eventName + "_" +  _handlerName);
    initialize();
  } // ctor

  EventSubscription::~EventSubscription() {
  } // dtor

  void EventSubscription::initialize() {
    m_FilterOption = foMatchAll;
    addPropertyFilter(new EventPropertyMatchFilter(EventPropertyName, m_EventName));
  } // initialize

  void EventSubscription::addPropertyFilter(EventPropertyFilter* _pPropertyFilter) {
    m_Filter.push_back(_pPropertyFilter);
  } // addPropertyFilter

  bool EventSubscription::matches(Event& _event) {
    for(boost::ptr_vector<EventPropertyFilter>::iterator ipFilter = m_Filter.begin(), e = m_Filter.end();
        ipFilter != e; ++ipFilter)
    {
      if(ipFilter->matches(_event)) {
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
  } // matches


  //================================================== SubscriptionOptions

  SubscriptionOptions::SubscriptionOptions()
  { } // ctor

  SubscriptionOptions::~SubscriptionOptions()
  { } // dtor

  const string& SubscriptionOptions::getParameter(const string& _name) const {
    return m_Parameters.get(_name);
  } // getParameter

  bool SubscriptionOptions::hasParameter(const string& _name) const {
    return m_Parameters.has(_name);
  } // hasParameter

  void SubscriptionOptions::setParameter(const string& _name, const string& _value) {
    m_Parameters.set(_name, _value);
  } // setParameter

  void SubscriptionOptions::loadParameterFromXML(XMLNode& _node) {
    XMLNodeList nodes = _node.getChildren();
    for(XMLNodeList::iterator iNode = nodes.begin(); iNode != nodes.end(); ++iNode) {
      string nodeName = iNode->getName();
      if(nodeName == "parameter") {
        string value;
        string name;
        if(!iNode->getChildren().empty()) {
          value = iNode->getChildren()[0].getContent();
        }
        name = iNode->getAttributes()["name"];
        if(!name.empty()) {
          setParameter(name, value);
        }
      }
    }
  } // loadParameterFromXML


  //================================================== EventInterpreterPlugin

  EventInterpreterPlugin::EventInterpreterPlugin(const string& _name, EventInterpreter* _interpreter)
  : m_Name(_name),
    m_pInterpreter(_interpreter)
  { } // ctor

  SubscriptionOptions* EventInterpreterPlugin::createOptionsFromXML(XMLNodeList& _nodes) {
    return NULL;
  } // createOptionsFromXML

  //================================================== EventPropertyFilter

  EventPropertyFilter::EventPropertyFilter(const string& _propertyName)
  : m_PropertyName(_propertyName)
  { } // ctor


  //================================================== EventPropertyMatchFilter

  bool EventPropertyMatchFilter::matches(const Event& _event) {
    if(_event.hasPropertySet(getPropertyName())) {
      return _event.getPropertyByName(getPropertyName()) == m_Value;
    }
    return false;
  } // matches


  //================================================== EventPropertyExistsFilter

  EventPropertyExistsFilter::EventPropertyExistsFilter(const string& _propertyName)
  : EventPropertyFilter(_propertyName)
  { } // ctor

  bool EventPropertyExistsFilter::matches(const Event& _event) {
    return _event.hasPropertySet(getPropertyName());
  } // matches


  //================================================= EventPropertyMissingFilter

  EventPropertyMissingFilter::EventPropertyMissingFilter(const string& _propertyName)
  : EventPropertyFilter(_propertyName)
  { } // ctor

  bool EventPropertyMissingFilter::matches(const Event& _event) {
    return !_event.hasPropertySet(getPropertyName());
  } // matches


  //================================================== External consts

  const char* EventPropertyName = "name";
  const char* EventPropertyLocation = "location";
  const char* EventPropertyContext = "context";
  const char* EventPropertyTime = "time";

} // namespace dss
