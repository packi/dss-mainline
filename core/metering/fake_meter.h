/*
 * fake_meter.h
 *
 *  Created on: Apr 8, 2009
 *      Author: patrick
 */

#ifndef FAKE_METER_H_
#define FAKE_METER_H_

#include "metering.h"
#include "series.h"
#include "unix/serialcom.h"

#include <boost/shared_ptr.hpp>
#include <string>

namespace dss {

  class FakeMeter : public Subsystem,
                    private Thread {
  private:
    int m_LastValue;
    boost::shared_ptr<Series<CurrentValue> > m_Series;
    SerialCom m_SerialCom;
    std::string m_MeteringStorageLocation;
  private:
    virtual void Execute();
  protected:
    virtual void DoStart();
  public:
    FakeMeter(DSS* _pDSS);
    virtual ~FakeMeter() {};

  }; // FakeMeter

}

#endif /* FAKE_METER_H_ */
