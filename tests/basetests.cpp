/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#include "../core/base.h"
#include "../core/datetools.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(Base)

BOOST_AUTO_TEST_CASE(testCRC) {
  const char testStr[] = "123456789";

  uint16_t crc = crc16((unsigned const char*)&testStr, sizeof(testStr)-1);
  BOOST_CHECK_EQUAL((uint16_t)0x2189, crc);


  const char testStr2[] = "\xfd\x01\x03\x14\xbb\x01\x00\x00";//\xeb\x08";
  crc = crc16((unsigned const char*)&testStr2, sizeof(testStr2)-1);
  BOOST_CHECK_EQUAL((uint16_t)0x08eb, crc);
}

BOOST_AUTO_TEST_CASE(testUrlDecode) {
  BOOST_CHECK_EQUAL(std::string(" "), urlDecode("%20"));
  BOOST_CHECK_EQUAL(std::string("a "), urlDecode("a%20"));
  BOOST_CHECK_EQUAL(std::string(" a"), urlDecode("%20a"));
  BOOST_CHECK_EQUAL(std::string("v a"), urlDecode("v%20a"));
  BOOST_CHECK_EQUAL(std::string("  "), urlDecode("%20%20"));
  BOOST_CHECK_EQUAL(std::string("   "), urlDecode("%20%20%20"));

  BOOST_CHECK_EQUAL(std::string(" "), urlDecode("+"));
  BOOST_CHECK_EQUAL(std::string("  "), urlDecode("++"));
  BOOST_CHECK_EQUAL(std::string(" a "), urlDecode("+a+"));
  BOOST_CHECK_EQUAL(std::string("  a"), urlDecode("++a"));
  BOOST_CHECK_EQUAL(std::string("a  "), urlDecode("a++"));
  BOOST_CHECK_EQUAL(std::string("  b"), urlDecode("+%20b"));
  BOOST_CHECK_EQUAL(std::string("sourceid=1&schedule=FREQ=MINUTELY;INTERVAL=1&start=20080520T080000Z"),
                       urlDecode("sourceid=1&schedule=FREQ%3DMINUTELY%3BINTERVAL%3D1&start=20080520T080000Z"));
} // teturlDecode

BOOST_AUTO_TEST_CASE(testStrToInt) {
  BOOST_CHECK_EQUAL(1, strToInt("1"));
  BOOST_CHECK_EQUAL(1, strToInt("0x1"));
  BOOST_CHECK_EQUAL(10, strToInt("0xa"));

  BOOST_CHECK_THROW(strToInt("asdf"), std::invalid_argument);
  BOOST_CHECK_THROW(strToInt(""), std::invalid_argument);
  BOOST_CHECK_THROW(strToInt(" "), std::invalid_argument);
} // testStrToInt

BOOST_AUTO_TEST_CASE(testStrToIntDef) {
  BOOST_CHECK_EQUAL(-1, strToIntDef("", -1));
  BOOST_CHECK_EQUAL(-1, strToIntDef(" ", -1));
  BOOST_CHECK_EQUAL(-1, strToIntDef("gfdfg", -1));
  BOOST_CHECK_EQUAL(1, strToIntDef("1", -1));
} // testStrToIntDef

