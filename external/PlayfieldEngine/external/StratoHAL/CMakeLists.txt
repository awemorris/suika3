cmake_minimum_required(VERSION 3.13)
project(StratoHAL LANGUAGES C CXX)

#
# Configuration
#

# Target
option(STRATO_TARGET_WINDOWS          "Build for Windows 2000 or later"    OFF)
option(STRATO_TARGET_WINDOWS95        "Build for Windows 95/98/Me"         OFF)
option(STRATO_TARGET_MACOS            "Build for macOS"                    OFF)
option(STRATO_TARGET_LINUX            "Build for Linux X11"                OFF)
option(STRATO_TARGET_LINUX_WAYLAND    "Build for Linux Wayland"            OFF)
option(STRATO_TARGET_LINUX_GBM        "Build for Linux GBM Framebuffer"    OFF)
option(STRATO_TARGET_LINUX_FBDEV      "Build for Linux fbdev Framebuffer"  OFF)
option(STRATO_TARGET_LINUX_X11SOFT    "Build for Linux X11 Soft Render"    OFF)
option(STRATO_TARGET_IOS              "Build for iOS"                      OFF)
option(STRATO_TARGET_ANDROID          "Build for Android"                  OFF)
option(STRATO_TARGET_OPENHARMONY      "Build for OpenHarmony"              OFF)
option(STRATO_TARGET_WASM             "Build for Emscripten"               OFF)
option(STRATO_TARGET_WASM_LOCAL       "Build for Emscripten (local file)"  OFF)
option(STRATO_TARGET_UNITY            "Build for Unity"                    OFF)
option(STRATO_TARGET_FREEBSD          "Build for FreeBSD X11"              OFF)
option(STRATO_TARGET_FREEBSD_WAYLAND  "Build for FreeBSD Wayland"          OFF)
option(STRATO_TARGET_NETBSD           "Build for NetBSD"                   OFF)
option(STRATO_TARGET_OPENBSD          "Build for OpenBSD"                  OFF)
option(STRATO_TARGET_GDK_WINDOWS      "Build for GDK for Desktop"          OFF)
option(STRATO_TARGET_GDK_XBOX_XS      "Build for GDK for Xbox Series X|S"  OFF)
option(STRATO_TARGET_UWP              "Build for UWP"                      OFF)

# Options
option(STRATO_ENABLE_I18N             "Enable translation"                 OFF)
option(STRATO_ENABLE_OBJECT           "Build an object library"            OFF)
option(STRATO_ENABLE_SHARED           "Use shared libraries"               OFF)
option(STRATO_ENABLE_ROT90            "Rotate screen"                      OFF)
option(STRATO_ENABLE_MEDIAFOUNDATION  "Enable Media Foundation support"    OFF)
option(STRATO_ENABLE_GST              "Enable GStreamer support"           OFF)

#
# Automatic Target Detection
#

# For when a target is manually specified.
if(   STRATO_TARGET_WINDOWS
   OR STRATO_TARGET_WINDOWS95
   OR STRATO_TARGET_MACOS
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
   OR STRATO_TARGET_NETBSD
   OR STRATO_TARGET_OPENBSD
   OR STRATO_TARGET_GDK_WINDOWS
   OR STRATO_TARGET_GDK_XBOX_XS
   OR STRATO_TARGET_UWP
)
  # A target and options are manually specified.
else()
  if(WIN32)
    set(STRATO_TARGET_WINDOWS    ON)
  endif()
  if(APPLE AND NOT IOS)
    set(STRATO_TARGET_MACOS      ON)
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(STRATO_TARGET_LINUX      ON)
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(STRATO_TARGET_FREEBSD    ON)
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    set(STRATO_TARGET_NETBSD     ON)
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    set(STRATO_TARGET_OPENBSD    ON)
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

#
# Dependency
#

if(STRATO_ENABLE_SHARED)
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
  # Build the dependencies.
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

