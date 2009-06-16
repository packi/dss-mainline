/*
 * subsystem.h
 *
 *  Created on: Feb 10, 2009
 *      Author: patrick
 */

#ifndef SUBSYSTEM_H_
#define SUBSYSTEM_H_

#include "logger.h"

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

    void log(const std::string& _message, aLogSeverity _severity = lsDebug);
  };

}; // namespace dss

#endif /* SUBSYSTEM_H_ */
