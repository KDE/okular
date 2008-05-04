# cmake macro to test if libksane is installed
#
#  KSANE_FOUND - system has KSANE libs
#  KSANE_INCLUDE_DIR - the KSANE include directory
#  KSANE_LIBRARY - The library needed to use KSANE

if (KSANE_INCLUDE_DIR)
  # Already in cache, be silent
  set(KSANE_FIND_QUIETLY TRUE)
endif (KSANE_INCLUDE_DIR)

FIND_PATH(KSANE_INCLUDE_DIR libksane/ksane.h ${KDE4_INCLUDES})

FIND_LIBRARY(KSANE_LIBRARY ksane PATH ${KDE4_LIB_DIR})

if (KSANE_INCLUDE_DIR AND KSANE_LIBRARY)
   set(KSANE_FOUND TRUE)
else (KSANE_INCLUDE_DIR AND KSANE_LIBRARY)
   set(KSANE_FOUND FALSE)
endif (KSANE_INCLUDE_DIR AND KSANE_LIBRARY)

if (KSANE_FOUND)
   if (NOT KSane_FIND_QUIETLY)
      message(STATUS "Found libksane: ${KSANE_LIBRARY}")
   endif (NOT KSane_FIND_QUIETLY)
else (KSANE_FOUND)
   if (KSane_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libksane")
   endif (KSane_FIND_REQUIRED)
endif (KSANE_FOUND)

MARK_AS_ADVANCED(KSANE_INCLUDE_DIR KSANE_LIBRARY)
