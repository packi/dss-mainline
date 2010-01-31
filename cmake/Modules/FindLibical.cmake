find_library(LIBICAL_LIBRARY NAMES ical)
find_path(LIBICAL_INCLUDE_DIR libical/ical.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libical DEFAULT_MSG LIBICAL_LIBRARY LIBICAL_INCLUDE_DIR)

if(LIBICAL_FOUND)
    set(LIBICAL_LIBRARIES ${LIBICAL_LIBRARY})
    set(LIBICAL_INCLUDE_DIRS ${LIBICAL_INCLUDE_DIR})
endif()
