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

#include "src/web/json.h"
#include "base.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(JSON)

BOOST_AUTO_TEST_CASE(testStreamOperator) {
  JSONObject obj;
  std::ostringstream s;
  obj.addProperty("id", "foo");
  s << obj;
  s << obj;
  BOOST_CHECK_EQUAL(s.str(), "{\"id\":\"foo\"}{\"id\":\"foo\"}");
}

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
  boost::shared_ptr<JSONObject> obj2 = boost::make_shared<JSONObject>();
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
  boost::shared_ptr<JSONArray<int> > array = boost::make_shared<JSONArray<int> >();
  obj.addElement("elements", array);
  array->add(1);
  array->add(2);
  std::string value = obj.toString();
  BOOST_CHECK_EQUAL(value, "{\"elements\":[1,2]}");
}

BOOST_AUTO_TEST_CASE(testObjectWithSubArrayOfObject) {
  JSONObject obj;
  boost::shared_ptr<JSONArray<JSONObject> > array = boost::make_shared<JSONArray<JSONObject> >();
  int i;

  obj.addElement("elements", array);
  for (i = 0; i < 2; i++) {
    JSONObject tmp;
    tmp.addProperty("id", "foo" + intToString(i));
    tmp.addProperty("val", "bar" + intToString(i));
    array->add(tmp);
  }
  std::string value = obj.toString();
  BOOST_CHECK_EQUAL(value, "{\"elements\":[{\"id\":\"foo0\",\"val\":\"bar0\"},{\"id\":\"foo1\",\"val\":\"bar1\"}]}");
}

BOOST_AUTO_TEST_CASE(testJSONEscape) {
  JSONValue<std::string> str("\\\"\b\f\n\r\t\e");
  std::string value = str.toString();
  BOOST_CHECK_EQUAL(value, "\"\\\\\\\"\\b\\f\\n\\r\\t\\u001B\"");
}

BOOST_AUTO_TEST_CASE(testJSONEscapeMixedEnd) {
  JSONValue<std::string> str("abc\n");
  std::string value = str.toString();
  BOOST_CHECK_EQUAL(value, "\"abc\\n\"");
}

BOOST_AUTO_TEST_CASE(testJSONEscapeMixedStart) {
  JSONValue<std::string> str("\nabc");
  std::string value = str.toString();
  BOOST_CHECK_EQUAL(value, "\"\\nabc\"");
}

BOOST_AUTO_TEST_CASE(testJSONEscapeMixedMiddle) {
  JSONValue<std::string> str("abc\ndef");
  std::string value = str.toString();
  BOOST_CHECK_EQUAL(value, "\"abc\\ndef\"");
}

BOOST_AUTO_TEST_CASE(testGetElementByName) {
  const std::string kElementName = "child";
  boost::shared_ptr<JSONElement> parent = boost::make_shared<JSONObject>();
  boost::shared_ptr<JSONElement> child = boost::make_shared<JSONObject>();
  parent->addElement(kElementName, child);
  BOOST_CHECK_EQUAL(parent->getElementByName(kElementName), child);
}

BOOST_AUTO_TEST_CASE(testGetElementByNameReturnsNull) {
  const std::string kElementName = "child";
  boost::shared_ptr<JSONElement> parent = boost::make_shared<JSONObject>();
  BOOST_CHECK_EQUAL(parent->getElementByName(kElementName), boost::shared_ptr<JSONElement>());
}

BOOST_AUTO_TEST_CASE(testGetElementName) {
  const std::string kElementName = "child";
  boost::shared_ptr<JSONElement> parent = boost::make_shared<JSONObject>();
  boost::shared_ptr<JSONElement> child = boost::make_shared<JSONObject>();
  parent->addElement(kElementName, child);
  BOOST_CHECK_EQUAL(parent->getElementName(0), kElementName);
}

BOOST_AUTO_TEST_SUITE_END()
