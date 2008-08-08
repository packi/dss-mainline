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
#include "ds485proxy.h"
#include "dssim.h"
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

  class EventRunner : public Thread {
  private:
    boost::ptr_vector<ScheduledEvent> m_ScheduledEvents;

    DateTime GetNextOccurence();
    DateTime m_WakeTime;
    SyncEvent m_NewItem;
  public:
    EventRunner();
    virtual ~EventRunner() {};

    void AddEvent(ScheduledEvent* _scheduledEvent);

    void RaisePendingEvents(DateTime& _from, int _deltaSeconds);

    int GetSize() const;
    const ScheduledEvent& GetEvent(const int _idx) const;
    void RemoveEvent(const int _idx);

    virtual void Execute();
  }; // EventRunner


  /** Main class
    *
    */
  class DSS {
  private:
    static DSS* m_Instance;
    WebServer m_WebServer;
    Config m_Config;
    DS485Proxy m_DS485Proxy;
    Apartment m_Apartment;
    DSModulatorSim m_ModulatorSim;
    EventRunner m_EventRunner;
    WebServices m_WebServices;

    DSS() { };

    void LoadConfig();
  public:
    void Run();

    static DSS* GetInstance();
    Config& GetConfig() { return m_Config; };
    DS485Proxy& GetDS485Proxy() { return m_DS485Proxy; };
    Apartment& GetApartment() { return m_Apartment; };
    DSModulatorSim& GetModulatorSim() { return m_ModulatorSim; };
    EventRunner& GetEventRunner() { return m_EventRunner; };
    WebServices& GetWebServices() { return m_WebServices; };
  }; // DSS

}

#endif // DSS_H_INLUDED

