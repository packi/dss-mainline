IF(WITH_GCOV)
  ADD_DEFINITIONS(-fprofile-arcs -ftest-coverage)
ENDIF(WITH_GCOV)

add_definitions(-DWITH_OPENSSL)

file(GLOB files *.cpp)

add_library(webservices ${files})
target_link_libraries(webservices gsoap++)
