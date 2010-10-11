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

#include "core/model/device.h"
#include "core/model/apartment.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(DeviceTests)

BOOST_AUTO_TEST_CASE(testGroups) {
  Apartment apt(NULL);
  boost::shared_ptr<Device> dev = apt.allocateDevice(dss_dsid_t(0,1));
  BOOST_CHECK_EQUAL(dev->getGroupsCount(), 0);
  dev->resetGroups();
  BOOST_CHECK_EQUAL(dev->getGroupsCount(), 0);
  dev->addToGroup(1);
  BOOST_CHECK_EQUAL(dev->getGroupsCount(), 1);
  BOOST_CHECK_EQUAL(dev->getGroupIdByIndex(0), 1);
  BOOST_CHECK_EQUAL(dev->getGroupByIndex(0)->getID(), 1);
  dev->addToGroup(2);
  BOOST_CHECK_EQUAL(dev->getGroupsCount(), 2);
  BOOST_CHECK_EQUAL(dev->getGroupIdByIndex(0), 1);
  BOOST_CHECK_EQUAL(dev->getGroupByIndex(0)->getID(), 1);
  BOOST_CHECK_EQUAL(dev->getGroupIdByIndex(1), 2);
  BOOST_CHECK_EQUAL(dev->getGroupByIndex(1)->getID(), 2);
  // adding the device to the same groups again shouldn't change a thing
  dev->addToGroup(1);
  dev->addToGroup(2);
  BOOST_CHECK_EQUAL(dev->getGroupsCount(), 2);
  BOOST_CHECK_EQUAL(dev->getGroupIdByIndex(0), 1);
  BOOST_CHECK_EQUAL(dev->getGroupByIndex(0)->getID(), 1);
  BOOST_CHECK_EQUAL(dev->getGroupIdByIndex(1), 2);
  BOOST_CHECK_EQUAL(dev->getGroupByIndex(1)->getID(), 2);
  BOOST_CHECK(dev->getGroupBitmask().test(0));
  BOOST_CHECK(dev->getGroupBitmask().test(1));
  
  dev->removeFromGroup(1);
  BOOST_CHECK_EQUAL(dev->getGroupsCount(), 1);
  BOOST_CHECK_EQUAL(dev->getGroupIdByIndex(0), 2);
  BOOST_CHECK_EQUAL(dev->getGroupByIndex(0)->getID(), 2);
  BOOST_CHECK(!dev->getGroupBitmask().test(0));
  BOOST_CHECK(dev->getGroupBitmask().test(1));
}

BOOST_AUTO_TEST_SUITE_END()
