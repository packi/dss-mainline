/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#include "event.h"

#include "logger.h"
#include "dss.h"
#include "propertysystem.h"

#include "foreach.h"
#include "core/model/apartment.h"
#include "core/model/device.h"
#include "core/model/devicereference.h"
#include "core/model/set.h"

#include <set>
#include <iostream>
#include <sstream>

#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/Attr.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/SAX/SAXException.h>

using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Attr;
using Poco::XML::Text;
using Poco::XML::ProcessingInstruction;
using Poco::XML::AutoPtr;
using Poco::XML::DOMParser;
using Poco::XML::InputSource;
using Poco::XML::Node;

using std::set;

namespace dss {

  //================================================== Event

  Event::Event(const std::string& _name)
  : m_Name(_name),
    m_RaiseLocation(erlApartment),
    m_RaisedAtZone(NULL),
    m_RaisedAtDevice(NULL)
  {
    reset();
  } // ctor

  Event::Event(const std::string& _name, Zone* _zone)
  : m_Name(_name),
    m_RaiseLocation(erlZone),
    m_RaisedAtZone(_zone),
    m_RaisedAtDevice(NULL)
  {
    reset();
  } // ctor

  Event::Event(const std::string& _name, DeviceReference* _reference)
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

  const std::string& Event::getPropertyByName(const std::string& _name) const {
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

  bool Event::hasPropertySet(const std::string& _name) const {
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

  void Event::unsetProperty(const std::string& _name) {
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

  bool Event::setProperty(const std::string& _name, const std::string& _value) {
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
      getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "subscriptionfile", getDSS().getConfigDirectory() + "subscriptions.xml", true, false);
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

          Logger::getInstance()->log(std::string("EventInterpreter: Got event from queue: '") + toProcess->getName() + "'", lsInfo);
          for(HashMapConstStringString::const_iterator iParam = toProcess->getProperties().getContainer().begin(), e = toProcess->getProperties().getContainer().end();
              iParam != e; ++iParam)
          {
            Logger::getInstance()->log("EventInterpreter:  Parameter '" + iParam->first + "' = '" + iParam->second + "'");
          }

          for(std::vector< boost::shared_ptr<EventSubscription> >::iterator ipSubscription = m_Subscriptions.begin(), e = m_Subscriptions.end();
              ipSubscription != e; ++ipSubscription)
          {
            if((*ipSubscription)->matches(*toProcess)) {
              bool called = false;
              Logger::getInstance()->log(std::string("EventInterpreter: Subscription '") + (*ipSubscription)->getID() + "' matches event");

              EventInterpreterPlugin* plugin = getPluginByName((*ipSubscription)->getHandlerName());
              if(plugin != NULL) {
                Logger::getInstance()->log("EventInterpreter: Found handler '" + plugin->getName() + "' calling...");
                try {
                  plugin->handleEvent(*toProcess, **ipSubscription);
                } catch(std::runtime_error& e) {
                  Logger::getInstance()->log(std::string("Caught exception while handling event: ") + e.what(), lsError);
                }
                called = true;
                Logger::getInstance()->log("EventInterpreter: called.");
              }
              if(!called) {
                Logger::getInstance()->log(std::string("EventInterpreter: Could not find handler '") + (*ipSubscription)->getHandlerName(), lsInfo);
              }

            }
          }

          m_EventsProcessed++;
          Logger::getInstance()->log(std::string("EventInterpreter: Done processing event '") + toProcess->getName() + "'", lsInfo);
        }
      }
    }
  } // execute

  EventInterpreterPlugin* EventInterpreter::getPluginByName(const std::string& _name) {
    for(std::vector<EventInterpreterPlugin*>::iterator ipPlugin = m_Plugins.begin(), e = m_Plugins.end();
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

  void EventInterpreter::unsubscribe(const std::string& _subscriptionID) {
    for(std::vector< boost::shared_ptr<EventSubscription> >::iterator ipSubscription = m_Subscriptions.begin(), e = m_Subscriptions.end();
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
    for(std::vector< boost::shared_ptr<EventSubscription> >::iterator ipSubscription = m_Subscriptions.begin(), e = m_Subscriptions.end();
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

  void EventInterpreter::loadFromXML(const std::string& _fileName) {
    const int eventConfigVersion = 1;
    Logger::getInstance()->log(std::string("EventInterpreter: Loading subscriptions from '") + _fileName + "'");

    std::ifstream inFile(_fileName.c_str());

    try {
      InputSource input(inFile);
      DOMParser parser;
      AutoPtr<Document> pDoc = parser.parse(&input);
      Element* rootNode = pDoc->documentElement();

      if(rootNode->localName() == "subscriptions") {
        if(rootNode->hasAttribute("version") && (strToInt(rootNode->getAttribute("version")) == eventConfigVersion)) {
          Node* curNode = rootNode->firstChild();
          while(curNode != NULL) {
            if(curNode->localName() == "subscription") {
	            loadSubscription(curNode);
            }
            curNode = curNode->nextSibling();
          }
        }
      } else {
        log(_fileName + " must have a root-node named 'subscriptions'", lsFatal);
      }
    } catch(Poco::XML::SAXParseException& e) {
      log("Error parsing file: " + _fileName + ". message: " + e.message(), lsFatal);
    }
  } // loadFromXML

  void EventInterpreter::loadFilter(Node* _node, EventSubscription& _subscription) {
    if(_node != NULL) {
      Element* elem = dynamic_cast<Element*>(_node);
      if(elem != NULL) {
        std::string matchType = elem->getAttribute("match");
        if(matchType == "all") {
          _subscription.setFilterOption(EventSubscription::foMatchAll);
        } else if(matchType == "none") {
          _subscription.setFilterOption(EventSubscription::foMatchNone);
        } else if(matchType == "one") {
          _subscription.setFilterOption(EventSubscription::foMatchOne);
        } else {
          log(std::string("loadFilter: Could not determine the match-type (\"") + matchType + "\", reverting to 'all'", lsError);
          _subscription.setFilterOption(EventSubscription::foMatchAll);
        }
      }

      Node* curNode = _node->firstChild();
      while(curNode != NULL) {
        std::string nodeName = curNode->localName();
        if(nodeName == "property-filter") {
          loadPropertyFilter(curNode, _subscription);
        }
        curNode = curNode->nextSibling();
      }
    }
  } // loadFilter

  void EventInterpreter::loadPropertyFilter(Node* _pNode, EventSubscription& _subscription) {
    Element* elem = dynamic_cast<Element*>(_pNode);
    if(elem != NULL) {
      EventPropertyFilter* filter = NULL;
      std::string filterType;
      if(elem->hasAttribute("type")) {
        filterType = elem->getAttribute("type");
      }
      std::string propertyName;
      if(elem->hasAttribute("property")) {
        propertyName = elem->getAttribute("property");
      }
      if(filterType.empty() || propertyName.empty()) {
        Logger::getInstance()->log("EventInterpreter::loadProperty: Missing type and/or property-name", lsFatal);
      } else {
        if(filterType == "exists") {
          filter = new EventPropertyExistsFilter(propertyName);
        } else if(filterType == "missing") {
          filter = new EventPropertyMissingFilter(propertyName);
        } else if(filterType == "matches") {
          std::string matchValue;
          if(elem->hasAttribute("value")) {
            matchValue = elem->getAttribute("value");
          }
          filter = new EventPropertyMatchFilter(propertyName, matchValue);
        } else {
          Logger::getInstance()->log("Unknown property-filter type", lsError);
        }
      }
      if(filter != NULL) {
        _subscription.addPropertyFilter(filter);
      }
    }
  } // loadPropertyFilter

  void EventInterpreter::loadSubscription(Node* _node) {
    Element* elem = dynamic_cast<Element*>(_node);
    if(elem != NULL) {
      std::string evtName;
      if(elem->hasAttribute("event-name")) {
        evtName = elem->getAttribute("event-name");
      }
      std::string handlerName;
      if(elem->hasAttribute("handler-name")) {
        handlerName = elem->getAttribute("handler-name");
      }

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
        Logger::getInstance()->log(std::string("EventInterpreter::loadSubscription: could not find plugin for handler-name '") + handlerName + "'", lsWarning);
        Logger::getInstance()->log(       "EventInterpreter::loadSubscription: Still generating a subscription but w/o inner parameter", lsWarning);
      } else {
        opts.reset(plugin->createOptionsFromXML(_node));
        hadOpts = true;
      }
      try {
        Element* paramElem = elem->getChildElement("parameter");
        if(opts == NULL) {
          opts.reset(new SubscriptionOptions());
        }
        opts->loadParameterFromXML(paramElem);
      } catch(std::runtime_error& e) {
        // only delete options created in the try-part...
        if(!hadOpts) {
          opts.reset();
        }
      }

      boost::shared_ptr<EventSubscription> subscription(new EventSubscription(evtName, handlerName, *this, opts));
      try {
        Element* filterElem = elem->getChildElement("filter");
        loadFilter(filterElem, *subscription);
      } catch(std::runtime_error& e) {
      }

      subscribe(subscription);
    }
  } // loadSubsription


  //================================================== EventQueue

  EventQueue::EventQueue()
  : m_EventRunner(NULL)
  { } // ctor

  boost::shared_ptr<Schedule> EventQueue::scheduleFromEvent(boost::shared_ptr<Event> _event) {
    boost::shared_ptr<Schedule> result;
    if(_event->hasPropertySet(EventPropertyTime)) {
      DateTime when;
      bool validDate = false;
      std::string timeStr = _event->getPropertyByName(EventPropertyTime);
      if(timeStr.size() >= 2) {
        // relative time
        if(timeStr[0] == '+') {
          std::string timeOffset = timeStr.substr(1, std::string::npos);
          int offset = strToIntDef(timeOffset, -1);
          if(offset >= 0) {
            when = when.addSeconds(offset);
            validDate = true;
          } else {
            Logger::getInstance()->log(std::string("EventQueue::scheduleFromEvent: Could not parse offset or offset is below zero: '") + timeOffset + "'", lsError);
          }
        } else {
          try {
            when = DateTime::fromISO(timeStr);
            validDate = true;
          } catch(std::runtime_error& e) {
            Logger::getInstance()->log(std::string("EventQueue::scheduleFromEvent: Invalid time specified '") + timeStr + "' error: " + e.what(), lsError);
          }
        }
      }
      if(validDate) {
        Logger::getInstance()->log(std::string("EventQueue::scheduleFromEvent: Event has a valid time, rescheduling at ") + (std::string)when, lsInfo);
        result.reset(new StaticSchedule(when));
      } else {
        Logger::getInstance()->log("EventQueue::scheduleFromEvent: Dropping event with invalid time", lsError);
      }
    } else if(_event->hasPropertySet(EventPropertyICalStartTime) && _event->hasPropertySet(EventPropertyICalRRule)) {
      std::string timeStr = _event->getPropertyByName(EventPropertyICalStartTime);
      std::string rRuleStr = _event->getPropertyByName(EventPropertyICalRRule);
      Logger::getInstance()->log(std::string("EventQueue::schedleFromEvent: Event has a ICalRule rescheduling at ") + timeStr + " with Rule " + rRuleStr, lsInfo);
      result.reset(new ICalSchedule(rRuleStr, timeStr));
    }
    return result;
  } // scheduleFromEvent

  void EventQueue::pushEvent(boost::shared_ptr<Event> _event) {
    Logger::getInstance()->log(std::string("EventQueue: New event '") + _event->getName() + "' in queue...", lsInfo);
    boost::shared_ptr<Schedule> schedule = scheduleFromEvent(_event);
    if(schedule != NULL) {
      ScheduledEvent* scheduledEvent = new ScheduledEvent(_event, schedule);
      m_EventRunner->addEvent(scheduledEvent);
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
    for(boost::ptr_vector<ScheduledEvent>::iterator ipSchedEvt = m_ScheduledEvents.begin();
        ipSchedEvt != m_ScheduledEvents.end(); )
    {
      DateTime next = ipSchedEvt->getSchedule().getNextOccurence(now);
      if(DebugEventRunner) {
        Logger::getInstance()->log(std::string("next:   ") + (std::string)next);
        Logger::getInstance()->log(std::string("result: ") + (std::string)result);
      }
      if(next == DateTime::NullDate) {
        Logger::getInstance()->log("EventRunner: Removing event");
        ipSchedEvt = m_ScheduledEvents.erase(ipSchedEvt);
        continue;
      }
      result = std::min(result, next);
      if(DebugEventRunner) {
        Logger::getInstance()->log(std::string("chosen: ") + (std::string)result);
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
    std::ostringstream logSStream;
    DateTime virtualNow = _from.addSeconds(-_deltaSeconds/2);
    if(DebugEventRunner) {
      logSStream << "vNow:    " << virtualNow;
      Logger::getInstance()->log(logSStream.str());
      logSStream.str("");
    }

    m_EventsMutex.lock();
    for(boost::ptr_vector<ScheduledEvent>::iterator ipSchedEvt = m_ScheduledEvents.begin(), e = m_ScheduledEvents.end();
        ipSchedEvt != e; ++ipSchedEvt)
    {
      DateTime nextOccurence = ipSchedEvt->getSchedule().getNextOccurence(virtualNow);
      if(DebugEventRunner) {
        logSStream << "nextOcc: " << nextOccurence << "; "
                   << "diff:    " << nextOccurence.difference(virtualNow);
        Logger::getInstance()->log(logSStream.str());
        logSStream.str("");
      }
      if(abs(nextOccurence.difference(virtualNow)) <= _deltaSeconds/2) {
        result = true;
        if(m_EventQueue != NULL) {
          boost::shared_ptr<Event> evt = ipSchedEvt->getEvent();
          if(evt->hasPropertySet(EventPropertyTime)) {
            evt->unsetProperty(EventPropertyTime);
          }
          if(evt->hasPropertySet(EventPropertyICalStartTime)) {
            evt->unsetProperty(EventPropertyICalStartTime);
          }
          if(evt->hasPropertySet(EventPropertyICalRRule)) {
            evt->unsetProperty(EventPropertyICalRRule);
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

  EventSubscription::EventSubscription(const std::string& _eventName, const std::string& _handlerName, EventInterpreter& _interpreter, boost::shared_ptr<SubscriptionOptions> _options)
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

  const std::string& SubscriptionOptions::getParameter(const std::string& _name) const {
    return m_Parameters.get(_name);
  } // getParameter

  bool SubscriptionOptions::hasParameter(const std::string& _name) const {
    return m_Parameters.has(_name);
  } // hasParameter

  void SubscriptionOptions::setParameter(const std::string& _name, const std::string& _value) {
    m_Parameters.set(_name, _value);
  } // setParameter

  void SubscriptionOptions::loadParameterFromXML(Node* _node) {
    if(_node !=  NULL) {
      Node* curNode = _node->firstChild();
      while(curNode != NULL) {
        std::string nodeName = curNode->localName();
        if(nodeName == "parameter") {
          Element* elem = dynamic_cast<Element*>(curNode);
          if(elem != NULL) {
            std::string value;
            std::string name;
            if(curNode->hasChildNodes()) {
              value = curNode->firstChild()->getNodeValue();
            }
            name = elem->getAttribute("name");
            if(!name.empty()) {
              setParameter(name, value);
            }
          }
        }
        curNode = curNode->nextSibling();
      }
    }
  } // loadParameterFromXML


  //================================================== EventInterpreterPlugin

  EventInterpreterPlugin::EventInterpreterPlugin(const std::string& _name, EventInterpreter* _interpreter)
  : m_Name(_name),
    m_pInterpreter(_interpreter)
  { } // ctor

  SubscriptionOptions* EventInterpreterPlugin::createOptionsFromXML(Node* _node) {
    return NULL;
  } // createOptionsFromXML


  //================================================== EventPropertyFilter

  EventPropertyFilter::EventPropertyFilter(const std::string& _propertyName)
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

  EventPropertyExistsFilter::EventPropertyExistsFilter(const std::string& _propertyName)
  : EventPropertyFilter(_propertyName)
  { } // ctor

  bool EventPropertyExistsFilter::matches(const Event& _event) {
    return _event.hasPropertySet(getPropertyName());
  } // matches


  //================================================= EventPropertyMissingFilter

  EventPropertyMissingFilter::EventPropertyMissingFilter(const std::string& _propertyName)
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
  const char* EventPropertyICalStartTime = "iCalStartTime";
  const char* EventPropertyICalRRule = "iCalRRule";

} // namespace dss
