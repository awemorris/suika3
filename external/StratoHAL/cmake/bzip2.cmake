# Extract archive.
file(ARCHIVE_EXTRACT
  INPUT       ${CMAKE_CURRENT_SOURCE_DIR}/lib/archive/bzip2-1.0.6.tar.gz
  DESTINATION ${CMAKE_BINARY_DIR}
)

# Rename it to "bzip2".
file(GLOB BZIP2_EXTRACTED_DIR ${CMAKE_BINARY_DIR}/bzip2-*)
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/bzip2)
file(RENAME ${BZIP2_EXTRACTED_DIR} ${CMAKE_BINARY_DIR}/bzip2)

# Source list.
add_library(bz2 OBJECT
  ${CMAKE_BINARY_DIR}/bzip2/blocksort.c
  ${CMAKE_BINARY_DIR}/bzip2/compress.c
  ${CMAKE_BINARY_DIR}/bzip2/decompress.c
  ${CMAKE_BINARY_DIR}/bzip2/huffman.c
  ${CMAKE_BINARY_DIR}/bzip2/randtable.c
  ${CMAKE_BINARY_DIR}/bzip2/bzlib.c
  ${CMAKE_BINARY_DIR}/bzip2/crctable.c
)

# Include directory.
target_include_directories(bz2 PUBLIC ${CMAKE_BINARY_DIR}/bzip2)

# Suppress compilation errors.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(bz2 PRIVATE -std=c89 -w)
elseif(MSVC)
  target_compile_options(bz2 PRIVATE /W0 /wd4267 /wd4334)
endif()

target_compile_definitions(bz2 PRIVATE BZ_STRICT_ANSI)
