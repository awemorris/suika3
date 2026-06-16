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
option(PLAYFIELD_TARGET_LINUX            "Build for Linux (Wayland/X11 Dual)"         OFF)
option(PLAYFIELD_TARGET_LINUX_X11        "Build for Linux (X11, OpenGL)"              OFF)
option(PLAYFIELD_TARGET_LINUX_WAYLAND    "Build for Linux (Wayland, OpenGL)"          OFF)
option(PLAYFIELD_TARGET_LINUX_DRI        "Build for Linux (DRI, OpenGL ES)"           OFF)
option(PLAYFIELD_TARGET_LINUX_FBDEV      "Build for Linux (fbdev, Soft3D)"            OFF)
option(PLAYFIELD_TARGET_LINUX_X11SOFT    "Build for Linux (X11, Soft3D)"              OFF)
option(PLAYFIELD_TARGET_IOS              "Build for iOS"                              OFF)
option(PLAYFIELD_TARGET_ANDROID          "Build for Android (OpenGL ES, OpenSL ES)"   OFF)
option(PLAYFIELD_TARGET_OPENHARMONY      "Build for OpenHarmony (OpenGL ES)"          OFF)
option(PLAYFIELD_TARGET_WASM             "Build for Emscripten (WebGL)"               OFF)
option(PLAYFIELD_TARGET_WASM_LOCAL       "Build for Emscripten Local (WebGL)"         OFF)
option(PLAYFIELD_TARGET_UNITY            "Build for Unity"                            OFF)
option(PLAYFIELD_TARGET_FREEBSD          "Build for FreeBSD (Wayland/X11 Dual)"       OFF)
option(PLAYFIELD_TARGET_FREEBSD_X11      "Build for FreeBSD (X11, OpenGL)"            OFF)
option(PLAYFIELD_TARGET_FREEBSD_WAYLAND  "Build for FreeBSD (Wayland, OpenGL)"        OFF)
option(PLAYFIELD_TARGET_FREEBSD_X11SOFT  "Build for FreeBSD (X11, Soft3D)"            OFF)
option(PLAYFIELD_TARGET_NETBSD           "Build for NetBSD (X11, OpenGL)"             OFF)
option(PLAYFIELD_TARGET_NETBSD_X11SOFT   "Build for NetBSD (X11, Soft3D)"             OFF)
option(PLAYFIELD_TARGET_OPENBSD          "Build for OpenBSD (X11, OpenGL)"            OFF)
option(PLAYFIELD_TARGET_OPENBSD_X11SOFT  "Build for OpenBSD (X11, Soft3D)"            OFF)
option(PLAYFIELD_TARGET_SOLARIS11        "Build for Solaris 11 (X11, Soft3D)"         OFF)
option(PLAYFIELD_TARGET_GENERICUNIX      "Build for Unknown UNIX (X11, Soft3D)"       OFF)
option(PLAYFIELD_TARGET_HAIKU            "Build for Haiku OS (Soft3D)"                OFF)
option(PLAYFIELD_TARGET_GDK_WINDOWS      "Build for GDK for Desktop"                  OFF)
option(PLAYFIELD_TARGET_GDK_XBOX_XS      "Build for GDK for Xbox Series X|S"          OFF)
option(PLAYFIELD_TARGET_PC98             "Build for NEC PC-9801"                      OFF)
option(PLAYFIELD_TARGET_PCAT             "Build for IBM PC/AT"                        OFF)

