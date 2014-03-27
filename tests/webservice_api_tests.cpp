#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>

#define HAVE_CURL 1
#include "src/url.h"
#include "src/propertysystem.h"
#include "src/dss.h"
#include "src/event.h"

#include "webservice_api.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(CloudTest)

BOOST_AUTO_TEST_CASE(parseModelChangeTest) {

  /* test sample response */
  ModelChangeResponse resp;
  {
    const char *buf =  "{\"ReturnCode\":2,\"ReturnMessage\":\"The dSS was not found in the database\"}";
    BOOST_CHECK_NO_THROW(resp = parseModelChange(buf));
  }
  {
    const char *buf =  "{\"ReturnCode\":\"2\",\"ReturnMessage\":\"The dSS was not found in the database\"}";
    BOOST_CHECK_THROW(resp = parseModelChange(buf), ParseError);
  }
  {
    const char *buf =  "{\"ReturnCode\":2,\"ReturnMessage\":42}";
    BOOST_CHECK_THROW(resp = parseModelChange(buf), ParseError);
  }
  {
    const char *buf =  "";
    BOOST_CHECK_THROW(resp = parseModelChange(buf), ParseError);
  }
  {
    const char *buf =  "{128471049asdfhla}";
    BOOST_CHECK_THROW(resp = parseModelChange(buf), ParseError);
  }
  {
    const char *buf =  NULL;
    BOOST_CHECK_THROW(resp = parseModelChange(buf), ParseError);
  }
}

BOOST_AUTO_TEST_CASE(apartmentChangeTest) {
  URLResult result;

  boost::shared_ptr<URL> curl(new URL());
  PropertySystem propSystem;
  std::string aprtmntChange("https://testdsservices.aizo.com/internal/dss/v1_0/DSSApartment/ApartmentHasChanged");
  aprtmntChange += "?apartmentChangeType=Apartment";
  aprtmntChange += "&dssid=3504175feff28d2044084179";

  BOOST_CHECK_EQUAL(curl->request(aprtmntChange, POST, &result), 200);

  BOOST_CHECK_NO_THROW(ModelChangeResponse resp = parseModelChange(result.content()));
}

BOOST_AUTO_TEST_SUITE_END()
