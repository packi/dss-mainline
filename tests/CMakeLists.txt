file(GLOB files *.cpp)

add_library(tests ${files})

add_executable( dsstests EXCLUDE_FROM_ALL ${files} ../namespaces.cpp)
target_link_libraries( dsstests -lboost_unit_test_framework-mt core unix webservices pthread shttpd cppunit ical PocoXML ${MOZJS} ${DNS_SD_LIB})

add_custom_target( test
  COMMAND dsstests --report_level=detailed --log_level=all
  COMMENT "running tests")
