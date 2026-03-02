cmake_minimum_required(VERSION 3.13)
project(suika3 LANGUAGES C CXX)

#
# Configuration
#

# Targets
option(SUIKA_TARGET_WINDOWS          "Build for Windows"                     OFF)
option(SUIKA_TARGET_MACOS            "Build for macOS"                       OFF)
option(SUIKA_TARGET_LINUX            "Build for Linux X11"                   OFF)
option(SUIKA_TARGET_LINUX_WAYLAND    "Build for Linux Wayland"               OFF)
option(SUIKA_TARGET_LINUX_GBM        "Build for Linux Framebuffer"           OFF)
Option(SUIKA_TARGET_IOS              "Build for iOS"                         OFF)
option(SUIKA_TARGET_ANDROID          "Build for Android"                     OFF)
option(SUIKA_TARGET_WASM             "Build for Emscripten"                  OFF)
option(SUIKA_TARGET_WASM_LOCAL       "Build for Emscripten (local file)"     OFF)
option(SUIKA_TARGET_UNITY            "Build for Unity"                       OFF)
option(SUIKA_TARGET_FREEBSD          "Build for FreeBSD X11"                 OFF)
option(SUIKA_TARGET_FREEBSD_WAYLAND  "Build for FreeBSD Wayland"             OFF)
option(SUIKA_TARGET_NETBSD           "Build for NetBSD X11"                  OFF)
option(SUIKA_TARGET_OPENBSD          "Build for OpenBSD X11"                 OFF)

# Options
option(SUIKA_ENABLE_JIT           "Enable JIT"              OFF)
option(SUIKA_ENABLE_I18N          "Enable translation"      OFF)
option(SUIKA_ENABLE_BUNDLE        "Enable macOS bundle"     OFF)
option(SUIKA_ENABLE_PACK          "Enable packager"         OFF)
option(SUIKA_ENABLE_STATIC        "Build a static library"  OFF)
option(SUIKA_ENABLE_SHARED        "Build a dynamic library" OFF)
option(SUIKA_ENABLE_DIST          "Build for packages"      OFF)
option(SUIKA_ENABLE_ROT90         "Rotate screen for GBM"   OFF)

#
# Automatic Target Detection
#

# For when a target is manually specified.
if(   SUIKA_TARGET_WINDOWS
   OR SUIKA_TARGET_MACOS
   OR SUIKA_TARGET_LINUX
   OR SUIKA_TARGET_LINUX_WAYLAND
   OR SUIKA_TARGET_LINUX_GBM
   OR SUIKA_TARGET_IOS
   OR SUIKA_TARGET_ANDROID
   OR SUIKA_TARGET_WASM
   OR SUIKA_TARGET_WASM_LOCAL
   OR SUIKA_TARGET_UNITY
   OR SUIKA_TARGET_FREEBSD
   OR SUIKA_TARGET_FREEBSD_WAYLAND
   OR SUIKA_TARGET_NETBSD
   OR SUIKA_TARGET_OPENBSD
)
  # A target and options are manually specified.
else()   
  if(WIN32)
    # Windows is detected.
    set(SUIKA_TARGET_WINDOWS    ON)
    set(SUIKA_ENABLE_JIT        ON)
    set(SUIKA_ENABLE_I18N       ON)
    set(SUIKA_ENABLE_PACK       ON)
  endif()

  if(APPLE AND NOT IOS)
    # macOS is detected.
    set(SUIKA_TARGET_MACOS      ON)
    set(SUIKA_ENABLE_JIT        ON)
    set(SUIKA_ENABLE_I18N       ON)
    set(SUIKA_ENABLE_BUNDLE     ON)
    set(SUIKA_ENABLE_PACK       ON)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux is detected.
    set(SUIKA_TARGET_LINUX      ON)
    set(SUIKA_ENABLE_JIT        ON)
    set(SUIKA_ENABLE_I18N       ON)
    set(SUIKA_ENABLE_PACK       ON)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(SUIKA_TARGET_FREEBSD    ON)
    set(SUIKA_ENABLE_JIT        ON)
    set(SUIKA_ENABLE_I18N       ON)
    set(SUIKA_ENABLE_PACK       ON)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    set(SUIKA_TARGET_NETBSD     ON)
    set(SUIKA_ENABLE_JIT        ON)
    set(SUIKA_ENABLE_I18N       ON)
    set(SUIKA_ENABLE_PACK       ON)
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    set(SUIKA_TARGET_OPENBSD    ON)
    set(SUIKA_ENABLE_I18N       ON)
    set(SUIKA_ENABLE_PACK       ON)
  endif()

  # No automatic detection for:
  #  - Android
  #  - Wasm (Emscripten)
  #  - Unity
endif()

#
# PlayfieldEngine Configuration
#

set(PLAYFIELD_ENABLE_STATIC ON)
set(PLAYFIELD_ENABLE_INITHOOK ON)

# Set the target and its specific options.
if(SUIKA_TARGET_WINDOWS)
  set(PLAYFIELD_TARGET_WINDOWS ON)
elseif(SUIKA_TARGET_MACOS)
  set(PLAYFIELD_TARGET_MACOS ON)
