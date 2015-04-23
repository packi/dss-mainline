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
#include "scenehelper.h"
#include "apartment.h"
#include "src/propertysystem.h"
#include "src/foreach.h"

#include "src/model/modelconst.h"
namespace dss {

    //============================================= Group

  Cluster::Cluster(const int _id, Apartment& _apartment)
  : Group(_id, _apartment.getZone(0), _apartment),
    m_Location(0),
    m_ProtectionClass(0),
    m_Floor(0),
    m_ConfigurationLocked(false),
    m_readFromDsm(false)
  {
  } // ctor

  void Cluster::publishToPropertyTree() {
    Group::publishToPropertyTree();
    if (m_pPropertyNode != NULL) {
      m_pPropertyNode->createProperty("Location")
        ->linkToProxy(PropertyProxyReference<int>(m_Location, false));
      m_pPropertyNode->createProperty("ProtectionClass")
            ->linkToProxy(PropertyProxyReference<int>(m_ProtectionClass, false));
      m_pPropertyNode->createProperty("Floor")
            ->linkToProxy(PropertyProxyReference<int>(m_Floor, false));
      m_pPropertyNode->createProperty("ConfigurationLocked")
            ->linkToProxy(PropertyProxyReference<bool>(m_ConfigurationLocked, false));
      PropertyNodePtr pLockedScenes = m_pPropertyNode->createProperty("lockedScenes");
      foreach (int scene, m_LockedScenes) {
        PropertyNodePtr gsubnode = pLockedScenes->createProperty("scene" + intToString(scene));
        gsubnode->createProperty("id")->setIntegerValue(scene);
      }
    }
  } // publishToPropertyTree

} // namespace dss
