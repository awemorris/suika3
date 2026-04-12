cmake_minimum_required(VERSION 3.18)  # Require ARCHIVE_EXTRACT.
project(suika3 LANGUAGES C CXX)       # Actually only C is required for many platforms.

# ---

#
# Target Configuration
#  - Only one target can be turned ON.
#  - If not manually specified, automatically detected later.
#

option(SUIKA_TARGET_WINDOWS          "Build for Windows 2000 or later"                OFF)
option(SUIKA_TARGET_WINDOWS95        "Build for Windows 95/98/Me"                     OFF)
option(SUIKA_TARGET_MACOS            "Build for macOS (App)"                          OFF)
option(SUIKA_TARGET_MACOS_CLI        "Build for macOS (CLI)"                          OFF)
option(SUIKA_TARGET_MACOS_LIB        "Build for macOS (Library)"                      OFF)
option(SUIKA_TARGET_LINUX            "Build for Linux (X11, OpenGL)"                  OFF)
option(SUIKA_TARGET_LINUX_WAYLAND    "Build for Linux (Wayland, OpenGL ES)"           OFF)
option(SUIKA_TARGET_LINUX_GBM        "Build for Linux (GBM, OpenGL ES)"               OFF)
option(SUIKA_TARGET_LINUX_FBDEV      "Build for Linux (fbdev, Soft3D)"                OFF)
option(SUIKA_TARGET_LINUX_X11SOFT    "Build for Linux (X11, Soft3D)"                  OFF)
option(SUIKA_TARGET_IOS              "Build for iOS"                                  OFF)
option(SUIKA_TARGET_ANDROID          "Build for Android"                              OFF)
option(SUIKA_TARGET_OPENHARMONY      "Build for HarmonyOS NEXT"                       OFF)
option(SUIKA_TARGET_WASM             "Build for Emscripten"                           OFF)
option(SUIKA_TARGET_WASM_LOCAL       "Build for Emscripten (Local files)"             OFF)
option(SUIKA_TARGET_UNITY            "Build for Unity"                                OFF)
option(SUIKA_TARGET_FREEBSD          "Build for FreeBSD (X11, OpenGL)"                OFF)
option(SUIKA_TARGET_FREEBSD_WAYLAND  "Build for FreeBSD (Wayland, OpenGL ES)"         OFF)
option(SUIKA_TARGET_FREEBSD_X11SOFT  "Build for FreeBSD (X11, Soft3D)"                OFF)
option(SUIKA_TARGET_NETBSD           "Build for NetBSD (X11, OpenGL)"                 OFF)
option(SUIKA_TARGET_NETBSD_X11SOFT   "Build for NetBSD (X11, Soft3D)"                 OFF)
option(SUIKA_TARGET_OPENBSD          "Build for OpenBSD (X11, OpenGL)"                OFF)
option(SUIKA_TARGET_OPENBSD_X11SOFT  "Build for OpenBSD (X11, Soft3D)"                OFF)
option(SUIKA_TARGET_SOLARIS11        "Build for Solaris 11 (X11, Soft3D)"             OFF)
option(SUIKA_TARGET_SOLARIS10        "Build for Solaris 10 (X11, Soft3D)"             OFF)
option(SUIKA_TARGET_GENERICUNIX      "Build for Generic Unknow UNIX (X11, Soft3D)"    OFF)
option(SUIKA_TARGET_HAIKU            "Build for Haiku OS (Soft3D)"                    OFF)

# ---

#
# Build Options (all OFF by default)
#

#
# [SUIKA_ENABLE_DIST]
#  - Build for OS packages such as FreeBSD Ports and Debian packages.
#  - This configuration uses dynamic linking to system's shared libraries for
#    dependencies such as libpng, libvorbis, and freetype2.
#
option(SUIKA_ENABLE_DIST "Build for OS package" OFF)

#
# [SUIKA_ENABLE_INSTALL]
#  - Install binaries and documents to the system.
#
option(SUIKA_ENABLE_INSTALL "Enable installation" OFF)

#
# [SUIKA_ENABLE_STATIC]
#  - Build a static library for use with project templates for IDEs.
#  - Currenly used by the "macos-lib", "ios-*", "openharmony-*", and "unity-*"
#    presets.
#
option(SUIKA_ENABLE_STATIC "Build a static library" OFF)

