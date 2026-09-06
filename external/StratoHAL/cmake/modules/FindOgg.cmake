find_path(OGG_INCLUDE_DIR ogg/ogg.h)

find_library(OGG_LIBRARY NAMES ogg ogg_static libogg libogg_static)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ogg DEFAULT_MSG OGG_LIBRARY OGG_INCLUDE_DIR)

set(OGG_INCLUDE_DIRS ${OGG_INCLUDE_DIR})
set(OGG_LIBRARIES ${OGG_LIBRARY})
