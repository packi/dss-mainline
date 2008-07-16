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
  TestAction(int& _toModify) 
  : Action("test", "Test Action"), 
  m_ToModify(_toModify) { };
  
  virtual void Perform(const Arguments& args) {
    m_ToModify++;
  };
};

class ModelTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(ModelTest);
  CPPUNIT_TEST(testSubscriptions);
  CPPUNIT_TEST(testSet);
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
    Arguments args;
    Subscription& subs = appt.Subscribe(act, args,ids);
    
    Event evtOne(1, 1);
    Event evtTwo(2, 1);
    
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
  } // testSubscriptions
  
  void testSet() {
    Apartment apt;
    
    Device& dev1 = apt.AllocateDevice(1);
    dev1.SetShortAddress(1);
    Device& dev2 = apt.AllocateDevice(2);
    dev2.SetShortAddress(2);
    Device& dev3 = apt.AllocateDevice(3);
    dev3.SetShortAddress(3);
    Device& dev4 = apt.AllocateDevice(4);
    dev4.SetShortAddress(4);
    
    Set allDevices = apt.GetDevices();
    
    CPPUNIT_ASSERT_EQUAL(dev1, allDevices.GetByID(1).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev2, allDevices.GetByID(2).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allDevices.GetByID(3).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allDevices.GetByID(4).GetDevice());

    Set setdev1 = Set(dev1);
    
    Set allMinusDev1 = allDevices.Remove(setdev1);
    
    CPPUNIT_ASSERT_EQUAL(3, allMinusDev1.Length());
    
    try {
      allMinusDev1.GetByID(1);
      CPPUNIT_ASSERT(false);
    } catch(ItemNotFoundException) {
      CPPUNIT_ASSERT(true);
    }
    
    CPPUNIT_ASSERT_EQUAL(dev2, allMinusDev1.GetByID(2).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allMinusDev1.GetByID(3).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allMinusDev1.GetByID(4).GetDevice());

    // check that the other sets are not afected by our operation
    CPPUNIT_ASSERT_EQUAL(1, setdev1.Length());
    CPPUNIT_ASSERT_EQUAL(dev1, setdev1.GetByID(1).GetDevice());
    
    CPPUNIT_ASSERT_EQUAL(4, allDevices.Length());
    CPPUNIT_ASSERT_EQUAL(dev1, allDevices.GetByID(1).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev2, allDevices.GetByID(2).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allDevices.GetByID(3).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allDevices.GetByID(4).GetDevice());
    
    Set allRecombined = allMinusDev1.Combine(setdev1);

    CPPUNIT_ASSERT_EQUAL(4, allRecombined.Length());
    CPPUNIT_ASSERT_EQUAL(dev1, allRecombined.GetByID(1).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev2, allRecombined.GetByID(2).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allRecombined.GetByID(3).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allRecombined.GetByID(4).GetDevice());
    
    CPPUNIT_ASSERT_EQUAL(1, setdev1.Length());
    CPPUNIT_ASSERT_EQUAL(dev1, setdev1.GetByID(1).GetDevice());
    
    allRecombined = allRecombined.Combine(setdev1);
    CPPUNIT_ASSERT_EQUAL(4, allRecombined.Length());

    allRecombined = allRecombined.Combine(setdev1);
    CPPUNIT_ASSERT_EQUAL(4, allRecombined.Length());
    
    allRecombined = allRecombined.Combine(allRecombined);
    CPPUNIT_ASSERT_EQUAL(4, allRecombined.Length());
    CPPUNIT_ASSERT_EQUAL(dev1, allRecombined.GetByID(1).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev2, allRecombined.GetByID(2).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allRecombined.GetByID(3).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allRecombined.GetByID(4).GetDevice());
    
    allMinusDev1 = allRecombined;
    allMinusDev1.RemoveDevice(dev1);
    CPPUNIT_ASSERT_EQUAL(3, allMinusDev1.Length());
    CPPUNIT_ASSERT_EQUAL(dev2, allMinusDev1.GetByID(2).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev3, allMinusDev1.GetByID(3).GetDevice());
    CPPUNIT_ASSERT_EQUAL(dev4, allMinusDev1.GetByID(4).GetDevice());    
  } // testSet
};

CPPUNIT_TEST_SUITE_REGISTRATION(ModelTest);



