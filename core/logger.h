/*
 *  logger.h
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

#ifndef LOGGER_H_INLUDED
#define LOGGER_H_INLUDED

#include <string>

using namespace std;

typedef enum {
  lsDebug,
  lsInfo,
  lsWarning,
  lsError,
  lsFatal
} aLogSeverity;

namespace dss {

  class Logger {
  private:
    static Logger* m_Instance;

    Logger() {}
  public:
    static Logger* GetInstance();

    void Log(const string& _message, const aLogSeverity _severity = lsDebug);
    void Log(const char* _message, const aLogSeverity _severity = lsDebug);
    void Log(const wchar_t* _message, const aLogSeverity _severity = lsDebug);
    void Log(const wstring& _message, const aLogSeverity _severity = lsDebug);
  }; // Logger
}

#endif
