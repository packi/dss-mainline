IF(WITH_GCOV)
  ADD_DEFINITIONS(-fprofile-arcs -ftest-coverage)
ENDIF(WITH_GCOV)

file(GLOB files *.cpp)

add_library(webservices ${files})
target_link_libraries(webservices gsoap++)
