/*
    Copyright (c) 2009,2010 digitalSTROM.org, Zurich, Switzerland
    Copyright (c) 2008 Patrick Staehlin <me@packi.ch>

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
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <fstream>

#include "src/logger.h"
#include "src/datetools.h"
#include "src/web/webrequests.h"

int testGetter() {
#ifdef VERBOSE_TESTS
  cout << "testGetter() called" << endl;
#endif
  return 2;
}

template<class t>
class PropTest {
private:
  t m_Value;
public:
  PropTest(const t _value) :
    m_Value(_value) {
  }

  void setValue(t _value) {
    m_Value = _value;
#ifdef VERBOSE_TESTS
    cout << "PropTest::setValue new value: " << _value << endl;
#endif
  }

  t getValue() const {
    return m_Value;
  }
};

class PropTestString {
private:
  std::string m_Value;
public:
  PropTestString(const std::string& _value) :
    m_Value(_value) {
  }

  void setValue(const std::string& _value) {
    m_Value = _value;
#ifdef VERBOSE_TESTS
    cout << "PropTestString::setValue new value: " << _value << endl;
#endif
  }

  const std::string& getValue() const {
    return m_Value;
  }
};

#include "src/base.h"
#include "src/propertysystem.h"
#include "src/propertyquery.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(PropertySystemTests)

BOOST_AUTO_TEST_CASE(testBoolProxy) {
  PropertySystem propSys;
  PropertyNodePtr prop = propSys.createProperty("/value");

  PropTest<bool> t(true);
  PropertyProxyMemberFunction<class PropTest<bool>, bool>
    proxy(t, &PropTest<bool>::getValue, &PropTest<bool>::setValue);
  prop->linkToProxy(proxy);

  BOOST_CHECK_EQUAL(true, proxy.getValue());
  BOOST_CHECK_EQUAL(true, prop->getBoolValue());

  prop->setBooleanValue(false);
  BOOST_CHECK_EQUAL(false, proxy.getValue());
  BOOST_CHECK_EQUAL(false, prop->getBoolValue());
} // testBoolProxy

BOOST_AUTO_TEST_CASE(testIntProxy) {
  PropertySystem propSys;
  PropertyNodePtr prop = propSys.createProperty("/value");

  PropTest<int> t(7);
  PropertyProxyMemberFunction<class PropTest<int>, int>
    proxy(t, &PropTest<int>::getValue, &PropTest<int>::setValue);
  prop->linkToProxy(proxy);

  BOOST_CHECK_EQUAL(7, proxy.getValue());
  BOOST_CHECK_EQUAL(7, prop->getIntegerValue());

  prop->setIntegerValue(17);
  BOOST_CHECK_EQUAL(17, proxy.getValue());
  BOOST_CHECK_EQUAL(17, prop->getIntegerValue());
} // testIntProxy

BOOST_AUTO_TEST_CASE(testFloatProxy) {
  PropertySystem propSys;
  PropertyNodePtr prop = propSys.createProperty("/value");

  PropTest<double> t(7.0);
  PropertyProxyMemberFunction<class PropTest<double>, double>
    proxy(t, &PropTest<double>::getValue, &PropTest<double>::setValue);
  prop->linkToProxy(proxy);

  BOOST_CHECK_EQUAL(7.0, proxy.getValue());
  BOOST_CHECK_EQUAL(7.0, prop->getFloatingValue());

  prop->setFloatingValue(17.65);
  BOOST_CHECK_EQUAL(17.65, proxy.getValue());
  BOOST_CHECK_EQUAL(17.65, prop->getFloatingValue());
} // testIntProxy

BOOST_AUTO_TEST_CASE(testStringProxy) {
  PropertySystem propSys;
  PropertyNodePtr prop = propSys.createProperty("/value");

  PropTestString t("initial");
  PropertyProxyMemberFunction<class PropTestString, std::string>
    proxy(t, &PropTestString::getValue, &PropTestString::setValue);
  prop->linkToProxy(proxy);

  BOOST_CHECK_EQUAL("initial", proxy.getValue());
  BOOST_CHECK_EQUAL("initial", prop->getStringValue());

  prop->setStringValue("testValue");
  BOOST_CHECK_EQUAL("testValue", proxy.getValue());
  BOOST_CHECK_EQUAL("testValue", prop->getStringValue());
} // testStringProxy


BOOST_AUTO_TEST_CASE(testPropertyNodeSize) {
  PropertySystem propSys;
  BOOST_CHECK_EQUAL(0, propSys.getRootNode()->size());

  PropertyNodePtr prop = propSys.createProperty("/node");

  BOOST_CHECK_EQUAL(1, propSys.getRootNode()->size());
  propSys.getRootNode()->removeChild(prop);

  BOOST_CHECK_EQUAL(0, propSys.getRootNode()->size());
} // testPropertyNodeSize

BOOST_AUTO_TEST_CASE(testPropertySystem) {
  boost::scoped_ptr<PropertySystem> propSys(new PropertySystem());
  PropertyNodePtr propND1 = propSys->createProperty("/config");
  PropertyNodePtr propND2 = propSys->createProperty("/system");
  PropertyNodePtr propND3(new PropertyNode("UI"));
  propND1->addChild(propND3);
  PropertyNodePtr propND4(new PropertyNode("settings"));
  propND3->addChild(propND4);
  BOOST_CHECK(propSys->getProperty("/config/UI/settings[ last ]") == propND4);
  PropertyNodePtr propND5(new PropertyNode("settings"));
  propND3->addChild(propND5);
  BOOST_CHECK(propSys->getProperty("/config/UI/settings[ last ]") == propND5);

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

  propND1 = propSys->createProperty("/testing/toRemove");
  propSys->createProperty("/testing/toRemove/childNode");
  propND2 = propSys->createProperty("/testing/here");

  propND2->addChild(propND1);
  BOOST_CHECK(propSys->getProperty("/testing/here/toRemove/childNode") != NULL);

  propND2->removeChild(propND1);
  propND1.reset();

  BOOST_CHECK(propSys->getProperty("/testing/here/toRemove/childNode") == NULL);
  BOOST_CHECK(propSys->getProperty("/testing/here/toRemove") == NULL);
  BOOST_CHECK(propSys->getProperty("/testing/here") != NULL);

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
  PropTest<int> t(2);
  PropertyProxyMemberFunction<class PropTest<int>, int> to(t, &PropTest<int>::getValue);
  propND1->linkToProxy(to);
  BOOST_CHECK_EQUAL(2, propND1->getIntegerValue());
  propND1->setIntegerValue(5);
  BOOST_CHECK_EQUAL(2, propND1->getIntegerValue());
  propND1->unlinkProxy();

  PropertyProxyMemberFunction<class PropTest<int>, int> to2(t,
                                                       &PropTest<int>::getValue,
                                                       &PropTest<int>::setValue);
  propND1->linkToProxy(to2);
  BOOST_CHECK_EQUAL(2, propND1->getIntegerValue());
  propND1->setIntegerValue(5);
  BOOST_CHECK_EQUAL(5, propND1->getIntegerValue());
  propND1->unlinkProxy();
} // testPropertySystem

BOOST_AUTO_TEST_CASE(testAliases) {
  boost::scoped_ptr<PropertySystem> propSys(new PropertySystem());

  propSys->createProperty("/config/zone0/device1");
  propSys->createProperty("/config/zone0/device1/name")->setStringValue("dev1");
  propSys->createProperty("/config/zone0/device1/dsid")->setIntegerValue(1111);
  propSys->createProperty("/config/zone0/device1/isOn")->setBooleanValue(false);
  propSys->createProperty("/config/zone0/device2");
  propSys->createProperty("/config/zone0/device3");
  propSys->createProperty("/config/zone0/device4");

  propSys->createProperty("/config/zone1");
  propSys->createProperty("/config/zone2");

  propSys->createProperty("/config/zone1/device1")->alias(propSys->getProperty("/config/zone0/device1"));

  BOOST_CHECK_EQUAL(propSys->getStringValue("/config/zone1/device1/name"), "dev1");
  BOOST_CHECK_EQUAL(propSys->getIntValue("/config/zone1/device1/dsid"), 1111);
  BOOST_CHECK_EQUAL(propSys->getBoolValue("/config/zone1/device1/isOn"), false);

  propSys->setStringValue("/config/zone1/device1/name", "new name");

  BOOST_CHECK_EQUAL(propSys->getStringValue("/config/zone1/device1/name"), "new name");
  BOOST_CHECK_EQUAL(propSys->getStringValue("/config/zone0/device1/name"), "new name");

  propSys->setIntValue("/config/zone1/device1/dsid", 1234);

  BOOST_CHECK_EQUAL(propSys->getIntValue("/config/zone1/device1/dsid"), 1234);
  BOOST_CHECK_EQUAL(propSys->getIntValue("/config/zone0/device1/dsid"), 1234);

  propSys->setBoolValue("/config/zone1/device1/isOn", true);

  BOOST_CHECK_EQUAL(propSys->getBoolValue("/config/zone1/device1/isOn"), true);
  BOOST_CHECK_EQUAL(propSys->getBoolValue("/config/zone0/device1/isOn"), true);

  // move the alias
  propSys->getProperty("/config/zone2")->addChild(propSys->getProperty("/config/zone1/device1"));

  BOOST_CHECK_EQUAL(propSys->getStringValue("/config/zone2/device1/name"), "new name");
  BOOST_CHECK_EQUAL(propSys->getIntValue("/config/zone2/device1/dsid"), 1234);
  BOOST_CHECK_EQUAL(propSys->getBoolValue("/config/zone2/device1/isOn"), true);

  // move the alias target
  propSys->getProperty("/config/zone1")->addChild(propSys->getProperty("/config/zone0/device1"));

  BOOST_CHECK_EQUAL(propSys->getStringValue("/config/zone2/device1/name"), "new name");
  BOOST_CHECK_EQUAL(propSys->getIntValue("/config/zone2/device1/dsid"), 1234);
  BOOST_CHECK_EQUAL(propSys->getBoolValue("/config/zone2/device1/isOn"), true);

  // un-alias
  propSys->getProperty("/config/zone2/device1")->alias(PropertyNodePtr());

  BOOST_CHECK(propSys->getProperty("/config/zone2/device1") != NULL);
  BOOST_CHECK(propSys->getProperty("/config/zone2/device1/name") == NULL);
  BOOST_CHECK(propSys->getProperty("/config/zone2/device1/dsid") == NULL);
  BOOST_CHECK(propSys->getProperty("/config/zone2/device1/isOn") == NULL);

  propSys->getProperty("/config/zone2/device1")->alias(propSys->getProperty("/config/zone1/device1"));
  BOOST_CHECK(propSys->getProperty("/config/zone2/device1") != NULL);
  BOOST_CHECK(propSys->getProperty("/config/zone2/device1/name") != NULL);
  BOOST_CHECK(propSys->getProperty("/config/zone2/device1/dsid") != NULL);
  BOOST_CHECK(propSys->getProperty("/config/zone2/device1/isOn") != NULL);

  // remove alias
  propSys->getProperty("/config/zone1")->removeChild(propSys->getProperty("/config/zone1/device1"));

  BOOST_CHECK(propSys->getProperty("/config/zone2/device1") != NULL);
  BOOST_CHECK(propSys->getProperty("/config/zone2/device1/name") == NULL);
  BOOST_CHECK(propSys->getProperty("/config/zone2/device1/dsid") == NULL);
  BOOST_CHECK(propSys->getProperty("/config/zone2/device1/isOn") == NULL);
} // testAliases

class TestListener : public PropertyListener {
public:
  TestListener(PropertySystem& _system, const std::string& _path)
  : m_System(_system), m_Path(_path) {}

  virtual void propertyChanged(PropertyNodePtr _caller, PropertyNodePtr _pChangedNode) {
    m_System.setBoolValue(m_Path, true);
  }
private:
  PropertySystem& m_System;
  const std::string& m_Path;
};

BOOST_AUTO_TEST_CASE(testListener) {
  const std::string kTriggerPath = "/triggered";
  boost::scoped_ptr<PropertySystem> propSys(new PropertySystem());
  PropertyNodePtr node = propSys->createProperty("/testing");
  boost::scoped_ptr<TestListener> listener(new TestListener(*propSys, kTriggerPath));
  node->addListener(listener.get());

  propSys->setBoolValue(kTriggerPath, false);
  BOOST_CHECK_EQUAL(propSys->getBoolValue(kTriggerPath), false);

  node->setIntegerValue(1);

  BOOST_CHECK_EQUAL(propSys->getBoolValue(kTriggerPath), true);
} // testListener

BOOST_AUTO_TEST_CASE(testFlags) {
  boost::scoped_ptr<PropertySystem> propSys(new PropertySystem());
  PropertyNodePtr node = propSys->createProperty("/testing");

  BOOST_CHECK(node->hasFlag(PropertyNode::Readable));
  BOOST_CHECK(node->hasFlag(PropertyNode::Writeable));
  BOOST_CHECK(!node->hasFlag(PropertyNode::Archive));

  node->setFlag(PropertyNode::Archive, false);
  BOOST_CHECK(!node->hasFlag(PropertyNode::Archive));

  node->setFlag(PropertyNode::Archive, true);
  BOOST_CHECK(node->hasFlag(PropertyNode::Archive));
} // testFlags

BOOST_AUTO_TEST_CASE(testSearchForFlag) {
  boost::scoped_ptr<PropertySystem> propSys(new PropertySystem());
  PropertyNodePtr node = propSys->createProperty("/testing");

  BOOST_CHECK(!node->hasFlag(PropertyNode::Archive));
  BOOST_CHECK(!node->searchForFlag(PropertyNode::Archive));

  PropertyNodePtr subNode = node->createProperty("subnode");
  subNode->setFlag(PropertyNode::Archive, true);

  BOOST_CHECK(!node->hasFlag(PropertyNode::Archive));

  node->setFlag(PropertyNode::Archive, false);

  BOOST_CHECK(!node->hasFlag(PropertyNode::Archive));
  BOOST_CHECK(subNode->hasFlag(PropertyNode::Archive));
  BOOST_CHECK(node->searchForFlag(PropertyNode::Archive));

  node->setFlag(PropertyNode::Archive, true);

  BOOST_CHECK(node->hasFlag(PropertyNode::Archive));
  BOOST_CHECK(subNode->hasFlag(PropertyNode::Archive));
  BOOST_CHECK(node->searchForFlag(PropertyNode::Archive));
} // testSearchForFlag

BOOST_AUTO_TEST_CASE(testPropertyNodeGetAsString) {
  PropertySystem propSys;

  PropertyNodePtr intNode = propSys.createProperty("/int");
  intNode->setIntegerValue(1);
  BOOST_CHECK_EQUAL("1", intNode->getAsString());

  PropertyNodePtr floatNode = propSys.createProperty("/float");
  floatNode->setFloatingValue(100.1);
  BOOST_CHECK_EQUAL("100.1", floatNode->getAsString());

  PropertyNodePtr boolNode = propSys.createProperty("/bool");
  boolNode->setBooleanValue(true);
  BOOST_CHECK_EQUAL("true", boolNode->getAsString());
  boolNode->setBooleanValue(false);
  BOOST_CHECK_EQUAL("false", boolNode->getAsString());

  PropertyNodePtr stringNode = propSys.createProperty("/string");
  stringNode->setStringValue("test");
  BOOST_CHECK_EQUAL("test", stringNode->getAsString());

  PropertyNodePtr noneNode = propSys.createProperty("/none");
  BOOST_CHECK_EQUAL("", noneNode->getAsString());
} // testPropertyNodeGetAsString

BOOST_AUTO_TEST_CASE(testValueTypeFromAndToString) {
  BOOST_CHECK_EQUAL(vTypeBoolean, getValueTypeFromString(getValueTypeAsString(vTypeBoolean)));
  BOOST_CHECK_EQUAL(vTypeInteger, getValueTypeFromString(getValueTypeAsString(vTypeInteger)));
  BOOST_CHECK_EQUAL(vTypeFloating, getValueTypeFromString(getValueTypeAsString(vTypeFloating)));
  BOOST_CHECK_EQUAL(vTypeString, getValueTypeFromString(getValueTypeAsString(vTypeString)));
  BOOST_CHECK_EQUAL(vTypeNone, getValueTypeFromString(getValueTypeAsString(vTypeNone)));
} // testValueTypeFromAndToString

BOOST_AUTO_TEST_CASE(testReadingFromNonexistentFile) {
  boost::scoped_ptr<PropertySystem> propSys(new PropertySystem());
  BOOST_CHECK_EQUAL(loadFromXML("idontexistandneverwill.xml",
                                propSys->getProperty("/")), false);
} // testReadingFromNonexistentFile

BOOST_AUTO_TEST_CASE(testAddChildMovesNode) {
  PropertySystem propSys;
  PropertyNodePtr source = propSys.createProperty("/source");
  source->createProperty("subnode");
  PropertyNodePtr target = propSys.createProperty("/target");

  target->addChild(source);

  BOOST_CHECK_EQUAL(propSys.getProperty("/target/source"), source);
} // testAddChildMovesNode

BOOST_AUTO_TEST_CASE(testIndicesWorkCorrectly) {
  PropertySystem propSys;
  PropertyNodePtr prop1 = propSys.createProperty("/prop");
  PropertyNodePtr prop2 = propSys.createProperty("/prop+");
  PropertyNodePtr prop3 = propSys.createProperty("/prop+");

  BOOST_CHECK_EQUAL(propSys.getProperty("/prop[0]"), prop1);
  BOOST_CHECK_EQUAL(propSys.getProperty("/prop[1]"), prop2);
  BOOST_CHECK_EQUAL(propSys.getProperty("/prop[2]"), prop3);
}

BOOST_AUTO_TEST_CASE(testLoadingEmptyNodesWorks) {
  PropertySystem propSys;

  std::string fileName = getTempDir() + "/testconfig.xml";
  std::ofstream ofs(fileName.c_str());
  ofs <<
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<properties version=\"1\">\n"
    "  <property name=\"config\">\n"
    "    <property name=\"crasher\" type=\"string\">\n"
    "      <value/>\n"
    "    </property>\n"
    "  </property>\n"
    "</properties>";
  ofs.close();

  BOOST_CHECK_EQUAL(loadFromXML(getTempDir() + "testconfig.xml",
                                propSys.createProperty("/config")), true);
  boost::filesystem::remove_all(fileName);
}

BOOST_AUTO_TEST_CASE(testInvalidXML) {
  PropertySystem propSys;

  std::string fileName = getTempDir() + "/testconfig.xml";
  std::ofstream ofs(fileName.c_str());
  ofs <<
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<properties version=\"1\">\n"
    "  <property name=\"config\">\n"
    "    <property name=\"crasher\" type=\"string\">\n"
    "      <value\n"
    "    </property>\n"
    "  </property>\n"
    "</properties>";
  ofs.close();

  BOOST_CHECK_EQUAL(loadFromXML(getTempDir() + "testconfig.xml",
                                propSys.createProperty("/config")), false);
  boost::filesystem::remove_all(fileName);
}

BOOST_AUTO_TEST_CASE(testExceptionWhileParsing) {
  PropertySystem propSys;

  std::string fileName = getTempDir() + "/testconfig.xml";
  std::ofstream ofs(fileName.c_str());
  ofs <<
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<properties version=\"1\">\n"
    "  <property name=\"config\">\n"
    "    <property name=\"crasher\" type=\"integer\">\n"
    "      <value>UAZ is not an integer</value>\n"
    "    </property>\n"
    "  </property>\n"
    "</properties>";
  ofs.close();

  BOOST_CHECK_EQUAL(loadFromXML(getTempDir() + "testconfig.xml",
                                propSys.createProperty("/config")), false);
  boost::filesystem::remove_all(fileName);
}


BOOST_AUTO_TEST_CASE(testQueryAttributes) {
  PropertySystem propSys;
  propSys.createProperty("/test/test1")->setStringValue("content1");
  propSys.createProperty("/test/test2")->setStringValue("content2");
  propSys.createProperty("/test+/test1")->setStringValue("content3");
  propSys.createProperty("/test+/test2")->setStringValue("content4");
  PropertyQuery query(propSys.getProperty("/"), "/test(test1,test2)",true);
  JSONWriter json(JSONWriter::jsonNoneResult);
  query.run(json);
  BOOST_CHECK_EQUAL(json.successJSON(), "{\"test\":[{\"test1\":\"content1\",\"test2\":\"content2\"},{\"test1\":\"content3\"},{\"test2\":\"content4\"}]}");
}

BOOST_AUTO_TEST_CASE(testQueryMissingBracket) {
  PropertySystem propSys;
  propSys.createProperty("/test/test1")->setStringValue("content1");
  propSys.createProperty("/test/test2")->setStringValue("content2");
  BOOST_CHECK_THROW(PropertyQuery query(propSys.getProperty("/"), "/test(test1,test2", false), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(testQuerySubPropertyTwice) {
  PropertySystem propSys;
  propSys.createProperty("/test/test1")->setStringValue("content1");
  propSys.createProperty("/test/test2")->setStringValue("content2");
  propSys.createProperty("/test/content/text")->setStringValue("text1");
  PropertyQuery query(propSys.getProperty("/"), "/test(test1,test2)/content(text)", true);
  JSONWriter json(JSONWriter::jsonNoneResult);
  query.run(json);
  BOOST_CHECK_EQUAL(json.successJSON(), "{\"test\":[{\"test1\":\"content1\",\"test2\":\"content2\",\"content\":[{\"text\":\"text1\"}]}]}");
}

BOOST_AUTO_TEST_CASE(testQuerySubProperty) {
  PropertySystem propSys;
  propSys.createProperty("/test/subnode/content")->setStringValue("content1");
  PropertyQuery query(propSys.getProperty("/"), "/test/subnode(content)", true);
  JSONWriter json(JSONWriter::jsonNoneResult);
  query.run(json);
  BOOST_CHECK_EQUAL(json.successJSON(), "{\"subnode\":[{\"content\":\"content1\"}]}");
}

BOOST_AUTO_TEST_CASE(testCreatePropertySpeed) {
  std::string _path = "/foo/bar/baf/baz/tic/tac/toe";
  std::string _id = "1234859";

  TimeStamp tic, toc;

  {
    PropertySystem propSys;
    PropertyNodePtr p = propSys.createProperty(_path);
    tic.timestamp();
    for (int i = 0; i < 1000 * 1000; i++) {
      PropertyNodePtr q = p->getProperty(_id + "/time");
      if (q == NULL) {
        p->createProperty(_id + "/time")->setStringValue("abcdefghij");
      } else {
        q->setStringValue("abcdefghij");
      }
    }
    toc.timestamp();
  }
  TimeStamp ts_get_set = toc - tic;

  {
    PropertySystem propSys;
    PropertyNodePtr p = propSys.createProperty(_path);
    tic.timestamp();
    for (int i = 0; i < 1000 * 1000; i++) {
      p->createProperty(_id + "/time")->setStringValue("abcdefghij");
    }
    toc.timestamp();
  }
  TimeStamp ts_create = toc - tic;

  BOOST_CHECK_CLOSE(static_cast<double>(ts_create.toMicroSec()),
                    static_cast<double>(ts_get_set.toMicroSec()), 20);

#if 0
  std::cout << ts_create.toMicroSec() / 1000 << " ms\n";
  std::cout << ts_get_set.toMicroSec() / 1000 << " ms \n";
#endif
}
BOOST_AUTO_TEST_SUITE_END()
