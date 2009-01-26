
# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(COMPILER_PREFIX /opt/emlix/picocom1/bin/arm-linux-gnueabi-)
SET(BOARD_SYSROOT /opt/emlix/picocom1/sysroot/)
SET(LOCAL_SYSROOT /home/patrick/test/sysroot/)

SET(CMAKE_C_COMPILER "${COMPILER_PREFIX}gcc")
SET(CAKE_CXX_COMPILER "${COMPILER_PREFIX}g++")

include_directories(${LOCAL_SYSROOT}include/ ${BOARD_SYSROOT}include/)
link_directories(${LOCAL_SYSROOT}lib ${BOARD_SYSROOT}lib)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  ${BOARD_SYSROOT} ${LOCAL_SYSROOT})

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

