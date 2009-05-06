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
  CPPUNIT_TEST(testURLDecode);
  CPPUNIT_TEST(testConversions);
  CPPUNIT_TEST(testTrim);
  CPPUNIT_TEST(testSplitString);
  CPPUNIT_TEST(testISODate);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {}

protected:

  void testCRC() {
    const char testStr[] = "123456789";

    uint16_t crc = CRC16((unsigned const char*)&testStr, sizeof(testStr)-1);
    CPPUNIT_ASSERT_EQUAL((uint16_t)0x2189, crc);


    const char testStr2[] = "\xfd\x01\x03\x14\xbb\x01\x00\x00";//\xeb\x08";
    crc = CRC16((unsigned const char*)&testStr2, sizeof(testStr2)-1);
    CPPUNIT_ASSERT_EQUAL((uint16_t)0x08eb, crc);
  }

  void testURLDecode(void) {
    CPPUNIT_ASSERT_EQUAL(string(" "), URLDecode("%20"));
    CPPUNIT_ASSERT_EQUAL(string("a "), URLDecode("a%20"));
    CPPUNIT_ASSERT_EQUAL(string(" a"), URLDecode("%20a"));
    CPPUNIT_ASSERT_EQUAL(string("v a"), URLDecode("v%20a"));
    CPPUNIT_ASSERT_EQUAL(string("  "), URLDecode("%20%20"));
    CPPUNIT_ASSERT_EQUAL(string("   "), URLDecode("%20%20%20"));

    CPPUNIT_ASSERT_EQUAL(string(" "), URLDecode("+"));
    CPPUNIT_ASSERT_EQUAL(string("  "), URLDecode("++"));
    CPPUNIT_ASSERT_EQUAL(string(" a "), URLDecode("+a+"));
    CPPUNIT_ASSERT_EQUAL(string("  a"), URLDecode("++a"));
    CPPUNIT_ASSERT_EQUAL(string("a  "), URLDecode("a++"));
    CPPUNIT_ASSERT_EQUAL(string("  b"), URLDecode("+%20b"));
    CPPUNIT_ASSERT_EQUAL(string("sourceid=1&schedule=FREQ=MINUTELY;INTERVAL=1&start=20080520T080000Z"),
                         URLDecode("sourceid=1&schedule=FREQ%3DMINUTELY%3BINTERVAL%3D1&start=20080520T080000Z"));
  }

  void testConversions() {
    CPPUNIT_ASSERT_EQUAL(1, StrToInt("1"));
    CPPUNIT_ASSERT_EQUAL(1, StrToInt("0x1"));
    CPPUNIT_ASSERT_EQUAL(10, StrToInt("0xa"));

    CPPUNIT_ASSERT_EQUAL(-1, StrToIntDef("", -1));
    CPPUNIT_ASSERT_EQUAL(-1, StrToIntDef(" ", -1));
    CPPUNIT_ASSERT_EQUAL(-1, StrToIntDef("gfdfg", -1));
    CPPUNIT_ASSERT_EQUAL(1, StrToIntDef("1", -1));
  } // testConversions

  void testTrim() {
    // reductions to zero-string
    CPPUNIT_ASSERT_EQUAL(string(""), Trim(""));
    CPPUNIT_ASSERT_EQUAL(string(""), Trim(" "));
    CPPUNIT_ASSERT_EQUAL(string(""), Trim("  "));
    CPPUNIT_ASSERT_EQUAL(string(""), Trim("     "));
    CPPUNIT_ASSERT_EQUAL(string(""), Trim("\r"));
    CPPUNIT_ASSERT_EQUAL(string(""), Trim(" \r"));
    CPPUNIT_ASSERT_EQUAL(string(""), Trim(" \r "));
    CPPUNIT_ASSERT_EQUAL(string(""), Trim("\n"));
    CPPUNIT_ASSERT_EQUAL(string(""), Trim(" \n"));
    CPPUNIT_ASSERT_EQUAL(string(""), Trim(" \n "));
    CPPUNIT_ASSERT_EQUAL(string(""), Trim("\t"));
    CPPUNIT_ASSERT_EQUAL(string(""), Trim(" \t "));

    // reductions to single character
    CPPUNIT_ASSERT_EQUAL(string("a"), Trim(" a "));
    CPPUNIT_ASSERT_EQUAL(string("a"), Trim("  a "));
    CPPUNIT_ASSERT_EQUAL(string("a"), Trim("  a  "));

    // noop
    CPPUNIT_ASSERT_EQUAL(string("a a"), Trim("a a"));
    CPPUNIT_ASSERT_EQUAL(string("a\ra"), Trim("a\ra"));
    CPPUNIT_ASSERT_EQUAL(string("a\ta"), Trim("a\ta"));
    CPPUNIT_ASSERT_EQUAL(string("a\na"), Trim("a\na"));
    CPPUNIT_ASSERT_EQUAL(string("a  a"), Trim("a  a"));

    // reductions to multiple character & no-op
    CPPUNIT_ASSERT_EQUAL(string("a a"), Trim(" a a "));
    CPPUNIT_ASSERT_EQUAL(string("a a"), Trim("  a a "));
    CPPUNIT_ASSERT_EQUAL(string("a a"), Trim("  a a  "));
  } // testTrim

  void testSplitString() {

    vector<string> result;

    result = SplitString("", ',');
    CPPUNIT_ASSERT_EQUAL((size_t)0, result.size());

    result = SplitString("a", ',');
    CPPUNIT_ASSERT_EQUAL((size_t)1, result.size());

    result = SplitString("ab", ',');
    CPPUNIT_ASSERT_EQUAL((size_t)1, result.size());

    result = SplitString("a,b", ',');
    CPPUNIT_ASSERT_EQUAL((size_t)2, result.size());
    CPPUNIT_ASSERT_EQUAL(string("a"), result[0]);
    CPPUNIT_ASSERT_EQUAL(string("b"), result[1]);

    result = SplitString("a,b,", ',');
    CPPUNIT_ASSERT_EQUAL((size_t)3, result.size());
    CPPUNIT_ASSERT_EQUAL(string("a"), result[0]);
    CPPUNIT_ASSERT_EQUAL(string("b"), result[1]);
    CPPUNIT_ASSERT_EQUAL(string(""), result[2]);

    result = SplitString("a;,b\n", ',');
    CPPUNIT_ASSERT_EQUAL((size_t)2, result.size());
    CPPUNIT_ASSERT_EQUAL(string("a;"), result[0]);
    CPPUNIT_ASSERT_EQUAL(string("b\n"), result[1]);

    result = SplitString("a, b", ',', true);
    CPPUNIT_ASSERT_EQUAL((size_t)2, result.size());
    CPPUNIT_ASSERT_EQUAL(string("a"), result[0]);
    CPPUNIT_ASSERT_EQUAL(string("b"), result[1]);

  } // testSplitString

  void testISODate() {
    char c = 'C';
    struct tm inst;
    time_t now;
    time( &now );
    localtime_r(&now, &inst);
    mktime(&inst);

    string asString = DateToISOString<string>(&inst);
    struct tm parsed = DateFromISOString(asString.c_str());
    parsed.tm_zone = &c;

    DateTime instObj(inst);
    DateTime parsedObj(parsed);

    cout << "original: " << instObj << " parsed: " << parsedObj << endl;
    CPPUNIT_ASSERT_EQUAL(instObj, parsedObj);
  } // testISODate

};

CPPUNIT_TEST_SUITE_REGISTRATION(BaseTest);