#
# [SUIKA_ENABLE_SHARED]
#  - Build a shared library for use with project templates for IDEs.
#  - Currenly used by android-* and unity-*.
#
option(SUIKA_ENABLE_SHARED "Build a dynamic library" OFF)

#
# [SUIKA_ENABLE_OBJECT]
#  - Build object files only.
#  - Currently not used, but can be used for derived projects.
#
option(SUIKA_ENABLE_OBJECT "Build an object library" OFF)

#
# [SUIKA_ENABLE_JIT]
#  - Enable Just-in-Time compiler if the architecture is supported by NoctLang.
#
option(SUIKA_ENABLE_JIT "Enable JIT" OFF)

#
# [SUIKA_ENABLE_I18N]
#  - Enable i18n translation.
#  - Error messages will be customized by the system locale.
#
option(SUIKA_ENABLE_I18N "Enable translation" OFF)

#
# [SUIKA_ENABLE_PACK]
#  - Build the "suika3-pack" executable that generates package files.
#
option(SUIKA_ENABLE_PACK "Enable packager" OFF)

#
# [SUIKA_ENABLE_BYTECOMP]
#  - Build the "suika3-bcc" executable that generates bytecode files from Ray
#    scripts.
#
option(SUIKA_ENABLE_BYTECOMP "Enable bytecode compiler" OFF)

#
# [SUIKA_ENABLE_AOTCOMP]
#  - Build the "suika3-aotc" executable that generates a C source file from Ray
#    scripts.
#
option(SUIKA_ENABLE_AOTCOMP "Enable AOT compiler" OFF)

#
# [SUIKA_ENABLE_WEBSERVER]
#  - Build the "suika3-web" executable that opens a Web browser and acts as
#    a simple httpd server to let the browser load game file.
#  - Currently Windows-only. (Use "python3 -m http.server" on other platforms)
#
option(SUIKA_ENABLE_WEBSERVER "Enable simple Web server" OFF)

#
# [SUIKA_ENABLE_ROT90]
#  - Rotate the screen for framebuffer devices.
#  - Currently used by the "linux-gbm-rot90" preset.
#
option(SUIKA_ENABLE_ROT90 "Rotate screen" OFF)

#
# [SUIKA_ENABLE_GST]
#  - Enable Gstreamer support for video playback on X11.
#  - Currently used by the "linux" and "freebsd" presets.
#
option(SUIKA_ENABLE_GST "Enable Gstreamer support" OFF)

#
# [SUIKA_ENABLE_MAIN2]
#  - Rename main() to main2() for librarization.
#  - Currently used by the "macos-lib" preset.
#
option(SUIKA_ENABLE_MAIN2 "Enable main2() hook" OFF)

# --

#
# Path Options
#

set(SUIKA_DIR_DOCS "share/suika3" CACHE STRING "Document installation destination")
set(SUIKA_DIR_GAME "share/suika3" CACHE STRING "Example installation destination")

# ---

#
# Automatic Target Detection
#

# For when a target is manually specified.
if(   SUIKA_TARGET_WINDOWS
   OR SUIKA_TARGET_WINDOWS95
   OR SUIKA_TARGET_MACOS
   OR SUIKA_TARGET_MACOS_CLI
   OR SUIKA_TARGET_MACOS_LIB
   OR SUIKA_TARGET_LINUX
   OR SUIKA_TARGET_LINUX_WAYLAND
   OR SUIKA_TARGET_LINUX_GBM
   OR SUIKA_TARGET_LINUX_FBDEV
   OR SUIKA_TARGET_LINUX_X11SOFT
   OR SUIKA_TARGET_IOS
   OR SUIKA_TARGET_ANDROID
   OR SUIKA_TARGET_OPENHARMONY
   OR SUIKA_TARGET_WASM
   OR SUIKA_TARGET_WASM_LOCAL
   OR SUIKA_TARGET_UNITY
   OR SUIKA_TARGET_FREEBSD
   OR SUIKA_TARGET_FREEBSD_WAYLAND
   OR SUIKA_TARGET_FREEBSD_X11SOFT
   OR SUIKA_TARGET_NETBSD
   OR SUIKA_TARGET_NETBSD_X11SOFT
   OR SUIKA_TARGET_OPENBSD
   OR SUIKA_TARGET_OPENBSD_X11SOFT
   OR SUIKA_TARGET_SOLARIS11
   OR SUIKA_TARGET_SOLARIS10
   OR SUIKA_TARGET_GENERICUNIX
   OR SUIKA_TARGET_HAIKU
)
  # A target and options are manually specified.
