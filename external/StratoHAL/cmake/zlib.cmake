# Extract archive.
file(ARCHIVE_EXTRACT
  INPUT       ${CMAKE_CURRENT_SOURCE_DIR}/lib/archive/zlib-1.3.1.tar.gz
  DESTINATION ${CMAKE_BINARY_DIR}
)

# Rename it to "zlib".
file(GLOB ZLIB_EXTRACTED_DIR ${CMAKE_BINARY_DIR}/zlib-*)
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/zlib)
file(RENAME ${ZLIB_EXTRACTED_DIR} ${CMAKE_BINARY_DIR}/zlib)

# Source list.
add_library(z OBJECT
  ${CMAKE_BINARY_DIR}/zlib/adler32.c
  ${CMAKE_BINARY_DIR}/zlib/infback.c
  ${CMAKE_BINARY_DIR}/zlib/trees.c
  ${CMAKE_BINARY_DIR}/zlib/compress.c
  ${CMAKE_BINARY_DIR}/zlib/inffast.c
  ${CMAKE_BINARY_DIR}/zlib/uncompr.c
  ${CMAKE_BINARY_DIR}/zlib/crc32.c
  ${CMAKE_BINARY_DIR}/zlib/inflate.c
  ${CMAKE_BINARY_DIR}/zlib/deflate.c
  ${CMAKE_BINARY_DIR}/zlib/inftrees.c
  ${CMAKE_BINARY_DIR}/zlib/zutil.c
)

# Include directory.
target_include_directories(z PUBLIC ${CMAKE_BINARY_DIR}/zlib)

# Suppress compilation errors.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(z PRIVATE -std=c89 -w)
elseif(MSVC)
  target_compile_options(z PRIVATE /W0 /wd4267 /wd4334)
endif()
