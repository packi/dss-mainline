/*
 *  dss.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/2/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "dss.h"
#include "logger.h"
#include "xmlwrapper.h"
#include "scripting/modeljs.h"
#ifdef __GNUC__
#include "../unix/ds485proxy.h"
#endif

#include <cassert>
#include <string>
#include <iostream>

using namespace std;

namespace dss {

  //============================================ EventRunner

  const bool DebugEventRunner = false;

  int EventRunner::GetSize() const {
    return m_ScheduledEvents.size();
  } // GetSize

  const ScheduledEvent& EventRunner::GetEvent(const int _idx) const {
    return m_ScheduledEvents.at(_idx);
  } // GetEvent

  void EventRunner::RemoveEvent(const int _idx) {
    boost::ptr_vector<ScheduledEvent>::iterator it = m_ScheduledEvents.begin();
    advance(it, _idx);
    m_ScheduledEvents.erase(it);
  } // RemoveEvent

  void EventRunner::AddEvent(ScheduledEvent* _scheduledEvent) {
    m_ScheduledEvents.push_back(_scheduledEvent);
    m_NewItem.Signal();
  } // AddEvent

  DateTime EventRunner::GetNextOccurence() {
    DateTime now;
    DateTime result = now.AddYear(10);
    if(DebugEventRunner) {
      cout << "*********" << endl;
    }
    for(boost::ptr_vector<ScheduledEvent>::iterator ipSchedEvt = m_ScheduledEvents.begin(), e = m_ScheduledEvents.end();
        ipSchedEvt != e; ++ipSchedEvt)
    {
      DateTime next = ipSchedEvt->GetSchedule().GetNextOccurence(now);
      if(DebugEventRunner) {
        cout << "next:   " << next << endl;
        cout << "result: " << result << endl;
      }
      result = min(result, next);
      if(DebugEventRunner) {
        cout << "chosen: " << result << endl;
      }
    }
    return result;
  }

  void EventRunner::Run() {
    while(true) {
      if(m_ScheduledEvents.empty()) {
        m_NewItem.WaitFor(1000);
      } else {
        DateTime now;
        m_WakeTime = GetNextOccurence();
        int sleepSeconds = m_WakeTime.Difference(now);

        // Prevent loops when a cycle takes less than 1s
        if(sleepSeconds == 0) {
          m_NewItem.WaitFor(1000);
          continue;
        }

        if(!m_NewItem.WaitFor(sleepSeconds * 1000)) {
          RaisePendingEvents(m_WakeTime, 10);
        }
      }
    }
  } // Execute


  void EventRunner::RaisePendingEvents(DateTime& _from, int _deltaSeconds) {
    DateTime virtualNow = _from.AddSeconds(-_deltaSeconds/2);
    if(DebugEventRunner) {
      cout << "vNow:    " << virtualNow << endl;
    }
    for(boost::ptr_vector<ScheduledEvent>::iterator ipSchedEvt = m_ScheduledEvents.begin(), e = m_ScheduledEvents.end();
        ipSchedEvt != e; ++ipSchedEvt)
    {
      DateTime nextOccurence = ipSchedEvt->GetSchedule().GetNextOccurence(virtualNow);
      if(DebugEventRunner) {
        cout << "nextOcc: " << nextOccurence << endl;
        cout << "diff:    " << nextOccurence.Difference(virtualNow) << endl;
      }
      if(abs(nextOccurence.Difference(virtualNow)) <= _deltaSeconds/2) {
        DSS::GetInstance()->GetApartment().OnEvent(ipSchedEvt->GetEvent());
      }
    }
  } // RaisePendingEvents

  //============================================= DSS

  DSS* DSS::m_Instance = NULL;

  DSS* DSS::GetInstance() {
    if(m_Instance == NULL) {
      m_Instance = new DSS();
    }
    assert(m_Instance != NULL);
    return m_Instance;
  } // GetInstance

  void DSS::Run() {
    Logger::GetInstance()->Log("DSS stating up....", lsInfo);
    LoadConfig();
    m_WebServer.Initialize(m_Config);
    m_WebServer.Run();

    m_WebServices.Run();

    m_ModulatorSim.Initialize();
#ifndef __PARADIGM__
    m_DS485Interface = new DS485Proxy();
    (static_cast<DS485Proxy*>(m_DS485Interface))->Start();
#endif
    m_Apartment.Run();

    // pass control to the eventrunner
    m_EventRunner.Run();
  } // Run

  void DSS::LoadConfig() {
    Logger::GetInstance()->Log("Loading config", lsInfo);
    m_Config.ReadFromXML("data/config.xml");
  } // LoadConfig

}