else()   
  if(WIN32)
    # Windows is detected.
    set(SUIKA_TARGET_WINDOWS            ON)
    set(SUIKA_ENABLE_JIT                ON)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  elseif(APPLE AND NOT IOS)
    # macOS is detected.
    set(SUIKA_TARGET_MACOS_CLI          ON)   # Make a CLI command.
    set(SUIKA_ENABLE_JIT                ON)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_BUNDLE             ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux is detected.
    set(SUIKA_TARGET_LINUX              ON)   # X11, OpenGL
    set(SUIKA_ENABLE_JIT                ON)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)
    set(SUIKA_TARGET_GST                ON)   # Use Gstreamer

  elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    # FreeBSD is detected.
    set(SUIKA_TARGET_FREEBSD            ON)   # X11, OpenGL
    set(SUIKA_ENABLE_JIT                ON)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)
    set(SUIKA_TARGET_GST                ON)   # Use Gstreamer

  elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    # NetBSD is detected.
    set(SUIKA_TARGET_NETBSD_X11SOFT     ON)   # X11, Soft3D
    set(SUIKA_ENABLE_JIT                ON)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)
    set(SUIKA_ENABLE_GST                OFF)  # For older computers.

  elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    # OpenBSD is detected.
    set(SUIKA_TARGET_OPENBSD_X11SOFT    ON)   # X11, Soft3D
    set(SUIKA_ENABLE_JIT                OFF)  # Due to W^X
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)
    set(SUIKA_ENABLE_GST                OFF)  # Maybe?

  elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "5.11")
    # Solaris 11 is detected.
    set(SUIKA_TARGET_SOLARIS11          ON)
    set(SUIKA_ENABLE_JIT                OFF)  # XXX: This can be ON.
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_SYSTEM_VERSION VERSION_EQUAL "5.10")
    # Solaris 10 is detected.
    set(SUIKA_TARGET_SOLARIS10          ON)
    set(SUIKA_ENABLE_JIT                OFF)  # XXX: This can be ON.
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    # Haiku is detected.
    set(SUIKA_TARGET_HAIKU              ON)
    set(SUIKA_ENABLE_JIT                ON)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  else()
    # Consider the system is UNIX with X11.
    set(SUIKA_TARGET_GENERICUNIX        ON)
    set(SUIKA_ENABLE_JIT                OFF) # XXX: This can be ON.
    set(SUIKA_ENABLE_I18N               OFF) # XXX: Maybe cc can't parse utf-8?
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  endif()

  # No automatic detection for cross compilation:
  #  - iOS
  #  - Android
  #  - HarmonyOS NEXT
  #  - Wasm (Emscripten)
  #  - Unity
endif()

# ---

#
# PlayfieldEngine Configuration
#

# Build Playfield Engine as a library.
if(   SUIKA_ENABLE_STATIC
   OR SUIKA_ENABLE_SHARED
   OR SUIKA_ENABLE_OBJECT)
  # If we build Suika3 as a library, Playfield Engine must be
  # an object library. (It's a simple solution to merge libraries.)
  set(PLAYFIELD_ENABLE_OBJECT ON)
else()
  # Otherwise, build Playfield Engine as a static library.
  set(PLAYFIELD_ENABLE_STATIC ON)
endif()

# Use pf_init_hook() function.
set(PLAYFIELD_ENABLE_INITHOOK  ON)

# Set the target.
# (SUIKA_TARGET_XXX --> PLAYFIELD_TARGET_XXX)
if(SUIKA_TARGET_WINDOWS)
  set(PLAYFIELD_TARGET_WINDOWS ON)
elseif(SUIKA_TARGET_WINDOWS95)
  set(PLAYFIELD_TARGET_WINDOWS95 ON)
elseif(SUIKA_TARGET_MACOS)
  set(PLAYFIELD_TARGET_MACOS ON)
