cmake_minimum_required(VERSION 3.16)
project(Noct VERSION 1.0 LANGUAGES C)

#
# Build Configuration
#

# Choose one.
option(NOCT_ENABLE_STATIC       "Build a static library"           OFF)
option(NOCT_ENABLE_SHARED       "Build a shared library"           OFF)
option(NOCT_ENABLE_OBJECT       "Build objects only"               OFF)

# Options
option(NOCT_ENABLE_CLI          "Enable CLI"                       OFF)
option(NOCT_ENABLE_JIT          "Enable JIT compilation"           OFF)
option(NOCT_ENABLE_MULTITHREAD  "Enable multithread support"       OFF)
option(NOCT_ENABLE_I18N         "Enable translation (built-in)"    OFF)
option(NOCT_ENABLE_I18N_LIBINTL "Enable translation (gettext)"     OFF)
option(NOCT_ENABLE_API          "Enable standard API"              OFF)
option(NOCT_ENABLE_INSTALL      "Enable installation"              OFF)
option(NOCT_ENABLE_BCBACKEND    "Enable bytecode backend"          OFF)
option(NOCT_ENABLE_CBACKEND     "Enable C backend"                 OFF)
option(NOCT_ENABLE_ELBACKEND    "Enable Emacs Lisp backend"        OFF)
option(NOCT_ENABLE_SCMBACKEND   "Enable Scheme backend"            OFF)
option(NOCT_ENABLE_REPL         "Enable REPL"                      OFF)
option(NOCT_ENABLE_MDOC         "Install mdoc(7) man page"         OFF)

#
# Choose one if needed.
#  - These targets cannot be automatically determined.
#
option(NOCT_TARGET_UNITY        "Build for Unity"                  OFF)
option(NOCT_TARGET_OPENHARMONY  "Build for OpenHarmony"            OFF)
option(NOCT_TARGET_DOS4G        "Build for Unity"                  OFF)

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

# Determine the base files.
set(
  NOCT_BASE_SOURCE
  src/core/lexer.yy.c      # Lexer
  src/core/parser.tab.c    # Parser
  src/core/ast.c           # AST construction (called from parser)
  src/core/hir.c           # HIR (AST->HIR construction)
  src/core/lir.c           # LIR (HIR->LIR construction)
  src/core/noct.c          # Public functions
  src/core/runtime.c       # Runtime main part
  src/core/interpreter.c   # Runtime interpreter part
  src/core/jit.c           # Runtime JIT part
  src/core/execution.c     # Runtime execution helper part
  src/core/gc.c            # Runtime GC part
  src/core/intrinsics.c    # Intrinsics
  ${FLEX_Lexer_OUTPUTS}
  ${BISON_Parser_OUTPUT_SOURCE}
)

# Determine the object model source file.
if(NOCT_ENABLE_MULTITHREAD)
  set(
    NOCT_OBJMODEL_SOURCE
    src/core/objectmodel-mt.c  # Multi-threaded Object Model
  )
else()
  set(
    NOCT_OBJMODEL_SOURCE
    src/core/objectmodel-st.c  # Single-threaded Object Model
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

# Add the target "libnoct", the core sandboxed virtual machine.
add_library(
  noct
  ${NOCT_LIB_TYPE}

  ${NOCT_BASE_SOURCE}
  ${NOCT_OBJMODEL_SOURCE}
  ${NOCT_CBE_SOURCE}
  ${NOCT_BCBE_SOURCE}
  ${NOCT_ELBE_SOURCE}
  ${NOCT_SCMBE_SOURCE}
  ${NOCT_I18N_SOURCE}
)
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

# Add the target "libnoctapi", the standard library.
if(NOCT_ENABLE_API)
  add_library(
    noctapi
    ${NOCT_LIB_TYPE}

    # Optional libraries here.
    src/api/api-system.c
    src/api/api-console.c
    src/api/api-file.c
  )
  if(NOCT_ENABLE_SHARED)
    set_target_properties(noctapi PROPERTIES 
      VERSION 1.0.0 
      SOVERSION 1
    )
    set_target_properties(noct PROPERTIES
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
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)
if(NOCT_ENABLE_API)
  target_include_directories(noctapi PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
  )
endif()

# Multithread Support
if(NOCT_ENABLE_MULTITHREAD)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_USE_MULTITHREAD)
endif()

