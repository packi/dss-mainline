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

#include "src/web/webrequests.h"
#include "base.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(JSON)

BOOST_AUTO_TEST_CASE(testJSONEscape) {
  JSONWriter json(JSONWriter::jsonNoneResult);
  json.add("x", std::string("\\\"\b\f\n\r\t\e"));
  std::string value = json.successJSON();
  BOOST_CHECK_EQUAL(value, "{\"x\":\"\\\\\\\"\\b\\f\\n\\r\\t\\u001B\"}");
}

BOOST_AUTO_TEST_CASE(testJSONEscapeMixedEnd) {
  JSONWriter json(JSONWriter::jsonNoneResult);
  json.add("x", std::string("abc\n"));
  std::string value = json.successJSON();
  BOOST_CHECK_EQUAL(value, "{\"x\":\"abc\\n\"}");
}

BOOST_AUTO_TEST_CASE(testJSONEscapeMixedStart) {
  JSONWriter json(JSONWriter::jsonNoneResult);
  json.add("x", std::string("\nabc"));
  std::string value = json.successJSON();
  BOOST_CHECK_EQUAL(value, "{\"x\":\"\\nabc\"}");
}

BOOST_AUTO_TEST_CASE(testJSONEscapeMixedMiddle) {
  JSONWriter json(JSONWriter::jsonNoneResult);
  json.add("x", std::string("abc\ndef"));
  std::string value = json.successJSON();
  BOOST_CHECK_EQUAL(value, "{\"x\":\"abc\\ndef\"}");
}

BOOST_AUTO_TEST_SUITE_END()
