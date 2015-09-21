/*
    Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland

    Author: Christian Hitz, aizo ag <christian.hitz@aizo.com>

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

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_IGNORE_NON_ZERO_CHILD_CODE
#include <boost/test/unit_test.hpp>

#include "src/scripting/jsmodel.h"
#include "src/scripting/jsevent.h"
#include "src/scripting/jsmetering.h"
#include "src/scripting/scriptobject.h"
#include "src/scripting/jsproperty.h"
#include "src/event.h"
#include "src/eventinterpreterplugins.h"
#include "src/propertysystem.h"
#include "src/model/apartment.h"
#include "src/model/device.h"
#include "src/model/modulator.h"
#include "src/model/modelconst.h"
#include "src/metering/metering.h"

#include "util/dss_instance_fixture.h"

#include <rrd.h>

#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include <memory>

using namespace std;
using namespace dss;

DSUID_DEFINE(dsuid, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13);
DSUID_DEFINE(dsmeterDSID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10);

// wrapper class to access protected members
class testMetering: public dss::Metering {
public:
  testMetering(DSS* _pDSS) :
    dss::Metering(_pDSS) {
  }

  void test_checkDBConsistency() {
    checkDBConsistency();
  }

  bool test_checkDBReset(DateTime& _sampledAt, std::string& _rrdFileName) {
    return  checkDBReset(_sampledAt, _rrdFileName);
  }
};

BOOST_AUTO_TEST_SUITE(Metering)

BOOST_AUTO_TEST_CASE(seriesSizes) {
  Apartment apt(NULL);
  boost::shared_ptr<DSMeter> pMeter = apt.allocateDSMeter(dsuid);
  pMeter->setCapability_HasMetering(true);
  std::vector<boost::shared_ptr<DSMeter> > pMeters;
  pMeters.push_back(pMeter);
  dss::Metering metering(NULL);

  {
    DateTime startTime(DateTime::NullDate);
    DateTime endTime(DateTime::NullDate);
    int valueCount = 0;
    int resolution = 1;
    boost::shared_ptr<std::deque<Value> > pValues = metering.getSeries(pMeters,
                                                                       resolution,
                                                                       dss::Metering::etConsumption,
                                                                       false,
                                                                       startTime,
                                                                       endTime,
                                                                       valueCount);
    BOOST_CHECK_EQUAL(pValues->size(), 600 - 1);
  }
  {
    DateTime startTime(DateTime::NullDate);
    DateTime endTime(DateTime::NullDate);
    int valueCount = 0;
    int resolution = 60;
    boost::shared_ptr<std::deque<Value> > pValues = metering.getSeries(pMeters,
                                                                       resolution,
                                                                       dss::Metering::etConsumption,
                                                                       false,
                                                                       startTime,
                                                                       endTime,
                                                                       valueCount);
    BOOST_CHECK_EQUAL(pValues->size(), 720 - 1);
  }
  {
    DateTime startTime(DateTime::NullDate);
    DateTime endTime(DateTime::NullDate);
    int valueCount = 0;
    int resolution = 900;
    boost::shared_ptr<std::deque<Value> > pValues = metering.getSeries(pMeters,
                                                                       resolution,
                                                                       dss::Metering::etConsumption,
                                                                       false,
                                                                       startTime,
                                                                       endTime,
                                                                       valueCount);
    BOOST_CHECK_EQUAL(pValues->size(), 2976 - 1);
  }
  {
    DateTime startTime(DateTime::NullDate);
    DateTime endTime(DateTime::NullDate);
    int valueCount = 0;
    int resolution = 86400;
    boost::shared_ptr<std::deque<Value> > pValues = metering.getSeries(pMeters,
                                                                       resolution,
                                                                       dss::Metering::etConsumption,
                                                                       false,
                                                                       startTime,
                                                                       endTime,
                                                                       valueCount);
    BOOST_CHECK_EQUAL(pValues->size(), 370 - 1);
  }
  {
    DateTime startTime(DateTime::NullDate);
    DateTime endTime(DateTime::NullDate);
    int valueCount = 0;
    int resolution = 604800;
    boost::shared_ptr<std::deque<Value> > pValues = metering.getSeries(pMeters,
                                                                       resolution,
                                                                       dss::Metering::etConsumption,
                                                                       false,
                                                                       startTime,
                                                                       endTime,
                                                                       valueCount);
    BOOST_CHECK_EQUAL(pValues->size(), 260 - 1);
  }
  {
    DateTime startTime(DateTime::NullDate);
    DateTime endTime(DateTime::NullDate);
    int valueCount = 0;
    int resolution = 2592000;
    boost::shared_ptr<std::deque<Value> > pValues = metering.getSeries(pMeters,
                                                                       resolution,
                                                                       dss::Metering::etConsumption,
                                                                       false,
                                                                       startTime,
                                                                       endTime,
                                                                       valueCount);
    BOOST_CHECK_EQUAL(pValues->size(), 60 - 1);
  }
} // seriesSizes

BOOST_AUTO_TEST_CASE(seriesRanges) {
  Apartment apt(NULL);
  boost::shared_ptr<DSMeter> pMeter = apt.allocateDSMeter(dsuid);
  pMeter->setCapability_HasMetering(true);
  std::vector<boost::shared_ptr<DSMeter> > pMeters;
  pMeters.push_back(pMeter);
  dss::Metering metering(NULL);

  {
    DateTime now;
    DateTime endTime(DateTime::NullDate);
    DateTime startTime(now.secondsSinceEpoch() - 20);
    int valueCount = 0;
    int resolution = 1;
    boost::shared_ptr<std::deque<Value> > pValues = metering.getSeries(pMeters,
                                                                       resolution,
                                                                       dss::Metering::etConsumption,
                                                                       false,
                                                                       startTime,
                                                                       endTime,
                                                                       valueCount);
    BOOST_CHECK_EQUAL(pValues->size(), 20 - 1);
    BOOST_CHECK_EQUAL(pValues->at(0).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 19);
    BOOST_CHECK_EQUAL(pValues->at(1).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 18);
    BOOST_CHECK_EQUAL(pValues->at(18).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 1);
  }

  {
    DateTime now;
    DateTime endTime(DateTime::NullDate);
    DateTime startTime(now.secondsSinceEpoch() - 20);
    int valueCount = 10;
    int resolution = 1;
    boost::shared_ptr<std::deque<Value> > pValues = metering.getSeries(pMeters,
                                                                       resolution,
                                                                       dss::Metering::etConsumption,
                                                                       false,
                                                                       startTime,
                                                                       endTime,
                                                                       valueCount);
    BOOST_CHECK_EQUAL(pValues->size(), 10 - 1);
    BOOST_CHECK_EQUAL(pValues->at(0).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 19);
    BOOST_CHECK_EQUAL(pValues->at(1).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 18);
    BOOST_CHECK_EQUAL(pValues->at(8).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 11);
  }

  {
    DateTime now;
    DateTime endTime(now.secondsSinceEpoch() - 580);
    DateTime startTime(DateTime::NullDate);
    int valueCount = 0;
    int resolution = 1;
    boost::shared_ptr<std::deque<Value> > pValues = metering.getSeries(pMeters,
                                                                       resolution,
                                                                       dss::Metering::etConsumption,
                                                                       false,
                                                                       startTime,
                                                                       endTime,
                                                                       valueCount);
    BOOST_CHECK_EQUAL(pValues->size(), 20 - 1);
    BOOST_CHECK_EQUAL(pValues->at(0).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 599);
    BOOST_CHECK_EQUAL(pValues->at(1).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 598);
    BOOST_CHECK_EQUAL(pValues->at(18).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 581);
  }

  {
    DateTime now;
    DateTime endTime(now.secondsSinceEpoch() - 580);
    DateTime startTime(DateTime::NullDate);
    int valueCount = 10;
    int resolution = 1;
    boost::shared_ptr<std::deque<Value> > pValues = metering.getSeries(pMeters,
                                                                       resolution,
                                                                       dss::Metering::etConsumption,
                                                                       false,
                                                                       startTime,
                                                                       endTime,
                                                                       valueCount);
    BOOST_CHECK_EQUAL(pValues->size(), 10 - 1);
    BOOST_CHECK_EQUAL(pValues->at(0).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 589);
    BOOST_CHECK_EQUAL(pValues->at(1).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 588);
    BOOST_CHECK_EQUAL(pValues->at(8).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 581);
  }

  {
    DateTime now;
    DateTime endTime(now.secondsSinceEpoch() - 560);
    DateTime startTime(now.secondsSinceEpoch() - 580);
    int valueCount = 0;
    int resolution = 1;
    boost::shared_ptr<std::deque<Value> > pValues = metering.getSeries(pMeters,
                                                                       resolution,
                                                                       dss::Metering::etConsumption,
                                                                       false,
                                                                       startTime,
                                                                       endTime,
                                                                       valueCount);
    BOOST_CHECK_EQUAL(pValues->size(), 20 - 1);
    BOOST_CHECK_EQUAL(pValues->at(0).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 579);
    BOOST_CHECK_EQUAL(pValues->at(1).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 578);
    BOOST_CHECK_EQUAL(pValues->at(18).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 561);
  }

  {
    DateTime now;
    DateTime endTime(now.secondsSinceEpoch() - 560);
    DateTime startTime(now.secondsSinceEpoch() - 580);
    int valueCount = 10;
    int resolution = 1;
    boost::shared_ptr<std::deque<Value> > pValues = metering.getSeries(pMeters,
                                                                       resolution,
                                                                       dss::Metering::etConsumption,
                                                                       false,
                                                                       startTime,
                                                                       endTime,
                                                                       valueCount);
    BOOST_CHECK_EQUAL(pValues->size(), 20 - 1);
    BOOST_CHECK_EQUAL(pValues->at(0).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 579);
    BOOST_CHECK_EQUAL(pValues->at(1).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 578);
    BOOST_CHECK_EQUAL(pValues->at(18).getTimeStamp().secondsSinceEpoch(), now.secondsSinceEpoch() - 561);
  }
} // seriesSizes


BOOST_AUTO_TEST_CASE(seriesDateForward) {
  Apartment apt(NULL);
  boost::shared_ptr<DSMeter> pMeter = apt.allocateDSMeter(dsuid);
  pMeter->setCapability_HasMetering(true);
  std::vector<boost::shared_ptr<DSMeter> > pMeters;
  pMeters.push_back(pMeter);
  testMetering metering(NULL);

  std::string fileName = metering.getStorageLocation() + dsuid2str(pMeter->getDSID()) + ".rrd";

  {
    // add some entry to db.
    //check that timestamp of last entry is in range of latest entry
    DateTime now;
    for (int ctr = 0; ctr < 10; ++ctr) {
      metering.postMeteringEvent(pMeter, 22, 44, now);
      now = now.addSeconds(1);
    }
    metering.test_checkDBConsistency();
    DateTime lastDbEntry(rrd_last_r(fileName.c_str()));
    int delta = lastDbEntry.difference(now);

    bool success = (delta > -2) && (delta < 2);
    BOOST_CHECK_EQUAL(success, true);
  }

  {
    // db roll forward
    //check that timestamp of last entry is in range of latest entry
    DateTime now;
    now = now.addDay(6);
    metering.postMeteringEvent(pMeter, 22, 44, now);
    metering.test_checkDBConsistency();
    DateTime lastDbEntry(rrd_last_r(fileName.c_str()));
    int delta = lastDbEntry.difference(now);
    bool success = (delta > -2) && (delta < 2);
    BOOST_CHECK_EQUAL(success, true);
  }

  {
    // timestamp of data too far in the future.
    // recreate database
    DateTime now;
    DateTime future = now.addDay(15);
    metering.postMeteringEvent(pMeter, 22, 44, future);
    metering.test_checkDBConsistency();
    DateTime lastDbEntry(rrd_last_r(fileName.c_str()));
    int delta = lastDbEntry.difference(now);
    // the new database constains the timestamp of the system time.
    //the creation contains a timestamp a few seconds before the actual time
    bool success = ((delta <= 0) && (delta > -30));
    BOOST_CHECK_EQUAL(success, true);
  }
}

BOOST_AUTO_TEST_CASE(seriesDateReverse) {
  Apartment apt(NULL);
  boost::shared_ptr<DSMeter> pMeter = apt.allocateDSMeter(dsuid);
  pMeter->setCapability_HasMetering(true);
  std::vector<boost::shared_ptr<DSMeter> > pMeters;
  pMeters.push_back(pMeter);
  testMetering metering(NULL);
  DateTime recentTS;

  std::string fileName = metering.getStorageLocation() + dsuid2str(pMeter->getDSID()) + ".rrd";
  {
    //insert some data
    for (int ctr = 0; ctr < 10; ++ctr) {
      metering.postMeteringEvent(pMeter, 22, 44, recentTS);
      recentTS = recentTS.addSeconds(1);
    }
    metering.test_checkDBConsistency();
    recentTS = rrd_last_r(fileName.c_str());
  }

  {
    // insert earlier data
    // db should not change
    DateTime past = recentTS.addDay(-5);
    metering.postMeteringEvent(pMeter, 22, 44, past);
    metering.test_checkDBConsistency();
    DateTime lastDbEntry(rrd_last_r(fileName.c_str()));
    int delta = lastDbEntry.difference(recentTS);
    BOOST_CHECK_EQUAL(delta, 0);
  }

  {
    // insert earlier data
    // db should not change, since system date is used as a reference
    DateTime past = recentTS.addDay(-15);
    metering.postMeteringEvent(pMeter, 22, 44, past);
    metering.test_checkDBConsistency();
    DateTime lastDbEntry(rrd_last_r(fileName.c_str()));
    int delta = lastDbEntry.difference(recentTS);
    BOOST_CHECK_EQUAL(delta, 0);
  }
}

BOOST_AUTO_TEST_CASE(seriesDateCheckDbReset) {
  Apartment apt(NULL);
  boost::shared_ptr<DSMeter> pMeter = apt.allocateDSMeter(dsuid);
  pMeter->setCapability_HasMetering(true);
  std::vector<boost::shared_ptr<DSMeter> > pMeters;
  pMeters.push_back(pMeter);
  testMetering metering(NULL);
  DateTime recentTS;
  std::string fileName = metering.getStorageLocation() + dsuid2str(pMeter->getDSID()) + ".rrd";

  {
    //insert some data
    for (int ctr = 0; ctr < 10; ++ctr) {
      metering.postMeteringEvent(pMeter, 22, 44, recentTS);
      recentTS = recentTS.addSeconds(1);
    }
    metering.test_checkDBConsistency();
    recentTS = rrd_last_r(fileName.c_str());
  }

  {
    // check DB reset: timestamp within threshold
    // db not modified
    DateTime threshold = recentTS.addDay(-6);
    metering.test_checkDBReset(threshold,fileName);
    DateTime lastDbEntry(rrd_last_r(fileName.c_str()));
    int delta = lastDbEntry.difference(recentTS);
    BOOST_CHECK_EQUAL(delta, 0);
  }

  {
    // check DB reset: timestamp outside threshold
    // db should be recreated
    DateTime threshold = recentTS.addDay(-8);
    metering.test_checkDBReset(threshold, fileName);
    DateTime lastDbEntry(rrd_last_r(fileName.c_str()));
    int delta = lastDbEntry.difference(recentTS);
    BOOST_CHECK_NE(delta, 0);
  }
}

BOOST_FIXTURE_TEST_CASE(testEnergyMeterRestoreData, DSSInstanceFixture) {
  Apartment apt(DSS::getInstance());
  boost::shared_ptr<dss::Metering> pMetering = boost::make_shared<dss::Metering>(DSS::getInstance());
  apt.setMetering(pMetering.get());

  {
    std::string fileName = pMetering->getStorageLocation() + dsuid2str(dsmeterDSID) + ".rrd";
    boost::filesystem::remove(fileName);
  }

  DateTime now;
  {
    boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dsmeterDSID);
    meter->setCapability_HasMetering(true);
    meter->updateEnergyMeterValue(100);
    pMetering->postMeteringEvent(meter, 0, (unsigned long long)(meter->getCachedEnergyMeterValue() + 0.5), now);
    meter->updateEnergyMeterValue(120);
    pMetering->postMeteringEvent(meter, 0, (unsigned long long)(meter->getCachedEnergyMeterValue() + 0.5), now.addSeconds(1));
    BOOST_CHECK_EQUAL(120, meter->getEnergyMeterValue());
  }
  apt.removeDSMeter(dsmeterDSID);
  {
    boost::shared_ptr<DSMeter> meter = apt.allocateDSMeter(dsmeterDSID);
    meter->setCapability_HasMetering(true);
    meter->updateEnergyMeterValue(10);
    pMetering->postMeteringEvent(meter, 0, (unsigned long long)(meter->getCachedEnergyMeterValue() + 0.5), now.addSeconds(2));
    meter->updateEnergyMeterValue(20);
    pMetering->postMeteringEvent(meter, 0, (unsigned long long)(meter->getCachedEnergyMeterValue() + 0.5), now.addSeconds(3));
    BOOST_CHECK_EQUAL(130, meter->getEnergyMeterValue());
  }

  {
    std::string fileName = pMetering->getStorageLocation() + dsuid2str(dsmeterDSID) + ".rrd";
    boost::filesystem::remove(fileName);
  }
}

BOOST_AUTO_TEST_SUITE_END()
