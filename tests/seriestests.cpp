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
#include <boost/filesystem.hpp>
#include <math.h>

#include "core/metering/series.h"
#include "core/metering/seriespersistence.h"

using namespace dss;


BOOST_AUTO_TEST_SUITE(SeriesTests)

// calculates N + (N-1) + (N-2) + ... from N to N-limit
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

BOOST_AUTO_TEST_CASE(insertions) {
  Series<CurrentValue> secondly2(2, 10);

  DateTime now;
  DateTime testStart(static_cast<time_t>(now.secondsSinceEpoch() -
        now.secondsSinceEpoch() % 2));

  // [0]
  secondly2.addValue(0, testStart.addSeconds(2*0));
  BOOST_CHECK_EQUAL( (size_t)1, secondly2.getValues().size() );
  BOOST_CHECK_EQUAL( 0.0, secondly2.getValues().front().getValue() );

  // [2,0]
  secondly2.addValue(2, testStart.addSeconds(2*2));
  BOOST_CHECK_EQUAL( (size_t)2, secondly2.getValues().size() );
  BOOST_CHECK_EQUAL( 2.0, secondly2.getValues().front().getValue() );

  // [4,2,0]
  secondly2.addValue(4, testStart.addSeconds(2*4));
  BOOST_CHECK_EQUAL( (size_t)3, secondly2.getValues().size() );
  BOOST_CHECK_EQUAL( 4.0, secondly2.getValues().front().getValue() );

  // [4,2,1,0]
  secondly2.addValue(1, testStart.addSeconds(2*1));
  BOOST_CHECK_EQUAL( (size_t)4, secondly2.getValues().size() );
  BOOST_CHECK_EQUAL( 1.0, secondly2.getValues()[2].getValue() );

  // [4,3,2,1,0]
  secondly2.addValue(3, testStart.addSeconds(2*3));
  BOOST_CHECK_EQUAL( (size_t)5, secondly2.getValues().size() );
  BOOST_CHECK_EQUAL( 3.0, secondly2.getValues()[1].getValue() );

  // [5,4,3,2,1,0]
  secondly2.addValue(5, testStart.addSeconds(2*5));
  BOOST_CHECK_EQUAL( (size_t)6, secondly2.getValues().size() );
  BOOST_CHECK_EQUAL( 5.0, secondly2.getValues().front().getValue() );

  // insert an elemet that should be merged with the front value
  secondly2.addValue(10, testStart.addSeconds(2*5));
  BOOST_CHECK_EQUAL( (size_t)6, secondly2.getValues().size() );
  BOOST_CHECK_EQUAL( 10.0, secondly2.getValues().front().getValue() );
  BOOST_CHECK_EQUAL( 5.0, secondly2.getValues().front().getMin() );

  // insert two element that should be merged with the third value
  secondly2.addValue(1, testStart.addSeconds(2*3));
  secondly2.addValue(20, testStart.addSeconds(2*3));
  secondly2.addValue(11, testStart.addSeconds(2*3));
  BOOST_CHECK_EQUAL( (size_t)6, secondly2.getValues().size() );
  BOOST_CHECK_EQUAL( 1.0, secondly2.getValues()[2].getMin() );
  BOOST_CHECK_EQUAL( 20.0, secondly2.getValues()[2].getMax() );
  BOOST_CHECK_EQUAL( 11.0, secondly2.getValues()[2].getValue() );
} // insertions

BOOST_AUTO_TEST_CASE(slidingWindow) {
  int resolution = 2;
  int queueLength = 10;
  Series<CurrentValue> secondly2(resolution, queueLength);

  DateTime now;
  DateTime testStart(static_cast<time_t>(now.secondsSinceEpoch() -
        now.secondsSinceEpoch() % resolution));

  for(int iVal = 0; iVal < 30; iVal++) {
    secondly2.addValue(iVal, testStart.addSeconds(resolution*iVal));
    if(iVal < queueLength) {
      BOOST_CHECK_EQUAL(iVal, secondly2.getValues().front().getValue());
      BOOST_CHECK_EQUAL(0, secondly2.getValues().back().getValue());
      BOOST_CHECK_EQUAL((size_t)iVal+1, secondly2.getValues().size());
    } else {
      BOOST_CHECK_EQUAL(iVal, secondly2.getValues().front().getValue());
      BOOST_CHECK_EQUAL(iVal-(queueLength-1), secondly2.getValues().back().getValue());
      BOOST_CHECK_EQUAL((size_t)queueLength, secondly2.getValues().size());
    }
  }
} // slidingWindow

