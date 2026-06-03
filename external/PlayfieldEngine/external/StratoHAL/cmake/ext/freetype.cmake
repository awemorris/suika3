# Extract library archive.
file(ARCHIVE_EXTRACT
  INPUT       ${CMAKE_CURRENT_SOURCE_DIR}/lib/archive/freetype-2.13.3.tar.gz
  DESTINATION ${CMAKE_BINARY_DIR}
)

# Rename it to "freetype".
file(GLOB LIBPNG_EXTRACTED_DIR ${CMAKE_BINARY_DIR}/freetype-*)
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/freetype)
file(RENAME ${LIBPNG_EXTRACTED_DIR} ${CMAKE_BINARY_DIR}/freetype)
file(WRITE
  ${CMAKE_BINARY_DIR}/freetype/include/freetype/config/ftmodule.h
"
#ifdef FT_USE_MODULE
FT_USE_MODULE( FT_Driver_ClassRec, tt_driver_class )
FT_USE_MODULE( FT_Module_Class, sfnt_module_class )
FT_USE_MODULE( FT_Renderer_Class, ft_smooth_renderer_class )
#endif
"
)
file(APPEND
  ${CMAKE_BINARY_DIR}/freetype/include/freetype/config/ftoption.h
"
#define FT_CONFIG_OPTION_NO_ASSEMBLER
#undef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
#undef TT_CONFIG_OPTION_SUBPIXEL_HINTING
"
)

# Disable gzip
file(READ
  ${CMAKE_BINARY_DIR}/freetype/include/freetype/config/ftoption.h
  FTOPTION_H
)
string(REPLACE
  "#define FT_CONFIG_OPTION_USE_ZLIB"
  "/* #undef FT_CONFIG_OPTION_USE_ZLIB */"
  FTOPTION_H
  "${FTOPTION_H}"
)
file(WRITE
  ${CMAKE_BINARY_DIR}/freetype/include/freetype/config/ftoption.h
  "${FTOPTION_H}"
)

# Source list.
add_library(freetype OBJECT
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftbase.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftinit.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftsystem.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftdebug.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftbbox.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftbitmap.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftglyph.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftstroke.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftmm.c
  ${CMAKE_BINARY_DIR}/freetype/src/sfnt/sfnt.c
  ${CMAKE_BINARY_DIR}/freetype/src/smooth/smooth.c
  ${CMAKE_BINARY_DIR}/freetype/src/truetype/truetype.c
)

target_include_directories(freetype PUBLIC
  ${CMAKE_BINARY_DIR}/freetype/include
)
set(FREETYPE_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/freetype/include)

target_compile_definitions(freetype PRIVATE
  ft_memcpy=memcpy
  FT2_BUILD_LIBRARY
)

if(MSVC)
  target_compile_options(freetype PUBLIC /utf-8)
endif()

# Suppress compilation errors.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(freetype PRIVATE -std=gnu89 -w)
elseif(MSVC)
  target_compile_options(freetype PRIVATE /W0 /wd4267 /wd4334)
endif()
