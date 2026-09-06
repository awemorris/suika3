cmake_minimum_required(VERSION 3.5.0)
project(MyApp1)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
  include(${PACKAGE_FIND_FILE})
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Suika3 REQUIRED)

add_library(
  engine
  SHARED
  napi_init.cpp
  library.c
)

target_link_libraries(
  engine
  PUBLIC

  -Wl,--whole-archive
  Suika3::Suika3
  -Wl,--no-whole-archive

  libace_napi.z.so
  libace_ndk.z.so
  librawfile.z.so
  libhilog_ndk.z.so
  EGL
  GLESv3
  OpenSLES
)
