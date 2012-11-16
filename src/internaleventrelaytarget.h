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

#ifndef INTERNALEVENTRELAYTARGET_H_
#define INTERNALEVENTRELAYTARGET_H_

#include <boost/function.hpp>

#include "src/event.h"
#include "src/eventinterpreterplugins.h"

namespace dss {

  class InternalEventRelayTarget : public EventRelayTarget {
  public:
    InternalEventRelayTarget(EventInterpreterInternalRelay& _relay)
    : EventRelayTarget(_relay)
    { } // ctor

    virtual void handleEvent(Event& _event, const EventSubscription& _subscription) {
      if(m_Callback) {
        m_Callback(_event, _subscription);
      }
    }

    typedef boost::function<void (Event& _event, const EventSubscription& _subscription)> HandleEventCallBack;
    virtual void setCallback(HandleEventCallBack _callback) {
      m_Callback = _callback;
    }

  private:
    HandleEventCallBack m_Callback;
  }; // InternalEventRelayTarget

}

#endif
