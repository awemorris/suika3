file(ARCHIVE_EXTRACT
  INPUT       ${CMAKE_CURRENT_SOURCE_DIR}/lib/archive/libvorbis-1.3.7.tar.gz
  DESTINATION ${CMAKE_BINARY_DIR}
)

file(GLOB LIBPNG_EXTRACTED_DIR ${CMAKE_BINARY_DIR}/libvorbis-*)
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/libvorbis)
file(RENAME ${LIBPNG_EXTRACTED_DIR} ${CMAKE_BINARY_DIR}/libvorbis)

add_library(vorbisfile OBJECT
  ${CMAKE_BINARY_DIR}/libvorbis/lib/vorbisfile.c
)

add_library(vorbis OBJECT
  ${CMAKE_BINARY_DIR}/libvorbis/lib/analysis.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/bitrate.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/block.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/codebook.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/envelope.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/floor0.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/floor1.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/info.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/lookup.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/lpc.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/lsp.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/mapping0.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/mdct.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/psy.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/registry.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/res0.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/sharedbook.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/smallft.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/synthesis.c
  #${CMAKE_BINARY_DIR}/libvorbis/lib/vorbisenc.c
  ${CMAKE_BINARY_DIR}/libvorbis/lib/window.c
)

target_include_directories(vorbisfile PRIVATE ${CMAKE_BINARY_DIR}/libogg/include)
target_include_directories(vorbisfile PRIVATE ${CMAKE_BINARY_DIR}/libvorbis/include)
target_include_directories(vorbisfile PUBLIC  ${CMAKE_BINARY_DIR}/libvorbis/include)
set(VORBIS_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/libvorbis/include)

target_include_directories(vorbis PRIVATE ${CMAKE_BINARY_DIR}/libogg/include)
target_include_directories(vorbis PRIVATE ${CMAKE_BINARY_DIR}/libvorbis/include)
target_include_directories(vorbis PUBLIC  ${CMAKE_BINARY_DIR}/libvorbis/include)

target_link_libraries(vorbis PRIVATE ogg)

# Suppress compilation errors.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(vorbis PRIVATE -std=gnu89 -w)
  target_compile_options(vorbisfile PRIVATE -std=gnu89 -w)
elseif(MSVC)
  target_compile_options(vorbis PRIVATE /W0 /wd4267 /wd4244)
  target_compile_options(vorbisfile PRIVATE /W0 /wd4267 /wd4244)
endif()
