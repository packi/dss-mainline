#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <iostream>

#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

#include "foreach.h"
#include "logger.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(LoggerTest)


class LoggerThread {
  __DECL_LOG_CHANNEL__
public:
  void run();
};

__DEFINE_LOG_CHANNEL__(LoggerThread, lsInfo);

void LoggerThread::run() {
  /*
   * no BOOST_CHECK in here, it's not threadsafe
   * http://boost.2283326.n4.nabble.com/Is-boost-test-thread-safe-td3471644.html
   */
  for (int i = 0; i < 2048; i++) {
    /* we want to use a class channel */
    log("logger concurrency test", lsInfo);
  }
}

BOOST_AUTO_TEST_CASE(testConcurrentLogToChannel) {
  typedef boost::shared_ptr<boost::thread> thread_t;
  std::vector<thread_t> threads;
  std::vector<LoggerThread> obj(16);

  // http://stackoverflow.com/questions/8243743/is-there-a-null-stdostream-implementation-in-c-or-libraries
  std::cout.setstate(std::ios_base::badbit);

  // TODO add log output handler, ensure that all
  // "logger concurrency test" strings are printed correctly
  // unmangled or interleaved

  for (unsigned i = 0; i < obj.size(); i++) {
    thread_t th(new  boost::thread(boost::bind(&LoggerThread::run, &obj[i])));
    threads.push_back(th);
  }

  foreach(thread_t th, threads) {
    th->join();
  }

  std::cout.clear();
}

BOOST_AUTO_TEST_SUITE_END()
