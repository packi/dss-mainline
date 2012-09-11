/*
    Copyright (c) 2009,2012 digitalSTROM.org, Zurich, Switzerland

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

#include "event.h"

#include "logger.h"
#include "dss.h"
#include "propertysystem.h"
#include "subscription.h"

#include "foreach.h"
#include "src/model/apartment.h"
#include "src/model/zone.h"
#include "src/model/modelconst.h"
#include "src/model/device.h"
#include "src/model/devicereference.h"
#include "src/model/set.h"
#include "src/security/security.h"

#include <set>
#include <iostream>
#include <sstream>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread/locks.hpp>

using std::set;

namespace dss {

  //================================================== Event

  Event::Event(const std::string& _name)
  : m_Name(_name),
    m_RaiseLocation(erlApartment)
  {
    reset();
  } // ctor

  Event::Event(const std::string& _name, boost::shared_ptr<Zone> _zone)
  : m_Name(_name),
    m_RaiseLocation(erlGroup),
    m_RaisedAtGroup(_zone->getGroup(GroupIDBroadcast))
  {
    reset();
  } // ctor

  Event::Event(const std::string& _name, boost::shared_ptr<Group> _group)
  : m_Name(_name),
    m_RaiseLocation(erlGroup),
    m_RaisedAtGroup(_group)
  {
    reset();
  } // ctor

  Event::Event(const std::string& _name, boost::shared_ptr<DeviceReference> _reference)
  : m_Name(_name),
    m_RaiseLocation(erlDevice)
  {
    m_RaisedAtDevice = _reference;
    reset();
  }

  Event::Event()
  : m_Name("unnamed_event")
  {
    reset();
  }

  Event::~Event() {
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

  boost::shared_ptr<const Group> Event::getRaisedAtGroup(Apartment& _apartment) const {
    if(m_RaiseLocation == erlGroup) {
      return m_RaisedAtGroup;
    } else if(m_RaiseLocation == erlDevice) {
      boost::shared_ptr<const Device> dev = m_RaisedAtDevice->getDevice();
      return dev->getApartment().getZone(dev->getZoneID())->getGroup(dev->getGroupIdByIndex(0));
    } else {
      return _apartment.getZone(0)->getGroup(GroupIDBroadcast);
    }
  } // getRaisedAtZone

  bool Event::isReplacementFor(const dss::Event& _other) {
    bool sameName = getName() == _other.getName();
    bool sameContext = getPropertyByName("context") == _other.getPropertyByName("context");
    bool sameLocation = getPropertyByName("location") == _other.getPropertyByName("location");
    return sameName && sameContext && sameLocation;
  } // isReplacementFor

  void Event::applyProperties(const Properties& _others) {
    const HashMapStringString sourceMap = _others.getContainer();
    typedef const std::pair<const std::string, std::string> tItem;
    foreach(tItem kv, sourceMap) {
      setProperty(kv.first, kv.second);
    }
  } // applyProperties


  //================================================== EventInterpreter

  EventInterpreter::EventInterpreter(DSS* _pDSS)
  : Subsystem(_pDSS, "EventInterpreter"),
    Thread("EventInterpreter"),
    m_Queue(NULL),
    m_EventRunner(NULL),
    m_EventsProcessed(0)
  {
    if(DSS::hasInstance()) {
      getDSS().getPropertySystem().createProperty(getPropertyBasePath() + "eventsProcessed")
          ->linkToProxy(PropertyProxyPointer<int>(&m_EventsProcessed));
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
      getDSS().getPropertySystem().setStringValue(getConfigPropertyBasePath() + "subscriptiondir", getDSS().getConfigDirectory() + "subscriptions.d", true, false);

      PropertySystem subProperties;
      boost::shared_ptr<SubscriptionParserProxy> subParser(new SubscriptionParserProxy(
          subProperties.createProperty("/temp"),
          getDSS().getPropertySystem().createProperty("/usr/states")));

      if (!subParser) {
        log("Memory error while loading subscriptions", lsFatal);
        return;
      }

      std::string fileName = getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "subscriptionfile");
      bool ret = subParser->loadFromXML(fileName);
      if (!ret) {
        throw std::runtime_error("EventInterpreter::initialize: "
                                 "parse error in configuration file \"" + fileName + "\"");
      }

      if (boost::filesystem::is_directory((getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "subscriptiondir")))) {
        boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
        for (boost::filesystem::directory_iterator itr(getDSS().getPropertySystem().getStringValue(getConfigPropertyBasePath() + "subscriptiondir"));
             itr != end_itr;
             ++itr )
        {
          if (boost::filesystem::is_regular_file(itr->status()) &&  (itr->path().extension() == ".xml"))
          {
            subParser->reset();
#if defined(BOOST_VERSION_135)
            fileName = itr->path().file_string();
#else
            fileName = itr->path().string();
#endif
            ret = subParser->loadFromXML(fileName);
            if (!ret) {
              log("Parse error in configuration file \"" + fileName + "\"", lsFatal);
            }
          }
        }
      }

      // clear current subscription list
      {
        boost::mutex::scoped_lock lock(m_SubscriptionsMutex);
        m_Subscriptions.clear();
      }

      // reload subscriptions
      loadSubscriptionsFromProperty(subParser->getSubscriptionNode());
      loadStatesFromProperty(subParser->getStatesNode());
    }
  } // initialize

  void EventInterpreter::addPlugin(EventInterpreterPlugin* _plugin) {
    m_Plugins.push_back(_plugin);
  } // addPlugin

  void EventInterpreter::execute() {
    if(DSS::hasInstance()) {
      getDSS().getSecurity().loginAsSystemUser(
        "EventInterpreter needs to run as system user (for now)");
    }
    if(m_Queue == NULL) {
      log("No queue set. Can't work like that... exiting...", lsFatal);
      return;
    }

    if(m_EventRunner == NULL) {
      log("No runner set. exiting...", lsFatal);
      return;
    }
    while(!m_Terminated) {
      executePendingEvent();
    }
  } // execute

  void EventInterpreter::executePendingEvent() {
    if(m_Queue->waitForEvent()) {
      boost::shared_ptr<Event> toProcess = m_Queue->popEvent();
      if(toProcess != NULL) {
        PropertyNodePtr evtMonitor;

        log(std::string("Interpreter: got event from queue: '") + toProcess->getName() + "'");
        if (DSS::hasInstance()) {
           evtMonitor = getDSS().getPropertySystem().getProperty("/system/EventInterpreter");
           if (evtMonitor) {
             DateTime now;
             evtMonitor->createProperty("running/time")->setIntegerValue(now.secondsSinceEpoch());
             evtMonitor->createProperty("running/event")->setStringValue(toProcess->getName());
           }
        }

        for(HashMapStringString::const_iterator iParam = toProcess->getProperties().getContainer().begin(), e = toProcess->getProperties().getContainer().end();
            iParam != e; ++iParam)
        {
          log("Interpreter: - parameter '" + iParam->first + "' = '" + iParam->second + "'");
        }

        SubscriptionVector subscriptionsCopy;
        {
          boost::mutex::scoped_lock lock(m_SubscriptionsMutex);
          subscriptionsCopy = m_Subscriptions;
        }

        for(SubscriptionVector::iterator ipSubscription = subscriptionsCopy.begin(), e = subscriptionsCopy.end();
            ipSubscription != e; ++ipSubscription)
        {
          if((*ipSubscription)->matches(*toProcess)) {
            log(std::string("Interpreter: subscription '") + (*ipSubscription)->getID() + "' matches event");
            EventInterpreterPlugin* plugin = getPluginByName((*ipSubscription)->getHandlerName());
            if(plugin != NULL) {
              if (evtMonitor) {
                evtMonitor->createProperty("running/handler")->setStringValue((*ipSubscription)->getHandlerName());
              }
              try {
                plugin->handleEvent(*toProcess, **ipSubscription);
              } catch(std::runtime_error& e) {
                log(std::string("Interpreter: error while handling event: ") + e.what(), lsError);
              }
            }
            else {
              log(std::string("Interpreter: could not find handler '") + (*ipSubscription)->getHandlerName(), lsError);
            }

          }
        }

        if (evtMonitor) {
          evtMonitor->removeChild(evtMonitor->getProperty("running"));
        }

        m_EventsProcessed++;
      }
    }
  } // executePendingEvent

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
    boost::mutex::scoped_lock lock(m_SubscriptionsMutex);
    m_Subscriptions.push_back(_subscription);
  } // subscribe

  void EventInterpreter::unsubscribe(const std::string& _subscriptionID) {
    boost::mutex::scoped_lock lock(m_SubscriptionsMutex);
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
    boost::mutex::scoped_lock lock(m_SubscriptionsMutex);
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

  void EventInterpreter::loadSubscriptionsFromProperty(PropertyNodePtr _node) {
    try {
      PropertyNodePtr sProp = _node;
      for (int i = 0; sProp && i < sProp->getChildCount(); i++ ) {
        loadSubscription(sProp->getChild(i));
      }
    } catch (SecurityException& e) {
      log(std::string("Security error loading subscriptions: ") + e.what(), lsError);
    } catch (ItemNotFoundException& e) {
      log(std::string("Error loading subscriptions: ") + e.what(), lsError);
    }
  } // loadSubscriptionsFromProperty

  void EventInterpreter::loadStatesFromProperty(PropertyNodePtr _node) {
    try {
      PropertyNodePtr eProp = _node;
      for (int i = 0; eProp && i < eProp->getChildCount(); i++ ) {
        loadState(eProp->getChild(i));
      }
    } catch (SecurityException& e) {
      log(std::string("Security error loading states: ") + e.what(), lsError);
    } catch (ItemNotFoundException& e) {
      log(std::string("Error loading states: ") + e.what(), lsError);
    }
  } // loadStatesFromProperty

  void EventInterpreter::loadFilter(PropertyNodePtr _node, EventSubscription& _subscription) {
    if (_node) {
      std::string matchType = _node->getStringValue();
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
      for (int i = 0; i < _node->getChildCount(); i++) {
        PropertyNodePtr fProp = _node->getChild(i);
        EventPropertyFilter* filter = NULL;
        std::string fType;
        std::string fValue;
        std::string fProperty;
        if (fProp->getProperty("type")) {
          fType = fProp->getProperty("type")->getStringValue();
        }
        if (fProp->getProperty("value")) {
          fValue = fProp->getProperty("value")->getStringValue();
        }
        if (fProp->getProperty("property")) {
          fProperty = fProp->getProperty("property")->getStringValue();
        }
        if (fType.length() && fProperty.length()) {
          if (fType == "exists") {
            filter = new EventPropertyExistsFilter(fProperty);
          } else if (fType == "missing") {
            filter = new EventPropertyMissingFilter(fProperty);
          } else if (fType == "matches") {
            filter = new EventPropertyMatchFilter(fProperty, fValue);
          } else {
            log("Unknown property-filter type: " + fType, lsError);
          }
        }
        if(filter != NULL) {
          _subscription.addPropertyFilter(filter);
        }
      }
    }
  } // loadFilter

  void EventInterpreter::loadSubscription(PropertyNodePtr _node) {
    if (_node) {
      std::string evtName = _node->getProperty("event-name")->getStringValue();
      std::string handlerName = _node->getProperty("handler-name")->getStringValue();

      if(evtName.size() == 0) {
        log("loadSubscription: empty event-name, skipping this subscription", lsWarning);
        return;
      }

      if(handlerName.size() == 0) {
        log("loadSubscription: empty handler-name, skipping this subscription", lsWarning);
        return;
      }

      boost::shared_ptr<SubscriptionOptions> opts;
      bool hadOpts = false;

      EventInterpreterPlugin* plugin = getPluginByName(handlerName);
      if(plugin == NULL) {
        log(std::string("loadSubscription: could not find plugin for handler-name '") + handlerName + "'", lsWarning);
        log(       "loadSubscription: Still generating a subscription but w/o inner parameter", lsWarning);
      } else {
        opts = plugin->createOptionsFromProperty(_node);
        hadOpts = true;
      }
      try {
        if(opts == NULL) {
          opts.reset(new SubscriptionOptions());
        }
        opts->loadParameterFromProperty(_node->getPropertyByName("parameter"));
      } catch(std::runtime_error& e) {
        // only delete options created in the try-part...
        if(!hadOpts) {
          opts.reset();
        }
      }

      boost::shared_ptr<EventSubscription> subscription(new EventSubscription(evtName, handlerName, *this, opts));
      try {
        loadFilter(_node->getPropertyByName("filter"), *subscription);
      } catch(std::runtime_error& e) {
      }

      subscribe(subscription);
    }
  } // loadSubsription

  void EventInterpreter::loadState(PropertyNodePtr _node) {
    if (_node) {
      std::string evtName = _node->getProperty("name")->getStringValue();
      if (evtName.size() == 0) {
        log("loadState: empty state name, skipping this state", lsWarning);
        return;
      }

      _node->setFlag(PropertyNode::Writeable, false);
      for (int i = 0; i < _node->getChildCount(); i++) {
        _node->getChild(i)->setFlag(PropertyNode::Readable, true);
      }

      PropertyNodePtr pValue = _node->getPropertyByName("value");
      PropertyNodePtr pScript = _node->getPropertyByName("script_id");
      PropertyNodePtr pPersistent = _node->getPropertyByName("persistent");
      if (pPersistent != NULL && pScript != NULL) {
        pValue->setFlag(PropertyNode::Archive, true);
        std::string filename = DSS::getInstance()->getSavedPropsDirectory() + "/" +
            pScript->getStringValue() + "_" + evtName + ".xml";
        DSS::getInstance()->getPropertySystem().loadFromXML(filename, _node);
      }
    }
  } // loadState


  //================================================== EventQueue

  EventQueue::EventQueue(Subsystem* _subsystem, const int _eventTimeoutMS)
  : m_Subsystem(_subsystem),
    m_EventRunner(NULL),
    m_EventTimeoutMS(_eventTimeoutMS),
    m_ScheduledEventCounter(0)
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
          if(offset > 0) {
            when = when.addSeconds(offset);
            validDate = true;
          } else if(offset == 0) {
            log(std::string("scheduleFromEvent: offset == 0, schedule event directly"), lsInfo);
            return result;
          } else {
            log(std::string("scheduleFromEvent: could not parse offset or offset is below zero: '") + timeOffset + "'", lsError);
          }
        } else {
          try {
            when = DateTime::fromISO(timeStr);
            validDate = true;
          } catch(std::exception& e) {
            log(std::string("scheduleFromEvent: invalid time specified '") + timeStr + "' error: " + e.what(), lsError);
          }
        }
      }
      if(validDate) {
        log(std::string("scheduleFromEvent: event has a valid time, rescheduling at ") + (std::string)when, lsDebug);
        result.reset(new StaticSchedule(when));
      } else {
        log("scheduleFromEvent: dropping event with invalid time", lsError);
      }
    } else if(_event->hasPropertySet(EventPropertyICalStartTime) && _event->hasPropertySet(EventPropertyICalRRule)) {
      std::string timeStr = _event->getPropertyByName(EventPropertyICalStartTime);
      std::string rRuleStr = _event->getPropertyByName(EventPropertyICalRRule);
      log(std::string("scheduleFromEvent: Event \"" + _event->getName() + "\" has a ICalRule rescheduling at ") + timeStr + " with Rule " + rRuleStr, lsDebug);
      result.reset(new ICalSchedule(rRuleStr, timeStr));
    }
    return result;
  } // scheduleFromEvent

  void EventQueue::pushEvent(boost::shared_ptr<Event> _event) {
    log(std::string("Queue: new event '") + _event->getName() + "' in queue...", lsDebug);
    boost::shared_ptr<Schedule> schedule = scheduleFromEvent(_event);
    if(schedule != NULL) {
      ScheduledEvent* scheduledEvent = new ScheduledEvent(_event, schedule, m_ScheduledEventCounter++);
      assert(m_EventRunner != NULL);
      m_EventRunner->addEvent(scheduledEvent);
    } else {
      bool addToQueue = true;
      if(!_event->getPropertyByName("unique").empty()) {
        boost::mutex::scoped_lock lock(m_QueueMutex);

        foreach(boost::shared_ptr<Event> pEvent, m_EventQueue) {
          if(_event->isReplacementFor(*pEvent)) {
              pEvent->setProperties(_event->getProperties());
              if(_event->hasPropertySet("time")) {
                pEvent->setTime(_event->getPropertyByName("time"));
              }
              addToQueue = false;
              break;
          }
        }
      }
      if(addToQueue) {
        boost::mutex::scoped_lock lock(m_QueueMutex);
        m_EventQueue.push_back(_event);
        lock.unlock();
        m_EntryInQueueEvt.signal();
      }
    }
  } // pushEvent

  std::string EventQueue::pushTimedEvent(boost::shared_ptr<Event> _event) {
    boost::shared_ptr<Schedule> schedule = scheduleFromEvent(_event);
    if(schedule != NULL) {
      ScheduledEvent* scheduledEvent = new ScheduledEvent(_event, schedule, m_ScheduledEventCounter++);
      std::string result = scheduledEvent->getID();
      log("Queue: new timed-event '" + result + "' in queue...", lsInfo);
      m_EventRunner->addEvent(scheduledEvent);
      return result;
    }
    log("Queue: failed to schedule timed-event '" + _event->getName(), lsError);
    return std::string();
  } // pushTimedEvent

  boost::shared_ptr<Event> EventQueue::popEvent() {
    boost::mutex::scoped_lock lock(m_QueueMutex);
    boost::shared_ptr<Event> result;
    if(!m_EventQueue.empty()) {
      result = m_EventQueue.front();
      m_EventQueue.pop_front();
    }
    return result;
  } // popEvent

  bool EventQueue::waitForEvent() {
    if(m_EventQueue.empty()) {
      m_EntryInQueueEvt.waitFor(m_EventTimeoutMS);
    }
    return !m_EventQueue.empty();
  } // waitForEvent

  void EventQueue::shutdown() {
    m_EntryInQueueEvt.broadcast();
  } // shutdown

  void EventQueue::log(const std::string& message, aLogSeverity severity) {
      if(m_Subsystem != NULL)
          m_Subsystem->log(message, severity);
  } //log


  //================================================== EventRunner

  const bool DebugEventRunner = false;

  EventRunner::EventRunner(Subsystem* _subsystem, PropertyNodePtr _monitorNode)
  : m_Subsystem(_subsystem),
    m_EventQueue(NULL),
    m_ShutdownFlag(false),
    m_MonitorNode(_monitorNode)
  {
    if (_monitorNode != NULL) {
      _monitorNode->addListener(this);
    }
  } // ctor

  void EventRunner::shutdown() {
    m_ShutdownFlag = true;
    m_NewItem.broadcast();
  }

  size_t EventRunner::getSize() const {
    boost::mutex::scoped_lock lock(m_EventsMutex);
    return m_ScheduledEvents.size();
  } // getSize

  void EventRunner::propertyRemoved(PropertyNodePtr _parent,
                                    PropertyNodePtr _child) {
    if (DebugEventRunner) {
      log("Runner: property listener detected removal of \"" + _child->getName() + "\"", lsDebug);
    }
    removeEventInternal(_child->getName());
  }

  void EventRunner::removeEvent(const std::string& _eventID) {
    if (m_MonitorNode != NULL) {
      PropertyNodePtr child = m_MonitorNode->getProperty(_eventID);
      if (child != NULL) {
          m_MonitorNode->removeChild(child);
      } else {
        removeEventInternal(_eventID);
      }
    } else {
      removeEventInternal(_eventID);
    }
  } // removeEvent

  void EventRunner::removeEventInternal(const std::string& _eventID) {
    boost::mutex::scoped_lock lock(m_EventsMutex);
    boost::ptr_vector<ScheduledEvent>::iterator it;
    for (it = m_ScheduledEvents.begin(); it != m_ScheduledEvents.end(); it++) {
      if (it->getID() == _eventID) {
        if (DebugEventRunner) {
          log("Runner: remove event \"" + it->getName() + "\" with id " + _eventID, lsDebug);
        }
        m_ScheduledEvents.erase(it);
        return;
     }
    }
    log("Runner: cannot remove event with id " + _eventID + ", not found in queue", lsWarning);
  } // removeEventInternal

  const ScheduledEvent& EventRunner::getEvent(const std::string& _eventID) const {
    boost::mutex::scoped_lock lock(m_EventsMutex);
    for (size_t i = 0; i < m_ScheduledEvents.size(); i++) {
      if (m_ScheduledEvents.at(i).getID() == _eventID) {
        return m_ScheduledEvents.at(i);
      }
    }
    throw std::runtime_error("Event with id '" + _eventID + "' not found");
  } // getEvent

  std::vector<std::string> EventRunner::getEventIDs() const {
    std::vector<std::string> ids;
    boost::mutex::scoped_lock lock(m_EventsMutex);
    for (size_t i = 0; i < m_ScheduledEvents.size(); i++) {
      ids.push_back(m_ScheduledEvents.at(i).getID());
    }
    return ids;
  }

  void EventRunner::addEvent(ScheduledEvent* _scheduledEvent) {
    std::string id = _scheduledEvent->getID();

    boost::mutex::scoped_lock lock(m_EventsMutex);
    bool addToQueue = true;
    if(!_scheduledEvent->getEvent()->getPropertyByName("unique").empty()) {
      foreach(ScheduledEvent& scheduledEvent, m_ScheduledEvents) {
        if(_scheduledEvent->getEvent()->isReplacementFor(*scheduledEvent.getEvent())) {
          scheduledEvent.getEvent()->setProperties(_scheduledEvent->getEvent()->getProperties());
          id = scheduledEvent.getID();
          if(_scheduledEvent->getEvent()->hasPropertySet("time")) {
            scheduledEvent.getEvent()->setTime(_scheduledEvent->getEvent()->getPropertyByName("time"));
          }
          addToQueue = false;
          if( DebugEventRunner) {
            log("Runner: found target for unique event, not adding it to the queue");
          }
          break;
        }
      }
    }
    if(addToQueue) {
      if (m_MonitorNode != NULL) {
        m_MonitorNode->createProperty(id + "/id")->setStringValue(id);
        m_MonitorNode->createProperty(id + "/name")->setStringValue(
                                    _scheduledEvent->getEvent()->getName());
      }

      m_ScheduledEvents.push_back(_scheduledEvent);
    } else {
      delete _scheduledEvent;
    }
    lock.unlock();
    m_NewItem.signal();
  } // addEvent

  void EventRunner::run() {
    while(!m_ShutdownFlag) {
      raisePendingEvents();
      m_NewItem.waitFor(500);
    }
  } // run

  bool EventRunner::raisePendingEvents() {
    bool result = false;
    DateTime now;
    DateTime nextSecond = now.addSeconds(1);

    std::vector<std::string> removeIDs;
    boost::mutex::scoped_lock lock(m_EventsMutex);
    for(boost::ptr_vector<ScheduledEvent>::iterator ipSchedEvt = m_ScheduledEvents.begin(), e = m_ScheduledEvents.end();
        ipSchedEvt != e; ++ipSchedEvt)
    {
      DateTime nextOccurence = ipSchedEvt->getSchedule().getNextOccurence(now);

      if(nextOccurence == DateTime::NullDate) {
        log("Runner: event " + ipSchedEvt->getID() + " removed");
        removeIDs.push_back(ipSchedEvt->getID());
        continue;
      }

      if (m_MonitorNode) {
        if (NULL == m_MonitorNode->getProperty(ipSchedEvt->getID() + "/time")) {
          m_MonitorNode->createProperty(ipSchedEvt->getID() + "/time")->setStringValue(nextOccurence.toString());
        }
        m_MonitorNode->createProperty(ipSchedEvt->getID() + "/ticks")->setIntegerValue(nextOccurence.difference(now));
      }

      if(nextOccurence.before(now) || (nextOccurence == now)) {
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

          // check if the event will be raised again
          if(ipSchedEvt->getSchedule().getNextOccurence(nextSecond) == DateTime::NullDate) {
            log("Runner: event " + ipSchedEvt->getID() + " has no further schedule");
            removeIDs.push_back(ipSchedEvt->getID());
          }
        } else {
          log("Runner: cannot push event back to queue because the queue is NULL", lsFatal);
        }
      }
    }
    lock.unlock();
    for(size_t iID = 0; iID < removeIDs.size(); iID++) {
      removeEvent(removeIDs.at(iID));
    }
    return result;
  } // raisePendingEvents

  void EventRunner::log(const std::string& message, aLogSeverity severity) {
      if(m_Subsystem != NULL)
          m_Subsystem->log(message, severity);
  } //log


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

  void SubscriptionOptions::loadParameterFromProperty(PropertyNodePtr _node) {
    if(_node !=  NULL) {
      for (int i = 0; i < _node->getChildCount(); i++) {
        PropertyNodePtr param = _node->getChild(i);
        setParameter(param->getName(), param->getStringValue());
      }
    }
  } // loadParameterFromProperty


  //================================================== EventInterpreterPlugin

  EventInterpreterPlugin::EventInterpreterPlugin(const std::string& _name, EventInterpreter* _interpreter)
  : m_Name(_name),
    m_pInterpreter(_interpreter)
  { } // ctor

  boost::shared_ptr<SubscriptionOptions> EventInterpreterPlugin::createOptionsFromProperty(PropertyNodePtr _node) {
    return boost::shared_ptr<SubscriptionOptions>();
  } // createOptionsFromXML

  void EventInterpreterPlugin::log(const std::string& _message, aLogSeverity _severity) {
    m_pInterpreter->log(_message, _severity);
  }

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

  //================================================= ScheduledEvent

  ScheduledEvent::ScheduledEvent(boost::shared_ptr<Event> _pEvt,
                                 boost::shared_ptr<Schedule> _pSchedule,
                                 unsigned long int _counterID) :
                                    m_Event(_pEvt),
                                    m_Schedule(_pSchedule)
                                 {
    m_EventID = uintToString(_counterID) + "-" +
                uintToString(static_cast<long unsigned int>(DateTime().secondsSinceEpoch())) + '_' + _pEvt->getName();
  } // ScheduledEvent

  //================================================== External consts

  const char* EventPropertyName = "name";
  const char* EventPropertyLocation = "location";
  const char* EventPropertyContext = "context";
  const char* EventPropertyTime = "time";
  const char* EventPropertyICalStartTime = "iCalStartTime";
  const char* EventPropertyICalRRule = "iCalRRule";

} // namespace dss
