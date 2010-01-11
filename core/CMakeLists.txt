IF(WITH_GCOV)
  ADD_DEFINITIONS(-fprofile-arcs -ftest-coverage)
ENDIF(WITH_GCOV)


file(GLOB files *.cpp)

add_library(core ${files} scripting/modeljs.cpp scripting/propertyscriptextension.cpp 
sim/dssim.cpp sim/dsidsim.cpp
sim/dsid_js.cpp sim/dsid_plugin.cpp web/webserverapi.cpp web/webrequests.cpp 
metering/metering.cpp metering/series.cpp metering/seriespersistence.cpp
metering/fake_meter.cpp vfs/vfs.cpp web/restful.cpp web/restfulapiwriter.cpp
web/webserverplugin.cpp web/webserver.cpp web/json.cpp
web/handler/debugrequesthandler.cpp web/handler/simrequesthandler.cpp
web/handler/systemrequesthandler.cpp web/handler/meteringrequesthandler.cpp
web/handler/structurerequesthandler.cpp web/handler/eventrequesthandler.cpp
web/handler/propertyrequesthandler.cpp web/handler/deviceinterfacerequesthandler.cpp
web/handler/circuitrequesthandler.cpp web/handler/zonerequesthandler.cpp
web/handler/devicerequesthandler.cpp web/handler/apartmentrequesthandler.cpp 
web/handler/setrequesthandler.cpp web/handler/jsonhelper.cpp)


add_subdirectory(ds485)
add_subdirectory(model)
