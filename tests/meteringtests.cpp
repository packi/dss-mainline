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

#include <boost/scoped_ptr.hpp>
#include <memory>

using namespace std;
using namespace dss;

BOOST_AUTO_TEST_SUITE(Metering)

BOOST_AUTO_TEST_CASE(seriesSizes) {
  Apartment apt(NULL);
  boost::shared_ptr<DSMeter> pMeter = apt.allocateDSMeter(dss_dsid_t(0,13));
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
  boost::shared_ptr<DSMeter> pMeter = apt.allocateDSMeter(dss_dsid_t(0,13));
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

BOOST_AUTO_TEST_SUITE_END()
