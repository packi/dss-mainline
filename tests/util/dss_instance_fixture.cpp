/*
    Copyright (c) 2014 digitalSTROM.org, Zurich, Switzerland

    Author: Andreas Fenkart

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

#include "dss_instance_fixture.h"

#include <fstream>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>

#include "dss.h"
#include "base.h"
#include "propertysystem.h"

namespace dss {

// equivalent of $PREFIX/share
const std::string TEST_STATIC_DATADIR(ABS_SRCDIR "/tests/data");
const std::string TEST_BUILD_DATADIR(ABS_BUILDDIR "/tests/data");

__DEFINE_LOG_CHANNEL__(DSSInstanceFixture, lsInfo);

DSSInstanceFixture::DSSInstanceFixture() {
  std::vector<std::string> properties;

  boost::filesystem::remove_all(TEST_BUILD_DATADIR + "/tmp");
  boost::filesystem::create_directory(TEST_BUILD_DATADIR + "/tmp");

  std::string staticDataDir =
    boost::filesystem::canonical(TEST_STATIC_DATADIR).native();
  std::string dynamicDataDir =
    boost::filesystem::canonical(TEST_BUILD_DATADIR + "/tmp").native();

  properties.push_back("/config/datadirectory=" + staticDataDir);
  properties.push_back("/config/configdirectory=" + staticDataDir);
  properties.push_back("/config/webrootdirectory=" + staticDataDir);
  properties.push_back("/config/jslogdirectory=" + dynamicDataDir);
  properties.push_back("/config/savedpropsdirectory=" + dynamicDataDir);
  properties.push_back("/config/databasedirectory=" + dynamicDataDir);

  DSS::shutdown();
  if (!DSS::getInstance()->initialize(properties, TEST_STATIC_DATADIR + "/config.xml")) {
    log("DSS::initialize failed", lsWarning);
    DSS::shutdown();
    throw std::runtime_error("DSS::getInstance failed");
  }

  m_instance = DSS::m_Instance;
  m_incarnation = DSS::s_InstanceGeneration;
  assert(m_incarnation);

  // needed to initialize (EventInterpreter-) subsystem
  m_instance->m_State = ssInitializingSubsystems;
}

DSSInstanceFixture::~DSSInstanceFixture() {
  // DSS::getInstance will create a new instance
  if ((m_instance != DSS::m_Instance) ||
      m_incarnation != DSS::s_InstanceGeneration) {
    log("destructor -- already deleted", lsDebug);
    return;
  }

  log("destructor", lsDebug);
  DSS::shutdown();
  assert(!DSS::hasInstance());
  boost::filesystem::remove_all(m_configFileName);
  log("destructor - done", lsDebug);
}

void DSSInstanceFixture::initPlugins() {
  DSS::getInstance()->addDefaultInterpreterPlugins();
}

}
