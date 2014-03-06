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

static std::string pathSecurity = "/system/security";

static std::string pathUserRole = "/system/security/roles/user";
static std::string pathSystemRole = "/system/security/roles/system";

static std::string pathTestUser = "/system/security/users/testuser";
static std::string pathSystemUser = "/system/security/users/system";

BOOST_AUTO_TEST_SUITE(SecurityTests)

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

BOOST_AUTO_TEST_CASE(testSystemUserNotSet) {
  PropertySystem propertysystem;
  Security security(propertysystem.createProperty("/system/security"));
  security.loginAsSystemUser("system_user_does_not_exist");
  BOOST_CHECK(security.getCurrentlyLoggedInUser() == NULL);
}

class FixtureTestUserTest {
public:
  FixtureTestUserTest() {
    m_pSecurity.reset(new Security(m_PropertySystem.createProperty(pathSecurity)));
    boost::shared_ptr<PasswordChecker> checker(new BuiltinPasswordChecker());
    m_pSecurity->setPasswordChecker(checker);
    m_pSecurity->signOff();

    m_pSystemRole = m_PropertySystem.createProperty(pathSystemRole);
    m_pUserRole = m_PropertySystem.createProperty(pathUserRole);

    m_pUserNode = m_PropertySystem.createProperty(pathTestUser);
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

/**
 * TODO move this to security::init since it is how
 * the security is actually configured in the system
 */
void setupPrivileges(PropertySystem &propSys) {
  boost::shared_ptr<Privilege> privilegeSystem, privilegeNobody, privilegeUser;

  privilegeSystem.reset(new Privilege(propSys.getProperty(pathSystemRole)));
  privilegeSystem->addRight(Privilege::Write);

  privilegeUser.reset(new Privilege(propSys.getProperty(pathUserRole)));
  privilegeUser->addRight(Privilege::Write);

  privilegeNobody.reset(new Privilege(PropertyNodePtr()));

  boost::shared_ptr<NodePrivileges> privileges(new NodePrivileges());
  privileges->addPrivilege(privilegeSystem);
  privileges->addPrivilege(privilegeUser);
  privileges->addPrivilege(privilegeNobody);
  propSys.getProperty("/")->setPrivileges(privileges);

  /* security: passwords and credentials */
  boost::shared_ptr<Privilege> privilegeNobodySecurity(new Privilege(PropertyNodePtr()));

  boost::shared_ptr<NodePrivileges> privilegesSecurityNode(new NodePrivileges());
  privilegesSecurityNode->addPrivilege(privilegeNobodySecurity);
  propSys.getProperty(pathSecurity)->setPrivileges(privilegesSecurityNode);
}

class FixturePrivilegeTest : public FixtureTestUserTest {
public:
  FixturePrivilegeTest() : FixtureTestUserTest() {
    m_PropertySystem.createProperty("/folder")->createProperty("property");
    setupPrivileges(m_PropertySystem);
  }
};

BOOST_FIXTURE_TEST_CASE(testReadPrivilege, FixturePrivilegeTest) {
  BOOST_CHECK_NO_THROW(m_PropertySystem.getProperty("/folder/property"));
  m_pSecurity->authenticate("testuser", "test");
  BOOST_CHECK_NO_THROW(m_PropertySystem.getProperty("/folder/property"));
  m_pSecurity->signOff();
}

BOOST_FIXTURE_TEST_CASE(testReadPrivilegeSecurity, FixturePrivilegeTest) {
  BOOST_CHECK_NO_THROW(m_PropertySystem.getProperty(pathTestUser + "/password"));
  m_pSecurity->authenticate("testuser", "test");
  BOOST_CHECK_NO_THROW(m_PropertySystem.getProperty(pathTestUser));
  m_pSecurity->signOff();
}

BOOST_FIXTURE_TEST_CASE(testWritePrivilege, FixturePrivilegeTest) {
  BOOST_CHECK_THROW(m_PropertySystem.createProperty("/foo"), SecurityException);
  m_pSecurity->authenticate("testuser", "test");
  BOOST_CHECK_NO_THROW(m_PropertySystem.createProperty("/foo"));
  m_pSecurity->signOff();
}

BOOST_FIXTURE_TEST_CASE(testWritePrivilegeSecurity, FixturePrivilegeTest) {
  /* TODO, nobody has the right to create new users, probably a good thing */
  BOOST_CHECK_THROW(m_PropertySystem.createProperty(pathSecurity + "/users/evil_E"),
                    SecurityException);
  m_pSecurity->authenticate("testuser", "test");
  BOOST_CHECK_THROW(m_PropertySystem.createProperty(pathSecurity + "/users/new_user"),
                    SecurityException);
  m_pSecurity->signOff();
}

class FixtureSentinelTest : public FixtureTestUserTest {
public:
  FixtureSentinelTest() : FixtureTestUserTest() {
    PropertyNodePtr userNode, roleNode;
    boost::shared_ptr<User> sentinel;

    roleNode = m_PropertySystem.createProperty("/system/security/roles/sentinel");
    userNode = m_PropertySystem.createProperty("/system/security/users/sentinel");
    userNode->createProperty("role")->alias(roleNode);

    sentinel.reset(new User(userNode));
    sentinel->setPassword("sentinel");
    m_PropertySystem.createProperty("/readme");

    /* will not create privileges for role sentinel */
    setupPrivileges(m_PropertySystem);
    /* from now on need to logged in */
  }
};

BOOST_FIXTURE_TEST_CASE(testUserWithoutPrivilegesHasSameAccessAsNobody, FixtureSentinelTest) {
  BOOST_CHECK_NO_THROW(m_PropertySystem.getProperty("/readme"));
  m_pSecurity->authenticate("sentinel", "sentinel");
  BOOST_CHECK_NO_THROW(m_PropertySystem.getProperty("/readme"));
}

class FixtureSystemUser : public FixtureTestUserTest {
public:
  FixtureSystemUser() : FixtureTestUserTest() {
    PropertyNodePtr systemUserNode = m_PropertySystem.createProperty(pathSystemUser);
    systemUserNode->createProperty("role")->alias(m_pSystemRole);

    /* this will enable loginAsSystemUser */
    m_pSecurity->setSystemUser(new User(systemUserNode));

    /* test node */
    PropertyNodePtr pNode = m_PropertySystem.createProperty("/test");
    pNode->setStringValue("not modified");

    setupPrivileges(m_PropertySystem);
  }
};

BOOST_FIXTURE_TEST_CASE(testRolesWork, FixtureSystemUser) {
  PropertyNodePtr pNode = m_PropertySystem.getProperty("/test");

  BOOST_CHECK_THROW(pNode->setStringValue("Test"), SecurityException);
  BOOST_CHECK_EQUAL(pNode->getStringValue(), "not modified");

  m_pSecurity->loginAsSystemUser("unit tests");
  BOOST_CHECK_NO_THROW(pNode->setStringValue("Test"));
  BOOST_CHECK_EQUAL(pNode->getStringValue(), "Test");
}

BOOST_AUTO_TEST_SUITE_END()
