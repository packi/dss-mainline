IF(WITH_GCOV)
  ADD_DEFINITIONS(-fprofile-arcs -ftest-coverage)
ENDIF(WITH_GCOV)
IF(WITH_HTTP)
  ADD_DEFINITIONS(-DWITH_HTTP)
ENDIF(WITH_HTTP)

file(GLOB files *.cpp)

add_library(core ${files}
metering/metering.cpp
web/restful.cpp web/restfulapiwriter.cpp
web/webserverapi.cpp web/webrequests.cpp
web/webserver.cpp web/json.cpp
web/handler/simrequesthandler.cpp
web/handler/systemrequesthandler.cpp web/handler/meteringrequesthandler.cpp
web/handler/structurerequesthandler.cpp web/handler/eventrequesthandler.cpp
web/handler/propertyrequesthandler.cpp web/handler/deviceinterfacerequesthandler.cpp
web/handler/circuitrequesthandler.cpp web/handler/zonerequesthandler.cpp
web/handler/devicerequesthandler.cpp web/handler/apartmentrequesthandler.cpp
web/handler/setrequesthandler.cpp web/handler/jsonhelper.cpp
web/handler/subscriptionrequesthandler.cpp)

add_subdirectory(sim)
add_subdirectory(ds485)
add_subdirectory(model)
add_subdirectory(security)
add_subdirectory(scripting)
