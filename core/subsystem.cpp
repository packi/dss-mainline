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
      if(getDSS().getState() < ssCreatingSubsystems) {
        throw std::runtime_error("creating subsystem way too early");
      }
      getDSS().getPropertySystem().setIntValue(getConfigPropertyBasePath() + "loglevel", 0, true);
      getDSS().getPropertySystem().createProperty(getConfigPropertyBasePath() + "enabled")
         ->linkToProxy(PropertyProxyPointer<bool>(&m_Enabled));
    }
  } // ctor

  Subsystem::~Subsystem() {
  } // dtor

  void Subsystem::initialize() {
    aLogSeverity severity = lsDebug;
    if(m_pDSS != NULL) {
      if(getDSS().getState() < ssInitializingSubsystems) {
        throw std::runtime_error("you shouldn't initialize your subsystem at this phase");
      }
      severity = static_cast<aLogSeverity>(getDSS().getPropertySystem().getIntValue(getConfigPropertyBasePath() + "loglevel"));
    }
    m_pLogChannel.reset(new LogChannel(m_Name, severity));
    if(m_pDSS != NULL) {
      getDSS().getPropertySystem().getProperty(getConfigPropertyBasePath() + "loglevel")
        ->linkToProxy(PropertyProxyMemberFunction<Subsystem, int>(*this, &Subsystem::getLogSeverity, &Subsystem::setLogSeverity));
    }
    m_State = ssInitialized;
  } // initialize

  void Subsystem::start() {
    if(m_State != ssInitialized) {
      throw std::runtime_error("Subsystem::start: Subsystem '" + m_Name + "' was not initialized.");
    }
    if(m_Enabled) {
      doStart();
    }
  } // start

  std::string Subsystem::getConfigPropertyBasePath() {
    return "/config/subsystems/" + m_Name + "/";
  } // getConfigPropertyBasePath

  std::string Subsystem::getPropertyBasePath() {
    return "/system/" + m_Name + "/";
  } // getPropertyBasePath

  void Subsystem::log(const std::string& _message, aLogSeverity _severity) {
    Logger::getInstance()->log(*m_pLogChannel, _message, _severity);
  }

  int Subsystem::getLogSeverity() const {
    if(m_pLogChannel == NULL) {
      return 0;
    } else {
      return m_pLogChannel->getMinimumSeverity();
    }
  }

  void Subsystem::setLogSeverity(int _value) {
    if(m_pLogChannel != NULL) {
      m_pLogChannel->setMinimumSeverity(static_cast<aLogSeverity>(_value));
    }
  }

} // namespace dss
