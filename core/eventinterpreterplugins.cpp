/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#include "base.h"
#include "logger.h"
#include "DS485Interface.h"
#include "setbuilder.h"
#include "dss.h"
#include "scripting/modeljs.h"

#include <boost/scoped_ptr.hpp>

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
      string timeParam = _subscription.getOptions().getParameter("time");
      if(!timeParam.empty()) {
        Logger::getInstance()->log("RaiseEvent: Event has time");
        newEvent->setTime(timeParam);
      }
    }
    if(_subscription.getOptions().hasParameter(EventPropertyLocation)) {
      string location = _subscription.getOptions().getParameter(EventPropertyLocation);
      if(!location.empty()) {
        Logger::getInstance()->log("RaiseEvent: Event has location");
        newEvent->setLocation(location);
      }
    }
    newEvent->setProperties(_event.getProperties());
    getEventInterpreter().getQueue().pushEvent(newEvent);
  } // handleEvent


  //================================================== EventInterpreterPluginJavascript

  EventInterpreterPluginJavascript::EventInterpreterPluginJavascript(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("javascript", _pInterpreter)
  { } // ctor

  void EventInterpreterPluginJavascript::handleEvent(Event& _event, const EventSubscription& _subscription) {
    if(_subscription.getOptions().hasParameter("filename")) {
      string scriptName = _subscription.getOptions().getParameter("filename");
      if(fileExists(scriptName)) {

        if(!m_Environment.isInitialized()) {
          m_Environment.initialize();
          ScriptExtension* ext = new ModelScriptContextExtension(DSS::getInstance()->getApartment());
          m_Environment.addExtension(ext);
          ext = new EventScriptExtension(DSS::getInstance()->getEventQueue(), getEventInterpreter());
          m_Environment.addExtension(ext);
        }

        try {
          boost::scoped_ptr<ScriptContext> ctx(m_Environment.getContext());
          ctx->loadFromFile(scriptName);
          ctx->evaluate<void>();
        } catch(ScriptException& e) {
          Logger::getInstance()->log(string("EventInterpreterPluginJavascript::handleEvent: Cought event while running/parsing script '")
                              + scriptName + "'. Message: " + e.what(), lsError);
        }
      } else {
        Logger::getInstance()->log(string("EventInterpreterPluginJavascript::handleEvent: Could not find script: '") + scriptName + "'", lsError);
      }
    } else {
      throw runtime_error("EventInterpreteRPluginJavascript::handleEvent: missing argument filename");
    }
  } // handleEvent


  //================================================== EventInterpreterPluginDS485

  EventInterpreterPluginDS485::EventInterpreterPluginDS485(DS485Interface* _pInterface, EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("bus_handler", _pInterpreter),
    m_pInterface(_pInterface)
  { } // ctor

  class SubscriptionOptionsDS485 : public SubscriptionOptions {
  private:
    DS485Command m_Command;
    int m_ParameterIndex;
    int m_SceneIndex;
    string m_To;
    string m_Context;
  public:
    SubscriptionOptionsDS485()
    : m_ParameterIndex(-1), m_SceneIndex(-1)
    { }

    void setCommand(const DS485Command _value) { m_Command = _value; }
    DS485Command getCommand() const { return m_Command; }

    void setParameterIndex(const int _value) { m_ParameterIndex = _value; }
    int getParameterIndex() const { return m_ParameterIndex; }

    void setTo(const string& _value) { m_To = _value; }
    const string& GetTo() const { return m_To; }

    void setContext(const string& _value) { m_Context = _value; }
    const string& getContext() const { return m_Context; }

    void setSceneIndex(const int _value) { m_SceneIndex = _value; }
    int getSceneIndex() const { return m_SceneIndex; }
  };

  string EventInterpreterPluginDS485::getParameter(XMLNodeList& _nodes, const string& _parameterName) {
    for(XMLNodeList::iterator iNode = _nodes.begin(), e = _nodes.end();
        iNode != e; ++iNode)
    {
      if(iNode->getName() == "parameter") {
        if(iNode->getAttributes()["name"] == _parameterName) {
          XMLNodeList& children = iNode->getChildren();
          if(!children.empty()) {
            return children[0].getContent();
          }
        }
      }
    }
    return "";
  } // getParameter

  SubscriptionOptions* EventInterpreterPluginDS485::createOptionsFromXML(XMLNodeList& _nodes) {
    SubscriptionOptionsDS485* result = new SubscriptionOptionsDS485();
    for(XMLNodeList::iterator iNode = _nodes.begin(), e = _nodes.end();
        iNode != e; ++iNode)
    {
      if(iNode->getName() == "send") {
        string typeName = iNode->getAttributes()["type"];
        string paramName = "";
        bool needParam = false;
        if(typeName == "turnOn") {
          result->setCommand(cmdTurnOn);
        } else if(typeName == "turnOff") {
          result->setCommand(cmdTurnOff);
        } else if(typeName == "dimUp") {
          result->setCommand(cmdStartDimUp);
          paramName = "parameter";
        } else if(typeName == "stopDim") {
          result->setCommand(cmdStopDim);
          paramName = "parameter";
        } else if(typeName == "callScene") {
          result->setCommand(cmdCallScene);
          paramName = "scene";
          needParam = true;
        } else if(typeName == "saveScene") {
          result->setCommand(cmdSaveScene);
          paramName = "scene";
          needParam = true;
        } else if(typeName == "undoScene") {
          result->setCommand(cmdUndoScene);
          paramName = "scene";
          needParam = true;
        } else if(typeName == "increaseValue") {
          result->setCommand(cmdIncreaseValue);
          paramName = "parameter";
        } else if(typeName == "decreaseValue") {
          result->setCommand(cmdDecreaseValue);
          paramName = "parameter";
        } else if(typeName == "enable") {
          result->setCommand(cmdEnable);
        } else if(typeName == "disable") {
          result->setCommand(cmdDisable);
        } else if(typeName == "increaseParameter") {
          result->setCommand(cmdIncreaseParam);
          paramName = "parameter";
        } else if(typeName == "decreaseParameter") {
          result->setCommand(cmdDecreaseParam);
          paramName = "parameter";
        } else {
          Logger::getInstance()->log(string("unknown command: ") + typeName);
          delete result;
          return NULL;
        }

        if(!paramName.empty()) {
          string paramValue = getParameter(iNode->getChildren(), paramName);
          if(paramValue.size() == 0 && needParam) {
            Logger::getInstance()->log(string("bus_handler: Needed parameter '") + paramName + "' not found in subscription for type '" + typeName + "'", lsError);
          }

          if(paramName == "parameter") {
            result->setParameterIndex(strToIntDef(paramValue, -1));
          } else if(paramName == "scene") {
            result->setSceneIndex(strToIntDef(paramValue, -1));
          }
        }
      }
    }

    return result;
  }

  void EventInterpreterPluginDS485::handleEvent(Event& _event, const EventSubscription& _subscription) {
    const SubscriptionOptionsDS485* options = dynamic_cast<const SubscriptionOptionsDS485*>(&_subscription.getOptions());
    if(options != NULL) {
      DS485Command cmd = options->getCommand();


      // determine location
      // if a location is given
      //   evaluate relative to context
      // else
      //   send to context's parent-entity (zone)

      SetBuilder builder;
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

      if(cmd == cmdCallScene || cmd == cmdSaveScene || cmd == cmdUndoScene) {
        m_pInterface->sendCommand(cmd, to, options->getSceneIndex());
      } else if(cmd == cmdIncreaseParam || cmd == cmdDecreaseParam ||
                cmd == cmdIncreaseValue || cmd == cmdDecreaseValue ||
                cmd == cmdStartDimUp || cmd == cmdStartDimDown || cmd == cmdStopDim)
      {
        m_pInterface->sendCommand(cmd, to, options->getParameterIndex());
      } else {
        Logger::getInstance()->log("EventInterpreterPluginDS485::handleEvent: sending...");
        m_pInterface->sendCommand(cmd, to, 0);
      }
    } else {
      Logger::getInstance()->log("EventInterpreterPluginDS485::handleEvent: Options are not of type SubscriptionOptionsDS485, ignoring", lsError);
    }
  } // handleEvent

} // namespace dss
