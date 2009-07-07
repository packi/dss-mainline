file(GLOB files *.cpp)

add_library(core ${files} scripting/modeljs.cpp sim/dssim.cpp sim/dsidsim.cpp
metering/metering.cpp metering/series.cpp metering/seriespersistence.cpp
metering/fake_meter.cpp vfs/vfs.cpp web/restful.cpp web/restfulapiwriter.cpp)
