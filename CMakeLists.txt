cmake_minimum_required(VERSION 3.16)
project(Noct VERSION 1.0 LANGUAGES C)

# Let modern Apple linkers resolve archive dependencies without duplicates.
if(APPLE AND POLICY CMP0156)
  cmake_policy(SET CMP0156 NEW)
endif()
if(APPLE AND POLICY CMP0179)
  cmake_policy(SET CMP0179 NEW)
endif()

#
# Build Configuration
#

# Choose one.
option(NOCT_ENABLE_STATIC         "Build a static library"                OFF)
option(NOCT_ENABLE_SHARED         "Build a shared library"                OFF)
option(NOCT_ENABLE_OBJECT         "Build objects only"                    OFF)

# Core runtime options.
option(NOCT_ENABLE_JIT            "Enable JIT compilation"                OFF)
option(NOCT_ENABLE_OPTIMIZER      "Enable optimizer"                      OFF)
option(NOCT_ENABLE_MULTITHREAD    "Enable multithread support"            OFF)
option(NOCT_ENABLE_ACCEL          "Enable accelerator support"           OFF)

# Program config options.
option(NOCT_ENABLE_CLI            "Enable CLI"                            OFF)
option(NOCT_ENABLE_REPL           "Enable reusable REPL and CLI mode"     OFF)
option(NOCT_ENABLE_BCBACKEND      "Enable bytecode backend"               OFF)
option(NOCT_ENABLE_CBACKEND       "Enable C backend"                      OFF)
option(NOCT_ENABLE_ELBACKEND      "Enable Emacs Lisp backend"             OFF)
option(NOCT_ENABLE_SCMBACKEND     "Enable Scheme backend"                 OFF)

# Multilingualization options.
option(NOCT_ENABLE_I18N           "Enable translation (built-in)"         OFF)
option(NOCT_ENABLE_I18N_LIBINTL   "Enable translation (gettext)"          OFF)

# API options.
option(NOCT_ENABLE_API            "Enable standard API"                   OFF)
option(NOCT_ENABLE_API_SYSTEM     "Enable the System standard API"        OFF)
option(NOCT_ENABLE_API_CONSOLE    "Enable the Console standard API"       OFF)
option(NOCT_ENABLE_API_FILE       "Enable the File standard API"          OFF)
option(NOCT_ENABLE_API_THREAD     "Enable the Thread standard API"        OFF)
option(NOCT_ENABLE_API_HTTPSERVER "Enable the HttpServer standard API"    OFF)
option(NOCT_ENABLE_API_TERM       "Enable the Term non-standard API"      OFF)
option(NOCT_ENABLE_API_BEUI       "Enable the BeUI non-standard API"      OFF)
option(NOCT_ENABLE_API_PROCESS    "Enable the Process non-standard API"   OFF)
option(NOCT_ENABLE_API_NWT1       "Enable immutable NWT1 model weights"   OFF)

#
# Manual target options.
#  - These targets cannot be automatically determined.
#
option(NOCT_TARGET_UNITY          "Build for Unity"                       OFF)
option(NOCT_TARGET_OPENHARMONY    "Build for OpenHarmony"                 OFF)
option(NOCT_TARGET_MACOS7         "Build for MacOS 7"                     OFF)
option(NOCT_TARGET_DOS4G          "Build for DOS4G"                       OFF)
option(NOCT_TARGET_PC98DOS        "Build for MS-DOS on PC-98"             OFF)
option(NOCT_TARGET_ZEDBSD         "Build for zedBSD"                      OFF)

# Installation options.
option(NOCT_ENABLE_INSTALL        "Enable installation"                   OFF)
option(NOCT_ENABLE_MDOC           "Install mdoc(7) man page"              OFF)

if(NOCT_ENABLE_INSTALL)
  include(GNUInstallDirs)
endif()

# ---

#
# Debug/Release
#

# Debug Configuration
if(MSVC)
  # MSVC
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /Zi /DDEBUG /UNDEBUG")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  # GCC/Clang
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g3 -DDEBUG -Wall -Werror -Wextra -Wundef -Wconversion")
endif()

# Release Configuration
if(MSVC)
  # MSVC
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /DNDEBUG")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  # GCC/Clang
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os -g -DNDEBUG")
endif()

# ---

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

# ---

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

# ---

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

# ---

#
# gettext
#

if(NOCT_ENABLE_I18N_LIBINTL)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
  find_package(INTL REQUIRED)
  include(GNUInstallDirs)
  add_definitions(-DLOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}")
endif()

# ---

#
# Library Targets
#

# Determine the library type.
if(NOCT_ENABLE_SHARED)
  set(NOCT_LIB_TYPE SHARED)
elseif(NOCT_ENABLE_OBJECT)
  set(NOCT_LIB_TYPE OBJECT)
else()
  set(NOCT_LIB_TYPE STATIC)
endif()

if(NOCT_ENABLE_CLI AND NOT NOCT_LIB_TYPE STREQUAL "STATIC")
  message(FATAL_ERROR "NOCT_ENABLE_CLI requires a static Noct library")
