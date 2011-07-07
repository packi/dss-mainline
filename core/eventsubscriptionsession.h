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

#ifndef EVENTSUBSCRIPTIONSESSION_H
#define EVENTSUBSCRIPTIONSESSION_H

#include <map>
#include <deque>
#include <string>

#include <boost/shared_array.hpp>

#include "core/event.h"

namespace dss {

  class EventInterpreter;
  class Session;
  class EventCollector;
  class JSONObject;

  class EventSubscriptionSession {
  public:
    EventSubscriptionSession(EventInterpreter& _eventInterpreter, boost::shared_ptr<Session> _parentSession);

    std::string subscribe(const std::string& _eventName);
    void unsubscribe(const std::string& _eventName);
    Event popEvent();
    bool hasEvent();
    bool waitForEvent(const int _timeoutMS);
    // blocks if no events are available
  private:
    boost::shared_ptr<Session> m_parentSession;
    std::deque<Event> m_events;
    boost::shared_ptr<EventCollector> m_pEventCollector;
    void createCollector();
    // name, subscriptionID
    std::map<std::string, std::string> m_subscriptionMap;
    EventInterpreter& m_EventInterpreter;
  };

};

#endif
