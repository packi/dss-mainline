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

#ifndef USER_H_INCLUDED
#define USER_H_INCLUDED

#include <boost/shared_ptr.hpp>
#include <string>

namespace dss {

  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;

  class User {
  public:
    User(PropertyNodePtr _userNode);

    void setPassword(const std::string& _password);
    const std::string& getName();
    PropertyNodePtr getRole();
    const std::string& getToken() const;
    void setToken(const std::string _token);
  private:
    PropertyNodePtr m_pUserNode;
    PropertyNodePtr m_pRoleNode;
    std::string m_Token;
  }; // User

} // namespace dss

#endif
