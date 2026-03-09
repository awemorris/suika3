set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR powerpc64le)

set(CMAKE_C_COMPILER powerpc64le-linux-gnu-gcc)
set(CMAKE_FIND_ROOT_PATH /usr/powerpc64le-linux-gnu)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
set(BUILD_SHARED_LIBS OFF)
