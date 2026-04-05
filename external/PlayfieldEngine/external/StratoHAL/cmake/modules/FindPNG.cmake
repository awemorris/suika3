find_path(PNG_INCLUDE_DIR png.h)

find_library(PNG_LIBRARY NAMES png png_static png16 libpng libpng_static libpng16)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PNG DEFAULT_MSG PNG_LIBRARY PNG_INCLUDE_DIR)

set(PNG_INCLUDE_DIRS ${PNG_INCLUDE_DIR})
set(PNG_LIBRARIES ${PNG_LIBRARY})
