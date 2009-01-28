/*
 * seriestests.cpp
 *
 *  Created on: Jan 28, 2009
 *      Author: patrick
 */

#include <cassert>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include "core/metering/series.cpp"

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

using namespace dss;

class SeriesTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(SeriesTest);
  CPPUNIT_TEST(testWrapping);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {}
protected:

  void testWrapping() {
    Series<AdderValue> hourly (3600, 10);
    Series<AdderValue> five (60*5, 10, &hourly);
    Series<AdderValue> minutely(60, 10, &five);

    DateTime testStart;

    for(int iValue = 0; iValue < 60; iValue++) {
      minutely.AddValue(iValue, testStart.AddMinute(iValue));
    }
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION(SeriesTest);
