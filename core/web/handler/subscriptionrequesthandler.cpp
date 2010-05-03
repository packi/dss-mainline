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

#include "subscriptionrequesthandler.h"

#include "core/web/json.h"

#include "core/event.h"
#include "core/base.h"
#include "core/foreach.h"

namespace dss {

  //=========================================== EventRequestHandler

  SubscriptionRequestHandler::SubscriptionRequestHandler(EventInterpreter& _interpreter)
  : m_EventInterpreter(_interpreter)
  { }

  boost::shared_ptr<JSONObject> SubscriptionRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    if(_request.getMethod() == "list") {
      return list(_request, _session);
    } else if(_request.getMethod() == "remove") {
      return remove(_request, _session);
    } else if(_request.getMethod() == "add") {
      return add(_request, _session);
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest

  boost::shared_ptr<JSONObject> SubscriptionRequestHandler::list(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    typedef boost::shared_ptr<EventSubscription> EventSubscriptionPtr;
    std::vector<EventSubscriptionPtr> subscriptionsList = m_EventInterpreter.getSubscriptions();
    boost::shared_ptr<JSONObject> result(new JSONObject());
    boost::shared_ptr<JSONArrayBase> subscriptions(new JSONArrayBase());
    result->addElement("subscriptions", subscriptions);
    foreach(EventSubscriptionPtr pSubscription, subscriptionsList) {
      boost::shared_ptr<JSONObject> subscription(new JSONObject());
      subscriptions->addElement("", subscription);
      subscription->addProperty("id", pSubscription->getID());
      subscription->addProperty("handlerName", pSubscription->getHandlerName());
      subscription->addProperty("eventName", pSubscription->getEventName());
      SubscriptionOptions opts = pSubscription->getOptions();
      boost::shared_ptr<JSONObject> optsObj(new JSONObject());
      subscription->addElement("options", optsObj);
      HashMapConstStringString optsHash = opts.getParameters().getContainer();
      foreach(HashMapConstStringString::reference option, optsHash) {
        optsObj->addProperty(option.first, option.second);
      }
    }
    return success(result);
  } // list

  boost::shared_ptr<JSONObject> SubscriptionRequestHandler::remove(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    std::string subscriptionID = _request.getParameter("id");
    if(subscriptionID.empty()) {
      return failure("Need parameter id");
    }
    m_EventInterpreter.unsubscribe(subscriptionID);
    return success();
  } // remove

  boost::shared_ptr<JSONObject> SubscriptionRequestHandler::add(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    std::string eventName = _request.getParameter("eventName");
    std::string handlerName = _request.getParameter("handlerName");
    std::string optionsParameter = _request.getParameter("parameter");
    if(eventName.empty()) {
      return failure("need parameter eventName");
    }
    if(handlerName.empty()) {
      return failure("need parameter handlerName");
    }
    boost::shared_ptr<SubscriptionOptions> opts(new SubscriptionOptions());
    std::vector<std::string> optionLines = splitString(optionsParameter, ';');
    foreach(std::string optionLine, optionLines) {
      std::vector<std::string> keyValue = splitString(optionLine, '=');
      if(keyValue.size() != 2) {
        return failure("can't parse line: '" + optionLine + "' as key/value pair");
      } else {
        opts->setParameter(keyValue[0], keyValue[1]);
      }
    }
    boost::shared_ptr<EventSubscription> subscription(new EventSubscription(eventName, handlerName, m_EventInterpreter, opts));
    m_EventInterpreter.subscribe(subscription);

    boost::shared_ptr<JSONObject> result(new JSONObject());
    result->addProperty("id", subscription->getID());
    return success(result);
  } // add

} // namespace dss
