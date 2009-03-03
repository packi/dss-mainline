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

#include "core/metering/series.h"
#include "core/metering/seriespersistence.h"

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
  CPPUNIT_TEST(testRealtime);
  CPPUNIT_TEST(testReadWrite);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {}
protected:

  void testReadWrite() {
    Series<CurrentValue> five(5 * 60, 10);
    Series<CurrentValue> minutely(60, 10, &five);
    Series<CurrentValue> secondly(1, 10, &minutely);


    DateTime testStart;

    secondly.AddValue(1, testStart);
    secondly.AddValue(2, testStart.AddSeconds(1));

    CPPUNIT_ASSERT_EQUAL(2u, secondly.GetValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, secondly.GetValues().front().GetValue());

    secondly.AddValue(3, testStart.AddSeconds(1));
    secondly.AddValue(4, testStart.AddSeconds(1));
    secondly.AddValue(5, testStart.AddSeconds(1));

    CPPUNIT_ASSERT_EQUAL(2u, secondly.GetValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, secondly.GetValues().front().GetMin());
    CPPUNIT_ASSERT_EQUAL(5.0, secondly.GetValues().front().GetValue());
    CPPUNIT_ASSERT_EQUAL(5.0, secondly.GetValues().front().GetMax());

    SeriesWriter<CurrentValue> writer;
    writer.WriteToXML(secondly, "/home/patrick/workspace/dss/data/webroot/test.xml");

    SeriesReader<CurrentValue> reader;

    Series<CurrentValue>* series = reader.ReadFromXML("/home/patrick/workspace/dss/data/webroot/test.xml");
    CPPUNIT_ASSERT(series != NULL);
    CPPUNIT_ASSERT_EQUAL(2u, series->GetValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, series->GetValues().front().GetMin());
    CPPUNIT_ASSERT_EQUAL(5.0, series->GetValues().front().GetValue());
    CPPUNIT_ASSERT_EQUAL(5.0, series->GetValues().front().GetMax());

    delete series;
  } // testReadWrite

  void testRealtime() {
    Series<CurrentValue> five(5 * 60, 10);
    Series<CurrentValue> minutely(60, 10, &five);
    Series<CurrentValue> secondly(1, 10, &minutely);

    DateTime testStart;

    secondly.AddValue(1, DateTime());

    SleepSeconds(1);

    secondly.AddValue(2, DateTime());

    CPPUNIT_ASSERT_EQUAL(2u, secondly.GetValues().size());

    CPPUNIT_ASSERT_EQUAL(2.0, secondly.GetValues().front().GetValue());

    secondly.AddValue(3, DateTime());
    secondly.AddValue(4, DateTime());
    secondly.AddValue(5, DateTime());

    CPPUNIT_ASSERT_EQUAL(2u, secondly.GetValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, secondly.GetValues().front().GetMin());
    CPPUNIT_ASSERT_EQUAL(5.0, secondly.GetValues().front().GetValue());
    CPPUNIT_ASSERT_EQUAL(5.0, secondly.GetValues().front().GetMax());
  }

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

    cout << hourly.GetValues().size() << " " << hourly.GetValues().front().GetValue() << endl;

    CPPUNIT_ASSERT_EQUAL(1u, hourly.GetValues().size());

    double val = SeriesAdder<5,5>::value;
    CPPUNIT_ASSERT_EQUAL(val, hourly.GetValues().front().GetValue());

    minutely.AddValue(62, testStart.AddMinute(62));

    SeriesWriter<AdderValue> writer;
    writer.WriteToXML(minutely, "/home/patrick/workspace/dss/data/webroot/minutely.xml");

    CPPUNIT_ASSERT_EQUAL(1u, hourly.GetValues().size());
  } // testWrapping

};

CPPUNIT_TEST_SUITE_REGISTRATION(SeriesTest);
