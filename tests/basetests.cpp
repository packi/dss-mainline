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
  CPPUNIT_TEST(testURLDecode);
  CPPUNIT_TEST_SUITE_END();
  
public:
  void setUp(void) {}
  void tearDown(void) {} 
  
protected:
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
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(BaseTest);