endif()
if(NOCT_ENABLE_CLI)
  if(NOT NOCT_ENABLE_API)
    message(FATAL_ERROR "NOCT_ENABLE_CLI requires NOCT_ENABLE_API")
  endif()
  if(NOT NOCT_ENABLE_API_SYSTEM)
    message(FATAL_ERROR "NOCT_ENABLE_CLI requires NOCT_ENABLE_API_SYSTEM")
  endif()
  if(NOT NOCT_ENABLE_API_CONSOLE)
    message(FATAL_ERROR "NOCT_ENABLE_CLI requires NOCT_ENABLE_API_CONSOLE")
  endif()
  if(NOT NOCT_ENABLE_API_FILE)
    message(FATAL_ERROR "NOCT_ENABLE_CLI requires NOCT_ENABLE_API_FILE")
  endif()
  if(NOCT_ENABLE_MULTITHREAD AND NOT NOCT_ENABLE_API_THREAD)
    message(FATAL_ERROR
      "Multithreaded NOCT_ENABLE_CLI requires NOCT_ENABLE_API_THREAD")
  endif()
endif()

if(NOCT_ENABLE_ACCEL)
  if(NOT NOCT_ENABLE_CLI)
    message(FATAL_ERROR "NOCT_ENABLE_ACCEL requires NOCT_ENABLE_CLI")
  endif()
  if(NOT NOCT_ENABLE_API)
    message(FATAL_ERROR "NOCT_ENABLE_ACCEL requires NOCT_ENABLE_API")
  endif()
  if(NOT NOCT_ENABLE_OPTIMIZER)
    message(FATAL_ERROR "NOCT_ENABLE_ACCEL requires NOCT_ENABLE_OPTIMIZER")
  endif()
endif()

# Keep focused accelerator tests available without adding another public
# accelerator build option.  These targets remain EXCLUDE_FROM_ALL.
set(NOCT_BUILD_ACCEL_COMMON_TESTS OFF)
if(    NOCT_LIB_TYPE STREQUAL "STATIC"
   AND NOCT_ENABLE_OPTIMIZER
   AND NOT CMAKE_CROSSCOMPILING)
  set(NOCT_BUILD_ACCEL_COMMON_TESTS ON)
endif()

# Determine the base files.
set(
  NOCT_BASE_SOURCE
  src/core/lexer.yy.c           # Lexer
  src/core/parser.tab.c         # Parser
  src/core/ast.c                # AST construction (called from parser)
  src/core/hir.c                # HIR (AST->HIR construction)
  src/core/lir.c                # LIR (HIR->LIR construction)
  src/core/execution.c          # Runtime execution helper part
  src/core/interpreter.c        # Runtime interpreter part
  src/core/jit.c                # Runtime JIT part
  src/core/gc.c                 # Runtime GC part
  src/core/runtime.c            # Runtime main part
  src/core/noct.c               # Public functions
  src/core/intrinsics.c         # Intrinsics
)

# Determine the object-model sources
if(NOCT_ENABLE_MULTITHREAD)
  # Multi-threaded.
  set(
    NOCT_OBJMODEL_SOURCE
    src/core/objectmodel-mt.c
  )
else()
  # Single-threaded.
  set(
    NOCT_OBJMODEL_SOURCE
    src/core/objectmodel-st.c
  )
endif()

# Determine the optimizer files.
if(NOCT_ENABLE_OPTIMIZER)
  set(
    NOCT_OPTIMIZER_SOURCE
    src/optimizer/fast.c                # __fast function contracts
    src/optimizer/hir_fast_checked.c    # Checked __fast lowering
    src/optimizer/hir_opt_analysis.c # Common optimizer analysis
    src/optimizer/hir_fast_func.c    # __fast checked-index optimization
    src/optimizer/hir_opt_abce.c     # ABCE pass
    src/optimizer/hir_opt_inline.c   # static __inline pass
    src/optimizer/hir_opt_simd.c     # SIMD auto-vectorization
    src/optimizer/hir_opt_unroll.c   # scalar Packed loop unroll
    src/optimizer/hir_opt_loop.c     # loop analysis
    src/optimizer/hir_opt_doall.c    # DOALL analysis
    src/optimizer/hir_opt_dosum.c    # DOSUM analysis
    src/optimizer/hir_opt_cse.c      # CSE pass
    src/optimizer/hir_opt_typed.c    # Stage B type lattice
  )
endif()

# Determine the C backend files.
if(NOCT_ENABLE_CBACKEND)
  set(
    NOCT_CBE_SOURCE
    src/backend/cback.c
  )
endif()

# Determine the bytecode backend files.
if(NOCT_ENABLE_BCBACKEND)
  set(
    NOCT_BCBE_SOURCE
    src/backend/bcback.c
    src/backend/bcback_file.c
  )
endif()

# Determine the Emacs Lisp backend files.
if(NOCT_ENABLE_ELBACKEND)
  set(
    NOCT_ELBE_SOURCE
    src/backend/elback.c
  )
endif()

# Determine the Scheme backend files.
if(NOCT_ENABLE_SCMBACKEND)
  set(
    NOCT_SCMBE_SOURCE
    src/backend/scmback.c
  )
endif()

# Determine the i18n files.
if(NOCT_ENABLE_I18N)
  if(NOT NOCT_ENABLE_I18N_LIBINTL)
    set(
      NOCT_I18N_SOURCE
      src/i18n/i18n.c
      src/i18n/translation.c
    )
  else()
    set(
      NOCT_I18N_SOURCE
      src/i18n/i18n.c
    )
  endif()
