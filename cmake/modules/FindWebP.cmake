find_path(WEBP_INCLUDE_DIR brotli/decode.h)

find_library(WEBP_LIBRARY NAMES webp webp_static libwebp libwebp_static)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WebP DEFAULT_MSG WEBP_LIBRARY WEBP_INCLUDE_DIR)

set(WEBP_LIBRARIES ${WEBP_LIBRARY})
set(WEBP_INCLUDE_DIRS ${WEBP_INCLUDE_DIR})
