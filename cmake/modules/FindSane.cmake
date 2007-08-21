# cmake macro to test if we use sane
#
#  SANE_FOUND - system has SANE libs
#  SANE_INCLUDE_DIR - the SANE include directory
#  SANE_LIBRARIES - The libraries needed to use SANE

# Copyright (c) 2006, Marcus Hufgard <hufgardm@hufgard.de> 2006
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (SANE_INCLUDE_DIR AND SANE_LIBRARY)
  # Already in cache, be silent
  set(Sane_FIND_QUIETLY TRUE)
endif (SANE_INCLUDE_DIR AND SANE_LIBRARY)

FIND_PATH(SANE_INCLUDE_DIR sane/sane.h)

FIND_LIBRARY(SANE_LIBRARY NAMES  sane libsane
   PATHS
   /usr/lib/sane
   /usr/local/lib/sane
)

if (SANE_INCLUDE_DIR AND SANE_LIBRARY)
   set(SANE_FOUND TRUE)
   set(SANE_LIBRARIES ${SANE_LIBRARY})
else (SANE_INCLUDE_DIR AND SANE_LIBRARY)
   set(SANE_FOUND FALSE)
endif (SANE_INCLUDE_DIR AND SANE_LIBRARY)

if (SANE_FOUND)
   if (NOT Sane_FIND_QUIETLY)
      message(STATUS "Found sane: ${SANE_LIBRARIES}")
   endif (NOT Sane_FIND_QUIETLY)
else (SANE_FOUND)
   if (Sane_FIND_REQUIRED)
      message(FATAL_ERROR "Did not find SANE")
   endif (Sane_FIND_REQUIRED)
endif (SANE_FOUND)

MARK_AS_ADVANCED(SANE_INCLUDE_DIR SANE_LIBRARIES SANE_LIBRARY)
