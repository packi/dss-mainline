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

#include "core/web/json.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(JSON)

BOOST_AUTO_TEST_CASE(testObjectEmpty) {
  JSONObject obj;
  std::string value = obj.toString();
  BOOST_CHECK_EQUAL(value, "{}");
}

BOOST_AUTO_TEST_CASE(testValueInt) {
  JSONValue<int> i(1);
  std::string value = i.toString();
  BOOST_CHECK_EQUAL(value, "1");
}

BOOST_AUTO_TEST_CASE(testPropertyString) {
  JSONValue<std::string> str("1");
  std::string value = str.toString();
  BOOST_CHECK_EQUAL(value, "\"1\"");
}

BOOST_AUTO_TEST_CASE(testPropertyBoolFalse) {
  JSONValue<bool> b(false);
  std::string value = b.toString();
  BOOST_CHECK_EQUAL(value, "false");
}

BOOST_AUTO_TEST_CASE(testPropertyBoolTrue) {
  JSONValue<bool> b(true);
  std::string value = b.toString();
  BOOST_CHECK_EQUAL(value, "true");
}

BOOST_AUTO_TEST_CASE(testPropertyDoubleTrue) {
  JSONValue<double> d(10.5);
  std::string value = d.toString();
  BOOST_CHECK_EQUAL(value, "10.5");
}

BOOST_AUTO_TEST_CASE(testObjectWithProperty) {
  JSONObject obj;
  obj.addProperty("name", "test");
  std::string value = obj.toString();
  BOOST_CHECK_EQUAL(value, "{\"name\":\"test\"}");
}

BOOST_AUTO_TEST_CASE(testObjectWithProperties) {
  JSONObject obj;
  obj.addProperty("name", "test");
  obj.addProperty("i", 1);
  obj.addProperty("b", false);
  std::string value = obj.toString();
  BOOST_CHECK_EQUAL(value, "{\"name\":\"test\",\"i\":1,\"b\":false}");
}

BOOST_AUTO_TEST_CASE(testObjectWithSubObject) {
  JSONObject obj;
  obj.addProperty("name", "test");
  boost::shared_ptr<JSONObject> obj2(new JSONObject());
  obj2->addProperty("street", "blastreet");
  obj.addElement("address", obj2);
  std::string value = obj.toString();
  BOOST_CHECK_EQUAL(value, "{\"name\":\"test\",\"address\":{\"street\":\"blastreet\"}}");
}

BOOST_AUTO_TEST_CASE(testIntArrayNoElements) {
  JSONArray<int> array;
  array.add(1);
  std::string value = array.toString();
  BOOST_CHECK_EQUAL(value, "[1]");
}

BOOST_AUTO_TEST_CASE(testIntArrayOneElement) {
  JSONArray<int> array;
  array.add(1);
  std::string value = array.toString();
  BOOST_CHECK_EQUAL(value, "[1]");
}

BOOST_AUTO_TEST_CASE(testIntArrayTwoElements) {
  JSONArray<int> array;
  array.add(1);
  array.add(2);
  std::string value = array.toString();
  BOOST_CHECK_EQUAL(value, "[1,2]");
}

BOOST_AUTO_TEST_CASE(testStringArray) {
  JSONArray<std::string> array;
  array.add("1");
  array.add("2");
  std::string value = array.toString();
  BOOST_CHECK_EQUAL(value, "[\"1\",\"2\"]");
}

BOOST_AUTO_TEST_CASE(testObjectWithSubArray) {
  JSONObject obj;
  boost::shared_ptr<JSONArray<int> > array(new JSONArray<int>());
  obj.addElement("elements", array);
  array->add(1);
  array->add(2);
  std::string value = obj.toString();
  BOOST_CHECK_EQUAL(value, "{\"elements\":[1,2]}");
}


BOOST_AUTO_TEST_SUITE_END()