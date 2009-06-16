/*
 *  basetests.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 5/19/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>


#include "../core/base.h"
#include "../core/datetools.h"

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

using namespace dss;

class BaseTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(BaseTest);
  CPPUNIT_TEST(testCRC);
  CPPUNIT_TEST(testurlDecode);
  CPPUNIT_TEST(testConversions);
  CPPUNIT_TEST(testtrim);
  CPPUNIT_TEST(testsplitString);
  CPPUNIT_TEST(testISODate);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {}

protected:

  void testCRC() {
    const char testStr[] = "123456789";

    uint16_t crc = crc16((unsigned const char*)&testStr, sizeof(testStr)-1);
    CPPUNIT_ASSERT_EQUAL((uint16_t)0x2189, crc);


    const char testStr2[] = "\xfd\x01\x03\x14\xbb\x01\x00\x00";//\xeb\x08";
    crc = crc16((unsigned const char*)&testStr2, sizeof(testStr2)-1);
    CPPUNIT_ASSERT_EQUAL((uint16_t)0x08eb, crc);
  }

  void testurlDecode(void) {
    CPPUNIT_ASSERT_EQUAL(string(" "), urlDecode("%20"));
    CPPUNIT_ASSERT_EQUAL(string("a "), urlDecode("a%20"));
    CPPUNIT_ASSERT_EQUAL(string(" a"), urlDecode("%20a"));
    CPPUNIT_ASSERT_EQUAL(string("v a"), urlDecode("v%20a"));
    CPPUNIT_ASSERT_EQUAL(string("  "), urlDecode("%20%20"));
    CPPUNIT_ASSERT_EQUAL(string("   "), urlDecode("%20%20%20"));

    CPPUNIT_ASSERT_EQUAL(string(" "), urlDecode("+"));
    CPPUNIT_ASSERT_EQUAL(string("  "), urlDecode("++"));
    CPPUNIT_ASSERT_EQUAL(string(" a "), urlDecode("+a+"));
    CPPUNIT_ASSERT_EQUAL(string("  a"), urlDecode("++a"));
    CPPUNIT_ASSERT_EQUAL(string("a  "), urlDecode("a++"));
    CPPUNIT_ASSERT_EQUAL(string("  b"), urlDecode("+%20b"));
    CPPUNIT_ASSERT_EQUAL(string("sourceid=1&schedule=FREQ=MINUTELY;INTERVAL=1&start=20080520T080000Z"),
                         urlDecode("sourceid=1&schedule=FREQ%3DMINUTELY%3BINTERVAL%3D1&start=20080520T080000Z"));
  }

  void testConversions() {
    CPPUNIT_ASSERT_EQUAL(1, strToInt("1"));
    CPPUNIT_ASSERT_EQUAL(1, strToInt("0x1"));
    CPPUNIT_ASSERT_EQUAL(10, strToInt("0xa"));

    CPPUNIT_ASSERT_EQUAL(-1, strToIntDef("", -1));
    CPPUNIT_ASSERT_EQUAL(-1, strToIntDef(" ", -1));
    CPPUNIT_ASSERT_EQUAL(-1, strToIntDef("gfdfg", -1));
    CPPUNIT_ASSERT_EQUAL(1, strToIntDef("1", -1));
  } // testConversions

  void testtrim() {
    // reductions to zero-string
    CPPUNIT_ASSERT_EQUAL(string(""), trim(""));
    CPPUNIT_ASSERT_EQUAL(string(""), trim(" "));
    CPPUNIT_ASSERT_EQUAL(string(""), trim("  "));
    CPPUNIT_ASSERT_EQUAL(string(""), trim("     "));
    CPPUNIT_ASSERT_EQUAL(string(""), trim("\r"));
    CPPUNIT_ASSERT_EQUAL(string(""), trim(" \r"));
    CPPUNIT_ASSERT_EQUAL(string(""), trim(" \r "));
    CPPUNIT_ASSERT_EQUAL(string(""), trim("\n"));
    CPPUNIT_ASSERT_EQUAL(string(""), trim(" \n"));
    CPPUNIT_ASSERT_EQUAL(string(""), trim(" \n "));
    CPPUNIT_ASSERT_EQUAL(string(""), trim("\t"));
    CPPUNIT_ASSERT_EQUAL(string(""), trim(" \t "));

    // reductions to single character
    CPPUNIT_ASSERT_EQUAL(string("a"), trim(" a "));
    CPPUNIT_ASSERT_EQUAL(string("a"), trim("  a "));
    CPPUNIT_ASSERT_EQUAL(string("a"), trim("  a  "));

    // noop
    CPPUNIT_ASSERT_EQUAL(string("a a"), trim("a a"));
    CPPUNIT_ASSERT_EQUAL(string("a\ra"), trim("a\ra"));
    CPPUNIT_ASSERT_EQUAL(string("a\ta"), trim("a\ta"));
    CPPUNIT_ASSERT_EQUAL(string("a\na"), trim("a\na"));
    CPPUNIT_ASSERT_EQUAL(string("a  a"), trim("a  a"));

    // reductions to multiple character & no-op
    CPPUNIT_ASSERT_EQUAL(string("a a"), trim(" a a "));
    CPPUNIT_ASSERT_EQUAL(string("a a"), trim("  a a "));
    CPPUNIT_ASSERT_EQUAL(string("a a"), trim("  a a  "));
  } // testtrim

  void testsplitString() {

    vector<string> result;

    result = splitString("", ',');
    CPPUNIT_ASSERT_EQUAL((size_t)0, result.size());

    result = splitString("a", ',');
    CPPUNIT_ASSERT_EQUAL((size_t)1, result.size());

    result = splitString("ab", ',');
    CPPUNIT_ASSERT_EQUAL((size_t)1, result.size());

    result = splitString("a,b", ',');
    CPPUNIT_ASSERT_EQUAL((size_t)2, result.size());
    CPPUNIT_ASSERT_EQUAL(string("a"), result[0]);
    CPPUNIT_ASSERT_EQUAL(string("b"), result[1]);

    result = splitString("a,b,", ',');
    CPPUNIT_ASSERT_EQUAL((size_t)3, result.size());
    CPPUNIT_ASSERT_EQUAL(string("a"), result[0]);
    CPPUNIT_ASSERT_EQUAL(string("b"), result[1]);
    CPPUNIT_ASSERT_EQUAL(string(""), result[2]);

    result = splitString("a;,b\n", ',');
    CPPUNIT_ASSERT_EQUAL((size_t)2, result.size());
    CPPUNIT_ASSERT_EQUAL(string("a;"), result[0]);
    CPPUNIT_ASSERT_EQUAL(string("b\n"), result[1]);

    result = splitString("a, b", ',', true);
    CPPUNIT_ASSERT_EQUAL((size_t)2, result.size());
    CPPUNIT_ASSERT_EQUAL(string("a"), result[0]);
    CPPUNIT_ASSERT_EQUAL(string("b"), result[1]);

  } // testsplitString

  void testISODate() {
    char c = 'C';
    struct tm inst;
    time_t now;
    time( &now );
    localtime_r(&now, &inst);
    mktime(&inst);

    string asString = dateToISOString<string>(&inst);
    struct tm parsed = dateFromISOString(asString.c_str());
    parsed.tm_zone = &c;

    DateTime instObj(inst);
    DateTime parsedObj(parsed);
#ifdef VERBOSE_TESTS
    cout << "original: " << instObj << " parsed: " << parsedObj << endl;
#endif
    CPPUNIT_ASSERT_EQUAL(instObj, parsedObj);
  } // testISODate

};

CPPUNIT_TEST_SUITE_REGISTRATION(BaseTest);

