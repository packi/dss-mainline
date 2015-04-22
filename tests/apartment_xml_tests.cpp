/*
 *  Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
 *
 *  Author: Andreas Fenkart, <andreas.fenkart@digitalstrom.com>
 *
 *  This file is part of digitalSTROM Server.
 *
 *  digitalSTROM Server is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  digitalSTROM Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <boost/scoped_ptr.hpp>
#include <iostream>
#include <fstream>

#include "src/ds485types.h"
#include "src/model/device.h"
#include "src/model/apartment.h"
#include "src/model/modulator.h"
#include "src/model/devicereference.h"
#include "src/model/zone.h"
#include "src/model/group.h"
#include "src/model/set.h"
#include "src/model/modelpersistence.h"
#include "src/setbuilder.h"
#include "src/dss.h"
#include "src/model/modelconst.h"
#include "src/model/modelmaintenance.h"
#include "src/propertysystem.h"
#include "src/structuremanipulator.h"
#include "src/businterface.h"
#include "src/model/scenehelper.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(apartment_xml)

BOOST_AUTO_TEST_CASE(testApartmentXML) {
  Apartment apt(NULL);
  PropertySystem propSys;
  apt.setPropertySystem(&propSys);

  ModelPersistence model(apt);

  /*
   * Current apartment xml format,
   * Verify that we can still read it
   */

  std::string fileName = getTempDir() + "/apt.xml";
  std::string invalidBackup = getTempDir() + "/invalid.xml";
  std::ofstream ofs(fileName.c_str());
  ofs <<
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
    "<config version=\"1\">\n"
    "  <apartment>\n"
    "    <name>UAZ</name>\n"
    "  </apartment>\n"
    "  <devices>\n"
    "    <device dsid=\"3504175fe0000000ffc00024\" firstSeen=\"1327676779\" isPresent=\"1\" lastKnownDSMeter=\"3504175fe0000000ffc00010\" lastKnownShortAddress=\"24\" lastKnownZoneID=\"1\">\n"
    "      <name>UAZ-469</name>\n"
    "      <properties>\n"
    "        <property archive=\"true\" name=\"my/custom/setting\" readable=\"true\" type=\"integer\" writeable=\"true\">\n"
    "          <value>31512</value>\n"
    "        </property>\n"
    "      </properties>\n"
    "    </device>\n"
    "  </devices>\n"
    "  <zones>\n"
    "    <zone id=\"1\">\n"
    "      <name>garazh</name>\n"
    "    </zone>\n"
    "  </zones>\n"
    "  <dsMeters>\n"
    "    <dsMeter id=\"3504175fe0000000ffc00010\">\n"
    "      <name>UMZ-451</name>\n"
    "      <datamodelHash>559038737</datamodelHash>\n"
    "      <datamodelModification>0</datamodelModification>\n"
    "    </dsMeter>\n"
    "  </dsMeters>\n"
    "</config>";

  ofs.close();

  Logger::getInstance()->log("Checking apartment XML");

  model.readConfigurationFromXML(fileName, invalidBackup);
  BOOST_CHECK_EQUAL(apt.getName(), "UAZ");

  boost::shared_ptr<Device> dev = apt.getDeviceByName("UAZ-469");
  BOOST_CHECK(dev != NULL);

  boost::shared_ptr<DSMeter> meter = apt.getDSMeter("UMZ-451");
  BOOST_CHECK(meter != NULL);

  boost::shared_ptr<Zone> zone = apt.getZone("garazh");
  BOOST_CHECK(zone != NULL);

  BOOST_CHECK_EQUAL(dev->getZoneID(), 1);
  BOOST_CHECK_EQUAL(zone->getID(), 1);

  PropertyNodePtr prop = dev->getPropertyNode();
  BOOST_CHECK(prop != NULL);
  BOOST_CHECK_EQUAL(prop->getProperty("my/custom/setting")->getIntegerValue(),
                    31512);

  boost::filesystem::remove_all(fileName);
}

BOOST_AUTO_TEST_SUITE_END()
