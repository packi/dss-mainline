/*
 * Copyright (c) 2014 digitalSTROM.org, Zurich, Switzerland
 *
 * This file is part of digitalSTROM Server.
 *
 * digitalSTROM Server is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * digitalSTROM Server is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "src/base.h"
#include "src/sessionmanager.h"
#include "src/web/json.h"
#include "src/web/webserver.h"
#include "src/web/webrequests.h"

BOOST_AUTO_TEST_SUITE(Webserver)

BOOST_AUTO_TEST_CASE(testCookieGenarateParse) {
  boost::shared_ptr<dss::JSONObject> foo(new dss::JSONObject());
  std::string token = dss::SessionTokenGenerator::generate();

  dss::WebServerResponse response(foo);
  response.setCookie("path", "/");
  response.setCookie("token", token);

  std::string cookie_string = dss::generateCookieString(response.getCookies());
  dss::HashMapStringString cookies = dss::parseCookies(cookie_string.c_str());

  BOOST_CHECK(token == cookies["token"]);
}

BOOST_AUTO_TEST_CASE(testRequestGetClass) {
  const std::string urlid = "/icons/";
  std::string uri = "/icons/getDeviceIcon";
  std::string method = uri.substr(uri.find(urlid) + urlid.size());
  dss::RestfulRequest request(method, dss::HashMapStringString());
  BOOST_CHECK(request.getClass() == "getDeviceIcon");
  BOOST_CHECK(request.getUrlPath() == "getDeviceIcon");
}

BOOST_AUTO_TEST_CASE(testRequestGetClassMethod) {
  dss::RestfulRequest req("class/method", dss::HashMapStringString());
  BOOST_CHECK_EQUAL(req.getClass(), "class");
  BOOST_CHECK_EQUAL(req.getMethod(), "method");

  req = dss::RestfulRequest("class", dss::HashMapStringString());
  BOOST_CHECK_EQUAL(req.getClass(), "class");
  fprintf(stderr, "%s\n", req.getMethod().c_str());
  BOOST_CHECK(req.getMethod().empty());

  req = dss::RestfulRequest("", dss::HashMapStringString());
  BOOST_CHECK(req.getClass().empty());
  BOOST_CHECK(req.getMethod().empty());
}

BOOST_AUTO_TEST_SUITE_END()
