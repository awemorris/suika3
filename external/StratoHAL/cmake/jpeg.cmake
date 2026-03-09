file(ARCHIVE_EXTRACT
  INPUT       ${CMAKE_CURRENT_SOURCE_DIR}/lib/archive/jpegsrc.v9e.tar.gz
  DESTINATION ${CMAKE_BINARY_DIR}
)

file(GLOB LIBPNG_EXTRACTED_DIR ${CMAKE_BINARY_DIR}/jpeg-*)
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/jpeg)
file(RENAME ${LIBPNG_EXTRACTED_DIR} ${CMAKE_BINARY_DIR}/jpeg)

file(RENAME
  ${CMAKE_BINARY_DIR}/jpeg/jconfig.txt
  ${CMAKE_BINARY_DIR}/jpeg/jconfig.h
)

file(GLOB LIBPNG_HEADERS ${CMAKE_BINARY_DIR}/jpeg/*.h)
file(
  COPY        ${LIBPNG_HEADERS}
  DESTINATION ${CMAKE_BINARY_DIR}/jpeg/jpeg
)

add_library(jpeg OBJECT
  ${CMAKE_BINARY_DIR}/jpeg/cdjpeg.c
  ${CMAKE_BINARY_DIR}/jpeg/jaricom.c
  ${CMAKE_BINARY_DIR}/jpeg/jcapimin.c
  ${CMAKE_BINARY_DIR}/jpeg/jcapistd.c
  ${CMAKE_BINARY_DIR}/jpeg/jcarith.c
  ${CMAKE_BINARY_DIR}/jpeg/jccoefct.c
  ${CMAKE_BINARY_DIR}/jpeg/jccolor.c
  ${CMAKE_BINARY_DIR}/jpeg/jcdctmgr.c
  ${CMAKE_BINARY_DIR}/jpeg/jchuff.c
  ${CMAKE_BINARY_DIR}/jpeg/jcinit.c
  ${CMAKE_BINARY_DIR}/jpeg/jcmainct.c
  ${CMAKE_BINARY_DIR}/jpeg/jcmarker.c
  ${CMAKE_BINARY_DIR}/jpeg/jcmaster.c
  ${CMAKE_BINARY_DIR}/jpeg/jcomapi.c
  ${CMAKE_BINARY_DIR}/jpeg/jcparam.c
  ${CMAKE_BINARY_DIR}/jpeg/jcprepct.c
  ${CMAKE_BINARY_DIR}/jpeg/jcsample.c
  ${CMAKE_BINARY_DIR}/jpeg/jctrans.c
  ${CMAKE_BINARY_DIR}/jpeg/jdapimin.c
  ${CMAKE_BINARY_DIR}/jpeg/jdapistd.c
  ${CMAKE_BINARY_DIR}/jpeg/jdarith.c
  ${CMAKE_BINARY_DIR}/jpeg/jdatadst.c
  ${CMAKE_BINARY_DIR}/jpeg/jdatasrc.c
  ${CMAKE_BINARY_DIR}/jpeg/jdcoefct.c
  ${CMAKE_BINARY_DIR}/jpeg/jdcolor.c
  ${CMAKE_BINARY_DIR}/jpeg/jddctmgr.c
  ${CMAKE_BINARY_DIR}/jpeg/jdhuff.c
  ${CMAKE_BINARY_DIR}/jpeg/jdinput.c
  ${CMAKE_BINARY_DIR}/jpeg/jdmainct.c
  ${CMAKE_BINARY_DIR}/jpeg/jdmarker.c
  ${CMAKE_BINARY_DIR}/jpeg/jdmaster.c
  ${CMAKE_BINARY_DIR}/jpeg/jdmerge.c
  ${CMAKE_BINARY_DIR}/jpeg/jdpostct.c
  ${CMAKE_BINARY_DIR}/jpeg/jdsample.c
  ${CMAKE_BINARY_DIR}/jpeg/jdtrans.c
  ${CMAKE_BINARY_DIR}/jpeg/jerror.c
  ${CMAKE_BINARY_DIR}/jpeg/jfdctflt.c
  ${CMAKE_BINARY_DIR}/jpeg/jfdctfst.c
  ${CMAKE_BINARY_DIR}/jpeg/jfdctint.c
  ${CMAKE_BINARY_DIR}/jpeg/jidctflt.c
  ${CMAKE_BINARY_DIR}/jpeg/jidctfst.c
  ${CMAKE_BINARY_DIR}/jpeg/jidctint.c
  ${CMAKE_BINARY_DIR}/jpeg/jmemmgr.c
  ${CMAKE_BINARY_DIR}/jpeg/jmemnobs.c
  ${CMAKE_BINARY_DIR}/jpeg/jquant1.c
  ${CMAKE_BINARY_DIR}/jpeg/jquant2.c
  ${CMAKE_BINARY_DIR}/jpeg/jutils.c
  ${CMAKE_BINARY_DIR}/jpeg/transupp.c
)

target_include_directories(jpeg PUBLIC ${CMAKE_BINARY_DIR}/jpeg)
set(JPEG_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/jpeg)

# Suppress compilation errors.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(jpeg PRIVATE -std=c89 -w)
elseif(MSVC)
  target_compile_options(jpeg PRIVATE /W0 /wd4267 /wd4244)
endif()
