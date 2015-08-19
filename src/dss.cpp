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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifdef HAVE_BUILD_INFO_H
  #include "build_info.h"
#endif


#include "dss.h"

#include <cassert>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#ifndef WIN32
  #include <csignal>
#endif

#include <sys/resource.h>

#include "logger.h"
#include "propertysystem.h"
#include "eventinterpreterplugins.h"
#include "eventinterpretersystemplugins.h"
#include "handler/system_states.h"
#include "src/event.h"
#include "src/ds485/dsbusinterface.h"
#include "src/model/apartment.h"
#include "src/model/modelmaintenance.h"
#include "src/web/webserver.h"
#include "heatingregistering.h"
#include "sensor_data_uploader.h"
#include "subscription_profiler.h"
#include "defaultbuseventsink.h"
#ifdef WITH_BONJOUR
  #include "bonjour.h"
#endif

#include "metering/metering.h"
#include "src/watchdog.h"
#include "foreach.h"
#include "backtrace.h"
#include "src/sessionmanager.h"

#include "src/security/user.h"
#include "src/security/security.h"
#include "src/security/privilege.h"

#include "unix/systeminfo.h"

#include "scripting/jsmodel.h"
#include "scripting/jsevent.h"
#include "scripting/jsmetering.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/finder.hpp>

#include "webservice_connection.h"
#include "model-features.h"

namespace dss {

  //============================================= DSS

#ifndef PACKAGE_DATADIR
    #define PACKAGE_DATADIR "/usr/local/share/dss"
#endif

#ifdef DSS_DATADIR
const char* DataDirectory = DSS_DATADIR;
#else
const char* DataDirectory = PACKAGE_DATADIR "/data/";
#endif

#ifdef DSS_CONFIGDIR
const char* ConfigDirectory = DSS_CONFIGDIR;
#else
const char* ConfigDirectory = PACKAGE_DATADIR "/data/";
#endif

#ifdef DSS_WEBROOTDIR
const char* WebrootDirectory = DSS_WEBROOTDIR;
#else
const char* WebrootDirectory = PACKAGE_DATADIR "/data/webroot/";
#endif

#ifdef DSS_JSLOGDIR
const char* JSLogDirectory = DSS_JSLOGDIR;
#else
const char* JSLogDirectory = PACKAGE_DATADIR "/data/logs/";
#endif

#ifdef DSS_SAVEDPROPSDIR
const char* kSavedPropsDirectory = DSS_SAVEDPROPSDIR;
#else
const char* kSavedPropsDirectory = PACKAGE_DATADIR "/data/savedprops/";
#endif

const char* kDatabaseDirectory = PACKAGE_DATADIR "/data/databases";

  __DEFINE_LOG_CHANNEL__(DSS, lsInfo);

  bool DSS::s_shutdown;

  DSS::DSS()
  : m_commChannel(NULL)
  {
    m_ShutdownFlag = false;
    m_State = ssInvalid;
    m_TimeStarted = time(NULL);

    m_pPropertySystem = boost::make_shared<PropertySystem>();
    setupCommonProperties(*m_pPropertySystem);

    // TODO why this setFooDirectoryPath
    setupDirectories();
    m_pPropertySystem->createProperty("/system/start_time")
      ->setStringValue(DateTime().toString());
    m_pPropertySystem->createProperty("/system/time/current")->linkToProxy(
        PropertyProxyStaticFunction<std::string>(&DSS::getCurrentTime));
    m_pPropertySystem->createProperty("/system/time/current_sec")->linkToProxy(
        PropertyProxyStaticFunction<int>(&DSS::getCurrentTimeSec));
    m_pPropertySystem->createProperty("/system/time/gmt_offset")->linkToProxy(
        PropertyProxyStaticFunction<int>(&DSS::getCurrentGMTOffset));
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
    m_pPropertySystem->createProperty("/config/savedpropsdirectory")->linkToProxy(
        PropertyProxyMemberFunction<DSS,std::string>(*this, &DSS::getSavedPropsDirectory, &DSS::setSavedPropsDirectory));
    m_pPropertySystem->createProperty("/config/debug/propertyNodeCount")->linkToProxy(
        PropertyProxyStaticFunction<int>(&PropertyNode::getNodeCount));
    m_pPropertySystem->createProperty("/config/databasedirectory")->linkToProxy(
        PropertyProxyMemberFunction<DSS,std::string>(*this, &DSS::getDatabaseDirectory, &DSS::setDatabaseDirectory));

  } // ctor

