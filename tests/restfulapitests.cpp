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

#include "core/web/restful.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(RestfulAPITests)

BOOST_AUTO_TEST_CASE(testDeclaring) {
    RestfulAPI api;
    BOOST_CHECK(!api.hasClass("apartment"));
    
    RestfulClass& clsApartment = api.addClass("apartment")
       .withDocumentation("A wrapper for global functions as well as adressing all devices connected to the dSS");

    BOOST_CHECK(api.hasClass("apartment"));
    BOOST_CHECK(!clsApartment.hasMethod("getName"));

     clsApartment.addMethod("getName")
      .withDocumentation("Returns the name of the apartment");

    BOOST_CHECK(clsApartment.hasMethod("getName"));
}

BOOST_AUTO_TEST_CASE(testRestfulRequest) {
  HashMapConstStringString params;
  std::string method = "test/method";

  RestfulRequest req(method, params);

  BOOST_CHECK_EQUAL(req.getClass(), "test");
  BOOST_CHECK_EQUAL(req.getMethod(), "method");
}

BOOST_AUTO_TEST_SUITE_END()
