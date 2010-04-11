/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Authors: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>,
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

#ifndef __EVENT_COLLECTOR_H__
#define __EVENT_COLLECTOR_H__

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>

#include "core/event.h"
#include "core/eventinterpreterplugins.h"
#include "core/mutex.h"
#include "core/syncevent.h"


namespace dss {
  class EventCollector : public EventRelayTarget {
  public:
    EventCollector(EventInterpreterInternalRelay& _relay);
    ~EventCollector();

    virtual void handleEvent(Event& _event, const EventSubscription& _subscription);

    bool waitForEvent(const int _timeoutMS);
    Event popEvent();
    bool hasEvent();

    virtual std::string subscribeTo(const std::string& _eventName);
    virtual void unsubscribeFrom(const std::string& _subscriptionID);

  private:
    SyncEvent m_EventArrived;
    Mutex m_PendingEventsMutex;
    std::vector<Event> m_PendingEvents;
    std::vector<boost::shared_ptr<EventSubscription> > m_Subscriptions;
  }; // EventCollector
} // dss namespace

#endif//__EVENT_COLLECTOR_H__
