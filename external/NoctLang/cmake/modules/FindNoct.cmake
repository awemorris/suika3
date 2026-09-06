#
# [Usage]
#
# find_package(Noct REQUIRED)
# target_link_libraries(myapp Noct::noct)
#
# find_package(Noct REQUIRED COMPONENTS api)
# target_link_libraries(myapp Noct::noct Noct::api)
#

find_path(NOCT_INCLUDE_DIR NAMES noct/noct.h)
find_library(NOCT_LIBRARY NAMES noct)

set(_NOCT_REQUIRED_VARS NOCT_LIBRARY NOCT_INCLUDE_DIR)

if("api" IN_LIST Noct_FIND_COMPONENTS)
    find_library(NOCTAPI_LIBRARY NAMES noctapi)
    if(NOCTAPI_LIBRARY)
        set(Noct_api_FOUND TRUE)
        list(APPEND _NOCT_REQUIRED_VARS NOCTAPI_LIBRARY)
    else()
        set(Noct_api_FOUND FALSE)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Noct
    HANDLE_COMPONENTS
    REQUIRED_VARS ${_NOCT_REQUIRED_VARS}
)

if(Noct_FOUND)
    set(NOCT_INCLUDE_DIRS ${NOCT_INCLUDE_DIR})
    
    if(NOT TARGET Noct::noct)
        add_library(Noct::noct UNKNOWN IMPORTED)
        set_target_properties(Noct::noct PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${NOCT_INCLUDE_DIR}"
            IMPORTED_LOCATION "${NOCT_LIBRARY}"
        )
    endif()

    if(Noct_api_FOUND AND NOT TARGET Noct::api)
        add_library(Noct::api UNKNOWN IMPORTED)
        set_target_properties(Noct::api PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${NOCT_INCLUDE_DIR}"
            IMPORTED_LOCATION "${NOCTAPI_LIBRARY}"
            INTERFACE_LINK_LIBRARIES Noct::noct
        )
    endif()
endif()