BOOST_AUTO_TEST_CASE(sparcity) {
  int resolution = 2;
  int queueLength = 10;
  Series<CurrentValue> secondly2(resolution, queueLength);

  DateTime now;
  DateTime testStart(static_cast<time_t>(now.secondsSinceEpoch() -
        now.secondsSinceEpoch() % resolution));

  for(int iVal = 0; iVal < 30; iVal++) {
    secondly2.addValue(iVal, testStart.addSeconds(resolution*iVal*2));
    if(iVal < ceil(queueLength / 2)) {
      BOOST_CHECK_EQUAL(iVal, secondly2.getValues().front().getValue());
      BOOST_CHECK_EQUAL(0, secondly2.getValues().back().getValue());
      BOOST_CHECK_EQUAL((size_t)iVal+1, secondly2.getValues().size());
    } else {
      BOOST_CHECK_EQUAL(iVal, secondly2.getValues().front().getValue());
      BOOST_CHECK_EQUAL(iVal-(ceil(queueLength / 2)-1), secondly2.getValues().back().getValue());
      BOOST_CHECK_EQUAL((size_t)ceil(queueLength/2.0), secondly2.getValues().size());
    }
  }
} // sparcity

BOOST_AUTO_TEST_CASE(sparcity2) {
  int resolution = 2;
  int queueLength = 10;
  Series<CurrentValue> secondly2(resolution, queueLength);

  DateTime now;
  DateTime testStart(static_cast<time_t>(now.secondsSinceEpoch() -
        now.secondsSinceEpoch() % resolution - (4*queueLength*resolution)));

  for(int iVal = 0; iVal < resolution; iVal++) {
    secondly2.addValue(iVal, testStart.addSeconds(resolution*iVal));
  }

  now = now.addSeconds( -(now.secondsSinceEpoch() % resolution) );
  secondly2.addValue(0, now);

  BOOST_CHECK_EQUAL((size_t)1, secondly2.getValues().size());
} // sparcity2

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
  secondly.addValue(5, testStart.addSeconds(1));
  secondly.addValue(4, testStart.addSeconds(1));

  BOOST_CHECK_EQUAL( (size_t)2, secondly.getValues().size() );
  BOOST_CHECK_EQUAL( 2.0, secondly.getValues().front().getMin() );
  BOOST_CHECK_EQUAL( 4.0, secondly.getValues().front().getValue() );
  BOOST_CHECK_EQUAL( 5.0, secondly.getValues().front().getMax() );

  SeriesWriter<CurrentValue> writer;
  std::string fileName = getTempDir() + "/tmptest.xml";
  writer.writeToXML(secondly, fileName);

  SeriesReader<CurrentValue> reader;

  Series<CurrentValue>* series = reader.readFromXML(fileName);
  BOOST_REQUIRE_MESSAGE( series != NULL, "reading series from file" );
  BOOST_CHECK_EQUAL( (size_t)2, series->getValues().size() );
  BOOST_CHECK_EQUAL( 2.0, series->getValues().front().getMin() );
  BOOST_CHECK_EQUAL( 4.0, series->getValues().front().getValue() );
  BOOST_CHECK_EQUAL( 5.0, series->getValues().front().getMax() );

  series->setComment("my comment");
  series->setUnit("kW");
  writer.writeToXML(*series, fileName);
  delete series;

  series = reader.readFromXML(fileName);
  BOOST_REQUIRE_MESSAGE( series != NULL, "reading series from file" );
  BOOST_CHECK_EQUAL( (size_t)2, series->getValues().size() );
  BOOST_CHECK_EQUAL( 2.0, series->getValues().front().getMin() );
  BOOST_CHECK_EQUAL( 4.0, series->getValues().front().getValue() );
  BOOST_CHECK_EQUAL( 5.0, series->getValues().front().getMax() );
  BOOST_CHECK_EQUAL( std::string("my comment"), series->getComment() );
  BOOST_CHECK_EQUAL( std::string("kW"), series->getUnit() );

  boost::filesystem::remove_all(fileName);

  delete series;
} // testReadWrite

