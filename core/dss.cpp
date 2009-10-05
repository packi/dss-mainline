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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifdef HAVE_BUILD_INFO_H
  #include "build_info.h"
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
#ifdef WITH_BONJOUR
  #include "bonjour.h"
#endif
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
#include <sstream>

#ifndef WIN32
#include <csignal>
#endif

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
    setDataDirectory(DataDirectory);

    m_TimeStarted = time(NULL);
    m_pPropertySystem->createProperty("/system/uptime")->linkToProxy(
        PropertyProxyMemberFunction<DSS,int>(*this, &DSS::getUptime));
    m_pPropertySystem->createProperty("/config/datadirectory")->linkToProxy(
        PropertyProxyMemberFunction<DSS,string>(*this, &DSS::getDataDirectory, &DSS::setDataDirectory));
  } // ctor

  DSS::~DSS() {
    m_State = ssTerminating;

    m_pWebServer.reset();
    m_pWebServices.reset();

    m_pEventQueue.reset();
    m_pEventRunner.reset();

    m_pEventInterpreter.reset();
    m_pMetering.reset();
    m_pFakeMeter.reset();

#ifdef WITH_SIM
    m_pSimulation.reset();
#endif

    m_pDS485Interface.reset();


    m_pApartment.reset();
  }

  int DSS::getUptime() const {
    return (int)difftime( time( NULL ), m_TimeStarted );
  } // getUptime

  void DSS::setDataDirectory(const string& _value) {
    if(!_value.empty() && (_value.at(_value.length() - 1) != '/')) {
      m_DataDirectory = _value + "/";
    } else {
      m_DataDirectory = _value;
    }
  } // setDataDirectory

  void DSS::initialize(const vector<string>& _properties) {
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
        Logger::getInstance()->log("invalid property specified on commandline (format is name=value): '" + propLine + "'", lsError);
        abort();
      } else {
        string name = propLine.substr(0, pos);
        string value = propLine.substr(pos+1, string::npos);
        Logger::getInstance()->log("Setting property '" + name + "' to '" + value + "'", lsInfo);
        try {
          int val = strToInt(value);
          m_pPropertySystem->setIntValue(name, val, true);
          continue;
        } catch(invalid_argument&) {
        }

        if(value == "true") {
          m_pPropertySystem->setBoolValue(name, true, true);
          continue;
        }

        if(value == "false") {
          m_pPropertySystem->setBoolValue(name, false, true);
          continue;
        }

        m_pPropertySystem->setStringValue(name, value, true);
      }
    }
  }

#ifdef WITH_TESTS
  void DSS::teardown() {
    DSS* instance = m_Instance;
    m_Instance = NULL;
    delete instance;
  }
#endif

  DSS* DSS::m_Instance = NULL;

  DSS* DSS::getInstance() {
    if(m_Instance == NULL) {
      m_Instance = new DSS();
    }
    assert(m_Instance != NULL);
    return m_Instance;
  } // getInstance

  bool DSS::hasInstance() {
    return m_Instance != NULL;
  }

  void DSS::addDefaultInterpreterPlugins() {
    EventInterpreterPlugin* plugin = new EventInterpreterPluginRaiseEvent(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterPluginDS485(m_pDS485Interface.get(), m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterPluginJavascript(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterInternalRelay(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);

    m_pEventRunner->setEventQueue(m_pEventQueue.get());
    m_pEventInterpreter->setEventRunner(m_pEventRunner.get());
    m_pEventQueue->setEventRunner(m_pEventRunner.get());
    m_pEventInterpreter->setEventQueue(m_pEventQueue.get());
  }

  void InitializeSubsystem(Subsystem* _pSubsystem) {
    _pSubsystem->initialize();
  } // initializeSubsystem

  void StartSubsystem(Subsystem* _pSubsystem) {
    _pSubsystem->start();
  }

  void DSS::run() {
    Logger::getInstance()->log("DSS starting up....", lsInfo);
    if(!loadConfig()) {
      Logger::getInstance()->log("Could not parse config file", lsFatal);
      return;
    }
    
    // see whether we have a log file set in config.xml, and set the
    // log target accordingly
    PropertyNodePtr pNode = getPropertySystem().getProperty("/config/logfile");
    if (pNode) {
      std::string logFileName = pNode->getStringValue();      
      Logger::getInstance()->log("Logging to file: " + logFileName, lsInfo);
      
      boost::shared_ptr<dss::LogTarget> 
        logTarget(new dss::FileLogTarget(logFileName));
      if (!dss::Logger::getInstance()->setLogTarget(logTarget)) {
        Logger::getInstance()->log("Failed to open logfile '" + logFileName + 
                                   "'; exiting", lsFatal);
        return;
      }
    } else {
      Logger::getInstance()->log("No logfile configured, logging to stdout", 
                                 lsInfo);
    }
    
    dss::Logger::getInstance()->log(versionString(), lsInfo);

    m_State = ssInitializingSubsystems;
    std::for_each(m_Subsystems.begin(), m_Subsystems.end(), InitializeSubsystem);

    addDefaultInterpreterPlugins();

    m_State = ssStarting;
    std::for_each(m_Subsystems.begin(), m_Subsystems.end(), StartSubsystem);

#ifdef WITH_BONJOUR
    BonjourHandler bonjour;
    bonjour.run();
#endif

    m_State = ssRunning;

    // pass control to the eventrunner
    m_pEventRunner->run();
  } // run

  void DSS::shutdown() {
    DSS* inst = m_Instance;
    m_Instance = NULL;
    delete inst;
  } // shutdown

  bool DSS::loadConfig() {
    m_State = ssLoadingConfig;
    Logger::getInstance()->log("Loading config", lsInfo);
    return getPropertySystem().loadFromXML(getDataDirectory() + "config.xml", getPropertySystem().getProperty("/config"));
  } // loadConfig


  std::string DSS::versionString() {
    std::ostringstream ostr;
    ostr << "DSS";
#ifdef HAVE_CONFIG_H
    ostr << " v" << DSS_VERSION;
#endif
#ifdef HAVE_BUILD_INFO_H
    ostr << " (r" << DSS_RCS_REVISION << ")"
         << " (" << DSS_BUILD_USER << "@" << DSS_BUILD_HOST << ")"
         << " " << DSS_BUILD_DATE;
#endif
    return ostr.str();
  }

#ifndef WIN32
  void DSS::handleSignal(int _signum) {
    switch (_signum) {
      case SIGUSR1: {
        Logger::getInstance()->reopenLogTarget();
        break;
      }
      default: {
        std::ostringstream ostr;
        ostr << "DSS::handleSignal(): unhandled signal " << _signum << endl;
        Logger::getInstance()->log(ostr.str(), lsWarning);
        break;
      }
    }
  }
#endif
}
