/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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

#include "eventcollector.h"
#include "dss.h"

namespace dss {

  EventCollector::EventCollector(EventInterpreterInternalRelay& _relay) : EventRelayTarget(_relay) {}

  EventCollector::~EventCollector() {
    while(!m_Subscriptions.empty()) {
      unsubscribeFrom(m_Subscriptions.front()->getID());
    }
  } 

  bool EventCollector::hasEvent() { 
    return !m_PendingEvents.empty(); 
  }

  void EventCollector::handleEvent(Event& _event, const EventSubscription& _subscription) {
    m_PendingEventsMutex.lock();
    m_PendingEvents.push_back(_event);
    m_PendingEventsMutex.unlock();
    m_EventArrived.signal();
  } // handleEvent

  bool EventCollector::waitForEvent(const int _timeoutMS) {
    if(m_PendingEvents.empty()) {
      if(_timeoutMS == 0) {
        m_EventArrived.waitFor();
      } else if(_timeoutMS > 0) {
        m_EventArrived.waitFor(_timeoutMS);
      }
    }
    return !m_PendingEvents.empty();
  } // waitForEvent

  Event EventCollector::popEvent() {
    Event result;
    m_PendingEventsMutex.lock();
    result = m_PendingEvents.front();
    m_PendingEvents.erase(m_PendingEvents.begin());
    m_PendingEventsMutex.unlock();
    return result;
  } // popEvent

  std::string EventCollector::subscribeTo(const std::string& _eventName) {
    boost::shared_ptr<EventSubscription> subscription(
      new dss::EventSubscription(
            _eventName,
            EventInterpreterInternalRelay::getPluginName(),
            DSS::getInstance()->getEventInterpreter(),
            boost::shared_ptr<dss::SubscriptionOptions>()
      )
    );

    EventRelayTarget::subscribeTo(subscription);
    m_Subscriptions.push_back(subscription);
    DSS::getInstance()->getEventInterpreter().subscribe(subscription);

    return subscription->getID();
  } // subscribeTo

  void EventCollector::unsubscribeFrom(const std::string& _subscriptionID)
  {
    for(size_t i = 0; i < m_Subscriptions.size(); i++){
      if(m_Subscriptions.at(i)->getID() == _subscriptionID){
        m_Subscriptions.erase(m_Subscriptions.begin()+i);
        break;
      }
    }

    EventRelayTarget::unsubscribeFrom(_subscriptionID);
    DSS::getInstance()->getEventInterpreter().unsubscribe(_subscriptionID);
  }
}

