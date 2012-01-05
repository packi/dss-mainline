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

#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <fstream>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>

#include "src/foreach.h"

#include "src/propertysystem.h"
#include "src/base.h"

#include "src/security/user.h"
#include "src/security/privilege.h"
#include "src/security/security.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(SecurityTests)

BOOST_AUTO_TEST_CASE(testSystemUserIsInitialized) {
  PropertySystem propertysystem;
  Security security(propertysystem.createProperty("/system/security"));
  security.loginAsSystemUser("Some reason");
}

class FixtureTestUserTest {
public:
  FixtureTestUserTest() {
    m_pSecurity.reset(new Security(m_PropertySystem.createProperty("/system/security")));
    boost::shared_ptr<PasswordChecker> checker(new BuiltinPasswordChecker());
    m_pSecurity->setPasswordChecker(checker);
    m_pSecurity->signOff();
    m_pNobodyRole = m_PropertySystem.createProperty("/system/security/roles/nobody");
    m_pSystemRole = m_PropertySystem.createProperty("/system/security/roles/system");
    m_pUserRole = m_PropertySystem.createProperty("/system/security/roles/user");
    m_pUserNode = m_PropertySystem.createProperty("/system/security/users/testuser");
    m_pUserNode->createProperty("role")->alias(m_pUserRole);
    m_pUser.reset(new User(m_pUserNode));
    m_pUser->setPassword("test");
  }

protected:
  boost::shared_ptr<Security> m_pSecurity;
  PropertySystem m_PropertySystem;
  boost::shared_ptr<User> m_pUser;
  PropertyNodePtr m_pUserNode;
  PropertyNodePtr m_pSystemRole;
  PropertyNodePtr m_pUserRole;
  PropertyNodePtr m_pNobodyRole;
};

BOOST_FIXTURE_TEST_CASE(testFixtureDoesntLogInAUser, FixtureTestUserTest) {
  BOOST_CHECK(Security::getCurrentlyLoggedInUser() == NULL);
}

BOOST_FIXTURE_TEST_CASE(testPasswordIsNotPlaintext, FixtureTestUserTest) {
  BOOST_CHECK(m_pUserNode->getProperty("password") != NULL);
  BOOST_CHECK(m_pUserNode->getProperty("password")->getStringValue() != "test");
}

BOOST_FIXTURE_TEST_CASE(testAuthenticate, FixtureTestUserTest) {
  BOOST_CHECK(m_pSecurity->authenticate("testuser", "test"));
}

BOOST_FIXTURE_TEST_CASE(testAuthenticateWrongPassword, FixtureTestUserTest) {
  BOOST_CHECK(!m_pSecurity->authenticate("testuser", "test2"));
}

BOOST_FIXTURE_TEST_CASE(testAuthenticateWrongUser, FixtureTestUserTest) {
  BOOST_CHECK(!m_pSecurity->authenticate("testuser2", "test"));
}

BOOST_FIXTURE_TEST_CASE(testChangingPasswordWorks, FixtureTestUserTest) {
  BOOST_CHECK(m_pSecurity->authenticate("testuser", "test"));
  m_pUser->setPassword("test2");
  BOOST_CHECK(m_pSecurity->authenticate("testuser", "test2"));
}

BOOST_FIXTURE_TEST_CASE(testLoggingInSetsLoggedInUser, FixtureTestUserTest) {
  BOOST_CHECK(m_pSecurity->authenticate("testuser", "test"));
  BOOST_CHECK(Security::getCurrentlyLoggedInUser() != NULL);
}

BOOST_FIXTURE_TEST_CASE(testLoggingInSetsRightUser, FixtureTestUserTest) {
  BOOST_CHECK(m_pSecurity->authenticate("testuser", "test"));
  BOOST_CHECK(Security::getCurrentlyLoggedInUser()->getName() == "testuser");
}

class OtherThread {
public:
  void run() {
    result = (Security::getCurrentlyLoggedInUser() == NULL);
  }

