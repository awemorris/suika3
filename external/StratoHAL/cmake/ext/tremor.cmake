file(
  COPY        ${CMAKE_CURRENT_SOURCE_DIR}/lib/external/tremor/
  DESTINATION ${CMAKE_BINARY_DIR}/tremor
)

add_library(tremor OBJECT
  ${CMAKE_BINARY_DIR}/tremor/mapping0.c
  ${CMAKE_BINARY_DIR}/tremor/mdct.c
  ${CMAKE_BINARY_DIR}/tremor/floor0.c
  ${CMAKE_BINARY_DIR}/tremor/sharedbook.c
  ${CMAKE_BINARY_DIR}/tremor/registry.c
  ${CMAKE_BINARY_DIR}/tremor/res012.c
  ${CMAKE_BINARY_DIR}/tremor/floor1.c
  ${CMAKE_BINARY_DIR}/tremor/vorbisfile.c
  ${CMAKE_BINARY_DIR}/tremor/info.c
  ${CMAKE_BINARY_DIR}/tremor/block.c
  ${CMAKE_BINARY_DIR}/tremor/window.c
  ${CMAKE_BINARY_DIR}/tremor/synthesis.c
  ${CMAKE_BINARY_DIR}/tremor/codebook.c
)

target_include_directories(tremor PRIVATE ${CMAKE_BINARY_DIR}/libogg/include)
target_include_directories(tremor PRIVATE ${CMAKE_BINARY_DIR}/tremor/include/tremor)
target_include_directories(tremor PUBLIC  ${CMAKE_BINARY_DIR}/tremor/include)
set(TREMOR_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/tremor/include)

target_link_libraries(tremor PRIVATE ogg)

# Suppress compilation errors.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(tremor PRIVATE -std=gnu89 -w)
elseif(MSVC)
  target_compile_options(tremor PRIVATE /W0 /wd4267 /wd4244)
endif()

if(STRATO_TARGET_PC98 OR STRATO_TARGET_PCAT)
  target_compile_definitions(tremor PUBLIC _LOW_ACCURACY_)
  target_compile_definitions(tremor PUBLIC HAVE_ALLOCA_H)
  target_compile_definitions(tremor PUBLIC STIN=static)
endif()
