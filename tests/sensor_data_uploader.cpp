#define BOOST_TEST_NO_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>

#include "sensor_data_uploader.h"

using namespace dss;

BOOST_AUTO_TEST_SUITE(SensorDataUploaderTest)

BOOST_AUTO_TEST_CASE(test_upload_chunk_after_chunk) {
  std::cout << "hello world\n";
}

BOOST_AUTO_TEST_SUITE_END()
