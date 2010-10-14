set(files dssim.cpp dsmetersim.cpp dsidsim.cpp
          dsid_js.cpp businterface/simbusinterface.cpp
          businterface/simdevicebusinterface.cpp)

add_library(simulation ${files})
