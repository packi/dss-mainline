#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Series
#include <boost/test/unit_test.hpp>

#include "core/metering/series.h"
#include "core/metering/seriespersistence.h"

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

BOOST_AUTO_TEST_CASE(timestamps) {
//   Series<CurrentValue> five(5 * 60, 10);
//   Series<CurrentValue> minutely(60, 10, &five);
  Series<CurrentValue> secondly2(2, 10);
  
  DateTime now;
  DateTime testStart(static_cast<time_t>(now.secondsSinceEpoch() - 
        now.secondsSinceEpoch() % 2));
  
  secondly2.addValue(1, testStart);
  secondly2.addValue(2, testStart.addSeconds(1));
  secondly2.addValue(2, testStart.addSeconds(2));
  
  BOOST_CHECK_EQUAL( (size_t)2, secondly2.getValues().size() );
  BOOST_CHECK_EQUAL( 2.0, secondly2.getValues().front().getValue() );
  
  BOOST_CHECK_EQUAL( testStart.addSeconds(2), secondly2.getValues().front().getTimeStamp() );
  
  // skip one value
  secondly2.addValue(3, testStart.addSeconds(5));
  
  BOOST_CHECK_EQUAL( (size_t)3, secondly2.getValues().size() );
  BOOST_CHECK_EQUAL( 3.0, secondly2.getValues().front().getValue() );
  
  BOOST_CHECK_EQUAL( testStart.addSeconds(4), secondly2.getValues().front().getTimeStamp() );
} // testTimestamps

BOOST_AUTO_TEST_CASE(readWrite) {
  Series<CurrentValue> five(5 * 60, 10);
  Series<CurrentValue> minutely(60, 10, &five);
  Series<CurrentValue> secondly(1, 10, &minutely);
  
  DateTime testStart;
  
  secondly.addValue(1, testStart);
  secondly.addValue(2, testStart.addSeconds(1));
  
  BOOST_CHECK_EQUAL( (size_t)2, secondly.getValues().size() );
  BOOST_CHECK_EQUAL( 2.0, secondly.getValues().front().getValue() );
  
  secondly.addValue(3, testStart.addSeconds(1));
  secondly.addValue(4, testStart.addSeconds(1));
  secondly.addValue(5, testStart.addSeconds(1));
  
  BOOST_CHECK_EQUAL( (size_t)2, secondly.getValues().size() );
  BOOST_CHECK_EQUAL( 2.0, secondly.getValues().front().getMin() );
  BOOST_CHECK_EQUAL( 5.0, secondly.getValues().front().getValue() );
  BOOST_CHECK_EQUAL( 5.0, secondly.getValues().front().getMax() );
  
  SeriesWriter<CurrentValue> writer;
  writer.writeToXML(secondly, "/Users/pisco/Desktop/dss/tmptest.xml");
  
  SeriesReader<CurrentValue> reader;
  
  Series<CurrentValue>* series = reader.readFromXML("/Users/pisco/Desktop/dss/tmptest.xml");
  BOOST_REQUIRE_MESSAGE( series != NULL, "reading series from file" );
  BOOST_CHECK_EQUAL( (size_t)2, series->getValues().size() );
  BOOST_CHECK_EQUAL( 2.0, series->getValues().front().getMin() );
  BOOST_CHECK_EQUAL( 5.0, series->getValues().front().getValue() );
  BOOST_CHECK_EQUAL( 5.0, series->getValues().front().getMax() );
  
  series->setComment("my comment");
  series->setUnit("kW");
  writer.writeToXML(*series, "/Users/pisco/Desktop/dss/tmptest.xml");
  delete series;
  
  series = reader.readFromXML("/Users/pisco/Desktop/dss/tmptest.xml");
  BOOST_REQUIRE_MESSAGE( series != NULL, "reading series from file" );
  BOOST_CHECK_EQUAL( (size_t)2, series->getValues().size() );
  BOOST_CHECK_EQUAL( 2.0, series->getValues().front().getMin() );
  BOOST_CHECK_EQUAL( 5.0, series->getValues().front().getValue() );
  BOOST_CHECK_EQUAL( 5.0, series->getValues().front().getMax() );
  BOOST_CHECK_EQUAL( string("my comment"), series->getComment() );
  BOOST_CHECK_EQUAL( string("kW"), series->getUnit() );
  
  
  delete series;
} // testReadWrite

