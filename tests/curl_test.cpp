#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>

#define HAVE_CURL 1
#include "src/url.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(CurlTest)

BOOST_AUTO_TEST_CASE(curlTest) {

    URLResult result;
    boost::shared_ptr<URL> curl(new URL());
    //std::string url = "www.aizo.com";
    //std::string url = "http://www.aizo.com";
    std::string url = "http://testdsservices.aizo.com";

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

    /* cloud api */
    std::cout << "HTTP POST cloud aptm change\n";
    std::string aprtmntChange(url);
    aprtmntChange += "/dss/DSSApartment/ApartmentHasChanged";
    aprtmntChange += "?apartmentChangeType=Apartment";
    aprtmntChange += "&dssid=3504175feff28d2044084179";
    curl->request(aprtmntChange, POST, HashMapStringString(), HashMapStringString(), &result);
    std::cout << result.memory << std::endl;

    std::cout << "CURL test done\n";
}

BOOST_AUTO_TEST_SUITE_END()
