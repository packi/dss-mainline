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

  SeriesTest() {}
protected:

  void testTimestamps() {
    Series<CurrentValue> five(5 * 60, 10);
    Series<CurrentValue> minutely(60, 10, &five);
    Series<CurrentValue> secondly2(2, 10, &minutely);

    DateTime testStart;

    secondly2.addValue(1, testStart);
    secondly2.addValue(2, testStart.addSeconds(1));
    secondly2.addValue(2, testStart.addSeconds(2));

    CPPUNIT_ASSERT_EQUAL((size_t)2, secondly2.getValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, secondly2.getValues().front().getValue());

    CPPUNIT_ASSERT_EQUAL(testStart.addSeconds(2), secondly2.getValues().front().getTimeStamp());

    // skip one value
    secondly2.addValue(3, testStart.addSeconds(5));

    CPPUNIT_ASSERT_EQUAL((size_t)4, secondly2.getValues().size());
    CPPUNIT_ASSERT_EQUAL(3.0, secondly2.getValues().front().getValue());

    CPPUNIT_ASSERT_EQUAL(testStart.addSeconds(5), secondly2.getValues().front().getTimeStamp());
  } // testTimestamps

  void testReadWrite() {
    Series<CurrentValue> five(5 * 60, 10);
    Series<CurrentValue> minutely(60, 10, &five);
    Series<CurrentValue> secondly(1, 10, &minutely);

    DateTime testStart;

    secondly.addValue(1, testStart);
    secondly.addValue(2, testStart.addSeconds(1));

    CPPUNIT_ASSERT_EQUAL((size_t)2, secondly.getValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, secondly.getValues().front().getValue());

    secondly.addValue(3, testStart.addSeconds(1));
    secondly.addValue(4, testStart.addSeconds(1));
    secondly.addValue(5, testStart.addSeconds(1));

    CPPUNIT_ASSERT_EQUAL((size_t)2, secondly.getValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, secondly.getValues().front().getMin());
    CPPUNIT_ASSERT_EQUAL(5.0, secondly.getValues().front().getValue());
    CPPUNIT_ASSERT_EQUAL(5.0, secondly.getValues().front().getMax());

    SeriesWriter<CurrentValue> writer;
    writer.writeToXML(secondly, "/home/patrick/workspace/dss/data/webroot/test.xml");

    SeriesReader<CurrentValue> reader;

    Series<CurrentValue>* series = reader.readFromXML("/home/patrick/workspace/dss/data/webroot/test.xml");
    CPPUNIT_ASSERT(series != NULL);
    CPPUNIT_ASSERT_EQUAL((size_t)2, series->getValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, series->getValues().front().getMin());
    CPPUNIT_ASSERT_EQUAL(5.0, series->getValues().front().getValue());
    CPPUNIT_ASSERT_EQUAL(5.0, series->getValues().front().getMax());

    series->setComment("my comment");
    series->setUnit("kW");
    writer.writeToXML(*series, "/home/patrick/workspace/dss/data/webroot/test.xml");
    delete series;

    series = reader.readFromXML("/home/patrick/workspace/dss/data/webroot/test.xml");
    CPPUNIT_ASSERT(series != NULL);
    CPPUNIT_ASSERT_EQUAL((size_t)2, series->getValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, series->getValues().front().getMin());
    CPPUNIT_ASSERT_EQUAL(5.0, series->getValues().front().getValue());
    CPPUNIT_ASSERT_EQUAL(5.0, series->getValues().front().getMax());
    CPPUNIT_ASSERT_EQUAL(string("my comment"), series->getComment());
    CPPUNIT_ASSERT_EQUAL(string("kW"), series->getUnit());


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

    secondly2.addValue(values[0], startTime.addSeconds(delay[0]));
    SeriesWriter<CurrentValue> writer;
    writer.writeToXML(secondly2, "/home/patrick/workspace/dss/data/webroot/test_2seconds.xml");
    writer.writeToXML(minutely, "/home/patrick/workspace/dss/data/webroot/test_minutely.xml");
    writer.writeToXML(five, "/home/patrick/workspace/dss/data/webroot/test_five_minutely.xml");

    size_t lastNumValsSeconds = secondly2.getValues().size();
    size_t lastNumValsMinutely = minutely.getValues().size();
    size_t lastNumValsFive = five.getValues().size();

    SeriesReader<CurrentValue> reader;
    DateTime lastTimeStamp = startTime;

    for(int iVal = 1; iVal < numValues; iVal++) {
      int curDelay = delay[iVal];
      int curValue = values[iVal];
      int diff = curDelay - lastDelay;
#ifdef VERBOSE_TESTS
      cout << "\ndiff: " << diff << "\ncurDelay: " << curDelay << endl;
#endif
      boost::shared_ptr<Series<CurrentValue> > pFive(reader.readFromXML("/home/patrick/workspace/dss/data/webroot/test_five_minutely.xml"));
      boost::shared_ptr<Series<CurrentValue> > pMinutely(reader.readFromXML("/home/patrick/workspace/dss/data/webroot/test_minutely.xml"));
      boost::shared_ptr<Series<CurrentValue> > pSecondly2(reader.readFromXML("/home/patrick/workspace/dss/data/webroot/test_2seconds.xml"));

#ifdef VERBOSE_TESTS
      cout << "secondly last: " << lastNumValsSeconds << " current: " << pSecondly2->getValues().size() << endl;
#endif
      CPPUNIT_ASSERT_EQUAL(lastNumValsSeconds, pSecondly2->getValues().size());
      CPPUNIT_ASSERT_EQUAL(lastNumValsMinutely, pMinutely->getValues().size());
      CPPUNIT_ASSERT_EQUAL(lastNumValsFive, pFive->getValues().size());
#ifdef VERBOSE_TESTS
      cout << "last timestamp: " << lastTimeStamp << " current: " << pSecondly2->getValues().front().getTimeStamp() << endl;
#endif
      CPPUNIT_ASSERT_EQUAL(lastTimeStamp, pSecondly2->getValues().front().getTimeStamp());

      pMinutely->setNextSeries(pFive.get());
      pSecondly2->setNextSeries(pMinutely.get());

      pSecondly2->addValue(curValue, startTime.addSeconds(curDelay));

      if(diff > 2) {
#ifdef VERBOSE_TESTS
        int numNewVals = (diff + 1) / 2;
        cout << "diff       : " << diff << endl;
        cout << "numNewVals : " << numNewVals << endl;
        cout << "lastValSec : " << lastNumValsSeconds << endl;
        cout << "current    : " << pSecondly2->getValues().size() << endl;
        //CPPUNIT_ASSERT_EQUAL(lastNumValsSeconds + numNewVals, pSecondly)
#endif
      }

      lastNumValsSeconds = pSecondly2->getValues().size();
      lastNumValsMinutely = pMinutely->getValues().size();
      lastNumValsFive = pFive->getValues().size();
      lastTimeStamp = pSecondly2->getValues().front().getTimeStamp();

      writer.writeToXML(*pSecondly2, "/home/patrick/workspace/dss/data/webroot/test_2seconds.xml");
      writer.writeToXML(*pMinutely, "/home/patrick/workspace/dss/data/webroot/test_minutely.xml");
      writer.writeToXML(*pFive, "/home/patrick/workspace/dss/data/webroot/test_five_minutely.xml");
    }
  } // testReadWriteExtended

  void testRealtime() {
    Series<CurrentValue> five(5 * 60, 10);
    Series<CurrentValue> minutely(60, 10, &five);
    Series<CurrentValue> secondly(1, 10, &minutely);

    DateTime testStart;

    secondly.addValue(1, DateTime());

    sleepSeconds(1);

    secondly.addValue(2, DateTime());

    CPPUNIT_ASSERT_EQUAL((size_t)2, secondly.getValues().size());

    CPPUNIT_ASSERT_EQUAL(2.0, secondly.getValues().front().getValue());

    secondly.addValue(3, DateTime());
    secondly.addValue(4, DateTime());
    secondly.addValue(5, DateTime());

    CPPUNIT_ASSERT_EQUAL((size_t)2, secondly.getValues().size());
    CPPUNIT_ASSERT_EQUAL(2.0, secondly.getValues().front().getMin());
    CPPUNIT_ASSERT_EQUAL(5.0, secondly.getValues().front().getValue());
    CPPUNIT_ASSERT_EQUAL(5.0, secondly.getValues().front().getMax());
  }

  void testWrapping() {
    Series<AdderValue> hourly (3600, 10);
    Series<AdderValue> five (60*5, 10, &hourly);
    Series<AdderValue> minutely(60, 10, &five);

    DateTime testStart;

    // add values 1..60 to minutely (60 values)
    for(int iValue = 1; iValue <= 60; iValue++) {
      minutely.addValue(iValue, testStart.addMinute(iValue));
    }

    const std::deque<AdderValue>& minuteValues = minutely.getValues();
    CPPUNIT_ASSERT_EQUAL((size_t)10, minuteValues.size());

    // values 60 - 51 should still be present in minutely
    for(std::deque<AdderValue>::const_iterator iValue = minuteValues.begin(), e = minuteValues.end();
        iValue != e; ++iValue)
    {
      CPPUNIT_ASSERT_EQUAL(60.0 - distance(minuteValues.begin(), iValue), iValue->getValue());
    }

    const std::deque<AdderValue>& fiveminuteValues = five.getValues();
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
      CPPUNIT_ASSERT_EQUAL((double)fiveMinuteValuesExpected[distance(fiveminuteValues.begin(), iValue)], iValue->getValue());
    }

    CPPUNIT_ASSERT_EQUAL((size_t)0, hourly.getValues().size());

    minutely.addValue(61, testStart.addMinute(61));

    cout << hourly.getValues().size() << " " << hourly.getValues().front().getValue() << endl;

    CPPUNIT_ASSERT_EQUAL((size_t)1, hourly.getValues().size());

    double val = SeriesAdder<5,5>::value;
    CPPUNIT_ASSERT_EQUAL(val, hourly.getValues().front().getValue());

    minutely.addValue(62, testStart.addMinute(62));

    SeriesWriter<AdderValue> writer;
    writer.writeToXML(minutely, "/home/patrick/workspace/dss/data/webroot/minutely.xml");

    CPPUNIT_ASSERT_EQUAL((size_t)1, hourly.getValues().size());
  } // testWrapping

};

CPPUNIT_TEST_SUITE_REGISTRATION(SeriesTest);
