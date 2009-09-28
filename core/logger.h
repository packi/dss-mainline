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

#ifndef LOGGER_H_INLUDED
#define LOGGER_H_INLUDED

#include <string>
#include <ostream>
#include <fstream>
#include <iostream>
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
class LogTarget;

  class Logger {
  public:
    static Logger* getInstance();
    static void shutdown();

    void log(const std::string& _message, const aLogSeverity _severity = lsDebug);
    void log(const char* _message, const aLogSeverity _severity = lsDebug);

    void log(const LogChannel& _channel, const std::string& _message, const aLogSeverity _severity = lsDebug);
    
    bool setLogTarget(boost::shared_ptr<LogTarget>& _logTarget);
    
  private:
    Logger();
    
    static Logger* m_Instance;
    
    boost::shared_ptr<LogTarget> m_logTarget;

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


class LogTarget {
public:
  virtual bool open() = 0;
  virtual void close() = 0;  
  virtual std::ostream& outputStream() = 0;
}; // LogTarget

class CoutLogTarget : public LogTarget {
public:
  virtual bool open() { return true; }
  virtual void close() {}
  std::ostream& outputStream() { return std::cout; }
}; // CoutLogTarget

class FileLogTarget : public LogTarget {
public:
  FileLogTarget(const std::string& _fileName) : m_fileName(_fileName) {}
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

}

#endif
