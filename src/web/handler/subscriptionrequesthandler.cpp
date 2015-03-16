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
#include <boost/make_shared.hpp>

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "subscriptionrequesthandler.h"

#include "src/event.h"
#include "src/base.h"
#include "src/foreach.h"
#include "src/stringconverter.h"

namespace dss {

  //=========================================== EventRequestHandler

  SubscriptionRequestHandler::SubscriptionRequestHandler(EventInterpreter& _interpreter)
  : m_EventInterpreter(_interpreter)
  { }

  WebServerResponse SubscriptionRequestHandler::jsonHandleRequest(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    if(_request.getMethod() == "list") {
      return list(_request, _session);
    } else if(_request.getMethod() == "remove") {
      return remove(_request, _session);
    } else if(_request.getMethod() == "add") {
      return add(_request, _session);
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest

  std::string SubscriptionRequestHandler::list(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    typedef boost::shared_ptr<EventSubscription> EventSubscriptionPtr;
    std::vector<EventSubscriptionPtr> subscriptionsList = m_EventInterpreter.getSubscriptions();
    JSONWriter json;
    json.startArray("subscriptions");
    foreach(EventSubscriptionPtr pSubscription, subscriptionsList) {
      json.startObject();
      json.add("id", pSubscription->getID());
      json.add("handlerName", pSubscription->getHandlerName());
      json.add("eventName", pSubscription->getEventName());
      json.startObject("options");
      boost::shared_ptr<const SubscriptionOptions> opts = pSubscription->getOptions();
      if(opts != NULL) {
        HashMapStringString optsHash = opts->getParameters().getContainer();
        foreach(HashMapStringString::reference option, optsHash) {
          json.add(option.first, option.second);
        }
      }
      json.endObject();
      EventPropertyFilter** filter = pSubscription->getFilter();
      if (filter && *filter) {
        json.startObject("filter");
        json.add("mode", pSubscription->getFilterMode());
        json.startArray("list");
        for (; *filter; filter++) {
          json.startObject();
          json.add("property", (*filter)->getPropertyName());
          EventPropertyMatchFilter* fm = dynamic_cast<EventPropertyMatchFilter *> (*filter);
          if (fm) {
            json.add("match", fm->getValue());
          }
          else if (dynamic_cast<EventPropertyMissingFilter *> (*filter)) {
            json.add("missing", "");
          }
          else if (dynamic_cast<EventPropertyExistsFilter *> (*filter)) {
            json.add("exists", "");
          }
          json.endObject();
        }
        json.endArray();
        json.endObject();
      }
      json.endObject();
    }
    json.endArray();
    return json.successJSON();
  } // list

  std::string SubscriptionRequestHandler::remove(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    std::string subscriptionID = _request.getParameter("id");
    if(subscriptionID.empty()) {
      return JSONWriter::failure("Need parameter id");
    }
    m_EventInterpreter.unsubscribe(subscriptionID);
    return JSONWriter::success();
  } // remove

  std::string SubscriptionRequestHandler::add(const RestfulRequest& _request, boost::shared_ptr<Session> _session) {
    StringConverter st("UTF-8", "UTF-8");

    std::string eventName = st.convert(_request.getParameter("eventName"));
    std::string handlerName = st.convert(_request.getParameter("handlerName"));
    std::string optionsParameter = _request.getParameter("parameter");
    if(eventName.empty()) {
      return JSONWriter::failure("need parameter eventName");
    }
    if(handlerName.empty()) {
      return JSONWriter::failure("need parameter handlerName");
    }
    boost::shared_ptr<SubscriptionOptions> opts = boost::make_shared<SubscriptionOptions>();
    std::vector<std::string> optionLines = splitString(optionsParameter, ';');
    foreach(std::string optionLine, optionLines) {
      std::vector<std::string> keyValue = splitString(optionLine, '=');
      if(keyValue.size() != 2) {
        return JSONWriter::failure("Can't parse line: '" + optionLine + "' as key/value pair");
      } else {
        opts->setParameter(st.convert(keyValue[0]), st.convert(keyValue[1]));
      }
    }
    boost::shared_ptr<EventSubscription> subscription = boost::make_shared<EventSubscription>(eventName, handlerName, boost::ref<EventInterpreter>(m_EventInterpreter), opts);

    m_EventInterpreter.subscribe(subscription);

    JSONWriter json;
    json.add("id", subscription->getID());
    return json.successJSON();
  } // add

} // namespace dss
