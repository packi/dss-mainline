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

#include "security.h"

#include "core/hasher.h"
#include "core/propertysystem.h"

#include "core/security/user.h"

namespace dss {

  bool Security::authenticate(const std::string& _user, const std::string& _password) {
    PropertyNodePtr pUserNode = m_pRootNode->getProperty("users/" + _user);
    if(pUserNode != NULL) {
      Hasher hasher;
      hasher.add(pUserNode->getProperty("salt")->getStringValue());
      hasher.add(pUserNode->getDisplayName());
      hasher.add(_password);
      if(hasher.str() == pUserNode->getProperty("password")->getStringValue()) {
        m_LoggedInUser.reset(new User(pUserNode));
        return true;
      }
    }
    return false;
  } // authenticate

  void Security::signOff() {
    m_LoggedInUser.release();
  } // signOff

  boost::thread_specific_ptr<User> Security::m_LoggedInUser;

} // namespace dss
