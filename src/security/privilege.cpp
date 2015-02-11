/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>

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


#include "privilege.h"

#include "src/foreach.h"
#include "src/propertysystem.h"

namespace dss {

  //================================================== Privilege

  bool Privilege::isAssociatedWith(PropertyNodePtr _pRoleNode) {
    if((_pRoleNode == NULL) || (_pRoleNode->getAliasTarget() == NULL)) {
      return m_pRoleNode == _pRoleNode;
    } else {
      return m_pRoleNode.get() == _pRoleNode->getAliasTarget();
    }
  }


  //================================================== NodePrivileges

  void NodePrivileges::addPrivilege(boost::shared_ptr<Privilege> _privilege) {
    m_Privileges.push_back(_privilege);
  } // addPrivilege

  boost::shared_ptr<Privilege> NodePrivileges::getPrivilegeForRole(PropertyNodePtr _pRoleNode) {
    boost::shared_ptr<Privilege> result;
    foreach(boost::shared_ptr<Privilege> pPrivilege, m_Privileges) {
      if(pPrivilege->isAssociatedWith(_pRoleNode)) {
        result = pPrivilege;
      }
    }
    return result;
  } // getPrivilegeForRole

} // namespace dss
