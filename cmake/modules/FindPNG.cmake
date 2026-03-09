find_path(PNG_INCLUDE_DIR png.h)

find_library(PNG_LIBRARY NAMES png png_static libpng libpng_static)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PNG DEFAULT_MSG PNG_LIBRARY PNG_INCLUDE_DIR)

set(PNG_INCLUDE_DIRS ${PNG_INCLUDE_DIR})
set(PNG_LIBRARIES ${PNG_LIBRARY})