# If building a shared library.
if(NOCT_ENABLE_SHARED)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_USE_DLL)
endif()

# JIT Support
if(NOCT_ENABLE_JIT)
  # Exclude OpenBSD
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_USE_JIT)
  endif()
endif()

# Translation
if(NOCT_ENABLE_I18N)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_USE_TRANSLATION)
endif()  

# Translation (gettext)
if(NOCT_ENABLE_I18N_LIBINTL)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_USE_LIBINTL)
endif()

# OpenHarmony Target
if(NOCT_TARGET_OPENHARMONY)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_OPENHARMONY)
endif()

# Unity Target
if(NOCT_TARGET_UNITY)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_UNITY)
  if(NOCT_TARGET_UNITY_SWITCH)
    list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_UNITY_SWITCH)
  elseif(NOCT_TARGET_UNITY_PS5)
    list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_UNITY_PS5)
  elseif(NOCT_TARGET_UNITY_XBOX)
    list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_UNITY_XBOXSERIES)
  endif()
endif()

# DOS4G Target
if(NOCT_TARGET_DOS4G)
  list(APPEND NOCT_PUBLIC_CPPFLAGS NOCT_TARGET_DOS4G)
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
  else()
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

if(NOT MSVC)
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

# ---

#
# Export variables.
#

set(NOCT_INCLUDE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(NOCT_INCLUDE_DIRS ${NOCT_INCLUDE_DIR})
set(NOCT_LIBRARY      $<TARGET_FILE:noct> $<TARGET_FILE:noctapi>)
set(NOCT_LIBRARIES    ${NOCT_LIBRARY})

# ---

#
# CLI Target
#

if(NOCT_ENABLE_CLI)
  if(NOCT_ENABLE_BCBACKEND)
    set(CLI_BCBACK_SOURCES src/cli/cli-compile.c)
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
    ${CLI_BCBACK_SOURCES}
    ${CLI_CBACK_SOURCES}
    ${CLI_ELBACK_SOURCES}
    ${CLI_SCMBACK_SOURCES}
    ${CLI_REPL_SOURCES}
  )

  # Set the executable name.
  set_target_properties(noctcli PROPERTIES OUTPUT_NAME "noct")

  # Define feature macros.
  if(NOCT_ENABLE_SHARED)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_DLL)
  endif()
  if(NOCT_ENABLE_JIT)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_JIT)
  endif()
  if(NOCT_ENABLE_I18N)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_TRANSLATION)
  endif()  
  if(NOCT_ENABLE_I18N_LIBINTL)
    target_compile_definitions(noctcli PRIVATE NOCT_USE_LIBINTL)
  endif()
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
  if(NOT NOCT_VERSION)
    set(NOCT_VERSION "1.0-current")
  endif()
  target_compile_definitions(noctcli PRIVATE NOCT_VERSION="${NOCT_VERSION}")

  # Include include/noct/, src/core/, and src/backend/
  target_include_directories(noctcli PRIVATE
     ${NOCT_INCLUDE_DIRS}
     ${CMAKE_CURRENT_SOURCE_DIR}/include
     ${CMAKE_CURRENT_SOURCE_DIR}/src/core
     ${CMAKE_CURRENT_SOURCE_DIR}/src/backend
  )

  # Link libnoct and libnoctapi.
  target_link_libraries(noctcli PRIVATE noct noctapi)

  # Add -m on UNIX.
  if(UNIX)
    target_link_libraries(noctcli PRIVATE m)
  endif()

  # Win32 Unicode settings.
  if(WIN32)
    if(MSVC)
      target_compile_options(noctcli PRIVATE /utf-8)
    else()
      target_compile_options(noctcli PRIVATE -municode -finput-charset=utf-8 -fexec-charset=utf-8)
    endif()
  endif()
endif()

# ---

#
# Install
#

if(NOCT_ENABLE_INSTALL)
  include(GNUInstallDirs)

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
      "${CMAKE_CURRENT_SOURCE_DIR}/include/noct/aot.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/include/noct/backend.h"
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
