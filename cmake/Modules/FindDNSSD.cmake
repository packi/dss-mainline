find_library(DNSSD_LIBRARY-COMMON NAMES dns_sd)
find_path(DNSSD_INCLUDE_DIR dns_sd.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DNSSD DEFAULT_MSG DNSSD_LIBRARY-COMMON DNSSD_INCLUDE_DIR)

if(DNSSD_FOUND)
    set(DNSSD_LIBRARIES ${DNSSD_LIBRARY-COMMON})
    set(DNSSD_INCLUDE_DIRS ${DNSSD_INCLUDE_DIR})
endif()
