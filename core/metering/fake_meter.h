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

#ifndef FAKE_METER_H_
#define FAKE_METER_H_

#include "metering.h"
#include "series.h"
#include "unix/serialcom.h"

#include <boost/shared_ptr.hpp>
#include <string>

namespace dss {

  class FakeMeter : public ThreadedSubsystem {
  private:
    int m_LastValue;
    boost::shared_ptr<MeteringConfigChain> m_Config;
    std::vector<boost::shared_ptr<Series<CurrentValue> > > m_Series;

    SerialCom m_SerialCom;
    std::string m_MeteringStorageLocation;
  private:
    virtual void execute();
  protected:
    virtual void doStart();
  public:
    FakeMeter(DSS* _pDSS);
    virtual ~FakeMeter() {};

    virtual void initialize();
  }; // FakeMeter

}

#endif /* FAKE_METER_H_ */
