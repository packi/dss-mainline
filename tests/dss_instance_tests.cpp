/*
    Copyright (c) 2014 digitalSTROM.org, Zurich, Switzerland

    Author: Andreas Fenkart

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

#include "tests/dss_life_cycle.hpp"
#include "propertysystem.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(DssInstanceTests)

BOOST_AUTO_TEST_CASE(testInitTearDown) {
  boost::scoped_ptr<DSSLifeCycle> guard;

  BOOST_CHECK_NO_THROW(guard.reset(new DSSLifeCycle));
  guard.reset();
  BOOST_CHECK(DSS::hasInstance() == false);
}


BOOST_AUTO_TEST_CASE(testReinitPropertySystem) {
  boost::scoped_ptr<DSSLifeCycle> guard;
  std::string websvc_url_authority_orig;

  guard.reset(new DSSLifeCycle());
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();

  websvc_url_authority_orig =
    propSystem.getStringValue(pp_websvc_url_authority);
  propSystem.getProperty(pp_websvc_url_authority)
    ->setStringValue("invalid_url");

  BOOST_CHECK_NE(propSystem.getStringValue(pp_websvc_url_authority),
                 websvc_url_authority_orig);

  // destroy and recreate dss instance
  guard.reset(new DSSLifeCycle());
  PropertySystem &propSystem2 = DSS::getInstance()->getPropertySystem();

  BOOST_CHECK_EQUAL(propSystem2.getStringValue(pp_websvc_url_authority),
                    websvc_url_authority_orig);
}

/** CAUTION: this must the last test, other unit tests rely on this */
BOOST_AUTO_TEST_CASE(test_no_DSS_hasInstance_false) {
  Logger::getInstance()->log("check no instance present", lsInfo);
  BOOST_CHECK(DSS::hasInstance() == false);
  DSS::shutdown(); // ensure it's down, follow up tests rely on it
}

BOOST_AUTO_TEST_SUITE_END()
