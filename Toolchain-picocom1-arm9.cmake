
# this one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(COMPILER_PREFIX /opt/emlix/picocom1/bin/arm-linux-gnueabi-)
SET(CMAKE_C_COMPILER "${COMPILER_PREFIX}gcc")
SET(CAKE_CXX_COMPILER "${COMPILER_PREFIX}g++")
#SET(CMAKE_LINKER:FILEPATH "${COMPILER_PREFIX}ld")

#SET(CMAKE_RANLIB "${COMPILER_PREFIX}ranlib")
#SET(CMAKE_AR "${COMPILER_PREFIX}ar")

include_directories(/home/patrick/test/sysroot/include/ /opt/emlix/picocom1/sysroot/include/)
link_directories(/home/patrick/test/sysroot/lib /opt/emlix/picocom1/sysroot/lib)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  /opt/emlix/picocom1/sysroot/ /home/patrick/test/sysroot)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