elseif(SUIKA_TARGET_MACOS_CLI)
  set(PLAYFIELD_TARGET_MACOS_CLI ON)
elseif(SUIKA_TARGET_MACOS_LIB)
  set(PLAYFIELD_TARGET_MACOS_LIB ON)
elseif(SUIKA_TARGET_LINUX)
  set(PLAYFIELD_TARGET_LINUX ON)
elseif(SUIKA_TARGET_LINUX_WAYLAND)
  set(PLAYFIELD_TARGET_LINUX_WAYLAND ON)
elseif(SUIKA_TARGET_LINUX_GBM)
  set(PLAYFIELD_TARGET_LINUX_GBM ON)
elseif(SUIKA_TARGET_LINUX_FBDEV)
  set(PLAYFIELD_TARGET_LINUX_FBDEV ON)
elseif(SUIKA_TARGET_LINUX_X11SOFT)
  set(PLAYFIELD_TARGET_LINUX_X11SOFT ON)
elseif(SUIKA_TARGET_IOS)
  set(PLAYFIELD_TARGET_IOS ON)
elseif(SUIKA_TARGET_ANDROID)
  set(PLAYFIELD_TARGET_ANDROID ON)
elseif(SUIKA_TARGET_OPENHARMONY)
  set(PLAYFIELD_TARGET_OPENHARMONY ON)
elseif(SUIKA_TARGET_WASM)
  set(PLAYFIELD_TARGET_WASM ON)
elseif(SUIKA_TARGET_WASM_LOCAL)
  set(PLAYFIELD_TARGET_WASM_LOCAL ON)
elseif(SUIKA_TARGET_UNITY)
  set(PLAYFIELD_TARGET_UNITY ON)
elseif(SUIKA_TARGET_FREEBSD)
  set(PLAYFIELD_TARGET_FREEBSD ON)
elseif(SUIKA_TARGET_FREEBSD_WAYLAND)
  set(PLAYFIELD_TARGET_FREEBSD_WAYLAND ON)
elseif(SUIKA_TARGET_FREEBSD_X11SOFT)
  set(PLAYFIELD_TARGET_FREEBSD_X11SOFT ON)
elseif(SUIKA_TARGET_NETBSD)
  set(PLAYFIELD_TARGET_NETBSD ON)
elseif(SUIKA_TARGET_NETBSD_X11SOFT)
  set(PLAYFIELD_TARGET_NETBSD_X11SOFT ON)
elseif(SUIKA_TARGET_OPENBSD)
  set(PLAYFIELD_TARGET_OPENBSD ON)
elseif(SUIKA_TARGET_OPENBSD_X11SOFT)
  set(PLAYFIELD_TARGET_OPENBSD_X11SOFT ON)
elseif(SUIKA_TARGET_SOLARIS11)
  set(PLAYFIELD_TARGET_SOLARIS11 ON)
elseif(SUIKA_TARGET_SOLARIS10)
  set(PLAYFIELD_TARGET_SOLARIS10 ON)
elseif(SUIKA_TARGET_GENERICUNIX)
  set(PLAYFIELD_TARGET_GENERICUNIX ON)
elseif(SUIKA_TARGET_HAIKU)
  set(PLAYFIELD_TARGET_HAIKU ON)
elseif(SUIKA_TARGET_GDK_WINDOWS)
  set(PLAYFIELD_TARGET_GDK_WINDOWS ON)
elseif(SUIKA_TARGET_GDK_XBOX_XS)
  set(PLAYFIELD_TARGET_GDK_XBOX_XS ON)
else()
  error("No target specified or detected.")
endif()

# Unity target arch
if(SUIKA_TARGET_UNITY_SWITCH)
  set(PLAYFIELD_TARGET_UNITY_SWITCH)
elseif(SUIKA_TARGET_UNITY_PS5)
  set(PLAYFIELD_TARGET_UNITY_PS5)
elseif(SUIKA_TARGET_UNITY_XBOX)
  set(PLAYFIELDTARGET_UNITY_XBOX)
endif()

# When building for an OS's packager
if(SUIKA_ENABLE_DIST)
  set(PLAYFIELD_ENABLE_DIST ON)
endif()

