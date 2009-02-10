/*
 * metering.h
 *
 *  Created on: Jan 29, 2009
 *      Author: patrick
 */

#ifndef METERING_H_
#define METERING_H_

#include "core/thread.h"
#include "core/subsystem.h"

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace dss {

  class MeteringConfig;

  class Metering : public Subsystem,
                   private Thread {
  private:
    int m_MeterCheckIntervalSeconds;
    std::string m_MeteringStorageLocation;
    std::vector<boost::shared_ptr<MeteringConfig> > m_Config;
  private:
    void CheckModulators();

    virtual void Execute();
  public:
    Metering(DSS* _pDSS);
    virtual ~Metering() {};

    virtual void Start();
  };

  class MeteringConfig {
  private:
    std::string m_FilenameSuffix;
    int m_Resolution;
    int m_NumberOfValues;
  public:
    MeteringConfig(const std::string& _filenameSuffix, int _resolution, int _numberOfValues)
    : m_FilenameSuffix(_filenameSuffix),
      m_Resolution(_resolution),
      m_NumberOfValues(_numberOfValues)
    { } // ctor

    const std::string& GetFilenameSuffix() const { return m_FilenameSuffix; }
    const int GetResolution() const { return m_Resolution; }
    const int GetNumberOfValues() const { return m_NumberOfValues; }
  };

} // namespace dss

#endif /* METERING_H_ */
