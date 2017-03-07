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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "logger.h"
#include "base.h"
#include "foreach.h"
#include "datetools.h"

#include <iostream>
#include <boost/make_shared.hpp>
#include <boost/thread/mutex.hpp>

namespace dss {

  Logger* Logger::m_Instance = NULL;
  boost::mutex Logger::m_handlerListMutex;
  boost::mutex Logger::m_streamMutex;
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
      case lsNotice:
        return "[Notice]";
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
    : m_logTarget(boost::make_shared<CoutLogTarget>()),
      m_defaultLogChannel(new LogChannel("System"))
  {}

  Logger* Logger::getInstance() {
    if(m_Instance == NULL) {
      m_Instance = new Logger();
    }
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

      logMessage = "[" + d.toISO8601_ms_local() + "]"
        + SeverityToString<const std::string>(_severity)
        + "[" + _channel.getName() + "]"
        + " " + _message + "\n";

      {
        boost::mutex::scoped_lock lock(m_streamMutex);
        m_logTarget->outputStream() << logMessage; // only for backward compatibility
      }

      {
        boost::mutex::scoped_lock lock(m_handlerListMutex);
        foreach(LogHandler *h,m_handlerList) {
          h->handle(logMessage);
        }
      }
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
    boost::mutex::scoped_lock lock(m_handlerListMutex);
    m_handlerList.push_back(&_logHandler);
  } //registerHandler

  void Logger::deregisterHandler(LogHandler& _logHandler) {
    boost::mutex::scoped_lock lock(m_handlerListMutex);
    std::list<LogHandler *>::iterator it = find(m_handlerList.begin(), m_handlerList.end(), &_logHandler);
    if (it != m_handlerList.end()) {
      m_handlerList.erase(it);
    }
  } // deregisterHandler
}
