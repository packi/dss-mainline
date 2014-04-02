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

#ifndef DSS_H_INCLUDED
#define DSS_H_INCLUDED

#include <vector>

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "dssfwd.h"

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include "bonjour.h"
#include "logger.h"

#include "comm-channel.h"

namespace dss {

  class DSS;
  class Subsystem;
  class WebServer;
  class BusInterface;
  class PropertySystem;
  class Metering;
  class WebServices;
  class ModelMaintenance;
  class SessionManager;
  class Security;
  class Watchdog;
  class BusEventSink;

  typedef enum {
    ssInvalid,
    ssCreatingSubsystems,
    ssLoadingConfig,
    ssInitializingSubsystems,
    ssStarting,
    ssRunning,
    ssTerminating
  } aDSSState;

  /** Main class
    *
    */
  class DSS {
    __DECL_LOG_CHANNEL__
  private:
    static DSS* m_Instance;
    std::vector<Subsystem*> m_Subsystems;
    time_t m_TimeStarted;
    boost::shared_ptr<WebServer> m_pWebServer;
    boost::shared_ptr<BusInterface> m_pBusInterface;
    boost::shared_ptr<PropertySystem> m_pPropertySystem;
    boost::shared_ptr<Apartment> m_pApartment;
    boost::shared_ptr<EventRunner> m_pEventRunner;
    boost::shared_ptr<WebServices> m_pWebServices;
    boost::shared_ptr<EventInterpreter> m_pEventInterpreter;
    boost::shared_ptr<EventQueue> m_pEventQueue;
    boost::shared_ptr<Metering> m_pMetering;
    boost::shared_ptr<ModelMaintenance> m_pModelMaintenance;
    boost::shared_ptr<SessionManager> m_pSessionManager;
    boost::shared_ptr<Security> m_pSecurity;
    boost::shared_ptr<BonjourHandler> m_pBonjour;
    boost::shared_ptr<Watchdog> m_pWatchdog;
    boost::shared_ptr<BusEventSink> m_pDefaultBusEventSink;
    std::string m_dataDirectory;
    std::string m_configDirectory;
    std::string m_webrootDirectory;
    std::string m_jsLogDirectory;
    std::string m_savedPropsDirectory;
    CommChannel *m_commChannel;

    aDSSState m_State;
    bool m_ShutdownFlag;

    /// Private constructor for singleton
    DSS();

    bool loadConfig(const std::string& _configFile);
    int loadConfigDir(const std::string& _configDir);
    bool parseProperties(const std::vector<std::string>& _properties);
    void addDefaultInterpreterPlugins();

    int getUptime() const;
    bool isSaneDirectory(const std::string& _path);
    bool checkDirectoriesExist();
    void setupDirectories();
    bool initSubsystems();
    bool initSecurity();
    std::string readDistroVersion();
    void publishDSID();
  public:
    ~DSS();
    bool initialize(const std::vector<std::string>& _properties, const std::string& _configFile);
    void run();

    static DSS* getInstance();
    static bool hasInstance();
    static void shutdown();
    void initiateShutdown();
#ifdef WITH_TESTS
    static void teardown();
#endif
    static std::string versionString();
    static std::vector<unsigned char> getRandomSalt(unsigned int len);

#ifndef WIN32
    static void* handleSignal(void* arg);
#endif

    aDSSState getState() const { return m_State; }

    BusInterface& getBusInterface() { return *m_pBusInterface; }
    Apartment& getApartment() { return *m_pApartment; }
    EventRunner& getEventRunner() { return *m_pEventRunner; }
    WebServices& getWebServices() { return *m_pWebServices; }
    EventQueue& getEventQueue() { return *m_pEventQueue; }
    Metering& getMetering() { return *m_pMetering; }
    PropertySystem& getPropertySystem() { return *m_pPropertySystem; }
    WebServer& getWebServer() { return *m_pWebServer; }
    EventInterpreter& getEventInterpreter() { return *m_pEventInterpreter; }
    ModelMaintenance& getModelMaintenance() { return *m_pModelMaintenance; }
    SessionManager& getSessionManager() { return *m_pSessionManager; }
    Security& getSecurity() { return *m_pSecurity; }
    BonjourHandler& getBonjourHandler() { return *m_pBonjour; }

    const std::string& getDataDirectory() const { return m_dataDirectory; }
    const std::string& getConfigDirectory() const { return m_configDirectory; }
    const std::string& getWebrootDirectory() const { return m_webrootDirectory; }
    const std::string& getJSLogDirectory() const { return m_jsLogDirectory; }
    const std::string& getSavedPropsDirectory() const { return  m_savedPropsDirectory; }
    void setDataDirectory(const std::string& _value);
    void setConfigDirectory(const std::string& _value);
    void setWebrootDirectory(const std::string& _value);
    void setJSLogDirectory(const std::string& _value);
    void setSavedPropsDirectory(const std::string& _value);
  }; // DSS

}

#endif // DSS_H_INCLUDED

