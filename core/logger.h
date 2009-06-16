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
#include <boost/shared_ptr.hpp>

typedef enum {
  lsDebug = 0,
  lsInfo = 1,
  lsWarning = 2,
  lsError = 3,
  lsFatal = 4
} aLogSeverity;

namespace dss {

  class LogChannel;

  class Logger {
  private:
    static Logger* m_Instance;

    Logger() {}
  public:
    static Logger* getInstance();
    static void shutdown();

    void log(const std::string& _message, const aLogSeverity _severity = lsDebug);
    void log(const char* _message, const aLogSeverity _severity = lsDebug);

    void log(const LogChannel& _channel, const std::string& _message, const aLogSeverity _severity = lsDebug);
  }; // Logger

  class LogChannel {
  private:
    const std::string m_Name;
    aLogSeverity m_MinimumSeverity;
  public:
    LogChannel(const std::string& _name, aLogSeverity _minimumSeverity = lsDebug)
    : m_Name(_name), m_MinimumSeverity(_minimumSeverity)
    {
      Logger::getInstance()->log(*this, "Logchannel created");
    }

    ~LogChannel() {
      Logger::getInstance()->log(*this, "Logchannel destroyed");
    }

    aLogSeverity getMinimumSeverity() const { return m_MinimumSeverity; }
    void setMinimumSeverity(aLogSeverity _value) { m_MinimumSeverity = _value; }
    const std::string& getName() const { return m_Name; }
    bool maylog(aLogSeverity _severity) const { return _severity >= m_MinimumSeverity; }
  }; // LogChannel


}

#endif
