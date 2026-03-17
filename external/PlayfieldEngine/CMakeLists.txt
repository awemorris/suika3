cmake_minimum_required(VERSION 3.13)
project(playfield LANGUAGES C CXX)

#
# Configuration
#

# Targets
option(PLAYFIELD_TARGET_WINDOWS          "Build for Windows XP or later"              OFF)
option(PLAYFIELD_TARGET_WINDOWS95        "Build for Windows 95/98/Me/2000"            OFF)
option(PLAYFIELD_TARGET_MACOS            "Build for macOS (app)"                      OFF)
option(PLAYFIELD_TARGET_MACOS_CLI        "Build for macOS (CLI)"                      OFF)
option(PLAYFIELD_TARGET_MACOS_LIB        "Build for macOS (library)"                  OFF)
option(PLAYFIELD_TARGET_LINUX            "Build for Linux (X11, OpenGL)"              OFF)
option(PLAYFIELD_TARGET_LINUX_WAYLAND    "Build for Linux (Wayland, OpenGL)"          OFF)
option(PLAYFIELD_TARGET_LINUX_GBM        "Build for Linux (GBM, OpenGL ES)"           OFF)
option(PLAYFIELD_TARGET_LINUX_FBDEV      "Build for Linux (fbdev, Soft3D)"            OFF)
option(PLAYFIELD_TARGET_LINUX_X11SOFT    "Build for Linux (X11, Soft3D)"              OFF)
option(PLAYFIELD_TARGET_IOS              "Build for iOS"                              OFF)
option(PLAYFIELD_TARGET_ANDROID          "Build for Android (OpenGL ES, OpenSL ES)"   OFF)
option(PLAYFIELD_TARGET_OPENHARMONY      "Build for OpenHarmony (OpenGL ES)"          OFF)
option(PLAYFIELD_TARGET_WASM             "Build for Emscripten (WebGL)"               OFF)
option(PLAYFIELD_TARGET_WASM_LOCAL       "Build for Emscripten Local (WebGL)"         OFF)
option(PLAYFIELD_TARGET_UNITY            "Build for Unity"                            OFF)
option(PLAYFIELD_TARGET_FREEBSD          "Build for FreeBSD (X11, OpenGL)"            OFF)
option(PLAYFIELD_TARGET_FREEBSD_WAYLAND  "Build for FreeBSD (Wayland, OpenGL)"        OFF)
option(PLAYFIELD_TARGET_FREEBSD_X11SOFT  "Build for FreeBSD (X11, Soft3D)"            OFF)
option(PLAYFIELD_TARGET_NETBSD           "Build for NetBSD (X11, OpenGL)"             OFF)
option(PLAYFIELD_TARGET_NETBSD_X11SOFT   "Build for NetBSD (X11, Soft3D)"             OFF)
option(PLAYFIELD_TARGET_OPENBSD          "Build for OpenBSD (X11, OpenGL)"            OFF)
option(PLAYFIELD_TARGET_OPENBSD_X11SOFT  "Build for OpenBSD (X11, Soft3D)"            OFF)
option(PLAYFIELD_TARGET_SOLARIS11        "Build for Solaris 11 (X11, Soft3D)"         OFF)
option(PLAYFIELD_TARGET_GENERICUNIX      "Build for Unknown UNIX (X11, Soft3D)"       OFF)
option(PLAYFIELD_TARGET_GDK_WINDOWS      "Build for GDK for Desktop"                  OFF)
option(PLAYFIELD_TARGET_GDK_XBOX_XS      "Build for GDK for Xbox Series X|S"          OFF)

