# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project(MyApp1)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

#set(PLAYFIELD_TARGET_OPENHARMONY "ON")
#set(PLAYFIELD_ENABLE_STATIC "ON")
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/PlayfieldEngine)

set(LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs)
add_library(playfield STATIC IMPORTED)
set_target_properties(playfield PROPERTIES IMPORTED_LOCATION ${LIBS_DIR}/${OHOS_ARCH}/libplayfield.a)

add_library(engine SHARED napi_init.cpp)
target_link_libraries(
  engine
  PUBLIC
  playfield
  libace_napi.z.so
  libace_ndk.z.so
  librawfile.z.so
  libhilog_ndk.z.so
  EGL
  GLESv3
  OpenSLES
)