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
#ifdef __GNUC__
  #include "dssim.h"
#endif
#include "syncevent.h"
#include "webserver.h"
#include "../webservices/webservices.h"
#include "model.h"

#include <cstdio>
#include <string>
#include <sstream>

using namespace std;

#include <boost/ptr_container/ptr_vector.hpp>

namespace dss {

  class DSS;
  class WebServer;
  class Config;
  class EventRunner;
  class DS485Interface;

  class EventRunner {
  private:
    boost::ptr_vector<ScheduledEvent> m_ScheduledEvents;

    DateTime GetNextOccurence();
    DateTime m_WakeTime;
    SyncEvent m_NewItem;
  public:
    void AddEvent(ScheduledEvent* _scheduledEvent);

    void RaisePendingEvents(DateTime& _from, int _deltaSeconds);

    int GetSize() const;
    const ScheduledEvent& GetEvent(const int _idx) const;
    void RemoveEvent(const int _idx);

    void Run();
  }; // EventRunner


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
#ifdef __GNUC__
    DSModulatorSim m_ModulatorSim;
#endif
    EventRunner m_EventRunner;
    WebServices m_WebServices;

    DSS() { };

    void LoadConfig();
  public:
    void Run();

    static DSS* GetInstance();
    Config& GetConfig() { return m_Config; };
    DS485Interface& GetDS485Interface() { return *m_DS485Interface; };
    Apartment& GetApartment() { return m_Apartment; };
#ifdef __GNUC__
    DSModulatorSim& GetModulatorSim() { return m_ModulatorSim; };
#endif
    EventRunner& GetEventRunner() { return m_EventRunner; };
    WebServices& GetWebServices() { return m_WebServices; };
  }; // DSS

}

#endif // DSS_H_INLUDED

