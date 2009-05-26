/*
 *  dss.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/2/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "dss.h"
#include "logger.h"
#include "xmlwrapper.h"
#include "propertysystem.h"
#include "scripting/modeljs.h"
#include "eventinterpreterplugins.h"
#ifdef __GNUC__
#include "../unix/ds485proxy.h"
#endif

#include "webserver.h"
#include "bonjour.h"
#ifdef WITH_SIM
  #include "sim/dssim.h"
#endif
#include "webservices/webservices.h"
#include "event.h"
#include "metering/metering.h"
#include "metering/fake_meter.h"
#include "foreach.h"

#include <cassert>
#include <string>

using namespace std;

namespace dss {

  //============================================= DSS

#ifdef WITH_DATADIR
const char* DataDirectory = WITH_DATADIR;
#else
const char* DataDirectory = "data/";
#endif

  DSS::DSS()
  {
    m_State = ssInvalid;
    m_pPropertySystem = boost::shared_ptr<PropertySystem>(new PropertySystem);
    SetDataDirectory(DataDirectory);

    m_TimeStarted = time(NULL);
    m_pPropertySystem->CreateProperty("/system/uptime")->LinkToProxy(
        PropertyProxyMemberFunction<DSS,int>(*this, &DSS::GetUptime));
    m_pPropertySystem->CreateProperty("/config/datadirectory")->LinkToProxy(
        PropertyProxyMemberFunction<DSS,string>(*this, &DSS::GetDataDirectory, &DSS::SetDataDirectory));
  } // ctor

  int DSS::GetUptime() const {
    return (int)difftime( time( NULL ), m_TimeStarted );
  } // GetUptime

  void DSS::SetDataDirectory(const string& _value) {
    if(!_value.empty() && (_value.at(_value.length() - 1) != '/')) {
      m_DataDirectory = _value + "/";
    } else {
      m_DataDirectory = _value;
    }
  } // SetDataDirectory

  void DSS::Initialize(const vector<string>& _properties) {
    m_State = ssCreatingSubsystems;

    m_pDS485Interface = boost::shared_ptr<DS485Proxy>(new DS485Proxy(this));
    m_Subsystems.push_back(dynamic_cast<DS485Proxy*>(m_pDS485Interface.get()));

    m_pApartment = boost::shared_ptr<Apartment>(new Apartment(this));
    m_Subsystems.push_back(m_pApartment.get());

    m_pWebServer = boost::shared_ptr<WebServer>(new WebServer(this));
    m_Subsystems.push_back(m_pWebServer.get());

    m_pWebServices = boost::shared_ptr<WebServices>(new WebServices(this));
    m_Subsystems.push_back(m_pWebServices.get());

#ifdef WITH_SIM
    m_pSimulation = boost::shared_ptr<DSSim>(new DSSim(this));
    m_Subsystems.push_back(m_pSimulation.get());
#endif

    m_pEventInterpreter = boost::shared_ptr<EventInterpreter>(new EventInterpreter(this));
    m_Subsystems.push_back(m_pEventInterpreter.get());

    m_pMetering = boost::shared_ptr<Metering>(new Metering(this));
    m_Subsystems.push_back(m_pMetering.get());

    m_pFakeMeter = boost::shared_ptr<FakeMeter>(new FakeMeter(this));
    m_Subsystems.push_back(m_pFakeMeter.get());

    m_pEventRunner = boost::shared_ptr<EventRunner>(new EventRunner);
    m_pEventQueue = boost::shared_ptr<EventQueue>(new EventQueue);

    foreach(string propLine, _properties) {
      string::size_type pos = propLine.find("=");
      if(pos == string::npos) {
        Logger::GetInstance()->Log("invalid property specified on commandline (format is name=value): '" + propLine + "'", lsError);
        abort();
      } else {
        string name = propLine.substr(0, pos);
        string value = propLine.substr(pos+1, string::npos);
        Logger::GetInstance()->Log("Setting property '" + name + "' to '" + value + "'", lsInfo);
        try {
          int val = StrToInt(value);
          m_pPropertySystem->SetIntValue(name, val, true);
          continue;
        } catch(invalid_argument&) {
        }

        if(value == "true") {
          m_pPropertySystem->SetBoolValue(name, true, true);
          continue;
        }

        if(value == "false") {
          m_pPropertySystem->SetBoolValue(name, false, true);
          continue;
        }

        m_pPropertySystem->SetStringValue(name, value, true);
      }
    }
  }

#ifdef WITH_TESTS
  void DSS::Teardown() {
    DSS* instance = m_Instance;
    m_Instance = NULL;
    delete instance;
  }
#endif

  DSS* DSS::m_Instance = NULL;

  DSS* DSS::GetInstance() {
    if(m_Instance == NULL) {
      m_Instance = new DSS();
    }
    assert(m_Instance != NULL);
    return m_Instance;
  } // GetInstance

  bool DSS::HasInstance() {
    return m_Instance != NULL;
  }

  void DSS::AddDefaultInterpreterPlugins() {
    EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(m_pEventInterpreter.get());
    m_pEventInterpreter->AddPlugin(plugin);
    plugin = new EventInterpreterPluginDS485(m_pDS485Interface.get(), m_pEventInterpreter.get());
    m_pEventInterpreter->AddPlugin(plugin);

    m_pEventRunner->SetEventQueue(m_pEventQueue.get());
    m_pEventInterpreter->SetEventRunner(m_pEventRunner.get());
    m_pEventQueue->SetEventRunner(m_pEventRunner.get());
    m_pEventInterpreter->SetEventQueue(m_pEventQueue.get());
  }

  void InitializeSubsystem(Subsystem* _pSubsystem) {
    _pSubsystem->Initialize();
  } // InitializeSubsystem

  void StartSubsystem(Subsystem* _pSubsystem) {
    _pSubsystem->Start();
  }

  void DSS::Run() {
    Logger::GetInstance()->Log("DSS stating up....", lsInfo);
    LoadConfig();


    m_State = ssInitializingSubsystems;
    std::for_each(m_Subsystems.begin(), m_Subsystems.end(), InitializeSubsystem);

    AddDefaultInterpreterPlugins();

    m_State = ssStarting;
    std::for_each(m_Subsystems.begin(), m_Subsystems.end(), StartSubsystem);

    BonjourHandler bonjour;
    bonjour.Run();

    m_State = ssRunning;

    // pass control to the eventrunner
    m_pEventRunner->Run();
  } // Run

  void DSS::LoadConfig() {
    m_State = ssLoadingConfig;
    Logger::GetInstance()->Log("Loading config", lsInfo);
    GetPropertySystem().LoadFromXML(GetDataDirectory() + "config.xml", GetPropertySystem().GetProperty("/config"));
  } // LoadConfig

}