# Options
option(PLAYFIELD_ENABLE_STATIC           "Build a static library"                     OFF)
option(PLAYFIELD_ENABLE_SHARED           "Build a dynamic library"                    OFF)
option(PLAYFIELD_ENABLE_OBJECT           "Build an object library"                    OFF)
option(PLAYFIELD_ENABLE_BIN              "Build an executable"                        OFF)
option(PLAYFIELD_ENABLE_DIST             "Link shared libraries"                      OFF)
option(PLAYFIELD_ENABLE_INSTALL          "Install library and header files"           OFF)
option(PLAYFIELD_ENABLE_JIT              "Enable JIT"                                 OFF)
option(PLAYFIELD_ENABLE_I18N             "Enable translation"                         OFF)
option(PLAYFIELD_ENABLE_I18N_LIBINTL     "Enable gettext"                             OFF)
option(PLAYFIELD_ENABLE_PACK             "Enable packager"                            OFF)
option(PLAYFIELD_ENABLE_BCC              "Enable bytecode compiler"                   OFF)
option(PLAYFIELD_ENABLE_AOTC             "Enable AOT compiler"                        OFF)
option(PLAYFIELD_ENABLE_WEBSERVER        "Enable Web server"                          OFF)
option(PLAYFIELD_ENABLE_EDITOR           "Enable simple editor for Windows"           OFF)
option(PLAYFIELD_ENABLE_ROT90            "Rotate screen for DRI"                      OFF)
option(PLAYFIELD_ENABLE_GST              "Enable Gstreamer support"                   OFF)
option(PLAYFIELD_ENABLE_CONSOLE          "Enable console debug output on Windows"     OFF)
option(PLAYFIELD_ENABLE_UNSAFE           "Enable unsafe APIs for launchers"           OFF)
option(PLAYFIELD_ENABLE_PORTAL           "Enable XDG Portal"                          OFF)
option(PLAYFIELD_ENABLE_APPMAIN          "Enable default main() for library"          OFF)

# ---

#
# Automatic Target Detection
#

# For when a target is manually specified.
if(   PLAYFIELD_TARGET_WINDOWS
   OR PLAYFIELD_TARGET_WINDOWS95
   OR PLAYFIELD_TARGET_MACOS
   OR PLAYFIELD_TARGET_MACOS_CLI
   OR PLAYFIELD_TARGET_LINUX
   OR PLAYFIELD_TARGET_LINUX_WAYLAND
   OR PLAYFIELD_TARGET_LINUX_DRI
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
   OR PLAYFIELD_TARGET_SOLARIS10
   OR PLAYFIELD_TARGET_SOLARIS11
   OR PLAYFIELD_TARGET_GENERICUNIX
   OR PLAYFIELD_TARGET_HAIKU
   OR PLAYFIELD_TARGET_GDK_WINDOWS
   OR PLAYFIELD_TARGET_GDK_XBOX_XS
   OR PLAYFIELD_TARGET_UWP
   OR PLAYFIELD_TARGET_PC98
   OR PLAYFIELD_TARGET_PCAT
)
  # A target and options are manually specified.