# Options
option(PLAYFIELD_ENABLE_STATIC           "Build a static library"                     OFF)
option(PLAYFIELD_ENABLE_SHARED           "Build a dynamic library"                    OFF)
option(PLAYFIELD_ENABLE_OBJECT           "Build an object library"                    OFF)
option(PLAYFIELD_ENABLE_DIST             "Link shared libraries"                      OFF)
option(PLAYFIELD_ENABLE_JIT              "Enable JIT"                                 OFF)
option(PLAYFIELD_ENABLE_I18N             "Enable translation"                         OFF)
option(PLAYFIELD_ENABLE_PACK             "Enable packager"                            OFF)
option(PLAYFIELD_ENABLE_BYTECOMP         "Enable bytecode compiler"                   OFF)
option(PLAYFIELD_ENABLE_AOTCOMP          "Enable AOT compiler"                        OFF)
option(PLAYFIELD_ENABLE_WEBSERVER        "Enable Web server"                          OFF)
option(PLAYFIELD_ENABLE_EDITOR           "Enable simple editor for Windows"           OFF)
option(PLAYFIELD_ENABLE_ROT90            "Rotate screen for GBM"                      OFF)
option(PLAYFIELD_ENABLE_GST              "Enable Gstreamer support"                   OFF)

# For derived projects
option(PLAYFIELD_ENABLE_INITHOOK         "Enable the inithook function"               OFF)

# ---

#
# Automatic Target Detection
#

# For when a target is manually specified.
if(   PLAYFIELD_TARGET_WINDOWS
   OR PLAYFIELD_TARGET_WINDOWS95
   OR PLAYFIELD_TARGET_MACOS
   OR PLAYFIELD_TARGET_MACOS_CLI
   OR PLAYFIELD_TARGET_MACOS_LIB
   OR PLAYFIELD_TARGET_LINUX
   OR PLAYFIELD_TARGET_LINUX_WAYLAND
   OR PLAYFIELD_TARGET_LINUX_GBM
   OR PLAYFIELD_TARGET_LINUX_FBDEV
   OR PLAYFIELD_TARGET_LINUX_X11SOFT
   OR PLAYFIELD_TARGET_IOS
   OR PLAYFIELD_TARGET_ANDROID
   OR PLAYFIELD_TARGET_OPENHARMONY
   OR PLAYFIELD_TARGET_WASM
   OR PLAYFIELD_TARGET_WASM_LOCAL
   OR PLAYFIELD_TARGET_UNITY
   OR PLAYFIELD_TARGET_FREEBSD
   OR PLAYFIELD_TARGET_FREEBSD_WAYLAND
   OR PLAYFIELD_TARGET_FREEBSD_X11SOFT
   OR PLAYFIELD_TARGET_NETBSD
   OR PLAYFIELD_TARGET_NETBSD_X11SOFT
   OR PLAYFIELD_TARGET_OPENBSD_X11SOFT
   OR PLAYFIELD_TARGET_SOLARIS11
   OR PLAYFIELD_TARGET_GENERICUNIX
   OR PLAYFIELD_TARGET_GDK_WINDOWS
   OR PLAYFIELD_TARGET_GDK_XBOX_XS
   OR PLAYFIELD_TARGET_UWP
)
  # A target and options are manually specified.
