#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "tests.h"

namespace dss {

  bool Tests::run() {
    return true;
  }


  static void _init(void) __attribute__ ((constructor));
  static void _init(void) {
    // make sure timezone gets set
    tzset();

    char* tzNameCopy = strdup("GMT");
    tzname[0] = tzname[1] = tzNameCopy;
    timezone = 0;
    daylight = 0;

    setenv("TZ", "UTC", 1);
  }



}
