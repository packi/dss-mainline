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

namespace dss {

  class Metering : public Thread {
  private:
    int m_MeterCheckIntervalSeconds;
    std::string m_MeteringStorageLocation;
  private:
    void CheckModulators();
  public:
    virtual void Execute();
  };

} // namespace dss

#endif /* METERING_H_ */
