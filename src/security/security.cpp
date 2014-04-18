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

#include "src/hasher.h"
#include "src/propertysystem.h"
#include "src/logger.h"

#include "src/security/user.h"
#include "src/session.h"

#include <boost/thread/mutex.hpp>

namespace dss {

  //================================================== SecurityTreeListener

  class SecurityTreeListener : public PropertyListener {
    __DECL_LOG_CHANNEL__
  public:
    SecurityTreeListener(PropertyNodePtr _pSecurityNode,
                         const std::string _path)
    : m_pSecurityNode(_pSecurityNode),
      m_Path(_path)
    {
      assert(_pSecurityNode != NULL);
      assert(!_path.empty());
      m_pSecurityNode->addListener(this);
    }

  public:
    void writeXML() {
      boost::mutex::scoped_lock lock(m_WriteXMLMutex);
      saveToXML(m_Path, m_pSecurityNode, PropertyNode::Archive);
    }
  protected:
    virtual void propertyChanged(PropertyNodePtr _caller,
                                 PropertyNodePtr _changedNode) {
      if(_changedNode->hasFlag(PropertyNode::Archive)) {
        writeXML();
      }
    }

  private:
    PropertyNodePtr m_pSecurityNode;
    boost::mutex m_WriteXMLMutex;
    const std::string m_Path;
  }; // SecurityTreeListener

  __DEFINE_LOG_CHANNEL__(SecurityTreeListener, lsInfo)

  //================================================== Security

  __DEFINE_LOG_CHANNEL__(Security, lsInfo)

  Security::~Security() {
    m_LoggedInUser.reset(); /* CAUTION: m_LoggedInUser is static */
    delete m_pSystemUser;
  }

  bool Security::authenticate(const std::string& _user, const std::string& _password) {
    signOff();
    assert(m_pPasswordChecker != NULL);
    PropertyNodePtr pUserNode = m_pRootNode->getProperty("users/" + _user);
    if(pUserNode != NULL) {
      if(m_pPasswordChecker->checkPassword(pUserNode, _password)) {
        m_LoggedInUser.reset(new User(pUserNode));
        return true;
      }
    }
    return false;
  } // authenticate

  bool Security::authenticate(boost::shared_ptr<Session> _session) {
    bool result = false;
    signOff();
    if(_session != NULL) {
      if (_session->getUser() != NULL) {
        m_LoggedInUser.reset(new User(*_session->getUser()));
        result = true;
      }
    }
    return result;
  } // authenticate

  bool Security::impersonate(const std::string& _user) {
    signOff();
    PropertyNodePtr pUserNode = m_pRootNode->getProperty("users/" + _user);
    if (pUserNode != NULL) {
      m_LoggedInUser.reset(new User(pUserNode));
      return true;
    }
    return false;
  } // impersonate

  bool Security::authenticateApplication(const std::string& _applicationToken) {
    signOff();
    PropertyNodePtr pTokenNode = m_pRootNode->getProperty("applicationTokens/enabled/" + _applicationToken);
    if(pTokenNode != NULL) {
      PropertyNodePtr pUserNameNode = pTokenNode->getProperty("user");
      if(pUserNameNode != NULL) {
        PropertyNodePtr pUserNode = m_pRootNode->getProperty("users/" + pUserNameNode->getStringValue());
        if (pUserNode != NULL) {
          m_LoggedInUser.reset(new User(pUserNode));
          m_LoggedInUser->setToken(_applicationToken);
          return true;
        }
      }
    }
    return false;
  } // authenticateApplication

  std::string Security::getApplicationName(const std::string& _applicationToken) {
    PropertyNodePtr pTokenNode = m_pRootNode->getProperty("applicationTokens/enabled/" + _applicationToken);
    if(pTokenNode != NULL) {
      PropertyNodePtr pApplicationNameNode = pTokenNode->getProperty("applicationName");
      if(pApplicationNameNode != NULL) {
        return pApplicationNameNode->getStringValue();
      }
    }
    return std::string();
  } // getApplicationName

  void Security::addUserRole(PropertyNodePtr userNode)
  {
    PropertyNodePtr userRole = m_pRootNode->createProperty("roles/owner");
    userNode->createProperty("role")->alias(userRole);
  }

  void Security::addSystemRole(PropertyNodePtr userNode)
  {
    PropertyNodePtr userRole = m_pRootNode->createProperty("roles/system");
    userNode->createProperty("role")->alias(userRole);
  }

  bool Security::signIn(User* _pUser) {
    m_LoggedInUser.reset(new User(*_pUser));
    return true;
  } // signIn

  void Security::signOff() {
    m_LoggedInUser.reset(NULL);
  } // signOff

  bool Security::loadFromXML() {
    assert(!m_FileName.empty());
    return dss::loadFromXML(m_FileName, m_pRootNode);
  } // loadFromXML

