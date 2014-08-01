#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>
#include <curl/curl.h>
#include <iostream>

#include "eventinterpreterplugins.h"
#include "http_client.h"
#include "src/propertysystem.h"
#include "src/dss.h"
#include "src/event.h"
#include "sessionmanager.h"
#include "unix/systeminfo.h"
#include "webservice_api.h"
#include "tests/dss_life_cycle.h"

using namespace dss;

static const char *websvc_url_authority_test = "https://testdsservices.aizo.com/";
static const char *websvc_osp_token_test = "76sdf9786f7d6fsd78f6asd7f6sd78f6sdf896as";

BOOST_AUTO_TEST_SUITE(WebserviceTest)

BOOST_AUTO_TEST_CASE(parseModelChangeTest) {

  /* test sample response */
  WebserviceReply resp;
  {
    const char *buf =  "{\"ReturnCode\":2,\"ReturnMessage\":\"The dSS was not found in the database\"}";
    BOOST_CHECK_NO_THROW(resp = parse_reply(buf));
    BOOST_CHECK_EQUAL(resp.code, 2);
    BOOST_CHECK_EQUAL(resp.desc, "The dSS was not found in the database");
  }
  {
    const char *buf =  "{\"ReturnCode\":\"2\",\"ReturnMessage\":\"The dSS was not found in the database\"}";
    BOOST_CHECK_THROW(resp = parse_reply(buf), ParseError);
  }
  {
    const char *buf =  "{\"ReturnCode\":2,\"ReturnMessage\":42}";
    BOOST_CHECK_THROW(resp = parse_reply(buf), ParseError);
  }
  {
    const char *buf =  "";
    BOOST_CHECK_THROW(resp = parse_reply(buf), ParseError);
  }
  {
    const char *buf =  "{128471049asdfhla}";
    BOOST_CHECK_THROW(resp = parse_reply(buf), ParseError);
  }
  {
    const char *buf =  NULL;
    BOOST_CHECK_THROW(resp = parse_reply(buf), ParseError);
  }
}

class WebserviceFixture {
public:
  WebserviceFixture() {

    /* TODO clean this up */
    SystemInfo info;
    info.collect();
    DSS::getInstance()->publishDSID();

    /* switch from production to test webservice */
    PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();
    propSystem.getProperty(pp_websvc_url_authority)
      ->setStringValue(websvc_url_authority_test);
    propSystem.createProperty(pp_websvc_rc_osptoken)
      ->setStringValue(websvc_osp_token_test);

    // TODO: webservice connection fetched original authority, restart it
    WebserviceConnection::shutdown();
    WebserviceConnection::getInstance();

    propSystem.createProperty(pp_websvc_enabled)->setBooleanValue(true);
  }

  DSSLifeCycle m_dss_guard;
};

class NotifyDone : public WebserviceCallDone {
public:
  NotifyDone(boost::mutex &mutex,
             boost::condition_variable &completion)
    : m_mutex(mutex)
    , m_completion(completion)
  {
  }

  void done(RestTransferStatus_t status, WebserviceReply reply) {
    boost::mutex::scoped_lock lock(m_mutex);
    this->status = status;
    this->reply = reply;
    m_completion.notify_one();
  }

  WebserviceReply reply;
  RestTransferStatus_t status;
private:
  boost::mutex &m_mutex;
  boost::condition_variable &m_completion;
};

BOOST_FIXTURE_TEST_CASE(test_notifyApartmentChange, WebserviceFixture) {
  boost::mutex mutex;
  boost::condition_variable completion;

  WebserviceCallDone_t cont(new NotifyDone(mutex, completion));
  NotifyDone *notifyDone = static_cast<NotifyDone*>(cont.get());

  boost::mutex::scoped_lock lock(mutex);
  WebserviceApartment::doModelChanged(WebserviceApartment::ApartmentChange,
                                      cont);
  completion.wait(lock);

  BOOST_CHECK_EQUAL(notifyDone->status, REST_OK);
  BOOST_CHECK_EQUAL(notifyDone->reply.code, 98); /* no access rights for token */
}

/* Access Management */
BOOST_FIXTURE_TEST_CASE(test_revokeToken, WebserviceFixture) {
  boost::mutex mutex;
  boost::condition_variable completion;

  WebserviceCallDone_t cont(new NotifyDone(mutex, completion));
  NotifyDone *notifyDone = static_cast<NotifyDone*>(cont.get());

  boost::mutex::scoped_lock lock(mutex);
  WebserviceAccessManagement::doNotifyTokenDeleted(SessionTokenGenerator::generate(),
                                                   cont);
  completion.wait(lock);

  BOOST_CHECK_EQUAL(notifyDone->status, REST_OK);
  BOOST_CHECK_EQUAL(notifyDone->reply.code, 98); /* no access rights for token */
}

BOOST_FIXTURE_TEST_CASE(test_WebscvEnableDisablePlugin, WebserviceFixture) {
  PropertySystem &propSystem = DSS::getInstance()->getPropertySystem();

  // also starts event runner et. al
  m_dss_guard.initPlugins();

  // check keep alive ical is scheduled
  propSystem.createProperty(pp_websvc_enabled)->setBooleanValue(true);
  BOOST_CHECK_EQUAL(DSS::getInstance()->getEventRunner().getSize(), 1);
  propSystem.createProperty(pp_websvc_enabled)->setBooleanValue(false);
  BOOST_CHECK_EQUAL(DSS::getInstance()->getEventRunner().getSize(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
