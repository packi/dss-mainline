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

#include "eventsubscriptionsession.h"

#include "core/event.h"
#include "core/web/json.h"
#include "core/session.h"
#include "core/eventinterpreterplugins.h"
#include "eventcollector.h"

namespace dss {

  EventSubscriptionSession::EventSubscriptionSession(EventInterpreter& _eventInterpreter,
                                                     boost::shared_ptr<Session> _parentSession)
  : m_parentSession(_parentSession),
    m_EventInterpreter(_eventInterpreter)
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

  boost::shared_ptr<JSONObject> EventSubscriptionSession::getEvents(const int _timeoutMS)  {
    createCollector();

    if(m_parentSession != NULL) {
      m_parentSession->use();
    }
    m_pEventCollector->waitForEvent(_timeoutMS);

    if(m_parentSession != NULL) {
      m_parentSession->unuse();
    }

    boost::shared_ptr<JSONObject> resultObj(new JSONObject());
    boost::shared_ptr<JSONArrayBase> eventsArray(new JSONArrayBase);
    resultObj->addElement("events", eventsArray);

    while(m_pEventCollector->hasEvent()) {
      Event evt = m_pEventCollector->popEvent();
      boost::shared_ptr<JSONObject> evtObj(new JSONObject());
      eventsArray->addElement("event", evtObj);

      evtObj->addProperty("name", evt.getName());
      boost::shared_ptr<JSONObject> evtprops(new JSONObject());
      evtObj->addElement("properties", evtprops);

      const dss::HashMapConstStringString& props =  evt.getProperties().getContainer();
      for(dss::HashMapConstStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam)
      {
        evtprops->addProperty(iParam->first, iParam->second);
      }
    }

    return resultObj;
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
    return m_pEventCollector->popEvent();
  }

  bool EventSubscriptionSession::hasEvent() {
    return m_pEventCollector->hasEvent();
  }

  bool EventSubscriptionSession::waitForEvent(const int _timeoutMS) {
    return m_pEventCollector->waitForEvent(_timeoutMS);
  }

} // namespace dss
