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

  void setValue(int _value) {
    m_Value = _value;
#ifdef VERBOSE_TESTS
    cout << "PropTest::setValue new value: " << _value << endl;
#endif
  }

  int getValue() const {
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
    PropertyNode* propND1 = new PropertyNode(propSys->getRootNode(), "config");
    PropertyNode* propND2 = new PropertyNode(propSys->getRootNode(), "system");
    PropertyNode* propND3 = new PropertyNode(propND1, "UI");
    PropertyNode* propND4 = new PropertyNode(propND3, "settings");
    CPPUNIT_ASSERT(propSys->getProperty("/config/UI/settings[ last ]") == propND4);
    PropertyNode* propND5 = new PropertyNode(propND3, "settings");
    CPPUNIT_ASSERT(propSys->getProperty("/config/UI/settings[ last ]") == propND5);

    getBasePath("/bla/asdf");
    getProperty("/bla/asdf");
    getBasePath("/");
    getProperty("/");

    CPPUNIT_ASSERT_EQUAL(2, propND3->count("settings"));
    CPPUNIT_ASSERT_EQUAL(0, propND3->count("asdf"));
    CPPUNIT_ASSERT_EQUAL(1, propSys->getRootNode()->count("config"));

    CPPUNIT_ASSERT(propSys->getProperty("/") == propSys->getRootNode());
    CPPUNIT_ASSERT(propSys->getProperty("/config") == propND1);
    CPPUNIT_ASSERT(propSys->getProperty("/system") == propND2);
    CPPUNIT_ASSERT(propSys->getProperty("/config/UI") == propND3);
    CPPUNIT_ASSERT(propSys->getProperty("/config/UI/settings") == propND4);
    CPPUNIT_ASSERT(propSys->getProperty("/config/UI/settings[0]") == propND4);
    CPPUNIT_ASSERT(propSys->getProperty("/config/UI/settings[ 0 ]") == propND4);
    CPPUNIT_ASSERT(propSys->getProperty("/config/UI/settings[0 ]") == propND4);
    CPPUNIT_ASSERT(propSys->getProperty("/config/UI/settings[ 1]") == propND5);
    CPPUNIT_ASSERT(propSys->getProperty("/config/UI/settings[1]") == propND5);
    CPPUNIT_ASSERT(propSys->getProperty("/config/UI/settings[ 1 ]") == propND5);
    CPPUNIT_ASSERT(propSys->getProperty("/config/UI/settings[1 ]") == propND5);
    CPPUNIT_ASSERT(propSys->getProperty("/config/UI/settings[ 1]") == propND5);
    CPPUNIT_ASSERT(propSys->getProperty("/config/UI/settings[ last ]") == propND5);
    propND4->setIntegerValue(2);
    CPPUNIT_ASSERT_EQUAL(2, propND4->getIntegerValue());
    propND4->setStringValue("bla");
    CPPUNIT_ASSERT(propND4->getStringValue() == "bla");
    propSys->createProperty("/bla/blubb");
    propSys->createProperty("/bla/bll");
    propSys->createProperty("/bla/bll/bsa");
    propSys->createProperty("/bla/bll+/bsa");
    CPPUNIT_ASSERT_EQUAL(2, propSys->getProperty("/bla")->count("bll"));
    CPPUNIT_ASSERT(propSys->getProperty("/af/sgd/sdf") == NULL);

    // test pointer to value
    int bla = 7;
    propND1 = propSys->getProperty("/bla/blubb");
    propND1->linkToProxy(PropertyProxyPointer<int> (&bla));
    CPPUNIT_ASSERT(propND1->getIntegerValue() == bla);
    propND1->setIntegerValue(222);
    CPPUNIT_ASSERT(propND1->getIntegerValue() == bla);
    propND1->unlinkProxy();

    // test pointer to static getter
    propND1->linkToProxy(PropertyProxyStaticFunction<int> (&testGetter));
    CPPUNIT_ASSERT_EQUAL(2, propND1->getIntegerValue());
    propND1->setIntegerValue(5);
    CPPUNIT_ASSERT_EQUAL(2,propND1->getIntegerValue());
    propND1->unlinkProxy();

    // test pointer to member, readonly
    PropTest t(2);
    PropertyProxyMemberFunction<class PropTest, int> to(t, &PropTest::getValue);
    propND1->linkToProxy(to);
    CPPUNIT_ASSERT_EQUAL(2, propND1->getIntegerValue());
    propND1->setIntegerValue(5);
    CPPUNIT_ASSERT_EQUAL(2, propND1->getIntegerValue());
    propND1->unlinkProxy();

    PropertyProxyMemberFunction<class PropTest, int> to2(t,
                                                         &PropTest::getValue,
                                                         &PropTest::setValue);
    propND1->linkToProxy(to2);
    CPPUNIT_ASSERT_EQUAL(2, propND1->getIntegerValue());
    propND1->setIntegerValue(5);
    CPPUNIT_ASSERT_EQUAL(5, propND1->getIntegerValue());
    propND1->unlinkProxy();

    delete propSys;
  } // testPropertySystem

}; // PropertySystemTest

CPPUNIT_TEST_SUITE_REGISTRATION(PropertySystemTest);

