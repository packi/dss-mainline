/*
    Copyright (c) 2015 digitalSTROM.org, Zurich, Switzerland

    Author: Christian Hitz, digitalSTROM AG <christian.hitz@digitalstrom.com>

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

#ifndef CLUSTER_H
#define CLUSTER_H

#include <map>
#include <boost/thread/mutex.hpp>

#include "modeltypes.h"
#include "device.h"
#include "group.h"
#include "businterface.h"
#include "data_types.h"

namespace dss {

  /** Represents a apartmentwide cluster */
  class Cluster : public Group {
  private:
    CardinalDirection_t m_Location;
    WindProtectionClass_t m_ProtectionClass;
    int m_Floor;
    std::vector<int> m_LockedScenes;
    bool m_ConfigurationLocked;
    bool m_readFromDsm;
    bool m_automatic;
  public:
    /** Constructs a cluster with the given id. */
    Cluster(const int _id, Apartment& _apartment);
    virtual ~Cluster();

    virtual void publishToPropertyTree();

    void setLocation(const CardinalDirection_t _location) { m_Location = _location; }
    CardinalDirection_t getLocation() const { return m_Location; }

    void setProtectionClass(const WindProtectionClass_t _protectionClass) { m_ProtectionClass = _protectionClass; }
    WindProtectionClass_t getProtectionClass() const { return m_ProtectionClass; }

    void setFloor(const int _floor) { m_Floor = _floor; }
    int getFloor() const { return m_Floor; }

    void setConfigurationLocked(const bool _configurationLocked) { m_ConfigurationLocked = _configurationLocked; }
    bool isConfigurationLocked() const { return m_ConfigurationLocked; }

    void setLockedScenes(const std::vector<int>& _lockedScenes) { m_LockedScenes = _lockedScenes; updateLockedScenes(); }
    const std::vector<int>& getLockedScenes() const { return m_LockedScenes; }
    void addLockedScene(int _lockedScene) { m_LockedScenes.push_back(_lockedScene); updateLockedScenes(); }

    void setReadFromDsm(const bool _readFromDsm) { m_readFromDsm = _readFromDsm; }
    bool isReadFromDsm() const { return m_readFromDsm; }

    void setAutomatic(const bool _automatic) { m_automatic = _automatic; }
    bool isAutomatic() const { return m_automatic; }

    bool equalConfig(const ClusterSpec_t &cluster);

    bool isOperationLock();
    void setOperationLock(bool _locked, callOrigin_t _callOrigin);

    bool releaseCluster();

    void reset();

    void removeDevice(Device& _device);

  private:
    void updateLockedScenes();
    boost::shared_ptr<State> m_oplock_state;
  }; // Group

} // namespace dss

#endif // CLUSTER_H
