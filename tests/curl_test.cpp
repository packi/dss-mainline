#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <iostream>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

#include "config.h"
#ifdef HAVE_CURL

#include "src/url.h"
#include "src/propertysystem.h"
#include "src/dss.h"
#include "src/event.h"
#include "src/foreach.h"

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

BOOST_AUTO_TEST_CASE(curlTestReuseHandle) {
    URLResult result;
    URL curl(true);
    std::string url = "http://www.digitalstrom.com";

    for (int i = 0; i < 20; i++) {
      BOOST_CHECK_EQUAL(curl.request(url, GET, &result), 200);
      BOOST_CHECK_EQUAL(curl.request(url, POST, &result), 200);
    }
}

void fetcher_do() {
  URL curl(false);
  std::string url = "http://www.google.com";

  for (int i = 0; i < 10; i++) {
    BOOST_CHECK_EQUAL(curl.request(url, GET, NULL), 200);
  }
}

BOOST_AUTO_TEST_CASE(test_rentrantCalls) {
  typedef boost::shared_ptr<boost::thread> thread_t;
  std::vector<thread_t> threads;

  for (int i = 0; i < 20; i++) {
    thread_t th(new  boost::thread(fetcher_do));
    threads.push_back(th);
  }

  foreach(thread_t th, threads) {
    th->join();
  }
}

BOOST_AUTO_TEST_SUITE_END()

#endif