  bool result;
};

BOOST_FIXTURE_TEST_CASE(testLoginDoesnLeakToOtherThread, FixtureTestUserTest) {
  BOOST_CHECK(m_pSecurity->authenticate("testuser", "test"));
  BOOST_CHECK(Security::getCurrentlyLoggedInUser()->getName() == "testuser");

  boost::shared_ptr<OtherThread> threadObj(new OtherThread);
  boost::thread th(boost::bind(&OtherThread::run, threadObj));
  th.join();
  BOOST_CHECK(threadObj->result == true);
}

class FixtureTwoUsers : public FixtureTestUserTest {
public:
  FixtureTwoUsers()
  : FixtureTestUserTest()
  {
    m_pSystemUserNode = m_PropertySystem.createProperty("/system/security/users/system");
    m_pSystemUserNode->createProperty("role")->alias(m_pSystemRole);
    m_pSystemUser.reset(new User(m_pSystemUserNode));
    m_pSystemUser->setPassword("secret");
  }

protected:
  PropertyNodePtr m_pSystemUserNode;
  boost::shared_ptr<User> m_pSystemUser;
};

BOOST_FIXTURE_TEST_CASE(testRolesWork, FixtureTwoUsers) {
  PropertyNodePtr pNode = m_PropertySystem.createProperty("/test");
  pNode->setStringValue("not modified");
  boost::shared_ptr<Privilege>
    privilegeSystem(
      new Privilege(
        m_pSystemRole));

  privilegeSystem->addRight(Privilege::Read);
  privilegeSystem->addRight(Privilege::Write);
  privilegeSystem->addRight(Privilege::Security);

  boost::shared_ptr<Privilege>
    privilegeNobody(
      new Privilege(
        PropertyNodePtr()));
  privilegeNobody->addRight(Privilege::Read);
  boost::shared_ptr<NodePrivileges> privileges(new NodePrivileges());
  privileges->addPrivilege(privilegeSystem);
  privileges->addPrivilege(privilegeNobody);
  m_PropertySystem.getProperty("/")->setPrivileges(privileges);


  boost::shared_ptr<Privilege>
    privilegeNobody2(
      new Privilege(
        PropertyNodePtr()));
  privilegeNobody2->addRight(Privilege::Read);
  boost::shared_ptr<NodePrivileges> privilegesSecurityNode(new NodePrivileges());
  privilegesSecurityNode->addPrivilege(privilegeNobody2);
  m_PropertySystem.getProperty("/system/security")->setPrivileges(privilegesSecurityNode);


  BOOST_CHECK_THROW(pNode->setStringValue("Test"), SecurityException);
  BOOST_CHECK_EQUAL(pNode->getStringValue(), "not modified");

  BOOST_CHECK(m_pSecurity->authenticate("system", "secret"));
  pNode->setStringValue("Test");
  BOOST_CHECK_EQUAL(pNode->getStringValue(), "Test");

  m_pSecurity->signOff();

  m_pSecurity->authenticate("testuser", "test");
}

BOOST_AUTO_TEST_CASE(testDigestPasswords) {
  std::string fileName = getTempDir() + "/digest_test_file";
  std::ofstream ofs(fileName.c_str());
  ofs << "dssadmin:dSS11:79f2e01bf54e8a0626f04b139a1decc2";
  ofs.close();

  PropertySystem propertySystem;
  PropertyNodePtr userNode = propertySystem.createProperty("/dssadmin");

  boost::shared_ptr<HTDigestPasswordChecker> checker(new HTDigestPasswordChecker(fileName));
  BOOST_CHECK_EQUAL(checker->checkPassword(userNode, "dssadmin"), true);
  BOOST_CHECK_EQUAL(checker->checkPassword(userNode, "asfd"), false);
  BOOST_CHECK_EQUAL(checker->checkPassword(userNode, ""), false);

  boost::filesystem::remove_all(fileName);
}

BOOST_AUTO_TEST_SUITE_END()
