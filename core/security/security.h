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

#ifndef SECURITY_H_INCLUDED
#define SECURITY_H_INCLUDED

#include <boost/thread.hpp>

#include "core/base.h"

namespace dss {

  class SecurityTreeListener;
  class Session;
  class User;
  class PropertySystem;
  class PropertyNode;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;

  class Security {
  public:
    Security(PropertyNodePtr _pRootNode)
    : m_pRootNode(_pRootNode)
    { }

    Security(PropertyNodePtr _pRootNode,
             boost::shared_ptr<PropertySystem> _pPropertySystem)
    : m_pRootNode(_pRootNode),
      m_pPropertySystem(_pPropertySystem)
    { }

    ~Security() {
      m_LoggedInUser.release();
    }

    bool authenticate(const std::string& _user, const std::string& _password);
    bool authenticate(boost::shared_ptr<Session> _session);
    bool signIn(User* _pUser);
    void signOff();

    bool loadFromXML();
    void setSystemUser(User* _value) {
      m_pSystemUser = _value;
    }
    void loginAsSystemUser(const std::string& _reason);

    static User* getCurrentlyLoggedInUser() {
      return m_LoggedInUser.get();
    }

    void startListeningForChanges();
    void setFileName(const std::string& _value) {
      m_FileName = _value;
    }
  private:
    static boost::thread_specific_ptr<User> m_LoggedInUser;
    PropertyNodePtr m_pRootNode;
    User* m_pSystemUser;
    boost::shared_ptr<SecurityTreeListener> m_pTreeListener;
    boost::shared_ptr<PropertySystem> m_pPropertySystem;
    std::string m_FileName;
  }; // Security

  class SecurityException : public DSSException {
  public:
      SecurityException(const std::string& _message)
      : DSSException(_message)
      {}
  }; // SecurityException

} // namespace dss

#endif
