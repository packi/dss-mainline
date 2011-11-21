cmake_minimum_required(VERSION 2.6)
project(DSS)

if(WIN32)
  option(WITH_TESTS "Include test-suite" OFF)
else(WIN32)
  option(WITH_TESTS "Include test-suite" ON)
endif(WIN32)
option(WITH_GCOV "Compile coverage information into the dss" OFF)
set(LIBDSM_PREFIX CACHE STRING "Location of libdsm-api")

if(NOT WITH_DATADIR)
  set(WITH_DATADIR "${CMAKE_INSTALL_PREFIX}/share/dss/data" CACHE FILEPATH "Data directory")
endif(NOT WITH_DATADIR)

if(NOT WITH_CONFIGDIR)
  set(WITH_CONFIGDIR "${WITH_DATADIR}" CACHE FILEPATH "Config directory")
endif(NOT WITH_CONFIGDIR)

if(NOT WITH_WEBROOTDIR)
  set(WITH_WEBROOTDIR "${WITH_DATADIR}/webroot" CACHE FILEPATH "Webroot directory")
endif(NOT WITH_WEBROOTDIR)

if(NOT WITH_JSLOGDIR)
  set(WITH_JSLOGDIR "${WITH_DATADIR}/logs" CACHE FILEPATH "JS Log directory")
endif(NOT WITH_JSLOGDIR)

if(NOT WITH_SAVEDPROPSDIR)
  set(WITH_SAVEDPROPSDIR "${WITH_DATADIR}/savedprops" CACHE FILEPATH "Saved properties directory")
endif(NOT WITH_SAVEDPROPSDIR)

option(WITH_HTTP "Build http protocol support into the dss" OFF)

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


add_definitions(-DXP_UNIX -DUSE_LIBXML)
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

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} /opt/local/include)
  link_directories(/opt/local/lib)
  set(CMAKE_OSX_ARCHITECTURES "x86_64")
  set(WITH_BONJOUR 1)
  set(HAVE_DNS_SD 1)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# header tests
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")
if(WIN32)
	set(REQUIRED_LIBS dl ws2_32 wsock32)
else(WIN32)
	set(REQUIRED_LIBS dl)
endif(WIN32)
check_include_files(stdint.h HAVE_STDINT)

# check for backtrace
check_function_exists(backtrace HAVE_BACKTRACE)
check_function_exists(backtrace_symbols HAVE_BACKTRACE_SYMBOLS)

include(CheckLibraryExists)
check_library_exists(rt clock_gettime "" HAVE_RT)
if(HAVE_RT)
  set(REQUIRED_LIBS ${REQUIRED_LIBS} rt)
endif(HAVE_RT)

# check for a library providing zeroconf (avahi/dnssd)
find_package(Avahi)
if(AVAHI_FOUND)
  include_directories(${AVAHI_INCLUDE_DIRS})
  set(REQUIRED_LIBS ${REQUIRED_LIBS} ${AVAHI_LIBRARIES})
  set(WITH_BONJOUR 1)
  set(HAVE_AVAHI 1)
else(AVAHI_FOUND)
  find_package(DNSSD)
  if(DNSSD_FOUND)
    include_directories(${DNSSD_INCLUDE_DIRS})
    set(REQUIRED_LIBS ${REQUIRED_LIBS} ${DNSSD_LIBRARIES})
    set(WITH_BONJOUR 1)
    set(HAVE_DNS_SD 1)
  endif(DNSSD_FOUND)
endif(AVAHI_FOUND)

# check for SpiderMonkey headers and library
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -DXP_UNIX)
find_package(SpiderMonkey REQUIRED)
include_directories(${SPIDERMONKEY_INCLUDE_DIR})
set(REQUIRED_LIBS ${REQUIRED_LIBS} ${SPIDERMONKEY_LIBRARY})
if(SPIDERMONKEY_THREADSAFE)
  add_definitions(-DJS_THREADSAFE)
else()
  MESSAGE (FATAL_ERROR "Need SpiderMonkey (libjs) built with JS_THREADSAFE")
endif()

# check for pthread
find_package(Threads REQUIRED)

include(${CMAKE_ROOT}/Modules/FindOpenSSL.cmake)
if (OPENSSL_FOUND)
  set(REQUIRED_LIBS ${REQUIRED_LIBS} ${OPENSSL_LIBRARIES})
  include_directories(${OPENSSL_INCLUDE_DIR})
else()
  MESSAGE (FATAL_ERROR "Cannot build without OpenSSL. Please install OpenSSL")
endif()

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

# sendmail
find_program(SENDMAIL sendmail)
if(SENDMAIL)
   set(HAVE_SENDMAIL 1)
endif(SENDMAIL)

# librrd
# TODO: actually check for librrd
set(REQUIRED_LIBS ${REQUIRED_LIBS} rrd)

# generate config.h header file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)
add_definitions(-DHAVE_CONFIG_H)

set(BOOST_REQUIRED_LIBS thread filesystem system program_options)

# check for Boost unit testing
if(WITH_TESTS)
  set(BOOST_REQUIRED_LIBS unit_test_framework ${BOOST_REQUIRED_LIBS})
endif(WITH_TESTS)

# check for Boost
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.35.0 COMPONENTS ${BOOST_REQUIRED_LIBS})
if(Boost_FOUND)
  set(REQUIRED_LIBS ${Boost_LIBRARIES} ${REQUIRED_LIBS})
else()
  MESSAGE (FATAL_ERROR "Cannot build without Boost.")
endif()

if(CMAKE_HAVE_PTHREAD_CREATE)
  set(REQUIRED_LIBS ${REQUIRED_LIBS} pthread)
endif(CMAKE_HAVE_PTHREAD_CREATE)

# libdsm-api
if(LIBDSM_PREFIX)
  link_directories(${LIBDSM_PREFIX}/lib)
  include_directories(${LIBDSM_PREFIX}/include)
endif()

set(REQUIRED_LIBS ${REQUIRED_LIBS} dsm-api-v2)

add_subdirectory(core)
add_subdirectory(unix)
add_subdirectory(webservices)
add_subdirectory(external)
add_subdirectory(doc)

set(DSS_LIBS core security ds485 model unix simulation scripting webservices)
add_executable(dss main.cpp namespaces.cpp)
target_link_libraries(dss ${TEST_LIB} ${DSS_LIBS} mongoose ${REQUIRED_LIBS})
add_dependencies(dss build_info.h)


#include test-directory if needed
set(TEST_LIB)
if(WITH_TESTS)
  set(TEST_LIB tests)
  add_subdirectory(tests)
  add_dependencies(dsstests build_info.h)
endif(WITH_TESTS)


## -- install targets:
install(TARGETS dss RUNTIME DESTINATION bin)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data/
  DESTINATION ${WITH_DATADIR}
  PATTERN "webroot" EXCLUDE
  PATTERN "*.xml" EXCLUDE
  PATTERN "*.js"
)


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
