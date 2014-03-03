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

#ifndef PRIVILEGE_H_INCLUDED
#define PRIVILEGE_H_INCLUDED

#include <vector>

#include <boost/shared_ptr.hpp>

namespace dss {

  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;

  class Privilege {
  public:
    Privilege(PropertyNodePtr _pRoleNode)
    : m_pRoleNode(_pRoleNode),
      m_Rights(0)
    {};

    typedef enum {
      Write = 2
    } Right;

    bool hasRight(Right _right) {
      return (m_Rights & _right) == _right;
    }

    void addRight(Right _right) {
      m_Rights |= _right;
    }

    void removeRight(Right _right) {
      m_Rights &= ~_right;
    }

    bool isAssociatedWith(PropertyNodePtr _pRoleNode);
  private:
    PropertyNodePtr m_pRoleNode;
    int m_Rights;
  };

  class NodePrivileges {
  public:
    boost::shared_ptr<Privilege> getPrivilegeForRole(PropertyNodePtr _pRoleNode);
    void addPrivilege(boost::shared_ptr<Privilege> _privilege);
  private:
    std::vector<boost::shared_ptr<Privilege> > m_Privileges;
  };

} // namespace dss

#endif
