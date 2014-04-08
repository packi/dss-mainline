#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <iostream>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

#include "src/url.h"
#include "src/propertysystem.h"
#include "src/dss.h"
#include "src/event.h"
#include "src/foreach.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(CurlTest)

BOOST_AUTO_TEST_CASE(curlTest) {
    boost::shared_ptr<URL> curl(new URL());
    std::string url = "http://www.digitalstrom.com";
    std::string result;

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
    URL curl(true);
    std::string url = "http://www.digitalstrom.com";
    std::string result;

    for (int i = 0; i < 20; i++) {
      BOOST_CHECK_EQUAL(curl.request(url, GET, &result), 200);
      BOOST_CHECK_EQUAL(curl.request(url, POST, &result), 200);
    }
}

BOOST_AUTO_TEST_CASE(test_emptyHeaderMaps) {
  std::string url = "http://www.digitalstrom.com";
  boost::shared_ptr<HashMapStringString> headers;
  URL curl(true);

  /* does it crash, is the test */
  BOOST_CHECK_EQUAL(curl.request(url, headers, std::string(), NULL), 200);
}

BOOST_AUTO_TEST_CASE(test_emptyHeaderAndFormPostMaps) {
  std::string url = "http://www.digitalstrom.com";
  boost::shared_ptr<HashMapStringString> headers;
  boost::shared_ptr<HashMapStringString> formpost;
  URL curl(true);

  /* does it crash, is the test */
  BOOST_CHECK_EQUAL(curl.request(url, POST, headers, formpost, NULL), 200);
}

BOOST_AUTO_TEST_CASE(test_URLRequestStruct) {
  HttpRequest req;
  req.url = "http://www.digitalstrom.com";
  req.type = POST;
  URL curl(true);
  std::string result;

  /* does it crash, is the test */
  BOOST_CHECK_EQUAL(curl.request(req, &result), 200);
}

void fetcher_do() {
  URL curl(false);
  std::string url = "http://www.google.com";

  /*
   * no BOOST_CHECK in here, it's not threadsafe
   * http://boost.2283326.n4.nabble.com/Is-boost-test-thread-safe-td3471644.html
   */

  for (int i = 0; i < 10; i++) {
    int ret = curl.request(url, GET, NULL);
    if (ret != 200) {
      Logger::getInstance()->log("test_rentrantCalls 200 != "  +
                                 intToString(ret), lsInfo);
    }
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
