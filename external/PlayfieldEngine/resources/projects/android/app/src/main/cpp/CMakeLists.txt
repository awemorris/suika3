cmake_minimum_required(VERSION 3.22)

project("playfield")

# Find the Playfield package from the AAR file.
find_package(playfield REQUIRED CONFIG)

# Define the libstrato.so
add_library(
  strato
  SHARED
  library.c  # AOT stub
)

# Link libplayfield.a in the AAR to libstrato.so
target_link_libraries(
  strato
  PRIVATE

  -Wl,--whole-archive
  playfield::playfield
  -Wl,--no-whole-archive

  android
  log
  m
  GLESv3
  OpenSLES
  log
)

# Do not remove libplayfield.a sections
target_link_options(strato PRIVATE
  "LINKER:--no-gc-sections"
)