# Use JIT
if(SUIKA_ENABLE_JIT)
  set(PLAYFIELD_ENABLE_JIT ON)
else()
  set(PLAYFIELD_ENABLE_JIT OFF)
endif()

# Use I18N
if(SUIKA_ENABLE_I18N)
  set(PLAYFIELD_ENABLE_I18N ON)
endif()

# Use Gstreamer
if(SUIKA_ENABLE_GST)
  set(PLAYFIELD_ENABLE_GST ON)
endif()

# main() --> main2()
if(SUIKA_ENABLE_MAIN2)
  set(PLAYFIELD_ENABLE_MAIN2 ON)
endif()

# Build libstratopack for suika3-pack
if(SUIKA_ENABLE_PACK)
  set(PLAYFIELD_ENABLE_PACK ON)
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
    # MSVC
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /Zi /DDEBUG /UNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /DDEBUG /UNDEBUG")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # GCC/Clang
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g3 -DDEBUG -Wall -Werror -Wextra -Wundef -Wconversion")
    set(CMAKE_OBJC_FLAGS_DEBUG "${CMAKE_OBJC_FLAGS_DEBUG} -O0 -g3 -DNDEBUG -Wall -Werror -Wextra -Wundef -Wconversion")
else()
    # Other
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
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -g -DNDEBUG")
  set(CMAKE_OBJC_FLAGS_RELEASE "${CMAKE_OBJC_FLAGS_RELEASE} -O2 -g -DNDEBUG")
else()
  # Other
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -g -DNDEBUG")
  set(CMAKE_OBJC_FLAGS_RELEASE "${CMAKE_OBJC_FLAGS_RELEASE} -O2 -g -DNDEBUG")
endif()

# ---

#
# Apple Quirks (Universal Binary 2)
#

if(   SUIKA_TARGET_MACOS
   OR SUIKA_TARGET_MACOS_CLI
   OR SUIKA_TARGET_MACOS_LIB)
  # Exclude Homebrew packaging.
  #  - Homebrew provides arch-specific libraries.
  #  - If we try to make a fat binary, link will fail.
  if(NOT SUIKA_ENABLE_DIST)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
  endif()
endif()

# ---

#
# Dependencies
#

# Playfield Engine
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/PlayfieldEngine)

# Change the startup file name. (main.pf --> main.ray)
target_compile_definitions(playfield PRIVATE STARTUP_FILE="main.ray")

# ---

#
# Checks
#

include(CheckIncludeFile)

# stdint.h
check_include_file("stdint.h" HAVE_STDINT_H)
if(HAVE_STDINT_H)
  add_definitions(-DHAVE_STDINT_H=1)
endif()

# inttypes.h
check_include_file("inttypes.h" HAVE_INTTYPES_H)
if(HAVE_INTTYPES_H)
  add_definitions(-DHAVE_INTTYPES_H=1)
endif()

# sys/types.h
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
if(HAVE_SYS_TYPES_H)
  add_definitions(-DHAVE_SYS_TYPES_H=1)
endif()

# ---

#
# Main Target (the "suika3" executable)
#

# Base Source
set(SUIKA_BASE_SOURCES
  src/anime.c
  src/api.c
  src/conf.c
  src/game.c
  src/gui.c
  src/hal.c
  src/history.c
  src/image.c
  src/mixer.c
  src/save.c
  src/seen.c
  src/stage.c
  src/sysbtn.c
  src/tag.c
  src/text.c
  src/vars.c
  src/cmd.c
  src/cmd_anime.c
  src/cmd_bg.c
  src/cmd_bgm.c
  src/cmd_callmacro.c
  src/cmd_ch.c
  src/cmd_chapter.c
  src/cmd_choose.c
  src/cmd_click.c
  src/cmd_config.c
  src/cmd_defmacro.c
  src/cmd_else.c
  src/cmd_elseif.c
  src/cmd_endif.c
  src/cmd_endmacro.c
  src/cmd_goto.c
  src/cmd_gui.c
  src/cmd_if.c
  src/cmd_label.c
  src/cmd_layer.c
  src/cmd_load.c
  src/cmd_move.c
  src/cmd_pencil.c
  src/cmd_returnmacro.c
  src/cmd_se.c
  src/cmd_set.c
  src/cmd_skip.c
  src/cmd_text.c
  src/cmd_video.c
  src/cmd_volume.c
  src/cmd_wait.c
)