BOOST_AUTO_TEST_CASE(testStrToUInt) {
  BOOST_CHECK_EQUAL(7, int(strToUInt("7")));

  BOOST_CHECK_THROW(strToUInt("asdf"), std::invalid_argument);
  BOOST_CHECK_THROW(strToUInt(""), std::invalid_argument);
  BOOST_CHECK_THROW(strToUInt(" "), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(testStrToDouble) {
  BOOST_CHECK_EQUAL(0.0, strToDouble("0.0"));
  BOOST_CHECK_EQUAL(-1.0, strToDouble("asdf", -1.0));
  BOOST_CHECK_EQUAL(1.0, strToDouble("1.00", -1.0));
  BOOST_CHECK_THROW(strToDouble("asdf"), std::invalid_argument);
} // testStrToDouble

BOOST_AUTO_TEST_CASE(testUintToString) {
  BOOST_CHECK_EQUAL("0", uintToString(0));
  BOOST_CHECK_EQUAL("2222", uintToString(2222));
} // testUintToString

BOOST_AUTO_TEST_CASE(testUnsignedLongIntToHexString) {
  BOOST_CHECK_EQUAL("42", unsignedLongIntToHexString(0x42));
} // testUnsignedLongIntToHexString

BOOST_AUTO_TEST_CASE(testTrim) {
  // reductions to zero-std::string
  BOOST_CHECK_EQUAL(std::string(""), trim(""));
  BOOST_CHECK_EQUAL(std::string(""), trim(" "));
  BOOST_CHECK_EQUAL(std::string(""), trim("  "));
  BOOST_CHECK_EQUAL(std::string(""), trim("     "));
  BOOST_CHECK_EQUAL(std::string(""), trim("\r"));
  BOOST_CHECK_EQUAL(std::string(""), trim(" \r"));
  BOOST_CHECK_EQUAL(std::string(""), trim(" \r "));
  BOOST_CHECK_EQUAL(std::string(""), trim("\n"));
  BOOST_CHECK_EQUAL(std::string(""), trim(" \n"));
  BOOST_CHECK_EQUAL(std::string(""), trim(" \n "));
  BOOST_CHECK_EQUAL(std::string(""), trim("\t"));
  BOOST_CHECK_EQUAL(std::string(""), trim(" \t "));

  // reductions to single character
  BOOST_CHECK_EQUAL(std::string("a"), trim(" a "));
  BOOST_CHECK_EQUAL(std::string("a"), trim("  a "));
  BOOST_CHECK_EQUAL(std::string("a"), trim("  a  "));

  // noop
  BOOST_CHECK_EQUAL(std::string("a a"), trim("a a"));
  BOOST_CHECK_EQUAL(std::string("a\ra"), trim("a\ra"));
  BOOST_CHECK_EQUAL(std::string("a\ta"), trim("a\ta"));
  BOOST_CHECK_EQUAL(std::string("a\na"), trim("a\na"));
  BOOST_CHECK_EQUAL(std::string("a  a"), trim("a  a"));

  // reductions to multiple character & no-op
  BOOST_CHECK_EQUAL(std::string("a a"), trim(" a a "));
  BOOST_CHECK_EQUAL(std::string("a a"), trim("  a a "));
  BOOST_CHECK_EQUAL(std::string("a a"), trim("  a a  "));
} // testTrim

BOOST_AUTO_TEST_CASE(testSplitString) {

  std::vector<std::string> result;

  result = splitString("", ',');
  BOOST_CHECK_EQUAL((size_t)0, result.size());

  result = splitString("a", ',');
  BOOST_CHECK_EQUAL((size_t)1, result.size());

  result = splitString("ab", ',');
  BOOST_CHECK_EQUAL((size_t)1, result.size());

  result = splitString("a,b", ',');
  BOOST_CHECK_EQUAL((size_t)2, result.size());
  BOOST_CHECK_EQUAL(std::string("a"), result[0]);
  BOOST_CHECK_EQUAL(std::string("b"), result[1]);

  result = splitString("a,b,", ',');
  BOOST_CHECK_EQUAL((size_t)3, result.size());
  BOOST_CHECK_EQUAL(std::string("a"), result[0]);
  BOOST_CHECK_EQUAL(std::string("b"), result[1]);
  BOOST_CHECK_EQUAL(std::string(""), result[2]);

  result = splitString("a;,b\n", ',');
  BOOST_CHECK_EQUAL((size_t)2, result.size());
  BOOST_CHECK_EQUAL(std::string("a;"), result[0]);
  BOOST_CHECK_EQUAL(std::string("b\n"), result[1]);

  result = splitString("a, b", ',', true);
  BOOST_CHECK_EQUAL((size_t)2, result.size());
  BOOST_CHECK_EQUAL(std::string("a"), result[0]);
  BOOST_CHECK_EQUAL(std::string("b"), result[1]);
} // testSplitString

BOOST_AUTO_TEST_CASE(testJoinOneElement) {
  std::vector<std::string> vec;
  vec.push_back("test");
  BOOST_CHECK_EQUAL("test", join(vec, ","));
}

BOOST_AUTO_TEST_CASE(testJoinNoElement) {
  std::vector<std::string> vec;
  BOOST_CHECK_EQUAL("", join(vec, ","));
}

BOOST_AUTO_TEST_CASE(testJoinMoreElements) {
  std::vector<std::string> vec;
  vec.push_back("test");
  vec.push_back("test2");
  vec.push_back("test3");
  BOOST_CHECK_EQUAL("test,test2,test3", join(vec, ","));
}

BOOST_AUTO_TEST_CASE(testProperties) {
  Properties props;
  BOOST_CHECK(!props.has("key"));
  props.set("key", "value");
  BOOST_CHECK(props.has("key"));
  BOOST_CHECK_EQUAL("value", props.get("key"));
  
  BOOST_CHECK_THROW(props.get("nonexisting"), std::runtime_error);
  BOOST_CHECK_EQUAL("default", props.get("nonexisting", "default"));
  
  BOOST_CHECK_EQUAL(true, props.unset("key"));
  BOOST_CHECK(!props.has("key"));
  
  BOOST_CHECK_EQUAL(false, props.unset("key"));
}

BOOST_AUTO_TEST_CASE(testISODate) {
  char c = 'C';
  struct tm inst;
  time_t now;
  time( &now );
  localtime_r(&now, &inst);
  mktime(&inst);

  std::string asString = dateToISOString<std::string>(&inst);
  struct tm parsed = dateFromISOString(asString.c_str());
  parsed.tm_zone = &c;

  DateTime instObj(inst);
  DateTime parsedObj(parsed);
#ifdef VERBOSE_TESTS
  cout << "original: " << instObj << " parsed: " << parsedObj << endl;
#endif
  BOOST_CHECK_EQUAL(instObj, parsedObj);
} // testISODate

BOOST_AUTO_TEST_SUITE_END()