endif()

# Determine the reusable REPL session engine.
if(NOCT_ENABLE_REPL)
  set(
    NOCT_REPL_SOURCE
    src/cli/repl.c
  )
endif()

# accel
if(NOCT_ENABLE_ACCEL)
  set(
    NOCT_ACCEL_COMMON_SOURCE
    src/accel/accel_ir.c
    src/accel/accel_program.c
    src/accel/accel_residency.c
    src/accel/accel_compile.c
    src/accel/accel_context.c
    src/accel/accel_mutex.c
    src/accel/accel_rewrite.c
    src/accel/accel_lir_budget.c
    src/accel/accel_callback.c
    src/accel/accel_device.c
    src/accel/accel_runtime.c
    src/accel/accel_shader_source.c
    src/accel/accel.c
  )

  if(WIN32)
    list(
      APPEND
      NOCT_ACCEL_BACKEND_SOURCE
      src/accel/accel_dx12.c
    )
  endif()

  if(APPLE)
    enable_language(OBJC)
    list(
      APPEND
      NOCT_ACCEL_BACKEND_SOURCE
      src/accel/accel_metal.m
    )
  endif()

  if(   CMAKE_SYSTEM_NAME STREQUAL "Linux"
     OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    list(
      APPEND
      NOCT_ACCEL_BACKEND_SOURCE
      src/accel/accel_opengles.c
    )
  endif()
endif()

# Add the target "libnoct", the core sandboxed virtual machine.
add_library(
  noct
  ${NOCT_LIB_TYPE}

  ${NOCT_BASE_SOURCE}
  ${NOCT_OBJMODEL_SOURCE}
  ${NOCT_OPTIMIZER_SOURCE}
  ${NOCT_CBE_SOURCE}
  ${NOCT_BCBE_SOURCE}
  ${NOCT_ELBE_SOURCE}
  ${NOCT_SCMBE_SOURCE}
  ${NOCT_I18N_SOURCE}
  ${NOCT_REPL_SOURCE}
  ${NOCT_ACCEL_COMMON_SOURCE}
  ${NOCT_ACCEL_BACKEND_SOURCE}
)

# Set the shared library version for Linux distributions.
if(NOCT_ENABLE_SHARED)
  set_target_properties(noct PROPERTIES 
    VERSION 1.0.0 
    SOVERSION 1
  )
  set_target_properties(noct PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
  )
endif()

#
# Determine the optional API files.
#

set(
  NOCT_API_SOURCE
  src/api/api-regex.c           # Regex.*
)

# System.*
if(NOCT_ENABLE_API_SYSTEM)
  list(APPEND NOCT_API_SOURCE src/api/api-system.c)
endif()

# Console.*
if(NOCT_ENABLE_API_CONSOLE)
  list(APPEND NOCT_API_SOURCE src/api/api-console.c)
endif()

# File.*
if(NOCT_ENABLE_API_FILE)
  list(APPEND NOCT_API_SOURCE src/api/api-file.c)
endif()

# Thread.*
if(NOCT_ENABLE_API_THREAD AND NOCT_ENABLE_MULTITHREAD)
  list(APPEND NOCT_API_SOURCE src/api/api-thread.c)
endif()

# Process.*
if(NOCT_ENABLE_API_PROCESS)
  list(APPEND NOCT_API_SOURCE src/api/api-process.c)
endif()

# Term.*
if(NOCT_ENABLE_API_TERM)
  if(WIN32)
    list(APPEND NOCT_API_SOURCE src/api/api-term-win32.c)
  else()
    list(APPEND NOCT_API_SOURCE src/api/api-term-ansi.c)
  endif()
endif()

# HttpServer.*
if(NOCT_ENABLE_API_HTTPSERVER)
  list(APPEND NOCT_API_SOURCE src/api/api-httpserver.c)
endif()

# BeUI.*
if(NOCT_ENABLE_API_BEUI)
  if(NOCT_TARGET_ZEDBSD)
    list(APPEND NOCT_API_SOURCE src/api/api-beui-zedbsd.c)
  elseif(NOCT_TARGET_PC98DOS)
    list(APPEND NOCT_API_SOURCE src/api/api-beui-pc98.c)
  else()
    find_package(SDL2 CONFIG QUIET)
    if(TARGET SDL2::SDL2)
      set(NOCT_SDL2_TARGET SDL2::SDL2)
    else()
      find_package(PkgConfig REQUIRED)
      pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
      set(NOCT_SDL2_TARGET PkgConfig::SDL2)
    endif()
    list(APPEND NOCT_API_SOURCE src/api/api-beui-sdl2.c)
  endif()
endif()

# Add the target "libnoctapi".
if(NOCT_ENABLE_API)
  # Define the libnoctapi target.
  add_library(
    noctapi
    ${NOCT_LIB_TYPE}
    ${NOCT_API_SOURCE}
  )

  # Set the .so version.
  if(NOCT_ENABLE_SHARED)
    set_target_properties(noctapi PROPERTIES 
      VERSION 1.0.0 
      SOVERSION 1
    )
    set_target_properties(noctapi PROPERTIES
      C_VISIBILITY_PRESET hidden
      VISIBILITY_INLINES_HIDDEN ON
    )
  endif()

endif()

# ---

#
# CPPFLAGS
#

set(NOCT_PUBLIC_CPPFLAGS)
set(NOCT_PRIVATE_CPPFLAGS)

# Include Path (./include and ./src/core)
target_include_directories(noct PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include/noct
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)
target_include_directories(noct PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/optimizer
)
if(NOCT_ENABLE_API)
  target_include_directories(noctapi PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/noct
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
  )
  target_include_directories(noctapi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/optimizer
  )
endif()

# If building a shared library.
if(NOCT_ENABLE_SHARED)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_USE_DLL)
endif()

