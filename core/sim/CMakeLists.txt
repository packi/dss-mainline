set(files dssim.cpp dsmetersim.cpp dsidsim.cpp
          businterface/businterfaceadaptor.cpp
          dsid_js.cpp businterface/simbusinterface.cpp
          businterface/simmeteringbusinterface.cpp
          businterface/simstructurequerybusinterface.cpp
          businterface/simstructuremodifyingbusinterface.cpp
          businterface/simdevicebusinterface.cpp)

add_library(simulation ${files})