  DSS::~DSS() {
    m_State = ssTerminating;

    m_pWatchdog.reset();

    m_pWebServer.reset();
    m_pWebServices.reset();

    m_pEventQueue.reset();
    m_pEventRunner.reset();

    m_pMetering.reset();

    m_pBusInterface.reset();
    m_pDefaultBusEventSink.reset();
    m_pModelMaintenance.reset();

    m_pEventInterpreter.reset();

    m_pApartment.reset();
    WebserviceConnection::shutdown();

    if (m_commChannel) {
      delete m_commChannel;
      m_commChannel = NULL;
    }
  }

  void DSS::setupDirectories()
  {
#if defined(__CYGWIN__)
    char AppDataDir[259];
    char WebDir[259];
    char JSLogDir[259];
    char SavedPropsDir[259];
    std::string aup = getenv("ALLUSERSPROFILE");

    if (std::string::npos != aup.std::string::find("ProgramData")) {
      sprintf(AppDataDir, "%s\\digitalSTROM\\DssData", aup.c_str());
    } else {
      std::string apd = getenv("APPDATA");
      int index = apd.std::string::find_last_of("\\");
      aup.std::string::append(apd.std::string::substr(index));
      sprintf(AppDataDir, "%s\\digitalSTROM\\DssData", aup.c_str());
    }

    sprintf(WebDir, "%s\\webroot",AppDataDir);
    sprintf(JSLogDir, "%s\\logs",AppDataDir);
    sprintf(SavedPropsDir, "%s\\savedprops", AppDataDir);

    setDataDirectory(AppDataDir);
    setConfigDirectory(AppDataDir);
    setWebrootDirectory(WebDir);
    setJSLogDirectory(JSLogDir);
    setSavedPropsDirectory(SavedPropsDir);
#else
    setDataDirectory(DataDirectory);
    setConfigDirectory(ConfigDirectory);
    setWebrootDirectory(WebrootDirectory);
    setJSLogDirectory(JSLogDirectory);
    setSavedPropsDirectory(kSavedPropsDirectory);
    setDatabaseDirectory(kDatabaseDirectory);
#endif
  } // setupDirectories

  int DSS::getUptime() const {
    return (int)difftime( time( NULL ), m_TimeStarted );
  } // getUptime

  time_t DSS::getStartTime() const {
    return m_TimeStarted;
  } // getStartTime

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

  void DSS::setSavedPropsDirectory(const std::string& _value) {
    m_savedPropsDirectory = addTrailingBackslash(_value);
  } // setSavedPropsDirectory

  void DSS::setDatabaseDirectory(const std::string& _value) {
    m_databaseDirectory = addTrailingBackslash(_value);
  } // setDatabaseDirectory


