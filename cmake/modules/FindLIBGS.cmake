# - Try to find the libgs library
# Once done this will define
#
#  LIBGS_FOUND - system has libgs
#  LIBGS_LIBRARY - Link this to use libgs
#
include(CheckLibraryExists)

# reset vars
set(LIBGS_LIBRARY)

set(CMAKE_LIBRARY_PATH "${LIB_INSTALL_DIR}")
find_library(LIBGS_LIBRARY NAMES gs gs-esp
  PATHS
  /usr/lib
  /usr/local/lib
  ${GNUWIN32_DIR}/lib
)

if(LIBGS_LIBRARY)
  check_library_exists(${LIBGS_LIBRARY} gsapi_new_instance "" LIBGS_HAVE_GSAPI_NEW_INSTANCE)

  if(LIBGS_HAVE_GSAPI_NEW_INSTANCE)
    set(LIBGS_FOUND TRUE)
  endif(LIBGS_HAVE_GSAPI_NEW_INSTANCE)
endif(LIBGS_LIBRARY)
set(CMAKE_LIBRARY_PATH)

if (LIBGS_FOUND)
  if (NOT LIBGS_FIND_QUIETLY)
    message(STATUS "Found the GS library: ${LIBGS_LIBRARY}")
  endif (NOT LIBGS_FIND_QUIETLY)
else (LIBGS_FOUND)
  if (LIBGS_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find the GS library")
  endif (LIBGS_FIND_REQUIRED)
endif (LIBGS_FOUND)

MARK_AS_ADVANCED(LIBGS_LIBRARY)
