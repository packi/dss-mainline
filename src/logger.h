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

#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#include <string>
#include <ostream>
#include <fstream>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <list>
#include "mutex.h"

/*
 * Usage:
 * class FooBar {
 *  __DECL_LOG_CHANNEL__
 *
 * foobar.cpp
 * __DEFINE_LOG_CHANNEL__(yourClassName, logLevel)
 *
 * Adds function 'log' with personalized 'FooBar' channel
 * to your class
 */
#define __DEFINE_LOG_CHANNEL__(_NAME_ , level) LogChannel _NAME_::s_logChannel(#_NAME_, level);
#define __DECL_LOG_CHANNEL__ static LogChannel s_logChannel; \
  static void log(const std::string& message, const aLogSeverity severity) { \
    Logger::getInstance()->log(s_logChannel, message, severity); \
  }

typedef enum {
  lsDebug = 0,
  lsInfo = 1,
  lsWarning = 2,
  lsError = 3,
  lsFatal = 4
} aLogSeverity;

namespace dss {

class LogChannel;
class LogTarget;
class LogHandler;

  class Logger {
  public:
    static Logger* getInstance();
    static void shutdown();

    void log(const std::string& _message, const aLogSeverity _severity = lsDebug);
    void log(const LogChannel& _channel, const std::string& _message, const aLogSeverity _severity = lsDebug);
    
    bool setLogTarget(boost::shared_ptr<LogTarget>& _logTarget);
    bool reopenLogTarget();
    
    void registerHandler(LogHandler& _logHandler);
    void deregisterHandler(LogHandler& _logHandler);

    boost::shared_ptr<LogChannel> getLogChannel() { return m_defaultLogChannel; }

  private:
    Logger();
    
    static Logger* m_Instance;
    
    boost::shared_ptr<LogTarget> m_logTarget;
    boost::shared_ptr<LogChannel> m_defaultLogChannel;

    static Mutex m_handlerListMutex;
    static std::list<LogHandler *> m_handlerList;
  }; // Logger

  class LogChannel {
  private:
    const std::string m_Name;
    aLogSeverity m_MinimumSeverity;
  public:
    LogChannel(const std::string& _name, aLogSeverity _minimumSeverity = lsDebug)
    : m_Name(_name), m_MinimumSeverity(_minimumSeverity)
    {}

    aLogSeverity getMinimumSeverity() const { return m_MinimumSeverity; }
    void setMinimumSeverity(aLogSeverity _value) { m_MinimumSeverity = _value; }
    const std::string& getName() const { return m_Name; }
    bool maylog(aLogSeverity _severity) const { return _severity >= m_MinimumSeverity; }
  }; // LogChannel


class LogTarget {
public:
  virtual bool open() = 0;
  virtual void close() = 0;  
  virtual std::ostream& outputStream() = 0;

  virtual ~LogTarget() {} // make g++ happy
}; // LogTarget

class CoutLogTarget : public LogTarget {
public:
  virtual bool open() { return true; }
  virtual void close() {}
  std::ostream& outputStream() { return std::cout; }
}; // CoutLogTarget

class FileLogTarget : public LogTarget {
public:
  FileLogTarget(const std::string& _fileName) : m_fileName(_fileName), m_fileOpen(false) {}
  virtual bool open() { 
    m_fileOutputStream.open(m_fileName.c_str(), std::ios::out|std::ios::app); 
    m_fileOpen = true;
    m_fileOutputStream << "-- Log opened" << std::endl;
    return m_fileOutputStream.good(); 
  }
  virtual void close() { 
    if (m_fileOpen) { 
      m_fileOutputStream << "-- Log closed" << std::endl;
      m_fileOutputStream. close(); 
      m_fileOpen = false; 
    } 
  }
  std::ostream& outputStream() { return m_fileOutputStream; }
  
private:
  std::string m_fileName;
  bool m_fileOpen;
  std::ofstream m_fileOutputStream;
}; // FileLogTarget

class LogHandler {
public:
  virtual void handle(const std::string &_message) = 0;

  virtual ~LogHandler() {};
}; // LogHandler

class CoutLogHandler : public LogHandler {
public:
  void handle(const std::string &_message) {
    std::cout << _message;
  }

  ~CoutLogHandler() {}
}; // CoutLogHandler

class FileLogHandler : public LogHandler {
public:
  FileLogHandler(const std::string& _fileName) : m_fileName(_fileName) {};
  ~FileLogHandler() {};

  void handle(const std::string &_message) {
    m_fileOutputStream << _message;
  }

  virtual bool open() {
    m_fileOutputStream.open(m_fileName.c_str(), std::ios::out|std::ios::app);
    m_fileOpen = true;
    m_fileOutputStream << "-- File opened" << std::endl;
    return m_fileOutputStream.good();
  }

  virtual void close() {
    if(m_fileOpen) {
      m_fileOutputStream << "-- File closed" << std::endl;
      m_fileOutputStream.close();
      m_fileOpen = false;
    }
  }

private:
  std::string m_fileName;
  bool m_fileOpen;
  std::ofstream m_fileOutputStream;
}; // FileLogHandler

}

#endif
