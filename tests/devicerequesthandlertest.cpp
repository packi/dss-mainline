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

#include "webfixture.h"

#include "core/model/apartment.h"
#include "core/web/handler/devicerequesthandler.h"
#include "core/model/device.h"
#include "core/model/zone.h"
#include "core/propertysystem.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(WebDevice)


class Fixture : public WebFixture {
public:
  Fixture() {
    m_pApartment.reset(new Apartment(NULL));
    m_pPropertySystem.reset(new PropertySystem());
    m_pApartment->setPropertySystem(m_pPropertySystem.get());
    m_pHandler.reset(new DeviceRequestHandler(*m_pApartment, NULL, NULL));
    m_ValidDSID = dss_dsid_t(0, 0x1234);
    m_ValidName = "device1";
    m_InvalidDSID = dss_dsid_t(0, 0xdeadbeef);
    m_InvalidName = "nonexisting";
    m_ValidZoneID = 2;
    m_ValidGroupID = 1;
    m_FunctionID = 6;
    m_ProductID = 9;
    m_RevisionID = 3;

    boost::shared_ptr<Zone> firstZone = m_pApartment->allocateZone(m_ValidZoneID);
    boost::shared_ptr<Device> dev = m_pApartment->allocateDevice(m_ValidDSID);
    DeviceReference devRef(dev, m_pApartment.get());
    firstZone->addDevice(devRef);
    dev->setName(m_ValidName);
    dev->addToGroup(m_ValidGroupID);
    dev->setFunctionID(m_FunctionID);
    dev->setProductID(m_ProductID);
    dev->setRevisionID(m_RevisionID);
  }
protected:
  boost::shared_ptr<Apartment> m_pApartment;
  boost::shared_ptr<DeviceRequestHandler> m_pHandler;
  boost::shared_ptr<PropertySystem> m_pPropertySystem;
  dss_dsid_t m_ValidDSID;
  dss_dsid_t m_InvalidDSID;
  std::string m_ValidName;
  std::string m_InvalidName;
  int m_ValidZoneID;
  int m_ValidGroupID;
  int m_FunctionID;
  int m_ProductID;
  int m_RevisionID;
};