if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_LINUX_X11SOFT
   OR STRATO_TARGET_FREEBSD
   OR STRATO_TARGET_NETBSD
   OR STRATO_TARGET_OPENBSD)
  find_library(X11_LIBRARY X11 REQUIRED)
  find_library(XPM_LIBRARY Xpm REQUIRED)
  find_library(GL_LIBRARY GL REQUIRED)
  if(STRATO_TARGET_LINUX OR STRATO_TARGET_FREEBSD)
    find_library(GLX_LIBRARY GLX)
  endif()

  if(STRATO_ENABLE_GST)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GST REQUIRED IMPORTED_TARGET gstreamer-1.0 gstreamer-video-1.0)
  endif()
endif()

if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_LINUX_X11SOFT)
  find_library(ALSA_LIBRARY asound REQUIRED)
endif()

if(STRATO_TARGET_LINUX_GBM)
  find_library(GBM_LIBRARY gbm REQUIRED)
  find_library(DRM_LIBRARY drm REQUIRED)
endif()

if(   STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_FREEBSD_WAYLAND)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-egl)
  pkg_check_modules(EGL     REQUIRED egl)
  pkg_check_modules(GLES2   REQUIRED glesv2)
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
# Main Target
#

# For Windows.
if(   STRATO_TARGET_WINDOWS
   OR STRATO_TARGET_WINDOWS95)
  if(STRATO_ENABLE_MEDIAFOUNDATION)
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
      src/digamepad.cc
      src/xigamepad.c
    )
  else()
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
      src/dsvideo.cc
      src/digamepad.cc
      src/xigamepad.c
    )
  endif()
endif()

# For macOS.
if(STRATO_TARGET_MACOS)
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

# For Linux and *BSD. (X11)
if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_FREEBSD
   OR STRATO_TARGET_NETBSD
   OR STRATO_TARGET_OPENBSD
)
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

# For Linux Wayland.
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

# For Linux GBM Framebuffer.
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

# For Linux fbdev Framebuffer.
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

# For Linux X11 Software Rendering.
if(STRATO_TARGET_LINUX_X11SOFT)
  set(STRATO_SOURCES
      src/image.c
      src/glyph.c
      src/wave.c
      src/stdfile.c
      src/asound.c
      src/evgamepad.c
      src/x11softmain.c
      src/icon.c
      src/gstplay.c
    )
endif()

# For Emscripten.
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

# For Emscripten (local file).
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

# For iOS.
if(STRATO_TARGET_IOS)
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

# For Android.
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

# For OpenHarmony.
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

# For Unity.
if(STRATO_TARGET_UNITY)
  set(STRATO_SOURCES
    src/image.c
    src/glyph.c
    src/wave.c
    src/halwrap.c
  )
endif()

# For GDK (Windows or Xbox).
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

# For UWP.
if(STRATO_TARGET_UWP)
  set(STRATO_SOURCES
    src/image.c
    src/glyph.c
    src/wave.c
    src/uwpmain.cpp
    src/d3d12render.cc
  )
endif()

# For translation.
if(STRATO_ENABLE_I18N)
  set(STRATO_I18N_SOURCES
    src/translation.c
  )
endif()

if(STRATO_ENABLE_SHARED)
  add_library(strato SHARED
    ${STRATO_SOURCES}
  )
