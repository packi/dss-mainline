add_library(tests basetests.cpp  datetoolstests.cpp  ds485tests.cpp
	eventtests.cpp  modeljstests.cpp  modeltests.cpp
	propertysystemtests.cpp  scriptstest.cpp  seriestests.cpp tests.cpp busrequesttests.cpp restfulapitests.cpp)

add_executable( dsstests EXCLUDE_FROM_ALL testrunner.cpp ../namespaces.cpp
	basetests.cpp  datetoolstests.cpp  jsontests.cpp ds485tests.cpp
        eventtests.cpp  modeljstests.cpp  modeltests.cpp
        propertysystemtests.cpp  scriptstest.cpp busrequesttests.cpp
	seriestests.cpp jshandlertests.cpp restfulapitests.cpp)

target_link_libraries(dsstests ${BOOST_TEST_LIB} core ds485 unix webservices model
	pthread mongoose ${REQUIRED_LIBS})

add_custom_target( test
  COMMAND dsstests --report_level=detailed --log_level=all
  COMMENT "running tests")
