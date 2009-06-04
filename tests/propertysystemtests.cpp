/*
 * propertysystemtests.cpp
 *
 *  Created on: Feb 9, 2009
 *      Author: patrick
 */

#include <iostream>

using namespace std;

int testGetter() {
#ifdef VERBOSE_TESTS
  cout << "testGetter() called" << endl;
#endif
  return 2;
}

class PropTest {
private:
  int m_Value;
public:
  PropTest(const int _value) :
    m_Value(_value) {
  }
  ;

  void SetValue(int _value) {
    m_Value = _value;
#ifdef VERBOSE_TESTS
    cout << "PropTest::SetValue new value: " << _value << endl;
#endif
  }

  int GetValue() const {
    return m_Value;
  }
};

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include "../core/propertysystem.h"

#undef CPPUNIT_ASSERT
#define CPPUNIT_ASSERT(condition) CPPUNIT_ASSERT_EQUAL(true, (condition))

using namespace dss;

class PropertySystemTest: public CPPUNIT_NS::TestCase {
CPPUNIT_TEST_SUITE(PropertySystemTest)
  ;
    CPPUNIT_TEST(testPropertySystem);
  CPPUNIT_TEST_SUITE_END()
  ;

public:
  void setUp(void) {
  }
  void tearDown(void) {
  }

protected:

  void testPropertySystem() {
    PropertySystem* propSys = new PropertySystem();
    PropertyNode* propND1 = new PropertyNode(propSys->GetRootNode(), "config");
    PropertyNode* propND2 = new PropertyNode(propSys->GetRootNode(), "system");
    PropertyNode* propND3 = new PropertyNode(propND1, "UI");
    PropertyNode* propND4 = new PropertyNode(propND3, "settings");
    CPPUNIT_ASSERT(propSys->GetProperty("/config/UI/settings[ last ]") == propND4);
    PropertyNode* propND5 = new PropertyNode(propND3, "settings");
    CPPUNIT_ASSERT(propSys->GetProperty("/config/UI/settings[ last ]") == propND5);

    GetBasePath("/bla/asdf");
    GetProperty("/bla/asdf");
    GetBasePath("/");
    GetProperty("/");

    CPPUNIT_ASSERT_EQUAL(2, propND3->Count("settings"));
    CPPUNIT_ASSERT_EQUAL(0, propND3->Count("asdf"));
    CPPUNIT_ASSERT_EQUAL(1, propSys->GetRootNode()->Count("config"));

    CPPUNIT_ASSERT(propSys->GetProperty("/") == propSys->GetRootNode());
    CPPUNIT_ASSERT(propSys->GetProperty("/config") == propND1);
    CPPUNIT_ASSERT(propSys->GetProperty("/system") == propND2);
    CPPUNIT_ASSERT(propSys->GetProperty("/config/UI") == propND3);
    CPPUNIT_ASSERT(propSys->GetProperty("/config/UI/settings") == propND4);
    CPPUNIT_ASSERT(propSys->GetProperty("/config/UI/settings[0]") == propND4);
    CPPUNIT_ASSERT(propSys->GetProperty("/config/UI/settings[ 0 ]") == propND4);
    CPPUNIT_ASSERT(propSys->GetProperty("/config/UI/settings[0 ]") == propND4);
    CPPUNIT_ASSERT(propSys->GetProperty("/config/UI/settings[ 1]") == propND5);
    CPPUNIT_ASSERT(propSys->GetProperty("/config/UI/settings[1]") == propND5);
    CPPUNIT_ASSERT(propSys->GetProperty("/config/UI/settings[ 1 ]") == propND5);
    CPPUNIT_ASSERT(propSys->GetProperty("/config/UI/settings[1 ]") == propND5);
    CPPUNIT_ASSERT(propSys->GetProperty("/config/UI/settings[ 1]") == propND5);
    CPPUNIT_ASSERT(propSys->GetProperty("/config/UI/settings[ last ]") == propND5);
    propND4->SetIntegerValue(2);
    CPPUNIT_ASSERT_EQUAL(2, propND4->GetIntegerValue());
    propND4->SetStringValue("bla");
    CPPUNIT_ASSERT(propND4->GetStringValue() == "bla");
    propSys->CreateProperty("/bla/blubb");
    propSys->CreateProperty("/bla/bll");
    propSys->CreateProperty("/bla/bll/bsa");
    propSys->CreateProperty("/bla/bll+/bsa");
    CPPUNIT_ASSERT_EQUAL(2, propSys->GetProperty("/bla")->Count("bll"));
    CPPUNIT_ASSERT(propSys->GetProperty("/af/sgd/sdf") == NULL);

    // test pointer to value
    int bla = 7;
    propND1 = propSys->GetProperty("/bla/blubb");
    propND1->LinkToProxy(PropertyProxyPointer<int> (&bla));
    CPPUNIT_ASSERT(propND1->GetIntegerValue() == bla);
    propND1->SetIntegerValue(222);
    CPPUNIT_ASSERT(propND1->GetIntegerValue() == bla);
    propND1->UnlinkProxy();

    // test pointer to static getter
    propND1->LinkToProxy(PropertyProxyStaticFunction<int> (&testGetter));
    CPPUNIT_ASSERT_EQUAL(2, propND1->GetIntegerValue());
    propND1->SetIntegerValue(5);
    CPPUNIT_ASSERT_EQUAL(2,propND1->GetIntegerValue());
    propND1->UnlinkProxy();

    // test pointer to member, readonly
    PropTest t(2);
    PropertyProxyMemberFunction<class PropTest, int> to(t, &PropTest::GetValue);
    propND1->LinkToProxy(to);
    CPPUNIT_ASSERT_EQUAL(2, propND1->GetIntegerValue());
    propND1->SetIntegerValue(5);
    CPPUNIT_ASSERT_EQUAL(2, propND1->GetIntegerValue());
    propND1->UnlinkProxy();

    PropertyProxyMemberFunction<class PropTest, int> to2(t,
                                                         &PropTest::GetValue,
                                                         &PropTest::SetValue);
    propND1->LinkToProxy(to2);
    CPPUNIT_ASSERT_EQUAL(2, propND1->GetIntegerValue());
    propND1->SetIntegerValue(5);
    CPPUNIT_ASSERT_EQUAL(5, propND1->GetIntegerValue());
    propND1->UnlinkProxy();

    delete propSys;
  } // testPropertySystem

}; // PropertySystemTest

CPPUNIT_TEST_SUITE_REGISTRATION(PropertySystemTest);

