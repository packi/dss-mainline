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


#include "../core/ds485.h"

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
    frame.push_back('\xFD'); 
    frame.push_back('\x05'); 
    frame.push_back('\x00'); 
    frame.push_back('\x71'); 
    frame.push_back('\x00'); 
    frame.push_back('\x99'); 
    frame.push_back('\x64');
    simPort->PutSimData(frame);
    reader->SetSerialCom(simPort);
    reader->Execute();
  }
  
};

CPPUNIT_TEST_SUITE_REGISTRATION(DS485Test);