# Multithread.
if(NOCT_ENABLE_MULTITHREAD)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_USE_MULTITHREAD)
endif()

# Require Windows Vista or later for SRW locks, condition variables, and
# strict UTF-16 to UTF-8 conversion flags used by the Windows backends.
if(WIN32)
  list(APPEND NOCT_PRIVATE_CPPFLAGS _WIN32_WINNT=0x0600)
endif()

# Optimizer.
if(NOCT_ENABLE_OPTIMIZER)
  list(APPEND NOCT_PRIVATE_CPPFLAGS NOCT_USE_OPTIMIZER)
endif()

# Keep accelerator attachment fields private to accelerator-aware targets.
if(NOCT_ENABLE_ACCEL)
  list(APPEND NOCT_PRIVATE_CPPFLAGS NOCT_USE_ACCEL)
elseif(NOCT_BUILD_ACCEL_COMMON_TESTS)
  target_compile_definitions(noct PRIVATE NOCT_USE_ACCEL)
  if(NOCT_ENABLE_API)
    target_compile_definitions(noctapi PRIVATE NOCT_USE_ACCEL)
  endif()
endif()

# JIT.
if(NOCT_ENABLE_JIT)
  # Exclude OpenBSD
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_USE_JIT)
  endif()
endif()

# Translation. (non-gettext, original implementation)
if(NOCT_ENABLE_I18N)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_USE_TRANSLATION)
endif()  

# Translation. (gettext)
if(NOCT_ENABLE_I18N_LIBINTL)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_USE_LIBINTL)
endif()

# accel
if(NOCT_ENABLE_ACCEL)
  if(WIN32)
    list(
      APPEND
      NOCT_PUBLIC_CPPFLAGS 
      NOCT_ACCEL_BACKEND_DEFINITIONS
      NOCT_ACCEL_BACKEND_DX12
    )
  endif()

  if(APPLE)
    list(
      APPEND
      NOCT_PUBLIC_CPPFLAGS 
      NOCT_ACCEL_BACKEND_DEFINITIONS
      NOCT_ACCEL_BACKEND_METAL
    )
  endif()

  if(   CMAKE_SYSTEM_NAME STREQUAL "Linux"
     OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    list(
      APPEND
      NOCT_PUBLIC_CPPFLAGS 
      NOCT_ACCEL_BACKEND_DEFINITIONS
      NOCT_ACCEL_BACKEND_OPENGLES
    )
  endif()
endif()

# OpenHarmony
if(NOCT_TARGET_OPENHARMONY)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_OPENHARMONY)
endif()

# zedBSD
if(NOCT_TARGET_ZEDBSD)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_MEMORY_SMALL NOCT_FORCE_SOFT_FMAF)
  set(NOCT_ZEDBSD_COMPILE_OPTIONS
    -nostdinc
    -U__linux__
    -U__linux
    -Ulinux
  )
endif()

# Unity Target
if(NOCT_TARGET_UNITY)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_UNITY NOCT_FORCE_SOFT_FMAF)
  if(NOCT_TARGET_UNITY_SWITCH)
    list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_UNITY_SWITCH)
  elseif(NOCT_TARGET_UNITY_PS5)
    list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_UNITY_PS5)
  elseif(NOCT_TARGET_UNITY_XBOX)
    list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_UNITY_XBOXSERIES)
  endif()
endif()

# MacOS 7 Target
if(NOCT_TARGET_MACOS7)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_MACOS7 NOCT_MEMORY_SMALL)
endif()

# DOS4G Target
if(NOCT_TARGET_DOS4G)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_DOS4G)
endif()

# MS-DOS on PC-98 Target
if(NOCT_TARGET_PC98DOS)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_PC98DOS)
endif()

# Common memory profile for DOS targets.
if(NOCT_TARGET_DOS4G OR NOCT_TARGET_PC98DOS)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_MEMORY_SMALL)
endif()

# PC-98 Bootstrap Environment Target
if(NOCT_TARGET_PC98BE)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_PC98BE NOCT_MEMORY_SMALL)
endif()

# _CRT_SECURE_NO_WARNINGS
if(WIN32 AND MSVC)
  list(APPEND NOCT_PUBLIC_CPPFLAGS _CRT_SECURE_NO_WARNINGS)
endif()

# Add compile definitions.
target_compile_definitions(noct PUBLIC ${NOCT_PUBLIC_CPPFLAGS})
target_compile_definitions(noct PRIVATE ${NOCT_PRIVATE_CPPFLAGS})
if(NOCT_ENABLE_API)
  target_compile_definitions(noctapi PUBLIC ${NOCT_PUBLIC_CPPFLAGS})
  target_compile_definitions(noctapi PRIVATE ${NOCT_PRIVATE_CPPFLAGS})
