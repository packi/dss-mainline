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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif


#include "cluster.h"
#include "apartment.h"
#include "src/propertysystem.h"
#include "src/foreach.h"

#include "src/model/modelconst.h"
#include "src/model/set.h"

namespace dss {

    //============================================= Group

  Cluster::Cluster(const int _id, Apartment& _apartment)
  : Group(_id, _apartment.getZone(0)),
    m_Location(cd_none),
    m_ProtectionClass(wpc_none),
    m_Floor(0),
    m_ConfigurationLocked(false),
    m_readFromDsm(false),
    m_automatic(false)
  {
  } // ctor

  Cluster::~Cluster() {
    // TODO unpublishPropertyTree
    if (NULL != m_oplock_state) {
      m_pApartment->removeState(m_oplock_state);
    }
  }

  void Cluster::updateLockedScenes() {
    if (!m_pPropertyNode) {
      return;
    }
    PropertyNodePtr pLockedScenes = m_pPropertyNode->getProperty("lockedScenes");
    if (pLockedScenes) {
      pLockedScenes->getParentNode()->removeChild(pLockedScenes);
    }
    pLockedScenes = m_pPropertyNode->createProperty("lockedScenes");
    foreach (int scene, m_LockedScenes) {
      PropertyNodePtr gsubnode = pLockedScenes->createProperty("scene" + intToString(scene));
      gsubnode->createProperty("id")->setIntegerValue(scene);
    }
  } // updateLockedScenes

  void Cluster::publishToPropertyTree() {
    Group::publishToPropertyTree();
    if (m_pPropertyNode != NULL) {
      m_pPropertyNode->createProperty("CardinalDirection")
            ->linkToProxy(PropertyProxyToString<CardinalDirection_t>(m_Location));
      m_pPropertyNode->createProperty("WindProtectionClass")
            ->linkToProxy(PropertyProxyReference<int, WindProtectionClass_t>(m_ProtectionClass));
      m_pPropertyNode->createProperty("Floor")
            ->linkToProxy(PropertyProxyReference<int>(m_Floor, false));
      m_pPropertyNode->createProperty("ConfigurationLocked")
            ->linkToProxy(PropertyProxyReference<bool>(m_ConfigurationLocked, false));
      m_pPropertyNode->createProperty("Automatic")
            ->linkToProxy(PropertyProxyReference<bool>(m_automatic, false));
      updateLockedScenes();
    }
  } // publishToPropertyTree

  bool Cluster::equalConfig(const ClusterSpec_t &cluster) {
    if (m_LockedScenes.size() != cluster.lockedScenes.size()) {
      return false;
    }
    for (unsigned int i = 0; i < m_LockedScenes.size(); ++i) {
      if (m_LockedScenes[i] != cluster.lockedScenes[i]) {
        return false;
      }
    }
    return ((getStandardGroupID() == cluster.StandardGroupID) &&
            (getName() == cluster.Name) &&
            (m_Location == cluster.location) &&
            (m_ProtectionClass == cluster.protectionClass) &&
            (m_Floor == cluster.floor) &&
            (m_ConfigurationLocked == cluster.configurationLocked) &&
            (getConfiguration() == (int)cluster.configuration));
  } // equalConfig

  void Cluster::reset() {
    setLocation(cd_none);
    setProtectionClass(wpc_none);
    setFloor(0);
    m_LockedScenes.clear();
    setConfigurationLocked(false);
    setAutomatic(false);
    setStandardGroupID(0);
    setName("");
    setConfiguration(0);
  } // reset

  void Cluster::removeDevice(Device& _device)
  {
    _device.removeFromGroup(getID());
  } // removeDevice

  bool Cluster::releaseCluster()
  {
    if (getDevices().isEmpty()) {
      reset();
      return true;
    }
    return false;
  }

  bool Cluster::isOperationLock() {
    // unknown -> not locked
    return (NULL != m_oplock_state) && (m_oplock_state->getState() == State_Active);
  }

  void Cluster::setOperationLock(bool _locked, callOrigin_t _callOrigin) {
    if (NULL == m_oplock_state) {
      std::string name = "cluster." + intToString(getID()) + ".operation_lock";
      m_oplock_state = m_pApartment->allocateState(StateType_Group, name, "");
      m_oplock_state->setPersistence(true);
      m_oplock_state->setState(coSystem, State_Unknown);
    }
    m_oplock_state->setState(_callOrigin, _locked ? State_Active : State_Inactive);
  }
} // namespace dss
