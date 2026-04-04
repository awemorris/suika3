cmake_minimum_required(VERSION 3.13)
project(StratoHAL LANGUAGES C CXX)

#
# Configuration
#

# Target
option(STRATO_TARGET_WINDOWS          "Build for Windows 2000 or later"         OFF)
option(STRATO_TARGET_WINDOWS95        "Build for Windows 95/98/Me"              OFF)
option(STRATO_TARGET_MACOS            "Build for macOS"                         OFF)
option(STRATO_TARGET_LINUX            "Build for Linux (X11, OpenGL)"           OFF)
option(STRATO_TARGET_LINUX_WAYLAND    "Build for Linux (Wayland, OpenGL)"       OFF)
option(STRATO_TARGET_LINUX_GBM        "Build for Linux (GBM, OpenGL ES)"        OFF)
option(STRATO_TARGET_LINUX_FBDEV      "Build for Linux (fbdev, Soft3D)"         OFF)
option(STRATO_TARGET_LINUX_X11SOFT    "Build for Linux (X11, Soft3D)"           OFF)
option(STRATO_TARGET_IOS              "Build for iOS"                           OFF)
option(STRATO_TARGET_ANDROID          "Build for Android (OpenGL ES)"           OFF)
option(STRATO_TARGET_OPENHARMONY      "Build for OpenHarmony (OpenGL)"          OFF)
option(STRATO_TARGET_WASM             "Build for Emscripten (WebGL)"            OFF)
option(STRATO_TARGET_WASM_LOCAL       "Build for Emscripten Local (WebGL)"      OFF)
option(STRATO_TARGET_UNITY            "Build for Unity"                         OFF)
option(STRATO_TARGET_FREEBSD          "Build for FreeBSD (X11, OpenGL)"         OFF)
option(STRATO_TARGET_FREEBSD_WAYLAND  "Build for FreeBSD (Wayland, OpenGL)"     OFF)
option(STRATO_TARGET_FREEBSD_X11SOFT  "Build for FreeBSD (X11, Soft3D)"         OFF)
option(STRATO_TARGET_NETBSD           "Build for NetBSD (X11, OpenGL)"          OFF)
option(STRATO_TARGET_NETBSD_X11SOFT   "Build for NetBSD (X11, Soft3D)"          OFF)
option(STRATO_TARGET_OPENBSD          "Build for OpenBSD (X11, OpenGL)"         OFF)
option(STRATO_TARGET_OPENBSD_X11SOFT  "Build for OpenBSD (X11, Soft3D)"         OFF)
option(STRATO_TARGET_SOLARIS11        "Build for Solaris 11 (X11, Soft3D)"      OFF)
option(STRATO_TARGET_GENERICUNIX      "Build for Unknown UNIX (X11, Soft3D)"    OFF)
option(STRATO_TARGET_HAIKU            "Build for Haiku OS (Soft3D)"             OFF)
option(STRATO_TARGET_UWP              "Build for UWP"                           OFF)
option(STRATO_TARGET_GDK_WINDOWS      "Build for GDK for Desktop"               OFF)
option(STRATO_TARGET_GDK_XBOX_XS      "Build for GDK for Xbox Series X|S"       OFF)

# Options
option(STRATO_ENABLE_OBJECT           "Build an object library"                 OFF)
option(STRATO_ENABLE_STATIC           "Build a static library"                  OFF)
option(STRATO_ENABLE_SHARED           "Build a shared libraries"                OFF)
option(STRATO_ENABLE_DIST             "Link shared libraries"                   OFF)
option(STRATO_ENABLE_PACK             "Build packager"                          OFF)
option(STRATO_ENABLE_I18N             "Enable translation"                      OFF)
option(STRATO_ENABLE_ROT90            "Rotate screen"                           OFF)
option(STRATO_ENABLE_GST              "Enable Gstreamer support"                OFF)
option(STRATO_ENABLE_MAIN2            "Enable macOS main2()"                    OFF)

#
# Automatic Target Detection
#

