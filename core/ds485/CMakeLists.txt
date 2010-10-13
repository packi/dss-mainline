set(files dsbusinterface.cpp
          dsactionrequest.cpp dsdevicebusinterface.cpp
          dsmeteringbusinterface.cpp dsstructurequerybusinterface.cpp)

add_library(ds485 ${files})
