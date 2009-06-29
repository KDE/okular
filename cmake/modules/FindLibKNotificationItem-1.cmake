# - Find libknotification-1 library
# Find the libknotification-1. This is an experimental library which is not 
# supposed to stay source- or binary compatible, it may even go away in the future again.
#
# This module defines
#  LIBKNOTIFICATIONITEM-1_FOUND - whether the libkonitification-1 library was found
#  LIBKNOTIFICATIONITEM-1_LIBRARIES - the libknotification-1 library
#  LIBKNOTIFICATIONITEM-1_INCLUDE_DIR - the include path of the libknotification-1 library



find_library (LIBKNOTIFICATIONITEM-1_LIBRARY
  NAMES
  knotificationitem-1
  HINTS
  ${LIB_INSTALL_DIR}
  ${KDE4_LIB_DIR}
)

set(LIBKNOTIFICATIONITEM-1_LIBRARIES  ${LIBKNOTIFICATIONITEM-1_LIBRARY})

find_path (LIBKNOTIFICATIONITEM-1_INCLUDE_DIR
  NAMES
  knotificationitem.h
  PATH_SUFFIXES
  knotificationitem-1
  HINTS
  ${INCLUDE_INSTALL_DIR}
  ${KDE4_INCLUDE_DIR}
)



include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibKNotificationItem-1  DEFAULT_MSG  LIBKNOTIFICATIONITEM-1_LIBRARY LIBKNOTIFICATIONITEM-1_INCLUDE_DIR)

mark_as_advanced(LIBKNOTIFICATIONITEM-1_INCLUDE_DIR LIBKNOTIFICATIONITEM-1_LIBRARY)
