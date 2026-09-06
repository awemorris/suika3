# Noct object-only cross build for the PC-98 Bootstrap Environment.
# The resulting objects are linked by linux-pc98/bootloader, not by CMake.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i386)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER gcc CACHE FILEPATH "C compiler for PC98BE")

set(PC98BE_I386_FLAGS
    "-m32 -march=i386 -Os -ffreestanding -fno-pic -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-isolate-erroneous-paths-dereference -msoft-float -mno-80387 -mno-fp-ret-in-387 -mno-mmx -mno-sse -mno-sse2")
set(CMAKE_C_FLAGS_INIT "${PC98BE_I386_FLAGS}")
