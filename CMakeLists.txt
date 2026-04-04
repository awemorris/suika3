cmake_minimum_required(VERSION 3.13)
project(suika3 LANGUAGES C CXX)

#
# Configuration
#

# Targets
option(SUIKA_TARGET_WINDOWS          "Build for Windows XP or later"                  OFF)
option(SUIKA_TARGET_WINDOWS95        "Build for Windows 95/98/Me/2000"                OFF)
option(SUIKA_TARGET_MACOS            "Build for macOS (App)"                          OFF)
option(SUIKA_TARGET_MACOS_CLI        "Build for macOS (CLI)"                          OFF)
option(SUIKA_TARGET_MACOS_LIB        "Build for macOS (Library)"                      OFF)
option(SUIKA_TARGET_LINUX            "Build for Linux (X11, OpenGL)"                  OFF)
option(SUIKA_TARGET_LINUX_WAYLAND    "Build for Linux (Wayland, OpenGL ES)"           OFF)
option(SUIKA_TARGET_LINUX_GBM        "Build for Linux (GBM, OpenGL ES)"               OFF)
option(SUIKA_TARGET_LINUX_FBDEV      "Build for Linux (fbdev, Soft3D)"                OFF)
option(SUIKA_TARGET_LINUX_X11SOFT    "Build for Linux (X11, Soft3D)"                  OFF)
option(SUIKA_TARGET_IOS              "Build for iOS"                                  OFF)
option(SUIKA_TARGET_ANDROID          "Build for Android (OpenGL ES)"                  OFF)
option(SUIKA_TARGET_OPENHARMONY      "Build for HarmonyOS NEXT (OpenGL ES)"           OFF)
option(SUIKA_TARGET_WASM             "Build for Emscripten (WebGL)"                   OFF)
option(SUIKA_TARGET_WASM_LOCAL       "Build for Emscripten Local (WebGL)"             OFF)
option(SUIKA_TARGET_UNITY            "Build for Unity"                                OFF)
option(SUIKA_TARGET_FREEBSD          "Build for FreeBSD (X11, OpenGL)"                OFF)
option(SUIKA_TARGET_FREEBSD_WAYLAND  "Build for FreeBSD (Wayland, OpenGL ES)"         OFF)
option(SUIKA_TARGET_FREEBSD_X11SOFT  "Build for FreeBSD (X11, Soft3D)"                OFF)
option(SUIKA_TARGET_NETBSD           "Build for NetBSD (X11, OpenGL)"                 OFF)
option(SUIKA_TARGET_NETBSD_X11SOFT   "Build for NetBSD (X11, Soft3D)"                 OFF)
option(SUIKA_TARGET_OPENBSD          "Build for OpenBSD (X11, OpenGL)"                OFF)
option(SUIKA_TARGET_OPENBSD_X11SOFT  "Build for OpenBSD (X11, Soft3D)"                OFF)
option(SUIKA_TARGET_SOLARIS11        "Build for Solaris 11 (X11, Soft3D)"             OFF)
option(SUIKA_TARGET_GENERICUNIX      "Build for Generic Unknow UNIX (X11, Soft3D)"    OFF)
option(SUIKA_TARGET_HAIKU            "Build for Haiku OS (Soft3D)"                    OFF)

# Options
option(SUIKA_ENABLE_STATIC           "Build a static library"                         OFF)
option(SUIKA_ENABLE_SHARED           "Build a dynamic library"                        OFF)
option(SUIKA_ENABLE_OBJECT           "Build an object library"                        OFF)
option(SUIKA_ENABLE_DIST             "Build for OS package"                           OFF)
option(SUIKA_ENABLE_JIT              "Enable JIT"                                     OFF)
option(SUIKA_ENABLE_I18N             "Enable translation"                             OFF)
option(SUIKA_ENABLE_PACK             "Enable packager"                                OFF)
option(SUIKA_ENABLE_BYTECOMP         "Enable bytecode compiler"                       OFF)
option(SUIKA_ENABLE_AOTCOMP          "Enable AOT compiler"                            OFF)
option(SUIKA_ENABLE_WEBSERVER        "Enable simple Web server"                       OFF)
option(SUIKA_ENABLE_ROT90            "Rotate screen"                                  OFF)
option(SUIKA_ENABLE_GST              "Enable Gstreamer support"                       OFF)
option(SUIKA_ENABLE_MAIN2            "Enable macOS main2()"                           OFF)
option(SUIKA_ENABLE_INSTALL          "Enable installation"                            OFF)

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
    set(SUIKA_TARGET_MACOS_CLI          ON)
    set(SUIKA_ENABLE_JIT                ON)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_BUNDLE             ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux is detected.
    set(SUIKA_TARGET_LINUX              ON)
    set(SUIKA_ENABLE_JIT                ON)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(SUIKA_TARGET_FREEBSD_X11SOFT    ON)
    set(SUIKA_ENABLE_JIT                ON)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    set(SUIKA_TARGET_NETBSD_X11SOFT     ON)
    set(SUIKA_ENABLE_JIT                ON)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    set(SUIKA_TARGET_OPENBSD_X11SOFT    ON)
    set(SUIKA_ENABLE_JIT                OFF)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "SunOS" AND CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "5.11")
    set(SUIKA_TARGET_SOLARIS11          ON)
    set(SUIKA_ENABLE_JIT                ON)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  elseif(CMAKE_SYSTEM_NAME STREQUAL "Haiku")
    set(SUIKA_TARGET_HAIKU              ON)
    set(SUIKA_ENABLE_JIT                ON)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  else()
    set(SUIKA_TARGET_GENERICUNIX        ON)
    set(SUIKA_ENABLE_JIT                ON)
    set(SUIKA_ENABLE_I18N               ON)
    set(SUIKA_ENABLE_PACK               ON)
    set(SUIKA_ENABLE_BYTECOMP           ON)
    set(SUIKA_ENABLE_AOTCOMP            ON)
    set(SUIKA_ENABLE_INSTALL            ON)

  endif()

  # No automatic detection for:
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
  set(PLAYFIELD_ENABLE_OBJECT ON)
