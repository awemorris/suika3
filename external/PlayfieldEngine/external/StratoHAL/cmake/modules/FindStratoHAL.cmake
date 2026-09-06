#
# [Usage]
#
# find_package(StratoHAL REQUIRED)
# target_link_libraries(myapp StratoHAL::strato)
#
# find_package(StratoHAL REQUIRED COMPONENTS pack)
# target_link_libraries(myapp StratoHAL::strato StratoHAL::pack)
#

find_path(STRATO_INCLUDE_DIR NAMES strato/strato.h)
find_library(STRATO_LIBRARY NAMES strato)

set(_STRATO_REQUIRED_VARS STRATO_LIBRARY STRATO_INCLUDE_DIR)

if("pack" IN_LIST StratoHAL_FIND_COMPONENTS)
    find_library(STRATOPACK_LIBRARY NAMES stratopack)
    if(STRATOPACK_LIBRARY)
        set(StratoHAL_pack_FOUND TRUE)
        list(APPEND _STRATO_REQUIRED_VARS STRATOPACK_LIBRARY)
    else()
        set(StratoHAL_pack_FOUND FALSE)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(StratoHAL
    HANDLE_COMPONENTS
    REQUIRED_VARS ${_STRATO_REQUIRED_VARS}
)

if(StratoHAL_FOUND)
    set(STRATO_INCLUDE_DIRS ${STRATO_INCLUDE_DIR})
    
    if(NOT TARGET StratoHAL::strato)
        add_library(StratoHAL::strato UNKNOWN IMPORTED)
        set_target_properties(StratoHAL::strato PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${STRATO_INCLUDE_DIR}"
            IMPORTED_LOCATION "${STRATO_LIBRARY}"
        )
    endif()

    if(StratoHAL_pack_FOUND AND NOT TARGET StratoHAL::pack)
        add_library(StratoHAL::pack UNKNOWN IMPORTED)
        set_target_properties(StratoHAL::pack PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${STRATO_INCLUDE_DIR}"
            IMPORTED_LOCATION "${STRATOPACK_LIBRARY}"
        )
    endif()
endif()
