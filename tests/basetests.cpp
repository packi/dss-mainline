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
  BOOST_CHECK_EQUAL(string(" "), urlDecode("%20"));
  BOOST_CHECK_EQUAL(string("a "), urlDecode("a%20"));
  BOOST_CHECK_EQUAL(string(" a"), urlDecode("%20a"));
  BOOST_CHECK_EQUAL(string("v a"), urlDecode("v%20a"));
  BOOST_CHECK_EQUAL(string("  "), urlDecode("%20%20"));
  BOOST_CHECK_EQUAL(string("   "), urlDecode("%20%20%20"));

  BOOST_CHECK_EQUAL(string(" "), urlDecode("+"));
  BOOST_CHECK_EQUAL(string("  "), urlDecode("++"));
  BOOST_CHECK_EQUAL(string(" a "), urlDecode("+a+"));
  BOOST_CHECK_EQUAL(string("  a"), urlDecode("++a"));
  BOOST_CHECK_EQUAL(string("a  "), urlDecode("a++"));
  BOOST_CHECK_EQUAL(string("  b"), urlDecode("+%20b"));
  BOOST_CHECK_EQUAL(string("sourceid=1&schedule=FREQ=MINUTELY;INTERVAL=1&start=20080520T080000Z"),
                       urlDecode("sourceid=1&schedule=FREQ%3DMINUTELY%3BINTERVAL%3D1&start=20080520T080000Z"));
} // teturlDecode

BOOST_AUTO_TEST_CASE(testConversions) {
  BOOST_CHECK_EQUAL(1, strToInt("1"));
  BOOST_CHECK_EQUAL(1, strToInt("0x1"));
  BOOST_CHECK_EQUAL(10, strToInt("0xa"));

  BOOST_CHECK_EQUAL(-1, strToIntDef("", -1));
  BOOST_CHECK_EQUAL(-1, strToIntDef(" ", -1));
  BOOST_CHECK_EQUAL(-1, strToIntDef("gfdfg", -1));
  BOOST_CHECK_EQUAL(1, strToIntDef("1", -1));
} // testConversions

BOOST_AUTO_TEST_CASE(testTrim) {
  // reductions to zero-string
  BOOST_CHECK_EQUAL(string(""), trim(""));
  BOOST_CHECK_EQUAL(string(""), trim(" "));
  BOOST_CHECK_EQUAL(string(""), trim("  "));
  BOOST_CHECK_EQUAL(string(""), trim("     "));
  BOOST_CHECK_EQUAL(string(""), trim("\r"));
  BOOST_CHECK_EQUAL(string(""), trim(" \r"));
  BOOST_CHECK_EQUAL(string(""), trim(" \r "));
  BOOST_CHECK_EQUAL(string(""), trim("\n"));
  BOOST_CHECK_EQUAL(string(""), trim(" \n"));
  BOOST_CHECK_EQUAL(string(""), trim(" \n "));
  BOOST_CHECK_EQUAL(string(""), trim("\t"));
  BOOST_CHECK_EQUAL(string(""), trim(" \t "));

  // reductions to single character
  BOOST_CHECK_EQUAL(string("a"), trim(" a "));
  BOOST_CHECK_EQUAL(string("a"), trim("  a "));
  BOOST_CHECK_EQUAL(string("a"), trim("  a  "));

  // noop
  BOOST_CHECK_EQUAL(string("a a"), trim("a a"));
  BOOST_CHECK_EQUAL(string("a\ra"), trim("a\ra"));
  BOOST_CHECK_EQUAL(string("a\ta"), trim("a\ta"));
  BOOST_CHECK_EQUAL(string("a\na"), trim("a\na"));
  BOOST_CHECK_EQUAL(string("a  a"), trim("a  a"));

  // reductions to multiple character & no-op
  BOOST_CHECK_EQUAL(string("a a"), trim(" a a "));
  BOOST_CHECK_EQUAL(string("a a"), trim("  a a "));
  BOOST_CHECK_EQUAL(string("a a"), trim("  a a  "));
} // testTrim

BOOST_AUTO_TEST_CASE(testSplitString) {

  vector<string> result;

  result = splitString("", ',');
  BOOST_CHECK_EQUAL((size_t)0, result.size());

  result = splitString("a", ',');
  BOOST_CHECK_EQUAL((size_t)1, result.size());

  result = splitString("ab", ',');
  BOOST_CHECK_EQUAL((size_t)1, result.size());

  result = splitString("a,b", ',');
  BOOST_CHECK_EQUAL((size_t)2, result.size());
  BOOST_CHECK_EQUAL(string("a"), result[0]);
  BOOST_CHECK_EQUAL(string("b"), result[1]);

  result = splitString("a,b,", ',');
  BOOST_CHECK_EQUAL((size_t)3, result.size());
  BOOST_CHECK_EQUAL(string("a"), result[0]);
  BOOST_CHECK_EQUAL(string("b"), result[1]);
  BOOST_CHECK_EQUAL(string(""), result[2]);

  result = splitString("a;,b\n", ',');
  BOOST_CHECK_EQUAL((size_t)2, result.size());
  BOOST_CHECK_EQUAL(string("a;"), result[0]);
  BOOST_CHECK_EQUAL(string("b\n"), result[1]);

  result = splitString("a, b", ',', true);
  BOOST_CHECK_EQUAL((size_t)2, result.size());
  BOOST_CHECK_EQUAL(string("a"), result[0]);
  BOOST_CHECK_EQUAL(string("b"), result[1]);

} // testSplitString

BOOST_AUTO_TEST_CASE(testISODate) {
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
  BOOST_CHECK_EQUAL(instObj, parsedObj);
} // testISODate

BOOST_AUTO_TEST_SUITE_END()