endif()

if(NOCT_TARGET_ZEDBSD)
  target_compile_options(noct PRIVATE ${NOCT_ZEDBSD_COMPILE_OPTIONS})
  if(NOCT_ENABLE_API)
    target_compile_options(noctapi PRIVATE ${NOCT_ZEDBSD_COMPILE_OPTIONS})
  endif()
endif()

if(NOCT_ENABLE_API AND NOCT_ENABLE_API_HTTPSERVER)
  target_compile_definitions(noct PRIVATE NOCT_USE_HTTPSERVER)
  target_compile_definitions(noctapi PRIVATE NOCT_USE_HTTPSERVER)
endif()

if(NOCT_ENABLE_API AND NOCT_ENABLE_API_TERM)
  target_compile_definitions(noct PRIVATE NOCT_USE_TERM)
  target_compile_definitions(noctapi PRIVATE NOCT_USE_TERM)
endif()

if(NOCT_ENABLE_API AND NOCT_ENABLE_API_BEUI)
  target_compile_definitions(noct PRIVATE NOCT_USE_BEUI)
  target_compile_definitions(noctapi PRIVATE NOCT_USE_BEUI)
endif()

if(NOCT_ENABLE_API AND NOCT_ENABLE_API_PROCESS)
  target_compile_definitions(noct PRIVATE NOCT_USE_PROCESS)
  target_compile_definitions(noctapi PRIVATE NOCT_USE_PROCESS)
endif()

# ---

#
# CFLAGS
#

# PIC
if(NOCT_ENABLE_SHARED)
  set_target_properties(noct PROPERTIES POSITION_INDEPENDENT_CODE ON)
  if(NOCT_ENABLE_API)
    set_target_properties(noctapi PROPERTIES POSITION_INDEPENDENT_CODE ON)
  endif()
endif()

# Win32 Unicode settings.
if(WIN32)
  if(MSVC)
    target_compile_options(noct PRIVATE /utf-8)
    if(NOCT_ENABLE_API)
      target_compile_options(noctapi PRIVATE /utf-8)
    endif()
  elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(noct PRIVATE -municode -finput-charset=utf-8 -fexec-charset=utf-8)
    if(NOCT_ENABLE_API)
      target_compile_options(noctapi PRIVATE -municode -finput-charset=utf-8 -fexec-charset=utf-8)
    endif()
  endif()
endif()

# ---

#
# LDFLAGS
#

if(    NOT WIN32
   AND ((NOCT_ENABLE_API
        AND NOCT_ENABLE_API_THREAD
        AND NOCT_ENABLE_MULTITHREAD)
        OR NOCT_ENABLE_ACCEL
        OR NOCT_BUILD_ACCEL_COMMON_TESTS))
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
endif()

if(    NOCT_ENABLE_API
   AND NOCT_ENABLE_API_THREAD
   AND NOCT_ENABLE_MULTITHREAD
   AND NOT WIN32)
  target_link_libraries(noctapi PUBLIC Threads::Threads)
endif()

if(    NOT MSVC
   AND NOT CMAKE_C_COMPILER_ID MATCHES "Watcom"
   AND NOT NOCT_TARGET_ZEDBSD)
  target_link_libraries(noct PUBLIC m)
endif()

if(NOCT_ENABLE_I18N_INTL)
  target_link_libraries(noct PUBLIC ${INTL_LIBRARY})
  if(NOCT_ENABLE_API)
    target_link_libraries(noctapi PUBLIC ${INTL_LIBRARY})
  endif()
endif()

if(NOCT_ENABLE_SHARED)
  if(NOCT_ENABLE_API)
    target_link_libraries(noctapi PUBLIC noct)
  endif()
endif()

# accel
if(NOCT_ENABLE_ACCEL)
  if(WIN32)
    target_link_libraries(
      noct
      PUBLIC
      d3d12
      dxgi
      dxguid
      d3dcompiler
      ole32
    )
  elseif(APPLE)
    find_library(NOCT_FOUNDATION_FRAMEWORK Foundation)
    find_library(NOCT_METAL_FRAMEWORK Metal)
    target_link_libraries(
      noct
      PUBLIC
      ${NOCT_FOUNDATION_FRAMEWORK}
      ${NOCT_METAL_FRAMEWORK}
    )
  elseif(   CMAKE_SYSTEM_NAME STREQUAL "Linux"
         OR CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NOCT_EGL REQUIRED IMPORTED_TARGET egl)
    pkg_check_modules(NOCT_GLESV2 REQUIRED IMPORTED_TARGET glesv2)
    target_link_libraries(
      noct
      PUBLIC
      PkgConfig::NOCT_EGL
      PkgConfig::NOCT_GLESV2
    )
  endif()
endif()

# winsock
if(NOCT_ENABLE_API AND NOCT_ENABLE_API_HTTPSERVER)
  if(WIN32)
    target_link_libraries(noctapi PUBLIC wsock32 ws2_32)
  endif()
endif()

# SDL2
if(    NOCT_ENABLE_API
   AND NOCT_ENABLE_API_BEUI
   AND NOT NOCT_TARGET_ZEDBSD
   AND NOT NOCT_TARGET_PC98DOS)
  target_link_libraries(noctapi PUBLIC ${NOCT_SDL2_TARGET})
