/*
 *  dss.h
 *  dSS
 *
 *  Created by Patrick Stählin on 4/2/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef DSS_H_INCLUDED
#define DSS_H_INCLUDED

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>

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
#ifdef USE_SIM
  class DSModulatorSim;
#endif

  typedef enum {
    ssInvalid,
    ssCreatingSubsystems,
    ssLoadingConfig,
    ssInitializingSubsystems,
    ssStarting,
    ssRunning
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
#ifdef USE_SIM
    boost::shared_ptr<DSModulatorSim> m_pModulatorSim;
#endif
    boost::shared_ptr<EventRunner> m_pEventRunner;
    boost::shared_ptr<WebServices> m_pWebServices;
    boost::shared_ptr<EventInterpreter> m_pEventInterpreter;
    boost::shared_ptr<EventQueue> m_pEventQueue;
    boost::shared_ptr<Metering> m_pMetering;
    boost::shared_ptr<FakeMeter> m_pFakeMeter;

    aDSSState m_State;

    /// Private constructor for singleton
    DSS();

    void LoadConfig();
    void AddDefaultInterpreterPlugins();

    int GetUptime() const;
  public:
    void Initialize();
    void Run();

    static DSS* GetInstance();
#ifdef WITH_TESTS
    static void Teardown();
#endif

    aDSSState GetState() const { return m_State; }

    DS485Interface& GetDS485Interface() { return *m_pDS485Interface; }
    Apartment& GetApartment() { return *m_pApartment; }
#ifdef USE_SIM
    DSModulatorSim& GetModulatorSim() { return *m_pModulatorSim; }
#endif
    EventRunner& GetEventRunner() { return *m_pEventRunner; }
    WebServices& GetWebServices() { return *m_pWebServices; }
    EventQueue& GetEventQueue() { return *m_pEventQueue; }
    Metering& GetMetering() { return *m_pMetering; }
    PropertySystem& GetPropertySystem() { return *m_pPropertySystem; }
    WebServer& GetWebServer() { return *m_pWebServer; }
  }; // DSS

}

#endif // DSS_H_INLUDED

