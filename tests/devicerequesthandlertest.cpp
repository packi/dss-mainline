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

#include "core/model/apartment.h"
#include "core/web/handler/devicerequesthandler.h"
#include "core/web/json.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(WebDevice)


class Fixture {
public:
  void testValidResponse(boost::shared_ptr<JSONObject> _response) {
    boost::shared_ptr<JSONElement> resultElem = _response->getElementByName("ok");
    if(resultElem == NULL) {
      BOOST_CHECK(false);
      return;
    }
    JSONValue<bool>* val = dynamic_cast<JSONValue<bool>*>(resultElem.get());
    if(val == NULL) {
      BOOST_CHECK(false);
      return;
    }
  }

  void testOkIs(boost::shared_ptr<JSONObject> _response, bool _value) {
    boost::shared_ptr<JSONElement> resultElem = _response->getElementByName("ok");
    if(resultElem == NULL) {
      BOOST_CHECK(false);
      return;
    }
    JSONValue<bool>* val = dynamic_cast<JSONValue<bool>*>(resultElem.get());
    if(val == NULL) {
      BOOST_CHECK(false);
      return;
    }
    BOOST_CHECK_EQUAL(val->getValue(), _value);
  }
};

BOOST_FIXTURE_TEST_CASE(testNoParam, Fixture) {
  Apartment apt(NULL);
  DeviceRequestHandler handler(apt);
  HashMapConstStringString params;
  RestfulRequest req("device/bla", params);
  boost::shared_ptr<JSONObject> response = handler.jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testEmptyNameParam, Fixture) {
  Apartment apt(NULL);
  DeviceRequestHandler handler(apt);
  HashMapConstStringString params;
  params["name"] = "";
  RestfulRequest req("device/bla", params);
  boost::shared_ptr<JSONObject> response = handler.jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testEmptyDSIDParam, Fixture) {
  Apartment apt(NULL);
  DeviceRequestHandler handler(apt);
  HashMapConstStringString params;
  params["dsid"] = "";
  RestfulRequest req("device/bla", params);
  boost::shared_ptr<JSONObject> response = handler.jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testInvalidDSIDParam, Fixture) {
  Apartment apt(NULL);
  DeviceRequestHandler handler(apt);
  HashMapConstStringString params;
  params["dsid"] = "asdfasdf";
  RestfulRequest req("device/bla", params);
  boost::shared_ptr<JSONObject> response = handler.jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testTooLongDSIDParam, Fixture) {
  Apartment apt(NULL);
  DeviceRequestHandler handler(apt);
  HashMapConstStringString params;
  params["dsid"] = "3504175fe0000000ffc000113504175fe0000000ffc00011";
  RestfulRequest req("device/bla", params);
  boost::shared_ptr<JSONObject> response = handler.jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_AUTO_TEST_SUITE_END()