endif()

# ---

#
# Export variables.
#

set(NOCT_INCLUDE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(NOCT_INCLUDE_DIRS ${NOCT_INCLUDE_DIR})
set(NOCT_LIBRARY      $<TARGET_FILE:noct>)
if(NOCT_ENABLE_API)
  list(APPEND NOCT_LIBRARY $<TARGET_FILE:noctapi>)
endif()
set(NOCT_LIBRARIES    ${NOCT_LIBRARY})

# ---

#
# Target-neutral accelerator tests
#

if(NOCT_BUILD_ACCEL_COMMON_TESTS)
  add_library(
    noct-accel-test-common
    STATIC
    EXCLUDE_FROM_ALL
    src/accel/accel_ir.c
    src/accel/accel_program.c
    src/accel/accel_residency.c
    src/accel/accel_compile.c
    src/accel/accel_context.c
    src/accel/accel_mutex.c
    src/accel/accel_rewrite.c
    src/accel/accel_lir_budget.c
    src/accel/accel_callback.c
    src/accel/accel_device.c
    src/accel/accel_runtime.c
    src/accel/accel_shader_source.c
  )
  target_compile_definitions(
    noct-accel-test-common
    PRIVATE
    ${NOCT_PUBLIC_CPPFLAGS}
    ${NOCT_PRIVATE_CPPFLAGS}
    NOCT_USE_ACCEL
  )
  target_include_directories(
    noct-accel-test-common
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/noct
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/optimizer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/accel
  )
  target_link_libraries(noct-accel-test-common PRIVATE noct)
  if(NOT WIN32)
    target_link_libraries(noct-accel-test-common PRIVATE Threads::Threads)
  endif()

  add_executable(
    noct-test-accel-device
    EXCLUDE_FROM_ALL
    tests/testcases/accel-device-test.c
  )
  target_compile_definitions(
    noct-test-accel-device
    PRIVATE
    ${NOCT_PUBLIC_CPPFLAGS}
    ${NOCT_PRIVATE_CPPFLAGS}
    NOCT_USE_ACCEL
  )
  target_include_directories(
    noct-test-accel-device
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/noct
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/optimizer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/accel
  )
  target_link_libraries(
    noct-test-accel-device
    PRIVATE
    noct-accel-test-common
    noct
  )

  add_executable(
    noct-test-accel-shader-source
    EXCLUDE_FROM_ALL
    tests/testcases/accel-shader-source-test.c
  )
  target_compile_definitions(
    noct-test-accel-shader-source
    PRIVATE
    ${NOCT_PUBLIC_CPPFLAGS}
    ${NOCT_PRIVATE_CPPFLAGS}
    NOCT_USE_ACCEL
  )
  target_include_directories(
    noct-test-accel-shader-source
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/noct
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/optimizer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/accel
  )
  target_link_libraries(
    noct-test-accel-shader-source
    PRIVATE
    noct-accel-test-common
    noct
  )

  add_executable(
    noct-test-accel-runtime
    EXCLUDE_FROM_ALL
    tests/testcases/accel-runtime-test.c
  )
  target_compile_definitions(
    noct-test-accel-runtime
    PRIVATE
    ${NOCT_PUBLIC_CPPFLAGS}
    ${NOCT_PRIVATE_CPPFLAGS}
    NOCT_USE_ACCEL
  )
  target_include_directories(
    noct-test-accel-runtime
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/noct
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/optimizer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/accel
  )
  target_link_libraries(
    noct-test-accel-runtime
    PRIVATE
    noct-accel-test-common
    noct
  )
  if(NOT WIN32)
    target_link_libraries(noct-test-accel-runtime PRIVATE Threads::Threads)
  endif()

  add_executable(
    noct-test-accel-plan
    EXCLUDE_FROM_ALL
    tests/testcases/accel-plan-test.c
  )
  target_compile_definitions(
    noct-test-accel-plan
    PRIVATE
    ${NOCT_PUBLIC_CPPFLAGS}
    ${NOCT_PRIVATE_CPPFLAGS}
    NOCT_USE_ACCEL
  )
  target_include_directories(
    noct-test-accel-plan
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/noct
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/optimizer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/accel
  )
  target_link_libraries(
    noct-test-accel-plan
    PRIVATE
    noct-accel-test-common
    noct
  )

  add_library(
    noct-accel-test-backend
    STATIC
    EXCLUDE_FROM_ALL
    tests/testcases/accel_test_backend.c
  )
  target_compile_definitions(
    noct-accel-test-backend
    PRIVATE
    ${NOCT_PUBLIC_CPPFLAGS}
    ${NOCT_PRIVATE_CPPFLAGS}
    NOCT_USE_ACCEL
  )
  target_include_directories(
    noct-accel-test-backend
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/noct
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/optimizer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/accel
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/testcases
  )
  target_link_libraries(
    noct-accel-test-backend
    PRIVATE
    noct-accel-test-common
    noct
  )

  add_executable(
    noct-test-accel-rewrite
    EXCLUDE_FROM_ALL
    tests/testcases/accel-rewrite-test.c
  )
  target_compile_definitions(
    noct-test-accel-rewrite
    PRIVATE
    ${NOCT_PUBLIC_CPPFLAGS}
    ${NOCT_PRIVATE_CPPFLAGS}
    NOCT_USE_ACCEL
  )
  target_include_directories(
    noct-test-accel-rewrite
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/noct
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/optimizer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/accel
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/testcases
  )
  target_link_libraries(
    noct-test-accel-rewrite
    PRIVATE
    noct-accel-test-common
    noct-accel-test-backend
    noct
  )
  if(NOT WIN32)
    target_link_libraries(noct-test-accel-rewrite PRIVATE Threads::Threads)
  endif()
endif()

# ---

#
# CLI Target
#

if(NOCT_ENABLE_CLI)
  if(NOCT_ENABLE_BCBACKEND)
    set(
      CLI_BCBACK_SOURCES
      src/cli/cli-compile.c
      src/cli/app_file.c
    )
  endif()
  
  if(NOCT_ENABLE_CBACKEND)
    set(CLI_CBACK_SOURCES src/cli/cli-ctrans.c)
  endif()
  
  if(NOCT_ENABLE_ELBACKEND)
    set(CLI_ELBACK_SOURCES src/cli/cli-eltrans.c)
  endif()

  if(NOCT_ENABLE_SCMBACKEND)
    set(CLI_SCMBACK_SOURCES src/cli/cli-scmtrans.c)
  endif()

  if(NOCT_ENABLE_REPL)
    set(CLI_REPL_SOURCES src/cli/cli-repl.c)
  endif()

  add_executable(
    noctcli
    src/cli/cli-main.c
    src/cli/cli-run.c
    src/cli/cli-cfunc.c
    src/cli/module.c
    src/cli/bytecode_file.c
    ${CLI_BCBACK_SOURCES}
    ${CLI_CBACK_SOURCES}
    ${CLI_ELBACK_SOURCES}
    ${CLI_SCMBACK_SOURCES}
    ${CLI_REPL_SOURCES}
  )

  if(WIN32)
    target_sources(noctcli PRIVATE src/cli/cli-win32.c)
  endif()

  # Set the executable name.
  set_target_properties(noctcli PROPERTIES OUTPUT_NAME "noct")

  # Define feature macros.
  target_compile_definitions(
    noctcli
    PRIVATE
    ${NOCT_PUBLIC_CPPFLAGS}
    ${NOCT_PRIVATE_CPPFLAGS}
  )

  if(NOCT_ENABLE_CBACKEND)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_CBACKEND)
  endif()

  if(NOCT_ENABLE_BCBACKEND)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_BCBACKEND)
  endif()

  if(NOCT_ENABLE_ELBACKEND)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_ELBACKEND)
  endif()

  if(NOCT_ENABLE_SCMBACKEND)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_SCMBACKEND)
  endif()

  if(NOCT_ENABLE_REPL)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_REPL)
  endif()

  if(NOCT_ENABLE_API AND NOCT_ENABLE_API_HTTPSERVER)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_HTTPSERVER)
  endif()

  if(NOCT_ENABLE_API AND NOCT_ENABLE_API_TERM)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_TERM)
  endif()

  if(NOCT_ENABLE_API AND NOCT_ENABLE_API_BEUI)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_BEUI)
  endif()

  if(NOCT_ENABLE_API AND NOCT_ENABLE_API_PROCESS)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_PROCESS)
  endif()

  if(NOT NOCT_VERSION)
    set(NOCT_VERSION "2.0-current")
  endif()

  target_compile_definitions(noctcli PRIVATE NOCT_VERSION="${NOCT_VERSION}")

  # Include include/noct/, src/core/, and src/backend/
  target_include_directories(noctcli PRIVATE
     ${NOCT_INCLUDE_DIRS}
     ${CMAKE_CURRENT_SOURCE_DIR}/include
     ${CMAKE_CURRENT_SOURCE_DIR}/src/core
     ${CMAKE_CURRENT_SOURCE_DIR}/src/backend
     ${CMAKE_CURRENT_SOURCE_DIR}/src/cli
  )

  if(NOCT_ENABLE_OPTIMIZER)
    target_include_directories(noctcli PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src/optimizer
    )
  endif()

  if(NOCT_TARGET_ZEDBSD)
    target_compile_options(noctcli PRIVATE ${NOCT_ZEDBSD_COMPILE_OPTIONS})
  endif()

  # Link libnoct and libnoctapi.
  target_link_libraries(noctcli PRIVATE noct noctapi)

  # Win32
  if(WIN32)
    target_link_libraries(noctcli PRIVATE shell32)
  endif()

  # Add -lm on UNIX.
  if(UNIX AND NOT NOCT_TARGET_ZEDBSD)
    target_link_libraries(noctcli PRIVATE m)
  endif()

  # Add -lutil on Linux.
  if(LINUX)
    target_link_libraries(noctcli PRIVATE util)
  endif()

  # Win32 Unicode settings.
  if(WIN32)
    if(MSVC)
      target_compile_options(noctcli PRIVATE /utf-8)
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
      target_compile_options(noctcli PRIVATE -finput-charset=utf-8 -fexec-charset=utf-8)
    endif()
  endif()

  # accel
  if(NOCT_ENABLE_ACCEL)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_ACCEL)

    target_include_directories(
      noctcli
      PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src/accel
    )

    if(APPLE)
      set_target_properties(noctcli PROPERTIES LINKER_LANGUAGE OBJC)
    endif()
  endif()