else()
  set(PLAYFIELD_ENABLE_STATIC ON)
endif()

# Use pf_init_hook() function.
set(PLAYFIELD_ENABLE_INITHOOK  ON)

# Set the target.
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

# Unity OS
if(SUIKA_TARGET_UNITY_SWITCH)
  set(PLAYFIELD_TARGET_UNITY_SWITCH)
elseif(SUIKA_TARGET_UNITY_PS5)
  set(PLAYFIELD_TARGET_UNITY_PS5)
elseif(SUIKA_TARGET_UNITY_XBOX)
  set(PLAYFIELDTARGET_UNITY_XBOX)
endif()

# Distribution binary
if(SUIKA_ENABLE_DIST)
  set(PLAYFIELD_ENABLE_DIST ON)
endif()

# JIT
if(SUIKA_ENABLE_JIT)
  set(PLAYFIELD_ENABLE_JIT ON)
else()
  set(PLAYFIELD_ENABLE_JIT OFF)
endif()

# I18N
if(SUIKA_ENABLE_I18N)
  set(PLAYFIELD_ENABLE_I18N ON)
endif()

# Gstreamer
if(SUIKA_ENABLE_GST)
  set(PLAYFIELD_ENABLE_GST ON)
endif()

if(SUIKA_ENABLE_MAIN2)
  set(PLAYFIELD_ENABLE_MAIN2 ON)
endif()

# libstratopack
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
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -DNDEBUG")
  set(CMAKE_OBJC_FLAGS_RELEASE "${CMAKE_OBJC_FLAGS_RELEASE} -O2 -DNDEBUG")
else()
  # Other
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -DNDEBUG")
  set(CMAKE_OBJC_FLAGS_RELEASE "${CMAKE_OBJC_FLAGS_RELEASE} -O2 -DNDEBUG")
endif()

# ---

#
# Apple Quirks (Universal Binary 2)
#

if(   SUIKA_TARGET_MACOS
   OR SUIKA_TARGET_MACOS_CLI
   OR SUIKA_TARGET_MACOS_LIB)
  set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
endif()

# ---

#
# Dependencies
#

# PlayfieldEngine
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/PlayfieldEngine)

target_compile_definitions(playfield PRIVATE STARTUP_FILE="main.ray")

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

# Resource
if(   SUIKA_TARGET_WINDOWS
   OR SUIKA_TARGET_WINDOWS95)
  if(NOT SUIKA3_ENABLE_STATIC)
    set(SUIKA_RESOURCES
      resources/windows/resource.rc
    )
  endif()
endif()

# Add Target
if(   SUIKA_ENABLE_STATIC
   OR SUIKA_ENABLE_SHARED
   OR SUIKA_ENABLE_OBJECT)
  if(SUIKA_ENABLE_STATIC)
    set(SUIKA_LIB_TYPE STATIC)
  elseif(SUIKA_ENABLE_SHARED)
    set(SUIKA_LIB_TYPE SHARED)
  elseif(SUIKA_ENABLE_OBJECT)
    set(SUIKA_LIB_TYPE OBJECT)
  endif()

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

  add_library(
    suika3
    ${SUIKA_LIB_TYPE}
    ${SUIKA_BASE_SOURCES}
    ${SUIKA_I18N_SOURCES}
    ${SUIKA_RESOURCES}
    ${SUIKA_DEPS}
  )
else()
  # Make an executable.
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
# Packager Target
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
# Bytcode Compiler Target
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
# AOT Compiler Target
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
# Web Server Target
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
# macOS Bundle
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
  install(TARGETS suika3       RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(TARGETS suika3-pack  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(TARGETS suika3-bcc   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(TARGETS suika3-aotc  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  if (   SUIKA_TARGET_FREEBSD
      OR SUIKA_TARGET_FREEBSD_WAYLAND
      OR SUIKA_TARGET_FREEBSD_X11SOFT
      OR SUIKA_TARGET_NETBSD
      OR SUIKA_TARGET_NETBSD_X11SOFT
      OR SUIKA_TARGET_OPENBSD
      OR SUIKA_TARGET_OPENBSD_X11SOFT)
    # BSD manpage
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
    # Linux / UNIX manpage
    install(
      FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3.1"
                  "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3-pack.1"
                  "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3-bcc.1"
                  "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3-aotc.1"
      DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    )
  endif()
endif()
