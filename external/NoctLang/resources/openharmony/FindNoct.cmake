#
# [Usage]
#
# list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
# find_package(Noct REQUIRED)
# target_link_libraries(entry PUBLIC Noct::Noct)

find_path(NOCT_INCLUDE_DIR 
  NAMES noct/noct.h
  HINTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/noct/include
  NO_DEFAULT_PATH
)

find_library(NOCT_LIBRARY 
  NAMES noct
  HINTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/noct/lib/${OHOS_ARCH}
  NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(StratoHAL 
  REQUIRED_VARS STRATO_LIBRARY STRATO_INCLUDE_DIR
)

if(Noct_FOUND)
  set(NOCT_LIBRARIES ${NOCT_LIBRARY})
  set(NOCT_INCLUDE_DIRS ${NOCT_INCLUDE_DIR})

  if(NOT TARGET Noct::Noct)
      add_library(Noct::Noct static IMPORTED)
      set_target_properties(Noct::Noct PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${NOCT_INCLUDE_DIR}"
        IMPORTED_LOCATION "${NOCT_LIBRARY}"
      )
  endif()
endif()
