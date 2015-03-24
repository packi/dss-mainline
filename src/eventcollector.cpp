/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
             Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org>

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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "eventcollector.h"

namespace dss {

  EventCollector::EventCollector(EventInterpreterInternalRelay& _relay) : EventRelayTarget(_relay) {}

  bool EventCollector::hasEvent() {
    return !m_PendingEvents.empty();
  }

  void EventCollector::handleEvent(Event& _event, const EventSubscription& _subscription) {
    {
      boost::mutex::scoped_lock lock(m_PendingEventsMutex);
      m_PendingEvents.push_back(_event);
    }
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
    boost::mutex::scoped_lock lock(m_PendingEventsMutex);
    result = m_PendingEvents.front();
    m_PendingEvents.erase(m_PendingEvents.begin());
    return result;
  } // popEvent

  std::string EventCollector::subscribeTo(const std::string& _eventName) {
    boost::shared_ptr<EventSubscription> subscription(
      new dss::EventSubscription(
            _eventName,
            EventInterpreterInternalRelay::getPluginName(),
            getRelay().getEventInterpreter(),
            boost::shared_ptr<SubscriptionOptions>()
      )
    );

    EventRelayTarget::subscribeTo(subscription);

    return subscription->getID();
  } // subscribeTo

}

