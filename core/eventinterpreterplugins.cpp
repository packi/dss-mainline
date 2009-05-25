/*
 * eventinterpreterplugins.cpp
 *
 *  Created on: Nov 10, 2008
 *      Author: patrick
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

  void EventInterpreterPluginRaiseEvent::HandleEvent(Event& _event, const EventSubscription& _subscription) {
    boost::shared_ptr<Event> newEvent(new Event(_subscription.GetOptions().GetParameter("event_name")));
    if(_subscription.GetOptions().HasParameter("time")) {
      string timeParam = _subscription.GetOptions().GetParameter("time");
      if(!timeParam.empty()) {
        Logger::GetInstance()->Log("RaiseEvent: Event has time");
        newEvent->SetTime(timeParam);
      }
    }
    if(_subscription.GetOptions().HasParameter(EventPropertyLocation)) {
      string location = _subscription.GetOptions().GetParameter(EventPropertyLocation);
      if(!location.empty()) {
        Logger::GetInstance()->Log("RaiseEvent: Event has location");
        newEvent->SetLocation(location);
      }
    }
    newEvent->SetProperties(_event.GetProperties());
    GetEventInterpreter().GetQueue().PushEvent(newEvent);
  } // HandleEvent


  //================================================== EventInterpreterPluginJavascript

  EventInterpreterPluginJavascript::EventInterpreterPluginJavascript(EventInterpreter* _pInterpreter)
  : EventInterpreterPlugin("javascript", _pInterpreter)
  { } // ctor

  void EventInterpreterPluginJavascript::HandleEvent(Event& _event, const EventSubscription& _subscription) {
    if(_subscription.GetOptions().HasParameter("filename")) {
      string scriptName = _subscription.GetOptions().GetParameter("filename");
      if(FileExists(scriptName)) {

        if(!m_Environment.IsInitialized()) {
          m_Environment.Initialize();
          ScriptExtension* ext = new ModelScriptContextExtension(DSS::GetInstance()->GetApartment());
          m_Environment.AddExtension(ext);
          ext = new EventScriptExtension(DSS::GetInstance()->GetEventQueue(), GetEventInterpreter());
          m_Environment.AddExtension(ext);
        }

        try {
          boost::scoped_ptr<ScriptContext> ctx(m_Environment.GetContext());
          ctx->LoadFromFile(_subscription.GetOptions().GetParameter("filename"));
          ctx->Evaluate<void>();
        } catch(ScriptException& e) {
          Logger::GetInstance()->Log(string("EventInterpreterPluginJavascript::HandleEvent: Cought event while running/parsing script '")
                              + scriptName + "'. Message: " + e.what(), lsError);
        }
      } else {
        Logger::GetInstance()->Log(string("EventInterpreterPluginJavascript::HandleEvent: Could not find script: '") + scriptName + "'", lsError);
      }
    } else {
      throw runtime_error("EventInterpreteRPluginJavascript::HandleEvent: missing argument filename");
    }
  } // HandleEvent


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

    void SetCommand(const DS485Command _value) { m_Command = _value; }
    DS485Command GetCommand() const { return m_Command; }

    void SetParameterIndex(const int _value) { m_ParameterIndex = _value; }
    int GetParameterIndex() const { return m_ParameterIndex; }

    void SetTo(const string& _value) { m_To = _value; }
    const string& GetTo() const { return m_To; }

    void SetContext(const string& _value) { m_Context = _value; }
    const string& GetContext() const { return m_Context; }

    void SetSceneIndex(const int _value) { m_SceneIndex = _value; }
    int GetSceneIndex() const { return m_SceneIndex; }
  };

  string EventInterpreterPluginDS485::GetParameter(XMLNodeList& _nodes, const string& _parameterName) {
    for(XMLNodeList::iterator iNode = _nodes.begin(), e = _nodes.end();
        iNode != e; ++iNode)
    {
      if(iNode->GetName() == "parameter") {
        if(iNode->GetAttributes()["name"] == _parameterName) {
          XMLNodeList& children = iNode->GetChildren();
          if(!children.empty()) {
            return children[0].GetContent();
          }
        }
      }
    }
    return "";
  } // GetParameter

  SubscriptionOptions* EventInterpreterPluginDS485::CreateOptionsFromXML(XMLNodeList& _nodes) {
    SubscriptionOptionsDS485* result = new SubscriptionOptionsDS485();
    for(XMLNodeList::iterator iNode = _nodes.begin(), e = _nodes.end();
        iNode != e; ++iNode)
    {
      if(iNode->GetName() == "send") {
        string typeName = iNode->GetAttributes()["type"];
        string paramName = "";
        bool needParam = false;
        if(typeName == "turnOn") {
          result->SetCommand(cmdTurnOn);
        } else if(typeName == "turnOff") {
          result->SetCommand(cmdTurnOff);
        } else if(typeName == "dimUp") {
          result->SetCommand(cmdStartDimUp);
          paramName = "parameter";
        } else if(typeName == "stopDim") {
          result->SetCommand(cmdStopDim);
          paramName = "parameter";
        } else if(typeName == "callScene") {
          result->SetCommand(cmdCallScene);
          paramName = "scene";
          needParam = true;
        } else if(typeName == "saveScene") {
          result->SetCommand(cmdSaveScene);
          paramName = "scene";
          needParam = true;
        } else if(typeName == "undoScene") {
          result->SetCommand(cmdUndoScene);
          paramName = "scene";
          needParam = true;
        } else if(typeName == "increaseValue") {
          result->SetCommand(cmdIncreaseValue);
          paramName = "parameter";
        } else if(typeName == "decreaseValue") {
          result->SetCommand(cmdDecreaseValue);
          paramName = "parameter";
        } else if(typeName == "enable") {
          result->SetCommand(cmdEnable);
        } else if(typeName == "disable") {
          result->SetCommand(cmdDisable);
        } else if(typeName == "increaseParameter") {
          result->SetCommand(cmdIncreaseParam);
          paramName = "parameter";
        } else if(typeName == "decreaseParameter") {
          result->SetCommand(cmdDecreaseParam);
          paramName = "parameter";
        } else {
          Logger::GetInstance()->Log(string("unknown command: ") + typeName);
          delete result;
          return NULL;
        }

        if(!paramName.empty()) {
          string paramValue = GetParameter(iNode->GetChildren(), paramName);
          if(paramValue.size() == 0 && needParam) {
            Logger::GetInstance()->Log(string("bus_handler: Needed parameter '") + paramName + "' not found in subscription for type '" + typeName + "'", lsError);
          }

          if(paramName == "parameter") {
            result->SetParameterIndex(StrToIntDef(paramValue, -1));
          } else if(paramName == "scene") {
            result->SetSceneIndex(StrToIntDef(paramValue, -1));
          }
        }
      }
    }

    return result;
  }

  void EventInterpreterPluginDS485::HandleEvent(Event& _event, const EventSubscription& _subscription) {
    const SubscriptionOptionsDS485* options = dynamic_cast<const SubscriptionOptionsDS485*>(&_subscription.GetOptions());
    if(options != NULL) {
      DS485Command cmd = options->GetCommand();


      // determine location
      // if a location is given
      //   evaluate relative to context
      // else
      //   send to context's parent-entity (zone)

      SetBuilder builder;
      Set to;
      if(_event.HasPropertySet(EventPropertyLocation)) {
        to = builder.BuildSet(_event.GetPropertyByName(EventPropertyLocation), &_event.GetRaisedAtZone());
      } else {
        if(_subscription.GetOptions().HasParameter(EventPropertyLocation)) {
          to = builder.BuildSet(_subscription.GetOptions().GetParameter(EventPropertyLocation), NULL);
        } else {
          to = _event.GetRaisedAtZone().GetDevices();
        }
      }

      if(cmd == cmdCallScene || cmd == cmdSaveScene || cmd == cmdUndoScene) {
        m_pInterface->SendCommand(cmd, to, options->GetSceneIndex());
      } else if(cmd == cmdIncreaseParam || cmd == cmdDecreaseParam ||
                cmd == cmdIncreaseValue || cmd == cmdDecreaseValue ||
                cmd == cmdStartDimUp || cmd == cmdStartDimDown || cmd == cmdStopDim)
      {
        m_pInterface->SendCommand(cmd, to, options->GetParameterIndex());
      } else {
        Logger::GetInstance()->Log("EventInterpreterPluginDS485::HandleEvent: sending...");
        m_pInterface->SendCommand(cmd, to, 0);
      }
    } else {
      Logger::GetInstance()->Log("EventInterpreterPluginDS485::HandleEvent: Options are not of type SubscriptionOptionsDS485, ignoring", lsError);
    }
  } // HandleEvent

} // namespace dss
