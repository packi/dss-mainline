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

#include <bitset>

#include "base.h"
#include "thread.h"
#ifdef USE_SIM
  #include "sim/dssim.h"
#endif
#include "syncevent.h"
#include "webserver.h"
#include "../webservices/webservices.h"
#include "model.h"
#include "event.h"

#include <cstdio>
#include <string>
#include <sstream>

using namespace std;

#include <boost/ptr_container/ptr_vector.hpp>

namespace dss {

  class DSS;
  class WebServer;
  class Config;
  class DS485Interface;

  /** Main class
    *
    */
  class DSS {
  private:
    static DSS* m_Instance;
    WebServer m_WebServer;
    Config m_Config;
    DS485Interface* m_DS485Interface;
    Apartment m_Apartment;
#ifdef USE_SIM
    DSModulatorSim m_ModulatorSim;
#endif
    EventRunner m_EventRunner;
    WebServices m_WebServices;
    EventInterpreter m_EventInterpreter;
    EventQueue m_EventQueue;

    DSS();

    void LoadConfig();
  public:
    void Run();

    static DSS* GetInstance();
    Config& GetConfig() { return m_Config; }
    DS485Interface& GetDS485Interface() { return *m_DS485Interface; }
    Apartment& GetApartment() { return m_Apartment; }
#ifdef USE_SIM
    DSModulatorSim& GetModulatorSim() { return m_ModulatorSim; }
#endif
    EventRunner& GetEventRunner() { return m_EventRunner; }
    WebServices& GetWebServices() { return m_WebServices; }
    EventQueue& GetEventQueue() { return m_EventQueue; }
  }; // DSS

}

#endif // DSS_H_INLUDED

