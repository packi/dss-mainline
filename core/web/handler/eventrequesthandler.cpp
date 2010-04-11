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

#include "eventrequesthandler.h"

#include "core/web/json.h"

#include "core/event.h"
#include "core/base.h"

#include <boost/any.hpp>

namespace dss {

  //=========================================== EventRequestHandler

  EventRequestHandler::EventRequestHandler(EventQueue& _queue)
  : m_Queue(_queue)
  { }

  boost::shared_ptr<JSONObject> EventRequestHandler::raise(const RestfulRequest& _request) {
    std::string name = _request.getParameter("name");
    std::string location = _request.getParameter("location");
    std::string context = _request.getParameter("context");
    std::string parameter = _request.getParameter("parameter");

    if (name.empty()) {
      return failure("Missing event name!");
    }

    boost::shared_ptr<Event> evt(new Event(name));
    if(!context.empty()) {
      evt->setContext(context);
    }
    if(!location.empty()) {
      evt->setLocation(location);
    }
    std::vector<std::string> params = dss::splitString(parameter, ';');
    for(std::vector<std::string>::iterator iParam = params.begin(), e = params.end();
        iParam != e; ++iParam)
    {
      std::vector<std::string> nameValue = dss::splitString(*iParam, '=');
      if(nameValue.size() == 2) {
        dss::Logger::getInstance()->log("WebServer::handleEventCall: Got parameter '" + nameValue[0] + "'='" + nameValue[1] + "'");
        evt->setProperty(nameValue[0], nameValue[1]);
      } else {
        dss::Logger::getInstance()->log(std::string("Invalid parameter found WebServer::handleEventCall: ") + *iParam );
      }
    }

    m_Queue.pushEvent(evt);
    return success();
  }

  // name=EventName&sid=EventSubscriptionID
  boost::shared_ptr<JSONObject> EventRequestHandler::subscribe(const RestfulRequest& _request, Session* _session) {
    std::string name = _request.getParameter("name");
    std::string tokenStr = _request.getParameter("sid");
    int token;

    if (_session == NULL) {
      return failure("Invalid session!");
    }

    if(name.empty()) {
      return failure("Missing event name!");
    }

    if(tokenStr.empty()) {
      return failure("Missing event subscription id!");
    }
    try{
     token = strToInt(tokenStr); 
    }
    catch(std::invalid_argument& err) {
      return failure("Could not parse event subscription id!");
    }

    boost::shared_ptr<EventSubscriptionSessionByTokenID> eventSessions;

    m_eventsMutex.lock();

    boost::shared_ptr<boost::any> a = _session->getData("eventSubscriptionIDs");
    
    if ((a.get() == NULL) || (a->empty())) {
      boost::shared_ptr<boost::any> b(new boost::any());
      eventSessions = boost::shared_ptr<EventSubscriptionSessionByTokenID>(new EventSubscriptionSessionByTokenID());
      *b = eventSessions;
      _session->addData(std::string("eventSubscriptionIDs"), b);
    } else {
      try {
        eventSessions = boost::any_cast<boost::shared_ptr<EventSubscriptionSessionByTokenID> >(*a);
      } catch (boost::bad_any_cast& e) {
        Logger::getInstance()->log("Fatal error: unexpected data type stored in session!", lsFatal);
        assert(0);
      }
    }

    EventSubscriptionSessionByTokenID::iterator entry = eventSessions->find(token);
    if(entry == eventSessions->end()){
        boost::shared_ptr<EventSubscriptionSession> session(new EventSubscriptionSession(token));
      (*eventSessions)[token] = session;
    }

    (*eventSessions)[token]->subscribe(name);
    
    m_eventsMutex.unlock();

    return success();
  }

  // name=EventName&sid=EventSubscriptionID
  boost::shared_ptr<JSONObject> EventRequestHandler::unsubscribe(const RestfulRequest& _request, Session* _session) {
    std::string name = _request.getParameter("name");
    std::string tokenStr = _request.getParameter("sid");
    int token;

    if (_session == NULL) {
      return failure("Invalid session!");
    }

    if(name.empty()) {
      return failure("Missing event name!");
    }

    if(tokenStr.empty()) {
      return failure("Missing event subscription id!");
    }

    try{
     token = strToInt(tokenStr); 
    }
    catch(std::invalid_argument& err) {
      return failure("Could not parse event subscription id!");
    }

    boost::shared_ptr<EventSubscriptionSessionByTokenID> eventSessions;

    boost::shared_ptr<boost::any> a = _session->getData("eventSubscriptionIDs");
    
    if ((a.get() == NULL) || (a->empty())) {
      return failure("Invalid session!");
    } else {
      try {
        eventSessions = boost::any_cast<boost::shared_ptr<EventSubscriptionSessionByTokenID> >(*a);
      } catch (boost::bad_any_cast& e) {
        Logger::getInstance()->log("Fatal error: unexpected data type stored in session!", lsFatal);
        assert(0);
      }
    }

    m_eventsMutex.lock();

    EventSubscriptionSessionByTokenID::iterator entry = eventSessions->find(token);
    if(entry == eventSessions->end()){
      m_eventsMutex.unlock();
      return failure("Token not found!");
    }

    try {
      (*eventSessions)[token]->unsubscribe(name);
    } catch(std::exception& e)
    {
      m_eventsMutex.unlock();
      return failure(e.what());
    }
   
    eventSessions->erase(entry);

    m_eventsMutex.unlock();

    return success();
  }