else()   
  if(WIN32)
    set(PLAYFIELD_TARGET_WINDOWS                  ON)
    set(PLAYFIELD_ENABLE_STATIC                   ON)
    set(PLAYFIELD_ENABLE_BIN                      ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)
    set(PLAYFIELD_ENABLE_WEBSERVER                ON)

  elseif(APPLE AND NOT IOS)
    set(PLAYFIELD_TARGET_MACOS_CLI                ON)
    set(PLAYFIELD_ENABLE_STATIC                   ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_BIN                      ON)
    set(PLAYFIELD_ENABLE_BUNDLE                   ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLAYFIELD_TARGET_LINUX                    ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_BIN                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(PLAYFIELD_TARGET_FREEBSD                  ON)
    set(PLAYFIELD_ENABLE_STATIC                   ON)
    set(PLAYFIELD_ENABLE_BIN                      ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_GST                      ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    set(PLAYFIELD_TARGET_NETBSD                   ON)
    set(PLAYFIELD_ENABLE_STATIC                   ON)
    set(PLAYFIELD_ENABLE_BIN                      ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    set(PLAYFIELD_TARGET_OPENBSD_X11SOFT          ON)
    set(PLAYFIELD_ENABLE_STATIC                   ON)
    set(PLAYFIELD_ENABLE_BIN                      ON)
    set(PLAYFIELD_ENABLE_JIT                      OFF)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "5.11")
    set(PLAYFIELD_TARGET_SOLARIS11                ON)
    set(PLAYFIELD_ENABLE_STATIC                   ON)
    set(PLAYFIELD_ENABLE_BIN                      ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    set(PLAYFIELD_TARGET_HAIKU                    ON)
    set(PLAYFIELD_ENABLE_STATIC                   ON)
    set(PLAYFIELD_ENABLE_BIN                      ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_I18N                     ON)
    set(PLAYFIELD_ENABLE_PACK                     ON)

  else()
    set(STRATO_TARGET_GENERICUNIX                 ON)
    set(PLAYFIELD_ENABLE_JIT                      ON)
    set(PLAYFIELD_ENABLE_STATIC                   ON)
    set(PLAYFIELD_ENABLE_BIN                      ON)
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
# Noct Configuration Propagation
# (For Static Link)
#

if(NOT PLAYFIELD_ENABLE_DIST)
  # Merge object files
  set(NOCT_ENABLE_OBJECT ON)

  # JIT
  if(PLAYFIELD_ENABLE_JIT)
    set(NOCT_ENABLE_JIT ON)
  endif()

  # API
  if(PLAYFIELD_ENABLE_UNSAFE)
    set(NOCT_ENABLE_API ON)
  else()
    set(NOCT_ENABLE_API OFF)
  endif()

  # I18N
  if(PLAYFIELD_ENABLE_I18N)
    set(NOCT_ENABLE_I18N ON)
  else()
    set(NOCT_ENABLE_I18N OFF)
  endif()

  # I18N (libintl)
  if(PLAYFIELD_ENABLE_I18N_LIBINTL)
    set(NOCT_ENABLE_I18N_LIBINTL ON)
  endif()

  # AOTC
  if(PLAYFIELD_ENABLE_AOTC)
    set(NOCT_ENABLE_CBACKEND ON)
  endif()

  # BCC
  if(PLAYFIELD_ENABLE_BCC)
    set(NOCT_ENABLE_BCBACKEND ON)
  endif()

  # Unity
  if(PLAYFIELD_TARGET_UNITY)
    set(NOCT_TARGET_UNITY ON)
  endif()

  # DOS
  if(PLAYFIELD_TARGET_PC98 OR PLAYFIELD_TARGET_PCAT)
    set(NOCT_TARGET_DOS4G ON)
  endif()
endif()

# ---

#
# Strato Configuration Propagation
# (For Static Link)
#

if(NOT PLAYFIELD_ENABLE_DIST)
  # Merge object files
  set(STRATO_ENABLE_OBJECT ON)

  # Propagate the target and its specific options.
  if(PLAYFIELD_TARGET_WINDOWS)
    set(STRATO_TARGET_WINDOWS ON)
  elseif(PLAYFIELD_TARGET_WINDOWS95)
    set(STRATO_TARGET_WINDOWS95 ON)
  elseif(PLAYFIELD_TARGET_MACOS)
    set(STRATO_TARGET_MACOS ON)
  elseif(PLAYFIELD_TARGET_MACOS_CLI)
    set(STRATO_TARGET_MACOS ON)
  elseif(PLAYFIELD_TARGET_LINUX)
    set(STRATO_TARGET_LINUX ON)
  elseif(PLAYFIELD_TARGET_LINUX_X11)
    set(STRATO_TARGET_LINUX_X11 ON)
  elseif(PLAYFIELD_TARGET_LINUX_WAYLAND)
    set(STRATO_TARGET_LINUX_WAYLAND ON)
  elseif(PLAYFIELD_TARGET_LINUX_DRI)
    set(STRATO_TARGET_LINUX_DRI ON)
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
  elseif(PLAYFIELD_TARGET_FREEBSD_X11)
    set(STRATO_TARGET_FREEBSD_X11 ON)
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
  elseif(PLAYFIELD_TARGET_SOLARIS10)
    set(STRATO_TARGET_SOLARIS10 ON)
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
  elseif(PLAYFIELD_TARGET_PC98)
    set(STRATO_TARGET_PC98 ON)
  elseif(PLAYFIELD_TARGET_PCAT)
    set(STRATO_TARGET_PCAT ON)
  else()
    error("No target specified or detected.")
  endif()

  if(PLAYFIELD_TARGET_UNITY_SWITCH)
    set(STRATO_TARGET_UNITY_SWITCH)
  elseif(PLAYFIELD_TARGET_UNITY_PS5)
    set(STRATO_TARGET_UNITY_PS5)
  elseif(PLAYFIELD_TARGET_UNITY_XBOX)
    set(STRATO_TARGET_UNITY_XBOX)
  endif()

  # I18N
  if(PLAYFIELD_ENABLE_I18N)
    set(STRATO_ENABLE_I18N ON)
  endif()

  # I18N (libintl)
  if(PLAYFIELD_ENABLE_I18N_LIBINTL)
    set(STRATO_ENABLE_I18N_LIBINTL ON)
  endif()

  # ROT90
  if(PLAYFIELD_ENABLE_ROT90)
    set(STRATO_ENABLE_ROT90 ON)
  endif()

  # Gstreamer
  if(PLAYFIELD_ENABLE_GST)
    set(STRATO_ENABLE_GST ON)
  endif()

  # Console output
  if(PLAYFIELD_ENABLE_CONSOLE)
    set(STRATO_ENABLE_CONSOLE)
  endif()

  # XDG-Portal
  if(PLAYFIELD_ENABLE_PORTAL)
    set(STRATO_ENABLE_PORTAL ON)
  endif()

  # PACK
  if(PLAYFIELD_ENABLE_PACK)
    set(STRATO_ENABLE_PACK ON)
  endif()
endif()

# ---

#
# gettext
#

if(PLAYFIELD_ENABLE_I18N_LIBINTL)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
  find_package(INTL REQUIRED)
  include(GNUInstallDirs)
  add_definitions(-DLOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}")
endif()

# ---

#
# Release/Debug
#

# Debug Configuration
if(MSVC)
  # MSVC
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /Zi /DDEBUG /UNDEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /DDEBUG /UNDEBUG")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  # GCC/Clang
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -O0 -g3 -Wall -Werror -Wextra -Wundef -Wconversion")
  set(CMAKE_OBJC_FLAGS_DEBUG "${CMAKE_OBJC_FLAGS_DEBUG} -DNDEBUG -O0 -g3 -Wall -Werror -Wextra -Wundef -Wconversion")
endif()

# Release Configuration
if(MSVC)
  # MSVC
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  # GCC/Clang
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG -O2 -g0")
  set(CMAKE_OBJC_FLAGS_RELEASE "${CMAKE_OBJC_FLAGS_RELEASE} -DNDEBUG -O2 -g0")
endif()

# ---

#
# Dependencies
#

if(NOT PLAYFIELD_ENABLE_DIST)
  # NoctLang
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/NoctLang)

  # StratoHAL
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/StratoHAL)
else()
  # Use system libraries.
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
  find_package(StratoHAL REQUIRED COMPONENTS pack)
  find_package(Noct REQUIRED)
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
# Library Target
#

# Base Source
set(PLAYFIELD_BASE_SOURCES
  src/api.c
  src/common.c
  src/mainloop.c
  src/vm.c
)

# I18N Source
if(PLAYFIELD_ENABLE_I18N)
  if(NOT PLAYFIELD_ENABLE_I18N_LIBINTL)
    set(PLAYFIELD_I18N_SOURCES
      src/i18n.c
      src/translation.c
    )
  else()
    set(PLAYFIELD_I18N_SOURCES
      src/i18n.c
    )
  endif()
endif()

# Determine the library type.
if(PLAYFIELD_ENABLE_STATIC)
  set(PLAYFIELD_LIB_TYPE STATIC)
elseif(PLAYFIELD_ENABLE_SHARED)
  set(PLAYFIELD_LIB_TYPE SHARED)
elseif(PLAYFIELD_ENABLE_OBJECT)
  set(PLAYFIELD_LIB_TYPE OBJECT)
endif()

# Determine the objects if we use static linking.
if(NOT PLAYFIELD_ENABLE_DIST)
  if(NOT PLAYFIELD_TARGET_PC98 AND NOT PLAYFIELD_TARGET_PCAT)
    set(
      PLAYFIELD_DEPS
      $<TARGET_OBJECTS:png>
      $<TARGET_OBJECTS:jpeg>
      $<TARGET_OBJECTS:webp>
      $<TARGET_OBJECTS:vorbisfile>
      $<TARGET_OBJECTS:vorbis>
      $<TARGET_OBJECTS:ogg>
      $<TARGET_OBJECTS:freetype>
      $<TARGET_OBJECTS:z>
    )      
  else()
    set(
      PLAYFIELD_DEPS
      $<TARGET_OBJECTS:png>
      $<TARGET_OBJECTS:freetype>
      $<TARGET_OBJECTS:z>
    )      
  endif()
endif()

# If the platform needs a library instead of executable,
# include main() to the library.
if(PLAYFIELD_ENABLE_APPMAIN)
  # Include main()
  set(PLAYFIELD_APP_SOURCES src/app.c)
endif()

# If building to merge into a downstream library,
# don't include aot_init_code() because the downstream defines it.
if(PLAYFIELD_ENABLE_APPMAIN)
  set(PLAYFIELD_AOT_SOURCES src/library.c)
endif()

# Make a library.
add_library(
  playfield
  ${PLAYFIELD_LIB_TYPE}
  ${PLAYFIELD_BASE_SOURCES}
  ${PLAYFIELD_I18N_SOURCES}
  ${PLAYFIELD_DEPS}
  ${PLAYFIELD_APP_SOURCES}
  ${PLAYFIELD_AOT_SOURCES}
)
if(PLAYFIELD_ENABLE_SHARED)
  set_target_properties(playfield PROPERTIES 
    VERSION 1.0.0 
    SOVERSION 1
  )
  set_target_properties(playfield PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
  )
endif()

# .so versioning.
set_target_properties(playfield PROPERTIES 
    VERSION 1.0.0
    SOVERSION 1
)

# ---

#
# CPPFLAGS for libplayfield
#

if(NOT PLAYFIELD_ENABLE_DIST)
  set(NOCT_HEADER external/NoctLang/src/core)
else()
  set(NOCT_HEADER ${NOCT_INCLUDE_DIR})
endif()

target_include_directories(
  playfield
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${NOCT_INCLUDE_DIR}
)

# DLL
if(PLAYFIELD_ENABLE_SHARED)
  target_compile_definitions(playfield PRIVATE PF_USE_DLL HAL_USE_DLL)
endif()

# JIT
if(PLAYFIELD_ENABLE_JIT)
  target_compile_definitions(playfield PRIVATE PF_USE_JIT)
endif()

# I18N
if(PLAYFIELD_ENABLE_I18N)
  target_compile_definitions(playfield PRIVATE PF_USE_TRANSLATION)
endif()

# PF_USE_LIBINTL
if(PLAYFIELD_ENABLE_I18N_LIBINTL)
  target_compile_definitions(playfield PRIVATE PF_USE_LIBINTL)
endif()  

# DLL
if(PLAYFIELD_ENABLE_SHARED)
  target_compile_definitions(playfield PRIVATE PF_USE_SHARED)
endif()

# OpenHarmony
if(PLAYFIELD_TARGET_OPENHARMONY)
  target_compile_definitions(playfield PUBLIC PF_TARGET_OPENHARMONY NOCT_TARGET_OPENHARMONY HAL_TARGET_OPENHARMONY)
endif()

# Unity
if(PLAYFIELD_TARGET_UNITY)
  target_compile_definitions(playfield PUBLIC PF_TARGET_UNITY)
endif()

# PC98
if(PLAYFIELD_TARGET_PC98)
  target_compile_definitions(playfield PUBLIC PF_TARGET_PC98)
endif()

# PC/AT
if(PLAYFIELD_TARGET_PCAT)
  target_compile_definitions(playfield PUBLIC PF_TARGET_PCAT)
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
  target_compile_definitions(playfield PRIVATE PF_USE_EMSCRIPTEN)
endif()

# Unsafe
if(PLAYFIELD_ENABLE_UNSAFE)
    target_compile_definitions(playfield PRIVATE PF_USE_UNSAFE)
endif()

# ---

#
# CFLAGS for libplayfield
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
# LDFLAGS for libplayfield
#

# Link NoctLang and Strato
if(NOT PLAYFIELD_ENABLE_DIST)
  target_link_libraries(playfield PUBLIC
    noct
    strato
  )
else()
  target_link_libraries(playfield PUBLIC
    ${NOCT_LIBRARY}
    noctapi
    ${STRATO_LIBRARY}
  )
endif()

# libintl
if(PF_ENABLE_I18N_INTL)
  target_link_libraries(playfield PUBLIC ${INTL_LIBRARY})
endif()

#---

#
# Executable Target
#

# Resource
if(   PLAYFIELD_TARGET_WINDOWS
   OR PLAYFIELD_TARGET_WINDOWS95)
  if(NOT PLAYFIELD_ENABLE_STATIC)
    set(PLAYFIELD_RESOURCES resources/windows/resource.rc)
  endif()
endif()

# Make an executable.
if(PLAYFIELD_ENABLE_BIN)
  add_executable(
    playfieldbin
    src/app.c
    src/library.c
    ${PLAYFIELD_RESOURCES}
  )

  if(PLAYFIELD_TARGET_MACOS AND PLAYFIELD_ENABLE_BIN)
    set_target_properties(playfieldbin PROPERTIES OUTPUT_NAME "Playfield")
  else()
    set_target_properties(playfieldbin PROPERTIES OUTPUT_NAME "playfield")
  endif()
endif()

# ---

#
# CPPFLAGS for playfieldbin
#

if(PLAYFIELD_ENABLE_BIN)
  target_include_directories(
    playfieldbin
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${NOCT_INCLUDE_DIR}
  )
endif()

# PC98
if(PLAYFIELD_TARGET_PC98 AND PLAYFIELD_ENABLE_BIN)
  target_compile_definitions(playfieldbin PRIVATE PF_TARGET_PC98)
endif()

# PC/AT
if(PLAYFIELD_TARGET_PCAT AND PLAYFIELD_ENABLE_BIN)
  target_compile_definitions(playfieldbin PRIVATE PF_TARGET_PCAT)
endif()

# ---

#
# LDFLAGS for playfieldbin
#

if(PLAYFIELD_ENABLE_BIN)
  target_link_libraries(playfieldbin PRIVATE playfield)

  # Windows
  if(PLAYFIELD_TARGET_WINDOWS)
    if(MSVC)
      target_link_options(playfieldbin PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup /MANIFEST:NO)
    else()
      target_link_options(playfieldbin PRIVATE -mwindows -static -Wl,-u,wWinMain -Wl,-u,_wWinMain@16)
    endif()
  endif()

  # Windows 95
  if(PLAYFIELD_TARGET_WINDOWS95)
    if(MSVC)
      target_link_options(playfieldbin PUBLIC /SUBSYSTEM:WINDOWS)
    else()
      target_link_options(playfieldbin PRIVATE -mwindows -static -static-libgcc -Wl,-u,WinMain -Wl,-u,_WinMain@16)
    endif()
  endif()

  # macOS
  if(    NOT PLAYFIELD_ENABLE_STATIC
     AND NOT PLAYFIELD_ENABLE_SHARED
     AND NOT PLAYFIELD_ENABLE_OBJECT)
    if(PLAYFIELD_TARGET_MACOS OR PLAYFIELD_TARGET_MACOS_CLI)
      # Workaround for _main() lookup.
      target_link_options(playfieldbin PRIVATE -Wl,-force_load,$<TARGET_FILE:playfield>)
    endif()
  endif()

  # Emscripten
  if(PLAYFIELD_TARGET_WASM AND NOT PLAYFIELD_ENABLE_STATIC)
    # Make the output name "index.html"
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set_target_properties(playfieldbin PROPERTIES OUTPUT_NAME index)
    target_link_options(playfieldbin PRIVATE
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
    set_target_properties(playfieldbin PROPERTIES OUTPUT_NAME index)
    target_link_options(playfieldbin PRIVATE
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
endif()

# ---

#
# Packager Target
#

if(PLAYFIELD_ENABLE_PACK)
  if(PLAYFIELD_TARGET_WINDOWS OR PLAYFIELD_TARGET_WINDOWS95)
    set(PLAYFIELD_PACK_RESOURCE resources/windows/pack-resource.rc)
  endif()

  add_executable(
    playfield-pack
    src/pack.c
    ${PLAYFIELD_PACK_RESOURCE}
  )

  if(NOT PLAYFIELD_ENABLE_DIST)
    target_link_libraries(playfield-pack PRIVATE stratopack)
  else()
    target_link_libraries(playfield-pack PRIVATE StratoHAL::pack)
  endif()
endif()

# ---

#
# Bytecode Compiler Target
#

if(PLAYFIELD_ENABLE_BCC)
  if(PLAYFIELD_TARGET_WINDOWS OR PLAYFIELD_TARGET_WINDOWS95)
    set(PLAYFIELD_BCC_RESOURCE resources/windows/bytecomp-resource.rc)
  endif()

  add_executable(
    playfield-bcc
    src/compiler.c
    ${PLAYFIELD_BCC_RESOURCE}
  )

  target_include_directories(
    playfield-bcc
    PRIVATE
    include
    external/StratoHAL/include
    external/NoctLang/src/core
    external/NoctLang/src/backend
  )

  if(NOT PLAYFIELD_ENABLE_DIST)
    target_link_libraries(playfield-bcc noct)
  else()
    target_link_libraries(playfield-bcc ${NOCT_LIBRARY})
  endif()

  if(UNIX)
    target_link_libraries(playfield-bcc m)
  endif()
endif()

# ---

#
# AOT Compiler Target
#

if(PLAYFIELD_ENABLE_AOTC)
  if(PLAYFIELD_TARGET_WINDOWS OR PLAYFIELD_TARGET_WINDOWS95)
    set(PLAYFIELD_AOTC_RESOURCE resources/windows/aotcomp-resource.rc)
  endif()

  add_executable(
    playfield-aotc
    src/aot.c
    ${PLAYFIELD_AOTC_RESOURCE}
  )

  target_include_directories(
    playfield-aotc
    PRIVATE
    include
    external/StratoHAL/include
    external/NoctLang/src/core
    external/NoctLang/src/backend
  )

  if(NOT PLAYFIELD_ENABLE_DIST)
    target_link_libraries(playfield-aotc noct)
  else()
    target_link_libraries(playfield-aotc ${NOCT_LIBRARY})
  endif()

  if(UNIX)
    target_link_libraries(playfield-aotc m)
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

if(PLAYFIELD_TARGET_MACOS AND PLAYFIELD_ENABLE_BIN)
  # Make an app bundle.
  set_target_properties(playfieldbin PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.noctvm.playfield"
    MACOSX_BUNDLE_BUNDLE_NAME "Playfield"
  )

  # Set an icon.
  set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/icon512.png")
  target_sources(playfieldbin PRIVATE ${APP_ICON})
  set_source_files_properties(${APP_ICON} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
  )
  set_target_properties(playfieldbin PROPERTIES
    MACOSX_BUNDLE_ICON_FILE "icon512.png"
  )

  # Copy the assets.arc file.
  set(APP_GAME_DATA "${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/assets.arc")
  set_source_files_properties(${APP_GAME_DATA} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
  )
  target_sources(playfieldbin PRIVATE ${APP_GAME_DATA})
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

if(PLAYFIELD_ENABLE_INSTALL)
  include(GNUInstallDirs)

  # libplayfield
  install(TARGETS playfield RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})

  # playfield
  if(PLAYFIELD_ENABLE_BIN)
    install(TARGETS playfieldbin RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

  # playfield-bcc
  if(PLAYFIELD_ENABLE_BCC)
    install(TARGETS playfield-bcc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

  # playfield-aotc
  if(PLAYFIELD_ENABLE_AOTC)
    install(TARGETS playfield-aotc RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

  # playfield-pack
  if(PLAYFIELD_ENABLE_PACK)
    install(TARGETS playfield-pack RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/playfield/playfield.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/playfield/c89compat.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/playfield)

  # Manpages.
  if(PLAYFIELD_ENABLE_BIN)
    if(   PLAYFIELD_TARGET_FREEBSD
       OR PLAYFIELD_TARGET_FREEBSD_X11
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
        FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/playfield-bcc.1.mdoc"
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
        RENAME      "playfield-bcc.1"
      )
      install(
        FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/playfield-aotc.1.mdoc"
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
        RENAME      "playfield-aotc.1"
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
                    "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/playfield-bcc.1"
                    "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/playfield-aotc.1"
                    "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/playfield-pack.1"
        DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
      )
    endif()
  endif()

  # gettext
  if(PLAYFIELD_ENABLE_I18N_LIBINTL)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/locale/
            DESTINATION ${CMAKE_INSTALL_FULL_LOCALEDIR})
  endif()

  # CMake module
  install(
    FILES "${CMAKE_SOURCE_DIR}/cmake/modules/FindPlayfield.cmake"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/cmake/Modules"
  )

  # pkgconfig pc
  configure_file(
    "${CMAKE_SOURCE_DIR}/resources/pkg-config/playfield.pc.in"
    playfield.pc
    @ONLY
  )
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/playfield.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
  )
endif()