# For when a target is manually specified.
if(   STRATO_TARGET_WINDOWS
   OR STRATO_TARGET_WINDOWS95
   OR STRATO_TARGET_MACOS
   OR STRATO_TARGET_MACOS_LIB
   OR STRATO_TARGET_LINUX
   OR STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_LINUX_GBM
   OR STRATO_TARGET_LINUX_FBDEV
   OR STRATO_TARGET_LINUX_X11SOFT
   OR STRATO_TARGET_IOS
   OR STRATO_TARGET_ANDROID
   OR STRATO_TARGET_OPENHARMONY
   OR STRATO_TARGET_WASM
   OR STRATO_TARGET_WASM_LOCAL
   OR STRATO_TARGET_UNITY
   OR STRATO_TARGET_FREEBSD
   OR STRATO_TARGET_FREEBSD_WAYLAND
   OR STRATO_TARGET_FREEBSD_X11SOFT
   OR STRATO_TARGET_NETBSD
   OR STRATO_TARGET_NETBSD_X11SOFT
   OR STRATO_TARGET_OPENBSD
   OR STRATO_TARGET_OPENBSD_X11SOFT
   OR STRATO_TARGET_SOLARIS11
   OR STRATO_TARGET_GENERICUNIX
   OR STRATO_TARGET_HAIKU
   OR STRATO_TARGET_UWP
   OR STRATO_TARGET_GDK_WINDOWS
   OR STRATO_TARGET_GDK_XBOX_XS
)
  # A target and options are manually specified.
else()
  # Detect OS.
  if(WIN32)
    message(STATUS "Targeting Windows.")
    set(STRATO_TARGET_WINDOWS                     ON)
    set(STRATO_ENABLE_STATIC                      ON)

  elseif(APPLE AND NOT IOS)
    message(STATUS "Targeting macOS.")
    set(STRATO_TARGET_MACOS                       ON)
    set(STRATO_ENABLE_SHARED                      ON)
    set(STRATO_ENABLE_DIST                        ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Targeting Linux.")
    set(STRATO_TARGET_LINUX                       ON)
    set(STRATO_ENABLE_SHARED                      ON)
    set(STRATO_ENABLE_DIST                        ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    message(STATUS "Targeting FreeBSD.")
    set(STRATO_TARGET_FREEBSD_X11SOFT             ON)
    set(STRATO_ENABLE_SHARED                      ON)
    set(STRATO_ENABLE_DIST                        ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    message(STATUS "Targeting NetBSD.")
    set(STRATO_TARGET_NETBSD_X11SOFT              ON)
    set(STRATO_ENABLE_SHARED                      ON)
    set(STRATO_ENABLE_DIST                        ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    message(STATUS "Targeting OpenBSD.")
    set(STRATO_TARGET_OPENBSD_X11SOFT             ON)
    set(STRATO_ENABLE_SHARED                      ON)
    set(STRATO_ENABLE_DIST                        ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "5.11")
    message(STATUS "Targeting Solaris 11.")
    set(STRATO_TARGET_SOLARIS11                   ON)
    set(STRATO_ENABLE_SHARED                      ON)
    set(STRATO_ENABLE_DIST                        ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    message(STATUS "Targeting Haiku OS.")
    set(STRATO_TARGET_HAIKU                       ON)

  else()
    message("No supported OS detected. Trying to build for UNIX with X11")
    set(STRATO_TARGET_GENERICUNIX                 ON)
    set(STRATO_ENABLE_STATIC                      ON)

  endif()

  # No automatic detection for:
  #  - iOS
  #  - Android
  #  - OpenHarmony
  #  - Wasm (Emscripten)
  #  - Unity
  #  - GDK (Windows or Xbox)
endif()

# ---

#
# Build Type
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
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} -O0 -g3 -UNDEBUG")
endif()

# Release Configuration
if(MSVC)
  # MSVC
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  # GCC/Clang
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -g0 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -g0 -DNDEBUG")
endif()

# ---

#
# Dependency
#

if(STRATO_ENABLE_DIST)
  # Use system installations.
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
  find_package(PNG REQUIRED)
  find_package(JPEG REQUIRED)
  find_package(WebP REQUIRED)
  find_package(Ogg REQUIRED)
  find_package(Vorbis MODULE REQUIRED)
  find_package(Freetype REQUIRED)
  find_package(Brotli REQUIRED)
  find_package(BZip2 REQUIRED)
  find_package(ZLIB REQUIRED)
else()
  # Build the dependencies statically.
  include(cmake/zlib.cmake)
  include(cmake/libpng.cmake)
  include(cmake/jpeg.cmake)
  include(cmake/libwebp.cmake)
  include(cmake/libogg.cmake)
  include(cmake/libvorbis.cmake)
  include(cmake/brotli.cmake)
  include(cmake/bzip2.cmake)
  include(cmake/freetype.cmake)
endif()

# libX11
if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_LINUX_X11SOFT
   OR STRATO_TARGET_FREEBSD
   OR STRATO_TARGET_FREEBSD_X11SOFT
   OR STRATO_TARGET_NETBSD
   OR STRATO_TARGET_NETBSD_X11SOFT
   OR STRATO_TARGET_OPENBSD
   OR STRATO_TARGET_OPENBSD_X11SOFT
   OR STRATO_TARGET_SOLARIS11
   OR STRATO_TARGET_GENERICUNIX)
  # Common
  find_library(X11_LIBRARY X11 REQUIRED)
  find_library(XPM_LIBRARY Xpm REQUIRED)
endif()

# libGL and libGLX
if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_FREEBSD
   OR STRATO_TARGET_NETBSD
   OR STRATO_TARGET_OPENBSD
   OR STRATO_TARGET_GENERICUNIX)
  find_library(GL_LIBRARY GL REQUIRED)
endif()

# GLX (for older Mesa on Linux and FreeBSD)
if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_FREEBSD)
  find_library(GLX_LIBRARY GLX)
endif()

# libEGL
if(   STRATO_TARGET_LINUX_GBM
   OR STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_FREEBSD_WAYLAND)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(EGL REQUIRED egl)
endif()

# libGLESv2
if(   STRATO_TARGET_LINUX_GBM
   OR STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_FREEBSD_WAYLAND)
  pkg_check_modules(GLES2 REQUIRED glesv2)
endif()

# Gstreamer
if(STRATO_ENABLE_GST)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GST REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-video-1.0)
endif()