  bool DSS::parseProperties(const std::vector<std::string>& _properties) {
    foreach(std::string propLine, _properties) {
      std::string::size_type pos = propLine.find("=");
      if(pos == std::string::npos) {
        log("invalid property specified on commandline (format is name=value): '" +
            propLine + "'", lsError);
        return false;
      } else {
        std::string name = propLine.substr(0, pos);
        std::string value = propLine.substr(pos+1, std::string::npos);
        log("Setting property '" + name + "' to '" + value + "'", lsInfo);
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

  bool DSS::initialize(const std::vector<std::string>& _properties, const std::string& _configFile) {
    log("DSS::initialize", lsInfo);
    m_State = ssCreatingSubsystems;

    ModelFeatures::createInstance();

    try {
      m_commChannel = CommChannel::createInstance();
      m_commChannel->run(); // TODO move this to ::run
      m_commChannel->suspendUpdateTask();
    } catch (std::runtime_error &err) {
      delete m_commChannel;
      log("Could not start dSA communication channel: " + std::string(err.what()), lsError);
    } catch (...) {
      delete m_commChannel;
      log("Could not start dSA communication channel: unkown error", lsError);
      return false;
    }

    m_pMetering = boost::make_shared<Metering>(this);
    m_Subsystems.push_back(m_pMetering.get());

    // will start a thread
    m_pModelMaintenance = boost::make_shared<ModelMaintenance>(this);
    m_Subsystems.push_back(m_pModelMaintenance.get());

    m_pApartment = boost::make_shared<Apartment>(this);
    m_pApartment->setPropertySystem(m_pPropertySystem.get());
    m_pModelMaintenance->setApartment(m_pApartment.get());

    m_pEventInterpreter = boost::make_shared<EventInterpreter>(this);
    m_Subsystems.push_back(m_pEventInterpreter.get());

    boost::shared_ptr<DSBusInterface> pDSBusInterface = boost::make_shared<DSBusInterface>(this, m_pModelMaintenance.get());
    m_Subsystems.push_back(pDSBusInterface.get());

    m_pDefaultBusEventSink = boost::make_shared<DefaultBusEventSink>(m_pApartment, m_pModelMaintenance);
    pDSBusInterface->setBusEventSink(m_pDefaultBusEventSink.get());

    m_pWebServer = boost::make_shared<WebServer>(this);
    m_Subsystems.push_back(m_pWebServer.get());

    m_pBusInterface = boost::shared_ptr<BusInterface>(pDSBusInterface);
    m_pApartment->setBusInterface(m_pBusInterface.get());
    m_pModelMaintenance->setStructureModifyingBusInterface(m_pBusInterface->getStructureModifyingBusInterface());
    m_pModelMaintenance->setStructureQueryBusInterface(m_pBusInterface->getStructureQueryBusInterface());

    m_pMetering->setMeteringBusInterface(m_pBusInterface->getMeteringBusInterface());
    m_pModelMaintenance->setMetering(m_pMetering.get());
    m_pApartment->setMetering(m_pMetering.get());

    PropertyNodePtr eventMonitor = m_pPropertySystem->createProperty("/system/EventInterpreter/ScheduledEvents");
    m_pEventRunner = boost::make_shared<EventRunner>(m_pEventInterpreter.get(), eventMonitor);
    m_pEventQueue = boost::make_shared<EventQueue>(m_pEventInterpreter.get());

    m_pSecurity.reset(
        new Security(m_pPropertySystem->createProperty("/system/security")));
    m_pSessionManager.reset(
      new SessionManager(getEventQueue(),
                         getEventInterpreter(),
                         m_pSecurity));
    m_pWebServer->setSessionManager(m_pSessionManager);

    parseProperties(_properties);

    // -- setup logging
    if(!loadConfig(_configFile)) {
      log("Could not parse config file", lsFatal);
      return false;
    }

    // we need to parse the properties twice to ensure that command line
    // options override config.xml
    parseProperties(_properties);

    WebserviceConnection::getInstanceMsHub();
    WebserviceConnection::getInstanceDsHub();

    // see whether we have a log file set in config.xml, and set the
    // log target accordingly
    PropertyNodePtr pNode = getPropertySystem().getProperty("/config/logfile");
    if (pNode) {
      std::string logFileName = pNode->getStringValue();
      log("Logging to file: " + logFileName, lsInfo);

      boost::shared_ptr<dss::LogTarget>
        logTarget(new dss::FileLogTarget(logFileName));
      if (!dss::Logger::getInstance()->setLogTarget(logTarget)) {
        log("Failed to open logfile '" + logFileName + "'", lsFatal);
        return false;
      }
    } else {
      log("No logfile configured, logging to stdout", lsInfo);
    }
    pNode = getPropertySystem().getProperty("/config/loglevel");
    if (pNode) {
      aLogSeverity logLevel = static_cast<aLogSeverity> (pNode->getIntegerValue());
      Logger::getInstance()->getLogChannel()->setMinimumSeverity(logLevel);
    }

    m_pWatchdog = boost::make_shared<Watchdog>(this);
    m_Subsystems.push_back(m_pWatchdog.get());
    return checkDirectoriesExist();
  } // initialize

  bool DSS::checkDirectoriesExist() {
    bool sane = true;
    if(!isSaneDirectory(m_dataDirectory)) {
      log("Invalid data directory specified: '" + m_dataDirectory + "'", lsFatal);
      sane = false;
    }

    if(!isSaneDirectory(m_configDirectory)) {
      log("Invalid config directory specified: '" + m_configDirectory + "'", lsFatal);
      sane = false;
    }

    if(!isSaneDirectory(m_webrootDirectory)) {
      log("Invalid webroot directory specified: '" + m_webrootDirectory + "'", lsFatal);
      sane = false;
    }

    if(!isSaneDirectory(m_jsLogDirectory)) {
      log("Invalid js-log directory specified: '" + m_jsLogDirectory + "'", lsFatal);
      sane = false;
    }

    if(!isSaneDirectory(m_savedPropsDirectory)) {
      log("Invalid saved-props directory specified: '" + m_savedPropsDirectory + "'", lsFatal);
      sane = false;
    }

    if(!isSaneDirectory(m_databaseDirectory)) {
      log("Invalid database directory specified: '" + m_databaseDirectory + "'", lsFatal);
      sane = false;
    }

    return sane;
  } // checkDirectoriesExist

  DSS* DSS::m_Instance = NULL;
  int DSS::s_InstanceGeneration = 0;

  DSS* DSS::getInstance() {
    if (s_shutdown) {
      assert(false);
    }

    if (m_Instance == NULL) {
      m_Instance = new DSS();
      s_InstanceGeneration++;
      log("getInstance: create new -- " +
          intToString(reinterpret_cast<long long int>(m_Instance), true),
          lsInfo);
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
    plugin = new EventInterpreterPluginJavascript(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterInternalRelay(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterPluginSendmail(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterPluginActionExecute(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterPluginHighlevelEvent(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterPluginSystemTrigger(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterPluginExecutionDeniedDigest(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterPluginApartmentChange(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterPluginSystemEventLog(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterPluginSystemState(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new BenchmarkPublisherPlugin(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterWebservicePlugin(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterStateSensorPlugin(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterSensorMonitorPlugin(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterHeatingMonitorPlugin(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new SensorDataUploadMsHubPlugin(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new SensorDataUploadDsHubPlugin(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterHeatingValveProtectionPlugin(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new HeatingRegisteringPlugin(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);
    plugin = new EventInterpreterDebugMonitorPlugin(m_pEventInterpreter.get());
    m_pEventInterpreter->addPlugin(plugin);

    m_pEventRunner->setEventQueue(m_pEventQueue.get());
    m_pEventInterpreter->setEventRunner(m_pEventRunner.get());
    m_pEventQueue->setEventRunner(m_pEventRunner.get());
    m_pEventInterpreter->setEventQueue(m_pEventQueue.get());
  }

  bool DSS::initSubsystems() {
    foreach (Subsystem *subsys, m_Subsystems) {
      if (m_ShutdownFlag) {
        return false;
      }

      try {
        log("Initialize subsystem \"" + subsys->getName() + "\"", lsDebug);
        subsys->initialize();
      } catch(std::exception& e) {
        log("Failed to initialize subsystem '" + subsys->getName() + "': " + e.what(),
            lsFatal);
        m_State = ssTerminating;
        return false;
      }
    }

    return true;
  }

  bool DSS::initSecurity() {
    PropertyNodePtr pDigestFile = getPropertySystem().getProperty("/config/digestfile");
    boost::shared_ptr<PasswordChecker> checker;
    if (pDigestFile != NULL) {
      log("Using digest file: '" + pDigestFile->getStringValue() +
          "' for authentication.", lsInfo);
      checker.reset(new HTDigestPasswordChecker(pDigestFile->getStringValue()));
    } else {
      log("Using internal authentication mechanism.", lsInfo);
      checker.reset(new BuiltinPasswordChecker());
    }
    m_pSecurity->setPasswordChecker(checker);
    m_pSecurity->setFileName(getDataDirectory() + "security.xml");
    m_pSecurity->loadFromXML();

    PropertyNodePtr pSecurityNode = m_pPropertySystem->getProperty("/system/security");
    pSecurityNode->setFlag(PropertyNode::Archive, true);
    pSecurityNode->createProperty("users")->setFlag(PropertyNode::Archive, true);

    // recreate system user, it's not archived
    PropertyNodePtr pSystemNode = pSecurityNode->createProperty("users/system");

    // set username/password to a dummy-value so nobody's able to log in
    pSystemNode->createProperty("salt")->setStringValue("dummyvalue");
    pSystemNode->createProperty("password")->setStringValue("dummyvalue");

    m_pSecurity->addSystemRole(pSystemNode);
    m_pSecurity->setSystemUser(new User(pSystemNode));

    // setup owner user
    PropertyNodePtr pOwnerNode = pSecurityNode->getProperty("users/dssadmin");
    if (pOwnerNode == NULL) {
      pOwnerNode = pSecurityNode->createProperty("users/dssadmin");
      pOwnerNode->setFlag(PropertyNode::Archive, true);
      m_pSecurity->addUserRole(pOwnerNode);
      User(pOwnerNode).setPassword("dssadmin"); // default password
    } else {
      /* role membership is not stored */
      m_pSecurity->addUserRole(pOwnerNode);
    }

    boost::shared_ptr<Privilege>
      privilegeSystem(
        new Privilege(
          pSecurityNode->getProperty("roles/system")));
    privilegeSystem->addRight(Privilege::Write);

    boost::shared_ptr<Privilege>
      privilegeOwner(
        new Privilege(
          pSecurityNode->getProperty("roles/owner")));
    privilegeOwner->addRight(Privilege::Write);

    NodePrivileges* privileges = new NodePrivileges;
    privileges->addPrivilege(privilegeSystem);
    privileges->addPrivilege(privilegeOwner);
    m_pPropertySystem->getProperty("/")->setPrivileges(privileges);

    NodePrivileges* privilegesSecurityNode = new NodePrivileges;
    privilegesSecurityNode->addPrivilege(privilegeSystem);
    pSecurityNode->setPrivileges(privilegesSecurityNode);

    m_pSecurity->startListeningForChanges();
    return true;
  } // initSecurity

  void DSS::run() {
    log("DSS starting up....", lsInfo);
    log(versionString(), lsInfo);
    log("Configuration: ", lsInfo);
    log("  data:         '" + getDataDirectory() + "'", lsInfo);
    log("  config:       '" + getConfigDirectory() + "'", lsInfo);
    log("  webroot:      '" + getWebrootDirectory() + "'", lsInfo);
    log("  log dir:      '" + getJSLogDirectory() + "'", lsInfo);
    log("  props dir:    '" + getSavedPropsDirectory() + "'", lsInfo);
    log("  database dir: '" + getDatabaseDirectory() + "'", lsInfo);

    SystemInfo info;
    info.collect();
    publishDSID();

    m_State = ssInitializingSubsystems;

    addDefaultInterpreterPlugins();

    if (!initSecurity()) {
      log("Failed to initialize security, exiting...", lsFatal);
      return;
    }
    m_pSecurity->loginAsSystemUser("Main thread needs system privileges");

    if (!initSubsystems()) {
      log("Failed to initialize subsystems, exiting...", lsFatal);
      return;
    }

    m_State = ssStarting;
    foreach (Subsystem *subsys, m_Subsystems) {
      log("Start subsystem \"" + subsys->getName() + "\"", lsDebug);
      subsys->start();
    }

#ifdef WITH_BONJOUR
    m_pBonjour = boost::make_shared<BonjourHandler>();
    m_pBonjour->run();
#endif

    if (!m_ShutdownFlag) {
      m_State = ssRunning;
      boost::shared_ptr<Event> runningEvent = boost::make_shared<Event>(EventName::Running);
      m_pEventQueue->pushEvent(runningEvent);

      // pass control to the eventrunner
      m_pEventRunner->run();
    }

    m_State = ssTerminating;

    foreach (Subsystem *subsys, m_Subsystems) {
      log("Shutdown subsystem \"" + subsys->getName() + "\"", lsDebug);
      subsys->shutdown();
    }

    m_pEventQueue->shutdown();
    m_pEventInterpreter->terminate();

    // shutdown all subsystems and services
#ifdef WITH_BONJOUR
    m_pBonjour->quit();
    m_pBonjour->terminate();
#endif
    if (m_commChannel) {
      m_commChannel->shutdown();
    }
  } // run

  void DSS::initiateShutdown() {
    m_ShutdownFlag = true;

    if (m_State != ssRunning) {
      return;
    }

    if (m_pEventRunner != NULL) {
      m_pEventRunner->shutdown();
    }
  }

  void DSS::shutdown() {
    if (!m_Instance) {
      return;
    }
    log("DSS::shutdown " +
        intToString(reinterpret_cast<long long int>(m_Instance), true),
        lsInfo);
    if (m_Instance->m_pSecurity) {
      m_Instance->getSecurity().loginAsSystemUser("Shutdown needs to be as system user");
    }
    DSS* inst = m_Instance;
    m_Instance = NULL;
    s_shutdown = true;
    delete inst;
    s_shutdown = false;
  } // shutdown

  int DSS::loadConfigDir(const std::string& _configDir) {
    log("Loading config directory " + _configDir, lsInfo);

    int n = 0;
    if (!boost::filesystem::exists(_configDir)) return n;
    boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
      for (boost::filesystem::directory_iterator itr(_configDir);
           itr != end_itr;
           ++itr )
      {
        if (boost::filesystem::is_regular_file(itr->status()) &&  (itr->path().extension() == ".xml"))
        {
#if defined(BOOST_VERSION_135)
          log("Loading config from " + itr->path().file_string(), lsInfo);
          if (loadFromXML(itr->path().file_string(), getPropertySystem().getProperty("/config")))
#else
          log("Loading config from " + itr->path().string(), lsInfo);
          if (loadFromXML(itr->path().string(), getPropertySystem().getProperty("/config")))
#endif
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

    log("Loading config file " + cfgFile, lsInfo);
    loadFromXML(cfgFile, getPropertySystem().getProperty("/config"));

    loadConfigDir(getConfigDirectory() + "config.d");
    return true;
  } // loadConfig


  std::string DSS::versionString() {
    std::ostringstream ostr;
    ostr << "dSS";
#ifdef HAVE_CONFIG_H
    ostr << " v" << DSS_VERSION;
#endif
#ifdef HAVE_BUILD_INFO_H
    ostr << " (" << DSS_RCS_REVISION << ")"
         << " (" << DSS_BUILD_USER << "@" << DSS_BUILD_HOST << ")";
#endif
    return ostr.str();
  }

  std::vector<unsigned char> DSS::getRandomSalt(unsigned int len) {
#if defined(__linux__) || defined(__APPLE__)
    std::vector<unsigned char> urandom(len);

    std::ifstream file ("/dev/urandom", std::ios::binary);
    if (!file.is_open()) {
      /* TODO this will probably crash the server at startup */
      throw std::runtime_error("failed to open /dev/urandom");
    }

    file.read(reinterpret_cast<char *>(&urandom[0]), urandom.size());
    if (!file) {
      throw std::runtime_error("failed to read from /dev/urandom");
    }
    file.close();
    return urandom;
#else
#error missing random genarator on platform
#endif
  }

  void init_libraries() {
    /*
     * we could implement decentralized approach using linker or compiler features
     * http://stackoverflow.com/questions/2053029/how-exactly-does-attribute-constructor-work
     */
    curl_global_init(CURL_GLOBAL_ALL);
  }

  void cleanup_libraries() {
    google::protobuf::ShutdownProtobufLibrary();
    curl_global_cleanup();
  }

#ifndef WIN32
  void* DSS::handleSignal(void* arg) {
    sigset_t signal_set;
    int sig;

    pthread_detach(pthread_self());

    /* wait for any and all signals */
    sigfillset(&signal_set);
    sigdelset(&signal_set, SIGUSR2); /* gperftools dump profile */
    sigdelset(&signal_set, SIGPROF); /* gperftools cpu profiler trigger */

    for (;;) {
      sigwait(&signal_set, &sig);
      switch (sig) {
        case SIGUSR1: {
          Logger::getInstance()->reopenLogTarget();
          break;
        }
        case SIGTERM:
          if (DSS::hasInstance()) {
              DSS::getInstance()->initiateShutdown();
          }
          break;
        default: {
          log("System signal unhandled: " + intToString(sig), lsDebug);
          break;
        }
      }
      if (DSS::hasInstance()) {
        boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(EventName::Signal);
        pEvent->setProperty("signum", intToString(sig));
        DSS::getInstance()->getEventQueue().pushEvent(pEvent);
      }
    }
    return (void*)0;
  }
#endif

  std::string DSS::readDistroVersion() {
    std::string version = "";
    try {
#ifdef DISTRO_VERSION_FILE
      std::string filename = std::string(DISTRO_VERSION_FILE);
#else
      std::string filename = "/etc/issue";
#endif

      if ((filename.size() > 0) && boost::filesystem::exists(filename)) {
        std::ifstream in(filename.c_str());
        if (!in.is_open()) {
          return version;
        }
        std::getline(in, version);
        in.close();
      }
    } catch (...) {}
    return version;
  }

  void DSS::publishDSID() {
    std::string dsid, dsuid;
    PropertyNodePtr lastValidIf;
    PropertyNodePtr interfaces =
        getPropertySystem().getProperty(
                "/system/host/interfaces");
    if (interfaces != NULL) {
        for (int i = 0; i < interfaces->getChildCount(); i++) {
            PropertyNodePtr iface = interfaces->getChild(i);
            if (iface->getName() != "lo") {
                lastValidIf = iface;
            }
            // The Ethernet MAC interface may have eth0 or eth0:alias name
            if (beginsWith(iface->getName(), "eth0")) {
                break;
            }
        }
    }

    if (lastValidIf != NULL) {
        PropertyNodePtr macNode = lastValidIf->getPropertyByName("mac");
        if (macNode != NULL) {
            std::string mac = macNode->getAsString();
            mac.erase(std::remove(mac.begin(), mac.end(), ':'), mac.end());
            dsuid_t du;
            dsid_t di;
            DsmApiGetEthernetDSUID(mac.c_str(), &du);
            dsuid = dsuid2str(du);
            if (dsuid_to_dsid(du, &di)) {
              dsid = dsid2str(di);
            }
        }
    }

    PropertyNodePtr dsidNode = getPropertySystem().createProperty(pp_sysinfo_dsid);
    dsidNode->setStringValue(dsid);
    dsidNode = getPropertySystem().createProperty(pp_sysinfo_dsuid);
    dsidNode->setStringValue(dsuid);
  }

  std::string DSS::getCurrentTime()
  {
    return DateTime().toISO8601_local();
  }

  int DSS::getCurrentTimeSec()
  {
    return DateTime().secondsSinceEpoch();
  }

  int DSS::getCurrentGMTOffset()
  {
    return DateTime().getTimezoneOffset();
  }
}
