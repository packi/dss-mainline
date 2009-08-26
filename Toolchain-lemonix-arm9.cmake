
# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(COMPILER_PREFIX arm-linux-)
SET(BOARD_SYSROOT /home/aizo/src/arm9/libs/sysroot/)
SET(LOCAL_SYSROOT /home/aizo/src/arm9/libs/sysroot/)

SET(CMAKE_C_COMPILER "${COMPILER_PREFIX}gcc")
SET(CMAKE_CXX_COMPILER "${COMPILER_PREFIX}g++")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}-mcpu=arm9")

include_directories(${LOCAL_SYSROOT}include/ ${BOARD_SYSROOT}include/)
link_directories(${LOCAL_SYSROOT}lib ${BOARD_SYSROOT}lib)

TARGET_LINK_LIBRARIES(dss boost_system-mt)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  ${BOARD_SYSROOT} ${LOCAL_SYSROOT})

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

