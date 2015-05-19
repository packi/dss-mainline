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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "autoclustermaintenance.h"

#include "src/dss.h"
#include "src/model/apartment.h"
#include "src/model/cluster.h"
#include "src/model/modelconst.h"
#include "src/model/modelmaintenance.h"
#include "src/model/set.h"
#include "src/model/zone.h"

#include <cassert>
#include <foreach.h>
#include <boost/make_shared.hpp>

namespace dss {

__DEFINE_LOG_CHANNEL__(AutoClusterMaintenance, lsInfo);

AutoClusterMaintenance::AutoClusterMaintenance(Apartment* _apartment) :
  m_pApartment(_apartment)
{
  assert(m_pApartment);
} /* AutoClusterMaintenance */

AutoClusterMaintenance::~AutoClusterMaintenance()
{
  m_pApartment = NULL;
} /* ~AutoClusterMaintenance */

void AutoClusterMaintenance::consistencyCheck(Device &_device)
{
  assert(m_pApartment);
  boost::recursive_mutex::scoped_lock scoped_lock(m_pApartment->getMutex());

  removeInvalidAssignments(_device);

  int deviceAssignedInLockedCluster = 0;
  std::vector<boost::shared_ptr<Cluster> > validClusters;
  foreach (boost::shared_ptr<Cluster> cluster, m_pApartment->getClusters()) {
    if (cluster->isAutomatic() &&
        (cluster->getLocation() == _device.getCardinalDirection()) &&
        (cluster->getProtectionClass() == _device.getWindProtectionClass()) &&
        (_device.isInGroup(cluster->getID()))) {

      if (cluster->isConfigurationLocked() &&
          (deviceAssignedInLockedCluster == 0)) {
        deviceAssignedInLockedCluster = cluster->getID();
      }

      if (!cluster->isConfigurationLocked()) {
        validClusters.push_back(cluster);
      }
    }
  }

  if (deviceAssignedInLockedCluster != 0) {
    // remove from all other automatic and unlocked clusters
    foreach (boost::shared_ptr<Cluster> cluster, validClusters) {
      removeDeviceFromCluster(_device, cluster);
    }
  } else if (!validClusters.empty()) {
    // keep first assignment
    int deviceAssignedInCluster = validClusters[0]->getID();
    foreach(boost::shared_ptr<Cluster> cluster, validClusters) {
      if (cluster->getID() == deviceAssignedInCluster) {
        continue;
      }
      removeDeviceFromCluster(_device, cluster);
    }
  } else {
    // No assignment. Try to create one
    boost::shared_ptr<Cluster> cluster = findOrCreateCluster(_device.getCardinalDirection(),
                                                             _device.getWindProtectionClass());
    if (cluster == NULL) {
      log("The targeted cluster can not be created :"
          " Cluster : Protection Class: " + intToString(_device.getWindProtectionClass()) +
          " Location: " + intToString(_device.getCardinalDirection()), lsWarning);
      return;
    }

    if (cluster->isConfigurationLocked()) {
      log("The targeted cluster is locked. Cluster ID: " + intToString(cluster->getID()) +
          " The device with DSUID: " + dsuid2str(_device.getDSID()) + " can not be added", lsWarning);
      return;
    }

    assignDeviceToCluster(_device, cluster);
  }
} /* consistencyCheck */

void AutoClusterMaintenance::joinIdenticalClusters ()
{
  assert(m_pApartment);
  boost::recursive_mutex::scoped_lock scoped_lock(m_pApartment->getMutex());

  std::vector<boost::shared_ptr<Cluster> > clusters = m_pApartment->getClusters();
  for (unsigned ctr = 0; ctr < clusters.size() - 1; ++ctr) {
    boost::shared_ptr<Cluster> cluster = clusters[ctr];
    if (!cluster->isAutomatic() || cluster->isConfigurationLocked()) {
      continue;
    }
    for (unsigned index = ctr + 1; index < clusters.size(); ++index) {
      if (!clusters[index]->isAutomatic() || clusters[index]->isConfigurationLocked()) {
        continue;
      }
      if (cluster->getLocation() == clusters[index]->getLocation() &&
          cluster->getProtectionClass() == clusters[index]->getProtectionClass()) {
        log("The clusters with ids " + intToString(cluster->getStandardGroupID()) + " and id " +
            intToString(cluster->getStandardGroupID()) + " are identical", lsWarning);
        moveClusterDevices(clusters[index], cluster);
      }
    }
  }
} /* joinIdenticalClusters */

void AutoClusterMaintenance::moveClusterDevices(boost::shared_ptr<Cluster> _clusterSource, boost::shared_ptr<Cluster> _clusterDestination)
{
  assert(m_pApartment);
  Set clusterDevices = _clusterSource->getDevices();

  for (int iDevice = 0; iDevice < clusterDevices.length(); ++iDevice) {
    DeviceReference& ref = clusterDevices.get(iDevice);
    boost::shared_ptr<Device> device = ref.getDevice();

    removeDeviceFromCluster(*device.get(), _clusterSource);
    assignDeviceToCluster(*device.get(), _clusterDestination);
  }
} /* moveClusterDevices */

void AutoClusterMaintenance::removeInvalidAssignments(Device  &_device)
{
  // remove invalid assignments from unlocked and automatic clusters
  foreach (boost::shared_ptr<Cluster> cluster, m_pApartment->getClusters()) {
    if (cluster->isAutomatic() && !cluster->isConfigurationLocked()) {
      if ((cluster->getLocation() != _device.getCardinalDirection()) ||
          (cluster->getProtectionClass() != _device.getWindProtectionClass())) {
        // device must not be in cluster
        removeDeviceFromCluster(_device, cluster);
      }
    }
  }
} /* removeInvalidAssignments */

void AutoClusterMaintenance::removeDeviceFromCluster(Device &_device, boost::shared_ptr<Cluster> _cluster)
{
  _cluster->removeDevice(_device);
  busRemoveFromGroup(_device, _cluster);

  if (_cluster->releaseCluster()) {
    busUpdateCluster(_cluster);
  }

} /* removeDeviceFromCluster */

void AutoClusterMaintenance::assignDeviceToCluster(Device &_device, boost::shared_ptr<Cluster> _cluster)
{
  _device.addToGroup(_cluster->getID());
  busAddToGroup(_device, _cluster);
} /* assignDeviceToCluster */

boost::shared_ptr<Cluster> AutoClusterMaintenance::findOrCreateCluster(CardinalDirection_t _cardinalDirection, WindProtectionClass_t _protection)
{
  // find or create cluster
  assert(m_pApartment);

  std::vector<boost::shared_ptr<Cluster> > clusters = m_pApartment->getClusters();
  foreach (boost::shared_ptr<Cluster> cluster, clusters) {
    if ((cluster->getStandardGroupID() != 0) &&
        (cluster->getProtectionClass() == _protection) &&
        (cluster->getLocation() == _cardinalDirection) &&
        (cluster->isAutomatic())) {
      return cluster;
    }
  }

  // cluster does not exist. Get new one
  boost::shared_ptr<Cluster> cluster = m_pApartment->getEmptyCluster();
  if (cluster == NULL) {
    return cluster;
  }

  cluster->setLocation(_cardinalDirection);
  cluster->setProtectionClass(_protection);
  cluster->setStandardGroupID(DEVICE_CLASS_GR);
  cluster->setName(toString(_cardinalDirection) + "-" + intToString(_protection));
  cluster->setAutomatic(true);

  busUpdateCluster(cluster);

  return cluster;
} /* findOrCreateCluster */

void AutoClusterMaintenance::busAddToGroup(Device &_device, boost::shared_ptr<Cluster> _cluster)
{
  // operation can be overridden for testing.

  m_pApartment->getBusInterface()->getStructureModifyingBusInterface()->addToGroup(
        _device.getDSMeterDSID(),
        _cluster->getID(),
        _device.getShortAddress());
} /* busAddToGroup */

void AutoClusterMaintenance::busRemoveFromGroup(Device &_device, boost::shared_ptr<Cluster> _cluster)
{
  // operation can be overridden for testing.
  m_pApartment->getBusInterface()->getStructureModifyingBusInterface()->removeFromGroup(
        _device.getDSMeterDSID(),
        _cluster->getID(),
        _device.getShortAddress());
} /* busRemoveFromGroup */

void AutoClusterMaintenance::busUpdateCluster(boost::shared_ptr<Cluster> _cluster)
{
  // operation can be overridden for testing.
  StructureModifyingBusInterface* itf = m_pApartment->getBusInterface()->getStructureModifyingBusInterface();
  itf->clusterSetName(_cluster->getID(), _cluster->getName());
  itf->clusterSetStandardID(_cluster->getID(), _cluster->getStandardGroupID());
  itf->clusterSetProperties(_cluster->getID(), _cluster->getLocation(),
                            _cluster->getFloor(), _cluster->getProtectionClass());
  itf->clusterSetLockedScenes(_cluster->getID(), _cluster->getLockedScenes());
} /* busUpdateCluster */

}
