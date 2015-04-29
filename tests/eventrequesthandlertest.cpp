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

#include <boost/lambda/lambda.hpp>
#include <boost/make_shared.hpp>

#include "webfixture.h"

#include "src/event.h"
#include "src/web/handler/eventrequesthandler.h"
#include "src/eventinterpreterplugins.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(WebEvent)

class Fixture : public WebFixture {
public:
  Fixture() {
    m_pInterpreter.reset(new EventInterpreter(NULL));
    m_pInterpreter->initialize();
    m_pQueue.reset(new EventQueue(m_pInterpreter.get(), 2));
    m_pRunner.reset(new EventRunner(m_pInterpreter.get()));
    m_pInterpreter->setEventQueue(m_pQueue.get());
    m_pInterpreter->setEventRunner(m_pRunner.get());
    m_pInterpreter->addPlugin(new EventInterpreterInternalRelay(m_pInterpreter.get()));

    m_pSession.reset(new Session("1"));
    m_pHandler.reset(new EventRequestHandler(*m_pInterpreter));
  }

protected:
  boost::shared_ptr<EventQueue> m_pQueue;
  boost::shared_ptr<EventRunner> m_pRunner;
  boost::shared_ptr<EventInterpreter> m_pInterpreter;
  boost::shared_ptr<EventRequestHandler> m_pHandler;
  boost::shared_ptr<Session> m_pSession;
  std::string m_Params;
}; // Fixture

