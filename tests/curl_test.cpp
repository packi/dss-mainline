#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <iostream>

#define HAVE_CURL 1
#include "src/url.h"
#include "src/propertysystem.h"
#include "src/dss.h"
#include "src/event.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(CurlTest)

BOOST_AUTO_TEST_CASE(curlTest) {

    URLResult result;
    boost::shared_ptr<URL> curl(new URL());
    std::string url = "http://www.digitalstrom.com";

    BOOST_CHECK_EQUAL(curl->request(url, GET, &result), 200);

    BOOST_CHECK_EQUAL(curl->request(url, POST, NULL), 200);

    HashMapStringString header;
    header["bar"] = "dada";
    HashMapStringString postform;
    postform["foo"] = "bogus";
    BOOST_CHECK_EQUAL(curl->request(url, POST, boost::make_shared<HashMapStringString>(header),
                                    boost::make_shared<HashMapStringString>(postform),
                                    &result), 200);
}

BOOST_AUTO_TEST_SUITE_END()
