/*
 * metering.h
 *
 *  Created on: Jan 29, 2009
 *      Author: patrick
 */

#ifndef METERING_H_
#define METERING_H_

#include "core/thread.h"

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace dss {

  class MeteringConfig;

  class Metering : public Thread {
  private:
    int m_MeterCheckIntervalSeconds;
    std::string m_MeteringStorageLocation;
    std::vector<boost::shared_ptr<MeteringConfig> > m_Config;
  private:
    void CheckModulators();
  public:
    Metering();
    virtual ~Metering() {};

    virtual void Execute();
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
