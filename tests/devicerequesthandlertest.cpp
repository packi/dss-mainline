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

#include <digitalSTROM/ds.h>
#include <digitalSTROM/dsuid.h>

#include "webfixture.h"

#include "src/model/apartment.h"
#include "src/web/handler/devicerequesthandler.h"
#include "src/model/device.h"
#include "src/model/zone.h"
#include "src/propertysystem.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(WebDevice)


class Fixture : public WebFixture {
public:
  Fixture() {
    m_pApartment.reset(new Apartment(NULL));
    m_pPropertySystem.reset(new PropertySystem());
    m_pApartment->setPropertySystem(m_pPropertySystem.get());
    m_pHandler.reset(new DeviceRequestHandler(*m_pApartment, NULL, NULL));

    memset(&m_ValidDSID, 0, 12);
    m_ValidDSID.id[12 - 1] =  1;
    m_ValidDSUID = DSUID_NULL;
    m_ValidDSUID.id[DSUID_SIZE - 1] =  1;
    m_ValidName = "device1";
    m_InvalidName = "nonexisting";
    m_ValidZoneID = 2;
    m_ValidGroupID = 1;
    m_FunctionID = 6;
    m_ProductID = 9;
    m_RevisionID = 3;

    boost::shared_ptr<Zone> firstZone = m_pApartment->allocateZone(m_ValidZoneID);
    boost::shared_ptr<Device> dev = m_pApartment->allocateDevice(m_ValidDSUID);
    DeviceReference devRef(dev, m_pApartment.get());
    firstZone->addDevice(devRef);
    dev->setName(m_ValidName);
    dev->addToGroup(m_ValidGroupID);
    dev->setFunctionID(m_FunctionID);
    dev->setProductID(m_ProductID);
    dev->setRevisionID(m_RevisionID);
    boost::shared_ptr<Device> dev2 = m_pApartment->allocateDevice(dsuid_from_dsid(m_ValidDSID));
    DeviceReference devRef2(dev2, m_pApartment.get());
    firstZone->addDevice(devRef2);
    dev2->setName(m_ValidName);
    dev2->addToGroup(m_ValidGroupID);
    dev2->setFunctionID(m_FunctionID);
    dev2->setProductID(m_ProductID);
    dev2->setRevisionID(m_RevisionID);
  }
protected:
  boost::shared_ptr<Apartment> m_pApartment;
  boost::shared_ptr<DeviceRequestHandler> m_pHandler;
  boost::shared_ptr<PropertySystem> m_pPropertySystem;
  dsid_t m_ValidDSID;
  dsuid_t m_ValidDSUID;
  std::string m_ValidName;
  std::string m_InvalidName;
  int m_ValidZoneID;
  int m_ValidGroupID;
  int m_FunctionID;
  int m_ProductID;
  int m_RevisionID;
};

BOOST_FIXTURE_TEST_CASE(testNoParam, Fixture) {
  RestfulRequest req("device/bla", "");
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testEmptyNameParam, Fixture) {
  RestfulRequest req("device/bla", "name=");
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testInvalidNameParam, Fixture) {
  RestfulRequest req("device/bla", "name=" + urlEncode(m_InvalidName));
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testEmptyDSIDParam, Fixture) {
  RestfulRequest req("device/bla", "dsid=");
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testInvalidDSIDParam, Fixture) {
  RestfulRequest req("device/bla", "dsid=asdfasdf");
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testInvalidDSUIDParam, Fixture) {
  RestfulRequest req("device/bla", "dsuid=asdfasdf");
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testTooLongDSIDParam, Fixture) {
  RestfulRequest req("device/bla",
                     "dsid=3504175fe0000000ffc000113504175fe0000000ffc00011");
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testTooLongDSUIDParam, Fixture) {
  RestfulRequest req("device/bla",
                     "dsuid=3504175fe0000000ffc000113504175fe0000000ffc00011");
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testRightDSID, Fixture) {
  std::string params = "dsid=" + urlEncode(dsid2str(m_ValidDSID));
  RestfulRequest req("device/bla", params);
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testRightDSUID, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/bla", params);
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testDeviceGetSpec, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/getSpec", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  checkPropertyEquals("functionID", m_FunctionID, response);
  checkPropertyEquals("revisionID", m_RevisionID, response);
  checkPropertyEquals("productID", m_ProductID, response);
}

BOOST_FIXTURE_TEST_CASE(testDeviceGetGroups, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/getGroups", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  BOOST_CHECK_EQUAL(response.getResponse(), "{\"result\":{\"groups\":[{\"id\":"+intToString(m_ValidGroupID)+",\"name\":\"yellow\"}]},\"ok\":true}");
}

BOOST_FIXTURE_TEST_CASE(testDeviceGetState, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/getState", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  checkPropertyEquals("isOn", false, response);
}

