/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#include "eventinterpreterplugins.h"

#include <Poco/Net/MailMessage.h>
#include <Poco/Net/SMTPClientSession.h>
#include <Poco/Net/MailRecipient.h>

#include "base.h"
#include "logger.h"
#include "DS485Interface.h"
#include "setbuilder.h"
#include "dss.h"
#include "core/scripting/scriptobject.h"
#include "core/scripting/modeljs.h"
#include "core/scripting/propertyscriptextension.h"
#include "core/scripting/jssocket.h"
#include "core/scripting/ds485scriptextension.h"
#include "core/scripting/jslogger.h"
#include "core/ds485/businterfacehandler.h"
#include "core/foreach.h"
#include "core/model/set.h"
#include "core/model/zone.h"

#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/Exception.h>

using Poco::XML::Element;
using Poco::XML::Node;

namespace dss {


  //================================================== EventInterpreterPluginRaiseEvent

  EventInterpreterPluginRaiseEvent::EventInterpreterPluginRaiseEvent(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("raise_event", _pInterpreter)
  { } // ctor

  EventInterpreterPluginRaiseEvent::~EventInterpreterPluginRaiseEvent()
  { } // dtor

  void EventInterpreterPluginRaiseEvent::handleEvent(Event& _event, const EventSubscription& _subscription) {
    boost::shared_ptr<Event> newEvent(new Event(_subscription.getOptions().getParameter("event_name")));
    if(_subscription.getOptions().hasParameter("time")) {
      std::string timeParam = _subscription.getOptions().getParameter("time");
      if(!timeParam.empty()) {
        Logger::getInstance()->log("RaiseEvent: Event has time");
        newEvent->setTime(timeParam);
      }
    }
    applyOptionsWithSuffix(_subscription.getOptions(), "_default", newEvent, false);
    if(_subscription.getOptions().hasParameter(EventPropertyLocation)) {
      std::string location = _subscription.getOptions().getParameter(EventPropertyLocation);
      if(!location.empty()) {
        Logger::getInstance()->log("RaiseEvent: Event has location");
        newEvent->setLocation(location);
      }
    }
    newEvent->applyProperties(_event.getProperties());
    applyOptionsWithSuffix(_subscription.getOptions(), "_override", newEvent, true);
    getEventInterpreter().getQueue().pushEvent(newEvent);
  } // handleEvent

  void EventInterpreterPluginRaiseEvent::applyOptionsWithSuffix(const SubscriptionOptions& _options, const std::string& _suffix, boost::shared_ptr<Event> _event, bool _onlyOverride) {
    const HashMapConstStringString sourceMap = _options.getParameters().getContainer();
    typedef const std::pair<const std::string, std::string> tItem;
    foreach(tItem kv, sourceMap) {
      if(endsWith(kv.first, _suffix)) {
        std::string propName = kv.first.substr(0, kv.first.length() - _suffix.length());
        if(_event->hasPropertySet(propName) || !_onlyOverride) {
          Logger::getInstance()->log("RaiseEvent: setting property " + propName + " to " + kv.second);
          _event->setProperty(propName, kv.second);
        }
      }
    }
  } // applyOptionsWithSuffix


  //================================================== ScriptContextWrapper

  class ScriptContextWrapper {
  public:
    ScriptContextWrapper(boost::shared_ptr<ScriptContext> _pContext)
    : m_pContext(_pContext)
    { }

    boost::shared_ptr<ScriptContext> get() { return m_pContext; }
    void addFile(const std::string& _name) {
      m_LoadedFiles.push_back(_name);
    }
  private:
    boost::shared_ptr<ScriptContext> m_pContext;
    DateTime m_StartTime;
    std::vector<std::string> m_LoadedFiles;
  };

  //================================================== EventInterpreterPluginJavascript

  EventInterpreterPluginJavascript::EventInterpreterPluginJavascript(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("javascript", _pInterpreter)
  { } // ctor

  void EventInterpreterPluginJavascript::handleEvent(Event& _event, const EventSubscription& _subscription) {
    if(_subscription.getOptions().hasParameter("filename")) {
      std::string scriptName = _subscription.getOptions().getParameter("filename");

      if(!m_Environment.isInitialized()) {
        initializeEnvironment();
      }

      try {
        boost::shared_ptr<ScriptContext> ctx(m_Environment.getContext());
        boost::shared_ptr<ScriptContextWrapper> wrapper(new ScriptContextWrapper(ctx));
        ScriptObject raisedEvent(*ctx, NULL);
        raisedEvent.setProperty<const std::string&>("name", _event.getName());
        ctx->getRootObject().setProperty("raisedEvent", &raisedEvent);

        // add raisedEvent.parameter
        ScriptObject param(*ctx, NULL);
        const HashMapConstStringString& props =  _event.getProperties().getContainer();
        for(HashMapConstStringString::const_iterator iParam = props.begin(), e = props.end();
            iParam != e; ++iParam)
        {
          Logger::getInstance()->log("EventInterpreterPluginJavascript::handleEvent: setting parameter " + iParam->first +
                                      " to " + iParam->second);
          param.setProperty<const std::string&>(iParam->first, iParam->second);
        }
        raisedEvent.setProperty("parameter", &param);

        // add raisedEvent.subscription
        ScriptObject subscriptionObj(*ctx, NULL);
        raisedEvent.setProperty("subscription", &subscriptionObj);
        subscriptionObj.setProperty<const std::string&>("name", _subscription.getEventName());

        wrapper->addFile(scriptName);
        ctx->evaluateScript<void>(scriptName);

        if(ctx->hasAttachedObjects()) {
          m_WrappedContexts.push_back(wrapper);
          Logger::getInstance()->log("EventInterpreterPluginJavascript::handleEvent: still has objects, keeping " + scriptName + " in memory", lsInfo);
        }
      } catch(ScriptException& e) {
        Logger::getInstance()->log(std::string("EventInterpreterPluginJavascript::handleEvent: Caught event while running/parsing script '")
                            + scriptName + "'. Message: " + e.what(), lsError);
      }
    } else {
      throw std::runtime_error("EventInterpreteRPluginJavascript::handleEvent: missing argument filename");
    }
  } // handleEvent

  const std::string EventInterpreterPluginJavascript::kCleanupScriptsEventName = "EventInterpreteRPluginJavascript_cleanupScripts";

  void EventInterpreterPluginJavascript::initializeEnvironment() {
    m_Environment.initialize();
    if(DSS::hasInstance()) {
      ScriptExtension* ext = new ModelScriptContextExtension(DSS::getInstance()->getApartment());
      m_Environment.addExtension(ext);
      ext = new EventScriptExtension(DSS::getInstance()->getEventQueue(), getEventInterpreter());
      m_Environment.addExtension(ext);
      ext = new PropertyScriptExtension(DSS::getInstance()->getPropertySystem());
      m_Environment.addExtension(ext);
      ext = new ModelConstantsScriptExtension();
      m_Environment.addExtension(ext);
      ext = new SocketScriptContextExtension();
      m_Environment.addExtension(ext);
      ext = new DS485ScriptExtension(*DSS::getInstance()->getDS485Interface().getFrameSenderInterface(), DSS::getInstance()->getBusInterfaceHandler());
      m_Environment.addExtension(ext);
      ext = new ScriptLoggerExtension(DSS::getInstance()->getJSLogDirectory(), DSS::getInstance()->getEventInterpreter());
      m_Environment.addExtension(ext);
      setupCleanupEvent();
    }
  } // initializeEnvironment

  void EventInterpreterPluginJavascript::setupCleanupEvent() {
    EventInterpreterInternalRelay* pRelay =
      dynamic_cast<EventInterpreterInternalRelay*>(getEventInterpreter().getPluginByName(EventInterpreterInternalRelay::getPluginName()));
    m_pRelayTarget = boost::shared_ptr<InternalEventRelayTarget>(new InternalEventRelayTarget(*pRelay));

    boost::shared_ptr<EventSubscription> cleanupEventSubscription(
            new dss::EventSubscription(
                kCleanupScriptsEventName,
                EventInterpreterInternalRelay::getPluginName(),
                getEventInterpreter(),
                boost::shared_ptr<SubscriptionOptions>())
    );
    m_pRelayTarget->subscribeTo(cleanupEventSubscription);
    m_pRelayTarget->setCallback(boost::bind(&EventInterpreterPluginJavascript::cleanupTerminatedScripts, this, _1, _2));
    sendCleanupEvent();
  } // setupCleanupEvent

  void EventInterpreterPluginJavascript::cleanupTerminatedScripts(Event& _event, const EventSubscription& _subscription) {
    typedef std::vector<boost::shared_ptr<ScriptContextWrapper> >::iterator tScriptContextWrapperIterator;
    tScriptContextWrapperIterator ipScriptContextWrapper = m_WrappedContexts.begin();
    while(ipScriptContextWrapper != m_WrappedContexts.end()) {
      if(!(*ipScriptContextWrapper)->get()->hasAttachedObjects()) {
        Logger::getInstance()->log("cleanupTerminatedScripts: erasing script");
        ipScriptContextWrapper = m_WrappedContexts.erase(ipScriptContextWrapper);
      } else {
        ++ipScriptContextWrapper;
      }
    }
    sendCleanupEvent();
  } // cleanupTerminatedScripts

  void EventInterpreterPluginJavascript::sendCleanupEvent() {
    boost::shared_ptr<Event> pEvent(new Event(kCleanupScriptsEventName));
    pEvent->setProperty("time", "+" + intToString(20));
    getEventInterpreter().getQueue().pushEvent(pEvent);
  } // sendCleanupEvent


  //================================================== EventInterpreterPluginDS485

  EventInterpreterPluginDS485::EventInterpreterPluginDS485(Apartment& _apartment, DS485Interface* _pInterface, EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("bus_handler", _pInterpreter),
    m_pInterface(_pInterface),
    m_Apartment(_apartment)
  { } // ctor

  class SubscriptionOptionsDS485 : public SubscriptionOptions {
  private:
    typedef boost::function<void(Set&)> Command;
    Command m_Command;
  public:
    void execute(Set& _set) const {
      assert(m_Command);
      m_Command(_set);
    }

    void setCommand(Command _command) {
      m_Command = _command;
    }
  };

  std::string EventInterpreterPluginDS485::getParameter(Node* _node, const std::string& _parameterName) {
    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "parameter") {
        Element* elem = dynamic_cast<Element*>(curNode);
        if(elem->getAttribute("name") == _parameterName) {
          if(elem->hasChildNodes()) {
            return elem->firstChild()->getNodeValue();
          }
        }
      }
      curNode = curNode->nextSibling();
    }
    Logger::getInstance()->log(std::string("bus_handler: Needed parameter '") + _parameterName + "' not found", lsError);
    return "";
  } // getParameter

  SubscriptionOptions* EventInterpreterPluginDS485::createOptionsFromXML(Node* _node) {
    SubscriptionOptionsDS485* result = new SubscriptionOptionsDS485();

    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "send") {
        Element* elem = dynamic_cast<Element*>(curNode);
        if(elem != NULL) {
          std::string typeName = elem->getAttribute("type");

          if(typeName == "turnOn") {
            result->setCommand(boost::bind(&Set::turnOn, _1));
          } else if(typeName == "turnOff") {
            result->setCommand(boost::bind(&Set::turnOff, _1));
          } else if(typeName == "dimUp") {
            result->setCommand(boost::bind(&Set::startDim, _1, true));
          } else if(typeName == "stopDim") {
            result->setCommand(boost::bind(&Set::endDim, _1));
          } else if(typeName == "increaseValue") {
            result->setCommand(boost::bind(&Set::increaseValue, _1));
          } else if(typeName == "decreaseValue") {
            result->setCommand(boost::bind(&Set::decreaseValue, _1));
          } else if(typeName == "callScene") {
            int sceneNr = strToInt(getParameter(curNode, "scene"));
            result->setCommand(boost::bind(&Set::callScene, _1, sceneNr));
          } else if(typeName == "saveScene") {
            int sceneNr = strToInt(getParameter(curNode, "scene"));
            result->setCommand(boost::bind(&Set::callScene, _1, sceneNr));
          } else if(typeName == "undoScene") {
            int sceneNr = strToInt(getParameter(curNode, "scene"));
            result->setCommand(boost::bind(&Set::callScene, _1, sceneNr));
          } else {
            Logger::getInstance()->log(std::string("unknown command: ") + typeName);
            delete result;
            return NULL;
          }

        }
      }
      curNode = curNode->nextSibling();
    }

