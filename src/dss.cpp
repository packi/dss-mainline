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
#include "src/ds485/dsbusinterface.h"
#include "src/dsidhelper.h"
#include "src/model/apartment.h"
#include "src/model/modelmaintenance.h"
#include "src/web/webserver.h"
#include "subscription_profiler.h"

#ifdef WITH_BONJOUR
  #include "bonjour.h"
#endif

#include "sim/dssim.h"
#include "sim/businterface/simbusinterface.h"
#include "sim/businterface/businterfaceadaptor.h"
#ifdef WITH_SOAP
  #include "webservices/webservices.h"
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

  DSS::DSS()
  {
    m_ShutdownFlag = false;
    m_State = ssInvalid;
    m_pPropertySystem = boost::shared_ptr<PropertySystem>(new PropertySystem);
    setupDirectories();

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
    m_pPropertySystem->createProperty("/config/savedpropsdirectory")->linkToProxy(
        PropertyProxyMemberFunction<DSS,std::string>(*this, &DSS::getSavedPropsDirectory, &DSS::setSavedPropsDirectory));
    m_pPropertySystem->createProperty("/config/debug/coredumps/enabled")->setBooleanValue(false);
    m_pPropertySystem->createProperty("/config/debug/coredumps/limit")->setFloatingValue(RLIM_INFINITY);
    m_pPropertySystem->createProperty("/system/version/version")->setStringValue(DSS_VERSION);
    m_pPropertySystem->createProperty("/system/version/distroVersion")->setStringValue(readDistroVersion());
    m_pPropertySystem->createProperty("/system/version/buildHost")->setStringValue(DSS_BUILD_HOST);
    m_pPropertySystem->createProperty("/system/version/gitRevision")->setStringValue(DSS_RCS_REVISION);
    m_pPropertySystem->createProperty(ModelChangedEvent::propPathDelay)->setIntegerValue(30);
    m_pPropertySystem->createProperty("/config/webservice-api/base-url")
        ->setStringValue("https://dsservices.aizo.com");

    m_pPropertySystem->createProperty(ModelChangedEvent::propPathUrl)
        ->setStringValue("internal/dss/v1_0/DSSApartment/ApartmentHasChanged");

    m_pPropertySystem->createProperty("/config/webservice-api/enabled")->setBooleanValue(false);
  } // ctor

  DSS::~DSS() {
    m_State = ssTerminating;

    m_pWatchdog.reset();

    m_pWebServer.reset();
    m_pWebServices.reset();

    m_pEventQueue.reset();
    m_pEventRunner.reset();

    m_pEventInterpreter.reset();
    m_pMetering.reset();

    m_pSimulation.reset();

    m_pBusInterface.reset();
    m_pModelMaintenance.reset();

    m_pApartment.reset();
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
#endif
  } // setupDirectories

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

  void DSS::setSavedPropsDirectory(const std::string& _value) {
    m_savedPropsDirectory = addTrailingBackslash(_value);
  } // setSavedPropsDirectory

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

  bool DSS::initialize(const std::vector<std::string>& _properties, const std::string& _configFile) {
    m_State = ssCreatingSubsystems;

    try {
      m_commChannel = CommChannel::createInstance();
      m_commChannel->run();
      m_commChannel->suspendUpdateTask();
    } catch (std::runtime_error &err) {
      Logger::getInstance()->log("Could not start dSA communication channel: " +
              std::string(err.what()), lsError);
    }

    m_pMetering = boost::shared_ptr<Metering>(new Metering(this));
    m_Subsystems.push_back(m_pMetering.get());

    m_pModelMaintenance = boost::shared_ptr<ModelMaintenance>(new ModelMaintenance(this));
    m_Subsystems.push_back(m_pModelMaintenance.get());

    m_pApartment = boost::shared_ptr<Apartment>(new Apartment(this));
    m_pApartment->setPropertySystem(m_pPropertySystem.get());
    m_pModelMaintenance->setApartment(m_pApartment.get());

    m_pEventInterpreter = boost::shared_ptr<EventInterpreter>(new EventInterpreter(this));
    m_Subsystems.push_back(m_pEventInterpreter.get());

    m_pSimulation = boost::shared_ptr<DSSim>(new DSSim(this));
    m_Subsystems.push_back(m_pSimulation.get());

    boost::shared_ptr<DSBusInterface> pDSBusInterface(new DSBusInterface(this, m_pModelMaintenance.get()));
    m_Subsystems.push_back(pDSBusInterface.get());

    boost::shared_ptr<SimBusInterface> pSimBusInterface(new SimBusInterface(m_pSimulation));

    m_pWebServer = boost::shared_ptr<WebServer>(new WebServer(this));
    m_Subsystems.push_back(m_pWebServer.get());

#ifdef WITH_SOAP
    m_pWebServices = boost::shared_ptr<WebServices>(new WebServices(this));
    m_Subsystems.push_back(m_pWebServices.get());
#endif

    m_pBusInterface = boost::shared_ptr<BusInterface>(
      new BusInterfaceAdaptor(pDSBusInterface, m_pSimulation,
                              pSimBusInterface, m_pModelMaintenance,
                              m_pApartment));
    m_pApartment->setBusInterface(m_pBusInterface.get());
    m_pModelMaintenance->setStructureModifyingBusInterface(m_pBusInterface->getStructureModifyingBusInterface());
    m_pModelMaintenance->setStructureQueryBusInterface(m_pBusInterface->getStructureQueryBusInterface());

    m_pMetering->setMeteringBusInterface(m_pBusInterface->getMeteringBusInterface());
    m_pModelMaintenance->setMetering(m_pMetering.get());
    m_pApartment->setMetering(m_pMetering.get());

    PropertyNodePtr eventMonitor = m_pPropertySystem->createProperty("/system/EventInterpreter/ScheduledEvents");
    m_pEventRunner = boost::shared_ptr<EventRunner>(new EventRunner(m_pEventInterpreter.get(), eventMonitor));
    m_pEventQueue = boost::shared_ptr<EventQueue>(new EventQueue(m_pEventInterpreter.get()));

    std::string randomSalt;
#ifdef __linux__
    {
      long long int u;
      std::ifstream file ("/dev/urandom", std::ios::binary);
      if (file.is_open()) {
        char *urandom;
        int size = sizeof(u);
        urandom = new char [size];
        file.read(urandom, size);
        file.close();
        u = *reinterpret_cast<long long int*>(urandom);
        delete[] urandom;
        std::ostringstream s;
        s << std::hex << u;
        randomSalt = s.str();
      }
    }
#endif

    m_pSecurity.reset(
        new Security(m_pPropertySystem->createProperty("/system/security"),
                     m_pPropertySystem));
    m_pSessionManager.reset(
      new SessionManager(getEventQueue(),
                         getEventInterpreter(),
                         m_pSecurity,
                         randomSalt));
    m_pWebServer->setSessionManager(m_pSessionManager);
#ifdef WITH_SOAP
    m_pWebServices->setSessionManager(m_pSessionManager);
#endif

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
    pNode = getPropertySystem().getProperty("/config/loglevel");
    if (pNode) {
      aLogSeverity logLevel = static_cast<aLogSeverity> (pNode->getIntegerValue());
      Logger::getInstance()->getLogChannel()->setMinimumSeverity(logLevel);
    }

    m_pWatchdog = boost::shared_ptr<Watchdog>(new Watchdog(this));
    m_Subsystems.push_back(m_pWatchdog.get());

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

    if(!isSaneDirectory(m_savedPropsDirectory)) {
      Logger::getInstance()->log("Invalid saved-props directory specified: '" + m_savedPropsDirectory + "'", lsFatal);
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

    m_pEventRunner->setEventQueue(m_pEventQueue.get());
    m_pEventInterpreter->setEventRunner(m_pEventRunner.get());
    m_pEventQueue->setEventRunner(m_pEventRunner.get());
    m_pEventInterpreter->setEventQueue(m_pEventQueue.get());
  }

  void InitializeSubsystem(Subsystem* _pSubsystem) {
    Logger::getInstance()->log("Initialize subsystem \"" +
        _pSubsystem->getName() + "\"", lsDebug);
    _pSubsystem->initialize();
  } // initializeSubsystem

  void StartSubsystem(Subsystem* _pSubsystem) {
    Logger::getInstance()->log("Start subsystem \"" +
        _pSubsystem->getName() + "\"", lsDebug);
    _pSubsystem->start();
  }

  void StopSubsystem(Subsystem* _pSubsystem) {
    Logger::getInstance()->log("Shutdown subsystem \"" +
        _pSubsystem->getName() + "\"", lsDebug);
    _pSubsystem->shutdown();
  }

  bool DSS::initSubsystems() {
    for (size_t i = 0; i < m_Subsystems.size(); i++) {
      if(m_ShutdownFlag) {
        return false;
      }

      try {
        InitializeSubsystem(m_Subsystems.at(i));
      } catch(std::exception& e) {
        Logger::getInstance()->log("Failed to initialize subsystem '" +
                                   m_Subsystems.at(i)->getName() +
                                   "': " + e.what(), lsFatal);
        m_State = ssTerminating;
        return false;
      }
    }

    return true;
  }

  bool DSS::initSecurity() {
    PropertyNodePtr pDigestFile = getPropertySystem().getProperty("/config/digestfile");
    boost::shared_ptr<PasswordChecker> checker;
    if(pDigestFile != NULL) {
      Logger::getInstance()->log("Using digest file: '" +
    		  pDigestFile->getStringValue() +
    		  "' for authentication.", lsInfo);
	  checker.reset(new HTDigestPasswordChecker(pDigestFile->getStringValue()));
    } else {
      Logger::getInstance()->log("Using internal authentication mechanism.", lsInfo);
      checker.reset(new BuiltinPasswordChecker());
    }
    m_pSecurity->setPasswordChecker(checker);
    m_pSecurity->setFileName(getDataDirectory() + "security.xml");
    m_pSecurity->loadFromXML();
    PropertyNodePtr pSecurityNode = m_pPropertySystem->getProperty("/system/security");
    pSecurityNode->setFlag(PropertyNode::Archive, true);
    pSecurityNode->createProperty("users")->setFlag(PropertyNode::Archive, true);

    // setup system user
    PropertyNodePtr pSystemNode = pSecurityNode->getProperty("users/system");
    if(pSystemNode == NULL) {
      PropertyNodePtr pSystemNode = pSecurityNode->createProperty("users/system");
      PropertyNodePtr pRoleNode = pSecurityNode->getProperty("roles/system");
      if(pRoleNode == NULL) {
        pRoleNode = pSecurityNode->createProperty("roles/system");
      }
      pSystemNode->createProperty("role")->alias(pRoleNode);
      // set username/password to a dummy-value so nobody's able to log in
      pSystemNode->createProperty("salt")->setStringValue("dummyvalue");
      pSystemNode->createProperty("password")->setStringValue("dummyvalue");
    }
    m_pSecurity->setSystemUser(new User(pSecurityNode->getProperty("users/system")));

    // setup owner user
    PropertyNodePtr pOwnerNode = pSecurityNode->getProperty("users/dssadmin");
    if(pOwnerNode == NULL) {
      pOwnerNode = pSecurityNode->createProperty("users/dssadmin");
      pOwnerNode->setFlag(PropertyNode::Archive, true);
    }
    PropertyNodePtr pRoleOwnerNode = pSecurityNode->getProperty("roles/owner");
    if(pRoleOwnerNode == NULL) {
      pRoleOwnerNode = pSecurityNode->createProperty("roles/owner");
    }
    pOwnerNode->createProperty("role")->alias(pRoleOwnerNode);
    if(pOwnerNode->getProperty("password") == NULL) {
      boost::shared_ptr<User> user(new User(pOwnerNode));
      user->setPassword("dssadmin"); // default password for dssadmin
    }

    boost::shared_ptr<Privilege>
      privilegeSystem(
        new Privilege(
          pSecurityNode->getProperty("roles/system")));
    privilegeSystem->addRight(Privilege::Read);
    privilegeSystem->addRight(Privilege::Write);
    privilegeSystem->addRight(Privilege::Security);

    boost::shared_ptr<Privilege>
      privilegeOwner(
        new Privilege(
          pSecurityNode->getProperty("roles/owner")));

    privilegeOwner->addRight(Privilege::Read);
    privilegeOwner->addRight(Privilege::Write);

    boost::shared_ptr<Privilege>
      privilegeNobody(
        new Privilege(
          PropertyNodePtr()));
    boost::shared_ptr<NodePrivileges> privileges(new NodePrivileges());
    privileges->addPrivilege(privilegeSystem);
    privileges->addPrivilege(privilegeOwner);
    privileges->addPrivilege(privilegeNobody);
    m_pPropertySystem->getProperty("/")->setPrivileges(privileges);

    boost::shared_ptr<Privilege>
      privilegeNobodySecurityNode(
        new Privilege(
          PropertyNodePtr()));
    privilegeNobodySecurityNode->addRight(Privilege::Read);
    boost::shared_ptr<NodePrivileges> privilegesSecurityNode(new NodePrivileges());
    privilegesSecurityNode->addPrivilege(privilegeNobodySecurityNode);
    pSecurityNode->setPrivileges(privilegesSecurityNode);

    m_pSecurity->startListeningForChanges();
    return true;
  } // initSecurity

  void DSS::run() {
    Logger::getInstance()->log("DSS starting up....", lsInfo);
    Logger::getInstance()->log(versionString(), lsInfo);
    Logger::getInstance()->log("Configuration: ", lsInfo);
    Logger::getInstance()->log("  data:      '" + getDataDirectory() + "'", lsInfo);
    Logger::getInstance()->log("  config:    '" + getConfigDirectory() + "'", lsInfo);
    Logger::getInstance()->log("  webroot:   '" + getWebrootDirectory() + "'", lsInfo);
    Logger::getInstance()->log("  log dir:   '" + getJSLogDirectory() + "'", lsInfo);
    Logger::getInstance()->log("  props dir: '" + getSavedPropsDirectory() + "'", lsInfo);

    SystemInfo info;
    info.collect();
    publishDSID();

    m_State = ssInitializingSubsystems;

    addDefaultInterpreterPlugins();

    if (!initSecurity()) {
      Logger::getInstance()->log("Failed to initialize security, exiting...", lsFatal);
      return;
    }
    m_pSecurity->loginAsSystemUser("Main thread needs system privileges");

    if (!initSubsystems()) {
      Logger::getInstance()->log("Failed to initialize subsystems, exiting...", lsFatal);
      return;
    }

    m_State = ssStarting;
    std::for_each(m_Subsystems.begin(), m_Subsystems.end(), StartSubsystem);

#ifdef WITH_BONJOUR
    m_pBonjour = boost::shared_ptr<BonjourHandler>(new BonjourHandler());
    m_pBonjour->run();
#endif

    if (!m_ShutdownFlag) {
      m_State = ssRunning;
      boost::shared_ptr<Event> runningEvent(new Event("running"));
      m_pEventQueue->pushEvent(runningEvent);

      // pass control to the eventrunner
      m_pEventRunner->run();
    }

    m_State = ssTerminating;

    std::for_each(m_Subsystems.begin(), m_Subsystems.end(), StopSubsystem);
    m_pEventQueue->shutdown();
    m_pEventInterpreter->terminate();

    // shutdown all subsystems and services
#ifdef WITH_BONJOUR
    m_pBonjour->quit();
    m_pBonjour->terminate();
#endif

    if (m_commChannel) {
      delete m_commChannel;
      m_commChannel = NULL;
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
    DSS::getInstance()->getSecurity().
      loginAsSystemUser("Shutdown needs to be as system user");
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
#if defined(BOOST_VERSION_135)
          Logger::getInstance()->log("Loading config from " + itr->path().file_string(), lsInfo);
          if (getPropertySystem().loadFromXML(itr->path().file_string(), getPropertySystem().getProperty("/config")))
#else
          Logger::getInstance()->log("Loading config from " + itr->path().string(), lsInfo);
          if (getPropertySystem().loadFromXML(itr->path().string(), getPropertySystem().getProperty("/config")))
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

    Logger::getInstance()->log("Loading config file " + cfgFile, lsInfo);
    getPropertySystem().loadFromXML(cfgFile, getPropertySystem().getProperty("/config"));

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

#ifndef WIN32
  void* DSS::handleSignal(void* arg) {
    sigset_t signal_set;
    int sig;

    pthread_detach(pthread_self());

    /* wait for any and all signals */
    sigfillset(&signal_set);
    sigdelset(&signal_set, SIGUSR2); /* tcmalloc dump profile */

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
          Logger::getInstance()->log("System signal unhandled: " +
              intToString(sig), lsDebug);
          break;
        }
      }
      if (DSS::hasInstance()) {
        boost::shared_ptr<Event> pEvent(new Event("SIGNAL"));
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
    std::string dsid;
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
            dsid_t d;
            DsmApiGetEthernetDSID(mac.c_str(), &d);
            dss_dsid_t D;
            dsid_helper::toDssDsid(d, D);
            dsid = D.toString();
        }
    }

    PropertyNodePtr dsidNode = getPropertySystem().createProperty("/system/dSID");
    dsidNode->setStringValue(dsid);
  }
}
