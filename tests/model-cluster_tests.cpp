/*
 *  Copyright (c) 2015 digitalSTROM.org, Zurich, Switzerland
 *
 *  Author: Andreas Fenkart, <andreas.fenkart@digitalstrom.com>
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/shared_ptr.hpp>

#include "src/model/apartment.h"
#include "src/model/cluster.h"
#include "src/model/modelconst.h"
#include "tests/util/dss_instance_fixture.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(cluster_test)

BOOST_FIXTURE_TEST_CASE(ClusterInit, DSSInstanceFixture) {
  Apartment &ap(DSS::getInstance()->getApartment());

  // apartment creates zone-0 and all clusters too
  boost::shared_ptr<Cluster> cl = ap.getCluster(GroupIDAppUserMin);
}

BOOST_FIXTURE_TEST_CASE(ClusterOpLock, DSSInstanceFixture) {
  Apartment &ap(DSS::getInstance()->getApartment());
  PropertySystem &props(DSS::getInstance()->getPropertySystem());

  boost::shared_ptr<Cluster> cl = ap.getCluster(GroupIDAppUserMin);

  std::string opLockProp = "/usr/states/cluster." + intToString(cl->getID()) + ".operation_lock";
  BOOST_CHECK(props.getProperty(opLockProp) == NULL);

  // activating the operation lock will create the corresponding state
  cl->setOperationLock(true, coTest);
  BOOST_CHECK(props.getProperty(opLockProp) != NULL);

  PropertyNodePtr n = props.getProperty(opLockProp);
  BOOST_CHECK_EQUAL(n->getProperty("name")->getStringValue(), "cluster.16.operation_lock");
  BOOST_CHECK_EQUAL(n->getProperty("state")->getStringValue(), "active");
  BOOST_CHECK_EQUAL(n->getProperty("value")->getIntegerValue(), 1);

  cl->setOperationLock(false, coTest);
  BOOST_CHECK_EQUAL(n->getProperty("state")->getStringValue(), "inactive");
  BOOST_CHECK_EQUAL(n->getProperty("value")->getIntegerValue(), 2);
}

BOOST_AUTO_TEST_SUITE_END()
