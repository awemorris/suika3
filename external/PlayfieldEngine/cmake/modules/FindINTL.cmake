find_path(INTL_INCLUDE_DIR libintl.h)

find_library(INTL_LIBRARY_RAW NAMES intl)

if(INTL_LIBRARY_RAW)
    set(INTL_LIBRARY ${INTL_LIBRARY_RAW})
else()
    include(CheckFunctionExists)
    check_function_exists(gettext HAVE_GETTEXT_IN_LIBC)
    if(HAVE_GETTEXT_IN_LIBC)
        set(INTL_LIBRARY "") 
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(INTL DEFAULT_MSG INTL_INCLUDE_DIR)

set(INTL_INCLUDE_DIRS ${INTL_INCLUDE_DIR})
set(INTL_LIBRARIES ${INTL_LIBRARY})
