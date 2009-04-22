cmake_minimum_required (VERSION 2.6)
project(DSS)

INCLUDE (${CMAKE_ROOT}/Modules/FindLibXml2.cmake)
IF (LIBXML2_FOUND)
	INCLUDE_DIRECTORIES (${LIBXML2_INCLUDE_DIR})
	LINK_LIBRARIES (${LIBXML2_LIBRARIES})
ELSE (LIBXML2_FOUND)
	MESSAGE (FATAL_ERROR "Cannot build without LibXML2. Please install
	LibXML2.")
ENDIF (LIBXML2_FOUND)

add_definitions(-DXP_UNIX -DUSE_SIM -DUSE_LIBXML -DNO_CGI)

include_directories(external)
include_directories(${DSS_SOURCE_DIR})

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  include_directories(/opt/local/include)
  link_directories(/opt/local/lib)
  link_libraries(boost_filesystem-mt boost_system-mt PocoFoundation PocoNet)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  link_libraries(boost_filesystem rt)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

add_subdirectory(core)
add_subdirectory(unix)
add_subdirectory(webservices)
add_subdirectory(external)

link_directories(${DSS_BINARY_DIR}/core ${DSS_BINARY_DIR}/unix)

add_executable(dss main.cpp namespaces.cpp)

link_libraries(gsoap++)

target_link_libraries(dss core unix webservices pthread shttpd
	 ical dl js PocoXML)