# ALSA
if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_LINUX_GBM
   OR STRATO_TARGET_LINUX_FBDEV
   OR STRATO_TARGET_LINUX_X11SOFT)
  find_library(ALSA_LIBRARY asound REQUIRED)
endif()

# GBM and DRM
if(STRATO_TARGET_LINUX_GBM)
  find_library(GBM_LIBRARY gbm REQUIRED)
  find_library(DRM_LIBRARY drm REQUIRED)
endif()

# Wayland
if(   STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_FREEBSD_WAYLAND)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-egl)
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
# Main Target
#

# Windows
if(   STRATO_TARGET_WINDOWS
   OR STRATO_TARGET_WINDOWS95)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(STRATO_SOURCES
    src/image.c
    src/glyph.c
    src/wave.c
    src/stdfile.c
    src/winmain.c
    src/d3drender.c
    src/d3d12render.cc
    src/d3d11render.cc
    src/d3d9render.cc
    src/gdirender.c
    src/dsound.c
    src/mfvideo.cc
    src/dsvideo.cc
    src/digamepad.cc
    src/xigamepad.c
  )
endif()

# macOS
if(STRATO_TARGET_MACOS)
  enable_language(OBJC)
  set(STRATO_SOURCES
    src/image.c
    src/glyph.c
    src/wave.c
    src/stdfile.c
    src/nsmain.m
    src/aunit.c
    src/GameRenderer.m
    src/GameShaders.c
  )
endif()

# X11 OpenGL
if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_FREEBSD
   OR STRATO_TARGET_NETBSD
   OR STRATO_TARGET_OPENBSD)
  set(STRATO_SOURCES
      src/image.c
      src/glyph.c
      src/wave.c
      src/stdfile.c
      src/x11main.c
      src/icon.c
      src/glrender.c
      src/asound.c
      src/bsdsound.c
      src/nosound.c
      src/evgamepad.c
      src/gstplay.c
    )
endif()

# X11 Soft3D
if(   STRATO_TARGET_LINUX_X11SOFT
   OR STRATO_TARGET_FREEBSD_X11SOFT
   OR STRATO_TARGET_NETBSD_X11SOFT
   OR STRATO_TARGET_OPENBSD_X11SOFT
   OR STRATO_TARGET_SOLARIS11
   OR STRATO_TARGET_GENERICUNIX)
  set(STRATO_SOURCES
      src/image.c
      src/glyph.c
      src/wave.c
      src/stdfile.c
      src/asound.c
      src/bsdsound.c
      src/evgamepad.c
      src/x11softmain.c
      src/icon.c
      src/gstplay.c
    )