else()   
  if(WIN32)
    set(PLAYFIELD_TARGET_WINDOWS                  ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)
    set(PLAYFIELD_ENABLE_WEBSERVER                ON)

  elseif(APPLE AND NOT IOS)
    set(PLAYFIELD_TARGET_MACOS_CLI                ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_BUNDLE                   ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLAYFIELD_TARGET_LINUX                    ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(PLAYFIELD_TARGET_FREEBSD_X11SOFT          ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    set(PLAYFIELD_TARGET_NETBSD_X11SOFT           ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    set(PLAYFIELD_TARGET_OPENBSD_X11SOFT          ON)
    set(PLAYFIELD_ENABLE_JIT                      OFF)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "5.11")
    set(PLAYFIELD_TARGET_SOLARIS11                ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    set(PLAYFIELD_TARGET_HAIKU                    ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  else()
    set(STRATO_TARGET_GENERICUNIX                 ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  endif()

  # No automatic detection for:
  #  - iOS
  #  - Android
  #  - OpenHarmony
  #  - Wasm (Emscripten)
  #  - Unity
  #  - GDK (Windows or Xbox)
  #  - UWP
endif()

# ---

#
# NoctLang Configuration Propagation
#

# Use Math.* API.
set(NOCTLANG_API_MATH     ON)
set(NOCTLANG_API_SYSTEM   OFF)
set(NOCTLANG_API_CONSOLE  OFF)

# If building with JIT.
if(PLAYFIELD_ENABLE_JIT)
  set(NOCTLANG_ENABLE_JIT ON)
else()
  set(NOCTLANG_ENABLE_JIT OFF)
endif()

# If building with translation.
if(PLAYFIELD_ENABLE_I18N)
  set(NOCTLANG_ENABLE_I18N ON)
endif()

# If building for Emscripten.
if(   PLAYFIELD_TARGET_WASM
   OR PLAYFIELD_TARGET_WASM_LOCAL)
  set(NOCTLANG_TARGET_WASM ON)
endif()

# If building for Unity.
if(PLAYFIELD_TARGET_UNITY)
  set(NOCTLANG_TARGET_UNITY ON)
endif()

# No `noct` CLI.
if(NOT NOCTLANG_ENABLE_CLI)
  set(NOCTLANG_ENABLE_CLI OFF)
endif()

# No install.
set(NOCTLANG_DISABLE_INSTALL ON)

# Magic:
#  merge object files that consists in libnoctlang.a into libplayfield.a.
if(   PLAYFIELD_ENABLE_STATIC
   OR PLAYFIELD_ENABLE_SHARED
   OR PLAYFIELD_ENABLE_OBJECT)
  # For library generation.
  set(NOCTLANG_ENABLE_OBJECT ON)
else()
  if(PLAYFIELD_ENABLE_DIST)
    # For dynamically linked binaries.
    set(NOCTLANG_ENABLE_STATIC ON)  # This will be changed to SHARED after NoctLang is packaged by OS.
  else()
    # For statically linked binaries.
    set(NOCTLANG_ENABLE_STATIC ON)
  endif()
endif()

# ---

#
# Strato Configuration Propagation
#

# Set the target and its specific options.
if(PLAYFIELD_TARGET_WINDOWS)
  set(STRATO_TARGET_WINDOWS ON)
elseif(PLAYFIELD_TARGET_WINDOWS95)
  set(STRATO_TARGET_WINDOWS95 ON)
elseif(PLAYFIELD_TARGET_MACOS)
  set(STRATO_TARGET_MACOS ON)
elseif(PLAYFIELD_TARGET_MACOS_CLI)
  set(STRATO_TARGET_MACOS ON)
elseif(PLAYFIELD_TARGET_MACOS_LIB)
  set(STRATO_TARGET_MACOS ON)
elseif(PLAYFIELD_TARGET_LINUX)
  set(STRATO_TARGET_LINUX ON)
elseif(PLAYFIELD_TARGET_LINUX_WAYLAND)
  set(STRATO_TARGET_LINUX_WAYLAND ON)
elseif(PLAYFIELD_TARGET_LINUX_GBM)
  set(STRATO_TARGET_LINUX_GBM ON)
elseif(PLAYFIELD_TARGET_LINUX_FBDEV)
  set(STRATO_TARGET_LINUX_FBDEV ON)
elseif(PLAYFIELD_TARGET_LINUX_X11SOFT)
  set(STRATO_TARGET_LINUX_X11SOFT ON)
elseif(PLAYFIELD_TARGET_IOS)
  set(STRATO_TARGET_IOS ON)
elseif(PLAYFIELD_TARGET_ANDROID)
  set(STRATO_TARGET_ANDROID ON)
elseif(PLAYFIELD_TARGET_OPENHARMONY)
  set(STRATO_TARGET_OPENHARMONY ON)
elseif(PLAYFIELD_TARGET_WASM)
  set(STRATO_TARGET_WASM ON)
elseif(PLAYFIELD_TARGET_WASM_LOCAL)
  set(STRATO_TARGET_WASM_LOCAL ON)
elseif(PLAYFIELD_TARGET_UNITY)
  set(STRATO_TARGET_UNITY ON)
elseif(PLAYFIELD_TARGET_FREEBSD)
  set(STRATO_TARGET_FREEBSD ON)
elseif(PLAYFIELD_TARGET_FREEBSD_WAYLAND)
  set(STRATO_TARGET_FREEBSD_WAYLAND ON)
elseif(PLAYFIELD_TARGET_FREEBSD_X11SOFT)
  set(STRATO_TARGET_FREEBSD_X11SOFT ON)
elseif(PLAYFIELD_TARGET_NETBSD)
  set(STRATO_TARGET_NETBSD ON)
elseif(PLAYFIELD_TARGET_NETBSD_X11SOFT)
  set(STRATO_TARGET_NETBSD_X11SOFT ON)
elseif(PLAYFIELD_TARGET_OPENBSD)
  set(STRATO_TARGET_OPENBSD ON)
elseif(PLAYFIELD_TARGET_OPENBSD_X11SOFT)
  set(STRATO_TARGET_OPENBSD_X11SOFT ON)
elseif(PLAYFIELD_TARGET_SOLARIS11)
  set(STRATO_TARGET_SOLARIS11 ON)
elseif(PLAYFIELD_TARGET_GENERICUNIX)
  set(STRATO_TARGET_GENERICUNIX ON)
elseif(PLAYFIELD_TARGET_HAIKU)
  set(STRATO_TARGET_HAIKU ON)
elseif(PLAYFIELD_TARGET_GDK_WINDOWS)
  set(STRATO_TARGET_GDK_WINDOWS ON)
elseif(PLAYFIELD_TARGET_GDK_XBOX_XS)
  set(STRATO_TARGET_GDK_XBOX_XS ON)
elseif(PLAYFIELD_TARGET_UWP)
  set(STRATO_TARGET_UWP ON)
endif()

if(PLAYFIELD_TARGET_UNITY_SWITCH)
  set(STRATO_TARGET_UNITY_SWITCH)
elseif(PLAYFIELD_TARGET_UNITY_PS5)
  set(STRATO_TARGET_UNITY_PS5)
elseif(PLAYFIELD_TARGET_UNITY_XBOX)
  set(STRATO_TARGET_UNITY_XBOX)
endif()

# Linking
if(   PLAYFIELD_ENABLE_STATIC
   OR PLAYFIELD_ENABLE_SHARED
   OR PLAYFIELD_ENABLE_OBJECT)
  # If we make a library, StratoHAL must be objects.
  set(STRATO_ENABLE_OBJECT ON)
else()
  # If we make an executable, StratoHAL should be a static library.
  set(STRATO_ENABLE_STATIC ON)

  # If we use dynamic link for dependencies.
  if(PLAYFIELD_ENABLE_DIST)
    set(STRATO_ENABLE_DIST ON)
  endif()
endif()

# Use translation.
if(PLAYFIELD_ENABLE_I18N)
  set(STRATO_ENABLE_I18N ON)
endif()

# Screen rotate
if(PLAYFIELD_ENABLE_ROT90)
  set(STRATO_ENABLE_ROT90 ON)
endif()

if(PLAYFIELD_ENABLE_GST)
  set(STRATO_ENABLE_GST ON)
endif()

# libstratopack
if(PLAYFIELD_ENABLE_PACK)
  set(STRATO_ENABLE_PACK ON)
endif()

# ---

#
# Release/Debug
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
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g3 -DDEBUG")
    set(CMAKE_OBJC_FLAGS_DEBUG "${CMAKE_OBJC_FLAGS_DEBUG} -O0 -g3 -DNDEBUG")
endif()

# Release Configuration
if(MSVC)
  # MSVC
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  # GCC/Clang
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -g0 -DNDEBUG")
  set(CMAKE_OBJC_FLAGS_RELEASE "${CMAKE_OBJC_FLAGS_RELEASE} -O2 -g0 -DNDEBUG")
endif()

# ---

#
# Apple Quirks
#

if(   PLAYFIELD_TARGET_MACOS
   OR PLAYFIELD_TARGET_MACOS_CLI
   OR PLAYFIELD_TARGET_MACOS_LIB)
  set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
endif()

# ---

#
# Dependencies
#

# NoctLang
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/NoctLang)

# StratoHAL
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/StratoHAL)

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
# Main Target
#

# Base Source
set(PLAYFIELD_BASE_SOURCES
  src/api.c
  src/common.c
  src/mainloop.c
  src/vm.c
  src/library.c
)

# I18N Source
if(PLAYFIELD_ENABLE_I18N)
  set(PLAYFIELD_I18N_SOURCES
    src/i18n.c
    src/translation.c
  )
endif()

# Resource
if(   PLAYFIELD_TARGET_WINDOWS
   OR PLAYFIELD_TARGET_WINDOWS95)
  if(NOT PLAYFIELD_ENABLE_STATIC)
    set(PLAYFIELD_RESOURCES resources/windows/resource.rc)
  endif()
endif()

# Add Target
if(   PLAYFIELD_TARGET_IOS
   OR PLAYFIELD_TARGET_ANDROID
   OR PLAYFIELD_TARGET_OPENHARMONY
   OR PLAYFIELD_TARGET_UNITY
   OR PLAYFIELD_ENABLE_STATIC
   OR PLAYFIELD_ENABLE_SHARED
)
  if(PLAYFIELD_ENABLE_SHARED)
    set(PLAYFIELD_LIB_TYPE SHARED)
  else()
    set(PLAYFIELD_LIB_TYPE STATIC)
  endif()

  if(NOT PLAYFIELD_ENABLE_DIST)
    set(
      PLAYFIELD_DEPS
      $<TARGET_OBJECTS:png>
      $<TARGET_OBJECTS:jpeg>
      $<TARGET_OBJECTS:webp>
      $<TARGET_OBJECTS:vorbisfile>
      $<TARGET_OBJECTS:vorbis>
      $<TARGET_OBJECTS:ogg>
      $<TARGET_OBJECTS:freetype>
      $<TARGET_OBJECTS:brotlidec>
      $<TARGET_OBJECTS:brotlicommon>
      $<TARGET_OBJECTS:bz2>
      $<TARGET_OBJECTS:z>
    )      
  endif()

  # Make a library.
  add_library(
    playfield
    ${PLAYFIELD_LIB_TYPE}
    ${PLAYFIELD_BASE_SOURCES}
    ${PLAYFIELD_I18N_SOURCES}
    ${PLAYFIELD_RESOURCES}
    ${PLAYFIELD_DEPS}
  )
else()
  # Make an executable.
  add_executable(
    playfield
    ${PLAYFIELD_BASE_SOURCES}
    ${PLAYFIELD_I18N_SOURCES}
    ${PLAYFIELD_RESOURCES}
  )
endif()

# ---

#
# CPPFLAGS
#

target_include_directories(
  playfield
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  external/NoctLang/src/core
)

# JIT
if(PLAYFIELD_ENABLE_JIT)
  target_compile_definitions(playfield PRIVATE PF_USE_JIT)
endif()

# I18N
if(PLAYFIELD_ENABLE_I18N)
  target_compile_definitions(playfield PRIVATE PF_USE_TRANSLATION)
endif()

# DLL
if(PLAYFIELD_ENABLE_SHARED)
  target_compile_definitions(playfield PRIVATE PF_USE_SHARED)
endif()

# Unity
if(PLAYFIELD_TARGET_UNITY)
  target_compile_definitions(playfield PRIVATE PF_TARGET_UNITY)
endif()

# Init Hook
if(PLAYFIELD_ENABLE_INITHOOK)
  target_compile_definitions(playfield PRIVATE PF_USE_INITHOOK)
endif()

# Windows
if(PLAYFIELD_TARGET_WINDOWS)
  if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  endif()
endif()

# Windows 95
if(PLAYFIELD_TARGET_WINDOWS95)
  if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  else()
    add_compile_definitions(_WIN32_WINNT=0x0400 __USE_MINGW_ANSI_STDIO=1)
  endif()
endif()

# Emscripten
if(PLAYFIELD_TARGET_WASM AND NOT PLAYFIELD_ENABLE_STATIC)
  target_compile_definitions(playfield PRIVATE USE_EMSCRIPTEN)
endif()

# ---

#
# CFLAGS
#

# Windows
if(PLAYFIELD_TARGET_WINDOWS)
  if(MSVC)
    target_compile_options(playfield PUBLIC /utf-8)
  else()
    target_compile_options(playfield PUBLIC -municode -finput-charset=utf-8 -fexec-charset=utf-8)
  endif()
endif()

# Windows 95
if(PLAYFIELD_TARGET_WINDOWS95)
  if(MSVC)
    target_compile_options(playfield PUBLIC /utf-8)
  else()
    target_compile_options(playfield PUBLIC -finput-charset=utf-8 -fexec-charset=shift-jis)
  endif()
endif()

# ---

#
# LDFLAGS
#

# Link NoctLang and Strato
target_link_libraries(playfield PUBLIC
  noctlang
  strato
)

# Windows
if(PLAYFIELD_TARGET_WINDOWS)
  if(MSVC)
    target_link_options(playfield PUBLIC /SUBSYSTEM:WINDOWS)
  else()
    target_link_options(playfield PUBLIC -mwindows -static)
  endif()
endif()

# Windows 95
if(PLAYFIELD_TARGET_WINDOWS95)
  if(MSVC)
    target_link_options(playfield PUBLIC /SUBSYSTEM:WINDOWS)
  else()
    target_link_options(playfield PUBLIC -mwindows -static -static-libgcc)
  endif()
endif()

# Emscripten
if(PLAYFIELD_TARGET_WASM AND NOT PLAYFIELD_ENABLE_STATIC)
  # Make the output name "index.html"
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  set_target_properties(playfield PROPERTIES OUTPUT_NAME index)
  target_link_options(playfield PRIVATE
    -o index.html
    -sSINGLE_FILE=1
    -sTOTAL_MEMORY=536870912
    -sNO_EXIT_RUNTIME=1
    -sEXPORTED_RUNTIME_METHODS=[ccall,UTF8ToString]
    -lopenal
    -lidbfs.js
    --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/src/shell.html"
    --pre-js "${CMAKE_CURRENT_SOURCE_DIR}/src/pre.js"
    --use-preload-plugins
    --profiling-funcs
  )
endif()

# Emscripten Local
if(PLAYFIELD_TARGET_WASM_LOCAL AND NOT PLAYFIELD_ENABLE_STATIC)
  # Make the output name "index.html"
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  set_target_properties(playfield PROPERTIES OUTPUT_NAME index)
  target_link_options(playfield PRIVATE
    -o index.html
    -sSINGLE_FILE=1
    -sTOTAL_MEMORY=536870912
    -sNO_EXIT_RUNTIME=1
    -sEXPORTED_FUNCTIONS=[_onLoadProject,_setVisible,_setHidden,_malloc]
    -sEXPORTED_RUNTIME_METHODS=[ccall,UTF8ToString,writeArrayToMemory]
    -s ASYNCIFY
    -lopenal
    -lidbfs.js
    --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/src/shelllocal.html"
    --pre-js "${CMAKE_CURRENT_SOURCE_DIR}/src/prelocal.js"
    --use-preload-plugins
    --profiling-funcs
  )
endif()

# ---

#
# Packager Target
#

if(PLAYFIELD_ENABLE_PACK)
  if(    NOT PLAYFIELD_ENABLE_STATIC
     AND NOT PLAYFIELD_ENABLE_SHARED
     AND NOT PLAYFIELD_ENABLE_OBJECT)
    if(PLAYFIELD_TARGET_WINDOWS OR PLAYFIELD_TARGET_WINDOWS95)
      set(PLAYFIELD_PACK_RESOURCE resources/windows/pack-resource.rc)
    endif()

    add_executable(
      playfield-pack
      src/pack.c
      ${PLAYFIELD_PACK_RESOURCE}}
    )

    target_link_libraries(playfield-pack stratopack)
  endif()
endif()

# ---

#
# Bytecode Compiler Target
#

if(PLAYFIELD_ENABLE_BYTECOMP)
  if(PLAYFIELD_TARGET_WINDOWS OR PLAYFIELD_TARGET_WINDOWS95)
    set(PLAYFIELD_BYTECOMP_RESOURCE resources/windows/bytecomp-resource.rc)
  endif()

  add_executable(
    playfield-bytecomp
    src/compiler.c
    ${PLAYFIELD_BYTECOMP_RESOURCE}
  )

  target_include_directories(
    playfield-bytecomp
    PRIVATE
    include
    external/StratoHAL/include
    external/NoctLang/src/core
    external/NoctLang/src/backend
  )

  target_link_libraries(playfield-bytecomp noctlang)

  if(UNIX)
    target_link_libraries(playfield-bytecomp m)
  endif()
endif()

# ---

#
# AOT Compiler Target
#

if(PLAYFIELD_ENABLE_AOTCOMP)
  if(PLAYFIELD_TARGET_WINDOWS OR PLAYFIELD_TARGET_WINDOWS95)
    set(PLAYFIELD_AOTCOMP_RESOURCE resources/windows/aotcomp-resource.rc)
  endif()

  add_executable(
    playfield-aotcomp
    src/aot.c
    external/NoctLang/src/backend/cback.c
    ${PLAYFIELD_AOTCOMP_RESOURCE}
  )

  target_include_directories(
    playfield-aotcomp
    PRIVATE
    include
    external/StratoHAL/include
    external/NoctLang/src/core
    external/NoctLang/src/backend
  )

  target_link_libraries(playfield-aotcomp noctlang)

  if(UNIX)
    target_link_libraries(playfield-aotcomp m)
  endif()
endif()

# ---

#
# Web Server Target
#

if(PLAYFIELD_ENABLE_WEBSERVER)
  if(PLAYFIELD_TARGET_WINDOWS OR PLAYFIELD_TARGET_WINDOWS95)
    set(PLAYFIELD_WEB_RESOURCE resources/windows/web-resource.rc)
  endif()

  add_executable(
    playfield-web
    src/webserver.c
    ${PLAYFIELD_WEB_RESOURCE}
  )

  if(PLAYFIELD_TARGET_WINDOWS OR PLAYFIELD_TARGET_WINDOWS95)
    target_link_libraries(playfield-web wsock32 ws2_32)
  endif()
endif()

# ---

#
# Editor Target
#

if(PLAYFIELD_ENABLE_EDITOR)
  add_executable(
    playfield-editor
    src/editor.c
    resources/windows/editor-resource.rc
  )

  if(MSVC)
    target_compile_options(playfield-editor PRIVATE /utf-8)
    target_compile_definitions(playfield-editor PRIVATE UNICODE _UNICODE)
    target_link_options(playfield-editor PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup /MANIFEST:NO)
  else()
    target_compile_options(playfield-editor PRIVATE -municode)
    target_link_options(playfield-editor PRIVATE -mwindows -municode)
  endif()
endif()

# ---

#
# macOS Bundle
#

if(PLAYFIELD_TARGET_MACOS
   AND NOT PLAYFIELD_ENABLE_STATIC
   AND NOT PLAYFIELD_ENABLE_SHARED)
  # Use capital name.
  set_target_properties(playfield PROPERTIES OUTPUT_NAME "Playfield")

  # Make an app bundle.
  set_target_properties(playfield PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.noctvm.playfield"
    MACOSX_BUNDLE_BUNDLE_NAME "Playfield"
  )

  # Set an icon.
  set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/icon512.png")
  target_sources(playfield PRIVATE ${APP_ICON})
  set_source_files_properties(${APP_ICON} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
  )
  set_target_properties(playfield PROPERTIES
    MACOSX_BUNDLE_ICON_FILE "icon512.png"
  )

  # Copy the assets.arc file.
  set(APP_GAME_DATA "${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/assets.arc")
  set_source_files_properties(${APP_GAME_DATA} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
  )
  target_sources(playfield PRIVATE ${APP_GAME_DATA})
endif()

# ---

#
# GDK Packaging
#

if(0)
if(PLAYFIELD_TARGET_GDK_WINDOWS)
  set(PKG_STAGING_DIR "${CMAKE_BINARY_DIR}/gdk_staging")
  set(PKG_OUTPUT_DIR  "${CMAKE_BINARY_DIR}/gdk_package")
  set(PKG_LAYOUT_XML "${CMAKE_BINARY_DIR}/layout.xml")

  # EXE
  set(GAME_TARGET playfield)

  # Path to makepkg.exe
  set(GDK_MAKEPKG "C:/Program Files (x86)/Microsoft GDK/bin/makepkg.exe")
  set(GDK_WDAPP   "C:/Program Files (x86)/Microsoft GDK/bin/wdapp.exe")

  add_custom_command(TARGET playfield POST_BUILD
    # Make gdk_staging.
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PKG_STAGING_DIR}"
    # Copy assets.
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/samples/bouncer/main.pf" "${PKG_STAGING_DIR}/"
    # Copy EXE file.
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${GAME_TARGET}>" "${PKG_STAGING_DIR}/"
    # Copy config files.
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/resources/windows/gdk-desktop/MicrosoftGame.config" "${PKG_STAGING_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/resources/windows/gdk-desktop/logo-44x44.png" "${PKG_STAGING_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/resources/windows/gdk-desktop/logo-100x100.png" "${PKG_STAGING_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/resources/windows/gdk-desktop/logo-150x150.png" "${PKG_STAGING_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/resources/windows/gdk-desktop/splashscreen.png" "${PKG_STAGING_DIR}/"
    # Make gdk_output.
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PKG_OUTPUT_DIR}"
    # Make a mapping file.
    COMMAND "${GDK_MAKEPKG}" genmap /f "${PKG_LAYOUT_XML}" /d "${PKG_STAGING_DIR}"
    # pack
    COMMAND "${GDK_MAKEPKG}" pack /f "${PKG_LAYOUT_XML}" /lt /d "${PKG_STAGING_DIR}" /nogameos /pc /pd "${PKG_OUTPUT_DIR}"
    # Install to local.
    COMMAND "${GDK_WDAPP}" install "${PKG_OUTPUT_DIR}/PlayfieldEngineSample_1.0.0.0_x64__6ng4m9yy9amvm.msixvc"
  )
endif()
endif()

# ---

#
# UNIX Install
#

if(   PLAYFIELD_TARGET_LINUX
   OR PLAYFIELD_TARGET_LINUX_WAYLAND
   OR PLAYFIELD_TARGET_LINUX_GBM
   OR PLAYFIELD_TARGET_LINUX_FBDEV
   OR PLAYFIELD_TARGET_LINUX_X11SOFT
   OR PLAYFIELD_TARGET_FREEBSD
   OR PLAYFIELD_TARGET_FREEBSD_WAYLAND
   OR PLAYFIELD_TARGET_FREEBSD_X11SOFT
   OR PLAYFIELD_TARGET_NETBSD
   OR PLAYFIELD_TARGET_NETBSD_X11SOFT
   OR PLAYFIELD_TARGET_OPENBSD
   OR PLAYFIELD_TARGET_OPENBSD_X11SOFT
   OR PLAYFIELD_TARGET_SOLARIS11
   OR PLAYFIELD_TARGET_GENERICUNIX
   OR PLAYFIELD_TARGET_HAIKU)
  if(NOT PLAYFIELD_ENABLE_STATIC AND NOT PLAYFIELD_ENABLE_SHARED)
    include(GNUInstallDirs)

    install(TARGETS playfield RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(TARGETS playfield-bytecomp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(TARGETS playfield-aotcomp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(TARGETS playfield-pack RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    if(   PLAYFIELD_TARGET_FREEBSD
       OR PLAYFIELD_TARGET_FREEBSD_WAYLAND
       OR PLAYFIELD_TARGET_FREEBSD_X11SOFT
       OR PLAYFIELD_TARGET_NETBSD
       OR PLAYFIELD_TARGET_NETBSD_X11SOFT
       OR PLAYFIELD_TARGET_OPENBSD
       OR PLAYFIELD_TARGET_OPENBSD_X11SOFT)
      # BSD manpage
      install(
        FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/playfield.1.mdoc"
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
        RENAME      "playfield.1"
      )
      install(
        FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/playfield-bytecomp.1.mdoc"
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
        RENAME      "playfield-bytecomp.1"
      )
      install(
        FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/playfield-aotcomp.1.mdoc"
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
        RENAME      "playfield-aotcomp.1"
      )
      install(
        FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/playfield-pack.1.mdoc"
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
        RENAME      "playfield-pack.1"
      )
    else()
      # UNIX and Linux manpage
      install(
        FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/playfield.1"
                    "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/playfield-bytecomp.1"
                    "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/playfield-aotcomp.1"
                    "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/playfield-pack.1"
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
      )
    endif()
  endif()
endif()
