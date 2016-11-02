/*
    Copyright (c) 2009, 2010 digitalSTROM.org, Zurich, Switzerland

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

#include <iostream>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include "config.h"

#include "src/base.h"
#include "src/expatparser.h"
#include "src/propertysystem.h"

#include "tests/util/dss_instance_fixture.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(xmlparser)

struct TemporaryFile {
  TemporaryFile(std::string &&path, const std::string &content);
  ~TemporaryFile();

  std::string path;
};

TemporaryFile::TemporaryFile(std::string &&path, const std::string &content)
  : path(path)
{
  std::ofstream ofs(path);
  ofs << content;
  ofs.close();
}

TemporaryFile::~TemporaryFile()
{
  boost::filesystem::remove_all(path);
}

/// wrapper to read xml content from memory
static bool parse(const std::string &xmldesc)
{
  TemporaryFile tmp(TEST_DYNAMIC_DATADIR + "/sample.xml", xmldesc);

  PropertySystem &ps = DSS::getInstance()->getPropertySystem();
  PropertyParser pp;
  return pp.loadFromXML(tmp.path, ps.createProperty("/"));
}

BOOST_FIXTURE_TEST_CASE(testReadFile, DSSInstanceFixture) {
  static const char xml[] =
R"xml(<?xml version="1.0" encoding="utf-8"?>
<properties version="1">

  <property name="integer">
    <property name="-1" type="integer">
      <value>-1</value>
    </property>
    <property name="1" type="integer">
      <value>1</value>
    </property>
  </property>

  <property name="unsigned_integer">
    <property name="1" type="unsignedinteger">
      <value>1</value>
    </property>
  </property>

  <property name="boolean">
    <property name="true" type="boolean">
      <value>true</value>
    </property>
    <property name="false" type="boolean">
      <value>false</value>
    </property>
  </property>

  <property name="string">
    <property name="empty_string" type="string">
      <value></value>
    </property>
    <property name="lorum_ipsum" type="string">
      <value>lorum ipsum</value>
    </property>
  </property>

  <property name="floating">
    <property name="1e+6" type="floating">
      <value>1e+6</value>
    </property>
    <property name="pi" type="floating">
      <value>3.14159</value>
    </property>
    <property name="1.6e-19" type="floating">
      <value>1.6e-19</value>
    </property>
  </property>

  <property name="node1">
    <property name="node2">
      <property name="node3">
        <property name="leaf3" type="string">
            <value>void</value>
        </property>
      </property>
      <property name="leaf2" type="string">
          <value>void</value>
      </property>
    </property>
    <property name="leaf1" type="string">
        <value>void</value>
    </property>
  </property>
  <property name="leaf0" type="string">
      <value>void</value>
  </property>
</properties>)xml";

  BOOST_CHECK(parse(xml));

  PropertySystem &ps = DSS::getInstance()->getPropertySystem();
  BOOST_CHECK_EQUAL(ps.getProperty("/integer/-1")->getValue<int>(), -1);
  BOOST_CHECK_EQUAL(ps.getProperty("/integer/1")->getValue<int>(), 1);

  BOOST_CHECK_THROW(ps.getProperty("/integer/1")->getValue<unsigned int>(), std::runtime_error);
  BOOST_CHECK_THROW(ps.getProperty("/integer/1")->getValue<bool>(), std::runtime_error);
  BOOST_CHECK_THROW(ps.getProperty("/integer/1")->getValue<std::string>(), std::runtime_error);
  BOOST_CHECK_THROW(ps.getProperty("/integer/1")->getValue<double>(), std::runtime_error);

  BOOST_CHECK_EQUAL(ps.getProperty("/unsigned_integer/1")->getValue<unsigned int>(), 1);
  BOOST_CHECK_THROW(ps.getProperty("/unsigned_integer/1")->getValue<int>(), std::runtime_error);

  BOOST_CHECK_EQUAL(ps.getProperty("/floating/1e+6")->getValue<double>(), 1e+6);
  BOOST_CHECK_EQUAL(ps.getProperty("/floating/pi")->getValue<double>(), 3.14159);
  BOOST_CHECK_EQUAL(ps.getProperty("/floating/1.6e-19")->getValue<double>(), 1.6e-19);
  BOOST_CHECK_THROW(ps.getProperty("/floating/pi")->getValue<int>(), std::runtime_error);

  BOOST_CHECK_EQUAL(ps.getProperty("/boolean/true")->getValue<bool>(), true);
  BOOST_CHECK_EQUAL(ps.getProperty("/boolean/false")->getValue<bool>(), false);
  BOOST_CHECK_THROW(ps.getProperty("/boolean/false")->getValue<int>(), std::runtime_error);

  BOOST_CHECK_EQUAL(ps.getProperty("/string/empty_string")->getValue<std::string>(), "");
  BOOST_CHECK_EQUAL(ps.getProperty("/string/lorum_ipsum")->getValue<std::string>(), "lorum ipsum");
  BOOST_CHECK_THROW(ps.getProperty("/string/lorum_ipsum")->getValue<bool>(), std::runtime_error);

  BOOST_CHECK(ps.getProperty("/node1/node2/node3/leaf3"));
  BOOST_CHECK(ps.getProperty("/node1/node2/leaf2"));
  BOOST_CHECK(ps.getProperty("/node1/leaf1"));
  BOOST_CHECK(ps.getProperty("/leaf0"));

  BOOST_CHECK(!ps.getProperty("/node1/node2/node3/leafX"));
  BOOST_CHECK(!ps.getProperty("/node1/node2/leafX"));
  BOOST_CHECK(!ps.getProperty("/node1/leafX"));
  BOOST_CHECK(!ps.getProperty("/leafX"));
}

BOOST_AUTO_TEST_CASE(testLoadMemory) {
  // TODO too much copy&paste to add from memory parsing
  // needs redesign of expat parser, e.g. allocate resources in constructor
  // throw exceptions if fails
}

BOOST_AUTO_TEST_SUITE_END()
