/*
 *  dss.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/2/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "dss.h"
#include "logger.h"
#include "xmlwrapper.h"
#include "propertysystem.h"
#include "scripting/modeljs.h"
#include "eventinterpreterplugins.h"
#ifdef __GNUC__
#include "../unix/ds485proxy.h"
#endif

#include <cassert>
#include <string>

using namespace std;

namespace dss {

  //============================================= DSS

  DSS::DSS() {
    m_EventInterpreter.SetEventQueue(&m_EventQueue);
    m_PropertySystem = boost::shared_ptr<PropertySystem>(new PropertySystem);
  } // ctor

  DSS* DSS::m_Instance = NULL;

  DSS* DSS::GetInstance() {
    if(m_Instance == NULL) {
      m_Instance = new DSS();
    }
    assert(m_Instance != NULL);
    return m_Instance;
  } // GetInstance

  void DSS::Run() {
    Logger::GetInstance()->Log("DSS stating up....", lsInfo);
    LoadConfig();
    m_WebServer.Initialize(m_Config);
    m_WebServer.Run();

    m_WebServices.Run();

    m_ModulatorSim.Initialize();
#ifndef __PARADIGM__
    m_DS485Interface = new DS485Proxy();
    (static_cast<DS485Proxy*>(m_DS485Interface))->Start();
#endif
    m_Apartment.Run();

//    m_Metering.Run();

    EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(&m_EventInterpreter);
    m_EventInterpreter.AddPlugin(plugin);
    plugin = new EventInterpreterPluginDS485(m_DS485Interface, &m_EventInterpreter);
    m_EventInterpreter.AddPlugin(plugin);

    m_EventInterpreter.LoadFromXML(m_Config.GetOptionAs<string>("subscription_file", "data/subscriptions.xml"));

    m_EventRunner.SetEventQueue(&m_EventQueue);
    m_EventInterpreter.SetEventRunner(&m_EventRunner);
    m_EventQueue.SetEventRunner(&m_EventRunner);

    m_EventInterpreter.Run();

    // pass control to the eventrunner
    m_EventRunner.Run();
  } // Run

  void DSS::LoadConfig() {
    Logger::GetInstance()->Log("Loading config", lsInfo);
    m_Config.ReadFromXML("data/config.xml");
  } // LoadConfig

}
