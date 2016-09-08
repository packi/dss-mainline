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

#include <boost/tuple/tuple.hpp>
#include <limits>

#include "src/base.h"
#include "src/datetools.h"
#include "src/model/data_types.h"
#include "src/util.h"

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

BOOST_AUTO_TEST_CASE(testUrlEncodeDecode) {
  const char escape[] = "!#$%&'()*+,/:;=?@[]";
  const char url_encoded[] = "%21%23%24%25%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D";
  BOOST_CHECK(dss::urlEncode(escape) == url_encoded);
  BOOST_CHECK(dss::urlDecode(url_encoded) == escape);
}

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
  BOOST_CHECK_EQUAL(10, strToIntDef("0xa", -1));
  BOOST_CHECK_EQUAL(-1, strToIntDef("0xqq", -1));
} // testStrToIntDef

BOOST_AUTO_TEST_CASE(testStrToUInt) {
  BOOST_CHECK_EQUAL(7, int(strToUInt("7")));

  BOOST_CHECK_THROW(strToUInt("asdf"), std::invalid_argument);
  BOOST_CHECK_THROW(strToUInt(""), std::invalid_argument);
  BOOST_CHECK_THROW(strToUInt(" "), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(testStrToUIntDef) {
  BOOST_CHECK_EQUAL(5, strToUIntDef("", 5));
  BOOST_CHECK_EQUAL(5, strToUIntDef(" ", 5));
  BOOST_CHECK_EQUAL(5, strToUIntDef("gfdfg", 5));
  BOOST_CHECK_EQUAL(1, strToUIntDef("1", 5));
} // testStrToUIntDef


BOOST_AUTO_TEST_CASE(testStrToDouble) {
  BOOST_CHECK_EQUAL(0.0, strToDouble("0.0"));
  BOOST_CHECK_EQUAL(-1.0, strToDouble("asdf", -1.0));
  BOOST_CHECK_EQUAL(1.0, strToDouble("1.00", -1.0));
  BOOST_CHECK_THROW(strToDouble("asdf"), std::invalid_argument);
} // testStrToDouble

BOOST_AUTO_TEST_CASE(testUintToString) {
  BOOST_CHECK_EQUAL("0", uintToString(0));
  BOOST_CHECK_EQUAL("2222", uintToString(2222));
  intToString(std::numeric_limits<long long int>::max());
  intToString(std::numeric_limits<long long int>::min());
  uintToString(std::numeric_limits<long long unsigned>::max());
  uintToString(std::numeric_limits<long long unsigned>::min());
} // testUintToString

BOOST_AUTO_TEST_CASE(testUnsignedLongIntToHexString) {
  BOOST_CHECK_EQUAL("42", unsignedLongIntToHexString(0x42));
} // testUnsignedLongIntToHexString

BOOST_AUTO_TEST_CASE(testHexEncodeByteArray) {
  /* leading zeros must not be ignored */
  unsigned char t[] = {0, };
  BOOST_CHECK_EQUAL("00", hexEncodeByteArray(t, 1));
  unsigned char t2[] = {0, 0, 0, 0, 0, 0, 0, 0 };
  BOOST_CHECK_EQUAL("0000000000000000", hexEncodeByteArray(t2, sizeof(t2)));
  std::vector<unsigned char> t3;

  /* verify every char gets mapped to hex values */
  for (int i = 0; i <= 255; i++) {
    t3.push_back(i);
  }
  BOOST_CHECK(hexEncodeByteArray(t3).find_first_not_of("1234567890abcdef") ==
              std::string::npos);
}

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

  result = splitString("a,", ',');
  BOOST_CHECK_EQUAL((size_t)2, result.size());
  BOOST_CHECK_EQUAL(std::string("a"), result[0]);
  BOOST_CHECK_EQUAL(std::string(""), result[1]);

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

  result = splitString(",a", ',', true);
  BOOST_CHECK_EQUAL((size_t)2, result.size());
  BOOST_CHECK_EQUAL(std::string(""), result[0]);
  BOOST_CHECK_EQUAL(std::string("a"), result[1]);

  result = splitString("a\\, b", ',');
  BOOST_CHECK_EQUAL((size_t)1, result.size());
  BOOST_CHECK_EQUAL(std::string("a, b"), result[0]);

  result = splitString("a\\,,b", ',', true);
  BOOST_CHECK_EQUAL((size_t)2, result.size());
  BOOST_CHECK_EQUAL(std::string("a,"), result[0]);
  BOOST_CHECK_EQUAL(std::string("b"), result[1]);

  result = splitString("\\,,\\,", ',');
  BOOST_CHECK_EQUAL((size_t)2, result.size());
  BOOST_CHECK_EQUAL(std::string(","), result[0]);
  BOOST_CHECK_EQUAL(std::string(","), result[1]);

  result = splitString("\\,", ',');
  BOOST_CHECK_EQUAL((size_t)1, result.size());
  BOOST_CHECK_EQUAL(std::string(","), result[0]);

  // TODO, here the user probably wants to escape the '\' not the ',' delimiter,
  // hence result should contain 2 elements, [ "\\", "" ]
  result = splitString("\\\\,", ',');
  BOOST_CHECK_EQUAL((size_t)1, result.size());
  BOOST_CHECK_EQUAL(std::string("\\,"), result[0]);
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

BOOST_AUTO_TEST_CASE(testSplitIntoKeyValue) {
  std::string key;
  std::string value;

  boost::tie(key, value) = splitIntoKeyValue("ab=cd");
  BOOST_CHECK_EQUAL(key, "ab");
  BOOST_CHECK_EQUAL(value, "cd");

  boost::tie(key, value) = splitIntoKeyValue("ef=");
  BOOST_CHECK_EQUAL(key, "ef");
  BOOST_CHECK_EQUAL(value, "");

  boost::tie(key, value) = splitIntoKeyValue("=gh");
  BOOST_CHECK_EQUAL(key, "");
  BOOST_CHECK_EQUAL(value, "gh");

  boost::tie(key, value) = splitIntoKeyValue("");
  BOOST_CHECK_EQUAL(key, "");
  BOOST_CHECK_EQUAL(value, "");

  boost::tie(key, value) = splitIntoKeyValue("test");
  BOOST_CHECK_EQUAL(key, "");
  BOOST_CHECK_EQUAL(value, "");

  boost::tie(key, value) = splitIntoKeyValue("test=equals=success");
  BOOST_CHECK_EQUAL(key, "test");
  BOOST_CHECK_EQUAL(value, "equals=success");
}

BOOST_AUTO_TEST_CASE(testCarCdrPath) {
  std::string path = "foo/bar/baz/baf/tic/tac/toe/";
  BOOST_CHECK_EQUAL(carCdrPath(path), "foo");
  BOOST_CHECK_EQUAL(carCdrPath(path), "bar");
  BOOST_CHECK_EQUAL(carCdrPath(path), "baz");
  BOOST_CHECK_EQUAL(carCdrPath(path), "baf");
  BOOST_CHECK_EQUAL(carCdrPath(path), "tic");
  BOOST_CHECK_EQUAL(carCdrPath(path), "tac");
  BOOST_CHECK_EQUAL(carCdrPath(path), "toe");
  BOOST_CHECK_EQUAL(carCdrPath(path), "");

  path = "";
  BOOST_CHECK_EQUAL(carCdrPath(path), "");

  path = "/";
  BOOST_CHECK_EQUAL(carCdrPath(path), "");

  path = "last";
  BOOST_CHECK_EQUAL(carCdrPath(path), "last");
  BOOST_CHECK_EQUAL(carCdrPath(path), "");

  path = "last/";
  BOOST_CHECK_EQUAL(carCdrPath(path), "last");
  BOOST_CHECK_EQUAL(carCdrPath(path), "");
}

BOOST_AUTO_TEST_CASE(testTruncateUTF8String) {
  BOOST_CHECK_EQUAL(truncateUTF8String("a", 2), "a");
  BOOST_CHECK_EQUAL(truncateUTF8String("a", 1), "a");
  BOOST_CHECK(truncateUTF8String("a", 0).empty());
  BOOST_CHECK(truncateUTF8String("", 0).empty());
  BOOST_CHECK(truncateUTF8String("", 1).empty());
  BOOST_CHECK(truncateUTF8String("", 2).empty());
  BOOST_CHECK_EQUAL(truncateUTF8String("ab", 1), "a");
  BOOST_CHECK_EQUAL(truncateUTF8String("ab", 2), "ab");
  // three-byte character
  BOOST_CHECK_EQUAL(truncateUTF8String("aaか", 3), "aa");
  BOOST_CHECK_EQUAL(truncateUTF8String("aaか", 4), "aa");
  BOOST_CHECK_EQUAL(truncateUTF8String("aaか", 5), "aaか");
  BOOST_CHECK_EQUAL(truncateUTF8String("aaか", 6), "aaか");
  // two-byte character
  BOOST_CHECK_EQUAL(truncateUTF8String("bbä", 2), "bb");
  BOOST_CHECK_EQUAL(truncateUTF8String("bbä", 3), "bb");
  BOOST_CHECK_EQUAL(truncateUTF8String("bbä", 4), "bbä");
}

BOOST_AUTO_TEST_CASE(testCardinalDirection) {
  std::string convert;
  CardinalDirection_t dir;

  BOOST_CHECK(parseCardinalDirection("none", &dir));
  BOOST_CHECK_EQUAL(dir, cd_none);
  BOOST_CHECK(parseCardinalDirection("north", &dir));
  BOOST_CHECK_EQUAL(dir, cd_north);
  BOOST_CHECK(parseCardinalDirection("north east", &dir));
  BOOST_CHECK_EQUAL(dir, cd_north_east);
  BOOST_CHECK(parseCardinalDirection("east", &dir));
  BOOST_CHECK_EQUAL(dir, cd_east);
  BOOST_CHECK(parseCardinalDirection("south east", &dir));
  BOOST_CHECK_EQUAL(dir, cd_south_east);
  BOOST_CHECK(parseCardinalDirection("south", &dir));
  BOOST_CHECK_EQUAL(dir, cd_south);
  BOOST_CHECK(parseCardinalDirection("south west", &dir));
  BOOST_CHECK_EQUAL(dir, cd_south_west);
  BOOST_CHECK(parseCardinalDirection("west", &dir));
  BOOST_CHECK_EQUAL(dir, cd_west);
  BOOST_CHECK(parseCardinalDirection("north west", &dir));
  BOOST_CHECK_EQUAL(dir, cd_north_west);

  BOOST_CHECK(!parseCardinalDirection("south south west", &dir));
  BOOST_CHECK(!parseCardinalDirection("north south east", &dir));
  BOOST_CHECK(!parseCardinalDirection("schlieren", &dir));

  BOOST_CHECK(valid(cd_north));
  BOOST_CHECK(valid(cd_north_east));
  BOOST_CHECK(valid(cd_north_west));
  BOOST_CHECK(valid(cd_south));
  BOOST_CHECK(valid(cd_south_east));
  BOOST_CHECK(valid(cd_south_west));
  BOOST_CHECK(valid(cd_east));
  BOOST_CHECK(valid(cd_west));
  BOOST_CHECK(!valid(cd_last));
  BOOST_CHECK(!valid(static_cast<CardinalDirection_t>(10)));

  BOOST_CHECK_EQUAL(toString(cd_north), "north");
  BOOST_CHECK_EQUAL(toString(cd_north_east), "north east");
  BOOST_CHECK_EQUAL(toString(cd_north_west), "north west");
  BOOST_CHECK_EQUAL(toString(cd_south), "south");
  BOOST_CHECK_EQUAL(toString(cd_south_east), "south east");
  BOOST_CHECK_EQUAL(toString(cd_south_west), "south west");
  BOOST_CHECK_EQUAL(toString(cd_east), "east");
  BOOST_CHECK_EQUAL(toString(cd_west), "west");
}

BOOST_AUTO_TEST_CASE(testWindProtection) {
  WindProtectionClass_t out;

  BOOST_CHECK(convertWindProtectionClass(wpc_awning_class_3, &out));
  BOOST_CHECK_EQUAL(out, wpc_awning_class_3);
  BOOST_CHECK(convertWindProtectionClass(wpc_awning_class_2, &out));
  BOOST_CHECK_EQUAL(out, wpc_awning_class_2);
  BOOST_CHECK(convertWindProtectionClass(wpc_awning_class_1, &out));
  BOOST_CHECK_EQUAL(out, wpc_awning_class_1);
  BOOST_CHECK(convertWindProtectionClass(wpc_blind_class_6, &out));
  BOOST_CHECK_EQUAL(out, wpc_blind_class_6);
  BOOST_CHECK(convertWindProtectionClass(wpc_blind_class_5, &out));
  BOOST_CHECK_EQUAL(out, wpc_blind_class_5);
  BOOST_CHECK(convertWindProtectionClass(wpc_blind_class_4, &out));
  BOOST_CHECK_EQUAL(out, wpc_blind_class_4);
  BOOST_CHECK(convertWindProtectionClass(wpc_blind_class_3, &out));
  BOOST_CHECK_EQUAL(out, wpc_blind_class_3);
  BOOST_CHECK(convertWindProtectionClass(wpc_blind_class_2, &out));
  BOOST_CHECK_EQUAL(out, wpc_blind_class_2);
  BOOST_CHECK(convertWindProtectionClass(wpc_blind_class_1, &out));
  BOOST_CHECK_EQUAL(out, wpc_blind_class_1);
  BOOST_CHECK(convertWindProtectionClass(wpc_none, &out));
  BOOST_CHECK_EQUAL(out, wpc_none);

  BOOST_CHECK(!convertWindProtectionClass(99, &out));
  BOOST_CHECK(!convertWindProtectionClass(-1, &out));

  BOOST_CHECK(valid(wpc_none));
  BOOST_CHECK(valid(wpc_blind_class_1));
  BOOST_CHECK(valid(wpc_blind_class_2));
  BOOST_CHECK(valid(wpc_blind_class_3));
  BOOST_CHECK(valid(wpc_blind_class_4));
  BOOST_CHECK(valid(wpc_blind_class_5));
  BOOST_CHECK(valid(wpc_blind_class_6));
  BOOST_CHECK(valid(wpc_awning_class_1));
  BOOST_CHECK(valid(wpc_awning_class_2));
  BOOST_CHECK(valid(wpc_awning_class_3));

  BOOST_CHECK(!valid(wpc_last));
  BOOST_CHECK(!valid(static_cast<WindProtectionClass_t>(99)));
  BOOST_CHECK(!valid(static_cast<WindProtectionClass_t>(-1)));
}

BOOST_AUTO_TEST_CASE(testParseBitfield) {
  uint8_t sceneLock[16] = {};
  sceneLock[0 / 8] |= (1 << (0 % 8));
  sceneLock[5 / 8] |= (1 << (5 % 8));
  sceneLock[17 / 8] |= (1 << (17 % 8));
  sceneLock[18 / 8] |= (1 << (18 % 8));
  sceneLock[19 / 8] |= (1 << (19 % 8));
  sceneLock[127 / 8] |= (1 << (127 % 8));

  std::vector<int> vec = parseBitfield(sceneLock, 128);
  BOOST_CHECK_EQUAL(vec.size(), 6);
  BOOST_CHECK_EQUAL(vec[0], 0);
  BOOST_CHECK_EQUAL(vec[1], 5);
  BOOST_CHECK_EQUAL(vec[2], 17);
  BOOST_CHECK_EQUAL(vec[3], 18);
  BOOST_CHECK_EQUAL(vec[4], 19);
  BOOST_CHECK_EQUAL(vec[5], 127);
}

BOOST_AUTO_TEST_CASE(testReadFile) {
    // tmpnam produces nasty link warning
    // hence create tmpfile then delete it
    char name[] = "/tmp/tmpfile.XXXXXX";
    int fd = mkstemp(name);
    std::string expect("a7d83559b6e2ea05ad646299e0a88c0c69174c99");
    BOOST_CHECK(write(fd, expect.c_str(), expect.size()) != -1);
    close(fd);

    // check content is read correctly
    std::string ret = readFile(name);
    BOOST_CHECK(ret == expect);

    // remove file, check that exception is thrown
    BOOST_CHECK(unlink(name) != -1);
    BOOST_CHECK_THROW(readFile(name), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()
