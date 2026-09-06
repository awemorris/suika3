find_path(TREMOR_INCLUDE_DIR tremor/ivorbiscodec.h)

find_library(TREMOR_LIBRARY NAMES vorbisidec)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Tremor DEFAULT_MSG TREMOR_LIBRARY TREMOR_INCLUDE_DIR)

set(TREMOR_LIBRARIES ${TREMOR_LIBRARY})
set(TREMOR_INCLUDE_DIRS ${TREMOR_INCLUDE_DIR})
