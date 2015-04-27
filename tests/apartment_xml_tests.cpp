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
#include "src/model/cluster.h"
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
    "    <device dsuid=\"0000000000000000000000000000000001\" isPresent=\"0\" firstSeen=\"1429729274\" inactiveSince=\"1970-01-01 01:00:00\" CardinalDirection=\"north west\" WindProtectionClass=\"3\" configurationLocked=\"0\">\n"
    "    </device>\n"
    "  </devices>\n"
    "  <zones>\n"
    "    <zone id=\"0\">\n"
    "      <groups>\n"
    "        <group id=\"16\">\n"
    "          <name>Jalousien</name>\n"
    "          <color>2</color>\n"
    "          <scenes>\n"
    "          </scenes>\n"
    "        </group>\n"
    "      </groups>\n"
    "    </zone>\n"
    "    <zone id=\"1\">\n"
    "      <name>garazh</name>\n"
    "    </zone>\n"
    "  </zones>\n"
    "  <clusters>\n"
    "    <cluster id=\"17\">\n"
    "      <name>Jalousien2</name>\n"
    "      <color>2</color>\n"
    "      <location>south east</location>\n"
    "      <protectionClass>2</protectionClass>\n"
    "      <floor>1</floor>\n"
    "      <configurationLocked>0</configurationLocked>\n"
    "      <lockedScenes>\n"
    "        <lockedScene id=\"5\" />\n"
    "        <lockedScene id=\"17\" />\n"
    "        <lockedScene id=\"18\" />\n"
    "        <lockedScene id=\"19\" />\n"
    "      </lockedScenes>\n"
    "    </cluster>\n"
    "  </clusters>\n"
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
  BOOST_CHECK_EQUAL(dev->getCardinalDirection(), cd_none);
  BOOST_CHECK_EQUAL(dev->getWindProtectionClass(), wpc_none);

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

  /* device with protection/direction */
  DSUID_DEFINE(protID, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
  dev = apt.getDeviceByDSID(protID);
  BOOST_CHECK_EQUAL(dev->getCardinalDirection(), cd_north_west);
  BOOST_CHECK_EQUAL(dev->getWindProtectionClass(), wpc_class_3);

  boost::shared_ptr<Cluster> pClust;
  BOOST_CHECK_NO_THROW(pClust = apt.getCluster(16));
  BOOST_CHECK_EQUAL(pClust->getName(), "Jalousien");
  BOOST_CHECK_EQUAL(pClust->getStandardGroupID(), 2);

  BOOST_CHECK_NO_THROW(pClust = apt.getCluster(17));
  BOOST_CHECK_EQUAL(pClust->getName(), "Jalousien2");
  BOOST_CHECK_EQUAL(pClust->getStandardGroupID(), 2);
  BOOST_CHECK_EQUAL(pClust->getLocation(), cd_south_east);
  BOOST_CHECK_EQUAL(pClust->getProtectionClass(), wpc_class_2);
  BOOST_CHECK_EQUAL(pClust->getFloor(), 1);
  BOOST_CHECK_EQUAL(pClust->getStandardGroupID(), 2);
  BOOST_CHECK_EQUAL(pClust->isConfigurationLocked(), false);
  BOOST_CHECK_EQUAL(pClust->getLockedScenes().size(), 4);

  boost::filesystem::remove_all(fileName);
}

BOOST_AUTO_TEST_CASE(testPersistCardinalDirection) {
  char *dirname, tmpl[] = "/tmp/dss‐persistence-test_XXXXXX";
  std::string filename;

  dirname = mkdtemp(tmpl);
  if (dirname == NULL) {
    BOOST_MESSAGE("Failed to create temporary folder\n");
  }

  {
    // create apartment xml
    Apartment apt1(NULL);
    boost::shared_ptr<Device> dev1 = apt1.allocateDevice(DSUID_NULL);
    dev1->setCardinalDirection(cd_west);
    dev1->setWindProtectionClass(wpc_class_2);

    // 2nd device without protecton calls, orientation
    boost::shared_ptr<Device> dev2 = apt1.allocateDevice(DSUID_BROADCAST);

    ModelPersistence persist1(apt1);
    filename = std::string(dirname) + "/orientation_protection.xml";
    persist1.writeConfigurationToXML(filename);
  }

  Apartment apt2(NULL);
  ModelPersistence persist2(apt2);
  persist2.readConfigurationFromXML(filename, "");

  BOOST_CHECK_NO_THROW(apt2.getDeviceByDSID(DSUID_NULL));
  boost::shared_ptr<Device> dev = apt2.getDeviceByDSID(DSUID_NULL);

  BOOST_CHECK_EQUAL(dev->getCardinalDirection(), cd_west);
  BOOST_CHECK_EQUAL(dev->getWindProtectionClass(), wpc_class_2);

  BOOST_CHECK_NO_THROW(apt2.getDeviceByDSID(DSUID_BROADCAST));
  dev = apt2.getDeviceByDSID(DSUID_BROADCAST);

  BOOST_CHECK_EQUAL(dev->getCardinalDirection(), cd_none);
  BOOST_CHECK_EQUAL(dev->getWindProtectionClass(), wpc_none);

  unlink(filename.c_str());
  rmdir(dirname);
}

BOOST_AUTO_TEST_CASE(testCluster) {
  char *dirname, tmpl[] = "/tmp/dss‐persistence-test_XXXXXX";
  std::string filename;

  dirname = mkdtemp(tmpl);
  if (dirname == NULL) {
    BOOST_MESSAGE("Failed to create temporary folder\n");
  }

  {
    // create apartment xml
    Apartment apt1(NULL);
    boost::shared_ptr<Cluster> pCluster;
    boost::shared_ptr<Zone> zoneBroadcast = apt1.getZone(0);
    pCluster.reset(new Cluster(39, apt1));
    zoneBroadcast->addGroup(pCluster);

    pCluster->setName("Testing");
    pCluster->setStandardGroupID(4);
    pCluster->setLocation(cd_east);
    pCluster->setProtectionClass(wpc_class_3);
    pCluster->setFloor(13);
    pCluster->setConfigurationLocked(true);
    std::vector<int> scenes;
    scenes.push_back(5);
    scenes.push_back(43);
    scenes.push_back(127);
    pCluster->setLockedScenes(scenes);

    ModelPersistence persist1(apt1);
    filename = std::string(dirname) + "/cluster.xml";
    persist1.writeConfigurationToXML(filename);
  }

  Apartment apt2(NULL);
  ModelPersistence persist2(apt2);
  persist2.readConfigurationFromXML(filename, "");

  boost::shared_ptr<Cluster> pClust;
  BOOST_CHECK_NO_THROW(pClust = apt2.getCluster(39));
  BOOST_CHECK_EQUAL(pClust->getName(), "Testing");
  BOOST_CHECK_EQUAL(pClust->getStandardGroupID(), 4);
  BOOST_CHECK_EQUAL(pClust->getLocation(), cd_east);
  BOOST_CHECK_EQUAL(pClust->getProtectionClass(), wpc_class_3);
  BOOST_CHECK_EQUAL(pClust->getFloor(), 13);
  BOOST_CHECK_EQUAL(pClust->isConfigurationLocked(), true);
  BOOST_CHECK_EQUAL(pClust->getLockedScenes().size(), 3);

  unlink(filename.c_str());
  rmdir(dirname);
}

BOOST_AUTO_TEST_SUITE_END()
