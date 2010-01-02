IF(WITH_GCOV)
  ADD_DEFINITIONS(-fprofile-arcs -ftest-coverage)
ENDIF(WITH_GCOV)


file(GLOB files *.cpp)

add_library(core ${files} scripting/modeljs.cpp sim/dssim.cpp sim/dsidsim.cpp
sim/dsid_js.cpp sim/dsid_plugin.cpp
metering/metering.cpp metering/series.cpp metering/seriespersistence.cpp
metering/fake_meter.cpp vfs/vfs.cpp web/restful.cpp web/restfulapiwriter.cpp
web/webserverplugin.cpp)


add_subdirectory(ds485)
add_subdirectory(model)