find_library(GSOAP_LIBRARY NAMES gsoap++)
find_path(GSOAP_INCLUDE_DIR stdsoap2.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GSOAP DEFAULT_MSG GSOAP_LIBRARY GSOAP_INCLUDE_DIR)

if(GSOAP_FOUND)
    set(GSOAP_LIBRARIES ${GSOAP_LIBRARY})
    set(GSOAP_INCLUDE_DIRS ${GSOAP_INCLUDE_DIR})
endif()
