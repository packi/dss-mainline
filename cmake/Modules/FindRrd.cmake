find_library(RRD_LIBRARY NAMES rrd_th)
find_path(RRD_INCLUDE_DIR rrd.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Rrd DEFAULT_MSG RRD_LIBRARY RRD_INCLUDE_DIR)

if(RRD_FOUND)
    set(RRD_LIBRARIES ${RRD_LIBRARY})
    set(RRD_INCLUDE_DIRS ${RRD_INCLUDE_DIR})
endif()