  void Security::loginAsSystemUser(const std::string& _reason) {
    if(m_pSystemUser != NULL) {
      Logger::getInstance()->log("Logged in as system user (" + _reason + ")", lsInfo);
      m_LoggedInUser.reset(new User(*m_pSystemUser));
    } else {
      Logger::getInstance()->log("Failed to login as system-user (" + _reason + ")", lsWarning);
    }
  } // loginAsSystemUser

  void Security::startListeningForChanges() {
    assert(!m_FileName.empty());
    m_pTreeListener.reset(new SecurityTreeListener(m_pRootNode, m_FileName));
  } // startListeningForChanges

  void Security::createApplicationToken(const std::string& _applicationName,
                                        const std::string& _token) {
    PropertyNodePtr pPendingTokens =
      m_pRootNode->createProperty("applicationTokens/pending");
    if(pPendingTokens->getProperty(_token) == NULL) {
      PropertyNodePtr pToken = pPendingTokens->createProperty(_token);
      pToken->createProperty("token")->setStringValue(_token);
      pToken->createProperty("applicationName")->setStringValue(_applicationName);
    }
  } // createApplicationToken

  bool Security::enableToken(const std::string& _token, User* _pUser) {
    PropertyNodePtr pTokensNode = m_pRootNode->createProperty("applicationTokens");
    PropertyNodePtr pPendingTokens = pTokensNode->createProperty("pending");
    PropertyNodePtr pEnabledTokens = pTokensNode->createProperty("enabled");
    PropertyNodePtr pToken = pPendingTokens->getProperty(_token);
    if(pToken != NULL) {
      pTokensNode->setFlag(PropertyNode::Archive, true);
      PropertyNodePtr pRealToken = pEnabledTokens->createProperty(_token);
      pRealToken->setFlag(PropertyNode::Archive, true);
      PropertyNodePtr pTokenNode = pRealToken->createProperty("token");
      pTokenNode->setStringValue(_token);
      pTokenNode->setFlag(PropertyNode::Archive, true);
      PropertyNodePtr pApplicationNameNode = pRealToken->createProperty("applicationName");
      pApplicationNameNode->setStringValue(
        pToken->getProperty("applicationName")->getStringValue());
      pApplicationNameNode->setFlag(PropertyNode::Archive, true);
      PropertyNodePtr pUserNode = pRealToken->createProperty("user");
      pUserNode->setFlag(PropertyNode::Archive, true);
      pUserNode->setStringValue(_pUser->getName());
      pPendingTokens->removeChild(pToken);
      return true;
    }
    return false;
  } // enableToken

  bool Security::revokeToken(const std::string& _token) {
    PropertyNodePtr pTokens = m_pRootNode->createProperty("applicationTokens/enabled");
    PropertyNodePtr pToken = m_pRootNode->getProperty("applicationTokens/enabled/" + _token);
    if(pToken != NULL) {
      pTokens->removeChild(pToken);
      if(m_pTreeListener != NULL) {
        m_pTreeListener->writeXML();
      }
      return true;
    }
    return false;
  } // revokeToken

  boost::thread_specific_ptr<User> Security::m_LoggedInUser;


  //================================================== BuiltinPasswordChecker

  bool BuiltinPasswordChecker::checkPassword(PropertyNodePtr _pUser, const std::string& _password) {
    Hasher hasher;
    hasher.add(_pUser->getProperty("salt")->getStringValue());
    hasher.add(_pUser->getDisplayName());
    hasher.add(_password);
    return hasher.str() == _pUser->getProperty("password")->getStringValue();
  } // checkPassword


  //================================================== HTDigestPasswordChecker

  HTDigestPasswordChecker::HTDigestPasswordChecker(const std::string& _passwordFile)
  : m_PasswordFile(_passwordFile)
  { }

  bool HTDigestPasswordChecker::checkPassword(PropertyNodePtr _pUser, const std::string& _password) {
    std::string hashFromFile = readHashFromFile(_pUser->getDisplayName());
    std::vector<std::string> splittedString = splitString(hashFromFile, ':');
    if(splittedString.size() != 3) {
      Logger::getInstance()->log("HTDigestPasswordChecker: File looks bogous, bailing out", lsError);
      return false;
    }
    HasherMD5 hasher;
    hasher.add(_pUser->getDisplayName() + ":" + splittedString[1] + ":" + _password);
    return hasher.str() == splittedString[2];
  } // checkPassword

  std::string HTDigestPasswordChecker::readHashFromFile(std::string _userName) {
    std::ifstream hashFile;
    hashFile.open(m_PasswordFile.c_str(), std::ios::in);
    if(hashFile.is_open()) {
      while(hashFile.good()) {
        char line[256];
        hashFile.getline(line, sizeof(line) - 1);
        if(beginsWith(line, _userName + ":")) {
          return line;
        }
      }
    } else {
      Logger::getInstance()->log("HTDigestPasswordChecker: Could not open file for reading '" + m_PasswordFile + "'", lsError);
    }
    return std::string();
  } // readHashFromFile

} // namespace dss
