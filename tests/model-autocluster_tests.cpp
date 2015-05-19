/*
 *  Copyright (c) 2015 digitalSTROM.org, Zurich, Switzerland
 *
 *  Author: Remy Mahler, <remy.mahler@digitalstrom.com>
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

#include "foreach.h"

#include <boost/test/unit_test.hpp>

#include "src/ds485types.h"
#include "src/model/device.h"
#include "src/model/apartment.h"
#include "src/model/modulator.h"
#include "src/model/modelconst.h"
#include "src/model/devicereference.h"
#include "src/model/zone.h"
#include "src/model/group.h"
#include "src/model/cluster.h"
#include "src/model/set.h"
#include "src/model/modelpersistence.h"
#include "src/model/autoclustermaintenance.h"
#include "src/setbuilder.h"
#include "src/dss.h"
#include "src/model/modelconst.h"
#include "src/model/modelmaintenance.h"
#include "src/propertysystem.h"
#include "src/structuremanipulator.h"
#include "src/businterface.h"
#include "src/model/scenehelper.h"
#include "util/ds485-bus-mockups.h"
#include "util/modelmaintenance-mockup.h"

using namespace dss;

class InstanceHelper
{
public:
  InstanceHelper(Apartment* _apartment) :
    modifyingInterface(new DummyStructureModifyingInterface()),
    queryInterface(new DummyStructureQueryBusInterface()),
    actionInterface(new DummyActionRequestInterface()),
    busInterface(NULL),
    manipulator(NULL),
    modelMaintenance(new ModelMaintenanceMock()),
    m_apartment(_apartment)
  {
    busInterface = new DummyBusInterface(modifyingInterface, queryInterface, actionInterface);
    manipulator = new StructureManipulator(*modifyingInterface, *queryInterface, *m_apartment);
    m_apartment->setBusInterface(busInterface);
    modelMaintenance->setApartment(m_apartment);
    modelMaintenance->setStructureModifyingBusInterface(modifyingInterface);
    modelMaintenance->initialize();
  };

  ~InstanceHelper()
  {
    delete modifyingInterface;
    delete queryInterface;
    delete actionInterface;
    delete busInterface;
    delete manipulator;
    delete modelMaintenance;
  };

public:
  DummyStructureModifyingInterface* modifyingInterface;
  DummyStructureQueryBusInterface* queryInterface;
  DummyActionRequestInterface* actionInterface;
  DummyBusInterface* busInterface;
  StructureManipulator* manipulator;
  ModelMaintenanceMock* modelMaintenance;

  Apartment* m_apartment;
};

void filterClusters(std::vector<boost::shared_ptr<Cluster> > _clusters,
                    std::vector<boost::shared_ptr<Cluster> > *_usedClusters,
                    std::vector<boost::shared_ptr<Cluster> > *_automaticClusters)
{
  foreach (boost::shared_ptr<Cluster> cluster, _clusters) {
    if (cluster->getStandardGroupID() != 0) {
      _usedClusters->push_back(cluster);
    }
    if (cluster->isAutomatic()) {
      _automaticClusters->push_back(cluster);
    }
  }
}

static const int MAX_CLUSTERS = GroupIDAppUserMax - GroupIDAppUserMin + 1;

BOOST_AUTO_TEST_SUITE(clustertest)

  BOOST_AUTO_TEST_CASE(assignSingleDeviceDirection) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);
  boost::shared_ptr<Device> dev1 = apt1.allocateDevice(DSUID_NULL);
  boost::shared_ptr<Device> dev2 = apt1.allocateDevice(DSUID_BROADCAST);
  std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();

  // check no cluster is assigned
  foreach (boost::shared_ptr<Cluster> cluster, clusters) {
    BOOST_CHECK_EQUAL(cluster->getStandardGroupID(), 0);
  }

  //----------------------------------------------------------------------------
  // assign cardinal direction, check one cluster is used
  dev1->setCardinalDirection(cd_west);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  {
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 1);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 1);
    BOOST_CHECK_EQUAL(usedClusters.front()->getLocation(), cd_west);
  }

  // assign another cardinal direction, check one cluster is used
  dev1->setCardinalDirection(cd_east);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  {
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 1);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 1);
    BOOST_CHECK_EQUAL(usedClusters.front()->getLocation(), cd_east);
  }

  //----------------------------------------------------------------------------
  // assign protection, check one cluster is used
  dev1->setWindProtectionClass(wpc_class_1);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  {
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 1);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 1);
    BOOST_CHECK_EQUAL(usedClusters.front()->getProtectionClass(), wpc_class_1);
  }

  // assign another protection, check one cluster is used
  dev1->setWindProtectionClass(wpc_class_2);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  {
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 1);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 1);
    BOOST_CHECK_EQUAL(usedClusters.front()->getProtectionClass(), wpc_class_2);
  }
}

BOOST_AUTO_TEST_CASE(assignDoubeDeviceDirection) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);
  boost::shared_ptr<Device> dev1 = apt1.allocateDevice(DSUID_NULL);
  boost::shared_ptr<Device> dev2 = apt1.allocateDevice(DSUID_BROADCAST);
  std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();

  // check no cluster is assigned
  foreach (boost::shared_ptr<Cluster> cluster, clusters) {
    BOOST_CHECK_EQUAL(cluster->getStandardGroupID(), 0);
  }

  //----------------------------------------------------------------------------
  // assign cardinal direction, check one cluster is used

  dev1->setCardinalDirection(cd_west);
  dev2->setCardinalDirection(cd_west);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  {
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 1);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 1);
    BOOST_CHECK_EQUAL(usedClusters.front()->getLocation(), cd_west);
  }

  // assign another cardinal direction, check two clusters are used
  dev1->setCardinalDirection(cd_east);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  {
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 2);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 2);
    CardinalDirection_t location1 = usedClusters[0]->getLocation();
    CardinalDirection_t location2 = usedClusters[1]->getLocation();
    BOOST_CHECK(location1 == cd_east || location1 == cd_west);
    BOOST_CHECK(location2 == cd_east || location2 == cd_west);
  }

  // assign same cardinal direction to second device, check one cluster is used
  dev2->setCardinalDirection(cd_east);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  {
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 1);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 1);
    BOOST_CHECK_EQUAL(usedClusters[0]->getLocation(), cd_east);
  }

  //----------------------------------------------------------------------------
  // assign wind protection , check one cluster is used

  dev1->setWindProtectionClass(wpc_class_2);
  dev2->setWindProtectionClass(wpc_class_2);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  {
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 1);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 1);
    WindProtectionClass_t class1 = usedClusters.front()->getProtectionClass();
    BOOST_CHECK(class1 == wpc_class_2);
  }

  // assign another wind protection class, check two clusters are used
  dev1->setWindProtectionClass(wpc_class_3);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  {
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 2);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 2);
    WindProtectionClass_t class1 = usedClusters.front()->getProtectionClass();
    WindProtectionClass_t class2 = usedClusters.front()->getProtectionClass();
    BOOST_CHECK(class1 == wpc_class_3 || class1 == wpc_class_2);
    BOOST_CHECK(class2 == wpc_class_3 || class2 == wpc_class_2);
  }

  // assign same cardinal direction to second device, check one cluster is used
  dev2->setWindProtectionClass(wpc_class_3);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  {
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 1);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 1);
    WindProtectionClass_t class1 = usedClusters.front()->getProtectionClass();
    BOOST_CHECK(class1 == wpc_class_3);
  }
}

void makeClustersInconsistent(Apartment &_apartment, boost::shared_ptr<Device> &_device, WindProtectionClass_t _protection, bool _clusterLocked)
{
  boost::shared_ptr<Cluster> cluster = _apartment.getEmptyCluster();
  if (cluster != NULL) {
    cluster->setLocation(_device->getCardinalDirection());
    cluster->setProtectionClass(_protection);
    cluster->setStandardGroupID(DEVICE_CLASS_GR);
    cluster->setName(toString(_device->getCardinalDirection()) + "-" +
                     intToString(_device->getWindProtectionClass()));
    cluster->setAutomatic(true);
    _device->addToGroup(cluster->getID());
    cluster->setConfigurationLocked(_clusterLocked);
  }
}

BOOST_AUTO_TEST_CASE(consistencyCheckUnlocked) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);
  boost::shared_ptr<Device> dev1 = apt1.allocateDevice(DSUID_NULL);

  // Assign device to a cluster
  dev1->setWindProtectionClass(wpc_class_1);
  dev1->setCardinalDirection(cd_north);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  int groupId = 0;
  for (int ctr = GroupIDAppUserMin; ctr <= GroupIDAppUserMax; ++ctr) {
    if (dev1->isInGroup(ctr)) {
      groupId = ctr;
      break;
    }
  }

  BOOST_CHECK(groupId > 0);

  // make clusters inconsistent
  for (int ctr = 0; ctr < (MAX_CLUSTERS); ++ctr) {
    makeClustersInconsistent(apt1, dev1, wpc_class_2, false);
  }

  {
    std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), MAX_CLUSTERS);
    BOOST_CHECK_EQUAL(automaticClusters.size(), MAX_CLUSTERS);
  }

  AutoClusterMaintenance maintenance(&apt1);
  maintenance.consistencyCheck(*dev1.get());

  {
    std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 1);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 1);

    boost::shared_ptr<Cluster> testCluster = usedClusters.front();
    BOOST_CHECK_EQUAL(testCluster->getProtectionClass(), wpc_class_1);
    BOOST_CHECK_EQUAL(testCluster->getLocation(), cd_north);

    int groupIdNew = 0;
    for (int ctr = GroupIDAppUserMin; ctr <= GroupIDAppUserMax; ++ctr) {
      if (dev1->isInGroup(ctr)) {
        groupIdNew = ctr;
      }
    }
    BOOST_CHECK_EQUAL(groupId, groupIdNew);
  }
}

BOOST_AUTO_TEST_CASE(consistencyCheckLocked) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);
  boost::shared_ptr<Device> dev1 = apt1.allocateDevice(DSUID_NULL);

  // Assign device to a cluster
  dev1->setWindProtectionClass(wpc_class_1);
  dev1->setCardinalDirection(cd_north);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  int groupId = 0;
  for (int ctr = GroupIDAppUserMin; ctr <= GroupIDAppUserMax; ++ctr) {
    if (dev1->isInGroup(ctr)) {
      groupId = ctr;
    }
  }
  BOOST_CHECK(groupId > 0);
  // make clusters inconsistent
  for (int ctr = 0; ctr < MAX_CLUSTERS; ++ctr) {
    makeClustersInconsistent(apt1, dev1, wpc_class_2, true);
  }

  {
    std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), MAX_CLUSTERS);
    BOOST_CHECK_EQUAL(automaticClusters.size(), MAX_CLUSTERS);
  }

  AutoClusterMaintenance maintenance(&apt1);
  maintenance.consistencyCheck(*dev1.get());

  {
    std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), MAX_CLUSTERS);
    BOOST_CHECK_EQUAL(automaticClusters.size(), MAX_CLUSTERS);

    foreach (boost::shared_ptr<Cluster> testCluster, automaticClusters) {
      BOOST_CHECK_EQUAL(testCluster->getLocation(), cd_north);
      if (testCluster->isConfigurationLocked()) {
        BOOST_CHECK_EQUAL(testCluster->getProtectionClass(), wpc_class_2);
      } else {
        BOOST_CHECK_EQUAL(testCluster->getProtectionClass(), wpc_class_1);
      }
    }
    for (int ctr = GroupIDAppUserMin; ctr <= GroupIDAppUserMax; ++ctr) {
      // must be still in all groups
      BOOST_CHECK(dev1->isInGroup(ctr));
    }
  }
}

BOOST_AUTO_TEST_CASE(joinCheckUnlocked) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);
  boost::shared_ptr<Device> dev1 = apt1.allocateDevice(DSUID_NULL);
  boost::shared_ptr<Device> dev2 = apt1.allocateDevice(DSUID_BROADCAST);

  // Assign device to a cluster
  dev1->setCardinalDirection(cd_north);
  dev1->setWindProtectionClass(wpc_class_3);
  dev2->setCardinalDirection(cd_north);
  dev2->setWindProtectionClass(wpc_class_3);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  // make clusters inconsistent
  for (int ctr = 0; ctr < MAX_CLUSTERS; ++ctr) {
    makeClustersInconsistent(apt1, dev1, dev1->getWindProtectionClass(), false);
  }

  {
    std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), MAX_CLUSTERS);
    BOOST_CHECK_EQUAL(automaticClusters.size(), MAX_CLUSTERS);
  }

  AutoClusterMaintenance maintenance(&apt1);
  maintenance.joinIdenticalClusters();

  {
    std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 1);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 1);
    boost::shared_ptr<Cluster> testCluster = automaticClusters.front();
    BOOST_CHECK_EQUAL(testCluster->getLocation(), cd_north);
    BOOST_CHECK_EQUAL(testCluster->getProtectionClass(), wpc_class_3);
  }
}

BOOST_AUTO_TEST_CASE(joinCheckLocked) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);
  boost::shared_ptr<Device> dev1 = apt1.allocateDevice(DSUID_NULL);
  boost::shared_ptr<Device> dev2 = apt1.allocateDevice(DSUID_BROADCAST);

  // Assign device to a cluster
  dev1->setCardinalDirection(cd_north);
  dev1->setWindProtectionClass(wpc_class_3);
  dev2->setCardinalDirection(cd_north);
  dev2->setWindProtectionClass(wpc_class_3);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  // make clusters inconsistent
  for (int ctr = 0; ctr < MAX_CLUSTERS; ++ctr) {
    makeClustersInconsistent(apt1, dev1, dev1->getWindProtectionClass(), true);
  }

  {
    std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), MAX_CLUSTERS);
    BOOST_CHECK_EQUAL(automaticClusters.size(), MAX_CLUSTERS);
  }

  AutoClusterMaintenance maintenance(&apt1);
  maintenance.joinIdenticalClusters();

  {
    int clusterUsed = 0;
    int clusterAutomatic = 0;
    std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();
    foreach (boost::shared_ptr<Cluster> cluster, clusters) {
      if (cluster->getStandardGroupID() != 0) {
        ++clusterUsed;
      }
      if (cluster->isAutomatic()) {
        ++clusterAutomatic;
        BOOST_CHECK_EQUAL(cluster->getLocation(), cd_north);
        BOOST_CHECK_EQUAL(cluster->getProtectionClass(), wpc_class_3);
      }
    }
    BOOST_CHECK_EQUAL(clusterUsed, MAX_CLUSTERS);
    BOOST_CHECK_EQUAL(clusterAutomatic, MAX_CLUSTERS);
  }

  {
    std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), MAX_CLUSTERS);
    BOOST_CHECK_EQUAL(automaticClusters.size(), MAX_CLUSTERS);
    foreach (boost::shared_ptr<Cluster> cluster, automaticClusters) {
      BOOST_CHECK_EQUAL(cluster->getLocation(), cd_north);
      BOOST_CHECK_EQUAL(cluster->getProtectionClass(), wpc_class_3);
    }
  }
}

class AccessAutoClusterMaintenance :
  public AutoClusterMaintenance
{
  public:
  AccessAutoClusterMaintenance(Apartment* _apartment) :
    AutoClusterMaintenance(_apartment)
  {}

  virtual ~AccessAutoClusterMaintenance()
  {};

  boost::shared_ptr<Cluster> mockfindOrCreateCluster(CardinalDirection_t _cardinalDirection, WindProtectionClass_t _protection)
  {
    return AutoClusterMaintenance::findOrCreateCluster(_cardinalDirection, _protection);
  }
};

BOOST_AUTO_TEST_CASE(getClusterUnassigned) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);
  AccessAutoClusterMaintenance clusterMaint(&apt1);
  boost::shared_ptr<Cluster> cluster = clusterMaint.mockfindOrCreateCluster(cd_none, wpc_none);
  BOOST_CHECK_EQUAL(cluster->getLocation(), cd_none);
  BOOST_CHECK_EQUAL(cluster->getProtectionClass(), wpc_none);
  BOOST_CHECK_EQUAL(cluster->isAutomatic(), true);
  BOOST_CHECK_EQUAL(cluster->getStandardGroupID(), DEVICE_CLASS_GR);
}

BOOST_AUTO_TEST_CASE(unassignmentCheck) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);
  boost::shared_ptr<Device> dev1 = apt1.allocateDevice(DSUID_NULL);

  // Assign device to a cluster
  dev1->setCardinalDirection(cd_none);
  dev1->setWindProtectionClass(wpc_class_3);

  while (helper.modelMaintenance->handleModelEvents())
  {};

  {
    std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 1);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 1);
    BOOST_CHECK_EQUAL(usedClusters.front()->getProtectionClass(), wpc_class_3);
  }

  dev1->setWindProtectionClass(wpc_none);
  while (helper.modelMaintenance->handleModelEvents())
  {};

  {
    // device unassigned. Make shure no cluster is assigned
    std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();
    std::vector<boost::shared_ptr<Cluster> > usedClusters;
    std::vector<boost::shared_ptr<Cluster> > automaticClusters;
    filterClusters(clusters, &usedClusters, &automaticClusters);
    BOOST_CHECK_EQUAL(usedClusters.size(), 0);
    BOOST_CHECK_EQUAL(automaticClusters.size(), 0);
  }
}

BOOST_AUTO_TEST_SUITE_END()
