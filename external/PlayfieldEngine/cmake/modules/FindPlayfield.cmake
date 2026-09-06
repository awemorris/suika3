#
# [Usage]
#
# find_package(Playfield REQUIRED)
# target_link_libraries(myapp Playfield::playfield)
#

find_path(PLAYFIELD_INCLUDE_DIR NAMES playfield/playfield.h)
find_library(PLAYFIELD_LIBRARY NAMES playfield)

set(_PLAYFIELD_REQUIRED_VARS PLAYFIELD_LIBRARY PLAYFIELD_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Playfield
    HANDLE_COMPONENTS
    REQUIRED_VARS ${_PLAYFIELD_REQUIRED_VARS}
)

if(Playfield_FOUND)
    set(PLAYFIELD_INCLUDE_DIRS ${PLAYFIELD_INCLUDE_DIR})
    
    if(NOT TARGET Playfield::playfield)
        add_library(Playfield::playfield UNKNOWN IMPORTED)
        set_target_properties(Playfield::playfield PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PLAYFIELD_INCLUDE_DIR}"
            IMPORTED_LOCATION "${PLAYFIELD_LIBRARY}"
        )
    endif()
endif()
