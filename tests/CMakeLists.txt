add_library(tests basetests.cpp  datetoolstests.cpp  ds485tests.cpp
	eventtests.cpp  modeljstests.cpp  modeltests.cpp devicetests.cpp
	propertysystemtests.cpp  scriptstest.cpp  seriestests.cpp tests.cpp
	busrequesttests.cpp restfulapitests.cpp jssockettests.cpp jsds485tests.cpp)

add_executable( dsstests EXCLUDE_FROM_ALL testrunner.cpp ../namespaces.cpp
	basetests.cpp  datetoolstests.cpp  jsontests.cpp ds485tests.cpp
	eventtests.cpp  modeljstests.cpp  modeltests.cpp devicetests.cpp
	propertysystemtests.cpp  scriptstest.cpp busrequesttests.cpp jsds485tests.cpp
	seriestests.cpp jshandlertests.cpp restfulapitests.cpp jssockettests.cpp)

target_link_libraries(dsstests core ds485 unix webservices model mongoose
                               ${REQUIRED_LIBS})

add_custom_target( test
  COMMAND dsstests --report_level=detailed --log_level=all
  COMMENT "running tests")
