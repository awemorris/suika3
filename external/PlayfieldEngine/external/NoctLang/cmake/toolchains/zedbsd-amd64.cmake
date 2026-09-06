# Noct cross build for zedBSD amd64 LP64.

set(ZEDBSD TRUE)
set(UNIX TRUE)
set(CMAKE_EXECUTABLE_SUFFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".a")
set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_DL_LIBS "")
set(CMAKE_SHARED_LIBRARY_SUPPORTED FALSE)

if((NOT DEFINED ZEDBSD_SOURCE_DIR OR ZEDBSD_SOURCE_DIR STREQUAL "") AND
   DEFINED ENV{ZEDBSD_SOURCE_DIR} AND
   NOT "$ENV{ZEDBSD_SOURCE_DIR}" STREQUAL "")
  set(ZEDBSD_SOURCE_DIR "$ENV{ZEDBSD_SOURCE_DIR}" CACHE PATH
      "Absolute path to the zedBSD source tree")
endif()

if(NOT DEFINED ZEDBSD_SOURCE_DIR OR ZEDBSD_SOURCE_DIR STREQUAL "")
  message(FATAL_ERROR
    "The zedBSD preset requires ZEDBSD_SOURCE_DIR to name the zedBSD source tree")
endif()
if(NOT IS_ABSOLUTE "${ZEDBSD_SOURCE_DIR}")
  message(FATAL_ERROR
    "ZEDBSD_SOURCE_DIR must be an absolute path: ${ZEDBSD_SOURCE_DIR}")
endif()
if(NOT IS_DIRECTORY "${ZEDBSD_SOURCE_DIR}")
  message(FATAL_ERROR
    "ZEDBSD_SOURCE_DIR is not a directory: ${ZEDBSD_SOURCE_DIR}")
endif()

get_filename_component(ZEDBSD_SOURCE_DIR "${ZEDBSD_SOURCE_DIR}" REALPATH)
set(ZEDBSD_SOURCE_DIR "${ZEDBSD_SOURCE_DIR}" CACHE PATH
    "Absolute path to the zedBSD source tree" FORCE)

set(_NOCT_ZEDBSD_REQUIRED_FILES
  Makefile
  include/hal/arch/amd64.h
  include/uapi/zedbsd/system.h
  libc/include/stdint.h
  platform/amd64/user.ld
  userland/packages/lang/noct/zedbsd.cmake
)
foreach(_NOCT_ZEDBSD_FILE IN LISTS _NOCT_ZEDBSD_REQUIRED_FILES)
  if(NOT EXISTS "${ZEDBSD_SOURCE_DIR}/${_NOCT_ZEDBSD_FILE}")
    message(FATAL_ERROR
      "ZEDBSD_SOURCE_DIR is missing required file: ${_NOCT_ZEDBSD_FILE}")
  endif()
endforeach()
unset(_NOCT_ZEDBSD_FILE)
unset(_NOCT_ZEDBSD_REQUIRED_FILES)

list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ZEDBSD_SOURCE_DIR)
list(REMOVE_DUPLICATES CMAKE_TRY_COMPILE_PLATFORM_VARIABLES)

set(CMAKE_SYSTEM_NAME zedBSD)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(_NOCT_ZEDBSD_CC NAMES gcc)
find_program(_NOCT_ZEDBSD_AR NAMES ar)
find_program(_NOCT_ZEDBSD_RANLIB NAMES ranlib)
if(NOT _NOCT_ZEDBSD_CC OR NOT _NOCT_ZEDBSD_AR OR
   NOT _NOCT_ZEDBSD_RANLIB)
  message(FATAL_ERROR
    "The zedBSD amd64 build requires gcc, ar, and ranlib in PATH")
endif()
set(CMAKE_C_COMPILER "${_NOCT_ZEDBSD_CC}" CACHE FILEPATH
    "C compiler for zedBSD amd64" FORCE)
set(CMAKE_ASM_COMPILER "${_NOCT_ZEDBSD_CC}" CACHE FILEPATH
    "Assembler driver for zedBSD amd64" FORCE)
set(CMAKE_AR "${_NOCT_ZEDBSD_AR}" CACHE FILEPATH
    "Archiver for zedBSD amd64" FORCE)
set(CMAKE_RANLIB "${_NOCT_ZEDBSD_RANLIB}" CACHE FILEPATH
    "Archive indexer for zedBSD amd64" FORCE)
unset(_NOCT_ZEDBSD_CC CACHE)
unset(_NOCT_ZEDBSD_AR CACHE)
unset(_NOCT_ZEDBSD_RANLIB CACHE)

set(CMAKE_C_FLAGS_INIT
  "-m64 -march=x86-64 -mno-red-zone -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-builtin -fno-common -ffunction-sections -fdata-sections -fno-strict-aliasing -nostdinc -U__linux__ -U__linux -Ulinux -D__ZEDBSD__ -DHAL_ARCH_AMD64 -DZEDBSD_USER_ABI_LP64")
set(CMAKE_ASM_FLAGS_INIT "-nostdinc")
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES
  "${ZEDBSD_SOURCE_DIR}/include"
  "${ZEDBSD_SOURCE_DIR}/include/uapi"
  "${ZEDBSD_SOURCE_DIR}/src"
  "${ZEDBSD_SOURCE_DIR}"
  "${ZEDBSD_SOURCE_DIR}/libc/include"
)

set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
set(CMAKE_SKIP_RPATH TRUE)
set(CMAKE_BUILD_RPATH "")
set(CMAKE_INSTALL_RPATH "")

set(CMAKE_FIND_ROOT_PATH "${ZEDBSD_SOURCE_DIR}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
