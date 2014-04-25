#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>
#include <curl/curl.h>
#include <iostream>

#include "src/http_client.h"
#include "src/propertysystem.h"
#include "src/dss.h"
#include "src/event.h"
#include "unix/systeminfo.h"
#include "webservice_api.h"
#include "tests/dss_life_cycle.h"

using namespace dss;

static const char *websvc_url_authority_test = "https://testdsservices.aizo.com/";

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

BOOST_AUTO_TEST_CASE(apartmentChangeTest) {
  boost::shared_ptr<HttpClient> curl(new HttpClient());
  std::string result;
  std::string url;

  PropertySystem propSystem;
  setupCommonProperties(propSystem);

  //std::string url("https://testdsservices.aizo.com/internal/dss/v1_0/DSSApartment/ApartmentHasChanged");
  url = websvc_url_authority_test;
  url += propSystem.getStringValue(pp_websvc_apartment_changed_url_path);
  url += "?apartmentChangeType=Apartment";
  url += "&dssid=3504175feff28d2044084179";

  BOOST_CHECK_EQUAL(curl->request(url, POST, &result), 200);
  WebserviceReply resp;
  BOOST_CHECK_NO_THROW(resp = parse_reply(result.c_str()));
  BOOST_CHECK_EQUAL(resp.code, 9); /* unknown dsid */
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
  BOOST_CHECK_EQUAL(notifyDone->reply.code, 9); /* unknown dsid */
}

BOOST_AUTO_TEST_SUITE_END()
