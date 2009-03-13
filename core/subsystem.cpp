/*
 * subsystem.cpp
 *
 *  Created on: Feb 10, 2009
 *      Author: patrick
 */

#include "subsystem.h"
#include "dss.h"
#include "propertysystem.h"

#include <stdexcept>

namespace dss {

  //================================================== Subsystem

  Subsystem::Subsystem(DSS* _pDSS, const std::string& _name)
  : m_State(ssCreated),
    m_pDSS(_pDSS),
    m_Name(_name),
    m_Enabled(true)
  {
    if(m_pDSS != NULL) {
      if(GetDSS().GetState() < ssCreatingSubsystems) {
        throw std::runtime_error("creating subsystem way too early");
      }
      GetDSS().GetPropertySystem().SetIntValue(GetConfigPropertyBasePath() + "loglevel", 0, true);
      GetDSS().GetPropertySystem().CreateProperty(GetConfigPropertyBasePath() + "enabled")
         ->LinkToProxy(PropertyProxyPointer<bool>(&m_Enabled));
    }
  } // ctor

  Subsystem::~Subsystem() {
  } // dtor

  void Subsystem::Initialize() {
    aLogSeverity severity = lsDebug;
    if(m_pDSS != NULL) {
      if(GetDSS().GetState() < ssInitializingSubsystems) {
        throw std::runtime_error("you shouldn't initialize your subsystem at this phase");
      }
      severity = static_cast<aLogSeverity>(GetDSS().GetPropertySystem().GetIntValue(GetConfigPropertyBasePath() + "loglevel"));
    }
    m_pLogChannel.reset(new LogChannel(m_Name, severity));
    GetDSS().GetPropertySystem().GetProperty(GetConfigPropertyBasePath() + "loglevel")
      ->LinkToProxy(PropertyProxyMemberFunction<Subsystem, int>(*this, &Subsystem::GetLogSeverity, &Subsystem::SetLogSeverity));
    m_State = ssInitialized;
  } // Initialize

  void Subsystem::Start() {
    if(m_State != ssInitialized) {
      throw new std::runtime_error("Subsystem::Start: Subsystem '" + m_Name + "' was not initialized.");
    }
    if(m_Enabled) {
      DoStart();
    }
  } // Start

  std::string Subsystem::GetConfigPropertyBasePath() {
    return "/config/subsystems/" + m_Name + "/";
  } // GetConfigPropertyBasePath

  std::string Subsystem::GetPropertyBasePath() {
    return "/system/" + m_Name + "/";
  } // GetPropertyBasePath

  void Subsystem::Log(const std::string& _message, aLogSeverity _severity) {
    Logger::GetInstance()->Log(*m_pLogChannel, _message, _severity);
  }

  int Subsystem::GetLogSeverity() const {
    if(m_pLogChannel.get() == NULL) {
      return 0;
    } else {
      return m_pLogChannel->GetMinimumSeverity();
    }
  }

  void Subsystem::SetLogSeverity(int _value) {
    if(m_pLogChannel.get() != NULL) {
      m_pLogChannel->SetMinimumSeverity(static_cast<aLogSeverity>(_value));
    }
  }

} // namespace dss
