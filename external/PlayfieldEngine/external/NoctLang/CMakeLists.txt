cmake_minimum_required(VERSION 3.16)
project(Noct VERSION 1.0 LANGUAGES C)

#
# Build Configuration
#

# Choose one.
option(NOCT_ENABLE_STATIC       "Build a static library"      OFF)
option(NOCT_ENABLE_SHARED       "Build a shared library"      OFF)
option(NOCT_ENABLE_OBJECT       "Build objects only"          OFF)

# Options
option(NOCT_ENABLE_CLI          "Build a CLI binary"          OFF)
option(NOCT_ENABLE_JIT          "Enable JIT compilation"      OFF)
option(NOCT_ENABLE_I18N         "Enable translation feature"  OFF)
option(NOCT_ENABLE_I18N_LIBINTL "Enable gettext"              OFF)
option(NOCT_ENABLE_API          "Enable standard API"         OFF)
option(NOCT_ENABLE_INSTALL      "Enable installation"         OFF)
option(NOCT_ENABLE_BCBACKEND    "Enable bytecode backend"     OFF)
option(NOCT_ENABLE_CBACKEND     "Enable C backend"            OFF)
option(NOCT_ENABLE_ELBACKEND    "Enable Emacs Lisp backend"   OFF)
option(NOCT_ENABLE_SCMBACKEND   "Enable Scheme backend"       OFF)
option(NOCT_ENABLE_REPL         "Enable REPL"                 OFF)
option(NOCT_ENABLE_MDOC         "Install mdoc(7) man page"    OFF)

# Choose one if needed.
# (These targets cannot be automatically determined.)
option(NOCT_TARGET_UNITY        "Build for Unity"             OFF)
option(NOCT_TARGET_OPENHARMONY  "Build for OpenHarmony"       OFF)
option(NOCT_TARGET_DOS4G        "Build for Unity"             OFF)

# Experimental.
option(NOCT_ENABLE_MULTITHREAD  "Enable multithread support"  OFF)

# ---

#
# Debug/Release
#

# Debug Configuration
if(MSVC)
  # MSVC
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /Zi /DDEBUG /UNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /DDEBUG /UNDEBUG")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  # GCC/Clang
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g3 -DDEBUG -Wall -Werror -Wextra -Wundef -Wconversion")
  set(CMAKE_OBJC_FLAGS_DEBUG "${CMAKE_OBJC_FLAGS_DEBUG} -O0 -g3 -DNDEBUG -Wall -Werror -Wextra -Wundef -Wconversion")
endif()

# Release Configuration
if(MSVC)
  # MSVC
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  # GCC/Clang
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -g -DNDEBUG")
  set(CMAKE_OBJC_FLAGS_RELEASE "${CMAKE_OBJC_FLAGS_RELEASE} -O2 -g -DNDEBUG")
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
  src/core/lexer.yy.c
  src/core/parser.tab.c
  ${FLEX_Lexer_OUTPUTS}
  ${BISON_Parser_OUTPUT_SOURCE}
)

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
  ${NOCT_CBE_SOURCE}
  ${NOCT_BCBE_SOURCE}
  ${NOCT_ELBE_SOURCE}
  ${NOCT_SCMBE_SOURCE}
  ${NOCT_I18N_SOURCE}

  # Mandatory libraries here.
  src/api/api-math.c
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

# -Iinclude
target_include_directories(noct PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(NOCT_ENABLE_API)
  target_include_directories(noctapi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

# -Isrc/core
target_include_directories(noct PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
if(NOCT_ENABLE_API)
  target_include_directories(noctapi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
endif()

# NOCT_USE_DLL -DNOCT_DLL_IMPL
if(NOCT_ENABLE_SHARED)
  target_compile_definitions(noct PRIVATE NOCT_USE_DLL NOCT_DLL_IMPL)
  if(NOCT_ENABLE_API)
    target_compile_definitions(noctapi PRIVATE NOCT_USE_DLL NOCT_DLL_IMPL)
  endif()
endif()

# NOCT_USE_JIT
if(    NOCT_ENABLE_JIT AND
   NOT CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
  target_compile_definitions(noct PRIVATE NOCT_USE_JIT)
  if(NOCT_ENABLE_API)
    target_compile_definitions(noctapi PRIVATE NOCT_USE_JIT)
  endif()
endif()

# NOCT_USE_MULTITHREAD
if(NOCT_ENABLE_MULTITHREAD)
  target_compile_definitions(noct PRIVATE NOCT_USE_MULTITHREAD)
  if(NOCT_ENABLE_API)
    target_compile_definitions(noctapi PRIVATE NOCT_USE_MULTITHREAD)
  endif()
endif()

# NOCT_USE_TRANSLATION
if(NOCT_ENABLE_I18N)
  target_compile_definitions(noct PRIVATE NOCT_USE_TRANSLATION)
  if(NOCT_ENABLE_API)
    target_compile_definitions(noctapi PRIVATE NOCT_USE_TRANSLATION)
  endif()
endif()  

# NOCT_USE_LIBINTL
if(NOCT_ENABLE_I18N_LIBINTL)
  target_compile_definitions(noct PRIVATE NOCT_USE_LIBINTL)
  if(NOCT_ENABLE_API)
    target_compile_definitions(noctapi PRIVATE NOCT_USE_LIBINTL)
  endif()
endif()  

# NOCT_TARGET_OPENHARMONY
if(NOCT_TARGET_OPENHARMONY)
  target_compile_definitions(noct PUBLIC NOCT_TARGET_OPENHARMONY)
  if(NOCT_ENABLE_API)
    target_compile_definitions(noctapi PUBLIC NOCT_TARGET_OPENHARMONY)
  endif()
endif()

# NOCT_TARGET_UNITY
if(NOCT_TARGET_UNITY)
  target_compile_definitions(noct PUBLIC NOCT_TARGET_UNITY)
  if(NOCT_ENABLE_API)
    target_compile_definitions(noctapi PUBLIC NOCT_TARGET_UNITY)
  endif()

  if(NOCT_TARGET_UNITY_SWITCH)
    target_compile_definitions(noct PRIVATE NOCT_UNITY_SWITCH)
    if(NOCT_ENABLE_API)
      target_compile_definitions(noctapi PUBLIC NOCT_UNITY_SWITCH)
    endif()
  elseif(NOCT_TARGET_UNITY_PS5)
    target_compile_definitions(noct PRIVATE NOCT_UNITY_PS5)
    if(NOCT_ENABLE_API)
      target_compile_definitions(noctapi PUBLIC NOCT_UNITY_PS5)
    endif()
  elseif(NOCT_TARGET_UNITY_XBOX)
    target_compile_definitions(noct PRIVATE NOCT_UNITY_GAMECORE_XBOXSERIES)
    if(NOCT_ENABLE_API)
      target_compile_definitions(noctapi PUBLIC NOCT_UNITY_GAMECORE_XBOXSERIES)
    endif()
  endif()
endif()

# NOCT_TARGET_DOS4G
if(NOCT_TARGET_DOS4G)
  target_compile_definitions(noct PUBLIC NOCT_TARGET_DOS4G)
  if(NOCT_ENABLE_API)
    target_compile_definitions(noctapi PUBLIC NOCT_TARGET_DOS4G)
  endif()
endif()

# _CRT_SECURE_NO_WARNINGS
if(WIN32 AND MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# ---

#
# CFLAGS
#

# -fPIC
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