elseif(SUIKA_TARGET_LINUX)
  set(PLAYFIELD_TARGET_LINUX ON)
elseif(SUIKA_TARGET_LINUX_WAYLAND)
  set(PLAYFIELD_TARGET_LINUX_WAYLAND ON)
elseif(SUIKA_TARGET_LINUX_GBM)
  set(PLAYFIELD_TARGET_LINUX_GBM ON)
elseif(SUIKA_TARGET_IOS)
  set(PLAYFIELD_TARGET_IOS ON)
elseif(SUIKA_TARGET_ANDROID)
  set(PLAYFIELD_TARGET_ANDROID ON)
elseif(SUIKA_TARGET_WASM)
  set(PLAYFIELD_TARGET_WASM ON)
elseif(SUIKA_TARGET_WASM_LOCAL)
  set(PLAYFIELD_TARGET_WASM_LOCAL ON)
elseif(SUIKA_TARGET_UNITY)
  set(PLAYFIELDTARGET_UNITY ON)
elseif(SUIKA_TARGET_FREEBSD)
  set(PLAYFIELD_TARGET_FREEBSD ON)
elseif(SUIKA_TARGET_FREEBSD_WAYLAND)
  set(PLAYFIELD_TARGET_FREEBSD_WAYLAND ON)
elseif(SUIKA_TARGET_NETBSD)
  set(PLAYFIELD_TARGET_NETBSD ON)
elseif(SUIKA_TARGET_OPENBSD)
  set(PLAYFIELD_TARGET_OPENBSD ON)
elseif(SUIKA_TARGET_GDK_WINDOWS)
  set(PLAYFIELD_TARGET_GDK_WINDOWS ON)
elseif(SUIKA_TARGET_GDK_XBOX_XS)
  set(PLAYFIELD_TARGET_GDK_XBOX_XS ON)
elseif(SUIKA_TARGET_UWP)
  set(PLAYFIELD_TARGET_UWP ON)
endif()

if(SUIKA_TARGET_UNITY_SWITCH)
  set(PLAYFIELD_TARGET_UNITY_SWITCH)
elseif(SUIKA_TARGET_UNITY_PS5)
  set(PLAYFIELD_TARGET_UNITY_PS5)
elseif(SUIKA_TARGET_UNITY_XBOX)
  set(PLAYFIELDTARGET_UNITY_XBOX)
endif()

# If building with JIT.
if(SUIKA_ENABLE_JIT)
  set(PLAYFIELD_ENABLE_JIT ON)
else()
  set(PLAYFIELD_ENABLE_JIT OFF)
endif()

# If building with translation.
if(SUIKA_ENABLE_I18N)
  set(PLAYFIELD_ENABLE_I18N ON)
endif()

# If building for Emscripten.
if(SUIKA_TARGET_WASM)
  set(PLAYFIELD_TARGET_WASM ON)
endif()
if(SUIKA_TARGET_WASM_LOCAL)
  set(PLAYFIELD_TARGET_WASM_LOCAL ON)
endif()

# If building for Unity.
if(SUIKA_TARGET_UNITY)
  set(PLAYFIELD_TARGET_UNITY ON)
endif()

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

#
# Apple Quirks
#

if(SUIKA_TARGET_MACOS)
  set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
endif()

#
# Dependencies
#

# PlayfieldEngine
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/PlayfieldEngine)

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
  src/translation.c
  src/vars.c
  src/cmd.c
  src/cmd_bg.c
  src/cmd_bgm.c
  src/cmd_ch.c
  src/cmd_choose.c
  src/cmd_click.c
  src/cmd_else.c
  src/cmd_elseif.c
  src/cmd_endif.c
  src/cmd_goto.c
  src/cmd_gui.c
  src/cmd_if.c
  src/cmd_label.c
  src/cmd_load.c
  src/cmd_set.c
  src/cmd_text.c
  src/cmd_volume.c
  src/cmd_wait.c
)

# I18N Source
#if(SUIKA_ENABLE_I18N)
#  set(SUIKA_I18N_SOURCES
#    src/i18n.c
#    src/translation.c
#  )
#endif()

# Resource
if(SUIKA_TARGET_WINDOWS AND NOT SUIKA3_ENABLE_STATIC)
  set(SUIKA_RESOURCES
    resources/windows/resource.rc
  )
endif()

