# Source: http://code.google.com/p/google-gadgets-for-linux/source/browse/trunk/cmake/GGLCheckFunctionExists.cmake
# Copied from CheckFunctionExists.cmake to support customized link directories.
#
# - Check if the function exists.
# GGL_CHECK_FUNCTION_EXISTS(FUNCTION VARIABLE)
# - macro which checks if the function exists
#  FUNCTION - the name of the function
#  VARIABLE - variable to store the result
#
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_FLAGS = string of compile command line flags
#  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#  CMAKE_REQUIRED_INCLUDES = list of include directories
#  CMAKE_REQUIRED_LIBRARIES = list of libraries to link
#  CMAKE_REQUIRED_LINK_DIR = list of link directories

MACRO(GGL_CHECK_FUNCTION_EXISTS FUNCTION VARIABLE)
  IF("${VARIABLE}" MATCHES "^${VARIABLE}$")
    SET(MACRO_CHECK_FUNCTION_DEFINITIONS 
      "-DCHECK_FUNCTION_EXISTS=${FUNCTION} ${CMAKE_REQUIRED_FLAGS}")
    MESSAGE(STATUS "Looking for ${FUNCTION}")
    IF(CMAKE_REQUIRED_LIBRARIES)
      SET(CHECK_FUNCTION_EXISTS_ADD_LIBRARIES
        "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    ELSE(CMAKE_REQUIRED_LIBRARIES)
      SET(CHECK_FUNCTION_EXISTS_ADD_LIBRARIES)
    ENDIF(CMAKE_REQUIRED_LIBRARIES)
    IF(CMAKE_REQUIRED_INCLUDES)
      SET(CHECK_FUNCTION_EXISTS_ADD_INCLUDES
        "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_REQUIRED_INCLUDES}")
    ELSE(CMAKE_REQUIRED_INCLUDES)
      SET(CHECK_FUNCTION_EXISTS_ADD_INCLUDES)
    ENDIF(CMAKE_REQUIRED_INCLUDES)
    IF(CMAKE_REQUIRED_LINK_DIR)
      SET(CHECK_FUNCTION_EXISTS_LINK_DIRECTORIES
        "-DLINK_DIRECTORIES:STRING=${CMAKE_REQUIRED_LINK_DIR}")
    ELSE(CMAKE_REQUIRED_LINK_DIR)
      SET(CHECK_FUNCTION_EXISTS_LINK_DIRECTORIES)
    ENDIF(CMAKE_REQUIRED_LINK_DIR)
    TRY_COMPILE(${VARIABLE}
      ${CMAKE_BINARY_DIR}
      ${CMAKE_SOURCE_DIR}/cmake/Modules/GGLCheckFunctionExists.c
      COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
      CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_FUNCTION_DEFINITIONS}
      "${CHECK_FUNCTION_EXISTS_ADD_LIBRARIES}"
      "${CHECK_FUNCTION_EXISTS_ADD_INCLUDES}"
      "${CHECK_FUNCTION_EXISTS_LINK_DIRECTORIES}"
      OUTPUT_VARIABLE OUTPUT)
    IF(${VARIABLE})
      SET(${VARIABLE} 1 CACHE INTERNAL "Have function ${FUNCTION}")
      MESSAGE(STATUS "Looking for ${FUNCTION} - found")
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log 
        "Determining if the function ${FUNCTION} exists passed with the following output:\n"
        "${OUTPUT}\n\n")
    ELSE(${VARIABLE})
      MESSAGE(STATUS "Looking for ${FUNCTION} - not found")
      SET(${VARIABLE} "" CACHE INTERNAL "Have function ${FUNCTION}")
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log 
        "Determining if the function ${FUNCTION} exists failed with the following output:\n"
        "${OUTPUT}\n\n")
    ENDIF(${VARIABLE})
  ENDIF("${VARIABLE}" MATCHES "^${VARIABLE}$")
ENDMACRO(GGL_CHECK_FUNCTION_EXISTS)
