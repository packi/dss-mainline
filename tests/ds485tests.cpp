/*
 *  ds485tests.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 6/4/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <boost/scoped_ptr.hpp>


#include "../unix/ds485.h"

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

using namespace dss;

class DS485Test : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(DS485Test);
  CPPUNIT_TEST(testFrameReader);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {}

protected:

  void testFrameReader() {
    boost::scoped_ptr<DS485FrameReader> reader(new DS485FrameReader());
    boost::shared_ptr<SerialComSim> simPort(new SerialComSim());
    string frame;
/*
    frame.push_back('\xFD');
    frame.push_back('\x05');
    frame.push_back('\x00');
    frame.push_back('\x71');
    frame.push_back('\x00');
    frame.push_back('\x99');
    frame.push_back('\x64');
 */
    /* Working frame

    frame.push_back('\xFD');
    frame.push_back('\x01');
    frame.push_back('\x00');
    frame.push_back('\x14');
    frame.push_back('\xce');
    frame.push_back('\x01');
    frame.push_back('\x00');
    frame.push_back('\x00');
    frame.push_back('\x39');
    frame.push_back('\x24');
     */

    /* Captured frames */
    frame.push_back('\xFD');
    frame.push_back('\x3F');
    frame.push_back('\x7E');
    frame.push_back('\xD7');
    frame.push_back('\x7E');
    frame.push_back('\x88');
    frame.push_back('\xFD');
    frame.push_back('\xF7');
    frame.push_back('\xF7');
    frame.push_back('\xB7');
    frame.push_back('\x78');
    frame.push_back('\x50');
    frame.push_back('\x05');
    frame.push_back('\x5E');
    frame.push_back('\xF7');
    frame.push_back('\x77');
    frame.push_back('\xF7');
    frame.push_back('\x87');
    frame.push_back('\xD6');
    frame.push_back('\xF7');
    frame.push_back('\xF7');
    frame.push_back('\x17');
    frame.push_back('\x72');
    frame.push_back('\x56');
    frame.push_back('\x05');
    frame.push_back('\xD6');
    frame.push_back('\xD7');
    frame.push_back('\x77');
    frame.push_back('\x77');
    frame.push_back('\xC8');
    frame.push_back('\xFD');
    frame.push_back('\xF7');
    frame.push_back('\xF7');
    frame.push_back('\x77');
    frame.push_back('\x63');
    frame.push_back('\x54');
    frame.push_back('\xFF');
    frame.push_back('\x05');
    frame.push_back('\xF8');
    frame.push_back('\x7F');
    frame.push_back('\x7F');
    frame.push_back('\x3F');
    frame.push_back('\xAE');
    frame.push_back('\x7E');
    frame.push_back('\x88');
    frame.push_back('\xFD');
    frame.push_back('\x7E');
    frame.push_back('\xFE');
    frame.push_back('\xFF');
    frame.push_back('\x7F');
    frame.push_back('\x0F');
    frame.push_back('\xD2');
    frame.push_back('\xFE');
    frame.push_back('\x05');
    frame.push_back('\xFC');
    frame.push_back('\x7F');
    frame.push_back('\xF2');
    frame.push_back('\xFE');
    frame.push_back('\xDF');
    frame.push_back('\x7E');
    frame.push_back('\x88');

    simPort->putSimData(frame);
    reader->setSerialCom(simPort);
    //reader->execute();
  }

};

CPPUNIT_TEST_SUITE_REGISTRATION(DS485Test);

