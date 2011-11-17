# Find the SpiderMonkey libjs header files and libraries
# Once done this will define
#
#  SPIDERMONKEY_INCLUDE_DIR    - where to find jsapi.h, etc.
#  SPIDERMONKEY_LIBRARY        - List of libraries when using libjs.
#  SPIDERMONKEY_FOUND          - True if SpiderMonkey found.
#  SPIDERMONKEY_THREADSAFE     - True if SpiderMonkey is compiled with multi threading support.
#
#  SPIDERMONKEY_VERSION_STRING - The version of libjs found (x.y.z)
#  SPIDERMONKEY_VERSION_MAJOR  - The major version of libjs
#  SPIDERMONKEY_VERSION_MINOR  - The minor version of libjs
#  SPIDERMONKEY_VERSION_PATCH  - The patch version of libjs

include(CheckIncludeFileCXX)
include(CheckCXXSourceCompiles)

if(SPIDERMONKEY_INCLUDE_DIR AND SPIDERMONKEY_LIBRARIES)
  set(SPIDERMONKEY_FIND_QUIETLY TRUE)
endif()

if(SPIDERMONKEY_ROOT)
    find_path(
        SPIDERMONKEY_INCLUDE_DIR
        js/jsapi.h
        PATHS "${SPIDERMONKEY_ROOT}/include"
        NO_DEFAULT_PATH)
else()
    find_path(
        SPIDERMONKEY_INCLUDE_DIR
        js/jsapi.h)
endif()

if(SPIDERMONKEY_ROOT)
    find_library(
        SPIDERMONKEY_LIBRARY
        NAMES mozjs185 mozjs-1.9.2 mozjs js185 js js32 js3250
        PATHS "${SPIDERMONKEY_ROOT}/lib"
        NO_DEFAULT_PATH)
else()
    find_library(
        SPIDERMONKEY_LIBRARY
        NAMES mozjs185 mozjs-1.9.2 mozjs js185 js js32 js3250)
endif()

set(SPIDERMONKEY_LIBRARIES ${SPIDERMONKEY_LIBRARY})
set(CMAKE_REQUIRED_INCLUDES ${SPIDERMONKEY_INCLUDE_DIR})
set(CMAKE_REQUIRED_DEFINITIONS ${SPIDERMONKEY_DEFINITIONS})
list(APPEND CMAKE_REQUIRED_LIBRARIES ${SPIDERMONKEY_LIBRARY})

check_cxx_source_compiles(
    "#include <js/jsapi.h>
     int main() {
        JSRuntime *rt = JS_NewRuntime(8L * 1024L * 1024L);
        if (!rt)
            return 1;
        return 0;
     }"
    SPIDERMONKEY_BUILDS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Spidermonkey
    DEFAULT_MSG
    SPIDERMONKEY_LIBRARIES
    SPIDERMONKEY_INCLUDE_DIR
    SPIDERMONKEY_BUILDS)

if(SPIDERMONKEY_FOUND)
    set(CMAKE_REQUIRED_INCLUDES ${SPIDERMONKEY_INCLUDE_DIR})
    set(CMAKE_REQUIRED_DEFINITIONS ${SPIDERMONKEY_DEFINITIONS})
    set(CMAKE_REQUIRED_LIBRARIES ${SPIDERMONKEY_LIBRARY})

    if(SPIDERMONKEY_ROOT)
        find_path(SPIDERMONKEY_JS_CONFIG_HEADER_PATH "js/js-config.h"
            PATHS "${SPIDERMONKEY_ROOT}/include"
            NO_DEFAULT_PATH)
        find_path(SPIDERMONKEY_JS_VERSION_HEADER_PATH "js/jsversion.h"
            PATHS "${SPIDERMONKEY_ROOT}/include"
            NO_DEFAULT_PATH)
    else()
        find_path(SPIDERMONKEY_JS_CONFIG_HEADER_PATH "js/js-config.h")
        find_path(SPIDERMONKEY_JS_VERSION_HEADER_PATH "js/jsversion.h")
    endif()

    check_include_file_cxx("${SPIDERMONKEY_JS_CONFIG_HEADER_PATH}/js/js-config.h" SPIDERMONKEY_JS_CONFIG_HEADER)
    check_include_file_cxx("${SPIDERMONKEY_JS_CONFIG_HEADER_PATH}/js/jsversion.h" SPIDERMONKEY_JS_VERSION_HEADER)

    if(SPIDERMONKEY_JS_VERSION_HEADER)
        FILE(STRINGS "${SPIDERMONKEY_JS_VERSION_HEADER_PATH}/js/jsversion.h" JSVERSION REGEX "^#define JS_VERSION [^\"]*$")
        STRING(REGEX REPLACE "^.*JS_VERSION ([0-9]+)*$" "\\1" SPIDERMONKEY_VERSION_STRING "${JSVERSION}")
        STRING(REGEX REPLACE "^([0-9]).." "\\1" SPIDERMONKEY_VERSION_MAJOR  "${SPIDERMONKEY_VERSION_STRING}")
        STRING(REGEX REPLACE "^.([0-9])." "\\1" SPIDERMONKEY_VERSION_MINOR  "${SPIDERMONKEY_VERSION_STRING}")
        STRING(REGEX REPLACE "^..([0-9])" "\\1" SPIDERMONKEY_VERSION_PATCH  "${SPIDERMONKEY_VERSION_STRING}")
    endif()

    if(NOT SPIDERMONKEY_JS_CONFIG_HEADER)
        check_cxx_source_runs(
            "#include <stdio.h>
             extern \"C\" void js_GetCurrentThread();
             int main() {
                 printf(\"%p\",(void*)js_GetCurrentThread);
                 return(void*)js_GetCurrentThread ? 0 : 1;
             }"
            SPIDERMONKEY_THREADSAFE)

        if(SPIDERMONKEY_THREADSAFE)
            set(
                SPIDERMONKEY_DEFINITIONS
                ${SPIDERMONKEY_DEFINITIONS} -DJS_THREADSAFE)
        endif()
    else()
        FILE(STRINGS "${SPIDERMONKEY_JS_CONFIG_HEADER_PATH}/js/js-config.h" JS_THREADSAFE REGEX "^#define JS_THREADSAFE [^\"]*$")
        IF(${JS_THREADSAFE} MATCHES "1")
            SET(SPIDERMONKEY_THREADSAFE "TRUE")
        ENDIF(${JS_THREADSAFE} MATCHES "1")
    endif()

    if(NOT SPIDERMONKEY_JS_VERSION_HEADER)
        message (FATAL_ERROR "SpiderMonkey (libjs) include file jsversion.h not found, probably unsupported libjs release.")
    endif()

    if(NOT SPIDERMONKEY_THREADSAFE)
        message (FATAL_ERROR "Need SpiderMonkey (libjs) built with JS_THREADSAFE")
    endif()

    get_filename_component(
      SPIDERMONKEY_LIBDIR
      ${SPIDERMONKEY_LIBRARY}
      PATH)
    link_directories(${SPIDERMONKEY_LIBDIR})

endif()

list(REMOVE_ITEM CMAKE_REQUIRED_LIBRARIES ${SPIDERMONKEY_LIBRARY})
list(REMOVE_DUPLICATES CMAKE_REQUIRED_LIBRARIES)
mark_as_advanced(SPIDERMONKEY_INCLUDE_DIR SPIDERMONKEY_LIBRARY)
mark_as_advanced(SPIDERMONKEY_JS_CONFIG_HEADER_PATH)