# Add Target
if(   SUIKA_TARGET_IOS
   OR SUIKA_TARGET_ANDROID
   OR SUIKA_TARGET_UNITY
   OR SUIKA_ENABLE_STATIC
)
  # Make a library.
  if(SUIKA_ENABLE_SHARED)
    set(SUIKA_LIB_TYPE SHARED)
  else()
    set(SUIKA_LIB_TYPE STATIC)
  endif()
  add_library(
    suika3
    ${SUIKA_LIB_TYPE}
    ${SUIKA_BASE_SOURCES}
    ${SUIKA_I18N_SOURCES}
    ${SUIKA_RESOURCES}
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
  # Make an executable.
  add_executable(
    suika3
    ${SUIKA_BASE_SOURCES}
    ${SUIKA_I18N_SOURCES}
    ${SUIKA_RESOURCES}
  )
endif()

# Link PlayfieldEngine
target_link_libraries(suika3 PRIVATE
  playfield
  strato
  playfield  # Specify twice to resolve circular references.
  noctlang
)

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

# Unity
if(SUIKA_TARGET_UNITY)
  target_compile_definitions(suika3 PRIVATE S3_TARGET_UNITY PF_TARGET_UNITY HAL_TARGET_UNITY)
endif()

# Windows quirks
if(SUIKA_TARGET_WINDOWS)
  if(MSVC)
    target_compile_options(suika3 PRIVATE /utf-8)
    target_link_options(suika3 PRIVATE /SUBSYSTEM:WINDOWS)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_options(suika3 PRIVATE -municode -finput-charset=utf-8 -fexec-charset=utf-8)
    target_link_options(suika3 PRIVATE -mwindows -static -Wl,-u,wWinMain -Wl,-u,_wWinMain@16)
  endif()
endif()

# Emscripten
if(SUIKA_TARGET_WASM)
  # Make the output name "index.html"
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  set_target_properties(suika3 PROPERTIES OUTPUT_NAME index)

  # -DUSE_EMSCRIPTEN
  target_compile_definitions(suika3 PRIVATE USE_EMSCRIPTEN)

  # Link options.
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

# Emscripten
if(SUIKA_TARGET_WASM_LOCAL)
  # Make the output name "index.html"
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  set_target_properties(suika3 PROPERTIES OUTPUT_NAME index)

  # -DUSE_EMSCRIPTEN
  target_compile_definitions(suika3 PRIVATE USE_EMSCRIPTEN)

  # Link options.
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

#
# Packager Target
#

if(SUIKA_ENABLE_PACK)
  if (NOT SUIKA_TARGET_WINDOWS)
    add_executable(
      suika3-pack
      external/PlayfieldEngine/src/pack.c
    )
  else()
    add_executable(
      suika3-pack
      external/PlayfieldEngine/src/pack.c
      external/PlayfieldEngine/resources/windows/pack-resource.rc
    )
  endif()
  target_link_libraries(suika3-pack stratopack)
endif()

#
# Compiler Target
#

if(   SUIKA_TARGET_WINDOWS
   OR SUIKA_TARGET_MACOS
   OR SUIKA_TARGET_LINUX
   OR SUIKA_TARGET_LINUX_WAYLAND
   OR SUIKA_TARGET_LINUX_GBM
   OR SUIKA_TARGET_FREEBSD
   OR SUIKA_TARGET_FREEBSD_WAYLAND
   OR SUIKA_TARGET_NETBSD
   OR SUIKA_TARGET_OPENBSD)
  add_executable(
    suika3-compiler
    external/PlayfieldEngine/src/compiler.c
  )
  target_include_directories(suika3-compiler PRIVATE
    external/PlayfieldEngine/external/NoctLang/src/core
    external/PlayfieldEngine/external/NoctLang/src/backend
  )
  target_link_libraries(suika3-compiler noctlang)
  if(UNIX)
    target_link_libraries(suika3-compiler m)
  endif()
endif()

#
# Web Server Target
#

if(SUIKA_TARGET_WINDOWS)
  add_executable(
    suika3-web
    external/PlayfieldEngine/src/webserver.c
    external/PlayfieldEngine/resources/windows/web-resource.rc
  )
  target_link_libraries(suika3-web wsock32 ws2_32)
endif()

#
# macOS Bundle
#

if(SUIKA_TARGET_MACOS AND SUIKA_ENABLE_BUNDLE)
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
  set(APP_GAME_DATA "${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/assets.arc")
  set_source_files_properties(${APP_GAME_DATA} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
  )
  target_sources(suika3 PRIVATE ${APP_GAME_DATA})
endif()

#
# Install
#

if (   SUIKA_TARGET_LINUX
    OR SUIKA_TARGET_LINUX_WAYLAND
    OR SUIKA_TARGET_LINUX_GBM
    OR SUIKA_TARGET_FREEBSD
    OR SUIKA_TARGET_FREEBSD_WAYLAND
    OR SUIKA_TARGET_NETBSD
    OR SUIKA_TARGET_OPENBSD)

  include(GNUInstallDirs)

  install(TARGETS suika3          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(TARGETS suika3-compiler RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(TARGETS suika3-pack     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  if (   SUIKA_TARGET_FREEBSD
      OR SUIKA_TARGET_FREEBSD_WAYLAND
      OR SUIKA_TARGET_NETBSD
      OR SUIKA_TARGET_OPENBSD)
    install(
      FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3.1.mdoc"
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
      RENAME      "suika3.1"
    )
    install(
      FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3-compiler.1.mdoc"
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
      RENAME      "suika3-compiler.1"
    )
    install(
      FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3-pack.1.mdoc"
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
      RENAME      "suika3-pack.1"
    )
  else()
    install(
      FILES       "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3.1"
                  "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3-compiler.1"
                  "${CMAKE_CURRENT_SOURCE_DIR}/resources/manpage/suika3-pack.1"
      DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    )
  endif()
endif()
