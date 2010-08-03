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

#ifndef DSS_H_INCLUDED
#define DSS_H_INCLUDED

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>

#include <vector>

namespace dss {

  class DSS;
  class Subsystem;
  class WebServer;
  class DS485Interface;
  class PropertySystem;
  class Metering;
  class FakeMeter;
  class EventRunner;
  class EventQueue;
  class EventInterpreter;
  class Apartment;
  class WebServices;
  class DS485BusRequestDispatcher;
  class BusInterfaceHandler;
  class ModelMaintenance;
  class DSSim;

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
  private:
    static DSS* m_Instance;
    std::vector<Subsystem*> m_Subsystems;
    time_t m_TimeStarted;
    boost::shared_ptr<WebServer> m_pWebServer;
    boost::shared_ptr<DS485Interface> m_pDS485Interface;
    boost::shared_ptr<PropertySystem> m_pPropertySystem;
    boost::shared_ptr<Apartment> m_pApartment;
    boost::shared_ptr<DSSim> m_pSimulation;
    boost::shared_ptr<EventRunner> m_pEventRunner;
    boost::shared_ptr<WebServices> m_pWebServices;
    boost::shared_ptr<EventInterpreter> m_pEventInterpreter;
    boost::shared_ptr<EventQueue> m_pEventQueue;
    boost::shared_ptr<Metering> m_pMetering;
    boost::shared_ptr<FakeMeter> m_pFakeMeter;
    boost::shared_ptr<DS485BusRequestDispatcher> m_pBusDispatcher;
    boost::shared_ptr<BusInterfaceHandler> m_pBusInterfaceHandler;
    boost::shared_ptr<ModelMaintenance> m_pModelMaintenance;
    std::string m_dataDirectory;
    std::string m_configDirectory;
    std::string m_webrootDirectory;
    std::string m_jsLogDirectory;

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

#ifndef WIN32
    static void handleSignal(int signum);
#endif

    aDSSState getState() const { return m_State; }

    DS485Interface& getDS485Interface() { return *m_pDS485Interface; }
    Apartment& getApartment() { return *m_pApartment; }
    DSSim& getSimulation() { return *m_pSimulation; }
    EventRunner& getEventRunner() { return *m_pEventRunner; }
    WebServices& getWebServices() { return *m_pWebServices; }
    EventQueue& getEventQueue() { return *m_pEventQueue; }
    Metering& getMetering() { return *m_pMetering; }
    PropertySystem& getPropertySystem() { return *m_pPropertySystem; }
    WebServer& getWebServer() { return *m_pWebServer; }
    EventInterpreter& getEventInterpreter() { return *m_pEventInterpreter; }
    BusInterfaceHandler& getBusInterfaceHandler() { return *m_pBusInterfaceHandler; }
    ModelMaintenance& getModelMaintenance() { return *m_pModelMaintenance; }

    const std::string& getDataDirectory() const { return m_dataDirectory; }
    const std::string& getConfigDirectory() const { return m_configDirectory; }
    const std::string& getWebrootDirectory() const { return m_webrootDirectory; }
    const std::string& getJSLogDirectory() const { return m_jsLogDirectory; }
    void setDataDirectory(const std::string& _value);
    void setConfigDirectory(const std::string& _value);
    void setWebrootDirectory(const std::string& _value);
    void setJSLogDirectory(const std::string& _value);
  }; // DSS

}

#endif // DSS_H_INCLUDED

