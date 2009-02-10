/*
 * subsystem.cpp
 *
 *  Created on: Feb 10, 2009
 *      Author: patrick
 */

#include "subsystem.h"
#include "dss.h"
#include "propertysystem.h"

namespace dss {

  //================================================== Subsystem

  Subsystem::Subsystem(DSS* _pDSS, const std::string& _name)
  : m_State(ssCreated), m_pDSS(_pDSS), m_Name(_name)
  {
    if(m_pDSS != NULL) {
      if(GetDSS().GetState() < ssCreatingSubsystems) {
        throw runtime_error("creating subsystem way too early");
      }
      GetDSS().GetPropertySystem().SetIntValue(GetPropertyBasePath() + "loglevel", 0, true);
    }
  } // ctor

  void Subsystem::Initialize() {
    aLogSeverity severity = lsDebug;
    if(m_pDSS != NULL) {
      if(GetDSS().GetState() < ssInitializingSubsystems) {
        throw runtime_error("you shouldn't initialize your subsystem at this phase");
      }
      severity = static_cast<aLogSeverity>(GetDSS().GetPropertySystem().GetIntValue(GetPropertyBasePath() + "loglevel"));
    }
    m_pLogChannel.reset(new LogChannel(m_Name, severity));
    m_State = ssInitialized;
  } // Initialize

  void Subsystem::Start() {
    if(m_State != ssInitialized) {
      throw new runtime_error(string("Subsystem::Start: Subsystem '") + m_Name + "' was not initialized.");
    }
  } // Start

  std::string Subsystem::GetPropertyBasePath() {
    return "/config/subsystems/" + m_Name + "/";
  } // GetPropertyBasePath

  void Subsystem::Log(const std::string& _message, aLogSeverity _severity) {
    Logger::GetInstance()->Log(*m_pLogChannel, _message, _severity);
  }
} // namespace dss