  // sid=SubscriptionID&timeout=0
  boost::shared_ptr<JSONObject> EventRequestHandler::get(const RestfulRequest& _request, Session* _session) {
    std::string tokenStr = _request.getParameter("sid");
    std::string timeoutStr = _request.getParameter("timeout");
    int timeout = 0;
    int token;
 
    if (_session == NULL) {
      return failure("Invalid session!");
    }

    if(tokenStr.empty()) {
      return failure("Missing event subscription id!");
    }

    try {
      token = strToInt(tokenStr); 
    }
    catch(std::invalid_argument& err) {
      return failure("Could not parse subsription id!");
    }

    if(!timeoutStr.empty()) {
      try {
        timeout = strToInt(timeoutStr); 
      }
      catch(std::invalid_argument& err) {
        return failure("Could not parse timeout parameter!");
      }
    }

    boost::shared_ptr<EventSubscriptionSessionByTokenID> eventSessions;

    m_eventsMutex.lock();

    boost::shared_ptr<boost::any> a = _session->getData("eventSubscriptionIDs");
    
    if ((a.get() == NULL) || (a->empty())) {
      m_eventsMutex.unlock();
      return failure("Invalid session!");
    } else {
      try {
        eventSessions = boost::any_cast<boost::shared_ptr<EventSubscriptionSessionByTokenID> >(*a);
      } catch (boost::bad_any_cast& e) {
        Logger::getInstance()->log("Fatal error: unexpected data type stored in session!", lsFatal);
        assert(0);
      }
    }

    EventSubscriptionSessionByTokenID::iterator entry = eventSessions->find(token);
    if(entry == eventSessions->end()){
      m_eventsMutex.unlock();
      return failure("Subsription id not found!");
    }
    
    boost::shared_ptr<EventSubscriptionSession> session = (*eventSessions)[token];
    m_eventsMutex.unlock();

    return success(session->getEvents(timeout));
  }

  boost::shared_ptr<JSONObject> EventRequestHandler::jsonHandleRequest(const RestfulRequest& _request, Session* _session) {
    if(_request.getMethod() == "raise") {
      return raise(_request);
    } else if(_request.getMethod() == "subscribe") {
      return subscribe(_request, _session);
    } else if(_request.getMethod() == "unsubscribe") {
      return unsubscribe(_request, _session);
    } else if(_request.getMethod() == "get") {
      return get(_request, _session);
    }
    throw std::runtime_error("Unhandled function");
  } // handleRequest

  EventSubscriptionSession::EventSubscriptionSession() : Session() {}

  EventSubscriptionSession::EventSubscriptionSession(const int _tokenID) : Session(_tokenID) {
  }

  int EventSubscriptionSession::getTokenID() {
    return m_Token;
  }

  void EventSubscriptionSession::subscribe(const std::string& _eventName) {
    createCollector();
    // we do not want any double subscriptions here
    try {
      unsubscribe(_eventName);
    } catch(std::runtime_error& err) {}

    m_subscriptionMap[_eventName] = m_pEventCollector->subscribeTo(_eventName); 
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

  boost::shared_ptr<JSONObject> EventSubscriptionSession::getEvents(const int _timeoutMS)
  {
    createCollector();

    m_pEventCollector->waitForEvent(_timeoutMS);

    boost::shared_ptr<JSONObject> resultObj(new JSONObject());
    boost::shared_ptr<JSONArrayBase> eventsArray(new JSONArrayBase);
    resultObj->addElement("events", eventsArray);

    while(m_pEventCollector->hasEvent()) {
      Event evt = m_pEventCollector->popEvent();
      boost::shared_ptr<JSONObject> evtObj(new JSONObject());
      eventsArray->addElement("event", evtObj);

      evtObj->addProperty("name", evt.getName());
      boost::shared_ptr<JSONArrayBase> evtprops(new JSONArrayBase);
      evtObj->addElement("properties", evtprops);

      const dss::HashMapConstStringString& props =  evt.getProperties().getContainer();
      for(dss::HashMapConstStringString::const_iterator iParam = props.begin(), e = props.end(); iParam != e; ++iParam)
      {
        boost::shared_ptr<JSONObject> property(new JSONObject());
        property->addProperty(iParam->first, iParam->second);
        evtprops->addElement("property", property);
      }
    }

    return resultObj;
  }

  void EventSubscriptionSession::createCollector() {
    if(m_pEventCollector == NULL) {
      EventInterpreterInternalRelay* pPlugin =
          dynamic_cast<EventInterpreterInternalRelay*>(
              DSS::getInstance()->getEventInterpreter().getPluginByName(
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
} // namespace dss
