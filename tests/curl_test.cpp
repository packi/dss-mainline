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

using namespace dss;

BOOST_AUTO_TEST_SUITE(CurlTest)

BOOST_AUTO_TEST_CASE(curlTest) {

    URLResult result;
    boost::shared_ptr<URL> curl(new URL());
    std::string url = "http://www.aizo.com";

    std::cout << "HTTP GET\n";
    curl->request(url, GET, &result);

    std::cout << "HTTP POST without postform\n";
    curl->request(url, POST, HashMapStringString(), HashMapStringString(),
                  NULL);

    std::cout << "HTTP POST with postform\n";
    HashMapStringString header;
    header["bar"] = "dada";
    HashMapStringString postform;
    postform["foo"] = "bogus";
    curl->request(url, POST, header, postform, &result);

    std::cout << "CURL test done\n";
}

BOOST_AUTO_TEST_SUITE_END()