BOOST_AUTO_TEST_CASE(readWriteExtended) {
  int delay[]   =  { 0, 1, 2, 4, 5, 7, 8, 120, 160 };
  int values[]  =  { 1, 2, 3, 4, 5, 6, 7,  8,  9 };
  int lastDelay = 0;
  int numValues = sizeof(delay)/sizeof(int);


  DateTime now;
  DateTime startTime(static_cast<time_t>(now.secondsSinceEpoch() -
        now.secondsSinceEpoch() % (5*60)));

  Series<CurrentValue> five(5 * 60, 10);
  Series<CurrentValue> minutely(60, 10, &five);
  Series<CurrentValue> secondly2(2, 10, &minutely);

  secondly2.addValue(values[0], startTime.addSeconds(delay[0]));
  SeriesWriter<CurrentValue> writer;
  std::string secondsFile = getTempDir() + "/tmptest_2seconds.xml";
  writer.writeToXML(secondly2, secondsFile);
  std::string minutelyFile = getTempDir() + "/tmptest_minutely.xml";
  writer.writeToXML(minutely, minutelyFile);
  std::string fiveMinutelyFile = getTempDir() + "/tmptest_five_minutely.xml";
  writer.writeToXML(five, fiveMinutelyFile);

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

    boost::shared_ptr<Series<CurrentValue> > pFive(reader.readFromXML(fiveMinutelyFile));
    boost::shared_ptr<Series<CurrentValue> > pMinutely(reader.readFromXML(minutelyFile));
    boost::shared_ptr<Series<CurrentValue> > pSecondly2(reader.readFromXML(secondsFile));


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

    writer.writeToXML(*pSecondly2, secondsFile);
    writer.writeToXML(*pMinutely, minutelyFile);
    writer.writeToXML(*pFive, fiveMinutelyFile);
  }

  boost::filesystem::remove_all(secondsFile);
  boost::filesystem::remove_all(minutelyFile);
  boost::filesystem::remove_all(fiveMinutelyFile);
} // testReadWriteExtended

BOOST_AUTO_TEST_CASE(ImNotExactlySureWhatItestAtThisMoment) {
  /* we build a serie of N values where each value equals its time offset
   * (value of time0 + 4 * day = 4)
   */
  const unsigned int N = 400;
  const unsigned int SECONDS_PER_DAY = 86400;
  DateTime startT;

  /* build serie */
  Series<CurrentValue> daily(SECONDS_PER_DAY, N);
  BOOST_CHECK_EQUAL( static_cast<int>(SECONDS_PER_DAY), daily.getResolution() );
  BOOST_CHECK_EQUAL( static_cast<int>(N), daily.getNumberOfValues() );
  for (unsigned int i = 0; i < N; i++) {
    DateTime ts = startT.addSeconds(i * SECONDS_PER_DAY);
    daily.addValue(i, ts);
    const DateTime& insertedDate = daily.getValues().front().getTimeStamp();
    /* because the timestamp contains 22:20 hours only the date will be equal.
       The time must be equal the buckets boundary (00:00:00) */
    BOOST_CHECK_EQUAL( ts.getYear(), insertedDate.getYear() );
    BOOST_CHECK_EQUAL( ts.getMonth(), insertedDate.getMonth() );
    BOOST_CHECK_EQUAL( ts.getDay(), insertedDate.getDay() );
    BOOST_CHECK_EQUAL( 0, insertedDate.getHour() );
    BOOST_CHECK_EQUAL( 0, insertedDate.getMinute() );
    BOOST_CHECK_EQUAL( 0, insertedDate.getSecond() );
  }

  typedef Series<CurrentValue>::QueueType queue_type_;

  /* test values */
  queue_type_ queue(daily.getValues());
  BOOST_CHECK_EQUAL( N, queue.size() );
unsigned int numberOfElements = 0;
  for (queue_type_::const_iterator iter = queue.begin();
       iter != queue.end();
       iter++)
  {
    ++numberOfElements;
    double v = iter->getValue();
    BOOST_CHECK_EQUAL( (N - numberOfElements), static_cast<unsigned int>(v) );
    DateTime ts = startT.addSeconds((N - numberOfElements) * SECONDS_PER_DAY);
    BOOST_CHECK_EQUAL( ts.getYear(), iter->getTimeStamp().getYear() );
    BOOST_CHECK_EQUAL( ts.getMonth(), iter->getTimeStamp().getMonth() );
    BOOST_CHECK_EQUAL( ts.getDay(), iter->getTimeStamp().getDay() );
  }
  BOOST_CHECK_EQUAL( numberOfElements, N );

  /* test expanded values */
  boost::shared_ptr<queue_type_> queueExpanded = daily.getExpandedValues();
  numberOfElements = 0;
  for (queue_type_::const_iterator iter = queueExpanded->begin();
       iter != queueExpanded->end();
       iter++)
  {
    ++numberOfElements;
    double v = iter->getValue();
    BOOST_CHECK_EQUAL( (N - numberOfElements), static_cast<unsigned int>(v) );
  }
} // testImNotExactlySureWhatItestAtThisMoment

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
    minutely.addValue(iValue, testStart.addMinute(iValue-1));
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

  std::vector<int> fiveMinuteValuesExpected;
  fiveMinuteValuesExpected.push_back(SeriesAdder<60,5>::value);
  fiveMinuteValuesExpected.push_back(SeriesAdder<55,5>::value);
  fiveMinuteValuesExpected.push_back(SeriesAdder<50,5>::value);
  fiveMinuteValuesExpected.push_back(SeriesAdder<45,5>::value);
  fiveMinuteValuesExpected.push_back(SeriesAdder<40,5>::value);
  fiveMinuteValuesExpected.push_back(SeriesAdder<35,5>::value);
  fiveMinuteValuesExpected.push_back(SeriesAdder<30,5>::value);
  fiveMinuteValuesExpected.push_back(SeriesAdder<25,5>::value);
  fiveMinuteValuesExpected.push_back(SeriesAdder<20,5>::value);
  fiveMinuteValuesExpected.push_back(SeriesAdder<15,5>::value);

  // values 50 - 1 should still be present in minutely
  for(std::deque<AdderValue>::const_iterator iValue = fiveminuteValues.begin(), e = fiveminuteValues.end();
      iValue != e; ++iValue)
  {
    BOOST_CHECK_EQUAL( (double)fiveMinuteValuesExpected[distance(fiveminuteValues.begin(), iValue)], iValue->getValue() );
  }

  BOOST_CHECK_EQUAL( (size_t)1, hourly.getValues().size() );

  double val = SeriesAdder<60,60>::value;
  BOOST_CHECK_EQUAL( val, hourly.getValues().front().getValue() );

  minutely.addValue(61, testStart.addMinute(60));

  BOOST_CHECK_EQUAL( (size_t)2, hourly.getValues().size() );

  SeriesWriter<AdderValue> writer;
  std::string fileName = getTempDir() + "/tmpminutely.xml";
  writer.writeToXML(minutely, fileName);

  BOOST_CHECK_EQUAL( (size_t)2, hourly.getValues().size() );

  boost::filesystem::remove_all(fileName);
} // testWrapping

