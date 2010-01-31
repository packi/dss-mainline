cmake_minimum_required(VERSION 2.8)
project(DSS)

option(WITH_TESTS "Include test-suite" ON)
option(WITH_SIM "Compile and start the simulation" ON)
option(WITH_GCOV "Compile coverage information into the dss" OFF)

if(NOT WITH_DATADIR)
  set(WITH_DATADIR "${CMAKE_INSTALL_PREFIX}/share/dss/data" CACHE FILEPATH "Data directory")
endif(NOT WITH_DATADIR)

if(NOT WITH_CONFIGDIR)
  set(WITH_CONFIGDIR "${WITH_DATADIR}" CACHE FILEPATH "Config directory")
endif(NOT WITH_CONFIGDIR)

if(NOT WITH_WEBROOTDIR)
  set(WITH_WEBROOTDIR "${WITH_DATADIR}/webroot" CACHE FILEPATH "Webroot directory")
endif(NOT WITH_WEBROOTDIR)

set(DSS_GEN_VERSION_CMD "")
if(WIN32)
  set(DSS_GEN_VERSION_CMD "tools/dss_gen_version_win32.bat")
endif(WIN32)
if(UNIX)
  set(DSS_GEN_VERSION_CMD "tools/dss_gen_version_unix.sh")
endif(UNIX)

add_custom_target(build_info.h ALL
	COMMAND ${DSS_GEN_VERSION_CMD} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_definitions(-DHAVE_BUILD_INFO_H)

include_directories(${CMAKE_CURRENT_BINARY_DIR})


add_definitions(-DXP_UNIX -DUSE_LIBXML -DNO_CGI)
add_definitions(-Wall)
include_directories(external)
include_directories(${DSS_SOURCE_DIR})

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-Wno-deprecated" HAVE_WNO_DEPRECATED)
  if(HAVE_WNO_DEPRECATED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
  endif(HAVE_WNO_DEPRECATED)
 
  #check_cxx_compiler_flag("-Wold-style-cast" HAVE_WOLD_STYLE_CAST)
  #if(HAVE_WOLD_STYLE_CAST)
  #  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast")
  #endif(HAVE_WOLD_STYLE_CAST)
  if(WITH_GCOV)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} -fprofile-arcs -ftest-coverage")
  endif(WITH_GCOV)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")


# header tests
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
set(REQUIRED_LIBS)

check_include_files(stdint.h HAVE_STDINT)

# check for optional avahi
find_package(Avahi)
include_directories(${AVAHI_INCLUDE_DIRS})
set(REQUIRED_LIBS ${REQUIRED_LIBS} ${AVAHI_LIBRARIES} dl)

check_include_files(avahi-client/publish.h HAVE_AVAHI)
if(${HAVE_AVAHI} MATCHES 1) 
  set(WITH_BONJOUR 1)
endif(${HAVE_AVAHI} MATCHES 1)

check_include_files(dns_sd.h HAVE_DNS_SD)
if(${HAVE_DNS_SD} MATCHES 1)
  set(WITH_BONJOUR 1)
endif(${HAVE_DNS_SD} MATCHES 1)

# check for SpiderMonkey headers and library
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -DXP_UNIX)
find_package(SpiderMonkey REQUIRED)
include_directories(${SPIDERMONKEY_INCLUDE_DIRS})
check_include_files(mozjs/jsapi.h HAVE_MOZJS_JSAPI_H)
check_include_files(js/jsapi.h HAVE_JS_JSAPI_H)
check_include_files(jsapi.h HAVE_JSAPI_H)
set(REQUIRED_LIBS ${REQUIRED_LIBS} ${SPIDERMONKEY_LIBRARIES})

# check for gsoap headers and library
find_package(GSOAP REQUIRED)
include_directories(${GSOAP_INCLUDE_DIRS})
check_include_files(stdsoap2.h HAVE_STDSOAP2_H)
set(REQUIRED_LIBS ${REQUIRED_LIBS} ${GSOAP_LIBRARIES})

# check for libical headers and library
find_package(Libical REQUIRED)
include_directories(${LIBICAL_INCLUDE_DIRS})
check_include_files(libical/ical.h HAVE_LIBICAL_ICAL_H)
set(REQUIRED_LIBS ${REQUIRED_LIBS} ${LIBICAL_LIBRARIES})

# check for libpoco headers and library
find_package(Poco REQUIRED)
include_directories(${POCO_INCLUDE_DIRS})
set(REQUIRED_LIBS ${REQUIRED_LIBS} ${POCO_LIBRARIES} PocoFoundation PocoNet)

# check for libxml
find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})
set(REQUIRED_LIBS ${REQUIRED_LIBS} ${LIBXML2_LIBRARIES})

# check for pthread
find_package(Threads REQUIRED)

# generate config.h header file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
add_definitions(-DHAVE_CONFIG_H)

# check for Boost
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.36.0 COMPONENTS filesystem system program_options)
set(REQUIRED_LIBS ${REQUIRED_LIBS} ${Boost_LIBRARIES})

# check for Boost unit testing
set(TEST_LIB)
if(WITH_TESTS) 
  find_package(Boost 1.36.0 COMPONENTS unit_test_framework)
  set(REQUIRED_LIBS ${REQUIRED_LIBS} ${Boost_LIBRARIES})
  set(TEST_LIB tests)
  add_subdirectory(tests)
  add_dependencies(dsstests build_info.h)
endif(WITH_TESTS)


add_subdirectory(core)
add_subdirectory(unix)
add_subdirectory(webservices)
add_subdirectory(external)
add_subdirectory(doc)


add_executable(dss main.cpp namespaces.cpp)
target_link_libraries(dss ${TEST_LIB} core ds485 model unix webservices mongoose
                          ${Boost_LIBRARIES} ${LIBXML2_LIBRARIES} ${LIBICAL_LIBRARIES}
                          ${GSOAP_LIBRARIES} ${SPIDERMONKEY_LIBRARIES}
                          ${POCO_LIBRARY} PocoFoundation PocoNet
                          ${AVAHI_LIBRARIES} dl)
add_dependencies(dss build_info.h)

## -- install targets:
install(TARGETS dss RUNTIME DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
		  DESTINATION ${WITH_DATADIR}
		  PATTERN "webroot" EXCLUDE
		  PATTERN "*.xml" EXCLUDE
		  PATTERN "*.js"
		  )


install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/webroot/ DESTINATION ${WITH_WEBROOTDIR})
install(FILES
          ${CMAKE_CURRENT_SOURCE_DIR}/data/config.xml
          ${CMAKE_CURRENT_SOURCE_DIR}/data/subscriptions.xml
          ${CMAKE_CURRENT_SOURCE_DIR}/data/sim.xml
	DESTINATION ${WITH_CONFIGDIR}
       )


add_custom_target(debian
	COMMAND dpkg-buildpackage -rfakeroot
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
