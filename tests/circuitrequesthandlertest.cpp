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
#include <digitalSTROM/dsuid/dsuid.h>

#include "webfixture.h"

#include "src/model/apartment.h"
#include "src/model/modulator.h"
#include "src/model/modelmaintenance.h"
#include "src/web/handler/circuitrequesthandler.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(WebCircuit)

class Fixture : public WebFixture {
public:
  Fixture() {
    SetNullDsuid(m_ValidDSUID);
    m_ValidDSUID.id[DSUID_SIZE-1] = 1;
    m_ValidDSID.id[12-1] = 1;
    m_ValidName = "mod";

    m_pApartment.reset(new Apartment(NULL));
    boost::shared_ptr<DSMeter> mod = m_pApartment->allocateDSMeter(m_ValidDSUID);
    mod->setName(m_ValidName);
    mod->setCapability_HasMetering(true);
    m_pHandler.reset(new CircuitRequestHandler(*m_pApartment, NULL, NULL));
  }
protected:
  boost::shared_ptr<Apartment> m_pApartment;
  boost::shared_ptr<CircuitRequestHandler> m_pHandler;
  std::string m_Params;
  dsuid_t m_ValidDSUID;
  dsid_t m_ValidDSID;
  std::string m_ValidName;
}; // Fixture

BOOST_FIXTURE_TEST_CASE(testMissingID, Fixture) {
  RestfulRequest req("circuit/bla", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testInvalidID, Fixture) {
  m_Params += "&id=asdfasdf";
  RestfulRequest req("circuit/bla", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testInvalidDSUID, Fixture) {
  m_Params += "&dsuid=asdfasdf";
  RestfulRequest req("circuit/bla", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

//BOOST_FIXTURE_TEST_CASE(testInvalidFunctionValidID, Fixture) {
//  m_Params += "&id=" + urlEncode(dsid2str(m_ValidDSID));
//  RestfulRequest req("circuit/adasfaf", m_Params);
//  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
//}

BOOST_FIXTURE_TEST_CASE(testInvalidFunctionValidDSUID, Fixture) {
  m_Params += "&dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("circuit/adasfaf", m_Params);
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testValidID, Fixture) {
  m_Params += "&dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("circuit/getName", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);
}

BOOST_FIXTURE_TEST_CASE(testCircuitGetName, Fixture) {
  m_Params += "&dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("circuit/getName", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  boost::shared_ptr<JSONObject> result = getResultObject(response);
  checkPropertyEquals("name", m_ValidName, result);
}

BOOST_FIXTURE_TEST_CASE(testCircuitSetNameMissingNewName, Fixture) {
  m_Params += "&dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("circuit/setName", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testCircuitSetName, Fixture) {
  const std::string& kNewName = "renamed";
  m_Params += "&dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  m_Params += "&newName=" + urlEncode(kNewName);
  RestfulRequest req("circuit/setName", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);

  RestfulRequest req2("circuit/getName", m_Params);
  WebServerResponse response2 = m_pHandler->jsonHandleRequest(req2, boost::shared_ptr<Session>());

  boost::shared_ptr<JSONObject> result = getResultObject(response2);
  checkPropertyEquals("name", kNewName, result);
}

BOOST_FIXTURE_TEST_CASE(testCircuitRescan, Fixture) {
  m_Params += "&dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  RestfulRequest req("circuit/rescan", m_Params);
  m_pApartment->getDSMeterByDSID(m_ValidDSUID)->setIsValid(true);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);

  BOOST_CHECK_EQUAL(m_pApartment->getDSMeterByDSID(m_ValidDSUID)->isValid(), false);
}

BOOST_FIXTURE_TEST_CASE(testGetPowerConsumption, Fixture) {
  m_Params += "&dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  const long unsigned int kConsumption = 77;
  const long unsigned int kNullConsumption = 0;

  RestfulRequest req("circuit/getConsumption", m_Params);
  m_pApartment->getDSMeterByDSID(m_ValidDSUID)->setPowerConsumption(kConsumption);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  boost::shared_ptr<JSONObject> result = getResultObject(response);
  checkPropertyEquals("consumption", kConsumption, result);

  // verify that power consumption for inactive devices is zero
  sleep(2);
  RestfulRequest req2("circuit/getConsumption", m_Params);
  WebServerResponse response2 = m_pHandler->jsonHandleRequest(req2, boost::shared_ptr<Session>());
  boost::shared_ptr<JSONObject> result2 = getResultObject(response2);
  checkPropertyEquals("consumption", kNullConsumption, result2);
}

BOOST_FIXTURE_TEST_CASE(testGetEnergyMeterValue, Fixture) {
  m_Params += "&dsuid=" + urlEncode(dsuid2str(m_ValidDSUID));
  const long long unsigned int kMeterValue = 8888888;
  m_pApartment->getDSMeterByDSID(m_ValidDSUID)->initializeEnergyMeterValue(kMeterValue);
  RestfulRequest req("circuit/getEnergyMeterValue", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());

  boost::shared_ptr<JSONObject> result = getResultObject(response);
  checkPropertyEquals("meterValue", kMeterValue, result);
}

BOOST_AUTO_TEST_SUITE_END()
