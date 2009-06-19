add_library(tests basetests.cpp  datetoolstests.cpp  ds485tests.cpp
	eventtests.cpp  modeljstests.cpp  modeltests.cpp
	propertysystemtests.cpp  scriptstest.cpp  seriestests.cpp)

add_executable( dsstests EXCLUDE_FROM_ALL tests.cpp ../namespaces.cpp
	basetests.cpp  datetoolstests.cpp  ds485tests.cpp
	        eventtests.cpp  modeljstests.cpp  modeltests.cpp
		        propertysystemtests.cpp  scriptstest.cpp
			seriestests.cpp)
target_link_libraries( dsstests -lboost_unit_test_framework-mt core unix webservices pthread shttpd ical PocoXML ${MOZJS} ${DNS_SD_LIB})

add_custom_target( test
  COMMAND dsstests --report_level=detailed --log_level=all
  COMMENT "running tests")
