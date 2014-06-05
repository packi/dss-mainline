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
  dss::WebServerResponse response(foo);

  const char set_cookie_str[] =
    "token=4a9b442b2554e794a126f761b953c3b88b5d67d1f665a7e1590b8df67b9c6846; path=/";
  const char static_token[] =
    "4a9b442b2554e794a126f761b953c3b88b5d67d1f665a7e1590b8df67b9c6846";
  response.setCookie("path", "/");
  response.setCookie("token", static_token);
  std::string cookie_string = dss::generateCookieString(response.getCookies());
  BOOST_CHECK_EQUAL(cookie_string, set_cookie_str);
  BOOST_CHECK_EQUAL(dss::extractToken(cookie_string.c_str()), static_token);

  // also check current token generator
  std::string token = dss::SessionTokenGenerator::generate();
  response.setCookie("path", "/");
  response.setCookie("token", token);
  cookie_string = dss::generateCookieString(response.getCookies());
  BOOST_CHECK_EQUAL(dss::extractToken(cookie_string.c_str()), token);

  const char static firefox_token[] =
	  "csrftoken=ArERl0K0L; compact_display_state=true; token=ccf67fe364c";
  BOOST_CHECK_EQUAL(dss::extractToken(firefox_token), "ccf67fe364c");
}

BOOST_AUTO_TEST_CASE(testUriToplevelSplit) {
  std::string toplevel, sublevel;
  std::string uri_path = "/json/event/subscribe";
  size_t offset = uri_path.find('/', 1);
  if (std::string::npos != offset) {
    toplevel = uri_path.substr(0, offset);
    sublevel = uri_path.substr(offset);
  }
  BOOST_CHECK(toplevel == "/json");
  BOOST_CHECK(sublevel == "/event/subscribe");
}

BOOST_AUTO_TEST_CASE(testRequestGetClass) {
  dss::RestfulRequest request("/getDeviceIcon", "");
  BOOST_CHECK(request.getClass() == "getDeviceIcon");
  BOOST_CHECK(request.getUrlPath() == "/getDeviceIcon");
}

BOOST_AUTO_TEST_CASE(testRequestGetClassMethod) {
  dss::RestfulRequest req("/class/method", "");
  BOOST_CHECK_EQUAL(req.getClass(), "class");
  BOOST_CHECK_EQUAL(req.getMethod(), "method");

  req = dss::RestfulRequest("/class", "");
  BOOST_CHECK_EQUAL(req.getClass(), "class");
  fprintf(stderr, "%s\n", req.getMethod().c_str());
  BOOST_CHECK(req.getMethod().empty());

  req = dss::RestfulRequest("/", "");
  BOOST_CHECK(req.getClass().empty());
  BOOST_CHECK(req.getMethod().empty());

  req = dss::RestfulRequest("", "");
  BOOST_CHECK(req.getClass().empty());
  BOOST_CHECK(req.getMethod().empty());
}

BOOST_AUTO_TEST_CASE(testRequestHasParams) {
  dss::RestfulRequest req("/url", "foo=1&bar=2");
  BOOST_CHECK(req.hasParameter("foo"));
  BOOST_CHECK(req.hasParameter("bar"));
  BOOST_CHECK(!req.hasParameter("gro"));

  req = dss::RestfulRequest("/url", "foo=&bar=");
  BOOST_CHECK(req.hasParameter("foo"));
  BOOST_CHECK(req.hasParameter("bar"));
  BOOST_CHECK(!req.hasParameter("gro"));

  req = dss::RestfulRequest("/url", "ana=b&a=bar");
  BOOST_CHECK(req.hasParameter("a"));
  BOOST_CHECK(req.hasParameter("ana"));
  BOOST_CHECK(!req.hasParameter("na"));
}

