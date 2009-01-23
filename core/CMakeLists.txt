file(GLOB files *.cpp)


add_library(core ${files} scripting/modeljs.cpp sim/dssim.cpp)
