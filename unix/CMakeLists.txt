IF(WITH_GCOV)
  ADD_DEFINITIONS(-fprofile-arcs -ftest-coverage)
ENDIF(WITH_GCOV)


set(files thread.cpp systeminfo.cpp)

add_library(unix ${files})
