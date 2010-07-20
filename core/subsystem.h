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

#ifndef SUBSYSTEM_H_
#define SUBSYSTEM_H_

#include "logger.h"
#include "thread.h"

#include <string>
#include <boost/shared_ptr.hpp>

namespace dss {

  class DSS;
  class LogChannel;

  class Subsystem {
  private:
    typedef enum {
      ssCreated,
      ssInitialized,
      ssStarted
    } aSubsystemState;
    aSubsystemState m_State;
    DSS* m_pDSS;
    std::string m_Name;
    bool m_Enabled;
    boost::shared_ptr<LogChannel> m_pLogChannel;
  protected:
    std::string getConfigPropertyBasePath();
    std::string getPropertyBasePath();

    int getLogSeverity() const;
    void setLogSeverity(int _value);

    virtual void doStart() = 0;
  public:
    Subsystem(DSS* _pDSS, const std::string& _name);
    virtual ~Subsystem();

    DSS& getDSS() { return *m_pDSS; }

    virtual void initialize();
    void start();
    virtual void shutdown() {}

    void log(const std::string& _message, aLogSeverity _severity = lsDebug);

    std::string getName() { return m_Name; }
  };

  class ThreadedSubsystem: public Subsystem, protected Thread {
    public:
      ThreadedSubsystem(DSS* _pDSS, const std::string& _name, const std::string& _threadName) : Subsystem(_pDSS, _name), Thread(_threadName.c_str()) {}
      virtual void shutdown() { Thread::terminate(); }
      bool isRunning() { return Thread::isRunning(); }
  };

}; // namespace dss

#endif /* SUBSYSTEM_H_ */
