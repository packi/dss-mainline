IF(WITH_GCOV)
  ADD_DEFINITIONS(-fprofile-arcs -ftest-coverage)
ENDIF(WITH_GCOV)

add_definitions(-DWITH_OPENSSL)

set(gsoap_generated ${CMAKE_CURRENT_BINARY_DIR}/soapC.cpp ${CMAKE_CURRENT_BINARY_DIR}/soapServer.cpp)
set(gsoap_files gs-locks.cpp model_soap.cpp soapdssSSLObject.cpp webservices.cpp model_soap.h)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/soapServer.cpp
  COMMAND ${GSOAP_SOAPCPP2} -I${GSOAP_IMPORT_DIR} -I${CMAKE_CURRENT_SOURCE_DIR} -SLx ${CMAKE_CURRENT_SOURCE_DIR}/model_soap.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/model_soap.h
)
add_custom_target(soapgen ALL
  COMMAND echo "Generated soap bindings in ${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/soapServer.cpp
)
set_source_files_properties(${gsoap_generated} PROPERTIES GENERATED 1)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(webservices ${gsoap_files} ${gsoap_generated})
target_link_libraries(webservices ${GSOAP_LIBRARIES})
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dss.wsdl
  ${CMAKE_CURRENT_BINARY_DIR}/dss.xsd
  ${CMAKE_CURRENT_BINARY_DIR}/dss.nsmap
  DESTINATION ${WITH_WEBROOTDIR}
)