endif()

if(WIN32 AND NOCT_ENABLE_CLI)
  add_executable(
    noct-test-cli-win32-gpu-name
    EXCLUDE_FROM_ALL
    src/cli/cli-win32.c
    tests/testcases/cli-win32-gpu-name-test.c
  )
  target_compile_definitions(
    noct-test-cli-win32-gpu-name
    PRIVATE
    ${NOCT_PUBLIC_CPPFLAGS}
    ${NOCT_PRIVATE_CPPFLAGS}
  )
  target_include_directories(
    noct-test-cli-win32-gpu-name
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/noct
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cli
  )
  target_link_libraries(noct-test-cli-win32-gpu-name PRIVATE shell32)
  set_target_properties(
    noct-test-cli-win32-gpu-name
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-bin"
  )
  foreach(NOCT_TEST_CONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${NOCT_TEST_CONFIG}" NOCT_TEST_CONFIG_UPPER)
    set_target_properties(
      noct-test-cli-win32-gpu-name
      PROPERTIES
      "RUNTIME_OUTPUT_DIRECTORY_${NOCT_TEST_CONFIG_UPPER}"
      "${CMAKE_CURRENT_BINARY_DIR}/test-bin"
    )
  endforeach()
endif()

if(NOCT_LIB_TYPE STREQUAL "STATIC" AND NOT CMAKE_CROSSCOMPILING)
  add_executable(
    noct-test-accel-hint-cpu
    EXCLUDE_FROM_ALL
    tests/testcases/accel-hint-cpu-test.c
  )
  target_compile_definitions(
    noct-test-accel-hint-cpu
    PRIVATE
    ${NOCT_PUBLIC_CPPFLAGS}
    ${NOCT_PRIVATE_CPPFLAGS}
  )
  target_include_directories(
    noct-test-accel-hint-cpu
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/noct
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
  )
  target_link_libraries(noct-test-accel-hint-cpu PRIVATE noct)

  add_executable(
    noct-test-optimizer-callback
    EXCLUDE_FROM_ALL
    tests/testcases/optimizer-callback-test.c
  )
  target_compile_definitions(
    noct-test-optimizer-callback
    PRIVATE
    ${NOCT_PUBLIC_CPPFLAGS}
    ${NOCT_PRIVATE_CPPFLAGS}
  )
  if(NOCT_ENABLE_ACCEL OR NOCT_BUILD_ACCEL_COMMON_TESTS)
    target_compile_definitions(
      noct-test-optimizer-callback
      PRIVATE
      NOCT_USE_ACCEL
    )
  endif()
  target_include_directories(
    noct-test-optimizer-callback
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/noct
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/optimizer
  )
  target_link_libraries(noct-test-optimizer-callback PRIVATE noct)
endif()

# ---

#
# Install
#

if(NOCT_ENABLE_INSTALL)
  # Check for BSD.
  if(   CMAKE_SYSTEM_NAME STREQUAL "FreeBSD"
     OR CMAKE_SYSTEM_NAME STREQUAL "NetBSD"
     OR CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    set(NOCT_ENABLE_MDOC ON)
  endif()

  # Headers
  install(
    FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/include/noct/noct.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/include/noct/c89compat.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/include/noct/executor.h"
    DESTINATION
      "${CMAKE_INSTALL_INCLUDEDIR}/noct"
  )
  # libnoct
  install(
    TARGETS noct RUNTIME
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  )

  # libnoctapi
  if(NOCT_ENABLE_API)
    install(
      TARGETS noctapi RUNTIME
      DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
  endif()

  # noct
  if(NOCT_ENABLE_CLI)
    install(
      TARGETS noctcli RUNTIME
      DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )

    # man noct.1
    if(NOT NOCT_ENABLE_MDOC)
      set(NOCT_MANPAGE "${CMAKE_SOURCE_DIR}/resources/man/noct.1.man")
    else()
      set(NOCT_MANPAGE "${CMAKE_SOURCE_DIR}/resources/man/noct.1.mdoc")
    endif()
    install(
      FILES "${NOCT_MANPAGE}"
      DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
      RENAME noct.1
    )
  endif()

  # locales
  if(NOCT_ENABLE_I18N_LIBINTL)
    install(
      DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/locale/"
      DESTINATION "${CMAKE_INSTALL_FULL_LOCALEDIR}"
    )
  endif()

  # CMake module
  install(
    FILES "${CMAKE_SOURCE_DIR}/cmake/modules/FindNoct.cmake"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/cmake/Modules"
  )

  # pkgconfig pc
  configure_file(
    "${CMAKE_SOURCE_DIR}/resources/pkg-config/noct.pc.in"
    noct.pc
    @ONLY
  )
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/noct.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
  )
  if(NOCT_ENABLE_API)
    configure_file(
      "${CMAKE_SOURCE_DIR}/resources/pkg-config/noctapi.pc.in"
      noctapi.pc
      @ONLY
    )
    install(
      FILES "${CMAKE_CURRENT_BINARY_DIR}/noctapi.pc"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
    )
  endif()

  # Uninstall
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
  )
  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  )
endif()
