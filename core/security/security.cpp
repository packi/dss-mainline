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
#include "core/logger.h"

#include "core/security/user.h"
#include "core/session.h"

#include <boost/thread/mutex.hpp>

namespace dss {

  //================================================== SecurityTreeListener

  class SecurityTreeListener : public PropertyListener {
  public:
    SecurityTreeListener(boost::shared_ptr<PropertySystem> _pPropertySystem,
                         PropertyNodePtr _pSecurityNode,
                         const std::string _path)
    : m_pPropertySystem(_pPropertySystem),
      m_pSecurityNode(_pSecurityNode),
      m_Path(_path)
    {
      assert(_pPropertySystem != NULL);
      assert(_pSecurityNode != NULL);
      assert(!_path.empty());
      m_pSecurityNode->addListener(this);
    }

  protected:
    virtual void propertyChanged(PropertyNodePtr _caller,
                                 PropertyNodePtr _changedNode) {
      if(_changedNode->hasFlag(PropertyNode::Archive)) {
        boost::mutex::scoped_lock lock(m_WriteXMLMutex);
        m_pPropertySystem->saveToXML(m_Path, m_pSecurityNode, PropertyNode::Archive);
      }
    }
  private:
    boost::shared_ptr<PropertySystem> m_pPropertySystem;
    PropertyNodePtr m_pSecurityNode;
    boost::mutex m_WriteXMLMutex;
    const std::string m_Path;
  }; // SecurityTreeListener


  //================================================== Security

  bool Security::authenticate(const std::string& _user, const std::string& _password) {
    signOff();
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

  bool Security::authenticate(boost::shared_ptr<Session> _session) {
    bool result = false;
    signOff();
    if(_session != NULL) {
      if(_session->getUser() != NULL) {
        m_LoggedInUser.reset(new User(*_session->getUser()));
        result = true;
      }
    }
    return result;
  } // authenticate

  void Security::signOff() {
    m_LoggedInUser.release();
  } // signOff

  bool Security::loadFromXML() {
    bool result = false;
    if(m_pPropertySystem != NULL) {
      result = m_pPropertySystem->loadFromXML(m_FileName, m_pRootNode);
    }
    return result;
  } // loadFromXML

  void Security::loginAsSystemUser(const std::string& _reason) {
    if(m_pSystemUser != NULL) {
      Logger::getInstance()->log("Logged in as system user (" + _reason + ")", lsInfo);
      m_LoggedInUser.reset(new User(*m_pSystemUser));
    } else {
      Logger::getInstance()->log("Failed to login as system-user (" + _reason + ")", lsFatal);
    }
  } // loginAsSystemUser

  void Security::startListeningForChanges() {
    m_pTreeListener.reset(new SecurityTreeListener(m_pPropertySystem, m_pRootNode, m_FileName));
  } // startListeningForChanges

  boost::thread_specific_ptr<User> Security::m_LoggedInUser;

} // namespace dss
