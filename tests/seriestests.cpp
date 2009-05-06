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
  CPPUNIT_TEST(testReadWriteExtended);
  CPPUNIT_TEST(testTimestamps);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {}
protected:

  void testTimestamps() {
    Series<CurrentValue> five(5 * 60, 10);
    Series<CurrentValue> minutely(60, 10, &five);
    Series<CurrentValue> secondly2(2, 10, &minutely);

    DateTime testStart;

    secondly2.AddValue(1, testStart);
    secondly2.AddValue(2, testStart.AddSeconds(1));
    secondly2.AddValue(2, testStart.AddSeconds(2));

    CPPUNIT_ASSERT_EQUAL((size_t)2, secondly2.GetValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, secondly2.GetValues().front().GetValue());

    CPPUNIT_ASSERT_EQUAL(testStart.AddSeconds(2), secondly2.GetValues().front().GetTimeStamp());

    // skip one value
    secondly2.AddValue(3, testStart.AddSeconds(5));

    CPPUNIT_ASSERT_EQUAL((size_t)4, secondly2.GetValues().size());
    CPPUNIT_ASSERT_EQUAL(3.0, secondly2.GetValues().front().GetValue());

    CPPUNIT_ASSERT_EQUAL(testStart.AddSeconds(5), secondly2.GetValues().front().GetTimeStamp());
  } // testTimestamps

  void testReadWrite() {
    Series<CurrentValue> five(5 * 60, 10);
    Series<CurrentValue> minutely(60, 10, &five);
    Series<CurrentValue> secondly(1, 10, &minutely);

    DateTime testStart;

    secondly.AddValue(1, testStart);
    secondly.AddValue(2, testStart.AddSeconds(1));

    CPPUNIT_ASSERT_EQUAL((size_t)2, secondly.GetValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, secondly.GetValues().front().GetValue());

    secondly.AddValue(3, testStart.AddSeconds(1));
    secondly.AddValue(4, testStart.AddSeconds(1));
    secondly.AddValue(5, testStart.AddSeconds(1));

    CPPUNIT_ASSERT_EQUAL((size_t)2, secondly.GetValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, secondly.GetValues().front().GetMin());
    CPPUNIT_ASSERT_EQUAL(5.0, secondly.GetValues().front().GetValue());
    CPPUNIT_ASSERT_EQUAL(5.0, secondly.GetValues().front().GetMax());

    SeriesWriter<CurrentValue> writer;
    writer.WriteToXML(secondly, "/home/patrick/workspace/dss/data/webroot/test.xml");

    SeriesReader<CurrentValue> reader;

    Series<CurrentValue>* series = reader.ReadFromXML("/home/patrick/workspace/dss/data/webroot/test.xml");
    CPPUNIT_ASSERT(series != NULL);
    CPPUNIT_ASSERT_EQUAL((size_t)2, series->GetValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, series->GetValues().front().GetMin());
    CPPUNIT_ASSERT_EQUAL(5.0, series->GetValues().front().GetValue());
    CPPUNIT_ASSERT_EQUAL(5.0, series->GetValues().front().GetMax());

    delete series;
  } // testReadWrite

  void testReadWriteExtended() {
    int delay[]   =  { 0, 1, 2, 4, 5, 7, 8, 120, 160 };
    int values[]  =  { 1, 2, 3, 4, 5, 6, 7,  8,  9 };
    int lastDelay = 0;
    int numValues = sizeof(delay)/sizeof(int);


    DateTime startTime;

    Series<CurrentValue> five(5 * 60, 10);
    Series<CurrentValue> minutely(60, 10, &five);
    Series<CurrentValue> secondly2(2, 10, &minutely);

    secondly2.AddValue(values[0], startTime.AddSeconds(delay[0]));
    SeriesWriter<CurrentValue> writer;
    writer.WriteToXML(secondly2, "/home/patrick/workspace/dss/data/webroot/test_2seconds.xml");
    writer.WriteToXML(minutely, "/home/patrick/workspace/dss/data/webroot/test_minutely.xml");
    writer.WriteToXML(five, "/home/patrick/workspace/dss/data/webroot/test_five_minutely.xml");

    size_t lastNumValsSeconds = secondly2.GetValues().size();
    size_t lastNumValsMinutely = minutely.GetValues().size();
    size_t lastNumValsFive = five.GetValues().size();

    SeriesReader<CurrentValue> reader;
    DateTime lastTimeStamp = startTime;

    for(int iVal = 1; iVal < numValues; iVal++) {
      int curDelay = delay[iVal];
      int curValue = values[iVal];
      int diff = curDelay - lastDelay;
      cout << "\ndiff: " << diff << "\ncurDelay: " << curDelay << endl;

      boost::shared_ptr<Series<CurrentValue> > pFive(reader.ReadFromXML("/home/patrick/workspace/dss/data/webroot/test_five_minutely.xml"));
      boost::shared_ptr<Series<CurrentValue> > pMinutely(reader.ReadFromXML("/home/patrick/workspace/dss/data/webroot/test_minutely.xml"));
      boost::shared_ptr<Series<CurrentValue> > pSecondly2(reader.ReadFromXML("/home/patrick/workspace/dss/data/webroot/test_2seconds.xml"));

      cout << "secondly last: " << lastNumValsSeconds << " current: " << pSecondly2->GetValues().size() << endl;
      CPPUNIT_ASSERT_EQUAL(lastNumValsSeconds, pSecondly2->GetValues().size());
      CPPUNIT_ASSERT_EQUAL(lastNumValsMinutely, pMinutely->GetValues().size());
      CPPUNIT_ASSERT_EQUAL(lastNumValsFive, pFive->GetValues().size());
      cout << "last timestamp: " << lastTimeStamp << " current: " << pSecondly2->GetValues().front().GetTimeStamp() << endl;
      CPPUNIT_ASSERT_EQUAL(lastTimeStamp, pSecondly2->GetValues().front().GetTimeStamp());

      pMinutely->SetNextSeries(pFive.get());
      pSecondly2->SetNextSeries(pMinutely.get());

      pSecondly2->AddValue(curValue, startTime.AddSeconds(curDelay));

      if(diff > 2) {
        int numNewVals = (diff + 1) / 2;
        cout << "diff       : " << diff << endl;
        cout << "numNewVals : " << numNewVals << endl;
        cout << "lastValSec : " << lastNumValsSeconds << endl;
        cout << "current    : " << pSecondly2->GetValues().size() << endl;
        //CPPUNIT_ASSERT_EQUAL(lastNumValsSeconds + numNewVals, pSecondly)
      }

      lastNumValsSeconds = pSecondly2->GetValues().size();
      lastNumValsMinutely = pMinutely->GetValues().size();
      lastNumValsFive = pFive->GetValues().size();
      lastTimeStamp = pSecondly2->GetValues().front().GetTimeStamp();

      writer.WriteToXML(*pSecondly2, "/home/patrick/workspace/dss/data/webroot/test_2seconds.xml");
      writer.WriteToXML(*pMinutely, "/home/patrick/workspace/dss/data/webroot/test_minutely.xml");
      writer.WriteToXML(*pFive, "/home/patrick/workspace/dss/data/webroot/test_five_minutely.xml");
    }
  } // testReadWriteExtended

  void testRealtime() {
    Series<CurrentValue> five(5 * 60, 10);
    Series<CurrentValue> minutely(60, 10, &five);
    Series<CurrentValue> secondly(1, 10, &minutely);

    DateTime testStart;

    secondly.AddValue(1, DateTime());

    SleepSeconds(1);

    secondly.AddValue(2, DateTime());

    CPPUNIT_ASSERT_EQUAL((size_t)2, secondly.GetValues().size());

    CPPUNIT_ASSERT_EQUAL(2.0, secondly.GetValues().front().GetValue());

    secondly.AddValue(3, DateTime());
    secondly.AddValue(4, DateTime());
    secondly.AddValue(5, DateTime());

    CPPUNIT_ASSERT_EQUAL((size_t)2, secondly.GetValues().size());
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
    CPPUNIT_ASSERT_EQUAL((size_t)10, minuteValues.size());

    // values 60 - 51 should still be present in minutely
    for(std::deque<AdderValue>::const_iterator iValue = minuteValues.begin(), e = minuteValues.end();
        iValue != e; ++iValue)
    {
      CPPUNIT_ASSERT_EQUAL(60.0 - distance(minuteValues.begin(), iValue), iValue->GetValue());
    }

    const std::deque<AdderValue>& fiveminuteValues = five.GetValues();
    CPPUNIT_ASSERT_EQUAL((size_t)10, fiveminuteValues.size());

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

    CPPUNIT_ASSERT_EQUAL((size_t)0, hourly.GetValues().size());

    minutely.AddValue(61, testStart.AddMinute(61));

    cout << hourly.GetValues().size() << " " << hourly.GetValues().front().GetValue() << endl;

    CPPUNIT_ASSERT_EQUAL((size_t)1, hourly.GetValues().size());

    double val = SeriesAdder<5,5>::value;
    CPPUNIT_ASSERT_EQUAL(val, hourly.GetValues().front().GetValue());

    minutely.AddValue(62, testStart.AddMinute(62));

    SeriesWriter<AdderValue> writer;
    writer.WriteToXML(minutely, "/home/patrick/workspace/dss/data/webroot/minutely.xml");

    CPPUNIT_ASSERT_EQUAL((size_t)1, hourly.GetValues().size());
  } // testWrapping

};

CPPUNIT_TEST_SUITE_REGISTRATION(SeriesTest);
