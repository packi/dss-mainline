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

// calculates N + (N-1) + (N-2) from N to N-limit
// we could use the following formula but using MTP is way cooler ;-)
// N * (N-1)/2 - (N-limit)*(N-limit - 1)/2
template <int N, int limit>
struct SeriesAdder
{
  enum { value = N + SeriesAdder< (limit > 1) ? N - 1 : 0, (limit > 1) ? limit - 1 : 0>::value };
};

template <>
struct SeriesAdder<0, 0>
{
  enum { value = 0 };
};

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

    // add values 1..60 to minutely (60 values)
    for(int iValue = 1; iValue <= 60; iValue++) {
      minutely.AddValue(iValue, testStart.AddMinute(iValue));
    }

    const std::deque<AdderValue>& minuteValues = minutely.GetValues();
    CPPUNIT_ASSERT_EQUAL(10u, minuteValues.size());

    // values 60 - 51 should still be present in minutely
    for(std::deque<AdderValue>::const_iterator iValue = minuteValues.begin(), e = minuteValues.end();
        iValue != e; ++iValue)
    {
      CPPUNIT_ASSERT_EQUAL(60.0 - distance(minuteValues.begin(), iValue), iValue->GetValue());
    }

    const std::deque<AdderValue>& fiveminuteValues = five.GetValues();
    CPPUNIT_ASSERT_EQUAL(10u, fiveminuteValues.size());

    vector<int> fiveMinuteValuesExpected;
    fiveMinuteValuesExpected.push_back(SeriesAdder<50,5>::value);
    fiveMinuteValuesExpected.push_back(SeriesAdder<45,5>::value);
    fiveMinuteValuesExpected.push_back(SeriesAdder<40,5>::value);
    fiveMinuteValuesExpected.push_back(SeriesAdder<35,5>::value);
    fiveMinuteValuesExpected.push_back(SeriesAdder<30,5>::value);
    fiveMinuteValuesExpected.push_back(SeriesAdder<25,5>::value);
    fiveMinuteValuesExpected.push_back(SeriesAdder<20,5>::value);
    fiveMinuteValuesExpected.push_back(SeriesAdder<15,5>::value);
    fiveMinuteValuesExpected.push_back(SeriesAdder<10,5>::value);
    fiveMinuteValuesExpected.push_back(SeriesAdder< 5,5>::value);

    // values 50 - 1 should still be present in minutely
    for(std::deque<AdderValue>::const_iterator iValue = fiveminuteValues.begin(), e = fiveminuteValues.end();
        iValue != e; ++iValue)
    {
      CPPUNIT_ASSERT_EQUAL((double)fiveMinuteValuesExpected[distance(fiveminuteValues.begin(), iValue)], iValue->GetValue());
    }

    CPPUNIT_ASSERT_EQUAL(0u, hourly.GetValues().size());

    minutely.AddValue(61, testStart.AddMinute(61));

    CPPUNIT_ASSERT_EQUAL(1u, hourly.GetValues().size());

    double val = SeriesAdder<5,5>::value;
    CPPUNIT_ASSERT_EQUAL(val, hourly.GetValues().front().GetValue());

    minutely.AddValue(62, testStart.AddMinute(62));

    CPPUNIT_ASSERT_EQUAL(1u, hourly.GetValues().size());
  } // testWrapping

};

CPPUNIT_TEST_SUITE_REGISTRATION(SeriesTest);