BOOST_AUTO_TEST_CASE(expansion) {
  Series<AdderValue> minutely(60, 10);

  DateTime now;
  DateTime testStart(static_cast<time_t>(now.secondsSinceEpoch() -
        now.secondsSinceEpoch() % 60));

  // add values 1..60 to minutely (60 values)
  for(int iValue = 1; iValue <= 5; iValue++) {
    minutely.addValue(iValue, testStart.addMinute((iValue-1)*2));
  }

  boost::shared_ptr<std::deque<AdderValue> > expanded = minutely.getExpandedValues();
  BOOST_CHECK_EQUAL( (size_t)9, expanded->size() );
} // expansion

BOOST_AUTO_TEST_CASE(expansionToPresent) {
  Series<AdderValue> minutely(60, 10);

  DateTime now;
  DateTime testStart(static_cast<time_t>(now.secondsSinceEpoch() -
        now.secondsSinceEpoch() % 60 - 19 * 60));

  // add values 1..60 to minutely (60 values)
  for(int iValue = 1; iValue <= 5; iValue++) {
    minutely.addValue(iValue, testStart.addMinute((iValue-1)*2));
  }

  boost::shared_ptr<std::deque<AdderValue> > expanded = minutely.getExpandedValues();
  BOOST_CHECK_EQUAL( (size_t)10, expanded->size() );
  for(std::deque<AdderValue>::iterator iValue = expanded->begin(), e = expanded->end(); iValue != e; iValue++) {
    BOOST_CHECK_EQUAL( (size_t)5, iValue->getValue() );
  }
} // expansion

BOOST_AUTO_TEST_CASE(testReadingNonexistentFile) {
  SeriesReader<CurrentValue> reader;
  Series<CurrentValue>* series = reader.readFromXML("idontexistandneverwill.xml");
  BOOST_CHECK(series == NULL);
}

BOOST_AUTO_TEST_SUITE_END()
