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

add_definitions(-DXP_UNIX -DUSE_SIM -DUSE_LIBXML -DNO_CGI -DWITH_TESTS)

include_directories(external)
include_directories(${DSS_SOURCE_DIR})

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  include_directories(/opt/local/include)
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} /opt/local/include)
  link_directories(/opt/local/lib)
  link_libraries(boost_filesystem-mt boost_system-mt PocoFoundation PocoNet)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  link_libraries(boost_filesystem rt)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -Wno-deprecated)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

link_libraries(PocoFoundation PocoNet)

# header tests
INCLUDE(CheckIncludeFiles)

# check for SpiderMonkey API header
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -DXP_UNIX)
CHECK_INCLUDE_FILES(mozjs/jsapi.h HAVE_MOZJS_JSAPI_H)
CHECK_INCLUDE_FILES(js/jsapi.h HAVE_JS_JSAPI_H)

# check for SpiderMonkey Library
find_library(MOZJS mozjs)

# generate config.h header file
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
add_definitions(-DHAVE_CONFIG_H)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(core)
add_subdirectory(unix)
add_subdirectory(webservices)
add_subdirectory(external)
add_subdirectory(tests)

link_directories(${DSS_BINARY_DIR}/core ${DSS_BINARY_DIR}/unix ${DSS_BINARY_DIR}/tests)

add_executable(dss main.cpp namespaces.cpp)

link_libraries(gsoap++)

target_link_libraries(dss tests core unix webservices pthread shttpd ical dl PocoXML ${MOZJS} cppunit)
