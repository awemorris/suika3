#
# [Usage]
#
# list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
# find_package(StratoHAL REQUIRED)
# target_link_libraries(entry PUBLIC StratoHAL::StratoHAL)

find_path(STRATO_INCLUDE_DIR 
  NAMES strato/strato.h
  HINTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/StratoHAL/include
  NO_DEFAULT_PATH
)

find_library(STRATO_LIBRARY 
  NAMES strato
  HINTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/StratoHAL/lib/${OHOS_ARCH}
  NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(StratoHAL 
  REQUIRED_VARS STRATO_LIBRARY STRATO_INCLUDE_DIR
)

if(StratoHAL_FOUND)
  set(STRATO_LIBRARIES ${STRATO_LIBRARY})
  set(STRATO_INCLUDE_DIRS ${STRATO_INCLUDE_DIR})

  if(NOT TARGET StratoHAL::StratoHAL)
      add_library(StratoHAL::StratoHAL static IMPORTED)
      set_target_properties(StratoHAL::StratoHAL PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${STRATO_INCLUDE_DIR}"
        IMPORTED_LOCATION "${STRATO_LIBRARY}"
      )
  endif()
endif()
