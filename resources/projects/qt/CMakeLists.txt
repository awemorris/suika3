cmake_minimum_required(VERSION 3.16)
project(noctvm LANGUAGES C CXX)

set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)

set(CMAKE_AUTOMOC ON)

#
# Libraries
#

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../../cmake/modules")

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets Gui Multimedia)

find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(WebP REQUIRED)
find_package(Vorbis MODULE REQUIRED)
find_package(Ogg REQUIRED)
find_package(Freetype REQUIRED)
find_package(Brotli REQUIRED)
find_package(BZip2 REQUIRED)
find_package(ZLIB REQUIRED)

#
# Definitions
#

add_definitions(-DUSE_QT)

#
# Include
#

include_directories(../../external/NoctLang/include)
include_directories(../../external/StratoHAL/include)

include_directories(${PNG_INCLUDE_DIRS})
include_directories(${JPEG_INCLUDE_DIRS})
include_directories(${WEBP_INCLUDE_DIRS})
include_directories(${OGG_INCLUDE_DIRS})
include_directories(${VORBIS_INCLUDE_DIRS})
include_directories(${FREETYPE_INCLUDE_DIRS})
include_directories(${BROTLI_INCLUDE_DIRS})
include_directories(${BZIP2_INCLUDE_DIRS})
include_directories(${ZLIB_INCLUDE_DIRS})

#
# Source
#

# Main sources.
set(SOURCES
    ../../external/NoctLang/src/lexer.yy.c
    ../../external/NoctLang/src/parser.tab.c
    ../../external/NoctLang/src/ast.c
    ../../external/NoctLang/src/hir.c
    ../../external/NoctLang/src/lir.c
    ../../external/NoctLang/src/runtime.c
    ../../external/NoctLang/src/interpreter.c
    ../../external/NoctLang/src/execution.c
    ../../external/NoctLang/src/intrinsics.c
    ../../external/NoctLang/src/jit.c
    ../../external/StratoHAL/src/image.c
    ../../external/StratoHAL/src/glyph.c
    ../../external/StratoHAL/src/wave.c
    ../../external/StratoHAL/src/glrender.c
    ../../external/StratoHAL/src/qtgamewidget.cpp
    ../../external/StratoHAL/src/qtmain.cpp
    ../../src/main.c
    ../../src/api.c
    ../../src/vm.c
)

# Finalize.
add_executable(noctvm ${SOURCES})

#
# Link
#

target_link_libraries(noctvm PRIVATE
  Qt${QT_VERSION_MAJOR}::Widgets
  Qt${QT_VERSION_MAJOR}::OpenGLWidgets
  Qt${QT_VERSION_MAJOR}::Multimedia

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
