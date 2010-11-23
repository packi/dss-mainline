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

#include "user.h"

#include <cassert>

#include "core/base.h"
#include "core/propertysystem.h"
#include "core/hasher.h"

namespace dss {

  User::User(PropertyNodePtr _userNode)
  : m_pUserNode(_userNode)
  {
    assert(m_pUserNode != NULL);
    m_pRoleNode = m_pUserNode->getProperty("role");
  } // ctor

  const std::string& User::getName() {
    return m_pUserNode->getDisplayName();
  } // getName

  PropertyNodePtr User::getRole() {
    return m_pRoleNode;
  } // getRole

  void User::setPassword(const std::string& _password) {
    std::string salt = intToString(rand(), true);
    salt = salt.substr(2);
    Hasher hasher;
    hasher.add(salt);
    hasher.add(m_pUserNode->getDisplayName());
    hasher.add(_password);
    m_pUserNode->createProperty("salt")->setStringValue(salt);
    m_pUserNode->createProperty("password")->setStringValue(hasher.str());
  } // setPassword

} // namespace dss
