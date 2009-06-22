/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland

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

#include <cassert>
#include <iostream>

using namespace std;

namespace dss {

  Logger* Logger::m_Instance = NULL;

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
  const string SeverityToString(const aLogSeverity _severity) {
    return string(SeverityToString<const char*>(_severity));
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
  const wstring SeverityToString(const aLogSeverity _severity) {
    return wstring(SeverityToString<const wchar_t*>(_severity));
  } // severityToString<const wstring>


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

  void Logger::log(const string& _message, const aLogSeverity _severity) {
    time_t now = time( NULL );
    struct tm t;
#ifdef WIN32
    localtime_s( &t, &now );
#else
    localtime_r( &now, &t );
#endif
    cout << "[" << dateToISOString<string>(&t) << "]" << SeverityToString<const string>(_severity) << " " << _message << endl;
  } // log

  void Logger::log(const char* _message, const aLogSeverity _severity) {
    log(string(_message), _severity);
  } // log

  void Logger::log(const LogChannel& _channel, const std::string& _message, const aLogSeverity _severity) {
    if(_channel.maylog(_severity)) {
      log(string("[") + _channel.getName() + "] " + _message, _severity);
    }
  } // log

}
