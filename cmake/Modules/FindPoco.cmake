find_library(POCO_LIBRARY PocoXML)
find_path(POCO_INCLUDE_DIR Poco/XML/XML.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Poco DEFAULT_MSG POCO_LIBRARY POCO_INCLUDE_DIR)

if(POCO_FOUND)
    set(POCO_LIBRARIES ${POCO_LIBRARY})
    set(POCO_INCLUDE_DIRS ${POCO_INCLUDE_DIR})
endif()
