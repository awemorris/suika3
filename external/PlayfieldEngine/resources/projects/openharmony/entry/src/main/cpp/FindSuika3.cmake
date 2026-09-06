#
# [Usage]
#
# list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
# find_package(Suika3 REQUIRED)
# target_link_libraries(entry PUBLIC Suika3::Suika3)

find_path(SUIKA3_INCLUDE_DIR
  NAMES suika3/suika3.h
  PATHS "${NATIVERENDER_ROOT_PATH}/third_party/suika3/include"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)

find_library(SUIKA3_LIBRARY
  NAMES suika3
  PATHS "${NATIVERENDER_ROOT_PATH}/third_party/suika3/lib/${OHOS_ARCH}"
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Suika3
  REQUIRED_VARS SUIKA3_LIBRARY SUIKA3_INCLUDE_DIR
)

if(Suika3_FOUND)
  if(NOT TARGET Suika3::Suika3)
    add_library(Suika3::Suika3 STATIC IMPORTED)
    set_target_properties(Suika3::Suika3 PROPERTIES
      IMPORTED_LOCATION "${SUIKA3_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${SUIKA3_INCLUDE_DIR}"
    )
  endif()
endif()
