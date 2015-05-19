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

#ifndef AUTOCLUSTERMAINTENANCE_H
#define AUTOCLUSTERMAINTENANCE_H


#include "ds485types.h"
#include "logger.h"
#include "data_types.h"
#include "src/model/device.h"

#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

namespace dss {

class Apartment;
class Device;
class Cluster;


class AutoClusterMaintenance : public boost::noncopyable
{
  __DECL_LOG_CHANNEL__

public:
  AutoClusterMaintenance(Apartment* _apartment);
  ~AutoClusterMaintenance();

  void consistencyCheck(Device &_device);
  void joinIdenticalClusters();

protected:
  boost::shared_ptr<Cluster> findOrCreateCluster(CardinalDirection_t _cardinalDirection, WindProtectionClass_t _protection); // protected for unit tests

private:
  void removeDeviceFromCluster(Device  &_device, boost::shared_ptr<Cluster> _cluster);
  void assignDeviceToCluster(Device &_device, boost::shared_ptr<Cluster> _cluster);

  void moveClusterDevices(boost::shared_ptr<Cluster> _clusterSource, boost::shared_ptr<Cluster> _clusterDestination);
  void removeInvalidAssignments(Device  &_device);
  std::vector<boost::shared_ptr<Cluster> > getUnlockedClusterAssignment(Device &_device);
  int getFirstLockedClusterAssignment(Device &_device);

  void busAddToGroup(Device &_device, boost::shared_ptr<Cluster> _cluster);
  void busRemoveFromGroup(Device &_device, boost::shared_ptr<Cluster> _cluster);
  void busUpdateCluster(boost::shared_ptr<Cluster> _cluster);

private:
  Apartment* m_pApartment;
};

} // namespace dss

#endif // AUTOCLUSTERMAINTENANCE_H