    return result;
  }

  void EventInterpreterPluginDS485::handleEvent(Event& _event, const EventSubscription& _subscription) {
    const SubscriptionOptionsDS485* options = dynamic_cast<const SubscriptionOptionsDS485*>(&_subscription.getOptions());
    if(options != NULL) {
      SetBuilder builder(m_Apartment);
      Set to;
      if(_event.hasPropertySet(EventPropertyLocation)) {
        to = builder.buildSet(_event.getPropertyByName(EventPropertyLocation), &_event.getRaisedAtZone());
      } else {
        if(_subscription.getOptions().hasParameter(EventPropertyLocation)) {
          to = builder.buildSet(_subscription.getOptions().getParameter(EventPropertyLocation), NULL);
        } else {
          to = _event.getRaisedAtZone().getDevices();
        }
      }
      options->execute(to);
    } else {
      Logger::getInstance()->log("EventInterpreterPluginDS485::handleEvent: Options are not of type SubscriptionOptionsDS485, ignoring", lsError);
    }
  } // handleEvent


  //================================================== EventRelayTarget

  EventRelayTarget::~EventRelayTarget() {
    while(!m_Subscriptions.empty()) {
      unsubscribeFrom(m_Subscriptions.front()->getID());
    }
  } // dtor

  void EventRelayTarget::subscribeTo(boost::shared_ptr<EventSubscription> _pSubscription) {
    m_Relay.registerSubscription(this, _pSubscription->getID());
    m_Subscriptions.push_back(_pSubscription);
    getRelay().getEventInterpreter().subscribe(_pSubscription);
  } // subscribeTo

  void EventRelayTarget::unsubscribeFrom(const std::string& _subscriptionID) {
    m_Relay.removeSubscription(_subscriptionID);
    SubscriptionPtrList::iterator it =
      std::find_if(m_Subscriptions.begin(), m_Subscriptions.end(),
                   boost::bind(&EventSubscription::getID, _1) == _subscriptionID);
    getRelay().getEventInterpreter().unsubscribe(_subscriptionID);
    if(it != m_Subscriptions.end()) {
      m_Subscriptions.erase(it);
    }
  } // unsubscribeFrom


  //================================================== EventInterpreterInternalRelay

  EventInterpreterInternalRelay::EventInterpreterInternalRelay(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin(getPluginName(), _pInterpreter)
  { } // ctor

  EventInterpreterInternalRelay::~EventInterpreterInternalRelay() {
  } // dtor

  void EventInterpreterInternalRelay::handleEvent(Event& _event, const EventSubscription& _subscription) {
    EventRelayTarget* target = m_IDTargetMap[_subscription.getID()];
    if(target != NULL) {
      target->handleEvent(_event, _subscription);
    }
  } // handleEvent

  void EventInterpreterInternalRelay::registerSubscription(EventRelayTarget* _pTarget, const std::string& _subscriptionID) {
    m_IDTargetMap[_subscriptionID] = _pTarget;
  } // registerSubscription

  void EventInterpreterInternalRelay::removeSubscription(const std::string& _subscriptionID) {
    m_IDTargetMap[_subscriptionID] = NULL;
  } // removeSubscription

  //================================================== EventInterpreterPluginEmail

  EventInterpreterPluginEmail::EventInterpreterPluginEmail(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("send_email", _pInterpreter)
  { } // ctor

  void EventInterpreterPluginEmail::handleEvent(Event& _event, const EventSubscription& _subscription) {
    std::string emailServer;
    std::string login;
    std::string password;
    std::string sender;
    std::string receiver;
    std::string body;
    std::string subject;
    if(_event.hasPropertySet("email_server")) {
      emailServer=_event.getPropertyByName("email_server");
    } else if(_subscription.getOptions().hasParameter("email_server")) {
      emailServer=_subscription.getOptions().getParameter("email_server");
    } else {
      return;
    }

    if(_event.hasPropertySet("login")) {
      login=_event.getPropertyByName("login");
    } else if(_subscription.getOptions().hasParameter("login")) {
      login=_subscription.getOptions().getParameter("login");
    } else {
      return;
    }

    if(_event.hasPropertySet("password")) {
      password=_event.getPropertyByName("password");
    } else if(_subscription.getOptions().hasParameter("password")) {
      password=_subscription.getOptions().getParameter("password");
    } else {
      return;
    }

    if(_event.hasPropertySet("sender")) {
      sender=_event.getPropertyByName("sender");
    } else if(_subscription.getOptions().hasParameter("sender")) {
      sender=_subscription.getOptions().getParameter("sender");
    } else {
      return;
    }

    if(_event.hasPropertySet("receiver")) {
      receiver=_event.getPropertyByName("receiver");
    } else if(_subscription.getOptions().hasParameter("receiver")) {
      receiver=_subscription.getOptions().getParameter("receiver");
    } else {
      return;
    }

    if (_event.hasPropertySet("body")) {
      body=_event.getPropertyByName("body");
    } else if(_subscription.getOptions().hasParameter("body")) {
      body=_subscription.getOptions().getParameter("body");
    } else {
      return;
    }

    if(_event.hasPropertySet("subject")) {
      subject=_event.getPropertyByName("subject");
    } else if(_subscription.getOptions().hasParameter("subject")) {
      subject=_subscription.getOptions().getParameter("subject");
    } else {
      return;
    }

    try {
      Logger::getInstance()->log("EventInterpreterPluginEmail::handleEvent: Sending Email", lsInfo);
      Poco::Net::MailMessage message;
      message.setSender(sender);
      message.addRecipient(Poco::Net::MailRecipient(Poco::Net::MailRecipient::PRIMARY_RECIPIENT, receiver));
      message.setSubject(subject);
      message.setContent(body);
      Logger::getInstance()->log("EventInterpreterPluginEmail::handleEvent: host "+ emailServer, lsInfo);
      Poco::Net::SMTPClientSession session(emailServer);
      session.login(Poco::Net::SMTPClientSession::AUTH_LOGIN, login, password);
      session.sendMessage(message);
      session.close();
    }
    catch (Poco::Exception& exc)
    {
      Logger::getInstance()->log("exc.displayText()=" + exc.displayText(), lsFatal);
    }
  } // handleEvent


} // namespace dss
