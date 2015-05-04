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

#include "eventrequesthandler.h"

#include "src/base.h"

#include "src/eventcollector.h"
#include "src/eventinterpreterplugins.h"
#include "src/eventsubscriptionsession.h"
#include "src/stringconverter.h"

#include "src/model/apartment.h"
#include "src/model/group.h"
#include "src/model/device.h"

namespace dss {

  //=========================================== EventRequestHandler

  EventRequestHandler::EventRequestHandler(EventInterpreter& _interpreter)
  : m_EventInterpreter(_interpreter)
  { }

  std::string EventRequestHandler::raise(const RestfulRequest& _request) {
    StringConverter st("UTF-8", "UTF-8");
    std::string name = st.convert(_request.getParameter("name"));
    std::string location = st.convert(_request.getParameter("location"));
    std::string context = st.convert(_request.getParameter("context"));
    std::string parameter = _request.getParameter("parameter");

    if (name.empty()) {
      return JSONWriter::failure("Missing event name!");
    }

    boost::shared_ptr<Event> evt = boost::make_shared<Event>(name);
    if (!context.empty()) {
      evt->setContext(context);
    }
    if (!location.empty()) {
      evt->setLocation(location);
    }
    std::vector<std::string> params = dss::splitString(parameter, ';');
    for (std::vector<std::string>::iterator iParam = params.begin(), e = params.end();
        iParam != e; ++iParam)
    {
      std::string key;
      std::string value;
      std::string& keyValue = *iParam;
      boost::tie(key, value) = splitIntoKeyValue(keyValue);
      if (!key.empty() && !value.empty()) {
        dss::Logger::getInstance()->log("EventRequestHandler::raise: Got parameter '" + key + "'='" + value + "'");
        evt->setProperty(st.convert(key), st.convert(value));
      } else {
        return JSONWriter::failure("Invalid parameter found: " + keyValue);
      }
    }

    m_EventInterpreter.getQueue().pushEvent(evt);
    return JSONWriter::success();
  }

  int EventRequestHandler::validateArgs(boost::shared_ptr<Session> _session,
                                     const std::string &name,
                                     const std::string &subscribtionID) {
    int token;
    if (_session == NULL) {
      throw std::runtime_error("Invalid session");
    }
    if (name.empty()) {
      throw std::runtime_error("Missing event name");
    }
    if (subscribtionID.empty()) {
      throw std::runtime_error("Missing event subscription id");
    }
    try {
      token = strToInt(subscribtionID);
    } catch (std::invalid_argument& err) {
      throw std::runtime_error(std::string("Invalid event subscription id ") + subscribtionID);
    }
    return token;
  }

  // name=EventName&sid=EventSubscriptionID
  std::string EventRequestHandler::subscribe(const RestfulRequest& _request,
                                             boost::shared_ptr<Session> _session) {
    StringConverter st("UTF-8", "UTF-8");
    std::string name = st.convert(_request.getParameter("name"));
    std::string tokenStr = _request.getParameter("subscriptionID");
    int token;

    try {
      token = validateArgs(_session, name, tokenStr);
    } catch (std::runtime_error e) {
      return JSONWriter::failure(e.what());
    }

    boost::mutex::scoped_lock lock(m_Mutex);
    boost::shared_ptr<EventSubscriptionSession> subscription =
      _session->createEventSubscription(m_EventInterpreter, token);
    subscription->subscribe(name);
    return JSONWriter::success();
  }

  // name=EventName&sid=EventSubscriptionID
  std::string EventRequestHandler::unsubscribe(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    StringConverter st("UTF-8", "UTF-8");
    std::string name = st.convert(_request.getParameter("name"));
    std::string subscribtionID = _request.getParameter("subscriptionID");
    int token;

    try {
      token = validateArgs(_session, name, subscribtionID);
    } catch (std::runtime_error e) {
      return JSONWriter::failure(e.what());
    }

    boost::mutex::scoped_lock lock(m_Mutex);
    boost::shared_ptr<EventSubscriptionSession> sub;
    try {
      sub = _session->getEventSubscription(token);
      sub->unsubscribe(name);
    } catch (std::exception& e) {
      // TODO still erase the subscription
      return JSONWriter::failure(e.what());
    }

    _session->deleteEventSubscription(sub);
    return JSONWriter::success();
  }