BOOST_FIXTURE_TEST_CASE(testNoParam, Fixture) {
  HashMapConstStringString params;
  RestfulRequest req("device/bla", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testEmptyNameParam, Fixture) {
  HashMapConstStringString params;
  params["name"] = "";
  RestfulRequest req("device/bla", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testInvalidNameParam, Fixture) {
  HashMapConstStringString params;
  params["name"] = m_InvalidName;
  RestfulRequest req("device/bla", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testEmptyDSIDParam, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = "";
  RestfulRequest req("device/bla", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testInvalidDSIDParam, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = "asdfasdf";
  RestfulRequest req("device/bla", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testTooLongDSIDParam, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = "3504175fe0000000ffc000113504175fe0000000ffc00011";
  RestfulRequest req("device/bla", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testRightDSID, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/bla", params);
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testDeviceGetSpec, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/getSpec", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  boost::shared_ptr<JSONObject> result = getResultObject(response);
  checkPropertyEquals("functionID", m_FunctionID, result);
  checkPropertyEquals("revisionID", m_RevisionID, result);
  checkPropertyEquals("productID", m_ProductID, result);
}

BOOST_FIXTURE_TEST_CASE(testDeviceGetGroups, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/getGroups", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  boost::shared_ptr<JSONObject> result = getResultObject(response);
  boost::shared_ptr<JSONElement> groupsArr = result->getElementByName("groups");
  BOOST_CHECK_EQUAL(groupsArr->getElementCount(), 1);
  boost::shared_ptr<JSONObject> groupObj = boost::dynamic_pointer_cast<JSONObject>(groupsArr->getElement(0));
  BOOST_CHECK(groupObj != NULL);
  checkPropertyEquals("id", m_ValidGroupID, groupObj);
  checkPropertyEquals("name", std::string("yellow"), groupObj);
}

BOOST_FIXTURE_TEST_CASE(testDeviceGetState, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/getState", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  boost::shared_ptr<JSONObject> result = getResultObject(response);
  checkPropertyEquals("isOn", false, result);
}

BOOST_FIXTURE_TEST_CASE(testDeviceGetName, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/getName", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  boost::shared_ptr<JSONObject> result = getResultObject(response);
  checkPropertyEquals("name", m_ValidName, result);
}

BOOST_FIXTURE_TEST_CASE(testDeviceSetNameMissingNewName, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/setName", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testDeviceSetName, Fixture) {
  const std::string& kNewName = "renamed";
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  params["newName"] = kNewName;
  RestfulRequest req("device/setName", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);

  RestfulRequest req2("device/getName", params);
  WebServerResponse response2 = m_pHandler->jsonHandleRequest(req2, boost::shared_ptr<Session>());

  boost::shared_ptr<JSONObject> result = getResultObject(response2);
  checkPropertyEquals("name", kNewName, result);
}

BOOST_FIXTURE_TEST_CASE(testDeviceAddTagMissingTag, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/addTag", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testDeviceHasTagMissingTag, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/hasTag", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testDeviceRemoveTagMissingTag, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/removeTag", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testDeviceTestTags, Fixture) {
  const std::string& kTagName = "justATag";
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  params["tag"] = kTagName;
  RestfulRequest reqHasTag("device/hasTag", params);
  RestfulRequest reqAddTag("device/addTag", params);
  RestfulRequest reqRemoveTag("device/removeTag", params);
  boost::shared_ptr<JSONObject> result;

  // check that tag doesn't exist
  WebServerResponse response = m_pHandler->jsonHandleRequest(reqHasTag, boost::shared_ptr<Session>());
  result = getResultObject(response);
  checkPropertyEquals("hasTag", false, result);

  // add tag
  WebServerResponse response2 = m_pHandler->jsonHandleRequest(reqAddTag, boost::shared_ptr<Session>());
  testOkIs(response2, true);

  // check that tag does exist
  WebServerResponse response3 = m_pHandler->jsonHandleRequest(reqHasTag, boost::shared_ptr<Session>());
  result = getResultObject(response3);
  checkPropertyEquals("hasTag", true, result);

  // remove tag
  WebServerResponse response4 = m_pHandler->jsonHandleRequest(reqRemoveTag, boost::shared_ptr<Session>());
  testOkIs(response4, true);

  // check that tag doesn't exist
  WebServerResponse response5 = m_pHandler->jsonHandleRequest(reqHasTag, boost::shared_ptr<Session>());
  result = getResultObject(response5);
  checkPropertyEquals("hasTag", false, result);
}

BOOST_FIXTURE_TEST_CASE(testDeviceGetTagsNoTags, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/getTags", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  boost::shared_ptr<JSONObject> result;

  result = getResultObject(response);
  boost::shared_ptr<JSONElement> tagsArray = result->getElementByName("tags");
  BOOST_CHECK(tagsArray != NULL);
  BOOST_CHECK_EQUAL(tagsArray->getElementCount(), 0);
}

BOOST_FIXTURE_TEST_CASE(testDeviceGetTagsOneTag, Fixture) {
  const std::string& kTagName = "testing";
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  m_pApartment->getDeviceByDSID(m_ValidDSID)->addTag(kTagName);
  RestfulRequest req("device/getTags", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  boost::shared_ptr<JSONObject> result;

  result = getResultObject(response);
  boost::shared_ptr<JSONElement> tagsArray = result->getElementByName("tags");
  BOOST_CHECK(tagsArray != NULL);
  BOOST_CHECK_EQUAL(tagsArray->getElementCount(), 1);
  boost::shared_ptr<JSONValue<std::string> > strValue = boost::dynamic_pointer_cast<JSONValue<std::string> >(tagsArray->getElement(0));
  BOOST_CHECK_EQUAL(strValue->getValue(), kTagName);
}

BOOST_FIXTURE_TEST_CASE(testEnable, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/enable", params);
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testDisable, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/disable", params);
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testLockNotPresent, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/lock", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testLockPresent, Fixture) {
  HashMapConstStringString params;
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsConnected(true);
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsPresent(true);
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/lock", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);
}

BOOST_FIXTURE_TEST_CASE(testUnlockNotPresent, Fixture) {
  HashMapConstStringString params;
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/unlock", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testUnlockPresent, Fixture) {
  HashMapConstStringString params;
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsConnected(true);
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsPresent(true);
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/unlock", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);
}

BOOST_FIXTURE_TEST_CASE(testSetConfigNoParameters, Fixture) {
  HashMapConstStringString params;
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsPresent(true);
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/setConfig", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testSetConfigInvalidParameters, Fixture) {
  HashMapConstStringString params;
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsPresent(true);
  params["dsid"] = m_ValidDSID.toString();
  params["class"] = "469";
  params["index"] = "452";
  RestfulRequest req("device/setConfig", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testSetConfig, Fixture) {
  HashMapConstStringString params;
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsPresent(true);
  params["dsid"] = m_ValidDSID.toString();
  params["class"] = "3";
  params["index"] = "0";
  params["value"] = "21";
  RestfulRequest req("device/setConfig", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);
}

BOOST_FIXTURE_TEST_CASE(testGetConfigInvalidParameters, Fixture) {
  HashMapConstStringString params;
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsPresent(true);
  params["dsid"] = m_ValidDSID.toString();
  params["class"] = "469";
  params["index"] = "452";
  RestfulRequest req("device/getConfig", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testGetConfigNoParameters, Fixture) {
  HashMapConstStringString params;
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsPresent(true);
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/getConfig", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testGetConfig, Fixture) {
  HashMapConstStringString params;
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsPresent(true);
  params["dsid"] = m_ValidDSID.toString();
  params["class"] = "3";
  params["index"] = "0";
  RestfulRequest req("device/getConfig", params);
  // TODO: setup bus interface
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testGetConfigWordInvalidParameters, Fixture) {
  HashMapConstStringString params;
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsPresent(true);
  params["dsid"] = m_ValidDSID.toString();
  params["class"] = "469";
  params["index"] = "452";
  RestfulRequest req("device/getConfigWord", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testGetConfigWordNoParameters, Fixture) {
  HashMapConstStringString params;
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsPresent(true);
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/getConfigWord", params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testGetConfigWord, Fixture) {
  HashMapConstStringString params;
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsPresent(true);
  params["dsid"] = m_ValidDSID.toString();
  params["class"] = "3";
  params["index"] = "0";
  RestfulRequest req("device/getConfigWord", params);
  // TODO: setup bus interface
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testGetTransmissionQuality, Fixture) {
  HashMapConstStringString params;
  m_pApartment->getDeviceByDSID(m_ValidDSID)->setIsPresent(true);
  params["dsid"] = m_ValidDSID.toString();
  RestfulRequest req("device/getTransmissionQuality", params);
  // TODO: setup bus interface
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
