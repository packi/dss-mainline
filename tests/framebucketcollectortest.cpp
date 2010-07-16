/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

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

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "core/base.h"
#include "core/ds485/framebucketcollector.h"
#include "core/ds485/businterfacehandler.h"

using namespace std;
using namespace dss;

static void addFrame(FrameBucketCollector& _collector) {
  sleepMS(10);
  boost::shared_ptr<DS485CommandFrame> frame(new DS485CommandFrame());
  _collector.addFrame(frame);
}

BOOST_AUTO_TEST_SUITE(FrameBucketCollectorTest)

BOOST_AUTO_TEST_CASE(testIsEmptyAtStart) {
  FrameBucketHolder holder;
  FrameBucketCollector collector(&holder, 0, 0);
  BOOST_CHECK_EQUAL(collector.isEmpty(), true);
  BOOST_CHECK_EQUAL(collector.getFrameCount(), 0);
}

BOOST_AUTO_TEST_CASE(testWaitForWithPacketReturnsTrue) {
  FrameBucketHolder holder;
  FrameBucketCollector collector(&holder, 0, 0);
  boost::shared_ptr<DS485CommandFrame> frame(new DS485CommandFrame());
  collector.addFrame(frame);
  BOOST_CHECK_EQUAL(collector.isEmpty(), false);
  BOOST_CHECK_EQUAL(collector.getFrameCount(), 1);
}

BOOST_AUTO_TEST_CASE(testWaitForWithDelayedPacketReturnsTrue) {
  FrameBucketHolder holder;
  FrameBucketCollector collector(&holder, 0, 0);
  boost::thread th;
  th = boost::thread(boost::bind(&addFrame, boost::ref(collector)));
  BOOST_CHECK(collector.waitForFrame(20));
  BOOST_CHECK_EQUAL(collector.isEmpty(), false);
  BOOST_CHECK_EQUAL(collector.getFrameCount(), 1);
  th.join();
}

BOOST_AUTO_TEST_SUITE_END()