  std::string EventRequestHandler::buildEventResponse(boost::shared_ptr<EventSubscriptionSession> _subscriptionSession) {
    JSONWriter json;
    json.startArray("events");

    while (_subscriptionSession->hasEvent()) {
      Event evt = _subscriptionSession->popEvent();
      json.startObject();

      json.add("name", evt.getName());
      json.startObject("properties");

      const dss::HashMapStringString& props =  evt.getProperties().getContainer();
      for (dss::HashMapStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam) {
        json.add(iParam->first, iParam->second);
      }
      json.endObject();

      json.startObject("source");

      EventRaiseLocation raiseLocation = evt.getRaiseLocation();
      if ((raiseLocation == erlGroup) || (raiseLocation == erlApartment)) {
        if (DSS::hasInstance()) {
          boost::shared_ptr<const Group> group =
              evt.getRaisedAtGroup(DSS::getInstance()->getApartment());
          json.add("set", ".zone(" + intToString(group->getZoneID())+
                  ").group(" + intToString(group->getID()) + ")");
          json.add("groupID", group->getID());
          json.add("zoneID", group->getZoneID());
          json.add("isApartment", raiseLocation == erlApartment);
          json.add("isGroup", raiseLocation == erlGroup);
          json.add("isDevice", false);
        }
      } else if (raiseLocation == erlDevice) {
        boost::shared_ptr<const DeviceReference> device = evt.getRaisedAtDevice();
        try {
          json.add("set", "dsid(" + dsuid2str(device->getDSID()) + ")");
          json.add("dsid", dsuid2str(device->getDSID()));
          json.add("zoneID", device->getDevice()->getZoneID());
        } catch (ItemNotFoundException& e) {
        }
        json.add("isApartment", false);
        json.add("isGroup", false);
        json.add("isDevice", true);
      } else if (raiseLocation == erlState) {
        boost::shared_ptr<const State> state = evt.getRaisedAtState();
        if (state->getType() == StateType_Device) {
          boost::shared_ptr<Device> device = state->getProviderDevice();
          dsid_t dsid;
          if (dsuid_to_dsid(device->getDSID(), &dsid)) {
            json.add("dsid", dsid2str(dsid));
          } else {
            json.add("dsid", "");
          }
          json.add("dSUID", dsuid2str(device->getDSID()));
          json.add("zoneID", device->getZoneID());
          json.add("isApartment", false);
          json.add("isGroup", false);
          json.add("isDevice", true);
        } else if (state->getType() == StateType_Apartment) {
          json.add("isApartment", true);
          json.add("isGroup", false);
          json.add("isDevice", false);
        } else if (state->getType() == StateType_Group) {
          boost::shared_ptr<Group> group = state->getProviderGroup();
          json.add("isApartment", false);
          json.add("isGroup", true);
          json.add("isDevice", false);
          json.add("groupID", group->getID());
          json.add("zoneID", group->getZoneID());
        } else if (state->getType() == StateType_Service) {
          json.add("isService", true);
          json.add("isGroup", false);
          json.add("isDevice", false);
          json.add("serviceName", state->getProviderService());
        } else if (state->getType() == StateType_Script) {
          json.add("isScript", true);
          json.add("isGroup", false);
          json.add("isDevice", false);
          json.add("serviceName", state->getProviderService());
        }
      }
      json.endObject();
      json.endObject();
    }
    json.endArray();

    return json.successJSON();
  } // buildEventResponse

  // sid=SubscriptionID&timeout=0
  std::string EventRequestHandler::get(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    std::string tokenStr = _request.getParameter("subscriptionID");
    std::string timeoutStr = _request.getParameter("timeout");
    int timeout = 0;
    int token;

    try {
      token = validateArgs(_session, "sentinel", tokenStr);
    } catch (std::runtime_error e) {
      return JSONWriter::failure(e.what());
    }

    if (!timeoutStr.empty()) {
      try {
        timeout = strToInt(timeoutStr);
      } catch (std::invalid_argument& err) {
        return JSONWriter::failure("Could not parse timeout parameter!");
      }
    }

    boost::mutex::scoped_lock lock(m_Mutex);
    boost::shared_ptr<EventSubscriptionSession> subscriptionSession;
    try {
      subscriptionSession = _session->getEventSubscription(token);
    } catch (std::runtime_error& e) {
      return JSONWriter::failure(e.what());
    }

    _session->use();

    bool haveEvents = false;
    bool timedOut = false;
    const int kSocketDisconnectTimeoutMS = 200;
    int timeoutMSLeft = timeout;
    while (!timedOut && !haveEvents) {
      // check if we're still connected
      if (!_request.isActive()) {
        Logger::getInstance()->log("EventRequestHanler::get: connection dropped");
        break;
      }
      // calculate the length of our wait
      int waitTime;
      if (timeout == -1) {
        timedOut = true;
        waitTime = -1;
      } else if (timeout != 0) {
        waitTime = std::min(timeoutMSLeft, kSocketDisconnectTimeoutMS);
        timeoutMSLeft -= waitTime;
        timedOut = (timeoutMSLeft == 0);
      } else {
        waitTime = kSocketDisconnectTimeoutMS;
        timedOut = false;
      }
      // wait for the event
      haveEvents = subscriptionSession->waitForEvent(waitTime);
    }

    _session->unuse();

    return buildEventResponse(subscriptionSession);
  }

  WebServerResponse EventRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    if (_request.getMethod() == "raise") {
      return raise(_request);
    } else if (_request.getMethod() == "subscribe") {
      return subscribe(_request, _session);
    } else if (_request.getMethod() == "unsubscribe") {
      return unsubscribe(_request, _session);
    } else if (_request.getMethod() == "get") {
      return get(_request, _session);
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest

} // namespace dss
