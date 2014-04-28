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

#include "src/base.h"
#include "logger.h"

namespace dss {

  class SecurityTreeListener;
  class Session;
  class User;
  class PropertySystem;
  class PropertyNode;
  class PasswordChecker;
  typedef boost::shared_ptr<PropertyNode> PropertyNodePtr;

  namespace EventName {
    static std::string ApplicationTokenDeleted = "ApplicationTokenDeleted";
  }

  namespace EventProperty {
    static std::string ApplicationToken = "ApplicationToken";
  }

  class Security {
    __DECL_LOG_CHANNEL__
  public:
    Security(PropertyNodePtr _pRootNode)
    : m_pRootNode(_pRootNode),
      m_pSystemUser(NULL)
    { }

    ~Security();

    bool authenticate(const std::string& _user, const std::string& _password);
    bool authenticate(boost::shared_ptr<Session> _session);
    bool authenticateApplication(const std::string& _applicationToken);
    std::string getApplicationName(const std::string& _applicationToken);
    bool signIn(User* _pUser);
    void signOff();
    bool impersonate(const std::string& _user);

    bool loadFromXML();
    void setSystemUser(User* _value) {
      m_pSystemUser = _value;
    }
    void loginAsSystemUser(const std::string& _reason);

    static User* getCurrentlyLoggedInUser() {
      return m_LoggedInUser.get();
    }

    /**
     * Add user to role 'user'
     * @param userNode -- user to be added
     */
    void addUserRole(PropertyNodePtr userNode);

    /**
     * Add user to role 'system'
     * @param userNode -- user to be added
     */
    void addSystemRole(PropertyNodePtr userNode);

    void startListeningForChanges();
    void setFileName(const std::string& _value) {
      m_FileName = _value;
    }
    void setPasswordChecker(boost::shared_ptr<PasswordChecker> _value) {
      m_pPasswordChecker = _value;
    }
    void createApplicationToken(const std::string& _applicationName,
                                const std::string& _token);
    bool enableToken(const std::string& _token, User* _pUser);
    bool revokeToken(const std::string& _token);
  private:
    /**
     * HACK! Instead of
     * DSS::getInstance()->getSecurity()->getCurrentlyLoggedInUser() or
     * Security::getInstance()->getCurrentlyLoggedInUser() we do
     * Security::getCurrentlyLoggedInUser()
     * ... but manage it from DSS::getInstance()->getSecurity()
     */
    static boost::thread_specific_ptr<User> m_LoggedInUser;
    PropertyNodePtr m_pRootNode;
    boost::shared_ptr<SecurityTreeListener> m_pTreeListener;
    boost::shared_ptr<PasswordChecker> m_pPasswordChecker;
    User* m_pSystemUser;
    std::string m_FileName;
  }; // Security

  class PasswordChecker {
  public:
    virtual bool checkPassword(PropertyNodePtr _pUser, const std::string& _password) = 0;
  }; // PasswordChecker

  class BuiltinPasswordChecker : public PasswordChecker {
  public:
    virtual bool checkPassword(PropertyNodePtr _pUser, const std::string& _password);
  }; // BuiltinPasswordChecker

  class HTDigestPasswordChecker : public PasswordChecker {
  public:
    HTDigestPasswordChecker(const std::string& _passwordFile);
    virtual bool checkPassword(PropertyNodePtr _pUser, const std::string& _password);
  private:
    std::string readHashFromFile(std::string _userName);
  private:
    std::string m_PasswordFile;
  }; // HTDigestPasswordChecker

  class SecurityException : public DSSException {
  public:
      SecurityException(const std::string& _message)
      : DSSException(_message)
      {}
  }; // SecurityException

} // namespace dss

#endif