# I18N Source
if(SUIKA_ENABLE_I18N)
  set(SUIKA_I18N_SOURCES
    src/translation.c
  )
endif()

# Resources for Windows
if(   SUIKA_TARGET_WINDOWS
   OR SUIKA_TARGET_WINDOWS95)
  if(NOT SUIKA3_ENABLE_STATIC)
    set(SUIKA_RESOURCES
      resources/windows/resource.rc
    )
  endif()
endif()

# Add the target
if(   SUIKA_ENABLE_STATIC
   OR SUIKA_ENABLE_SHARED
   OR SUIKA_ENABLE_OBJECT)
  #
  # Build a library.
  #

  # Get the library type.
  if(SUIKA_ENABLE_STATIC)
    set(SUIKA_LIB_TYPE STATIC)
  elseif(SUIKA_ENABLE_SHARED)
    set(SUIKA_LIB_TYPE SHARED)
  elseif(SUIKA_ENABLE_OBJECT)
    set(SUIKA_LIB_TYPE OBJECT)
  endif()

  # Link dependencies into the library if not packaing.
  if(NOT SUIKA_ENABLE_DIST)
    set(SUIKA_DEPS
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
    suika3
    ${SUIKA_LIB_TYPE}
    ${SUIKA_BASE_SOURCES}
    ${SUIKA_I18N_SOURCES}
    ${SUIKA_RESOURCES}
    ${SUIKA_DEPS}
  )
else()
  #
  # Make an executable.
  #
  add_executable(
    suika3
    ${SUIKA_BASE_SOURCES}
    ${SUIKA_I18N_SOURCES}
    ${SUIKA_RESOURCES}
  )
endif()

# ---

#
# CPPFLAGS
#