endif()

# Wayland
if(   STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_FREEBSD_WAYLAND)
  set(STRATO_SOURCES
      src/image.c
      src/glyph.c
      src/wave.c
      src/stdfile.c
      src/wlmain.c
      src/glrender.c
      src/asound.c
      src/evgamepad.c
    )
endif()

# GBM
if(STRATO_TARGET_LINUX_GBM)
  set(STRATO_SOURCES
      src/image.c
      src/glyph.c
      src/wave.c
      src/stdfile.c
      src/gbmmain.c
      src/glrender.c
      src/asound.c
      src/evgamepad.c
    )
endif()

# Fbdev
if(STRATO_TARGET_LINUX_FBDEV)
  set(STRATO_SOURCES
      src/image.c
      src/glyph.c
      src/wave.c
      src/stdfile.c
      src/fbmain.c
      src/asound.c
      src/evgamepad.c
    )
endif()

#  Emscripten
if(STRATO_TARGET_WASM)
  set(STRATO_SOURCES
    src/image.c
    src/glyph.c
    src/wave.c
    src/stdfile.c
    src/emmain.c
    src/alsound.c
    src/glrender.c
  )
endif()

# Emscripten Local
if(STRATO_TARGET_WASM_LOCAL)
  set(STRATO_SOURCES
    src/image.c
    src/glyph.c
    src/wave.c
    src/emfile.c
    src/emlocalmain.c
    src/alsound.c
    src/glrender.c
  )
endif()

# iOS
if(STRATO_TARGET_IOS)
  enable_language(OBJC)
  set(STRATO_SOURCES
    src/image.c
    src/glyph.c
    src/wave.c
    src/stdfile.c
    src/uimain.m
    src/aunit.c
    src/GameRenderer.m
    src/GameShaders.c
  )
endif()

# Android
if(STRATO_TARGET_ANDROID)
  set(STRATO_SOURCES
    src/image.c
    src/glyph.c
    src/wave.c
    src/glrender.c
    src/slsound.c
    src/ndkmain.c
    src/ndkfile.c
  )
endif()

# HarmonyOS NEXT / OpenHarmony
if(STRATO_TARGET_OPENHARMONY)
  set(STRATO_SOURCES
    src/image.c
    src/glyph.c
    src/wave.c
    src/glrender.c
    src/slsound.c
    src/ohmain.cpp
    src/ohfile.cpp
  )
endif()

# Unity
if(STRATO_TARGET_UNITY)
  set(STRATO_SOURCES
    src/image.c
    src/glyph.c
    src/wave.c
    src/halwrap.c
  )
endif()

# GDK
if(   STRATO_TARGET_GDK_WINDOWS
   OR STRATO_TARGET_GDK_XBOX_XS)
  set(STRATO_SOURCES
    src/image.c
    src/glyph.c
    src/wave.c
    src/gdkmain.cpp
    src/gdkfile.cpp
    src/d3d12render.cc
    src/gigamepad.cpp
    src/xa2sound.cpp
  )
endif()

# Haiku
if(STRATO_TARGET_HAIKU)
  set(STRATO_SOURCES
    src/image.c
    src/glyph.c
    src/wave.c
    src/stdfile.c
    src/bemain.cc
  )
endif()

# UWP
if(STRATO_TARGET_UWP)
  set(STRATO_SOURCES
    src/image.c
    src/glyph.c
    src/wave.c
    src/uwpmain.cpp
    src/uwpfile.cpp
    src/d3d12render.cc
  )
endif()

# I18N
if(STRATO_ENABLE_I18N)
  set(STRATO_I18N_SOURCES
    src/translation.c
  )
endif()

