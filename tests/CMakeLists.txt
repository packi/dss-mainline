add_library(tests basetests.cpp  datetoolstests.cpp  ds485tests.cpp
	eventtests.cpp  modeljstests.cpp  modeltests.cpp
	propertysystemtests.cpp  scriptstest.cpp  seriestests.cpp tests.cpp)

add_executable( dsstests EXCLUDE_FROM_ALL testrunner.cpp ../namespaces.cpp
	basetests.cpp  datetoolstests.cpp  ds485tests.cpp
	        eventtests.cpp  modeljstests.cpp  modeltests.cpp
		        propertysystemtests.cpp  scriptstest.cpp
			seriestests.cpp)

target_link_libraries(dsstests ${BOOST_TEST_LIB} core unix webservices 
	pthread mongoose ${REQUIRED_LIBS})

add_custom_target( test
  COMMAND dsstests --report_level=detailed --log_level=all
  COMMENT "running tests")
