# Extract library archive.
file(ARCHIVE_EXTRACT
  INPUT       ${CMAKE_CURRENT_SOURCE_DIR}/lib/archive/freetype-2.13.3.tar.gz
  DESTINATION ${CMAKE_BINARY_DIR}
)

# Rename it to "freetype".
file(GLOB LIBPNG_EXTRACTED_DIR ${CMAKE_BINARY_DIR}/freetype-*)
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/freetype)
file(RENAME ${LIBPNG_EXTRACTED_DIR} ${CMAKE_BINARY_DIR}/freetype)

# Source list.
add_library(freetype OBJECT
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftbase.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftinit.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftsystem.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftbbox.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftbitmap.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftglyph.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftstroke.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftmm.c
  ${CMAKE_BINARY_DIR}/freetype/src/base/ftdebug.c
  ${CMAKE_BINARY_DIR}/freetype/src/bdf/bdf.c
  ${CMAKE_BINARY_DIR}/freetype/src/bzip2/ftbzip2.c
  ${CMAKE_BINARY_DIR}/freetype/src/cache/ftcache.c
  ${CMAKE_BINARY_DIR}/freetype/src/cff/cff.c
  ${CMAKE_BINARY_DIR}/freetype/src/cid/type1cid.c
  ${CMAKE_BINARY_DIR}/freetype/src/otvalid/otvalid.c
  ${CMAKE_BINARY_DIR}/freetype/src/pfr/pfr.c
  ${CMAKE_BINARY_DIR}/freetype/src/psaux/psaux.c
  ${CMAKE_BINARY_DIR}/freetype/src/pshinter/pshinter.c
  ${CMAKE_BINARY_DIR}/freetype/src/psnames/psnames.c
  ${CMAKE_BINARY_DIR}/freetype/src/raster/raster.c
  ${CMAKE_BINARY_DIR}/freetype/src/sdf/sdf.c
  ${CMAKE_BINARY_DIR}/freetype/src/sfnt/sfnt.c
  ${CMAKE_BINARY_DIR}/freetype/src/smooth/smooth.c
  ${CMAKE_BINARY_DIR}/freetype/src/svg/svg.c
  ${CMAKE_BINARY_DIR}/freetype/src/truetype/truetype.c
  ${CMAKE_BINARY_DIR}/freetype/src/winfonts/winfnt.c
  ${CMAKE_BINARY_DIR}/freetype/src/lzw/ftlzw.c
  ${CMAKE_BINARY_DIR}/freetype/src/pcf/pcf.c
  ${CMAKE_BINARY_DIR}/freetype/src/autofit/autofit.c
  ${CMAKE_BINARY_DIR}/freetype/src/type1/type1.c
  ${CMAKE_BINARY_DIR}/freetype/src/type42/type42.c
  ${CMAKE_BINARY_DIR}/freetype/src/svg/svg.c
  ${CMAKE_BINARY_DIR}/freetype/src/gzip/ftgzip.c
)

target_include_directories(freetype PUBLIC
  ${CMAKE_BINARY_DIR}/freetype/include
)
set(FREETYPE_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/freetype/include)

target_compile_definitions(freetype PRIVATE
  FT2_BUILD_LIBRARY
  FT_CONFIG_OPTION_SYSTEM_ZLIB
  ft_memcpy=memcpy
)

if(MSVC)
  target_compile_options(freetype PUBLIC /utf-8)
endif()

target_link_libraries(freetype PRIVATE
  brotlidec
  brotlicommon
  z
)

# Suppress compilation errors.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(freetype PRIVATE -std=gnu89 -w)
elseif(MSVC)
  target_compile_options(freetype PRIVATE /W0 /wd4267 /wd4334)
endif()