BOOST_AUTO_TEST_CASE(testRestfulParamParser) {
  std::string params;

  params = "ana=b&a=bar";
  dss::RestfulRequest req("device/setConfig", params);
  BOOST_CHECK(req.getParameter("a") == "bar");

  req = dss::RestfulRequest("device/bla", "name=");
  BOOST_CHECK(req.hasParameter("name"));
  req = dss::RestfulRequest("device/bla", "name=nonexisting");
  BOOST_CHECK(req.getParameter("name") == "nonexisting");
  req = dss::RestfulRequest("device/bla", "dsid=");
  BOOST_CHECK(req.getParameter("dsid").empty());
  req = dss::RestfulRequest("device/bla",
                            "dsid=3504175fe0000000ffc000113504175fe0000000ffc00011");
  BOOST_CHECK(req.getParameter("dsid") ==
              "3504175fe0000000ffc000113504175fe0000000ffc00011");
  req = dss::RestfulRequest("circuit/bla", "&id=asdfasdf");
  BOOST_CHECK(req.getParameter("id") == "asdfasdf");

  params = "dsid=0x12345";
  params += "&class=469";
  params += "&index=452";
  params += "&value=21";
  req = dss::RestfulRequest("device/setConfig", params);
  BOOST_CHECK(req.getParameter("class") == "469");
  BOOST_CHECK(req.getParameter("index") == "452");
  BOOST_CHECK(req.getParameter("value") == "21");

  params = "foo=bar&token=0x123456789abcdef";
  req = dss::RestfulRequest("/foo/bar", params);
  BOOST_CHECK(req.getParameter("token") == "0x123456789abcdef");
}

BOOST_AUTO_TEST_CASE(testRestfulParamParserComplex) {
  char params[] = "name=test-addon.config&parameter=actions%3Dtest%3Bvalue%3D%257B%27name%27%253A%27Zeit%252520zum%252520gehen%27%252C%27id%27%253A%270%27%252C%27time%27%253A%257B%27offset%27%253A62100%252C%27timeBase%27%253A%27daily%27%257D%252C%27recurrence%27%253A%257B%27timeArray%27%253A%255B%27MO%27%252C%27WE%27%252C%27TH%27%255D%252C%27recurrenceBase%27%253A%27weekly%27%257D%252C%27actions%27%253A%255B%257B%270%27%253A%255B%257B%257D%252C%257B%257D%252C%257B%257D%252C%257B%257D%252C%257B%257D%255D%252C%27type%27%253A%27zone-blink%27%252C%27zone%27%253A1695%252C%27group%27%253A1%252C%27delay%27%253A0%252C%27category%27%253A%27timer%27%257D%255D%252C%27conditions%27%253A%257B%27enabled%27%253Atrue%252C%27weekdays%27%253Anull%252C%27timeframe%27%253Anull%252C%27zoneState%27%253Anull%252C%27systemState%27%253A%255B%257B%27name%27%253A%27holiday%27%252C%27value%27%253A%27off%27%257D%255D%252C%27addonState%27%253Anull%257D%257D&a=0.8279124496545545&";

  char unencoded_check[] =
    "actions=test;value=%7B'name'%3A'Zeit%2520zum%2520gehen'%2C'id'%3A'0'%2C'time'%3A%7B'offset'%3A62100%2C'timeBase'%3A'daily'%7D%2C'recurrence'%3A%7B'timeArray'%3A%5B'MO'%2C'WE'%2C'TH'%5D%2C'recurrenceBase'%3A'weekly'%7D%2C'actions'%3A%5B%7B'0'%3A%5B%7B%7D%2C%7B%7D%2C%7B%7D%2C%7B%7D%2C%7B%7D%5D%2C'type'%3A'zone-blink'%2C'zone'%3A1695%2C'group'%3A1%2C'delay'%3A0%2C'category'%3A'timer'%7D%5D%2C'conditions'%3A%7B'enabled'%3Atrue%2C'weekdays'%3Anull%2C'timeframe'%3Anull%2C'zoneState'%3Anull%2C'systemState'%3A%5B%7B'name'%3A'holiday'%2C'value'%3A'off'%7D%5D%2C'addonState'%3Anull%7D%7D";

  dss::RestfulRequest req("/foo/bar", params);
  BOOST_CHECK(req.getParameter("name") == "test-addon.config");
  BOOST_CHECK(req.getParameter("parameter") == unencoded_check);
  BOOST_CHECK(req.getParameter("a") == "0.8279124496545545");
}
BOOST_AUTO_TEST_SUITE_END()