# Deps (merge third-party library objects into the target)
if(NOT STRATO_ENABLE_DIST)
  set(STRATO_DEPS
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

# Library type.
if(STRATO_ENABLE_STATIC)
  set(STRATO_LIB_TYPE STATIC)
elseif(STRATO_ENABLE_SHARED)
  set(STRATO_LIB_TYPE SHARED)
elseif(STRATO_ENABLE_OBJECT)
  set(STRATO_LIB_TYPE OBJECT)
endif()

# Target.
add_library(
  strato
  ${STRATO_LIB_TYPE}
  ${STRATO_SOURCES}
  ${STRATO_I18N_SOURCES}
  ${STRATO_DEPS}
)

# ---

#
# CPPFLAGS
#

target_include_directories(strato PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(strato PRIVATE ${PNG_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${JPEG_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${WEBP_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${OGG_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${VORBIS_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${BROTLI_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${BZIP2_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${ZLIB_INCLUDE_DIRS})

if(STRATO_ENABLE_SHARED OR STRATO_ENABLE_DIST)
  target_compile_definitions(strato PRIVATE HAL_USE_SHARED)
endif()

if(STRATO_ENABLE_JIT)
  target_compile_definitions(strato PRIVATE USE_JIT)
endif()

# Windows
if(   STRATO_TARGET_WINDOWS
   OR STRATO_TARGET_WINDOWS95
   OR STRATO_TARGET_GDK_WINDOWS
   OR STRATO_TARGET_GDK_XBOX_XS)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

  target_compile_definitions(strato PRIVATE HAL_USE_MFVIDEO HAL_USE_DSVIDEO)

  target_include_directories(
    strato
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/res
    ${CMAKE_BINARY_DIR}/dx12headers/include/directx
    ${CMAKE_BINARY_DIR}/dx12headers/include/dxguids
  )

  # DX12 header
  if(STRATO_TARGET_WINDOWS OR STRATO_TARGET_WINDOWS95)
    file(ARCHIVE_EXTRACT
      INPUT       ${CMAKE_CURRENT_SOURCE_DIR}/lib/archive/dx12headers.tar.gz
      DESTINATION ${CMAKE_BINARY_DIR}
    )
  endif()
endif()

# macOS library
if(STRATO_TARGET_MACOS)
  if(STRATO_ENABLE_MAIN2)
    target_compile_definitions(strato PUBLIC HAL_USE_MAIN2)
  endif()
endif()

# FreeBSD
if(STRATO_TARGET_FREEBSD)
  target_include_directories(strato PUBLIC /usr/local/include)
endif()

# NetBSD
if(STRATO_TARGET_NETBSD)
  target_include_directories(strato PUBLIC /usr/local/include /usr/X11R7/include)
endif()

# OpenBSD
if(STRATO_TARGET_OPENBSD)
  target_include_directories(strato PUBLIC /usr/local/include /usr/X11R6/include)
endif()

# Solaris11
if(STRATO_TARGET_SOLARIS11)
  target_include_directories(strato PUBLIC /usr/local/include /usr/X11R6/include)
endif()

# Generic Unknown UNIX
if(STRATO_TARGET_GENERICUNIX)
  target_include_directories(
    strato
    PUBLIC
    /usr/local/include
    /usr/pkg/include
    /usr/pkgsrc/include
    /usr/X11R4/include
    /usr/X11R5/include
    /usr/X11R6/include
    /usr/X11R7/include
  )
endif()

# OpenHarmony
if(STRATO_TARGET_OPENHARMONY)
  target_compile_definitions(strato PUBLIC HAL_TARGET_OPENHARMONY)
endif()

# Unity
if(STRATO_TARGET_UNITY)
  target_compile_definitions(strato PUBLIC HAL_TARGET_UNITY HAL_USE_CSHARP)
  if(STRATO_TARGET_UNITY_SWITCH)
    target_compile_definitions(strato PRIVATE HAL_UNITY_SWITCH)
  elseif(STRATO_TARGET_UNITY_PS5)
    target_compile_definitions(strato PRIVATE HAL_UNITY_PS5)
  elseif(STRATO_TARGET_UNITY_XBOX)
    target_compile_definitions(strato PRIVATE HAL_UNITY_GAMECORE_XBOXSERIES)
  endif()
endif()

# GLES
if(   STRATO_TARGET_LINUX_GBM
   OR STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_FREEBSD_WAYLAND
   OR STRATO_TARGET_ANDROID
   OR STRATO_TARGET_OPENHARMONY)
  target_compile_definitions(strato PRIVATE HAL_USE_GLES)
  target_compile_definitions(strato PUBLIC HAL_USE_GLES)
endif()

# Wayland
if(   STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_FREEBSD_WAYLAND)
  target_compile_definitions(strato PRIVATE HAL_USE_WAYLAND)
endif()

# GBM
if(STRATO_TARGET_LINUX_GBM)
  target_include_directories(strato PRIVATE /usr/include/libdrm)
endif()

# fbdev
if(STRATO_TARGET_LINUX_FBDEV)
  target_include_directories(strato PRIVATE /usr/include/libdrm)
endif()

# GDK
if(STRATO_TARGET_GDK_WINDOWS)
  target_compile_definitions(strato PRIVATE HAL_TARGET_GDK_WINDOWS)
endif()
if(STRATO_TARGET_GDK_XBOX_XS)
  target_compile_definitions(strato PRIVATE HAL_TARGET_GDK_XBOX_XS)
endif()

# UWP
if(STRATO_TARGET_UWP)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS BZ_STRICT_ANSI)
  target_compile_definitions(strato PRIVATE HAL_TARGET_UWP)
  file(ARCHIVE_EXTRACT
    INPUT       ${CMAKE_CURRENT_SOURCE_DIR}/lib/archive/dx12headers.tar.gz
    DESTINATION ${CMAKE_BINARY_DIR}
  )
endif()

# Gstreamer
if(STRATO_ENABLE_GST)
  target_compile_definitions(strato PRIVATE HAL_USE_GSTREAMER)
endif()

# Translation
if(STRATO_ENABLE_I18N)
  target_compile_definitions(strato PRIVATE HAL_USE_TRANSLATION)
endif()

# Rotation
if(STRATO_ENABLE_ROT90)
  target_compile_definitions(strato PRIVATE HAL_USE_ROT90)
endif()

# Software rendering.
if(   STRATO_TARGET_LINUX_X11SOFT
   OR STRATO_TARGET_FREEBSD_X11SOFT
   OR STRATO_TARGET_NETBSD_X11SOFT
   OR STRATO_TARGET_OPENBSD_X11SOFT
   OR STRATO_TARGET_SOLARIS11
   OR STRATO_TARGET_GENERICUNIX)
  target_compile_definitions(strato PUBLIC HAL_USE_X11_SOFTRENDER)
endif()

# ---

#
# CLFAGS
#

# Windows
if(   STRATO_TARGET_WINDOWS
   OR STRATO_TARGET_WINDOWS95
   OR STRATO_TARGET_GDK_WINDOWS
   OR STRATO_TARGET_GDK_XBOX_XS)
  # /utf-8
  if(MSVC)
    target_compile_options(strato PUBLIC /utf-8)
    target_compile_definitions(strato PUBLIC UNICODE _UNICODE)
  endif()

  # -municode
  if(MINGW)
    target_compile_options(strato PUBLIC -municode)
    target_compile_definitions(strato PUBLIC _UNICODE UNICODE)
  endif()
endif()
if(STRATO_TARGET_WINDOWS95)
  # /utf-8
  if(MSVC)
    target_compile_options(strato PUBLIC /utf-8)
  endif()
  if(MINGW)
    add_definitions(-UUNICODE -U_UNICODE)
  endif()
endif()

# Apple
if(STRATO_TARGET_MACOS OR STRATO_TARGET_IOS)
  target_compile_options(strato PUBLIC -fmodules)
endif()

# ---

#
# LDFLAGS
#

# Libraries.
if(STRATO_ENABLE_SHARED OR STRATO_ENABLE_DIST)
  target_link_libraries(
    strato
    PUBLIC
    ${PNG_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${WEBP_LIBRARIES}
    ${VORBIS_LIBRARY}
    ${VORBISFILE_LIBRARY}
    ${OGG_LIBRARY}
    ${FREETYPE_LIBRARY}
    ${BROTLIDEC_LIBRARIES}
    ${BROTLICOMMON_LIBRARIES}
    ${BZIP2_LIBRARIES}
    ${ZLIB_LIBRARIES}
  )
endif()

# Windows
if(   STRATO_TARGET_WINDOWS
   OR STRATO_TARGET_WINDOWS95)
  target_link_libraries(strato PUBLIC
    gdi32
    ole32
    dxguid
  )

  if(MINGW)
    target_link_options(strato PUBLIC -mwindows)
  endif()

  if(MINGW AND STRATO_TARGET_WINDOWS)
    target_link_options(strato PUBLIC -municode)
  endif()

  if(MSVC)
    target_link_options(strato PUBLIC
      /SUBSYSTEM:WINDOWS
      /ENTRY:wWinMainCRTStartup
      /MANIFEST:NO
    )
  endif()
endif()

# macOS
if(STRATO_TARGET_MACOS)
  target_link_libraries(strato PUBLIC
    "-framework AppKit"
    "-framework MetalKit"
    "-framework AVFoundation"
  )
endif()

# X11
if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_LINUX_X11SOFT
   OR STRATO_TARGET_FREEBSD
   OR STRATO_TARGET_FREEBSD_X11SOFT
   OR STRATO_TARGET_NETBSD
   OR STRATO_TARGET_NETBSD_X11SOFT
   OR STRATO_TARGET_OPENBSD
   OR STRATO_TARGET_OPENBSD_X11SOFT
   OR STRATO_TARGET_SOLARIS11
   OR STRATO_TARGET_GENERICUNIX)
  target_link_libraries(strato PUBLIC m pthread Xpm X11)
endif()

# OpenGL
if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_FREEBSD
   OR STRATO_TARGET_NETBSD
   OR STRATO_TARGET_OPENBSD
   OR STRATO_TARGET_SOLARIS11)
  target_link_libraries(strato PUBLIC GL ${GLX_LIBRARY})
endif()

# GLX
if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_FREEBSD)
  target_link_libraries(strato PUBLIC ${GLX_LIBRARY})
endif()

# GStreamer
if(STRATO_ENABLE_GST)
  target_link_libraries(strato PUBLIC PkgConfig::GST)
endif()

# Wayland
if(   STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_FREEBSD_WAYLAND)
  target_link_libraries(
    strato
    PUBLIC
    m
    pthread
    asound
    ${WAYLAND_LIBRARIES}
    ${EGL_LIBRARIES}
    ${GLES2_LIBRARIES}
  )
endif()

# GBM
if(STRATO_TARGET_LINUX_GBM)
  target_link_libraries(
    strato
    PUBLIC
    m
    pthread
    gbm
    drm
    EGL
    GLESv2
  )
endif()

# fbdev
if(STRATO_TARGET_LINUX_FBDEV)
  target_link_libraries(
    strato
    PUBLIC
    m
    pthread
    asound
  )
  target_include_directories(strato PUBLIC /usr/include/libdrm)
endif()

# Linux
if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_LINUX_GBM
   OR STRATO_TARGET_LINUX_FBDEV
   OR STRATO_TARGET_LINUX_X11SOFT)
  target_link_libraries(strato PUBLIC asound)
endif()

# FreeBSD
if(   STRATO_TARGET_FREEBSD
   OR STRATO_TARGET_FREEBSD_WAYLAND)
  target_link_directories(strato PUBLIC /usr/local/lib /usr/X11R7/lib)
endif()

# NetBSD
if(   STRATO_TARGET_NETBSD
   OR STRATO_TARGET_NETBSD_X11SOFT)
  target_link_directories(strato PUBLIC /usr/local/lib /usr/X11R7/lib)
endif()

# OpenBSD
if(   STRATO_TARGET_OPENBSD
   OR STRATO_TARGET_OPENBSD_X11SOFT)
  target_link_directories(strato PUBLIC /usr/local/lib /usr/X11R6/lib)
endif()

# Android
if(STRATO_TARGET_ANDROID)
  target_link_libraries(strato PUBLIC -lm -lGLESv3 -lOpenSLES -llog)
endif()

# OpenHarmony
if(STRATO_TARGET_OPENHARMONY)
  target_link_libraries(strato PUBLIC m GLESv2 OpenSLES librawfile.z.so)
endif()

# Solaris11
if(STRATO_TARGET_SOLARIS11)
  target_link_directories(strato PUBLIC /usr/local/lib /usr/X11R6/lib)
endif()

# Generic Unknown UNIX
if(STRATO_TARGET_GENERICUNIX)
  target_link_directories(
    strato
    PUBLIC
    /usr/local/lib
    /usr/pkg/lib
    /usr/pkgsrc/lib
    /usr/X11R4/lib
    /usr/X11R5/lib
    /usr/X11R6/lib
    /usr/X11R7/lib
  )
endif()

# GDK
if(   STRATO_TARGET_GDK_WINDOWS
   OR STRATO_TARGET_GDK_XBOX_XS)

  set(_GDK_ROOT "$ENV{GameDKCoreLatest}")
  if(NOT _GDK_ROOT)
    set(_GDK_ROOT "$ENV{GRDKLatest}")
  endif()

  if(NOT _GDK_ROOT)
    message(FATAL_ERROR "GDK env var not found. Set GameDKCoreLatest or GRDKLatest.")
  endif()

  set(_GDK_GAMERUNTIME_LIB_DIR "${_GDK_ROOT}GRDK\\GameKit\\Lib\\amd64")
  if(NOT IS_DIRECTORY "${_GDK_GAMERUNTIME_LIB_DIR}")
    message(FATAL_ERROR "Not a directory: '${_GDK_GAMERUNTIME_LIB_DIR}'")
  endif()
  file(GLOB _gdk_libs "${_GDK_GAMERUNTIME_LIB_DIR}/*.lib")
  list(FILTER _gdk_libs INCLUDE REGEX "xgame.*runtime.*\\.lib$")
  message(STATUS "GDK runtime libs:\n  ${_gdk_libs}")

  set(XGAMERUNTIME_LIB "${_GDK_GAMERUNTIME_LIB_DIR}/xgameruntime.lib")
  if(NOT EXISTS "${XGAMERUNTIME_LIB}")
    message(FATAL_ERROR "Not found: ${XGAMERUNTIME_LIB}")
  endif()

  target_link_libraries(strato PUBLIC "${XGAMERUNTIME_LIB}" user32 advapi32 gdi32 ole32 uuid appnotify gameinput xaudio2)

  if(MSVC)
    target_link_options(strato PUBLIC
      /SUBSYSTEM:WINDOWS
      /ENTRY:wWinMainCRTStartup
      /MANIFEST:NO
    )
  endif()
endif()

# Haiku
if(STRATO_TARGET_HAIKU)
  target_link_libraries(strato PUBLIC be media)
endif()

# UWP
if(STRATO_TARGET_UWP)
  target_link_libraries(strato PUBLIC windowsapp d3d12 dxgi dxguid xaudio2 gameinput)
endif()

# ---

#
# Archiver Target
#

if(STRATO_ENABLE_PACK)
  add_library(
    stratopack
    STATIC
    src/archive.c
  )
  target_include_directories(stratopack PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

# ---

#
# Wayland Custom Process
#

if(   STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_FREEBSD_WAYLAND)
  find_program(WAYLAND_SCANNER NAMES wayland-scanner REQUIRED)

  set(XDG_SHELL_XML
      /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml)
  set(XDG_DECORATION_XML
      /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml)

  # generate xdg-shell sources
  set(GEN_SHELL_HDR ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h)
  set(GEN_SHELL_SRC ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c)
  add_custom_command(
    OUTPUT ${GEN_SHELL_HDR} ${GEN_SHELL_SRC}
    COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL_XML} ${GEN_SHELL_HDR}
    COMMAND ${WAYLAND_SCANNER} private-code ${XDG_SHELL_XML} ${GEN_SHELL_SRC}
    DEPENDS ${XDG_SHELL_XML}
  )

  # generate xdg-decoration sources
  set(GEN_DECORATION_HDR ${CMAKE_CURRENT_BINARY_DIR}/zxdg-decoration-unstable-v1-client-protocol.h)
  set(GEN_DECORATION_SRC ${CMAKE_CURRENT_BINARY_DIR}/zxdg-decoration-unstable-v1-protocol.c)
  add_custom_command(
    OUTPUT ${GEN_DECORATION_HDR} ${GEN_DECORATION_SRC}
    COMMAND ${WAYLAND_SCANNER} client-header ${XDG_DECORATION_XML} ${GEN_DECORATION_HDR}
    COMMAND ${WAYLAND_SCANNER} private-code ${XDG_DECORATION_XML} ${GEN_DECORATION_SRC}
    DEPENDS ${XDG_DECORATION_XML}
  )

  # add both generated sources
  target_sources(strato PRIVATE
    ${GEN_SHELL_SRC} ${GEN_SHELL_HDR}
    ${GEN_DECORATION_SRC} ${GEN_DECORATION_HDR}
  )
  target_include_directories(strato PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()
