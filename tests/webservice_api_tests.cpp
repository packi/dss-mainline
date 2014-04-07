#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include "config.h"
#ifdef HAVE_CURL // without CURL no HTTP client, no WebService

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>
#include <curl/curl.h>
#include <iostream>

#include "src/url.h"
#include "src/propertysystem.h"
#include "src/dss.h"
#include "src/event.h"
#include "unix/systeminfo.h"
#include "webservice_api.h"

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
  boost::shared_ptr<URL> curl(new URL());
  URLResult result;
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
  BOOST_CHECK_NO_THROW(resp = parse_reply(result.content()));
  BOOST_CHECK_EQUAL(resp.code, 9); /* unknown dsid */
}

BOOST_AUTO_TEST_SUITE_END()

#endif // HAVE_CURL
