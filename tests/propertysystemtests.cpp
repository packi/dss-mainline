#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
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


#include "../core/propertysystem.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(PropertySystemTests)

BOOST_AUTO_TEST_CASE(testPropertySystem) {
  PropertySystem* propSys = new PropertySystem();
  PropertyNode* propND1 = new PropertyNode(propSys->getRootNode(), "config");
  PropertyNode* propND2 = new PropertyNode(propSys->getRootNode(), "system");
  PropertyNode* propND3 = new PropertyNode(propND1, "UI");
  PropertyNode* propND4 = new PropertyNode(propND3, "settings");
  BOOST_CHECK(propSys->getProperty("/config/UI/settings[ last ]") == propND4);
  PropertyNode* propND5 = new PropertyNode(propND3, "settings");
  BOOST_CHECK(propSys->getProperty("/config/UI/settings[ last ]") == propND5);

  getBasePath("/bla/asdf");
  getProperty("/bla/asdf");
  getBasePath("/");
  getProperty("/");

  BOOST_CHECK_EQUAL(2, propND3->count("settings"));
  BOOST_CHECK_EQUAL(0, propND3->count("asdf"));
  BOOST_CHECK_EQUAL(1, propSys->getRootNode()->count("config"));

  BOOST_CHECK(propSys->getProperty("/") == propSys->getRootNode());
  BOOST_CHECK(propSys->getProperty("/config") == propND1);
  BOOST_CHECK(propSys->getProperty("/system") == propND2);
  BOOST_CHECK(propSys->getProperty("/config/UI") == propND3);
  BOOST_CHECK(propSys->getProperty("/config/UI/settings") == propND4);
  BOOST_CHECK(propSys->getProperty("/config/UI/settings[0]") == propND4);
  BOOST_CHECK(propSys->getProperty("/config/UI/settings[ 0 ]") == propND4);
  BOOST_CHECK(propSys->getProperty("/config/UI/settings[0 ]") == propND4);
  BOOST_CHECK(propSys->getProperty("/config/UI/settings[ 1]") == propND5);
  BOOST_CHECK(propSys->getProperty("/config/UI/settings[1]") == propND5);
  BOOST_CHECK(propSys->getProperty("/config/UI/settings[ 1 ]") == propND5);
  BOOST_CHECK(propSys->getProperty("/config/UI/settings[1 ]") == propND5);
  BOOST_CHECK(propSys->getProperty("/config/UI/settings[ 1]") == propND5);
  BOOST_CHECK(propSys->getProperty("/config/UI/settings[ last ]") == propND5);
  propND4->setIntegerValue(2);
  BOOST_CHECK_EQUAL(2, propND4->getIntegerValue());
  propND4->setStringValue("bla");
  BOOST_CHECK(propND4->getStringValue() == "bla");
  propSys->createProperty("/bla/blubb");
  propSys->createProperty("/bla/bll");
  propSys->createProperty("/bla/bll/bsa");
  propSys->createProperty("/bla/bll+/bsa");
  BOOST_CHECK_EQUAL(2, propSys->getProperty("/bla")->count("bll"));
  BOOST_CHECK(propSys->getProperty("/af/sgd/sdf") == NULL);

  // test pointer to value
  int bla = 7;
  propND1 = propSys->getProperty("/bla/blubb");
  propND1->linkToProxy(PropertyProxyPointer<int> (&bla));
  BOOST_CHECK(propND1->getIntegerValue() == bla);
  propND1->setIntegerValue(222);
  BOOST_CHECK(propND1->getIntegerValue() == bla);
  propND1->unlinkProxy();

  // test pointer to static getter
  propND1->linkToProxy(PropertyProxyStaticFunction<int> (&testGetter));
  BOOST_CHECK_EQUAL(2, propND1->getIntegerValue());
  propND1->setIntegerValue(5);
  BOOST_CHECK_EQUAL(2,propND1->getIntegerValue());
  propND1->unlinkProxy();

  // test pointer to member, readonly
  PropTest t(2);
  PropertyProxyMemberFunction<class PropTest, int> to(t, &PropTest::getValue);
  propND1->linkToProxy(to);
  BOOST_CHECK_EQUAL(2, propND1->getIntegerValue());
  propND1->setIntegerValue(5);
  BOOST_CHECK_EQUAL(2, propND1->getIntegerValue());
  propND1->unlinkProxy();

  PropertyProxyMemberFunction<class PropTest, int> to2(t,
                                                       &PropTest::getValue,
                                                       &PropTest::setValue);
  propND1->linkToProxy(to2);
  BOOST_CHECK_EQUAL(2, propND1->getIntegerValue());
  propND1->setIntegerValue(5);
  BOOST_CHECK_EQUAL(5, propND1->getIntegerValue());
  propND1->unlinkProxy();

  delete propSys;
} // testPropertySystem

BOOST_AUTO_TEST_SUITE_END()