BOOST_FIXTURE_TEST_CASE(testDeviceGetName, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/getName", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  checkPropertyEquals("name", m_ValidName, response);
}

BOOST_FIXTURE_TEST_CASE(testDeviceSetNameMissingNewName, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/setName", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testDeviceSetName, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  std::string kNewName = "renamed";
  params += "&newName=" + urlEncode(kNewName);
  RestfulRequest req("device/setName", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);

  RestfulRequest req2("device/getName", params);
  WebServerResponse response2 = m_pHandler->jsonHandleRequest(req2, boost::shared_ptr<Session>());

  checkPropertyEquals("name", kNewName, response2);
}

BOOST_FIXTURE_TEST_CASE(testDeviceAddTagMissingTag, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/addTag", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testDeviceHasTagMissingTag, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/hasTag", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testDeviceRemoveTagMissingTag, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/removeTag", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testDeviceTestTags, Fixture) {
  const std::string& kTagName = "justATag";
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  params += "&tag=" + urlEncode(kTagName);
  RestfulRequest reqHasTag("device/hasTag", params);
  RestfulRequest reqAddTag("device/addTag", params);
  RestfulRequest reqRemoveTag("device/removeTag", params);

  // check that tag doesn't exist
  WebServerResponse response = m_pHandler->jsonHandleRequest(reqHasTag, boost::shared_ptr<Session>());
  checkPropertyEquals("hasTag", false, response);

  // add tag
  WebServerResponse response2 = m_pHandler->jsonHandleRequest(reqAddTag, boost::shared_ptr<Session>());
  testOkIs(response2, true);

  // check that tag does exist
  WebServerResponse response3 = m_pHandler->jsonHandleRequest(reqHasTag, boost::shared_ptr<Session>());
  checkPropertyEquals("hasTag", true, response3);

  // remove tag
  WebServerResponse response4 = m_pHandler->jsonHandleRequest(reqRemoveTag, boost::shared_ptr<Session>());
  testOkIs(response4, true);

  // check that tag doesn't exist
  WebServerResponse response5 = m_pHandler->jsonHandleRequest(reqHasTag, boost::shared_ptr<Session>());
  checkPropertyEquals("hasTag", false, response5);
}

BOOST_FIXTURE_TEST_CASE(testDeviceGetTagsNoTags, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/getTags", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());

  checkPropertyEquals("tags", std::string("[ ]"), response);
}

BOOST_FIXTURE_TEST_CASE(testDeviceGetTagsOneTag, Fixture) {
  const std::string& kTagName = "testing";
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/getTags", params);
  m_pApartment->getDeviceByDSID(m_ValidDSUID)->addTag(kTagName);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());

  checkPropertyEquals("tags", "[ \""+kTagName+"\" ]", response);
}

BOOST_FIXTURE_TEST_CASE(testEnable, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/enable", params);
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testDisable, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/disable", params);
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testLockNotPresent, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/lock", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testLockPresent, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/lock", params);
  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsConnected(true);
  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsPresent(true);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);
}

BOOST_FIXTURE_TEST_CASE(testUnlockNotPresent, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/unlock", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testUnlockPresent, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/unlock", params);
  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsConnected(true);
  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsPresent(true);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);
}

BOOST_FIXTURE_TEST_CASE(testSetConfigNoParameters, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/setConfig", params);
  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsPresent(true);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testSetConfigInvalidParameters, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  params += "&class=469";
  params += "&index=452";
  RestfulRequest req("device/setConfig", params);

  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsPresent(true);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testSetConfig, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  params += "&class=3";
  params += "&index=0";
  params += "&value=21";
  RestfulRequest req("device/setConfig", params);

  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsPresent(true);
  // no ds485 bus present, exception is a good thing
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()),
                    std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testGetConfigInvalidParameters, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  params += "&class=469";
  params += "&index=452";
  RestfulRequest req("device/getConfig", params);

  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsPresent(true);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testGetConfigNoParameters, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/getConfig", params);

  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsPresent(true);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testGetConfig, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  params += "&class=3";
  params += "&index=0";
  RestfulRequest req("device/getConfig", params);

  // TODO: setup bus interface
  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsPresent(true);
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testGetConfigWordInvalidParameters, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  params += "&class=469";
  params += "&index=452";

  RestfulRequest req("device/getConfigWord", params);
  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsPresent(true);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testGetConfigWordNoParameters, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/getConfigWord", params);
  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsPresent(true);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testGetConfigWord, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  params += "&class=3";
  params += "&index=0";
  RestfulRequest req("device/getConfigWord", params);
  // TODO: setup bus interface
  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsPresent(true);
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testGetTransmissionQuality, Fixture) {
  std::string params = "dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("device/getTransmissionQuality", params);
  // TODO: setup bus interface
  m_pApartment->getDeviceByDSID(m_ValidDSUID)->setIsPresent(true);
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
