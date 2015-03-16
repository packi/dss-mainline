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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "eventsubscriptionsession.h"

#include "src/event.h"
#include "src/session.h"
#include "src/eventinterpreterplugins.h"
#include "eventcollector.h"

namespace dss {

  /**
   * @_eventInterpreter
   * @subscribtion_id -- provided by client!!
   */
  EventSubscriptionSession::EventSubscriptionSession(EventInterpreter& _eventInterpreter,
                                                     int subscription_id)
  : m_EventInterpreter(_eventInterpreter),
    m_subscription_id(subscription_id)
  { }

  std::string EventSubscriptionSession::subscribe(const std::string& _eventName) {
    createCollector();
    // we do not want any double subscriptions here
    try {
      unsubscribe(_eventName);
    } catch(std::runtime_error& err) {}

    m_subscriptionMap[_eventName] = m_pEventCollector->subscribeTo(_eventName);
    return m_subscriptionMap[_eventName];
  }

  void EventSubscriptionSession::unsubscribe(const std::string& _eventName) {
    createCollector();
    std::map<std::string,std::string>::iterator entry = m_subscriptionMap.find(_eventName);
    if(entry != m_subscriptionMap.end()){
      m_pEventCollector->unsubscribeFrom(m_subscriptionMap[_eventName]);
    } else {
      throw std::runtime_error("Event " + _eventName + " is not subscribed in this session");
    }

    m_subscriptionMap.erase(_eventName);
  }

  void EventSubscriptionSession::createCollector() {
    if(m_pEventCollector == NULL) {
      EventInterpreterInternalRelay* pPlugin =
          dynamic_cast<EventInterpreterInternalRelay*>(
              m_EventInterpreter.getPluginByName(
                std::string(EventInterpreterInternalRelay::getPluginName())
              )
          );
      if(pPlugin == NULL) {
        throw std::runtime_error("Need EventInterpreterInternalRelay to be registered");
      }
      m_pEventCollector.reset(new EventCollector(*pPlugin));
    }
    assert(m_pEventCollector != NULL);
  }

  Event EventSubscriptionSession::popEvent() {
    createCollector();
    return m_pEventCollector->popEvent();
  }

  bool EventSubscriptionSession::hasEvent() {
    createCollector();
    return m_pEventCollector->hasEvent();
  }

  bool EventSubscriptionSession::waitForEvent(const int _timeoutMS) {
    createCollector();
    return m_pEventCollector->waitForEvent(_timeoutMS);
  }

} // namespace dss
