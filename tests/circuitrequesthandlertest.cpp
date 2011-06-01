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
#include "core/model/modulator.h"
#include "core/model/modelmaintenance.h"
#include "core/web/handler/circuitrequesthandler.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(WebCircuit)

class Fixture : public WebFixture {
public:
  Fixture() {
    m_ValidDSID = dss_dsid_t(0, 1);
    m_ValidName = "mod";
    m_BorderOrange = 55;
    m_BorderRed = 100;

    m_pApartment.reset(new Apartment(NULL));
    boost::shared_ptr<DSMeter> mod = m_pApartment->allocateDSMeter(m_ValidDSID);
    mod->setName(m_ValidName);
    mod->setEnergyLevelOrange(m_BorderOrange);
    mod->setEnergyLevelRed(m_BorderRed);
    m_pMaintenance.reset(new ModelMaintenance(NULL));
    m_pHandler.reset(new CircuitRequestHandler(*m_pApartment, *m_pMaintenance,
                                               NULL, NULL));
  }
protected:
  boost::shared_ptr<Apartment> m_pApartment;
  boost::shared_ptr<ModelMaintenance> m_pMaintenance;
  boost::shared_ptr<CircuitRequestHandler> m_pHandler;
  HashMapConstStringString m_Params;
  dss_dsid_t m_ValidDSID;
  std::string m_ValidName;
  int m_BorderOrange;
  int m_BorderRed;
}; // Fixture

BOOST_FIXTURE_TEST_CASE(testMissingID, Fixture) {
  RestfulRequest req("circuit/bla", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testInvalidID, Fixture) {
  m_Params["id"] = "asdfasdf";
  RestfulRequest req("circuit/bla", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testInvalidFunctionValidID, Fixture) {
  m_Params["id"] = m_ValidDSID.toString();
  RestfulRequest req("circuit/asdf", m_Params);
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testValidID, Fixture) {
  m_Params["id"] = m_ValidDSID.toString();
  RestfulRequest req("circuit/getName", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);
}

BOOST_FIXTURE_TEST_CASE(testCircuitGetName, Fixture) {
  m_Params["id"] = m_ValidDSID.toString();
  RestfulRequest req("circuit/getName", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  boost::shared_ptr<JSONObject> result = getResultObject(response);
  checkPropertyEquals("name", m_ValidName, result);
}

BOOST_FIXTURE_TEST_CASE(testCircuitSetNameMissingNewName, Fixture) {
  m_Params["id"] = m_ValidDSID.toString();
  RestfulRequest req("circuit/setName", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testCircuitSetName, Fixture) {
  const std::string& kNewName = "renamed";
  m_Params["id"] = m_ValidDSID.toString();
  m_Params["newName"] = kNewName;
  RestfulRequest req("circuit/setName", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);

  RestfulRequest req2("circuit/getName", m_Params);
  WebServerResponse response2 = m_pHandler->jsonHandleRequest(req2, boost::shared_ptr<Session>());

  boost::shared_ptr<JSONObject> result = getResultObject(response2);
  checkPropertyEquals("name", kNewName, result);
}

BOOST_FIXTURE_TEST_CASE(testGetEnergyBorder, Fixture) {
  m_Params["id"] = m_ValidDSID.toString();
  RestfulRequest req("circuit/getEnergyBorder", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());

  boost::shared_ptr<JSONObject> result = getResultObject(response);
  checkPropertyEquals("orange", m_BorderOrange, result);
  checkPropertyEquals("red", m_BorderRed, result);
}

BOOST_FIXTURE_TEST_CASE(testCircuitRescan, Fixture) {
  m_Params["id"] = m_ValidDSID.toString();
  m_pApartment->getDSMeterByDSID(m_ValidDSID)->setIsValid(true);
  RestfulRequest req("circuit/rescan", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);

  BOOST_CHECK_EQUAL(m_pApartment->getDSMeterByDSID(m_ValidDSID)->isValid(), false);
}

BOOST_FIXTURE_TEST_CASE(testGetPowerConsumption, Fixture) {
  m_Params["id"] = m_ValidDSID.toString();
  const long unsigned int kConsumption = 77;
  m_pApartment->getDSMeterByDSID(m_ValidDSID)->setPowerConsumption(kConsumption);
  RestfulRequest req("circuit/getConsumption", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());

  boost::shared_ptr<JSONObject> result = getResultObject(response);
  checkPropertyEquals("consumption", kConsumption, result);
}

BOOST_FIXTURE_TEST_CASE(testGetEnergyMeterValue, Fixture) {
  m_Params["id"] = m_ValidDSID.toString();
  const long unsigned int kMeterValue = 8888888;
  m_pApartment->getDSMeterByDSID(m_ValidDSID)->setEnergyMeterValue(kMeterValue);
  RestfulRequest req("circuit/getEnergyMeterValue", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());

  boost::shared_ptr<JSONObject> result = getResultObject(response);
  checkPropertyEquals("meterValue", kMeterValue, result);
}

BOOST_AUTO_TEST_SUITE_END()
