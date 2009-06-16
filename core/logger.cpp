/*
 *  logger.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/2/08.
 *  Copyright:
 *  (c) 2008 by
 *  futureLAB AG
 *  Schwalmenackerstrasse 4
 *  CH-8400 Winterthur / Schweiz
 *  Alle Rechte vorbehalten.
 *  Jede Art der Vervielfaeltigung, Verbreitung,
 *  Auswertung oder Veraenderung - auch auszugsweise -
 *  ist ohne vorgaengige schriftliche Genehmigung durch
 *  die futureLAB AG untersagt.
 *
 * Last change $Date$
 * by $Author$
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
