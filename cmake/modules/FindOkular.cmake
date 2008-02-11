# - Find Okular
# Find the Okular core library
#
# This module defines
#  OKULAR_FOUND - whether the Okular core library was found
#  OKULAR_LIBRARIES - the Okular core library
#  OKULAR_INCLUDE_DIR - the include path of the Okular core library
#

if (OKULAR_INCLUDE_DIR AND OKULAR_LIBRARIES)

  # Already in cache
  set (OKULAR_FOUND TRUE)

else (OKULAR_INCLUDE_DIR AND OKULAR_LIBRARIES)

  find_library (OKULAR_LIBRARIES
    NAMES
    okularcore
    PATHS
    ${LIB_INSTALL_DIR}
    ${KDE4_LIB_DIR}
  )

  find_path (OKULAR_INCLUDE_DIR
    NAMES
    okular/core/document.h
    PATHS
    ${INCLUDE_INSTALL_DIR}
    ${KDE4_INCLUDE_DIR}
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Okular DEFAULT_MSG OKULAR_LIBRARIES OKULAR_INCLUDE_DIR)

endif (OKULAR_INCLUDE_DIR AND OKULAR_LIBRARIES)

mark_as_advanced(OKULAR_INCLUDE_DIR OKULAR_LIBRARIES)
