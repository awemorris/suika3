find_path(BROTLI_INCLUDE_DIR brotli/decode.h)

find_library(BROTLIDEC_LIBRARY NAMES brotlidec brotlidec_static libbrotlidec libbrotlidec_static)
find_library(BROTLICOMMON_LIBRARY NAMES brotlicommon brotlicommon_static libbrotlicommon libbrotlicommon_static)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Brotli DEFAULT_MSG BROTLIDEC_LIBRARY BROTLICOMMON_LIBRARY BROTLI_INCLUDE_DIR)

set(BROTLIDEC_LIBRARIES ${BROTLIDEC_LIBRARY})
set(BROTLICOMMON_LIBRARIES ${BROTLICOMMON_LIBRARY})
set(BROTLI_INCLUDE_DIRS ${BRTOLI_INCLUDE_DIR})
