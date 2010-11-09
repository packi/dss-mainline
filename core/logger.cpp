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

#include "logger.h"
#include "base.h"
#include "foreach.h"
#include "datetools.h"

#include <cassert>
#include <iostream>

#if defined(__CYGWIN__)
#include <time.h>
#endif

namespace dss {

  Logger* Logger::m_Instance = NULL;
  Mutex Logger::m_handlerListMutex = Mutex();
  std::list<LogHandler *> Logger::m_handlerList = std::list<LogHandler *>();

  template <class t>
  t SeverityToString(const aLogSeverity _severity);

  template <>
  const char* SeverityToString(const aLogSeverity _severity) {
    switch(_severity) {
      case lsDebug:
        return "[Debug]";
      case lsInfo:
        return "[Info]";
      case lsWarning:
        return "[Warning]";
      case lsError:
        return "[Error]";
      case lsFatal:
        return "[Fatal]";
      default:
        return "[unknown severity]";
    }
  } // severityToString<const char*>

  template <>
  const std::string SeverityToString(const aLogSeverity _severity) {
    return std::string(SeverityToString<const char*>(_severity));
  } // severityToString<const string>

  template <>
  const wchar_t* SeverityToString(const aLogSeverity _severity) {
    switch(_severity) {
      case lsDebug:
        return L"[Debug]";
      case lsInfo:
        return L"[Info]";
      case lsWarning:
        return L"[Warning]";
      case lsError:
        return L"[Error]";
      case lsFatal:
        return L"[Fatal]";
      default:
        return L"[unknown severity]";
    }
  } // severityToString<const wchar_t*>

  template <>
  const std::wstring SeverityToString(const aLogSeverity _severity) {
    return std::wstring(SeverityToString<const wchar_t*>(_severity));
  } // severityToString<const wstring>

  Logger::Logger()
    : m_logTarget(boost::shared_ptr<LogTarget>(new CoutLogTarget())),
      m_defaultLogChannel(new LogChannel("System"))
  {}

  Logger* Logger::getInstance() {
    if(m_Instance == NULL) {
      m_Instance = new Logger();
    }
    assert(m_Instance != NULL);
    return m_Instance;
  } // getInstance

  void Logger::shutdown() {
    if(m_Instance != NULL) {
      Logger* inst = m_Instance;
      m_Instance = NULL;
      delete inst;
    }
  }

  void Logger::log(const std::string& _message, const aLogSeverity _severity) {
    log(*m_defaultLogChannel, std::string(_message), _severity);
  } // log

  void Logger::log(const LogChannel& _channel, const std::string& _message, const aLogSeverity _severity) {
    if(_channel.maylog(_severity)) {
      std::string logMessage;

      DateTime d = DateTime();

      logMessage = "[" + d.fromUTC().toString() + "]"
        + SeverityToString<const std::string>(_severity)
        + "[" + _channel.getName() + "]"
        + " " + _message + "\n";

      m_logTarget->outputStream() << logMessage; // only for backward compatibility

      m_handlerListMutex.lock();
      foreach(LogHandler *h,m_handlerList) {
        h->handle(logMessage);
      }
      m_handlerListMutex.unlock();
    }
  } // log


  bool Logger::setLogTarget(boost::shared_ptr<LogTarget>& _logTarget) {
    if (_logTarget->open()) {
      m_logTarget->close();
      m_logTarget = _logTarget;
      return true;
    }
    return false;
  } // setLogTarget

  bool Logger::reopenLogTarget() {
    m_logTarget->close();
    return m_logTarget->open();
  }


  void Logger::registerHandler(LogHandler& _logHandler) {
	  m_handlerListMutex.lock();
	  m_handlerList.push_back(&_logHandler);
	  m_handlerListMutex.unlock();
  } //registerHandler

  void Logger::deregisterHandler(LogHandler& _logHandler) {
	  m_handlerListMutex.lock();
	  std::list<LogHandler *>::iterator it = find(m_handlerList.begin(), m_handlerList.end(), &_logHandler);
	  if (it != m_handlerList.end()) {
		  m_handlerList.erase(it);
	  }
	  m_handlerListMutex.unlock();
  } // deregisterHandler
}
