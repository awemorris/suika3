cmake_minimum_required(VERSION 3.16)
project(NoctLang LANGUAGES C)

#
# Build Configuration
#

option(NOCTLANG_ENABLE_CLI          "Build a CLI binary"          ON)
option(NOCTLANG_ENABLE_JIT          "Enable JIT compilation"      ON)
option(NOCTLANG_ENABLE_STATIC       "Build a static library"      OFF)
option(NOCTLANG_ENABLE_SHARED       "Build a shared library"      OFF)
option(NOCTLANG_ENABLE_OBJECT       "Build objects only"          OFF)
option(NOCTLANG_ENABLE_MULTITHREAD  "Enable multithread support"  OFF)
option(NOCTLANG_ENABLE_I18N         "Enable translation feature"  OFF)
option(NOCTLANG_ENABLE_MDOC         "Install mdoc(7) man page"    OFF)
option(NOCTLANG_DISABLE_INSTALL     "Install mdoc(7) man page"    OFF)
option(NOCTLANG_TARGET_WASM         "Build for Emscripten"        OFF)
option(NOCTLANG_TARGET_UNITY        "Build for Unity"             OFF)

option(NOCTLANG_API_SYSTEM	    "Add System.* API"            ON)
option(NOCTLANG_API_CONSOLE	    "Add Console.* API"           ON)
option(NOCTLANG_API_MATH	    "Add Math.* API"              ON)

#
# Debug/Release
#

# Use "Release" build type by default.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

# Debug Configuration
if(MSVC)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /Zi /DDEBUG /UNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /DDEBUG /UNDEBUG")
else()
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_RELEASE} -O0 -g3 -UNDEBUG")
endif()

# Release Configuration
if(MSVC)
  # MSVC
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  # GCC/Clang
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -g0 -DNDEBUG")
endif()

#
# Checks
#

include(CheckIncludeFile)

check_include_file("stdint.h" HAVE_STDINT_H)
if(HAVE_STDINT_H)
  add_definitions(-DHAVE_STDINT_H=1)
endif()

check_include_file("inttypes.h" HAVE_INTTYPES_H)
if(HAVE_INTTYPES_H)
  add_definitions(-DHAVE_INTTYPES_H=1)
endif()

check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
if(HAVE_SYS_TYPES_H)
  add_definitions(-DHAVE_SYS_TYPES_H=1)
endif()

#
# FLEX
#

#find_package(FLEX REQUIRED)
#
#FLEX_TARGET(Lexer
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/lexer.l
#  ${CMAKE_CURRENT_BINARY_DIR}/lexer.yy.c
#  COMPILE_FLAGS "--prefix=ast_yy"
#)

#
# BISON
#

#find_package(BISON REQUIRED)
#
#BISON_TARGET(Parser
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/parser.y
#  ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.c
#  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.h
#  COMPILE_FLAGS "-p ast_yy -Wcounterexamples"
#)

#
# Library Target
#

# Base source.
set(NOCTLANG_BASE_SOURCE
  ${FLEX_Lexer_OUTPUTS}
  ${BISON_Parser_OUTPUT_SOURCE}
  src/core/ast.c
  src/core/hir.c
  src/core/lir.c
  src/core/noct.c
  src/core/runtime.c
  src/core/interpreter.c
  src/core/jit.c
  src/core/execution.c
  src/core/intrinsics.c
  src/core/gc.c
  src/core/hash.c
  src/core/lexer.yy.c
  src/core/parser.tab.c
)

# Library sources.
set(NOCTLANG_API_SOURCES)
if(NOCTLANG_API_SYSTEM)
  set(NOCTLANG_API_SOURCES ${NOCTLANG_API_SOURCES} src/api/api-system.c)
endif()
if(NOCTLANG_API_CONSOLE)
  set(NOCTLANG_API_SOURCES ${NOCTLANG_API_SOURCES} src/api/api-console.c)
endif()
if(NOCTLANG_API_MATH)
  set(NOCTLANG_API_SOURCES ${NOCTLANG_API_SOURCES} src/api/api-math.c)
endif()

# i18n sources.
if(NOCTLANG_ENABLE_I18N)
  set(NOCTLANG_I18N_SOURCE
    src/i18n/i18n.c
    src/i18n/translation.c
  )
endif()

# Library type.
if(NOCTLANG_ENABLE_SHARED)
  set(NOCTLANG_LIB_TYPE SHARED)
