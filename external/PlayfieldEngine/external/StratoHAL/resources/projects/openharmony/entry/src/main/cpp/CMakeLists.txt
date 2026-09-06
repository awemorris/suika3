# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project(MyApp1)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

target_include_directories(entry PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/strato/include
)

set(LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs)
add_library(strato STATIC IMPORTED)
set_target_properties(strato PROPERTIES IMPORTED_LOCATION ${LIBS_DIR}/${OHOS_ARCH}/libstrato.a)

add_library(engine SHARED napi_init.cpp)
target_link_libraries(
  engine
  PUBLIC
  suika3
  libace_napi.z.so
  libace_ndk.z.so
  librawfile.z.so
  libhilog_ndk.z.so
  EGL
  GLESv3
  OpenSLES
)
