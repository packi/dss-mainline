/*
 *  basetests.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 5/19/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>


#include "../core/base.h"

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

using namespace dss;

class BaseTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(BaseTest);
  CPPUNIT_TEST(testCRC);
  CPPUNIT_TEST(testURLDecode);
  CPPUNIT_TEST_SUITE_END();
  
public:
  void setUp(void) {}
  void tearDown(void) {} 
  
protected:
  
  void testCRC() {
    const char testStr[] = "123456789";
    
    uint16_t crc = CRC16((unsigned const char*)&testStr, sizeof(testStr)-1);
//    CPPUNIT_ASSERT_EQUAL((uint16_t)0x29b1, crc);
  }
  
  void testURLDecode(void) { 
    CPPUNIT_ASSERT_EQUAL(string(" "), URLDecode("%20"));
    CPPUNIT_ASSERT_EQUAL(string("a "), URLDecode("a%20"));
    CPPUNIT_ASSERT_EQUAL(string(" a"), URLDecode("%20a"));
    CPPUNIT_ASSERT_EQUAL(string("v a"), URLDecode("v%20a"));
    CPPUNIT_ASSERT_EQUAL(string("  "), URLDecode("%20%20"));
    CPPUNIT_ASSERT_EQUAL(string("   "), URLDecode("%20%20%20"));
    
    CPPUNIT_ASSERT_EQUAL(string(" "), URLDecode("+"));
    CPPUNIT_ASSERT_EQUAL(string("  "), URLDecode("++"));
    CPPUNIT_ASSERT_EQUAL(string(" a "), URLDecode("+a+"));
    CPPUNIT_ASSERT_EQUAL(string("  a"), URLDecode("++a"));
    CPPUNIT_ASSERT_EQUAL(string("a  "), URLDecode("a++"));
    CPPUNIT_ASSERT_EQUAL(string("  b"), URLDecode("+%20b"));
    CPPUNIT_ASSERT_EQUAL(string("sourceid=1&schedule=FREQ=MINUTELY;INTERVAL=1&start=20080520T080000Z"),
                         URLDecode("sourceid=1&schedule=FREQ%3DMINUTELY%3BINTERVAL%3D1&start=20080520T080000Z"));
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(BaseTest);

