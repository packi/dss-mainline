file(GLOB files *.cpp)

add_library(webservices ${files})
target_link_libraries(webservices gsoap++)