elseif(NOCTLANG_ENABLE_OBJECT)
  set(NOCTLANG_LIB_TYPE OBJECT)
else()
  set(NOCTLANG_LIB_TYPE STATIC)
endif()

# Add the library.
add_library(
  noctlang
  ${NOCTLANG_LIB_TYPE}

  ${NOCTLANG_BASE_SOURCE}
  ${NOCTLANG_API_SOURCES}
  ${NOCTLANG_I18N_SOURCE}
)

# Add include directories.
target_include_directories(noctlang PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(noctlang PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)

# Add macros.
if(NOCTLANG_ENABLE_SHARED)
  target_compile_definitions(noctlang PRIVATE NOCT_USE_DLL NOCT_DLL_IMPL)
endif()
if(NOCTLANG_ENABLE_JIT)
  target_compile_definitions(noctlang PRIVATE NOCT_USE_JIT)
endif()
if(NOCTLANG_ENABLE_MULTITHREAD)
  target_compile_definitions(noctlang PRIVATE NOCT_USE_MULTITHREAD)
endif()
if(NOCTLANG_ENABLE_I18N)
  target_compile_definitions(noctlang PRIVATE NOCT_USE_TRANSLATION)
endif()  
if(NOCTLANG_TARGET_UNITY)
  target_compile_definitions(noctlang PRIVATE NOCT_TARGET_UNITY)
endif()

# Win32 Unicode settings.
if(WIN32)
  if(MSVC)
    target_compile_options(noctlang PRIVATE /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_options(noctlang PRIVATE -municode -finput-charset=utf-8 -fexec-charset=utf-8)
  endif()
endif()

# Export variables.
set(NOCTLANG_INCLUDE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(NOCTLANG_INCLUDE_DIRS ${NOCTLANG_INCLUDE_DIR})
set(NOCTLANG_LIBRARY      $<TARGET_FILE:noctlang>)
set(NOCTLANG_LIBRARIES    ${LOCTLANG_LIBRARY})

#
# CLI Target
#

if(NOCTLANG_ENABLE_CLI)
  add_executable(
    noct
    src/backend/cback.c
    src/backend/elback.c
    src/cli/cli-main.c
    src/cli/cli-run.c
    src/cli/cli-compile.c
    src/cli/cli-ctrans.c
    src/cli/cli-eltrans.c
    src/cli/cli-repl.c
    src/cli/cli-ffi.c
  )

  # Define feature macros.
  if(NOCTLANG_ENABLE_SHARED)
    target_compile_definitions(noct PRIVATE NOCT_USE_DLL)
  endif()
  if(NOCTLANG_ENABLE_JIT)
    target_compile_definitions(noct PRIVATE NOCT_USE_JIT)
  endif()
  if(NOCTLANG_ENABLE_I18N)
    target_compile_definitions(noct PRIVATE NOCT_USE_TRANSLATION)
  endif()  

  # Include include/noct/, src/core/, and src/backend/
  target_include_directories(noct PRIVATE
     ${NOCTLANG_INCLUDE_DIRS}
     ${CMAKE_CURRENT_SOURCE_DIR}/src/core
     ${CMAKE_CURRENT_SOURCE_DIR}/src/backend
  )

  # Link libnoctlang.
  target_link_libraries(noct
    PRIVATE
    noctlang
  )

  # Add -m on UNIX.
  if(UNIX)
    target_link_libraries(noct PRIVATE m)
  endif()

  # Win32 Unicode settings.
  if(WIN32)
    if(MSVC)
      target_compile_options(noct PRIVATE /utf-8)
    else()
      target_compile_options(noct PRIVATE -municode -finput-charset=utf-8 -fexec-charset=utf-8)
    endif()
  endif()
endif()

#
# Install
#

if(NOT NOCTLANG_DISABLE_INSTALL)
  include(GNUInstallDirs)

  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/noct/noct.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/noct/c89compat.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/noct)

  install(TARGETS noctlang RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})

  if(NOCTLANG_ENABLE_CLI)
    install(TARGETS noct RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    if(NOCTLANG_ENABLE_MDOC)
      set(NOCTLANG_MANPAGE "${CMAKE_SOURCE_DIR}/man/noct.1.mdoc")
    else()
      set(NOCTLANG_MANPAGE "${CMAKE_SOURCE_DIR}/man/noct.1.man")
    endif()
    install(FILES ${NOCTLANG_MANPAGE} DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 RENAME noct.1)
  endif()
endif()
