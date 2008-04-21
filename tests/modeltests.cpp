/*
 *  modeltests.cpp
 *  dSS
 *
 *  Created by Patrick Stählin on 4/21/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "../core/model.h"

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

using namespace dss;



class TestAction : public Action {
private:
  int& m_ToModify;
public:
  TestAction(int& _toModify) : m_ToModify(_toModify) { };
  
  virtual void Perform(Arguments& args) {
    m_ToModify++;
  };
};

class ModelTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ModelTest);
  CPPUNIT_TEST(testSubscriptions);
  CPPUNIT_TEST_SUITE_END();
  
public:
  void setUp(void) {}
  void tearDown(void) {} 
  
  
protected:
  void testSubscriptions(void) { 
    int testActionCalled = 0;

    Apartment appt;
    TestAction act(testActionCalled);
    vector<int> ids;
    ids.push_back(1);    
    Subscription& subs = appt.Subscribe(act, ids);
    
    Event evtOne(1);
    Event evtTwo(2);
    
    appt.OnEvent(evtOne);
    CPPUNIT_ASSERT_EQUAL(1, testActionCalled);
    appt.OnEvent(evtTwo);
    CPPUNIT_ASSERT_EQUAL(1, testActionCalled);
    appt.OnEvent(evtOne);
    CPPUNIT_ASSERT_EQUAL(2, testActionCalled);
    
    appt.Unsubscribe(subs.GetID());

    appt.OnEvent(evtOne);
    appt.OnEvent(evtTwo);
    CPPUNIT_ASSERT_EQUAL(2, testActionCalled);
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(ModelTest);



