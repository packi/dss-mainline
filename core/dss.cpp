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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifdef HAVE_BUILD_INFO_H
  #include "build_info.h"
#endif


#include "dss.h"
#include "logger.h"
#include "propertysystem.h"
#include "scripting/modeljs.h"
#include "eventinterpreterplugins.h"
#include "core/ds485/ds485proxy.h"
#include "core/ds485/businterfacehandler.h"
#include "core/ds485/ds485busrequestdispatcher.h"
#include "core/model/apartment.h"
#include "core/model/modelmaintenance.h"

#include "core/web/webserver.h"
#ifdef WITH_BONJOUR
  #include "bonjour.h"
#endif
#include "sim/dssim.h"
#include "webservices/webservices.h"
#include "event.h"
#include "metering/metering.h"
#include "metering/fake_meter.h"
#include "foreach.h"

#include "unix/systeminfo.h"

#include <cassert>
#include <string>
#include <sstream>

#ifndef WIN32
#include <csignal>
#endif

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace dss {

  //============================================= DSS

#ifdef WITH_DATADIR
const char* DataDirectory = WITH_DATADIR;
#else
const char* DataDirectory = "data/";
#endif

#ifdef WITH_CONFIGDIR
const char* ConfigDirectory = WITH_CONFIGDIR;
#else
const char* ConfigDirectory = "data/";
#endif

#ifdef WITH_WEBROOTDIR
const char* WebrootDirectory = WITH_WEBROOTDIR;
#else
const char* WebrootDirectory = "data/webroot/";
#endif

#ifdef WITH_JSLOGDIR
const char* JSLogDirectory = WITH_JSLOGDIR;
#else
const char* JSLogDirectory = "data/logs/";
#endif

  DSS::DSS()
  {
    m_State = ssInvalid;
    m_pPropertySystem = boost::shared_ptr<PropertySystem>(new PropertySystem);
    setDataDirectory(DataDirectory);
    setConfigDirectory(ConfigDirectory);
    setWebrootDirectory(WebrootDirectory);
    setJSLogDirectory(JSLogDirectory);

    m_TimeStarted = time(NULL);
    m_pPropertySystem->createProperty("/system/uptime")->linkToProxy(
        PropertyProxyMemberFunction<DSS,int>(*this, &DSS::getUptime));
    m_pPropertySystem->createProperty("/config/datadirectory")->linkToProxy(
        PropertyProxyMemberFunction<DSS,std::string>(*this, &DSS::getDataDirectory, &DSS::setDataDirectory));
    m_pPropertySystem->createProperty("/config/configdirectory")->linkToProxy(
        PropertyProxyMemberFunction<DSS,std::string>(*this, &DSS::getConfigDirectory, &DSS::setConfigDirectory));
    m_pPropertySystem->createProperty("/config/webrootdirectory")->linkToProxy(
        PropertyProxyMemberFunction<DSS,std::string>(*this, &DSS::getWebrootDirectory, &DSS::setWebrootDirectory));
    m_pPropertySystem->createProperty("/config/jslogdirectory")->linkToProxy(
        PropertyProxyMemberFunction<DSS,std::string>(*this, &DSS::getJSLogDirectory, &DSS::setJSLogDirectory));
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

    m_pSimulation.reset();

    m_pDS485Interface.reset();


    m_pApartment.reset();
  }

  int DSS::getUptime() const {
    return (int)difftime( time( NULL ), m_TimeStarted );
  } // getUptime

  bool DSS::isSaneDirectory(const std::string& _path) {
    return boost::filesystem::is_directory(_path);
  } // isSaneDirectory

  void DSS::setDataDirectory(const std::string& _value) {
    m_dataDirectory = addTrailingBackslash(_value);
  } // setDataDirectory

  void DSS::setConfigDirectory(const std::string& _value) {
    m_configDirectory = addTrailingBackslash(_value);
  } // setConfigDirectory

  void DSS::setWebrootDirectory(const std::string& _value) {
    m_webrootDirectory = addTrailingBackslash(_value);
  } // setWebrootDirectory

  void DSS::setJSLogDirectory(const std::string& _value) {
    m_jsLogDirectory = addTrailingBackslash(_value);
  } // setJSLogDirectory

  bool DSS::parseProperties(const std::vector<std::string>& _properties) {
    foreach(std::string propLine, _properties) {
      std::string::size_type pos = propLine.find("=");
      if(pos == std::string::npos) {
        Logger::getInstance()->log("invalid property specified on commandline (format is name=value): '" + propLine + "'", lsError);
        return false;
      } else {
        std::string name = propLine.substr(0, pos);
        std::string value = propLine.substr(pos+1, std::string::npos);
        Logger::getInstance()->log("Setting property '" + name + "' to '" + value + "'", lsInfo);
        try {
          int val = strToInt(value);
          m_pPropertySystem->setIntValue(name, val, true);
          continue;
        } catch(std::invalid_argument&) {
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

    return true;
  } // parseProperties

  bool DSS::initialize(const vector<std::string>& _properties, const std::string& _configFile) {
    m_State = ssCreatingSubsystems;

    m_pModelMaintenance = boost::shared_ptr<ModelMaintenance>(new ModelMaintenance(this));
    m_Subsystems.push_back(m_pModelMaintenance.get());

    m_pApartment = boost::shared_ptr<Apartment>(new Apartment(this));
    m_pApartment->setPropertySystem(m_pPropertySystem.get());
    m_pModelMaintenance->setApartment(m_pApartment.get());

    m_pSimulation = boost::shared_ptr<DSSim>(new DSSim(this));
    m_Subsystems.push_back(m_pSimulation.get());

    m_pDS485Interface = boost::shared_ptr<DS485Proxy>(new DS485Proxy(this, m_pModelMaintenance.get(), m_pSimulation.get()));
    m_Subsystems.push_back(dynamic_cast<DS485Proxy*>(m_pDS485Interface.get()));

    m_pBusDispatcher = boost::shared_ptr<DS485BusRequestDispatcher>(new DS485BusRequestDispatcher());
    m_pBusDispatcher->setFrameSender(m_pDS485Interface->getFrameSenderInterface());

    m_pApartment->setDS485Interface(m_pDS485Interface.get());
    m_pApartment->setBusRequestDispatcher(m_pBusDispatcher.get());

    m_pBusInterfaceHandler = boost::shared_ptr<BusInterfaceHandler>(new BusInterfaceHandler(this, getModelMaintenance()));
    m_Subsystems.push_back(m_pBusInterfaceHandler.get());
    dynamic_cast<DS485Proxy*>(m_pDS485Interface.get())->setBusInterfaceHandler(m_pBusInterfaceHandler.get());

    m_pWebServer = boost::shared_ptr<WebServer>(new WebServer(this));
    m_Subsystems.push_back(m_pWebServer.get());

    m_pWebServices = boost::shared_ptr<WebServices>(new WebServices(this));
    m_Subsystems.push_back(m_pWebServices.get());

    m_pEventInterpreter = boost::shared_ptr<EventInterpreter>(new EventInterpreter(this));
    m_Subsystems.push_back(m_pEventInterpreter.get());

    m_pMetering = boost::shared_ptr<Metering>(new Metering(this));
    m_Subsystems.push_back(m_pMetering.get());
    m_pMetering->setMeteringBusInterface(m_pDS485Interface->getMeteringBusInterface());
    m_pModelMaintenance->setMetering(m_pMetering.get());

    m_pFakeMeter = boost::shared_ptr<FakeMeter>(new FakeMeter(this));
    m_Subsystems.push_back(m_pFakeMeter.get());

    m_pEventRunner = boost::shared_ptr<EventRunner>(new EventRunner);
    m_pEventQueue = boost::shared_ptr<EventQueue>(new EventQueue);

    parseProperties(_properties);

    // -- setup logging
    if(!loadConfig(_configFile)) {
      Logger::getInstance()->log("Could not parse config file", lsFatal);
      return false;
    }

    // we need to parse the properties twice to ensure that command line
    // options override config.xml
    parseProperties(_properties);

    // see whether we have a log file set in config.xml, and set the
    // log target accordingly
    PropertyNodePtr pNode = getPropertySystem().getProperty("/config/logfile");
    if (pNode) {
      std::string logFileName = pNode->getStringValue();
      Logger::getInstance()->log("Logging to file: " + logFileName, lsInfo);

      boost::shared_ptr<dss::LogTarget>
        logTarget(new dss::FileLogTarget(logFileName));
      if (!dss::Logger::getInstance()->setLogTarget(logTarget)) {
        Logger::getInstance()->log("Failed to open logfile '" +
                                   logFileName + "'", lsFatal);
        return false;
      }
    } else {
      Logger::getInstance()->log("No logfile configured, logging to stdout",
                                 lsInfo);
    }

    return checkDirectoriesExist();
  } // initialize

  bool DSS::checkDirectoriesExist() {
    bool sane = true;
    if(!isSaneDirectory(m_dataDirectory)) {
      Logger::getInstance()->log("Invalid data directory specified: '" + m_dataDirectory + "'", lsFatal);
      sane = false;
    }

    if(!isSaneDirectory(m_configDirectory)) {
      Logger::getInstance()->log("Invalid config directory specified: '" + m_configDirectory + "'", lsFatal);
      sane = false;
    }

    if(!isSaneDirectory(m_webrootDirectory)) {
      Logger::getInstance()->log("Invalid webroot directory specified: '" + m_webrootDirectory + "'", lsFatal);
      sane = false;
    }

    if(!isSaneDirectory(m_jsLogDirectory)) {
      Logger::getInstance()->log("Invalid js-log directory specified: '" + m_jsLogDirectory + "'", lsFatal);
      sane = false;
    }
    return sane;
  } // checkDirectoriesExist

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
    plugin = new EventInterpreterPluginDS485(getApartment(), m_pDS485Interface.get(), m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterPluginJavascript(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterInternalRelay(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterPluginEmail(m_pEventInterpreter.get());
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

  void StopSubsystem(Subsystem* _pSubsystem) {
    _pSubsystem->shutdown();
  }
  void DSS::run() {
    Logger::getInstance()->log("DSS starting up....", lsInfo);
    Logger::getInstance()->log(versionString(), lsInfo);
    Logger::getInstance()->log("Configuration: ", lsInfo);
    Logger::getInstance()->log("  data:   '" + getDataDirectory() + "'", lsInfo);
    Logger::getInstance()->log("  config: '" + getConfigDirectory() + "'", lsInfo);
    Logger::getInstance()->log("  webroot '" + getWebrootDirectory() + "'", lsInfo);
    Logger::getInstance()->log("  log dir '" + getJSLogDirectory() + "'", lsInfo);

    SystemInfo info;
    info.collect();

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

    // shutdown all subsystems and services
#ifdef WITH_BONJOUR
    bonjour.quit();
    bonjour.terminate();
#endif

    m_State = ssTerminating;
  
    std::for_each(m_Subsystems.begin(), m_Subsystems.end(), StopSubsystem);
    m_pEventQueue->shutdown();
    m_pEventInterpreter->terminate();
  } // run

  void DSS::initiateShutdown() {
    m_pEventRunner->shutdown();
  }

  void DSS::shutdown() {
    DSS* inst = m_Instance;
    m_Instance = NULL;
    delete inst;
  } // shutdown

  int DSS::loadConfigDir(const std::string& _configDir) {
    Logger::getInstance()->log("Loading config directory " + _configDir, lsInfo);

    int n = 0;
    if (!boost::filesystem::exists(_configDir)) return n;
    boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
      for (boost::filesystem::directory_iterator itr(_configDir);
           itr != end_itr;
           ++itr )
      {
        if (boost::filesystem::is_regular_file(itr->status()) &&  (itr->path().extension() == ".xml"))
        {
          Logger::getInstance()->log("Loading config from " + itr->path().file_string(), lsInfo);
          if (getPropertySystem().loadFromXML(itr->path().file_string(), getPropertySystem().getProperty("/config")))
            n++;
        }
      }
      return n;
  } // loadConfigDir

  bool DSS::loadConfig(const std::string& _configFile) {
    m_State = ssLoadingConfig;
    std::string cfgFile;

    if((_configFile.length() > 0) && boost::filesystem::exists(_configFile))
      cfgFile = _configFile;
    else
      cfgFile = getConfigDirectory() + "config.xml";

    Logger::getInstance()->log("Loading config file " + cfgFile, lsInfo);
    getPropertySystem().loadFromXML(cfgFile, getPropertySystem().getProperty("/config"));

    loadConfigDir(getConfigDirectory() + "config.d");
    return true;
  } // loadConfig


  std::string DSS::versionString() {
    std::ostringstream ostr;
    ostr << "DSS";
#ifdef HAVE_CONFIG_H
    ostr << " v" << DSS_VERSION;
#endif
#ifdef HAVE_BUILD_INFO_H
    ostr << " (" << DSS_RCS_REVISION << ")"
         << " (" << DSS_BUILD_USER << "@" << DSS_BUILD_HOST << ")";
#endif
    return ostr.str();
  }

#ifndef WIN32
  void DSS::handleSignal(int _signum) {
    if (DSS::hasInstance()) {
      boost::shared_ptr<Event> pEvent(new Event("SIGNAL"));
      pEvent->setProperty("signum", intToString(_signum));
      DSS::getInstance()->getEventQueue().pushEvent(pEvent);
    }

    switch (_signum) {
      case SIGUSR1: {
        Logger::getInstance()->reopenLogTarget();
        break;
      }
      case SIGTERM:
      case SIGINT:
        if (DSS::hasInstance()) {
            DSS::getInstance()->initiateShutdown();
        }
      break;
      default: {
        std::ostringstream ostr;
        ostr << "DSS::handleSignal(): unhandled signal " << _signum << std::endl;
        Logger::getInstance()->log(ostr.str(), lsWarning);
        break;
      }
    }
  }
#endif
}
