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

#include "dss_life_cycle.h"

#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

#include "dss.h"
#include "base.h"
#include "propertysystem.h"

namespace dss {

static char config[] =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<properties version=\"1\">\n"
    "  <property name=\"config\">\n"
    "    <!-- ds485d -->\n"
    "    <property name=\"subsystems/DSBusInterface/connectionURI\" type=\"string\">\n"
    "      <value>tcp://localhost:8442</value>\n"
    "    </property>\n"
    "    <!-- metering configuration -->\n"
    "    <property name=\"subsystems/Metering/enabled\" type=\"boolean\">\n"
    "      <value>true</value>\n"
    "    </property>\n"
    "  </property>\n"
    "</properties>\n";


static int createConfig(const std::string &fileName) {
  std::ofstream ofs(fileName.c_str());
  ofs << config;
  ofs.close();
  return 0;
}

__DEFINE_LOG_CHANNEL__(DSSLifeCycle, lsInfo);

DSSLifeCycle::DSSLifeCycle() {
  std::vector<std::string> properties;
  properties.push_back("/config/webrootdirectory=" + getTempDir());

  m_configFileName = getTempDir() + "config.xml";
  createConfig(m_configFileName);

  DSS::shutdown();
  if (DSS::getInstance()->initialize(properties, m_configFileName) == false) {
    log("DSS::initialize failed", lsWarning);
    DSS::shutdown();
    throw std::runtime_error("DSS::getInstance failed");
  }

  m_instance = DSS::m_Instance;
  m_incarnation = DSS::s_InstanceGeneration;
  assert(m_incarnation);
}

DSSLifeCycle::~DSSLifeCycle() {
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

void DSSLifeCycle::initPlugins() {
  DSS::getInstance()->addDefaultInterpreterPlugins();
}

}
