#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <iostream>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

#include "http_client.h"
#include "src/propertysystem.h"
#include "src/dss.h"
#include "src/event.h"
#include "src/foreach.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(http_client)

BOOST_AUTO_TEST_CASE(testBase) {
  boost::shared_ptr<HttpClient> httpClient = boost::make_shared<HttpClient>();
  std::string url = "http://www.digitalstrom.com";
  std::string result;

  BOOST_CHECK_EQUAL(httpClient->request(url, GET, &result), 200);
  BOOST_CHECK_EQUAL(httpClient->request(url, POST, NULL), 200);

  HashMapStringString header;
  header["bar"] = "dada";
  BOOST_CHECK_EQUAL(httpClient->request(url, POST,
                                        boost::make_shared<HashMapStringString>(header),
                                        &result), 200);
}

BOOST_AUTO_TEST_CASE(testReuseHandle) {
  HttpClient httpClient(true);
  std::string url = "http://www.digitalstrom.com";
  std::string result;

  for (int i = 0; i < 20; i++) {
    BOOST_CHECK_EQUAL(httpClient.request(url, GET, &result), 200);
    BOOST_CHECK_EQUAL(httpClient.request(url, POST, &result), 200);
  }
}

BOOST_AUTO_TEST_CASE(testEmptyHeaderMaps) {
  std::string url = "http://www.digitalstrom.com";
  boost::shared_ptr<HashMapStringString> headers;
  HttpClient httpClient(true);

  /* does it crash, is the test */
  BOOST_CHECK_EQUAL(httpClient.request(url, headers, std::string(), NULL), 200);
}

BOOST_AUTO_TEST_CASE(testEmptyHeaderAndFormPostMaps) {
  std::string url = "http://www.digitalstrom.com";
  boost::shared_ptr<HashMapStringString> headers;
  HttpClient httpClient(true);

  /* does it crash, is the test */
  BOOST_CHECK_EQUAL(httpClient.request(url, POST, headers, NULL), 200);
}

BOOST_AUTO_TEST_CASE(testStructHttpRequest) {
  HttpRequest req;
  req.url = "http://www.digitalstrom.com";
  req.type = POST;
  HttpClient httpClient(true);
  std::string result;

  /* does it crash, is the test */
  BOOST_CHECK_EQUAL(httpClient.request(req, &result), 200);
}

void fetcher_do() {
  HttpClient httpClient(false);
  std::string url = "http://www.google.com";

  /*
   * no BOOST_CHECK in here, it's not threadsafe
   * http://boost.2283326.n4.nabble.com/Is-boost-test-thread-safe-td3471644.html
   */

  for (int i = 0; i < 10; i++) {
    int ret = httpClient.request(url, GET, NULL);
    if (ret != 200) {
      Logger::getInstance()->log("test_rentrantCalls 200 != "  +
                                 intToString(ret), lsInfo);
    }
  }
}

BOOST_AUTO_TEST_CASE(testReentrantCalls) {
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