elseif(STRATO_ENABLE_OBJECT)
  add_library(strato OBJECT
    ${STRATO_SOURCES}
    ${STRATO_I18N_SOURCES}
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
else()
  add_library(strato STATIC
    ${STRATO_SOURCES}
    ${STRATO_I18N_SOURCES}
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

# Windows
if(   STRATO_TARGET_WINDOWS
   OR STRATO_TARGET_WINDOWS95
   OR STRATO_TARGET_GDK_WINDOWS
   OR STRATO_TARGET_GDK_XBOX_XS)
  enable_language(RC)
  set(CMAKE_CXX_STANDARD 98)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  file(ARCHIVE_EXTRACT
    INPUT       ${CMAKE_CURRENT_SOURCE_DIR}/lib/archive/dx12headers.tar.gz
    DESTINATION ${CMAKE_BINARY_DIR}
  )
endif()

# macOS
if(STRATO_TARGET_MACOS)
  enable_language(OBJC)
endif()

# Linux
if(  STRATO_TARGET_LINUX
  OR STRATO_TARGET_LINUX_WAYLAND
  OR STRATO_TARGET_LINUX_GBM
  OR STRATO_TARGET_LINUX_FBDEV
  OR STRATO_TARGET_LINUX_X11SOFT)
endif()

# iOS
if(STRATO_TARGET_IOS)
endif()

# Android
if(STRATO_TARGET_ANDROID)
endif()

# OpenHarmony
if(STRATO_TARGET_OPENHARMONY)
  target_compile_definitions(strato PUBLIC HAL_TARGET_OPENHARMONY HAL_USE_GLES)
endif()

# *BSD
if(STRATO_TARGET_FREEBSD)
  target_compile_definitions(strato PUBLIC HAL_TARGET_FREEBSD)
endif()
if(STRATO_TARGET_NETBSD)
  target_compile_definitions(strato PUBLIC HAL_TARGET_NETBSD)
endif()
if(STRATO_TARGET_OPENBSD)
  target_compile_definitions(strato PUBLIC HAL_TARGET_OPENBSD)
endif()

# For Unity
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

# For GDK (Windows or Xbox).
if(STRATO_TARGET_GDK_WINDOWS)
  target_compile_definitions(strato PRIVATE HAL_TARGET_GDK_WINDOWS)
endif()
if(STRATO_TARGET_GDK_XBOX_XS)
  target_compile_definitions(strato PRIVATE HAL_TARGET_GDK_XBOX_XS)
endif()

# For UWP.
if(STRATO_TARGET_UWP)
  target_compile_definitions(strato PRIVATE HAL_TARGET_UWP)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  file(ARCHIVE_EXTRACT
    INPUT       ${CMAKE_CURRENT_SOURCE_DIR}/lib/archive/dx12headers.tar.gz
    DESTINATION ${CMAKE_BINARY_DIR}
  )
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  add_compile_definitions(BZ_STRICT_ANSI)
endif()

# For translation.
if(STRATO_ENABLE_I18N)
  target_compile_definitions(strato PRIVATE HAL_USE_TRANSLATION)
endif()

#
# Archiver Target
#

if(    NOT STRATO_TARGET_WASM
   AND NOT STRATO_TARGET_ANDROID
   AND NOT STRATO_TARGET_IOS
   AND NOT STRATO_TARGET_UNITY
)
  add_library(
    stratopack
    STATIC
    src/archive.c
  )
  target_include_directories(stratopack PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

#
# CPPFLAGS
#

# -Dinclude
target_include_directories(strato PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include)

# -DHAL_USE_SHARED if we use dynamic linking.
if(STRATO_ENABLE_SHARED)
  target_compile_definitions(strato PRIVATE HAL_USE_SHARED)
endif()

# -DUSE_JIT if we use JIT.
if(STRATO_ENABLE_JIT)
  target_compile_definitions(strato PRIVATE USE_JIT)
endif()

# Add library header directories.
target_include_directories(strato PRIVATE ${PNG_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${JPEG_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${WEBP_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${OGG_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${VORBIS_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${BROTLI_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${BZIP2_INCLUDE_DIRS})
target_include_directories(strato PRIVATE ${ZLIB_INCLUDE_DIRS})

# Windows: Add -Ires -Idx12headers/include/directx -Idx12headers/include/fxguids
if(   STRATO_TARGET_WINDOWS
   OR STRATO_TARGET_WINDOWS95)
  target_include_directories(strato PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/res
    ${CMAKE_BINARY_DIR}/dx12headers/include/directx
    ${CMAKE_BINARY_DIR}/dx12headers/include/dxguids
  )

  if(STRATO_ENABLE_MEDIAFOUNDATION)
    target_compile_definitions(strato PRIVATE HAL_USE_MFVIDEO)
  else()
    target_compile_definitions(strato PRIVATE HAL_USE_DSVIDEO)
  endif()
endif()

# Linux X11: Add -DHAS_USE_GL -DHAL_USE_X11 -DHAL_GSTREAMER
if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_FREEBSD)
  target_compile_definitions(strato PRIVATE HAL_USE_GL HAL_USE_X11)

  if(STRATO_ENABLE_GST)
    target_compile_definitions(strato PRIVATE HAL_USE_GSTREAMER)
  endif()
endif()

# Linux Wayland: Add -DHAS_USE_GLES -DHAL_USE_WAYLAND
if(   STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_FREEBSD_WAYLAND)
  target_compile_definitions(strato PRIVATE HAL_USE_GLES HAL_USE_WAYLAND HAL_USE_GSTREAMER)
endif()

# Linux Framebuffer: Add -DHAL_USE_GLES
if(STRATO_TARGET_LINUX_GBM)
  target_compile_definitions(strato PRIVATE HAL_USE_GLES)
endif()

# Linux GBM Framebuffer
if(STRATO_TARGET_LINUX_GBM AND STRATO_ENABLE_ROT90)
  target_compile_definitions(strato PRIVATE HAL_USE_ROT90)
endif()

# Linux fbdev Framebuffer
if(STRATO_TARGET_LINUX_FBDEV AND STRATO_ENABLE_ROT90)
  target_compile_definitions(strato PRIVATE HAL_USE_ROT90)
endif()

# FreeBSD: Add -I/usr/local/include
if(STRATO_TARGET_FREEBSD)
  target_include_directories(strato PUBLIC /usr/local/include)
endif()

# NetBSD: Add -I/usr/local/include -I/usr/X11R7/include
if(STRATO_TARGET_NETBSD)
  target_include_directories(strato PUBLIC /usr/local/include /usr/X11R7/include)
endif()

# OpenBSD: Add -I/usr/local/include -I/usr/X11R6/include
if(STRATO_TARGET_OPENBSD)
  target_include_directories(strato PUBLIC /usr/local/include /usr/X11R6/include)
endif()

# Android: Add -DHAL_USE_GLES
if(STARATO_TARGET_ANDROID)
  target_compile_definitions(strato PRIVATE HAL_USE_GLES)
endif()

# OpenHarmony: Add -DHAL_USE_GLES
if(STARATO_TARGET_OPENHARMONY)
  target_compile_definitions(strato PRIVATE HAL_USE_GLES)
endif()

#
# CFLAGS
#

# macOS and iOS
if(STRATO_TARGET_MACOS OR STRATO_TARGET_IOS)
  target_compile_options(strato PUBLIC -fmodules)
endif()

# Windows compile options.
if(   STRATO_TARGET_WINDOWS
   OR STRATO_TARGET_GDK_WINDOWS
   OR STRATO_TARGET_GDK_XBOX_XS)
  # -municode
  if(MINGW)
    target_compile_options(strato PUBLIC -municode)
  endif()

  # /utf-8
  if(MSVC)
    target_compile_options(strato PUBLIC /utf-8)
    target_compile_definitions(strato PUBLIC UNICODE _UNICODE)
  endif()
endif()
if(STRATO_TARGET_WINDOWS95)
  # /utf-8
  if(MSVC)
    target_compile_options(strato PUBLIC /utf-8)
  endif()
endif()

#
# Link
#

# Libraries.
if(STRATO_ENABLE_SHARED)
  target_link_libraries(strato PUBLIC
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

# Windows link options.
if(   STRATO_TARGET_WINDOWS
   OR STRATO_TARGET_WINDOWS95)
  # DLLs.
  target_link_libraries(strato PUBLIC
    gdi32
    ole32
    dsound
    strmiids
    dxguid
  )

  if(STRATO_ENABLE_MEDIAFOUNDATION)
    target_link_libraries(strato PUBLIC mf mfplat mfuuid mfreadwrite evr)
  else()
    target_link_libraries(strato PUBLIC strmiids)
  endif()

  # -mwindows -municode
  if(MINGW)
    target_link_options(strato PUBLIC -mwindows)
    if(STRATO_TARGET_WINDOWS)
      target_link_options(strato PUBLIC -municode)
    endif()
  endif()

  # /SUBSYSTEM:WINDOWS
  if(MSVC)
    target_link_options(strato PUBLIC
      /SUBSYSTEM:WINDOWS
      /ENTRY:wWinMainCRTStartup
      /MANIFEST:NO
    )
  endif()
endif()

# macOS link options.
if(STRATO_TARGET_MACOS)
  target_link_libraries(strato PUBLIC
    "-framework AppKit"
    "-framework MetalKit"
    "-framework AVFoundation"
  )
endif()

# Linux and *BSD. (X11)
if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_LINUX_X11SOFT
   OR STRATO_TARGET_FREEBSD
   OR STRATO_TARGET_NETBSD
   OR STRATO_TARGET_OPENBSD
)
  # Add -lm -lpthread -lXpm -lX11 -lGL -lGLX
  target_link_libraries(
    strato
    PUBLIC
    m
    pthread
    Xpm
    X11
    GL
    ${GLX_LIBRARY}  # We need this on systems with older Mesa.
  )

  # GStreamer
  if(STRATO_ENABLE_GST)
    target_link_libraries(strato PUBLIC PkgConfig::GST)
  endif()
endif()

# Linux Wayland.
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

# Linux GBM framebuffer.
if(STRATO_TARGET_LINUX_GBM)
  # Add -lm -lpthread -lgbm -ldrm -lEGL -lGLESv2 -lasound
  target_link_libraries(
    strato
    PUBLIC
    m
    pthread
    gbm
    drm
    EGL
    GLESv2
    asound
  )
  target_include_directories(strato PUBLIC /usr/include/libdrm)
endif()

# Linux GBM framebuffer.
if(STRATO_TARGET_LINUX_FBDEV)
  # Add -lm -lpthread -lasound
  target_link_libraries(
    strato
    PUBLIC
    m
    pthread
    asound
  )
  target_include_directories(strato PUBLIC /usr/include/libdrm)
endif()

# Linux X11: Add -lasound
if(   STRATO_TARGET_LINUX
   OR STRATO_TARGET_LINUX_WAYLAND
   OR STRATO_TARGET_LINUX_GBM
   OR STRATO_TARGET_LINUX_FBDEV
   OR STRATO_TARGET_LINUX_X11SOFT)
  target_link_libraries(strato PUBLIC asound)
endif()

# FreeBSD: Add -L/usr/local/lib -L/usr/X11R7/lib
if(STRATO_TARGET_FREEBSD)
  target_link_directories(strato PUBLIC /usr/local/lib /usr/X11R7/lib)
endif()

# NetBSD: Add -L/usr/local/lib -L/usr/X11R7/lib
if(STRATO_TARGET_NETBSD)
  target_link_directories(strato PUBLIC /usr/local/lib /usr/X11R7/lib)
endif()

# OpenBSD: Add -L/usr/local/lib -L/usr/X11R6/lib
if(STRATO_TARGET_OPENBSD)
  target_link_directories(strato PUBLIC /usr/local/lib /usr/X11R6/lib)
endif()

# Android: Add -lGLESv3 -lOpenSLES -llog
if(STRATO_TARGET_ANDROID)
  target_link_libraries(strato PUBLIC -lm -lGLESv3 -lOpenSLES -llog)
endif()

# OpenHarmony: Add -lGLESv2 -lOpenSLES -llibrawfile.z.so
if(STRATO_TARGET_OPENHARMONY)
  target_link_libraries(strato PUBLIC m GLESv2 OpenSLES librawfile.z.so)
endif()

# GDK link options.
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

  # /SUBSYSTEM:WINDOWS
  if(MSVC)
    target_link_options(strato PUBLIC
      /SUBSYSTEM:WINDOWS
      /ENTRY:wWinMainCRTStartup
      /MANIFEST:NO
    )
  endif()
endif()

# UWP.
if(STRATO_TARGET_UWP)
  target_link_libraries(strato PUBLIC windowsapp d3d12 dxgi dxguid xaudio2 gameinput)
endif()

#
# Custom
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
