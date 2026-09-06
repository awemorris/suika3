#
# [Usage]
#
# list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
# find_package(Playfield REQUIRED)
# target_link_libraries(entry PUBLIC Playfield::Playfield)

find_path(PLAYFIELD_INCLUDE_DIR 
  NAMES palyfield/playfield.h
  HINTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/playfield/include
  NO_DEFAULT_PATH
)

find_library(PLAYFIELD_LIBRARY 
  NAMES playfield
  HINTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/playfield/lib/${OHOS_ARCH}
  NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Playfield 
  REQUIRED_VARS PLAYFIELD_LIBRARY PLAYFIELD_INCLUDE_DIR
)

if(Playfield_FOUND)
  set(STRATO_LIBRARIES ${PLAYFIELD_LIBRARY})
  set(STRATO_INCLUDE_DIRS ${PLAYFIELD_INCLUDE_DIR})

  if(NOT TARGET Playfield::Playfield)
      add_library(Playfield::Playfield static IMPORTED)
      set_target_properties(Playfield::Playfield PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PLAYFIELD_INCLUDE_DIR}"
        IMPORTED_LOCATION "${PLAYFIELD_LIBRARY}"
      )
  endif()
endif()