# Include
target_include_directories(suika3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# JIT
if(SUIKA_ENABLE_JIT)
  target_compile_definitions(suika3 PRIVATE S3_USE_JIT PF_USE_JIT NOCT_USE_JIT)
endif()

# Translation
if(SUIKA_ENABLE_I18N)
  target_compile_definitions(suika3 PRIVATE S3_USE_TRANSLATION)
endif()

# DLL
if(SUIKA_ENABLE_SHARED)
  target_compile_definitions(suika3 PRIVATE S3_USE_SHARED)
endif()

# Windows
if(SUIKA_TARGET_WINDOWS)
  if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  endif()
endif()

# Windows 95
if(SUIKA_TARGET_WINDOWS95)
  if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  else()
    add_compile_definitions(_WIN32_WINNT=0x0400 __USE_MINGW_ANSI_STDIO=1)
    add_definitions(-UUNICODE -U_UNICODE)
  endif()
endif()

# Emscripten
if(SUIKA_TARGET_WASM OR SUIKA_TARGET_WASM_LOCAL)
  # -DUSE_EMSCRIPTEN
  target_compile_definitions(suika3 PRIVATE USE_EMSCRIPTEN)
endif()

# Unity
if(SUIKA_TARGET_UNITY)
  target_compile_definitions(suika3 PRIVATE S3_TARGET_UNITY PF_TARGET_UNITY HAL_TARGET_UNITY)
endif()

# ---

#
# CFLAGS
#

# Windows
if(SUIKA_TARGET_WINDOWS)
  if(MSVC)
    target_compile_options(suika3 PRIVATE /utf-8)
  else()
    target_compile_options(suika3 PRIVATE -municode -finput-charset=utf-8 -fexec-charset=utf-8)
  endif()
endif()

# Windows 95
if(SUIKA_TARGET_WINDOWS95)
  if(MSVC)
    target_compile_options(suika3 PRIVATE /utf-8)
  else()
    target_compile_options(suika3 PRIVATE -finput-charset=utf-8 -fexec-charset=shift-jis)
  endif()
endif()

# ---

#
# LDFLAGS
#

# Link Playfield Engine
target_link_libraries(suika3 PRIVATE
  playfield
  strato
  playfield  # Specify twice to resolve circular references.
  noctlang
)

# Windows
if(SUIKA_TARGET_WINDOWS)
  if(MSVC)
    target_link_options(suika3 PRIVATE /SUBSYSTEM:WINDOWS)
  else()
    target_link_options(suika3 PRIVATE -mwindows -static -Wl,-u,wWinMain -Wl,-u,_wWinMain@16)
  endif()
endif()

# Windows 95
if(SUIKA_TARGET_WINDOWS95)
  if(MSVC)
    target_link_options(suika3 PRIVATE /SUBSYSTEM:WINDOWS)
  else()
    target_link_options(suika3 PRIVATE -mwindows -static -static-libgcc -Wl,-u,WinMain -Wl,-u,_WinMain@16)
  endif()
endif()

# macOS
if(SUIKA_TARGET_MACOS_LIB)
  target_link_options(suika3 PRIVATE -ObjC -all_load)
endif()

# Emscripten
if(SUIKA_TARGET_WASM)
  # Make the output name "index.html"
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  set_target_properties(suika3 PROPERTIES OUTPUT_NAME index)
  target_link_options(suika3 PRIVATE
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
if(SUIKA_TARGET_WASM_LOCAL)
  # Make the output name "index.html"
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  set_target_properties(suika3 PROPERTIES OUTPUT_NAME index)
  target_link_options(suika3 PRIVATE
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
# Packager Target (The "suika3-pack" executable)
#

if(SUIKA_ENABLE_PACK)
  if (SUIKA_TARGET_WINDOWS)
    set(SUIKA_PACK_RESOURCE resources/windows/pack-resource.rc)
  endif()

  add_executable(
    suika3-pack
    external/PlayfieldEngine/external/StratoHAL/src/archive.c
    external/PlayfieldEngine/src/pack.c
    ${SUIKA_PACK_RESOURCE}
  )

  target_include_directories(
    suika3-pack
    PRIVATE
    include
    external/PlayfieldEngine/include
    external/PlayfieldEngine/external/StratoHAL/include
    external/PlayfieldEngine/external/NoctLang/src/core
    external/PlayfieldEngine/external/NoctLang/src/backend
  )

  target_link_libraries(suika3-pack stratopack)
endif()

# ---

#
# Bytcode Compiler Target (The "suika3-bcc" executable)
#

if(SUIKA_ENABLE_BYTECOMP)
  if (SUIKA_TARGET_WINDOWS)
    set(SUIKA_BYTECOMP_RESOURCE resources/windows/bytecomp-resource.rc)
  endif()

  add_executable(
    suika3-bcc
    external/PlayfieldEngine/src/compiler.c
    ${SUIKA_BYTECOMP_RESOURCE}
  )

  target_include_directories(
    suika3-bcc
    PRIVATE
    include
    external/PlayfieldEngine/include
    external/PlayfieldEngine/external/StratoHAL/include
    external/PlayfieldEngine/external/NoctLang/src/core
    external/PlayfieldEngine/external/NoctLang/src/backend
  )

  target_link_libraries(suika3-bcc noctlang)

  if(UNIX)
    target_link_libraries(suika3-bcc m)
  endif()
endif()

# ---

#
# AOT Compiler Target (The "suika3-aotc" executable)
#

if(SUIKA_ENABLE_AOTCOMP)
  if (SUIKA_TARGET_WINDOWS)
    set(SUIKA_AOTCOMP_RESOURCE resources/windows/aotcomp-resource.rc)
  endif()

  add_executable(
    suika3-aotc
    external/PlayfieldEngine/src/aot.c
    external/PlayfieldEngine/external/NoctLang/src/backend/cback.c
    ${SUIKA_AOTCOMP_RESOURCE}
  )

  target_include_directories(
    suika3-aotc
    PRIVATE
    include
    external/PlayfieldEngine/include
    external/PlayfieldEngine/external/StratoHAL/include
    external/PlayfieldEngine/external/NoctLang/src/core
    external/PlayfieldEngine/external/NoctLang/src/backend
  )

  target_link_libraries(suika3-aotc noctlang)

  if(UNIX)
    target_link_libraries(suika3-aotc m)
  endif()
endif()

# ---

#
# Web Server Target (The "suika3-web.exe" executable)
#

if(SUIKA_ENABLE_WEBSERVER)
  if (SUIKA_TARGET_WINDOWS)
    set(SUIKA_WEB_RESOURCE external/PlayfieldEngine/resources/windows/web-resource.rc)
  endif()

  add_executable(
    suika3-web
    external/PlayfieldEngine/src/webserver.c
    ${SUIKA_WEB_RESOURCE}
  )

  if (SUIKA_TARGET_WINDOWS OR SUIKA_TARGET_WINDOWS95)
    target_link_libraries(suika3-web wsock32 ws2_32)
  endif()
endif()

# ---

#
# macOS Bundle Handling
#

if(SUIKA_TARGET_MACOS)
  # Use capital name.
  set_target_properties(suika3 PROPERTIES OUTPUT_NAME "Suika3")

  # Make an app bundle.
  set_target_properties(suika3 PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.noctvm.suika3"
    MACOSX_BUNDLE_BUNDLE_NAME "Suika3"
  )

  # Set an icon.
  set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/icon512.png")
  target_sources(suika3 PRIVATE ${APP_ICON})
  set_source_files_properties(${APP_ICON} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
  )
  set_target_properties(suika3 PROPERTIES
    MACOSX_BUNDLE_ICON_FILE "icon512.png"
  )

  # Copy the assets.arc file.
  set(APP_GAME_DATA "${CMAKE_CURRENT_SOURCE_DIR}/resources/assets.arc")
  set_source_files_properties(${APP_GAME_DATA} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
  )
  target_sources(suika3 PRIVATE ${APP_GAME_DATA})
endif()

# ---

#
# UNIX Install
#

if(SUIKA_ENABLE_INSTALL)
  include(GNUInstallDirs)

  #
  # Install binaries.
  #

  # "suika3" command. 
  install(TARGETS suika3       RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  # "suika3-pack" command.
  if(SUIKA_ENABLE_PACK)
    install(TARGETS suika3-pack  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

  # "suika3-bcc" command. (bytecode compiler)
  if(SUIKA_ENABLE_BYTECOMP)
    install(TARGETS suika3-bcc   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

  # "suika3-aotc" command. (AOT compiler)
  if(SUIKA_ENABLE_AOTCOMP)
    install(TARGETS suika3-aotc  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

  #
  # Install manpages. (mdoc for *BSD, roff for others)
  #

  if(   SUIKA_TARGET_FREEBSD
     OR SUIKA_TARGET_FREEBSD_WAYLAND
     OR SUIKA_TARGET_FREEBSD_X11SOFT
     OR SUIKA_TARGET_NETBSD
     OR SUIKA_TARGET_NETBSD_X11SOFT
     OR SUIKA_TARGET_OPENBSD
     OR SUIKA_TARGET_OPENBSD_X11SOFT)
    # BSD mdoc
    install(
      FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3.1.mdoc"
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
      RENAME      "suika3.1"
    )
    install(
      FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3-pack.1.mdoc"
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
      RENAME      "suika3-pack.1"
    )
    install(
      FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3-bcc.1.mdoc"
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
      RENAME      "suika3-bcc.1"
    )
    install(
      FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3-aotc.1.mdoc"
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
      RENAME      "suika3-aotc.1"
    )
  else()
    # roff
    install(
      FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3.1"
                  "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3-pack.1"
                  "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3-bcc.1"
                  "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3-aotc.1"
      DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    )
  endif()

  #
  # Install documents.
  # 

  install(
    DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/docs/mkdocs-en/docs"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/${SUIKA_DIR_DOCS}
  )

  #
  # Install the sample game.
  #

  # Install ZLIB-licensed contents.
  if(SUIKA_ENABLE_DIST)
    # Exclude the font when packaging.
    # (A symlink to a system font should be created by packaging process.)
    install(
      DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/game"
      DESTINATION "${CMAKE_INSTALL_PREFIX}/${SUIKA_DIR_GAME}"
      FILES_MATCHING
      PATTERN "*"
      PATTERN "system/bgm2" EXCLUDE
      PATTERN "system/font/LICENSE" EXCLUDE
      PATTERN "system/font/rounded-l-mplus-1c-bold.ttf" EXCLUDE
    )
  else()
    # Include the font.
    install(
      DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}/game"
      DESTINATION "${CMAKE_INSTALL_PREFIX}/${SUIKA_DIR_GAME}"
    )
  endif()

endif()
