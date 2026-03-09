file(ARCHIVE_EXTRACT
  INPUT       ${CMAKE_CURRENT_SOURCE_DIR}/lib/archive/libpng-1.6.43.tar.gz
  DESTINATION ${CMAKE_BINARY_DIR}
)

file(GLOB LIBPNG_EXTRACTED_DIR ${CMAKE_BINARY_DIR}/libpng-*)
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/libpng)
file(RENAME ${LIBPNG_EXTRACTED_DIR} ${CMAKE_BINARY_DIR}/libpng)

file(
  COPY        ${CMAKE_BINARY_DIR}/libpng/scripts/pnglibconf.h.prebuilt
  DESTINATION ${CMAKE_BINARY_DIR}/libpng
)
file(RENAME
  ${CMAKE_BINARY_DIR}/libpng/pnglibconf.h.prebuilt
  ${CMAKE_BINARY_DIR}/libpng/pnglibconf.h
)

file(GLOB LIBPNG_HEADERS ${CMAKE_BINARY_DIR}/libpng/*.h)
file(
  COPY        ${LIBPNG_HEADERS}
  DESTINATION ${CMAKE_BINARY_DIR}/libpng/png
)

add_library(png OBJECT
  ${CMAKE_BINARY_DIR}/libpng/pngget.c
  ${CMAKE_BINARY_DIR}/libpng/pngread.c
  ${CMAKE_BINARY_DIR}/libpng/pngrutil.c
  ${CMAKE_BINARY_DIR}/libpng/pngtrans.c
  ${CMAKE_BINARY_DIR}/libpng/pngwtran.c
  ${CMAKE_BINARY_DIR}/libpng/png.c
  ${CMAKE_BINARY_DIR}/libpng/pngmem.c
  ${CMAKE_BINARY_DIR}/libpng/pngrio.c
  ${CMAKE_BINARY_DIR}/libpng/pngset.c
  ${CMAKE_BINARY_DIR}/libpng/pngwio.c
  ${CMAKE_BINARY_DIR}/libpng/pngwutil.c
  ${CMAKE_BINARY_DIR}/libpng/pngerror.c
  ${CMAKE_BINARY_DIR}/libpng/pngpread.c
  ${CMAKE_BINARY_DIR}/libpng/pngrtran.c
  ${CMAKE_BINARY_DIR}/libpng/pngwrite.c
  ${CMAKE_BINARY_DIR}/libpng/arm/arm_init.c
  ${CMAKE_BINARY_DIR}/libpng/arm/filter_neon_intrinsics.c
  ${CMAKE_BINARY_DIR}/libpng/arm/filter_neon.S
  ${CMAKE_BINARY_DIR}/libpng/arm/palette_neon_intrinsics.c
  ${CMAKE_BINARY_DIR}/libpng/intel/intel_init.c
  ${CMAKE_BINARY_DIR}/libpng/intel/filter_sse2_intrinsics.c
)

target_include_directories(png PRIVATE ${CMAKE_BINARY_DIR}/libpng)
target_include_directories(png PUBLIC  ${CMAKE_BINARY_DIR}/libpng)
set(PNG_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/libpng)

target_link_libraries(png PRIVATE z)

#target_compile_definitions(png PRIVATE PNG_ARM_NEON_OPT=0)

# Suppress compilation errors.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(png PRIVATE -std=c89 -w)
elseif(MSVC)
  target_compile_options(png PRIVATE /W0 /wd4267 /wd4334)
endif()