BOOST_FIXTURE_TEST_CASE(testInvalidFunction, Fixture) {
  RestfulRequest req("event/asdf", m_Params);
  BOOST_CHECK_THROW(m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>()), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(testRaiseMissingEventName, Fixture) {
  RestfulRequest req("event/raise", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testRaise, Fixture) {
  const std::string kEventName = "my_test_event";
  const std::string kEventLocation = "my_location";
  const std::string kEventContext = "my_context";
  m_Params += "&name=" + urlEncode(kEventName);
  m_Params += "&location=" + urlEncode(kEventLocation);
  m_Params += "&context=" + urlEncode(kEventContext);
  RestfulRequest req("event/raise", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);

  boost::shared_ptr<Event> pEvent = m_pQueue->popEvent();
  BOOST_CHECK(pEvent != NULL);
  BOOST_CHECK_EQUAL(pEvent->getName(), kEventName);
  BOOST_CHECK_EQUAL(pEvent->getPropertyByName("location"), kEventLocation);
  BOOST_CHECK_EQUAL(pEvent->getPropertyByName("context"), kEventContext);

  pEvent = m_pQueue->popEvent();
  BOOST_CHECK(pEvent == NULL);
}

BOOST_FIXTURE_TEST_CASE(testRaiseWithParams, Fixture) {
  const std::string kEventName = "my_test_event";
  const std::string kParamName = "param1";
  const std::string kParamValue = "aValue";
  m_Params += "&name=" + urlEncode(kEventName);
  m_Params += "&parameter=" + urlEncode(kParamName + "=" + kParamValue);
  RestfulRequest req("event/raise", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);

  boost::shared_ptr<Event> pEvent = m_pQueue->popEvent();
  BOOST_CHECK(pEvent != NULL);
  BOOST_CHECK_EQUAL(pEvent->getName(), kEventName);
  BOOST_CHECK_EQUAL(pEvent->getPropertyByName(kParamName), kParamValue);

  pEvent = m_pQueue->popEvent();
  BOOST_CHECK(pEvent == NULL);
}

BOOST_FIXTURE_TEST_CASE(testRaiseWithParamsNoValue, Fixture) {
  const std::string kEventName = "my_test_event";
  const std::string kParamName = "myParam";
  m_Params += "&name=" + urlEncode(kEventName);
  m_Params += "&parameter=" + urlEncode(kParamName);
  RestfulRequest req("event/raise", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);

  boost::shared_ptr<Event> pEvent = m_pQueue->popEvent();
  BOOST_CHECK(pEvent == NULL);
}

BOOST_FIXTURE_TEST_CASE(testRaiseWithParamsEmptyName, Fixture) {
  const std::string kEventName = "my_test_event";
  const std::string kParamName = "";
  const std::string kParamValue = "aValue";
  m_Params += "&name=" + urlEncode(kEventName);
  m_Params += "&parameter=" + urlEncode(kParamName + "=" + kParamValue);
  RestfulRequest req("event/raise", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);

  boost::shared_ptr<Event> pEvent = m_pQueue->popEvent();
  BOOST_CHECK(pEvent == NULL);
}

BOOST_FIXTURE_TEST_CASE(testRaiseWithParamsEmptyValue, Fixture) {
  const std::string kEventName = "my_test_event";
  const std::string kParamName = "param1";
  const std::string kParamValue = "";
  m_Params += "&name=" + urlEncode(kEventName);
  m_Params += "&parameter=" + urlEncode(kParamName + "=" + kParamValue);
  RestfulRequest req("event/raise", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);

  boost::shared_ptr<Event> pEvent = m_pQueue->popEvent();
  BOOST_CHECK(pEvent == NULL);
}

BOOST_FIXTURE_TEST_CASE(testRaiseWithParamsAndEqualSigns, Fixture) {
  const std::string kEventName = "my_test_event";
  const std::string kParamName1 = "param1";
  const std::string kParamValue1 = "aValue=someOtherValue";
  const std::string kParamName2 = "param2";
  const std::string kParamValue2 = "aValue2";
  m_Params += "&name=" + urlEncode(kEventName);
  m_Params += "&parameter=" + urlEncode(kParamName1 + "=" + kParamValue1 + ";"
                                        + kParamName2 + "=" + kParamValue2);
  RestfulRequest req("event/raise", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, true);

  boost::shared_ptr<Event> pEvent = m_pQueue->popEvent();
  BOOST_CHECK(pEvent != NULL);
  BOOST_CHECK_EQUAL(pEvent->getName(), kEventName);
  BOOST_CHECK_EQUAL(pEvent->getPropertyByName(kParamName1), kParamValue1);
  BOOST_CHECK_EQUAL(pEvent->getPropertyByName(kParamName2), kParamValue2);

  pEvent = m_pQueue->popEvent();
  BOOST_CHECK(pEvent == NULL);
}

BOOST_FIXTURE_TEST_CASE(testSubscribeNoSession, Fixture) {
  RestfulRequest req("event/subscribe", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testSubscribeNoName, Fixture) {
  RestfulRequest req("event/subscribe", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, m_pSession);
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testSubscribeNoSubscriptionID, Fixture) {
  const std::string kEventName = "someEvent";
  m_Params += "&name=" + urlEncode(kEventName);
  RestfulRequest req("event/subscribe", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, m_pSession);
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testSubscribeInvalidSubscriptionID, Fixture) {
  const std::string kEventName = "someEvent";
  const std::string kSubscriptionID = "asdfasdf";
  m_Params += "&name=" + urlEncode(kEventName);
  m_Params += "&subscriptionID=" + urlEncode(kSubscriptionID);
  RestfulRequest req("event/subscribe", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, m_pSession);
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testSubscribe, Fixture) {
  const std::string kEventName = "someEvent";
  const std::string kSubscriptionID = "2";
  m_Params += "&name=" + urlEncode(kEventName);
  m_Params += "&subscriptionID=" + urlEncode(kSubscriptionID);
  RestfulRequest req("event/subscribe", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, m_pSession);
  testOkIs(response, true);
}

BOOST_FIXTURE_TEST_CASE(testUnsubscribeNoSession, Fixture) {
  RestfulRequest req("event/unsubscribe", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testUnsubscribeNoName, Fixture) {
  RestfulRequest req("event/unsubscribe", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, m_pSession);
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testUnubscribeNoSubscriptionID, Fixture) {
  const std::string kEventName = "someEvent";
  m_Params += "&name=" + urlEncode(kEventName);
  RestfulRequest req("event/unsubscribe", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, m_pSession);
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testUnsubscribeInvalidSubscriptionID, Fixture) {
  const std::string kEventName = "someEvent";
  const std::string kSubscriptionID = "asdfasdf";
  m_Params += "&name=" + urlEncode(kEventName);
  m_Params += "&subscriptionID=" + urlEncode(kSubscriptionID);
  RestfulRequest req("event/unsubscribe", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, m_pSession);
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testGetNoSession, Fixture) {
  RestfulRequest req("event/get", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, boost::shared_ptr<Session>());
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testGetNoName, Fixture) {
  RestfulRequest req("event/get", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, m_pSession);
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testgetNoSubscriptionID, Fixture) {
  const std::string kEventName = "someEvent";
  m_Params += "&name=" + urlEncode(kEventName);
  RestfulRequest req("event/get", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, m_pSession);
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testGetInvalidSubscriptionID, Fixture) {
  const std::string kEventName = "someEvent";
  const std::string kSubscriptionID = "asdfasdf";
  m_Params += "&name=" + urlEncode(kEventName);
  m_Params += "&subscriptionID=" + urlEncode(kSubscriptionID);
  RestfulRequest req("event/get", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(req, m_pSession);
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testSubscriptionsWork, Fixture) {
  const std::string kEventName = "someEvent";
  const std::string kSubscriptionID = "2";
  m_Params += "&name=" + urlEncode(kEventName);
  m_Params += "&subscriptionID=" + urlEncode(kSubscriptionID);
  m_Params += "&timeout=5";
  RestfulRequest reqSubscribe("event/subscribe", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(reqSubscribe, m_pSession);
  testOkIs(response, true);

  boost::shared_ptr<Event> pUnrelatedEvent = boost::make_shared<Event>("asdfasdf");
  m_pQueue->pushEvent(pUnrelatedEvent);
  boost::shared_ptr<Event> pEvent = boost::make_shared<Event>(kEventName);
  m_pQueue->pushEvent(pEvent);

  m_pInterpreter->executePendingEvent();
  m_pInterpreter->executePendingEvent();

  RestfulRequest reqGet("event/get", m_Params);
  response = m_pHandler->jsonHandleRequest(reqGet, m_pSession);
  testOkIs(response, true);

  checkPropertyEquals("events", "[ { \"name\": \"" + kEventName + "\", \"properties\": { }, \"source\": { } } ]", response);

  RestfulRequest reqUnsubscribe("event/unsubscribe", m_Params);
  response = m_pHandler->jsonHandleRequest(reqUnsubscribe, m_pSession);
  testOkIs(response, true);

  m_pQueue->pushEvent(pUnrelatedEvent);
  m_pQueue->pushEvent(pEvent);

  sleepMS(5);

  response = m_pHandler->jsonHandleRequest(reqGet, m_pSession);
  testOkIs(response, false);
}

BOOST_FIXTURE_TEST_CASE(testGetReturnsIfConnectionIsInterrupted, Fixture) {
  const std::string kEventName = "someEvent";
  const std::string kSubscriptionID = "2";
  m_Params += "&name=" + urlEncode(kEventName);
  m_Params += "&subscriptionID=" + urlEncode(kSubscriptionID);
  m_Params += "&timeout=50000";
  RestfulRequest reqSubscribe("event/subscribe", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(reqSubscribe, m_pSession);
  testOkIs(response, true);

  DateTime reqStarted;
  RestfulRequest reqGet("event/get", m_Params);
  reqGet.setActiveCallback(boost::lambda::constant(false));
  response = m_pHandler->jsonHandleRequest(reqGet, m_pSession);
  testOkIs(response, true);
  DateTime reqEnded;
  BOOST_CHECK_SMALL(reqEnded.difference(reqStarted), 3);
}

BOOST_FIXTURE_TEST_CASE(testGetDoesntReturnIfConnectionIsNotInterrupted, Fixture) {
  const std::string kEventName = "someEvent";
  const std::string kSubscriptionID = "2";
  m_Params += "&name=" + urlEncode(kEventName);
  m_Params += "&subscriptionID=" + urlEncode(kSubscriptionID);
  m_Params += "&timeout=1000";
  RestfulRequest reqSubscribe("event/subscribe", m_Params);
  WebServerResponse response = m_pHandler->jsonHandleRequest(reqSubscribe, m_pSession);
  testOkIs(response, true);

  DateTime reqStarted;
  RestfulRequest reqGet("event/get", m_Params);
  reqGet.setActiveCallback(boost::lambda::constant(true));
  response = m_pHandler->jsonHandleRequest(reqGet, m_pSession);
  testOkIs(response, true);
  DateTime reqEnded;
  BOOST_CHECK_SMALL(reqEnded.difference(reqStarted), 2);
}

BOOST_AUTO_TEST_SUITE_END()
