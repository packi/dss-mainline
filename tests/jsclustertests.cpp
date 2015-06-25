/*
    Copyright (c) 2015 digitalSTROM.org, Zurich, Switzerland

    Author: Remy Mahler, digitalSTROM AG <remy.mahler@digitalstrom.com>

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

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include "foreach.h"

#include "src/base.h"
#include "src/ds485types.h"
#include "src/structuremanipulator.h"

#include "src/model/apartment.h"
#include "src/model/cluster.h"
#include "src/model/device.h"
#include "src/model/group.h"

#include "src/scripting/jscluster.h"

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
    m_apartment(_apartment) {
    busInterface = new DummyBusInterface(modifyingInterface, queryInterface, actionInterface);
    manipulator = new StructureManipulator(*modifyingInterface, *queryInterface, *m_apartment);
    m_apartment->setBusInterface(busInterface);
    modelMaintenance->setApartment(m_apartment);
    modelMaintenance->setStructureModifyingBusInterface(modifyingInterface);
    modelMaintenance->initialize();
  };

  ~InstanceHelper() {
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

static const int MAX_CLUSTERS = GroupIDAppUserMax - GroupIDAppUserMin + 1;
DSUID_DEFINE(dsuid1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10);

BOOST_AUTO_TEST_SUITE(JSCluster)

BOOST_AUTO_TEST_CASE(testJSClusterCreate_single) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  ScriptExtension* ext = new ClusterScriptExtension(apt1);
  env->addExtension(ext);
  std::string output = "test";

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  int clusterId = ctx->evaluate<int>("Cluster.createCluster(2,'" + output + "')");
  BOOST_CHECK(clusterId != -1);

  std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();
  boost::shared_ptr<Cluster> assignedCluster;
  foreach (boost::shared_ptr<Cluster> cluster, clusters) {
    if (cluster->getStandardGroupID() != 0) {
      assignedCluster = cluster;
    }
  }
  BOOST_CHECK(assignedCluster != NULL);
  BOOST_CHECK(output.compare(assignedCluster->getName()) == 0);
  BOOST_CHECK_EQUAL(assignedCluster->getStandardGroupID(), 2);
}

BOOST_AUTO_TEST_CASE(testJSClusterCreate_use_all_clusters) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  ScriptExtension* ext = new ClusterScriptExtension(apt1);
  env->addExtension(ext);
  std::string output = "test";

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());

  for (int i = 0; i <= MAX_CLUSTERS; ++i) {
    int clusterId = ctx->evaluate<int>("Cluster.createCluster(2, '" + output + "')");
    if (i == MAX_CLUSTERS) {
      BOOST_CHECK_EQUAL(clusterId, -1);
    } else {
      BOOST_CHECK(clusterId != -1);
    }
  }

  std::vector<boost::shared_ptr<Cluster> > clusters = apt1.getClusters();

  boost::shared_ptr<Cluster> assignedCluster;
  foreach (boost::shared_ptr<Cluster> cluster, clusters) {
    BOOST_CHECK(output.compare(cluster->getName()) == 0);
    BOOST_CHECK_EQUAL(cluster->getStandardGroupID(), 2);
  }
}

BOOST_AUTO_TEST_CASE(testJSClusterRelease) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);

  // create cluster
  std::string output = "test";
  boost::shared_ptr<Cluster> cluster = apt1.getEmptyCluster();
  cluster->setStandardGroupID(2);
  cluster->setName(output);

  // create device and add to cluster
  boost::shared_ptr<Device> dev = apt1.allocateDevice(dsuid1);
  dev->setShortAddress(1);
  dev->setName("dev");
  dev->addToGroup(cluster->getID());

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  ScriptExtension* ext = new ClusterScriptExtension(apt1);
  env->addExtension(ext);

  // cluster can not be removed. not empty
  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  bool success = ctx->evaluate<bool>("Cluster.removeCluster(" + intToString(cluster->getID()) + ")");
  BOOST_CHECK_EQUAL(success, false);

  // remove device from cluster
  dev->removeFromGroup(cluster->getID());

  // cluster can be removed
  success = ctx->evaluate<bool>("Cluster.removeCluster(" + intToString(cluster->getID()) + ")");
  BOOST_CHECK_EQUAL(success, true);
}

BOOST_AUTO_TEST_CASE(testJSClusterAddRemoveDevice) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);

  // create cluster
  std::string output = "test";
  boost::shared_ptr<Cluster> cluster = apt1.getEmptyCluster();
  cluster->setStandardGroupID(2);
  cluster->setName(output);

  // create device and add to cluster
  boost::shared_ptr<Device> dev = apt1.allocateDevice(dsuid1);
  dev->setShortAddress(1);
  dev->setName("dev");

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  ScriptExtension* ext = new ClusterScriptExtension(apt1);
  env->addExtension(ext);

  BOOST_CHECK_EQUAL(dev->isInGroup(cluster->getID()), false);
  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Cluster.clusterAddDevice(" + intToString(cluster->getID()) + ", '" + dsuid2str(dsuid1)+ "')");
  BOOST_CHECK_EQUAL(dev->isInGroup(cluster->getID()), true);

  ctx->evaluate<void>("Cluster.clusterRemoveDevice(" + intToString(cluster->getID()) + ", '" + dsuid2str(dsuid1)+ "')");
  BOOST_CHECK_EQUAL(dev->isInGroup(cluster->getID()), false);
}

BOOST_AUTO_TEST_CASE(testJSClusterLockUnlock) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);

  // create cluster
  std::string output = "test";
  boost::shared_ptr<Cluster> cluster = apt1.getEmptyCluster();
  cluster->setStandardGroupID(2);
  cluster->setName(output);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  ScriptExtension* ext = new ClusterScriptExtension(apt1);
  env->addExtension(ext);

  BOOST_CHECK_EQUAL(cluster->isConfigurationLocked(), false);
  boost::scoped_ptr<ScriptContext> ctx(env->getContext());
  ctx->evaluate<void>("Cluster.clusterConfigurationLock(" + intToString(cluster->getID()) + ", true)");
  BOOST_CHECK_EQUAL(cluster->isConfigurationLocked(), true);

  ctx->evaluate<void>("Cluster.clusterConfigurationLock(" + intToString(cluster->getID()) + ", false)");
  BOOST_CHECK_EQUAL(cluster->isConfigurationLocked(), false);
}

BOOST_AUTO_TEST_CASE(testJSClusterSetGetName) {
  Apartment apt1(NULL);
  InstanceHelper helper(&apt1);

  // create cluster
  std::string output = "test";
  boost::shared_ptr<Cluster> cluster = apt1.getEmptyCluster();
  cluster->setStandardGroupID(2);
  cluster->setName(output);

  boost::scoped_ptr<ScriptEnvironment> env(new ScriptEnvironment());
  env->initialize();

  ScriptExtension* ext = new ClusterScriptExtension(apt1);
  env->addExtension(ext);

  boost::scoped_ptr<ScriptContext> ctx(env->getContext());

  output = "firstNameTest";
  ctx->evaluate<void>("Cluster.clusterSetName(" + intToString(cluster->getID()) + ", '" + output + "')");
  BOOST_CHECK(output.compare(cluster->getName()) == 0);

  output = "secondNameTest";
  cluster->setName(output);

  std::string name = ctx->evaluate<std::string>("Cluster.clusterGetName(" + intToString(cluster->getID()) + ")");
  BOOST_CHECK(output.compare(name) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
