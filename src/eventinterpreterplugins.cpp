/*
    Copyright (c) 2009,2010 digitalSTROM.org, Zurich, Switzerland

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

#include "config.h"
#include "base.h"
#include "logger.h"
#include "businterface.h"
#include "setbuilder.h"
#include "dss.h"
#include "src/scripting/scriptobject.h"
#include "src/scripting/jsmodel.h"
#include "src/scripting/jsevent.h"
#include "src/scripting/jsmetering.h"
#include "src/scripting/jsproperty.h"
#include "src/scripting/jssocket.h"
#include "src/scripting/jslogger.h"
#if HAVE_CURL
  #include "src/scripting/jscurl.h"
#endif
#include "src/foreach.h"
#include "src/model/set.h"
#include "src/model/zone.h"
#include "src/model/group.h"
#include "src/model/device.h"

#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/Exception.h>
#include <Poco/Net/MailMessage.h>
#include <Poco/Net/SMTPClientSession.h>
#include <Poco/Net/MailRecipient.h>

#include <limits.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>

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
    boost::shared_ptr<Event> newEvent(new Event(_subscription.getOptions()->getParameter("event_name")));
    if(_subscription.getOptions()->hasParameter("time")) {
      std::string timeParam = _subscription.getOptions()->getParameter("time");
      if(!timeParam.empty()) {
        Logger::getInstance()->log("RaiseEvent: Event has time");
        newEvent->setTime(timeParam);
      }
    }
    applyOptionsWithSuffix(_subscription.getOptions(), "_default", newEvent, false);
    if(_subscription.getOptions()->hasParameter(EventPropertyLocation)) {
      std::string location = _subscription.getOptions()->getParameter(EventPropertyLocation);
      if(!location.empty()) {
        Logger::getInstance()->log("RaiseEvent: Event has location");
        newEvent->setLocation(location);
      }
    }
    newEvent->applyProperties(_event.getProperties());
    applyOptionsWithSuffix(_subscription.getOptions(), "_override", newEvent, true);
    getEventInterpreter().getQueue().pushEvent(newEvent);
  } // handleEvent

  void EventInterpreterPluginRaiseEvent::applyOptionsWithSuffix(boost::shared_ptr<const SubscriptionOptions> _options, const std::string& _suffix, boost::shared_ptr<Event> _event, bool _onlyOverride) {
    if(_options != NULL) {
      const HashMapStringString sourceMap = _options->getParameters().getContainer();
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
    }
  } // applyOptionsWithSuffix


  //================================================== ScriptContextWrapper

  ScriptContextWrapper::ScriptContextWrapper(boost::shared_ptr<ScriptContext> _pContext,
      PropertyNodePtr _pRootNode,
      const std::string& _identifier,
      bool _uniqueNode) :
      m_pContext(_pContext),
      m_Identifier(_identifier),
      m_UniqueNode(_uniqueNode) {
    if((_pRootNode != NULL) && !_identifier.empty()) {
      if(_uniqueNode) {
        m_pPropertyNode = _pRootNode->createProperty(_identifier + "+");
      } else {
        m_pPropertyNode = _pRootNode->createProperty(_identifier);
      }
      m_StopNode = m_pPropertyNode->createProperty("stopScript+");
      m_StopNode->linkToProxy(
          PropertyProxyMemberFunction<ScriptContextWrapper,bool>(*this, NULL, &ScriptContextWrapper::stopScript));
      m_StartedAtNode = m_pPropertyNode->createProperty("startedAt+");
      m_StartedAtNode->linkToProxy(
          PropertyProxyMemberFunction<DateTime, std::string, false>(m_StartTime, &DateTime::toString));
      m_AttachedObjectsNode = m_pPropertyNode->createProperty("attachedObjects+");
      m_AttachedObjectsNode->linkToProxy(
          PropertyProxyMemberFunction<ScriptContext,int>(*m_pContext, &ScriptContext::getAttachedObjectsCount));
    }
  }

  ScriptContextWrapper::~ScriptContextWrapper() {
    if((m_pPropertyNode != NULL) && (m_pPropertyNode->getParentNode() != NULL)) {
      if(m_UniqueNode) {
        m_pPropertyNode->getParentNode()->removeChild(m_pPropertyNode);
      } else {
        m_pPropertyNode->removeChild(m_StartedAtNode);
        m_pPropertyNode->removeChild(m_StopNode);
        m_pPropertyNode->removeChild(m_AttachedObjectsNode);
        m_pPropertyNode->removeChild(m_FilesNode);
      }
    }
  }

  boost::shared_ptr<ScriptContext> ScriptContextWrapper::get() {
    return m_pContext;
  }

  void ScriptContextWrapper::addFile(const std::string& _name) {
    m_LoadedFiles.push_back(_name);
    if(m_pPropertyNode != NULL) {
      if(m_FilesNode == NULL) {
        m_FilesNode = m_pPropertyNode->createProperty("files+");
      }
      m_FilesNode->createProperty("file+")->setStringValue(_name);
    }
  }

  PropertyNodePtr ScriptContextWrapper::getPropertyNode() {
    return m_pPropertyNode;
  }

  const std::string& ScriptContextWrapper::getIdentifier() const {
    return m_Identifier;
  }

  void ScriptContextWrapper::stopScript(bool _value) {
    Logger::getInstance()->log("Stop of script '" + m_pPropertyNode->getName() + "' requested.", lsInfo);
    m_pContext->stop();
  }

  class WrapperAwarePropertyScriptExtension : public PropertyScriptExtension {
  public:
    WrapperAwarePropertyScriptExtension(EventInterpreterPluginJavascript& _plugin, PropertySystem& _propertySystem,
                                        const std::string& _storeDirectory)
    : PropertyScriptExtension(_propertySystem),
      m_Plugin(_plugin),
      m_StoreDirectory(addTrailingBackslash(_storeDirectory))
    { }

    virtual PropertyNodePtr getProperty(ScriptContext* _context, const std::string& _path) {
      if(beginsWith(_path, "/")) {
        return PropertyScriptExtension::getProperty(_context, _path);
      } else {
        boost::shared_ptr<ScriptContextWrapper> wrapper = m_Plugin.getContextWrapperForContext(_context);
        assert(wrapper != NULL);
        return wrapper->getPropertyNode()->getProperty(_path);
      }
    }

    virtual PropertyNodePtr createProperty(ScriptContext* _context, const std::string& _name) {
      if(beginsWith(_name, "/")) {
        return PropertyScriptExtension::createProperty(_context, _name);
      } else {
        boost::shared_ptr<ScriptContextWrapper> wrapper = m_Plugin.getContextWrapperForContext(_context);
        assert(wrapper != NULL);
        return wrapper->getPropertyNode()->createProperty(_name);
      }
    }

    virtual bool store(ScriptContext* _context, PropertyNodePtr _node) {
      boost::shared_ptr<ScriptContextWrapper> wrapper = m_Plugin.getContextWrapperForContext(_context);
      assert(wrapper != NULL);
      std::string fileName(wrapper->getIdentifier());
      PropertyNodePtr pNode;
      if (_node) {
        pNode = _node;
        fileName += "_" + pNode->getDisplayName();
      } else {
        pNode = wrapper->getPropertyNode();
      }
      // TODO: sanitize filename to prevent world-domination
      return m_PropertySystem.saveToXML(
                                m_StoreDirectory + fileName + ".xml",
                                pNode, PropertyNode::Archive);
    }

    virtual bool load(ScriptContext* _context) {
      boost::shared_ptr<ScriptContextWrapper> wrapper = m_Plugin.getContextWrapperForContext(_context);
      assert(wrapper != NULL);
      // TODO: sanitize filename to prevent world-domination
      return m_PropertySystem.loadFromXML(
                                m_StoreDirectory + wrapper->getIdentifier() + ".xml",
                                wrapper->getPropertyNode());
    }
  private:
    EventInterpreterPluginJavascript& m_Plugin;
    std::string m_StoreDirectory;
  }; // WrapperAwarePropertyScriptExtension

  //================================================== EventInterpreterPluginJavascript

  EventInterpreterPluginJavascript::EventInterpreterPluginJavascript(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("javascript", _pInterpreter)
  { } // ctor

  EventInterpreterPluginJavascript::~EventInterpreterPluginJavascript() {
    log("Terminating all scripts...", lsInfo);
    typedef std::vector<boost::shared_ptr<ScriptContextWrapper> >::iterator tScriptContextWrapperIterator;
    tScriptContextWrapperIterator ipScriptContextWrapper = m_WrappedContexts.begin();
    int shutdownTimeout = 0;
    while(ipScriptContextWrapper != m_WrappedContexts.end()) {
      (*ipScriptContextWrapper)->get()->stop();
      while((*ipScriptContextWrapper)->get()->hasAttachedObjects()) {
        sleepMS(10);
        shutdownTimeout += 10;
        if (shutdownTimeout > 2500) {
          Logger::getInstance()->log("Forced to shutdown pending scripts, timeout reached", lsWarning);
          break;
        }
      }
      ipScriptContextWrapper = m_WrappedContexts.erase(ipScriptContextWrapper);
    }
    log("All scripts Terminated");
  }

  void EventInterpreterPluginJavascript::handleEvent(Event& _event, const EventSubscription& _subscription) {
    if(_subscription.getOptions()->hasParameter("filename1")) {

      if(m_pEnvironment == NULL) {
        initializeEnvironment();
      }

      boost::shared_ptr<ScriptContext> ctx(m_pEnvironment->getContext());
      std::string scriptID;
      if(_subscription.getOptions()->hasParameter("script_id")) {
        scriptID = _subscription.getOptions()->getParameter("script_id");
      }
      bool uniqueNode = false;
      if(scriptID.empty()) {
        uniqueNode = true;
        scriptID = _event.getName() + _subscription.getID();
      }

      boost::shared_ptr<ScriptContextWrapper> wrapper(
        new ScriptContextWrapper(ctx, m_pScriptRootNode, scriptID, uniqueNode));
      ctx->attachWrapper(wrapper);
      m_WrapperInAction = wrapper;

      {
        JSContextThread th(ctx.get());

        ScriptObject raisedEvent(*ctx, NULL);
        raisedEvent.setProperty<const std::string&>("name", _event.getName());
        ctx->getRootObject().setProperty("raisedEvent", &raisedEvent);
        if(_event.hasPropertySet(EventPropertyLocation)) {
          raisedEvent.setProperty<const std::string&>("location", _event.getPropertyByName(EventPropertyLocation));
        }
        EventRaiseLocation raiseLocation = _event.getRaiseLocation();
        ScriptObject source(*ctx, NULL);
        if((raiseLocation == erlGroup) || (raiseLocation == erlApartment)) {
          if(DSS::hasInstance()) {
            boost::shared_ptr<const Group> group =
              _event.getRaisedAtGroup(DSS::getInstance()->getApartment());
            source.setProperty("set", ".zone(" + intToString(group->getZoneID()) +
                               ").group(" + intToString(group->getID()) + ")");
            source.setProperty("groupID", group->getID());
            source.setProperty("zoneID", group->getZoneID());
            source.setProperty("isApartment", raiseLocation == erlApartment);
            source.setProperty("isGroup", raiseLocation == erlGroup);
            source.setProperty("isDevice", false);
          }
        } else {
          boost::shared_ptr<const DeviceReference> device = _event.getRaisedAtDevice();
          source.setProperty("set", "dsid(" + device->getDSID().toString() + ")");
          source.setProperty("dsid", device->getDSID().toString());
          source.setProperty("zoneID", device->getDevice()->getZoneID());
          source.setProperty("isApartment", false);
          source.setProperty("isGroup", false);
          source.setProperty("isDevice", true);
        }
        raisedEvent.setProperty("source", &source);

        // add raisedEvent.parameter
        ScriptObject param(*ctx, NULL);
        const HashMapStringString& props =  _event.getProperties().getContainer();
        for(HashMapStringString::const_iterator iParam = props.begin(), e = props.end();
            iParam != e; ++iParam)
        {
          Logger::getInstance()->log("JavaScript Event Handler: setting parameter " + iParam->first +
                                      " to " + iParam->second);
          param.setProperty<const std::string&>(iParam->first, iParam->second);
        }
        raisedEvent.setProperty("parameter", &param);

        // add raisedEvent.subscription
        ScriptObject subscriptionObj(*ctx, NULL);
        raisedEvent.setProperty("subscription", &subscriptionObj);
        subscriptionObj.setProperty<const std::string&>("name", _subscription.getEventName());
      }

      std::string scripts;

      for (int i = 0; i < UCHAR_MAX; i++) {
        std::string paramName = std::string("filename") + intToString(i + 1);
        if (!_subscription.getOptions()->hasParameter(paramName)) {
          break;
        }
        std::string scriptName =
            _subscription.getOptions()->getParameter(paramName);

        wrapper->addFile(scriptName);

        try {

          Logger::getInstance()->log("JavaScript Event Handler: "
              "running script " + scriptName, lsDebug);

          ctx->evaluateScript<void>(scriptName);

        } catch(ScriptException& ex) {
          Logger::getInstance()->log(
              std::string("JavaScript Event Handler: "
                  "Script error running/parsing script '")
              + scriptName + "'. Message: " + ex.what(), lsError);
          break;
        } catch(DSSException& ex) {
          Logger::getInstance()->log(
              std::string("JavaScript Event Handler:"
                  "dSS/Bus error running/parsing script '")
              + scriptName + "'. Message: " + ex.what(), lsError);
          break;
        } catch(boost::thread_resource_error& ex) {
          Logger::getInstance()->log(
              std::string("JavaScript Event Handler:"
                  "Fatal ressource error running/parsing script '")
          + scriptName + "'. Message: " + ex.what(), lsError);
          return;
        } catch(std::runtime_error& ex) {
          Logger::getInstance()->log(
              std::string("JavaScript Event Handler:"
                  "Fatal error running/parsing script '")
              + scriptName + "'. Message: " + ex.what(), lsError);
          return;
        }

        scripts = scripts + scriptName + " ";
      }

      if(ctx->hasAttachedObjects()) {
        m_WrappedContexts.push_back(wrapper);
        Logger::getInstance()->log("JavaScript Event Handler: "
                                   "keep " + scripts + " in memory", lsDebug);
      } else {
        ctx->detachWrapper();
      }

    } else {
      throw std::runtime_error("JavaScript Event Handler: missing argument filename1");
    }
  } // handleEvent

  const std::string EventInterpreterPluginJavascript::kCleanupScriptsEventName = "EventInterpreterPluginJavascript_cleanupScripts";

  void EventInterpreterPluginJavascript::initializeEnvironment() {
    if(DSS::hasInstance()) {
      m_pEnvironment.reset(new ScriptEnvironment(&DSS::getInstance()->getSecurity()));
      m_pEnvironment->initialize();

      ScriptExtension* ext;
      ext = new ScriptLoggerExtension(DSS::getInstance()->getJSLogDirectory(),
                                      DSS::getInstance()->getEventInterpreter());
      m_pEnvironment->addExtension(ext);

      ext = new EventScriptExtension(DSS::getInstance()->getEventQueue(), getEventInterpreter());
      m_pEnvironment->addExtension(ext);

      ext = new WrapperAwarePropertyScriptExtension(*this, DSS::getInstance()->getPropertySystem(),
                                                    DSS::getInstance()->getSavedPropsDirectory());
      m_pEnvironment->addExtension(ext);

      ext = new MeteringScriptExtension(DSS::getInstance()->getApartment(),
                                        DSS::getInstance()->getMetering());
      m_pEnvironment->addExtension(ext);

      ext = new SocketScriptContextExtension();
      m_pEnvironment->addExtension(ext);

#if HAVE_CURL
      ext = new CurlScriptContextExtension();
      m_pEnvironment->addExtension(ext);
#endif

      ext = new ModelScriptContextExtension(DSS::getInstance()->getApartment());
      m_pEnvironment->addExtension(ext);

      ext = new ModelConstantsScriptExtension();
      m_pEnvironment->addExtension(ext);

      setupCleanupEvent();
      m_pScriptRootNode = DSS::getInstance()->getPropertySystem().createProperty("/scripts");
    } else {
      m_pEnvironment.reset(new ScriptEnvironment());
      m_pEnvironment->initialize();
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
        Logger::getInstance()->log("JavaScript cleanup: erasing script "
            + (*ipScriptContextWrapper)->getIdentifier());
        (*ipScriptContextWrapper)->get()->detachWrapper();
        ipScriptContextWrapper = m_WrappedContexts.erase(ipScriptContextWrapper);
      } else {
        Logger::getInstance()->log("JavaScript cleanup: script "
            + (*ipScriptContextWrapper)->getIdentifier()
            + " still has objects: " + intToString((*ipScriptContextWrapper)->get()->getAttachedObjectsCount()));
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

  boost::shared_ptr<ScriptContextWrapper> EventInterpreterPluginJavascript::getContextWrapperForContext(ScriptContext* _pContext) {
    boost::shared_ptr<ScriptContextWrapper> result;
    if(!m_WrapperInAction.expired()) {
      result = m_WrapperInAction.lock();
      if(result->get().get() != _pContext) {
        result.reset();
      }
    }
    typedef std::vector<boost::shared_ptr<ScriptContextWrapper> >::iterator tScriptContextWrapperIterator;
    tScriptContextWrapperIterator ipScriptContextWrapper = m_WrappedContexts.begin();
    while(ipScriptContextWrapper != m_WrappedContexts.end()) {
      if((*ipScriptContextWrapper)->get().get() == _pContext) {
        result = (*ipScriptContextWrapper);
      }
      ++ipScriptContextWrapper;
    }
    return result;
  } // getContextWrapperForContext


  //================================================== EventInterpreterPluginDS485

  EventInterpreterPluginDS485::EventInterpreterPluginDS485(Apartment& _apartment, BusInterface* _pInterface, EventInterpreter* _pInterpreter)
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

  boost::shared_ptr<SubscriptionOptions> EventInterpreterPluginDS485::createOptionsFromXML(Node* _node) {
    boost::shared_ptr<SubscriptionOptionsDS485> result(new SubscriptionOptionsDS485());

    Node* curNode = _node->firstChild();
    while(curNode != NULL) {
      if(curNode->localName() == "send") {
        Element* elem = dynamic_cast<Element*>(curNode);
        if(elem != NULL) {
          std::string typeName = elem->getAttribute("type");

          if(typeName == "turnOn") {
            result->setCommand(boost::bind(&Set::turnOn, _1, IDeviceInterface::coSubscription));
          } else if(typeName == "turnOff") {
            result->setCommand(boost::bind(&Set::turnOff, _1, IDeviceInterface::coSubscription));
          } else if(typeName == "increaseValue") {
            result->setCommand(boost::bind(&Set::increaseValue, _1, IDeviceInterface::coSubscription));
          } else if(typeName == "decreaseValue") {
            result->setCommand(boost::bind(&Set::decreaseValue, _1, IDeviceInterface::coSubscription));
          } else if(typeName == "callScene") {
            int sceneNr = strToInt(getParameter(curNode, "scene"));
            result->setCommand(boost::bind(&Set::callScene, _1, IDeviceInterface::coSubscription, sceneNr, false));
          } else if(typeName == "saveScene") {
            int sceneNr = strToInt(getParameter(curNode, "scene"));
            result->setCommand(boost::bind(&Set::saveScene, _1, IDeviceInterface::coSubscription, sceneNr));
          } else if(typeName == "undoScene") {
            int sceneNr = strToInt(getParameter(curNode, "scene"));
            result->setCommand(boost::bind(&Set::undoScene, _1, IDeviceInterface::coSubscription, sceneNr));
          } else {
            Logger::getInstance()->log(std::string("unknown command: ") + typeName);
            return boost::shared_ptr<SubscriptionOptions>();
          }

        }
      }
      curNode = curNode->nextSibling();
    }

    return result;
  }

  void EventInterpreterPluginDS485::handleEvent(Event& _event, const EventSubscription& _subscription) {
    boost::shared_ptr<const SubscriptionOptionsDS485> options = boost::dynamic_pointer_cast<const SubscriptionOptionsDS485>(_subscription.getOptions());
    if(options != NULL) {
      SetBuilder builder(m_Apartment);
      Set to;
      if(_event.hasPropertySet(EventPropertyLocation)) {
        to = builder.buildSet(_event.getPropertyByName(EventPropertyLocation), _event.getRaisedAtGroup(m_Apartment));
      } else {
        if(_subscription.getOptions()->hasParameter(EventPropertyLocation)) {
          to = builder.buildSet(_subscription.getOptions()->getParameter(EventPropertyLocation), boost::shared_ptr<Group>());
        } else {
          to = _event.getRaisedAtGroup(m_Apartment)->getDevices();
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

  //================================================== EventInterpreterPluginSendmail

  EventInterpreterPluginSendmail::EventInterpreterPluginSendmail(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("sendmail", _pInterpreter)
  { } // ctor

  void EventInterpreterPluginSendmail::handleEvent(Event& _event, const EventSubscription& _subscription) {
    std::string sender;
    std::string recipient, recipient_cc, recipient_bcc;
    std::string subject;
    std::string body;

    if(_event.hasPropertySet("from")) {
      sender = _event.getPropertyByName("from");
    } else if(_subscription.getOptions()->hasParameter("from")) {
      sender = _subscription.getOptions()->getParameter("from");
    }

    if(_event.hasPropertySet("to")) {
      recipient = _event.getPropertyByName("to");
    } else if(_subscription.getOptions()->hasParameter("to")) {
      recipient = _subscription.getOptions()->getParameter("to");
    }

    if(_event.hasPropertySet("cc")) {
      recipient_cc = _event.getPropertyByName("cc");
    } else if(_subscription.getOptions()->hasParameter("cc")) {
      recipient_cc = _subscription.getOptions()->getParameter("cc");
    }

    if(_event.hasPropertySet("bcc")) {
      recipient_bcc = _event.getPropertyByName("bcc");
    } else if(_subscription.getOptions()->hasParameter("bcc")) {
      recipient_bcc = _subscription.getOptions()->getParameter("bcc");
    }

    if(_event.hasPropertySet("subject")) {
      subject = _event.getPropertyByName("subject");
    } else if(_subscription.getOptions()->hasParameter("subject")) {
      subject = _subscription.getOptions()->getParameter("subject");
    }

    if (_event.hasPropertySet("body")) {
      body = _event.getPropertyByName("body");
    } else if(_subscription.getOptions()->hasParameter("body")) {
      body = _subscription.getOptions()->getParameter("body");
    }

    try {
      Logger::getInstance()->log("EventInterpreterPluginSendmail::handleEvent: Sendmail", lsDebug);

      char mailText[] = "/tmp/mailXXXXXX";
      int mailFile = mkstemp((char *) mailText);
      if (mailFile < 0) {
        Logger::getInstance()->log("EventInterpreterPluginSendmail: generating temporary file failed [" +
            intToString(errno) + "]", lsFatal);
        return;
      }
      FILE* mailStream = fdopen(mailFile, "w");
      if (mailStream == NULL) {
        Logger::getInstance()->log("EventInterpreterPluginSendmail: writing to temporary file failed [" +
            intToString(errno) + "]", lsFatal);
        return;
      }

      DateTime now;
      std::ostringstream mail;
      mail << "Date: " << now.toRFC2822String() << "\n"
          << "To: " << recipient << "\n"
          << "From: " << sender << "\n"
          << "Subject: " << subject << "\n"
          << "X-Mailer: digitalSTROM Server (v" DSS_VERSION ")" << "\n\n";
      mail << body;
      fputs(mail.str().c_str(), mailStream);
      fclose(mailStream);

      pthread_t pid;
      pthread_attr_t attr;
      int err;

      pthread_attr_init(&attr);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      if ((err = pthread_create(&pid, &attr, EventInterpreterPluginSendmail::run, (void*) strdup(mailText))) < 0) {
        Logger::getInstance()->log("EventInterpreterPluginSendmail: failed to start mail thread, error " +
            intToString(err) + "[" + intToString(errno) + "]", lsFatal);
      }
      pthread_attr_destroy(&attr);

    } catch (std::exception& e) {
      Logger::getInstance()->log("EventInterpreterPluginSendmail: failed to send mail: " +
          std::string(e.what()), lsFatal);
    }
  } // handleEvent

  void* EventInterpreterPluginSendmail::run(void* arg) {
    const char* mailText = (const char *) arg;

#ifndef HAVE_SENDMAIL
    Logger::getInstance()->log("EventInterpreterPluginSendmail: "
        "sendmail binary not found by configure, sending mail is disabled", lsWarning);
#else
    posix_spawn_file_actions_t action;
    char* spawnedArgs[] = { (char *) SENDMAIL, (char *) "-t", NULL };
    int status, err;
    pid_t pid;
    if ((err = posix_spawn_file_actions_init(&action)) != 0) {
      Logger::getInstance()->log("EventInterpreterPluginSendmail: posix_spawn_file_actions_init error " +
          intToString(err), lsFatal);
    } else if ((err = posix_spawn_file_actions_addopen(&action, STDIN_FILENO, mailText, O_RDONLY, 0)) != 0) {
      Logger::getInstance()->log("EventInterpreterPluginSendmail: posix_spawn_file_actions_addopen error " +
          intToString(err), lsFatal);
    } else if ((err = posix_spawnp(&pid, spawnedArgs[0], &action, NULL, spawnedArgs, NULL)) != 0) {
      Logger::getInstance()->log("EventInterpreterPluginSendmail: posix_spawnp error " +
          intToString(err) + "[" + intToString(errno) + "]", lsFatal);
    } else {
      (void) waitpid(pid, &status, 0);
      if ((status & 0x7f) == 127) {
        Logger::getInstance()->log("EventInterpreterPluginSendmail: abnormal exit of child process", lsFatal);
      } else if (WIFEXITED(status) && (WEXITSTATUS(status) > 0)) {
        Logger::getInstance()->log("EventInterpreterPluginSendmail: sendmail returned error code " +
            intToString(WEXITSTATUS(status)), lsFatal);
      }
    }
#endif

    unlink(mailText);
    delete mailText;
    return NULL;
  } // run


} // namespace dss