BOOST_AUTO_TEST_CASE(readWriteExtended) {
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
  writer.writeToXML(secondly2, "/Users/pisco/Desktop/dss/tmptest_2seconds.xml");
  writer.writeToXML(minutely, "/Users/pisco/Desktop/dss/tmptest_minutely.xml");
  writer.writeToXML(five, "/Users/pisco/Desktop/dss/tmptest_five_minutely.xml");

  size_t lastNumValsSeconds = secondly2.getValues().size();
  size_t lastNumValsMinutely = minutely.getValues().size();
  size_t lastNumValsFive = five.getValues().size();

  SeriesReader<CurrentValue> reader;
  DateTime lastTimeStamp = startTime;

  for(int iVal = 1; iVal < numValues; iVal++) {
    int curDelay = delay[iVal];
    int curValue = values[iVal];
    int diff = curDelay - lastDelay;
    
    BOOST_TEST_MESSAGE("diff: " << diff << "\ncurDelay: " << curDelay);
    
    boost::shared_ptr<Series<CurrentValue> > pFive(reader.readFromXML("/Users/pisco/Desktop/dss/tmptest_five_minutely.xml"));
    boost::shared_ptr<Series<CurrentValue> > pMinutely(reader.readFromXML("/Users/pisco/Desktop/dss/tmptest_minutely.xml"));
    boost::shared_ptr<Series<CurrentValue> > pSecondly2(reader.readFromXML("/Users/pisco/Desktop/dss/tmptest_2seconds.xml"));

    
    BOOST_TEST_MESSAGE("secondly last: " << lastNumValsSeconds << " current: " << pSecondly2->getValues().size());
    
    BOOST_CHECK_EQUAL( lastNumValsSeconds, pSecondly2->getValues().size() );
    BOOST_CHECK_EQUAL( lastNumValsMinutely, pMinutely->getValues().size() );
    BOOST_CHECK_EQUAL( lastNumValsFive, pFive->getValues().size() );
    
    BOOST_TEST_MESSAGE("last timestamp: " << lastTimeStamp << " current: " << pSecondly2->getValues().front().getTimeStamp());
    
    BOOST_CHECK_EQUAL( lastTimeStamp, pSecondly2->getValues().front().getTimeStamp() );

    pMinutely->setNextSeries(pFive.get());
    pSecondly2->setNextSeries(pMinutely.get());

    pSecondly2->addValue(curValue, startTime.addSeconds(curDelay));

    if(diff > 2) {
      int numNewVals = (diff + 1) / 2;
      BOOST_TEST_MESSAGE("diff       : " << diff);
      BOOST_TEST_MESSAGE("numNewVals : " << numNewVals);
      BOOST_TEST_MESSAGE("lastValSec : " << lastNumValsSeconds);
      BOOST_TEST_MESSAGE("current    : " << pSecondly2->getValues().size());
      //BOOST_CHECK(lastNumValsSeconds + numNewVals, pSecondly)
    }

    lastNumValsSeconds = pSecondly2->getValues().size();
    lastNumValsMinutely = pMinutely->getValues().size();
    lastNumValsFive = pFive->getValues().size();
    lastTimeStamp = pSecondly2->getValues().front().getTimeStamp();

    writer.writeToXML(*pSecondly2, "/Users/pisco/Desktop/dss/tmptest_2seconds.xml");
    writer.writeToXML(*pMinutely, "/Users/pisco/Desktop/dss/tmptest_minutely.xml");
    writer.writeToXML(*pFive, "/Users/pisco/Desktop/dss/tmptest_five_minutely.xml");
  }
} // testReadWriteExtended

BOOST_AUTO_TEST_CASE(realtime) {
  Series<CurrentValue> five(5 * 60, 10);
  Series<CurrentValue> minutely(60, 10, &five);
  Series<CurrentValue> secondly(1, 10, &minutely);

  DateTime testStart;

  secondly.addValue(1, DateTime());

  sleepSeconds(1);

  secondly.addValue(2, DateTime());

  BOOST_CHECK_EQUAL( (size_t)2, secondly.getValues().size() );

  BOOST_CHECK_EQUAL( 2.0, secondly.getValues().front().getValue() );

  secondly.addValue(3, DateTime());
  secondly.addValue(4, DateTime());
  secondly.addValue(5, DateTime());

  BOOST_CHECK_EQUAL( (size_t)2, secondly.getValues().size() );
  BOOST_CHECK_EQUAL( 2.0, secondly.getValues().front().getMin() );
  BOOST_CHECK_EQUAL( 5.0, secondly.getValues().front().getValue() );
  BOOST_CHECK_EQUAL( 5.0, secondly.getValues().front().getMax() );
}

BOOST_AUTO_TEST_CASE(wrapping) {
  Series<AdderValue> hourly (3600, 10);
  Series<AdderValue> five (60*5, 10, &hourly);
  Series<AdderValue> minutely(60, 10, &five);

  DateTime now;
  DateTime testStart(static_cast<time_t>(now.secondsSinceEpoch() - 
        now.secondsSinceEpoch() % 3600));

  // add values 1..60 to minutely (60 values)
  for(int iValue = 1; iValue <= 60; iValue++) {
    minutely.addValue(iValue, testStart.addMinute(iValue));
  }

  const std::deque<AdderValue>& minuteValues = minutely.getValues();
  BOOST_CHECK_EQUAL( (size_t)10, minuteValues.size() );

  // values 60 - 51 should still be present in minutely
  for(std::deque<AdderValue>::const_iterator iValue = minuteValues.begin(), e = minuteValues.end();
      iValue != e; ++iValue)
  {
    BOOST_CHECK_EQUAL( (60.0 - distance(minuteValues.begin(), iValue)), iValue->getValue() );
  }

  const std::deque<AdderValue>& fiveminuteValues = five.getValues();
  BOOST_CHECK_EQUAL( (size_t)10, fiveminuteValues.size() );

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
    BOOST_CHECK_EQUAL( (double)fiveMinuteValuesExpected[distance(fiveminuteValues.begin(), iValue)], iValue->getValue() );
  }

  BOOST_CHECK_EQUAL( (size_t)0, hourly.getValues().size() );

  minutely.addValue(61, testStart.addMinute(61));

  BOOST_TEST_MESSAGE(hourly.getValues().size() << " " << hourly.getValues().front().getValue());

  BOOST_CHECK_EQUAL( (size_t)1, hourly.getValues().size() );

  double val = SeriesAdder<5,5>::value;
  BOOST_CHECK_EQUAL( val, hourly.getValues().front().getValue() );

  minutely.addValue(62, testStart.addMinute(62));

  SeriesWriter<AdderValue> writer;
  writer.writeToXML(minutely, "/Users/pisco/Desktop/dss/tmpminutely.xml");

  BOOST_CHECK_EQUAL( (size_t)1, hourly.getValues().size() );
} // testWrapping