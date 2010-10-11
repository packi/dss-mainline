set(files basetests.cpp  datetoolstests.cpp
        eventtests.cpp
        modeljstests.cpp
        modeltests.cpp
        devicetests.cpp
        propertysystemtests.cpp  scriptstest.cpp  seriestests.cpp tests.cpp
        jsloggertests.cpp deviceinterfacerequesthandlertest.cpp jsontests.cpp
        restfulapitests.cpp jssockettests.cpp jshandlertests.cpp
        dssimtest.cpp
        devicerequesthandlertest.cpp circuitrequesthandlertest.cpp
        eventrequesthandlertest.cpp)

add_library(tests ${files})

add_executable(dsstests EXCLUDE_FROM_ALL
              testrunner.cpp ../namespaces.cpp
              ${files})

target_link_libraries(dsstests core ds485 unix webservices simulation model mongoose
                               ${REQUIRED_LIBS})

add_custom_target(test
  COMMAND dsstests --report_level=detailed --log_level=all
  COMMENT "running tests")
