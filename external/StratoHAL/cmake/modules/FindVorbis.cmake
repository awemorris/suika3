find_path(VORBIS_INCLUDE_DIR vorbis/vorbisfile.h)

find_library(VORBIS_LIBRARY NAMES vorbis vorbis_static libvorbis libvorbis_static)
find_library(VORBISFILE_LIBRARY NAMES vorbisfile vorbisfile_static libvorbisfile libvorbisfile_static)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vorbis DEFAULT_MSG VORBIS_LIBRARY VORBISFILE_LIBRARY VORBIS_INCLUDE_DIR)

set(VORBIS_LIBRARIES ${VORBIS_LIBRARY})
set(VORBIS_INCLUDE_DIRS ${VORBIS_INCLUDE_DIR})
