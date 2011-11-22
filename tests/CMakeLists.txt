set(files basetests.cpp  datetoolstests.cpp securitytests.cpp
        eventtests.cpp
        modeltests.cpp
        devicetests.cpp
        propertysystemtests.cpp  scriptstest.cpp tests.cpp
        deviceinterfacerequesthandlertest.cpp jsontests.cpp
        restfulapitests.cpp dssimtest.cpp
        devicerequesthandlertest.cpp circuitrequesthandlertest.cpp
        eventrequesthandlertest.cpp
        jsloggertests.cpp modeljstests.cpp jssockettests.cpp jshandlertests.cpp
)


add_library(tests ${files})

add_executable(dsstests EXCLUDE_FROM_ALL
              testrunner.cpp ../namespaces.cpp
              ${files})

target_link_libraries(dsstests ${DSS_LIBS} mongoose security ${REQUIRED_LIBS})

add_custom_target(test
  COMMAND dsstests --report_level=detailed --log_level=all
  COMMENT "running tests")
